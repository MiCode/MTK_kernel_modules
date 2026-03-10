/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*
 ** Id: //Department/DaVinci/BRANCHES/
 *      MT6620_WIFI_DRIVER_V2_3/include/queue.h#1
 */

/*! \file   queue.h
 *    \brief  Definition for singly queue operations.
 *
 *    In this file we define the singly queue data structure and its
 *    queue operation MACROs.
 */

#ifndef _QUEUE_H
#define _QUEUE_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_typedef.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
/* Singly Queue Structures - Entry Part */
struct QUE_ENTRY {
	struct QUE_ENTRY *prNext;
	struct QUE_ENTRY *prPrev;	/* For Rx buffer reordering used only */
};

/* Singly Queue Structures - Queue Part */
struct QUE {
	struct QUE_ENTRY *prHead;
	struct QUE_ENTRY *prTail;
	uint32_t u4NumElem;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define MAXNUM_TDLS_PEER            4

#define QUEUE_INITIALIZE(prQueue) \
	{ \
	    (prQueue)->prHead = NULL; \
	    (prQueue)->prTail = NULL; \
	    (prQueue)->u4NumElem = 0; \
	    KAL_MB_W(); \
	}

#define QUEUE_IS_EMPTY(prQueue) (((struct QUE *)(prQueue))->prHead == NULL)

#define QUEUE_IS_NOT_EMPTY(prQueue)         ((prQueue)->u4NumElem > 0)

#define QUEUE_LENGTH(prQueue)               ((prQueue)->u4NumElem)

#define QUEUE_GET_HEAD(prQueue)             ((void *)(prQueue)->prHead)

#define QUEUE_GET_TAIL(prQueue)             ((void *)(prQueue)->prTail)

#define QUEUE_GET_NEXT_ENTRY(prQueueEntry)  \
			((void *)((struct QUE_ENTRY *)(prQueueEntry))->prNext)

#define QUEUE_ENTRY_SET_NEXT(_prQueueEntry, _prNextEntry) \
	do { \
		((struct QUE_ENTRY *)(_prQueueEntry))->prNext = \
					(struct QUE_ENTRY *)(_prNextEntry); \
		if (_prNextEntry) \
			((struct QUE_ENTRY *)(_prNextEntry))->prPrev = \
					(struct QUE_ENTRY *)(_prQueueEntry); \
	} while (0)


/**
 * prQueue (QUE) = prQueueEntry (QUE_ENTRY) + prQueue (QUE);
 */
#define QUEUE_INSERT_HEAD(prQueue, prQueueEntry) \
	do { \
		ASSERT(prQueue); \
		ASSERT(prQueueEntry); \
		((struct QUE_ENTRY *)(prQueueEntry))->prPrev = NULL; \
		((struct QUE_ENTRY *)(prQueueEntry))->prNext = \
						(prQueue)->prHead; \
		if ((prQueue)->prHead) \
			(prQueue)->prHead->prPrev = \
					(struct QUE_ENTRY *)(prQueueEntry); \
		else \
			(prQueue)->prTail = \
					(struct QUE_ENTRY *)(prQueueEntry); \
		(prQueue)->prHead = (struct QUE_ENTRY *)(prQueueEntry); \
		(prQueue)->u4NumElem++; \
		KAL_MB_W(); \
	} while (0)

/**
 * prQueue (QUE) = prQueue (QUE) + prQueueEntry (QUE_ENTRY)
 */
#define QUEUE_INSERT_TAIL(prQueue, prQueueEntry) \
	do { \
		ASSERT(prQueue); \
		ASSERT(prQueueEntry); \
		((struct QUE_ENTRY *)(prQueueEntry))->prPrev = \
						(prQueue)->prTail; \
		((struct QUE_ENTRY *)(prQueueEntry))->prNext = NULL; \
		if ((prQueue)->prTail) \
			(prQueue)->prTail->prNext = \
					(struct QUE_ENTRY *)(prQueueEntry); \
		else \
			(prQueue)->prHead = \
					(struct QUE_ENTRY *)(prQueueEntry); \
		(prQueue)->prTail = (struct QUE_ENTRY *)(prQueueEntry); \
		(prQueue)->u4NumElem++; \
		KAL_MB_W(); \
	} while (0)

/**
 * Given prQueuedEntry in prQueue, insert prInsertEntry to the position
 * right before prQueuedEntry.
 * prQueue (QUE) = prQueue (QUE) +
 *                   prInsertEntry (QUE_ENTRY) + prQueuedEntry (QUE_ENTRY) + ...
 * if prQueuedEntry == prQueue->prHead, it is equivalant to QUEUE_INSERT_HEAD()
 */
#define QUEUE_INSERT_BEFORE(prQueue, prQueuedEntry, prInsertEntry) \
	do { \
		((struct QUE_ENTRY *)(prInsertEntry))->prPrev = \
			((struct QUE_ENTRY *)(prQueuedEntry))->prPrev; \
		((struct QUE_ENTRY *)(prInsertEntry))->prNext = \
			(struct QUE_ENTRY *)(prQueuedEntry); \
		if ((struct QUE_ENTRY *)(prQueuedEntry) == (prQueue)->prHead) \
			(prQueue)->prHead = (struct QUE_ENTRY *)prInsertEntry; \
		else \
			((struct QUE_ENTRY *)(prQueuedEntry))->prPrev->prNext \
				= (struct QUE_ENTRY *)prInsertEntry; \
		((struct QUE_ENTRY *)(prQueuedEntry))->prPrev = \
				(struct QUE_ENTRY *)prInsertEntry; \
		(prQueue)->u4NumElem++; \
		KAL_MB_W(); \
	} while (0)


/**
 * prQueue (QUE) = prQueue (QUE) + prQueueEntry (QUE_ENTRY)
 * For prQueueEntry contains multiple entries not in struct QUE.
 */
#define QUEUE_INSERT_TAIL_ALL(prQueue, prQueueEntry) \
	do { \
		struct QUE_ENTRY *_entry = (struct QUE_ENTRY *)prQueueEntry; \
		struct QUE_ENTRY *_next; \
	\
		while (_entry) { \
			_next = QUEUE_GET_NEXT_ENTRY( \
					(struct QUE_ENTRY *) _entry); \
			QUEUE_INSERT_TAIL(prQueue, _entry); \
			_entry = _next; \
		} \
	} while (0)

/* NOTE: We assume the queue entry located at the beginning
 * of "prQueueEntry Type",
 * so that we can cast the queue entry to other data type without doubts.
 * And this macro also decrease the total entry count at the same time.
 * prQueueEntry (QUE_ENTRY) = (_P_TYPE) prQueue[0]
 * prQueue = prQueue[1:]
 */
#define QUEUE_REMOVE_HEAD(prQueue, prQueueEntry, _P_TYPE) \
	do { \
		ASSERT(prQueue); \
		prQueueEntry = (_P_TYPE)((prQueue)->prHead); \
		if (prQueueEntry) { \
			(prQueue)->prHead = \
				((struct QUE_ENTRY *)(prQueueEntry))->prNext; \
			if ((prQueue)->prHead) \
				(prQueue)->prHead->prPrev = NULL; \
			else \
				(prQueue)->prTail = NULL; \
			((struct QUE_ENTRY *)(prQueueEntry))->prPrev = NULL; \
			((struct QUE_ENTRY *)(prQueueEntry))->prNext = NULL; \
			(prQueue)->u4NumElem--; \
			KAL_MB_W(); \
		} \
	} while (0)

/* NOTE: We assume the queue entry located at the beginning
 * of "prQueueEntry Type",
 * so that we can cast the queue entry to other data type without doubts.
 * And this macro also decrease the total entry count at the same time.
 * prQueueEntry (QUE_ENTRY) = (_P_TYPE) prQueue[-1]
 * prQueue = prQueue[1:]
 */
#define QUEUE_REMOVE_TAIL(prQueue, prQueueEntry, _P_TYPE) \
	do { \
		ASSERT(prQueue); \
		prQueueEntry = (_P_TYPE)((prQueue)->prTail); \
		if (prQueueEntry) { \
			(prQueue)->prTail = \
				((struct QUE_ENTRY *)(prQueueEntry))->prPrev; \
			if ((prQueue)->prTail) \
				(prQueue)->prTail->prNext = NULL; \
			else \
				(prQueue)->prHead = NULL; \
			((struct QUE_ENTRY *)(prQueueEntry))->prPrev = NULL; \
			((struct QUE_ENTRY *)(prQueueEntry))->prNext = NULL; \
			(prQueue)->u4NumElem--; \
			KAL_MB_W(); \
		} \
	} while (0)

/**
 * prDestQueue (QUE) = prSrcQueue (QUE)
 * prSrcQueue = EMPTY;
 */
#define QUEUE_MOVE_ALL(prDestQueue, prSrcQueue) \
	do { \
		ASSERT(prDestQueue); \
		ASSERT(prSrcQueue); \
		*(struct QUE *)prDestQueue = *(struct QUE *)prSrcQueue; \
		QUEUE_INITIALIZE(prSrcQueue); \
	} while (0)

/**
 * prDestQueue (QUE) = prDestQueue (QUE) + prSrcQueue (QUE)
 * prSrcQueue = EMPTY;
 */
#define QUEUE_CONCATENATE_QUEUES(prDestQueue, prSrcQueue) \
	do { \
		ASSERT(prDestQueue); \
		ASSERT(prSrcQueue); \
		if ((prSrcQueue)->u4NumElem > 0) { \
			if ((prDestQueue)->prTail) { \
				(prDestQueue)->prTail->prNext = \
					(prSrcQueue)->prHead; \
				if (likely((prSrcQueue)->prHead)) \
					(prSrcQueue)->prHead->prPrev = \
						(prDestQueue)->prTail; \
			} else { \
				(prDestQueue)->prHead = (prSrcQueue)->prHead; \
			} \
			(prDestQueue)->prTail = (prSrcQueue)->prTail; \
			(prDestQueue)->u4NumElem += (prSrcQueue)->u4NumElem; \
			QUEUE_INITIALIZE(prSrcQueue); \
		} \
	} while (0)

/**
 * prDestQueue (QUE) = prSrcQueue (QUE) + prDestQueue (QUE)
 * prSrcQueue = EMPTY;
 */
#define QUEUE_CONCATENATE_QUEUES_HEAD(prDestQueue, prSrcQueue) \
	do { \
		ASSERT(prDestQueue); \
		ASSERT(prSrcQueue); \
		if ((prSrcQueue)->u4NumElem > 0 && (prSrcQueue)->prTail) { \
			(prSrcQueue)->prTail->prNext = (prDestQueue)->prHead; \
			if ((prDestQueue)->prHead) \
				(prDestQueue)->prHead->prPrev = \
							(prSrcQueue)->prTail; \
			(prDestQueue)->prHead = (prSrcQueue)->prHead; \
			if ((prDestQueue)->prTail == NULL) \
				(prDestQueue)->prTail = (prSrcQueue)->prTail; \
			(prDestQueue)->u4NumElem += (prSrcQueue)->u4NumElem; \
			QUEUE_INITIALIZE(prSrcQueue); \
		} \
	} while (0)

/*******************************************************************************
 *                            E X T E R N A L  D A T A
 *******************************************************************************
 */
extern uint8_t g_arTdlsLink[MAXNUM_TDLS_PEER];

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _QUEUE_H */
