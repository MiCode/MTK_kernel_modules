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

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

#if CFG_SUPPORT_SCAN_CACHE_RESULT
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
static OS_SYSTIME getCurrentScanTime(void);

static OS_SYSTIME getLastScanTime(struct GL_SCAN_CACHE_INFO *prScanCache);

static void setLastScanTime(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime);

static uint32_t getNumberOfScanChannels(struct GL_SCAN_CACHE_INFO *prScanCache);

static u_int8_t isMediaConnected(struct GL_SCAN_CACHE_INFO *prScanCache);

static u_int8_t isScanCacheChannels(struct GL_SCAN_CACHE_INFO *prScanCache);

static u_int8_t isScanCacheTimeReady(struct GL_SCAN_CACHE_INFO *prScanCache);

static u_int8_t isScanCacheTimeOverflow(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime);

static u_int8_t doScanCache(struct GL_SCAN_CACHE_INFO *prScanCache);

static void updateScanCacheLastScanTime(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime);

static void resetScanCacheLastScanTime(struct GL_SCAN_CACHE_INFO *prScanCache);

static u_int8_t inScanCachePeriod(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime);

static u_int8_t matchScanCache(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime);

static u_int8_t matchLastScanTimeUpdate(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static OS_SYSTIME getCurrentScanTime(void)
{
	OS_SYSTIME rCurrentTime = 0;

	GET_CURRENT_SYSTIME(&rCurrentTime);
	return rCurrentTime;
}

static OS_SYSTIME getLastScanTime(struct GL_SCAN_CACHE_INFO *prScanCache)
{
	return prScanCache->u4LastScanTime;
}

static void setLastScanTime(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime)
{
	prScanCache->u4LastScanTime = rCurrentTime;
}

static uint32_t getNumberOfScanChannels(struct GL_SCAN_CACHE_INFO *prScanCache)
{
	return (uint32_t) prScanCache->n_channels;
}

static u_int8_t isMediaConnected(struct GL_SCAN_CACHE_INFO *prScanCache)
{
	return PARAM_MEDIA_STATE_CONNECTED ==
		kalGetMediaStateIndicated(prScanCache->prGlueInfo,
		prScanCache->ucBssIndex);
}

static u_int8_t isScanCacheChannels(struct GL_SCAN_CACHE_INFO *prScanCache)
{
	return getNumberOfScanChannels(prScanCache) >=
		CFG_SCAN_CACHE_MIN_CHANNEL_NUM;
}

static u_int8_t isScanCacheTimeReady(struct GL_SCAN_CACHE_INFO *prScanCache)
{
	return prScanCache->u4LastScanTime != 0;
}

static u_int8_t isScanCacheTimeOverflow(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime)
{
	return getLastScanTime(prScanCache) > rCurrentTime;
}

static u_int8_t doScanCache(struct GL_SCAN_CACHE_INFO *prScanCache)
{
	GLUE_SPIN_LOCK_DECLARATION();

	kalUpdateBssTimestamp(prScanCache->prGlueInfo);
	GLUE_ACQUIRE_SPIN_LOCK(prScanCache->prGlueInfo, SPIN_LOCK_NET_DEV);
	kalCfg80211ScanDone(prScanCache->prRequest, FALSE);
	GLUE_RELEASE_SPIN_LOCK(prScanCache->prGlueInfo, SPIN_LOCK_NET_DEV);
	return TRUE;
}

static void updateScanCacheLastScanTime(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime)
{
	setLastScanTime(prScanCache, rCurrentTime);
}

static void resetScanCacheLastScanTime(struct GL_SCAN_CACHE_INFO *prScanCache)
{
	setLastScanTime(prScanCache, 0);
}

static u_int8_t inScanCachePeriod(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime)
{
	if (isScanCacheTimeReady(prScanCache) == FALSE)
		return FALSE;

	if (isScanCacheTimeOverflow(prScanCache, rCurrentTime) == TRUE)
		return FALSE;

	if (CHECK_FOR_TIMEOUT(rCurrentTime, getLastScanTime(prScanCache),
		CFG_SCAN_CACHE_RESULT_PERIOD) == FALSE)
		return TRUE;

	return FALSE;
}

static u_int8_t matchScanCache(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime)
{
	if (isMediaConnected(prScanCache) == TRUE &&
		inScanCachePeriod(prScanCache, rCurrentTime) == TRUE &&
		isScanCacheChannels(prScanCache) == TRUE)
		return TRUE;

	return FALSE;
}

static u_int8_t matchLastScanTimeUpdate(struct GL_SCAN_CACHE_INFO *prScanCache,
	OS_SYSTIME rCurrentTime)
{
	if (isMediaConnected(prScanCache) == TRUE &&
		inScanCachePeriod(prScanCache, rCurrentTime) == FALSE &&
		isScanCacheChannels(prScanCache) == TRUE)
		return TRUE;

	return FALSE;
}

/*
 * @brief This routine is responsible for checking scan cache
 *
 * @param prScanCache - pointer of struct GL_SCAN_CACHE_INFO
 *
 * @retval TRUE: do scan cache successful
 *         FALSE: didn't report scan cache
 */
u_int8_t isScanCacheDone(struct GL_SCAN_CACHE_INFO *prScanCache)
{
	OS_SYSTIME rCurrentTime = getCurrentScanTime();

	if (matchScanCache(prScanCache, rCurrentTime) == TRUE) {
		log_limited_dbg(REQ, INFO, "SCAN_CACHE: Skip scan too frequently(%u, %u). Call cfg80211_scan_done directly\n",
			rCurrentTime,
			getLastScanTime(prScanCache));

		return doScanCache(prScanCache);
	}

	if (matchLastScanTimeUpdate(prScanCache, rCurrentTime) == TRUE) {
		log_dbg(REQ, INFO, "SCAN_CACHE: set scan cache time (%u)->(%u)\n",
			getLastScanTime(prScanCache),
			rCurrentTime);

		updateScanCacheLastScanTime(prScanCache, rCurrentTime);
	}

	if (isMediaConnected(prScanCache) == FALSE) {
		log_dbg(REQ, TRACE, "SCAN_CACHE: reset scan cache time (%u)->(0)\n",
			getLastScanTime(prScanCache));

		resetScanCacheLastScanTime(prScanCache);
	}
	return FALSE;
}
#endif /* CFG_SUPPORT_SCAN_CACHE_RESULT */
