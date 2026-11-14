/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _NANRESCHEDULER_H_
#define _NANRESCHEDULER_H_

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#if CFG_SUPPORT_NAN
/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

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

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */


/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */


void nanRescheduleInit(struct ADAPTER *prAdapter);
void nanRescheduleDeInit(struct ADAPTER *prAdapter);

/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is used to
 * 1) check if rescheduling is needed and
 * 2) proceed rescheduling
 *
 * \param[in] prAdapter
 * \param[in] event : event origin to trigger this reschedule check
 * \param[in] prNDL : NULL if event is AIS_CONNECTED or AIS_DISCONNECTED
 *                    pointer to the (newly connected / disconnected) NDL
 *                    if event is NEW_NDL or REMOVE_NDL
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void nanRescheduleNdlIfNeeded(struct ADAPTER *prAdapter,
	enum RESCHEDULE_SOURCE,
	struct _NAN_NDL_INSTANCE_T *prNDL);
void nanRescheduleEnqueueNewToken(struct ADAPTER *prAdapter,
	enum RESCHEDULE_SOURCE event,
	struct _NAN_NDL_INSTANCE_T *prNDL);

uint32_t ReleaseNanSlotsForSchedulePrep(struct ADAPTER *prAdapter,
				const enum RESCHEDULE_SOURCE event,
				u_int8_t fgIsEhtRescheduleNewNDL);

unsigned char nanReschedIsOngoing(struct ADAPTER *prAdapter);

uint32_t nanReschedGetNdlNdc(struct ADAPTER *prAdapter);

#endif /* CFG_SUPPORT_NAN */

void nanResumeRescheduleTimeout(struct ADAPTER *prAdapter, uintptr_t ulParam);
#endif /* _NANRESCHEDULER_H_ */
