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
 *						C O M P I L E R	 F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *            E X T E R N A L	R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *						C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *            D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *            M A C R O   D E C L A R A T I O N S
 *******************************************************************************
 */
#include <linux/rtc.h>

#if (CFG_SUPPORT_STATISTICS == 1)
#define STATS_RX_PKT_INFO_DISPLAY			StatsRxPktInfoDisplay
#define STATS_TX_PKT_INFO_DISPLAY			StatsTxPktInfoDisplay
#else
#define STATS_RX_PKT_INFO_DISPLAY
#define STATS_TX_PKT_INFO_DISPLAY
#endif /* CFG_SUPPORT_STATISTICS */

/*******************************************************************************
 *            F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *						P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *						P R I V A T E  F U N C T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *						P U B L I C  F U N C T I O N S
 *******************************************************************************
 */

#define STATS_TX_TIME_ARRIVE(__Skb__)	\
do { \
	uint64_t __SysTime; \
	__SysTime = StatsEnvTimeGet(); /* us */	\
	GLUE_SET_PKT_XTIME(__Skb__, __SysTime);	\
} while (FALSE)

uint64_t StatsEnvTimeGet(void);

void StatsEnvTxTime2Hif(IN struct ADAPTER *prAdapter,
			IN struct MSDU_INFO *prMsduInfo);

void StatsEnvRxTime2Host(IN struct ADAPTER *prAdapter,
			 struct sk_buff *prSkb);

void StatsRxPktInfoDisplay(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb);

void StatsTxPktInfoDisplay(uint8_t *pPkt);

void StatsResetTxRx(void);

void StatsEnvSetPktDelay(IN uint8_t ucTxOrRx,
			 IN uint8_t ucIpProto, IN uint16_t u2UdpPort,
			 uint32_t u4DelayThreshold);

void StatsEnvGetPktDelay(OUT uint8_t *pucTxRxFlag,
			 OUT uint8_t *pucTxIpProto, OUT uint16_t *pu2TxUdpPort,
			 OUT uint32_t *pu4TxDelayThreshold,
			 OUT uint8_t *pucRxIpProto,
			 OUT uint16_t *pu2RxUdpPort,
			 OUT uint32_t *pu4RxDelayThreshold);
/* End of stats.h */
