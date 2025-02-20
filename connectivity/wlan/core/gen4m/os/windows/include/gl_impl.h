/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_impl.h
 * \brief  List the os-dependent structure/API that need to implement
 * to align with common part
 *
 * not to modify *.c while put current used linux-type strucutre/API here
 * For porting to new OS, the API listed in this file needs to be
 * implemented
 */
#ifndef _GL_IMPL_H
#define _GL_IMPL_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "config.h"
#include "gl_os.h"
#include "gl_typedef.h"
#include "gl_wext_priv.h"
#include "link_drv.h"
#include "nic/mac.h"
#include "nic/wlan_def.h"
#include "wlan_lib.h"
#include "wlan_oid.h"
#include "debug.h"
#if CFG_ENABLE_BT_OVER_WIFI
#include "nic/bow.h"
#endif
#include <intrin.h>

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

#define TYPEOF(__F)

uint32_t __KAL_INLINE__ kalGetFwVerOffsetAddr(void)
{
	DBGLOG(SW4, WARN, "NO firmware version build.\n");
	return 0;
}

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
uint8_t  __KAL_INLINE__ kalIsSupportMawd(void)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

uint8_t  __KAL_INLINE__ kalIsSupportSdo(void)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

uint8_t  __KAL_INLINE__ kalIsSupportRro(void)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

uint32_t  __KAL_INLINE__ kalGetMawdVer(void)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

uint32_t  __KAL_INLINE__ kalGetConnInfraId(void)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}
#endif

void __KAL_INLINE__ kalGetDev(void **dev)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

int32_t __KAL_INLINE__ kalGetFwFlavor(uint8_t *flavor)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

/*----------------------------------------------------------------------------*/
/* SCAN                                                                      */
/*----------------------------------------------------------------------------*/
void *kalGetGlueSchedScanReq(struct GLUE_INFO *prGlueInfo);
void kalClearGlueSchedScanReq(struct GLUE_INFO *prGlueInfo);

/*----------------------------------------------------------------------------*/
/* RX                                                                         */
/*----------------------------------------------------------------------------*/
#ifdef CFG_REMIND_IMPLEMENT
#define kalSetSerIntEvent(_pr) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _pr)
#else
void kalSetSerIntEvent(struct GLUE_INFO *pr);
#endif

#endif /* _GL_IMPL_H */
