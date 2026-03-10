/****************************************************************************
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
 ***************************************************************************/
/****************************************************************************
 *[File]             gl_kal.c
 *[Version]          v1.0
 *[Revision Date]    2019/01/01
 *[Author]
 *[Description]
 *    API implementation for os related logic. eg. threading, packet buf
 *[Copyright]
 *    Copyright (C) 2010 MediaTek Incorporation. All Rights Reserved.
 ****************************************************************************/


/*****************************************************************************
 *                         C O M P I L E R   F L A G S
 *****************************************************************************
 */

/*****************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *****************************************************************************
 */

#include "gl_os.h"

#include "precomp.h"

/*****************************************************************************
 *                              C O N S T A N T S
 *****************************************************************************
 */

/*****************************************************************************
 *                             D A T A   T Y P E S
 *****************************************************************************
 */

/*****************************************************************************
 *                            P U B L I C   D A T A
 *****************************************************************************
 */
/*
 * debug level maintain in os-related part for(?)
 * DBG_MODULE_NUM: include/debug.h
 * ENUM_WIFI_LOG_MODULE_NUM: include/wlan_oid.h
 * ENUM_WIFI_LOG_LEVEL_DEFAULT: include/wlan_oid.h
 *
 * access method in include/debug.h:
 * extern uint8_t aucDebugModule[];
 * extern uint32_t au4LogLevel[];
 */
uint8_t aucDebugModule[DBG_MODULE_NUM];
uint32_t au4LogLevel[ENUM_WIFI_LOG_MODULE_NUM] = {ENUM_WIFI_LOG_LEVEL_DEFAULT};
/*****************************************************************************
 *                           P R I V A T E   D A T A
 *****************************************************************************
 */
u_int8_t wlan_fb_power_down = FALSE;


/*****************************************************************************
 *                                 M A C R O S
 *****************************************************************************
 */

/*****************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *****************************************************************************
 */


/*****************************************************************************
 *                              F U N C T I O N S
 *****************************************************************************
 */
#if CFG_CHIP_RESET_SUPPORT
void kalRemoveProbe(struct GLUE_INFO *prGlueInfo)
{
	DBGLOG(INIT, WARN, "[SER][L0] not support..\n");
}
#endif

#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
void
kalApplyCustomRegulatory(
	const struct ieee80211_regdomain *pRegdom) { }
#endif

void kalSetLogTooMuch(uint32_t u4DriverLevel,
	uint32_t u4FwLevel)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kalGetRealTime(struct REAL_TIME *prRealTime)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kalVendorEventRssiBeyondRange(
	struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIdx, int rssi)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kalTxDirectStartCheckQTimer(
	struct GLUE_INFO *prGlueInfo,
	uint8_t offset)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

uint32_t kalGetTxDirectQueueLength(
				struct GLUE_INFO *prGlueInfo)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
#if CFG_SUPPORT_WPA3
int kalExternalAuthRequest(
	struct GLUE_INFO *prGlueInfo,
	uint8_t uBssIndex)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
int kalVendorExternalAuthRequest(struct GLUE_INFO *prGlueInfo,
		struct STA_RECORD *prStaRec, uint8_t ucBssIndex)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
#endif
#endif

void kalKfreeSkb(void *pvPacket, u_int8_t fgIsFreeData)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void *kalBuildSkb(void *pvPacket, uint32_t u4MgmtLength,
	uint32_t u4TotLen, u_int8_t fgIsSetLen)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

uint32_t kalGetSKBSharedInfoSize(void)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

#if (CFG_TX_MGMT_BY_DATA_Q == 1)
void kalSetMgmtDirectTxEvent2Hif(
		struct GLUE_INFO *pr)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
#endif

uint32_t kalGetChannelFrequency(
		uint8_t ucChannel,
		uint8_t ucBand)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

enum ENUM_BAND kalOperatingClassToBand(uint16_t u2OpClass)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

#if (CFG_SUPPORT_SINGLE_SKU == 1)
u_int8_t kalFillChannels(
	struct GLUE_INFO *prGlueInfo,
	struct CMD_DOMAIN_CHANNEL *pChBase,
	uint8_t ucChSize,
	uint8_t ucOpChannelNum,
	u_int8_t fgDisconnectUponInvalidOpChannel
)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
#endif

