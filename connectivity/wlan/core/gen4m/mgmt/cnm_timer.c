// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "cnm_timer.c"
 *    \brief
 *
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define DUMP_TIMER			TRUE

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static bool gDoTimeOut = FALSE;

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void cnmTimerStopTimer_impl(struct ADAPTER *prAdapter,
		struct TIMER *prTimer, u_int8_t fgAcquireSpinlock);
static u_int8_t cnmTimerIsTimerValid(struct ADAPTER *prAdapter,
		struct TIMER *prTimer);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine dump timer list for debug purpose
 *
 * \param[in]
 *
 * \retval
 *
 */
/*----------------------------------------------------------------------------*/
static void cnmTimerDumpTimer(struct ADAPTER *prAdapter)
{
#ifdef DUMP_TIMER
	struct ROOT_TIMER *prRootTimer;
	struct LINK_ENTRY *prLinkEntry;
	struct TIMER *prTimerEntry;
	struct LINK *prTimerList;
	int loopCnt = 0;

	prRootTimer = &prAdapter->rRootTimer;
	prTimerList = &prRootTimer->rLinkHead;

	log_dbg(CNM, INFO, "Current time:%u\n", kalGetTimeTick());

	LINK_FOR_EACH(prLinkEntry, prTimerList) {
		if (prLinkEntry == NULL)
			break;

		loopCnt++;
		if (loopCnt > prTimerList->u4NumElem) {
			log_dbg(CNM, WARN,
				"loopCnt=%d>[%d]\n",
				loopCnt, prTimerList->u4NumElem);
			break;
		}

		prTimerEntry = LINK_ENTRY(prLinkEntry,
			struct TIMER, rLinkEntry);

		log_dbg(CNM, INFO, "timer:%p, func:%ps, ExpiredSysTime:%u\n",
			prTimerEntry,
			prTimerEntry->pfMgmtTimeOutFunc,
			prTimerEntry->rExpiredSysTime);
	}
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to check if a timer exists in timer list.
 *
 * \param[in] prTimer The timer to check
 *
 * \retval TRUE Valid timer
 *         FALSE Invalid timer
 *
 */
/*----------------------------------------------------------------------------*/
static u_int8_t cnmTimerIsTimerValid(struct ADAPTER *prAdapter,
		struct TIMER *prTimer)
{
	struct ROOT_TIMER *prRootTimer;
	struct LINK *prTimerList;
	struct LINK_ENTRY *prLinkEntry;
	struct TIMER *prPendingTimer;

	ASSERT(prAdapter);

	prRootTimer = &prAdapter->rRootTimer;

	/* Check if the timer is in timer list */
	prTimerList = &(prAdapter->rRootTimer.rLinkHead);

	LINK_FOR_EACH(prLinkEntry, prTimerList) {
		if (prLinkEntry == NULL)
			break;

		prPendingTimer = LINK_ENTRY(prLinkEntry,
			struct TIMER, rLinkEntry);

		if (prPendingTimer == prTimer)
			return TRUE;
	}

	log_dbg(CNM, WARN, "invalid pending timer %p func %ps\n",
			prTimer, prTimer->pfMgmtTimeOutFunc);
	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set the time to do the time out check.
 *
 * \param[in] rTimeout Time out interval from current time.
 *
 * \retval TRUE Success.
 *
 */
/*----------------------------------------------------------------------------*/
static u_int8_t cnmTimerSetTimer(struct ADAPTER *prAdapter,
				OS_SYSTIME rTimeout,
				enum ENUM_TIMER_WAKELOCK_TYPE_T eType)
{
	struct ROOT_TIMER *prRootTimer;
	u_int8_t fgNeedWakeLock;

	ASSERT(prAdapter);

	/* CNM timeout is 20s, we use 19s as threshold */
	if ((uint32_t)(rTimeout) > (uint32_t)(19 * MSEC_PER_SEC))
		DBGLOG_LIMITED(CNM, INFO, "timer > 19s\n");

	prRootTimer = &prAdapter->rRootTimer;

	kalSetTimer(prAdapter->prGlueInfo, rTimeout);

	if ((eType == TIMER_WAKELOCK_REQUEST)
		|| (rTimeout <= SEC_TO_SYSTIME(WAKE_LOCK_MAX_TIME)
		&& (eType == TIMER_WAKELOCK_AUTO))) {
		fgNeedWakeLock = TRUE;

		if (!prRootTimer->fgWakeLocked) {
			KAL_WAKE_LOCK(prAdapter, prRootTimer->rWakeLock);
			prRootTimer->fgWakeLocked = TRUE;
		}
	} else {
		fgNeedWakeLock = FALSE;
	}

	return fgNeedWakeLock;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to initialize a root timer.
 *
 * \param[in] prAdapter
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerInitialize(struct ADAPTER *prAdapter)
{
	struct ROOT_TIMER *prRootTimer;
	struct LINK *prTimerList;
	struct LINK_ENTRY *prLinkEntry;
	struct TIMER *prPendingTimer;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	prRootTimer = &prAdapter->rRootTimer;

	/* Note: glue layer have configured timer */

	log_dbg(CNM, WARN, "reset timer list\n");

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

	/* Remove all pending timers */
	prTimerList = &(prAdapter->rRootTimer.rLinkHead);

	LINK_FOR_EACH(prLinkEntry, prTimerList) {
		if (prLinkEntry == NULL)
			break;

		prPendingTimer = LINK_ENTRY(prLinkEntry,
			struct TIMER, rLinkEntry);

		/* Remove timer to prevent collapsing timer structure */
		cnmTimerStopTimer_impl(prAdapter, prPendingTimer, FALSE);
	}

	LINK_INITIALIZE(&prRootTimer->rLinkHead);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

	KAL_WAKE_LOCK_INIT(prAdapter, prRootTimer->rWakeLock, "WLAN Timer");
	prRootTimer->fgWakeLocked = FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to destroy a root timer.
 *        When WIFI is off, the token shall be returned back to system.
 *
 * \param[in]
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerDestroy(struct ADAPTER *prAdapter)
{
	struct ROOT_TIMER *prRootTimer;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	prRootTimer = &prAdapter->rRootTimer;

	if (prRootTimer->fgWakeLocked) {
		KAL_WAKE_UNLOCK(prAdapter, prRootTimer->rWakeLock);
		prRootTimer->fgWakeLocked = FALSE;
	}
	KAL_WAKE_LOCK_DESTROY(prAdapter, prRootTimer->rWakeLock);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
	LINK_INITIALIZE(&prRootTimer->rLinkHead);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

	/* Note: glue layer will be responsible for timer destruction */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to initialize a timer.
 *
 * \param[in] prTimer Pointer to a timer structure.
 * \param[in] pfnFunc Pointer to the call back function.
 * \param[in] u4Data Parameter for call back function.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void
cnmTimerInitTimerOption(struct ADAPTER *prAdapter,
			struct TIMER *prTimer,
			PFN_MGMT_TIMEOUT_FUNC pfFunc,
			uintptr_t ulDataPtr,
			enum ENUM_TIMER_WAKELOCK_TYPE_T eType)
{
	struct LINK *prTimerList;
	struct LINK_ENTRY *prLinkEntry;
	struct TIMER *prPendingTimer;

	KAL_SPIN_LOCK_DECLARATION();
	ASSERT(prAdapter);

	ASSERT(prTimer);

	ASSERT((eType >= TIMER_WAKELOCK_AUTO) && (eType < TIMER_WAKELOCK_NUM));

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

	/* Remove pending timer */
	prTimerList = &(prAdapter->rRootTimer.rLinkHead);

	LINK_FOR_EACH(prLinkEntry, prTimerList) {
		if (prLinkEntry == NULL)
			break;

		prPendingTimer = LINK_ENTRY(prLinkEntry,
			struct TIMER, rLinkEntry);

		if (prPendingTimer == prTimer) {
			log_dbg(CNM, WARN, "re-init timer, timer %p func %ps\n",
				prTimer, pfFunc);

			if (timerPendingTimer(prTimer)) {
				/* Remove pending timer to prevent
				 * collapsing timer list.
				 */
				cnmTimerStopTimer_impl(prAdapter,
					prTimer, FALSE);
			}
			break;
		}
	}

	LINK_ENTRY_INITIALIZE(&prTimer->rLinkEntry);

	prTimer->pfMgmtTimeOutFunc = pfFunc;
	prTimer->ulDataPtr = ulDataPtr;
	prTimer->eType = eType;

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to stop a timer.
 *
 * \param[in] prTimer Pointer to a timer structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static void cnmTimerStopTimer_impl(struct ADAPTER *prAdapter,
	struct TIMER *prTimer, u_int8_t fgAcquireSpinlock)
{
	struct ROOT_TIMER *prRootTimer;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(prTimer);

	prRootTimer = &prAdapter->rRootTimer;

	if (fgAcquireSpinlock)
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

	if (timerPendingTimer(prTimer)) {
		LINK_REMOVE_KNOWN_ENTRY(&prRootTimer->rLinkHead,
			&prTimer->rLinkEntry);

		/* Reduce dummy timeout for power saving,
		 * especially HIF activity. If two or more timers
		 * exist and being removed timer is smallest,
		 * this dummy timeout will still happen, but it is OK.
		 */
		if (LINK_IS_EMPTY(&prRootTimer->rLinkHead)) {
			kalCancelTimer(prAdapter->prGlueInfo);

			if (fgAcquireSpinlock && prRootTimer->fgWakeLocked) {
				KAL_WAKE_UNLOCK(prAdapter,
					prRootTimer->rWakeLock);
				prRootTimer->fgWakeLocked = FALSE;
			}
		}
	}

	if (fgAcquireSpinlock)
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to stop a timer.
 *
 * \param[in] prTimer Pointer to a timer structure.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerStopTimer(struct ADAPTER *prAdapter, struct TIMER *prTimer)
{
	ASSERT(prAdapter);
	ASSERT(prTimer);

	DBGLOG_LIMITED(CNM, TRACE, "stop timer, timer %p func %ps\n",
		prTimer, prTimer->pfMgmtTimeOutFunc);

	cnmTimerStopTimer_impl(prAdapter, prTimer, TRUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to start a timer with wake_lock.
 *
 * \param[in] prTimer Pointer to a timer structure.
 * \param[in] u4TimeoutMs Timeout to issue the timer and call back function
 *                        (unit: ms).
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerStartTimer(struct ADAPTER *prAdapter, struct TIMER *prTimer,
	uint32_t u4TimeoutMs)
{
	struct ROOT_TIMER *prRootTimer;
	struct LINK *prTimerList;
	OS_SYSTIME rCurSysTime, rExpiredSysTime, rTimeoutSystime;
	OS_SYSTIME rInvalidNextExpiredSysTime;
	u_int8_t fgInvalidTime = FALSE;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(prTimer);

	if (!prTimer->pfMgmtTimeOutFunc)
		DBGLOG_LIMITED(CNM, WARN,
			"start timer, timer %p func is NULL %d ms\n",
			prTimer, u4TimeoutMs);
	else
		DBGLOG_LIMITED(CNM, TRACE,
			"start timer, timer %p func %ps %d ms\n",
			prTimer, prTimer->pfMgmtTimeOutFunc, u4TimeoutMs);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

	prRootTimer = &prAdapter->rRootTimer;
	prTimerList = &prRootTimer->rLinkHead;

	if (gDoTimeOut) {
		/* monitor the timer start in callback */
		DBGLOG_LIMITED(CNM, TRACE,
			"In DoTimeOut, timer %p func %ps %d ms timercount %d\n",
			prTimer, prTimer->pfMgmtTimeOutFunc,
			u4TimeoutMs, prTimerList->u4NumElem);
	}

	if (u4TimeoutMs > (19 * MSEC_PER_SEC))
		DBGLOG_LIMITED(CNM, INFO,
			"start timer > 19s, timer %d ms\n", u4TimeoutMs);

	/* If timeout interval is larger than 1 minute, the mod value is set
	 * to the timeout value first, then per minutue.
	 */
	if (u4TimeoutMs > MSEC_PER_MIN) {
		ASSERT(u4TimeoutMs <= ((uint32_t) 0xFFFF * MSEC_PER_MIN));

		prTimer->u2Minutes = (uint16_t) (u4TimeoutMs / MSEC_PER_MIN);
		u4TimeoutMs -= (prTimer->u2Minutes * MSEC_PER_MIN);
		if (u4TimeoutMs == 0) {
			u4TimeoutMs = MSEC_PER_MIN;
			prTimer->u2Minutes--;
		}
	} else {
		prTimer->u2Minutes = 0;
	}

	/* The assertion check if MSEC_TO_SYSTIME() may be overflow. */
	ASSERT(u4TimeoutMs < (((uint32_t) 0x80000000 - MSEC_PER_SEC) / KAL_HZ));
	rTimeoutSystime = MSEC_TO_SYSTIME(u4TimeoutMs);
	if (rTimeoutSystime == 0)
		rTimeoutSystime = 1;

	rCurSysTime = kalGetTimeTick();
	rExpiredSysTime = rCurSysTime + rTimeoutSystime;

	/* Check if root timer expired but not timeout. */
	if (TIME_BEFORE(prRootTimer->rNextExpiredSysTime, rCurSysTime) &&
		!KAL_TEST_BIT(GLUE_FLAG_TIMEOUT_BIT,
				       prAdapter->prGlueInfo->ulFlag)) {
		fgInvalidTime = TRUE;
		rInvalidNextExpiredSysTime =
			prRootTimer->rNextExpiredSysTime;
		KAL_SET_BIT(GLUE_FLAG_TIMEOUT_BIT,
				       prAdapter->prGlueInfo->ulFlag);
	}

	/* If no timer pending or the fast time interval is used. */
	if (LINK_IS_EMPTY(prTimerList)
		|| TIME_BEFORE(rExpiredSysTime,
			prRootTimer->rNextExpiredSysTime)) {

		prRootTimer->rNextExpiredSysTime = rExpiredSysTime;
		cnmTimerSetTimer(prAdapter, rTimeoutSystime, prTimer->eType);
	}

	/* Add this timer to checking list */
	prTimer->rExpiredSysTime = rExpiredSysTime;

	if (!timerPendingTimer(prTimer)) {
		LINK_INSERT_TAIL(prTimerList, &prTimer->rLinkEntry);
	} else {
		/* If the pending timer is not in timer list, we will have
		 * to add the timer to timer list anyway. Otherwise, the timer
		 * will never timeout.
		 */
		if (!cnmTimerIsTimerValid(prAdapter, prTimer))
			LINK_INSERT_TAIL(prTimerList, &prTimer->rLinkEntry);
	}

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

	if (fgInvalidTime) {
		DBGLOG_LIMITED(CNM, WARN,
			"Invalid NextExpiredSysTime: %u, currentSysTime: %u\n",
			rInvalidNextExpiredSysTime, rCurSysTime);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routines is called to check the timer list.
 *
 * \param[in]
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void cnmTimerDoTimeOutCheck(struct ADAPTER *prAdapter)
{
	struct ROOT_TIMER *prRootTimer;
	struct LINK *prTimerList;
	struct LINK_ENTRY *prLinkEntry;
	struct TIMER *prTimer;
	OS_SYSTIME rCurSysTime;
	PFN_MGMT_TIMEOUT_FUNC pfMgmtTimeOutFunc;
	uintptr_t ulTimeoutDataPtr;
	u_int8_t fgNeedWakeLock;
	enum ENUM_TIMER_WAKELOCK_TYPE_T eType = TIMER_WAKELOCK_NONE;
	int loopCnt = 0;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	/* acquire spin lock */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);

	prRootTimer = &prAdapter->rRootTimer;
	prTimerList = &prRootTimer->rLinkHead;

	rCurSysTime = kalGetTimeTick();

	/* Set the permitted max timeout value for new one */
	prRootTimer->rNextExpiredSysTime
		= rCurSysTime + MGMT_MAX_TIMEOUT_INTERVAL;

	log_dbg(CNM, TRACE, "loop start [%d]\n", prTimerList->u4NumElem);
	gDoTimeOut = TRUE;

	LINK_FOR_EACH(prLinkEntry, prTimerList) {
		if (prLinkEntry == NULL)
			break;

		loopCnt++;
		if (loopCnt > prTimerList->u4NumElem) {
			log_dbg(CNM, WARN,
				"loopCnt=%d>[%d]\n",
				loopCnt, prTimerList->u4NumElem);
			cnmTimerDumpTimer(prAdapter);
			break;
		}

		prTimer = LINK_ENTRY(prLinkEntry, struct TIMER, rLinkEntry);
		ASSERT(prTimer);

		/* Check if this entry is timeout. */
		if (!TIME_BEFORE(rCurSysTime, prTimer->rExpiredSysTime)) {
			if (timerPendingTimer(prTimer)) {
				cnmTimerStopTimer_impl(prAdapter,
					prTimer, FALSE);

				pfMgmtTimeOutFunc = prTimer->pfMgmtTimeOutFunc;
				ulTimeoutDataPtr = prTimer->ulDataPtr;

				if (prTimer->u2Minutes > 0) {
					prTimer->u2Minutes--;
					prTimer->rExpiredSysTime = rCurSysTime +
						MSEC_TO_SYSTIME(MSEC_PER_MIN) -
						(rCurSysTime -
						prTimer->rExpiredSysTime);
					LINK_INSERT_TAIL(prTimerList,
						&prTimer->rLinkEntry);
				} else if (pfMgmtTimeOutFunc) {
					KAL_RELEASE_SPIN_LOCK(prAdapter,
						SPIN_LOCK_TIMER);
				#ifdef UT_TEST_MODE
				if (testTimerTimeout(prAdapter,
						     pfMgmtTimeOutFunc,
						     ulTimeoutDataPtr))
				#endif
				log_dbg(CNM, TRACE,
					"timer timeout, timer %p func %ps\n",
					prTimer, prTimer->pfMgmtTimeOutFunc);

					(pfMgmtTimeOutFunc) (prAdapter,
						ulTimeoutDataPtr);
					KAL_ACQUIRE_SPIN_LOCK(prAdapter,
						SPIN_LOCK_TIMER);
				}
			} else {
				log_dbg(CNM, WARN,
					"timer re-inited, timer %p func %ps\n",
					prTimer, prTimer->pfMgmtTimeOutFunc);
				cnmTimerDumpTimer(prAdapter);
				break;
			}

			/* Search entire list again because of nest del and add
			 * timers and current MGMT_TIMER could be volatile after
			 * stopped
			 */
			prLinkEntry = (struct LINK_ENTRY *) prTimerList;
			if (prLinkEntry == NULL)
				break;
			loopCnt = 0;

			prRootTimer->rNextExpiredSysTime
				= rCurSysTime + MGMT_MAX_TIMEOUT_INTERVAL;
		} else if (TIME_BEFORE(prTimer->rExpiredSysTime,
			prRootTimer->rNextExpiredSysTime)) {
			prRootTimer->rNextExpiredSysTime
				= prTimer->rExpiredSysTime;

			if (prTimer->eType == TIMER_WAKELOCK_REQUEST)
				eType = TIMER_WAKELOCK_REQUEST;
			else if ((eType != TIMER_WAKELOCK_REQUEST)
				&& (prTimer->eType == TIMER_WAKELOCK_AUTO))
				eType = TIMER_WAKELOCK_AUTO;
		}
	}	/* end of for loop */

	log_dbg(CNM, TRACE, "loop end");
	gDoTimeOut = false;

	/* Setup the prNext timeout event. It is possible the timer was already
	 * set in the above timeout callback function.
	 */
	fgNeedWakeLock = FALSE;
	if (!LINK_IS_EMPTY(prTimerList)) {
		ASSERT(TIME_AFTER(
			prRootTimer->rNextExpiredSysTime, rCurSysTime));

		fgNeedWakeLock = cnmTimerSetTimer(prAdapter,
			(OS_SYSTIME)((int32_t) prRootTimer->rNextExpiredSysTime
				- (int32_t) rCurSysTime),
			eType);
	}

	if (prRootTimer->fgWakeLocked && !fgNeedWakeLock) {
		KAL_WAKE_UNLOCK(prAdapter, prRootTimer->rWakeLock);
		prRootTimer->fgWakeLocked = FALSE;
	}

	/* release spin lock */
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TIMER);
}
