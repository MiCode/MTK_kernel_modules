/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
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

#define QUEUE_IS_EMPTY(prQueue) \
	((((struct QUE *)(prQueue))->prHead == (struct QUE_ENTRY *)NULL) || \
	((prQueue)->u4NumElem == 0))

#define QUEUE_IS_NOT_EMPTY(prQueue) \
	((((struct QUE *)(prQueue))->prHead != (struct QUE_ENTRY *)NULL) && \
	((prQueue)->u4NumElem > 0))

#define QUEUE_LENGTH(prQueue)               ((prQueue)->u4NumElem)

#define QUEUE_GET_HEAD(prQueue)             ((void *)(prQueue)->prHead)

#define QUEUE_GET_TAIL(prQueue)             ((void *)(prQueue)->prTail)

#define QUEUE_GET_NEXT_ENTRY(prQueueEntry)  \
			((void *)((struct QUE_ENTRY *)(prQueueEntry))->prNext)

#if CFG_QUEUE_DEBUG
#define QUEUE_ADD_VALIDATE(prQue, prEntry) \
	do { \
		struct QUE_ENTRY *_prEntry = (struct QUE_ENTRY *)prEntry; \
		\
		if ((_prEntry == (prQue)->prHead || \
			_prEntry == (prQue)->prTail)) { \
			DBGLOG(QM, ERROR, \
				"double add: new:%p, head:%p, tail:%p\n", \
				_prEntry, (prQue)->prHead, \
				(prQue)->prTail); \
			ASSERT_QUEUE_DEBUG(); \
		} \
	} while (0)
#define QUEUE_ADD_BEFORE_VALIDATE(prQue, prEntry, prInsertEntry) \
	do { \
		if (prEntry == prInsertEntry) { \
			DBGLOG(QM, ERROR, \
				"double add before: entry:%p, new:%p\n", \
				prEntry, prInsertEntry); \
			ASSERT_QUEUE_DEBUG(); \
			break; \
		} \
		QUEUE_ADD_VALIDATE(prQue, prInsertEntry); \
	} while (0)
#define QUEUE_CONCAT_VALIDATE(prDestQue, prSrcQue) \
	do { \
		if ((prDestQue)->prHead == (prSrcQue)->prHead || \
			(prDestQue)->prHead == (prSrcQue)->prTail || \
			(prDestQue)->prTail == (prSrcQue)->prHead || \
			(prDestQue)->prTail == (prSrcQue)->prTail) { \
			DBGLOG(QM, ERROR, \
				"concat loop: dest(%p,%p) src(%p,%p)\n", \
				(prDestQue)->prHead, (prDestQue)->prTail, \
				(prSrcQue)->prHead, (prSrcQue)->prTail); \
			ASSERT_QUEUE_DEBUG(); \
		} \
	} while (0)
#else /* CFG_QUEUE_DEBUG */
#define QUEUE_ADD_VALIDATE(prQue, prEntry)
#define QUEUE_ADD_BEFORE_VALIDATE(prQue, prEntry, prInsertEntry)
#define QUEUE_CONCAT_VALIDATE(prDestQue, prSrcQue)
#endif /* CFG_QUEUE_DEBUG */

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
		QUEUE_ADD_VALIDATE(prQueue, prQueueEntry); \
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
		QUEUE_ADD_VALIDATE(prQueue, prQueueEntry); \
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
		QUEUE_ADD_BEFORE_VALIDATE(prQueue, prQueuedEntry, \
			prInsertEntry); \
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
		prQueueEntry = NULL; \
		if ((prQueue)->u4NumElem == 0) \
			break; \
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
		prQueueEntry = NULL; \
		if ((prQueue)->u4NumElem == 0) \
			break; \
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
			QUEUE_CONCAT_VALIDATE(prDestQueue, prSrcQueue); \
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
			QUEUE_CONCAT_VALIDATE(prDestQueue, prSrcQueue); \
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