u_int8_t kalIsValidChnl(struct GLUE_INFO *prGlueInfo,
			uint8_t ucNumOfChannel,
			enum ENUM_BAND eBand)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

uint8_t kalGetChannelCount(struct GLUE_INFO *prGlueInfo)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void *kalGetNetDevPriv(void *prNet)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

uint32_t kalGetNetDevRxPacket(void *prNet)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}
#if CFG_SUPPORT_TDLS

void kalTdlsOpReq(
	struct GLUE_INFO *prGlueInfo,
	struct STA_RECORD *prStaRec,
	uint16_t eOpMode,
	uint16_t u2ReasonCode
	)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
#endif

#if CFG_TCP_IP_CHKSUM_OFFLOAD
void kalConfigChksumOffload(
	struct GLUE_INFO *prGlueInfo, u_int8_t fgEnable)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}
#endif

uint32_t
kalQueryPacketLength(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

void
kalSetPacketLength(void *pvPacket, uint32_t u4len)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

uint16_t
kalQueryPacketEtherType(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}


uint8_t
kalQueryPacketIPVersion(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

uint8_t
kalQueryPacketIPV4Precedence(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

uint8_t
kalQueryPacketIPv4Protocol(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

uint16_t
kalQueryPacketIPv4Identification(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}


uint16_t
kalQueryPacketIPv4TCPUDPSrcPort(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}


uint16_t
kalQueryPacketIPv4TCPUDPDstPort(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}


int
kalComparePacketIPv4UDPPayload(void *pvPacket, int8_t *pattern, size_t length)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}


void
kalUpdatePacketIPv4UDPPayload(void *pvPacket,
		uint16_t offset,
		void *pattern,
		size_t length)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kalGetPacketBuf(void *pvPacket,
				uint8_t **ppucData)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kalGetPacketBufHeadManipulate(void *pvPacket,
		uint8_t **ppucData,
		int16_t length)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kalGetPacketBufTailManipulate(void *pvPacket,
		uint8_t **ppucData,
		int16_t length)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

uint32_t kalGetPacketMark(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

u_int8_t kalProcessRadiotap(void *pvPacket,
	uint8_t **ppucData,
	uint16_t radiotap_len,
	uint16_t u2RxByteCount)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

void kalSetPacketDev(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex,
	void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void *kalGetPacketDev(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

int kal_skb_checksum_help(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

void kalSkbCopyCbData(void *pvDstPacket, void *pvSrcPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void *kal_skb_copy(void *pvPacket)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

void kal_skb_reserve(void *pvPacket, uint8_t ucLength)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_skb_split(void *pvPacket, void *pvPacket1, const uint32_t u4Length)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

uint8_t *kal_skb_push(void *pvPacket, uint32_t u4Length)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

uint8_t *kal_skb_pull(void *pvPacket, uint32_t u4Length)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

void kalWlanHardStartXmit(void *pvPacket, void *pvDev)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

uint8_t kalNlaPut(void *pvPacket, uint32_t attrType,
		uint32_t attrLen, const void *data)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void *
kalProcessRttReportDone(struct GLUE_INFO *prGlueInfo,
		uint32_t u4DataLen, uint32_t u4Count)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void *kalGetGlueNetDevHdl(struct GLUE_INFO *prGlueInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return (void *)NULL;
}

void *kalGetGlueDevHdl(struct GLUE_INFO *prGlueInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return (void *)NULL;
}

void kalGetPlatDev(void **dev)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kalClearGlueScanReq(struct GLUE_INFO *prGlueInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void *kalGetGlueScanReq(struct GLUE_INFO *prGlueInfo)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return (void *)NULL;
}

void kalGetFtIeParam(void *pvftie,
	uint16_t *pu2MDID, uint32_t *pu4IeLength,
	const uint8_t **pucIe)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

const uint8_t *kalFindIeExtIE(uint8_t eid,
				uint8_t exteid,
				const uint8_t *ies, int len)
{
	if (eid != ELEM_ID_RESERVED)
		return kalFindIeMatchMask(eid, ies, len, NULL, 0, 0, NULL);
	else
		return kalFindIeMatchMask(eid, ies, len, &exteid, 1, 2, NULL);
}

uint32_t kalSyncTimeToFW(struct ADAPTER *prAdapter,
	u_int8_t fgInitCmd)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

int32_t kalGetFwFlavor(uint8_t *flavor)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

const uint8_t *kalFindIeMatchMask(uint8_t eid,
				const uint8_t *ies, int len,
				const uint8_t *match,
				int match_len, int match_offset,
				const uint8_t *match_mask)
{
	/* match_offset can't be smaller than 2, unless match_len is
	 * zero, in which case match_offset must be zero as well.
	 */
	if (KAL_WARN_ON((match_len && match_offset < 2) ||
		(!match_len && match_offset)))
		return NULL;
	while (len >= 2 && len >= ies[1] + 2) {
		if ((ies[0] == eid) &&
			(ies[1] + 2 >= match_offset + match_len) &&
			!kalMaskMemCmp(ies + match_offset,
			match, match_mask, match_len))
			return ies;
		len -= ies[1] + 2;
		ies += ies[1] + 2;
	}
	return NULL;
}

const uint8_t *kalFindVendorIe(uint32_t oui, int type,
				const uint8_t *ies, int len)
{
	const uint8_t *ie;
	uint8_t match[] = {oui >> 16, oui >> 8, oui, type};
	int match_len = type < 0 ? 3 : sizeof(match);

	if (KAL_WARN_ON(type > 0xff))
		return NULL;

	ie = kalFindIeMatchMask(ELEM_ID_VENDOR, ies, len, match,
		match_len, 2, NULL);

	if (ie && (ie[1] < 4))
		return NULL;

	return ie;
}

int kalStrniCmp(const char *s1, const char *s2, size_t n)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

char *kalStrSep(char **stringp, const char *delim)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

size_t kalStrnLen(const char *s, size_t b)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

char *kalStrtokR(char *s, const char *delim, char **last)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

int kalFfs(int s)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

int kal_test_bit(unsigned long nr, unsigned long *addr)
{
	unsigned long mask = BIT(nr % 32);
	unsigned long *p = ((unsigned long *)addr) + (nr / 32);
	int res = 0;

	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	/* TODO: disable interrupt */

	res = mask & *p;
	/* TODO: restore interrupt */

	return res;
}

int kal_test_and_clear_bit(unsigned long bit, unsigned long *p)
{
	unsigned int res;
	unsigned long mask = 1UL << (bit & 31);

	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	p += bit >> 5;
	/* TODO: disable interrupt */
	res = *p;
	*p = res & ~mask;
	/* TODO: restore interrupt */

	return (res & mask) != 0;
}

void kal_clear_bit(unsigned long bit, unsigned long *p)
{
	unsigned long mask = 1UL << (bit & 31);

	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	p += bit >> 5;
	/* TODO: disable interrupt */
	*p &= ~mask;
	/* TOD: restore interrupt */
}

void kal_set_bit(unsigned long nr, unsigned long *addr)
{
	unsigned long mask = BIT(nr % 32);
	unsigned long *p = ((unsigned long *)addr) + (nr / 32);

	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	/* TODO: disable interrupt */
	*p |= mask;
	/* TODO: restore interrupt */
}

int kal_strtou8(const char *s, unsigned int base, uint8_t *res)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

int kal_strtou16(const char *s, unsigned int base, uint16_t *res)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

int kal_strtou32(const char *s, unsigned int base, uint32_t *res)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

int kal_strtos32(const char *s, unsigned int base, int32_t *res)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

int kal_strtoint(const char *s, unsigned int base, int *res)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

int kal_strtoul(const char *s, unsigned int base, unsigned long *res)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

int kal_scnprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int i = 0;

	va_start(args, fmt);
	/* i = vscnprintf(buf, size, fmt, args); */
	va_end(args);

	return i;
}

void *kal_kmalloc(size_t size, enum gfp_t type)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

void *kal_vmalloc(size_t size)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return NULL;
}

void kal_kfree(void *addr)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

void kal_vfree(void *addr)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

int kal_hex_dump_to_buffer(const void *buf, size_t len, int rowsize,
	int groupsize, char *linebuf, size_t linebuflen, bool ascii)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

int kalRegulatoryHint(char *country)
{
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
	return 0;
}

bool kal_warn_on(uint8_t condition)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

bool kal_is_err(void *ptr)
{
	return KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__);
}

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
u_int8_t __weak kalIsSupportMawd(void)
{
	return FALSE;
}

u_int8_t __weak kalIsSupportSdo(void)
{
	return FALSE;
}

u_int8_t __weak kalIsSupportRro(void)
{
	return FALSE;
}
#endif
