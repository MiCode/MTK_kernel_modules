// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#if (CFG_SUPPORT_NAN == 1)

#include "precomp.h"

#define REG_2G_5G_MAX_SUPPORT_CHANNEL 13
#define REG_6G_MAX_SUPPORT_CHANNEL 59

/* Table E4 - Global Operating Classes */
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
#define REG_MAX_SUPPORT_CHANNEL		REG_6G_MAX_SUPPORT_CHANNEL
#else
#define REG_MAX_SUPPORT_CHANNEL		REG_2G_5G_MAX_SUPPORT_CHANNEL
#endif

struct _NAN_CHNL_REG_INFO_T {
	uint8_t ucOperatingClass;

	uint16_t u2Bw;

	enum ENUM_CHNL_EXT eSco;
	/* For 40M Bw to determine SCB or SCA */

	uint8_t aucSupportChnlList[REG_MAX_SUPPORT_CHANNEL];
	/* BW80, BW160=> Center Channel
	 * BW20, BW40=> Primary Channel
	 */
};

/*******************************************
 * Table E4 - Global Operating Classes
 *******************************************
 */
struct _NAN_CHNL_REG_INFO_T g_rNanRegInfo[] = {
	{81, 20, CHNL_EXT_SCN, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, } },
	{82, 20, CHNL_EXT_SCN, {14, } }, /* 1 */
	{83, 40, CHNL_EXT_SCA, {1, 2, 3, 4, 5, 6, 7, 8, 9, } }, /* 2 */
	{84, 40, CHNL_EXT_SCB, {5, 6, 7, 8, 9, 10, 11, 12, 13, } }, /* 3 */
	{115, 20, CHNL_EXT_SCN, {36, 40, 44, 48, } }, /* 4 */
	{116, 40, CHNL_EXT_SCA, {36, 44, } }, /* 5 */
	{117, 40, CHNL_EXT_SCB, {40, 48, } }, /* 6 */
	{118, 20, CHNL_EXT_SCN, {52, 56, 60, 64, } }, /* 7 */
	{119, 40, CHNL_EXT_SCA, {52, 60, } }, /* 8 */
	{120, 40, CHNL_EXT_SCB, {56, 64, } }, /* 9 */
	{121, 20, CHNL_EXT_SCN,
		{100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144, }
	}, /* 10 */
	{122, 40, CHNL_EXT_SCA, {100, 108, 116, 124, 132, 140, } }, /* 11 */
	{123, 40, CHNL_EXT_SCB, {104, 112, 120, 128, 136, 144, } }, /* 12 */
	{124, 20, CHNL_EXT_SCN, {149, 153, 157, 161, } }, /* 13 */
	{125, 20, CHNL_EXT_SCN, {149, 153, 157, 161, 165, 169, /* 14 */
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
				177,
#endif
				}
	}, /* 15 */
	{126, 40, CHNL_EXT_SCA, {149, 157, 165,
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
				173,
#endif
				}
	}, /* 16 */
	{127, 40, CHNL_EXT_SCB, {153, 161, 169,
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
				177,
#endif

				}
	}, /* 17 */
	{128, 80, CHNL_EXT_SCN, {42, 58, 106, 122, 138, 155,
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
				171,
#endif
				}
	}, /* 18 */ /* center channel list */
	{129, 160, CHNL_EXT_SCN, {50, 114,
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
				163
#endif
				}
	}, /* 19 */  /* center channel list */
	{130, 80, CHNL_EXT_SCN, {42, 58, 106, 122, 138, 155,
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
				171,
#endif
				}
	}, /* 20 */ /* center channel list */

#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
	{131, 20, CHNL_EXT_SCN,
		{ 1,   5,   9,  13,  17,  21,  25,  29,  33,  37,
		 41,  45,  49,  53,  57,  61,  65,  69,  73,  77,
		 81,  85,  89,  93,  97, 101, 105, 109, 113, 117,
		121, 125, 129, 133, 137, 141, 145, 149, 153, 157,
		161, 165, 169, 173, 177, 181, 185, 189, 193, 197,
		201, 205, 209, 213, 217, 221, 225, 229, 233}
	}, /* 21 *//* 6G BW20 => Channel set */
	{132, 40, CHNL_EXT_SCN,
		{ 3,  11,  19,  27,  35,  43,  51,  59,  67,  75,
		 83,  91,  99, 107, 115, 123, 131, 139, 147, 155,
		163, 171, 179, 187, 195, 203, 211, 219, 227, }
		/* 6G BW40 => Center Channel */
	}, /* 22 */
	{133, 80, CHNL_EXT_SCN,
		{ 7,  23,  39,  55,  71,  87, 103, 119, 135, 151,
		167, 183, 199, 215, }
		/* 6G BW80 => Center Channel */
	}, /* 23 */
	{ 134, 160, CHNL_EXT_SCN, { 15, 47, 79, 111, 143, 175, 207, }
		/* 6G BW160 => Center Channel */
	}, /* 24 */
	{135, 80, CHNL_EXT_SCN,
		{ 7,  23,  39,  55, 71, 87, 103, 119, 135, 151,
		167, 183, 199, 215, }
		/* 6G BW80 => Center Channel */
	}, /* 25 */
	{136, 20, CHNL_EXT_SCN, { 2, } /* 6G BW20 => Channel set */ }, /* 26 */
	{137, 320, CHNL_EXT_SCN, { 31, 95, 159 } },
#endif
};

#define REG_DB_ENTRY_NOT_FOUND ARRAY_SIZE(g_rNanRegInfo)

u_int8_t g_fgNanUseR4AvailAttr;

/*******************************************
 * Table E4 - Global Operating Classes
 *******************************************
 */
/* Return index of matched entry; REG_DB_ENTRY_NOT_FOUND if not found. */
uint8_t nanRegFindRecordIdx(uint8_t ucOperatingClass)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(g_rNanRegInfo); i++) {
		if (ucOperatingClass == g_rNanRegInfo[i].ucOperatingClass)
			break;
	}

	return i;
}

/* NAN 4.0 Spec, Table 101. Setting for Primary Channel Bitmap */
uint8_t nanRegGet20MHzPrimaryChnlIndex(uint8_t ucOperatingClass,
				       uint8_t ucPriChnlBitmap)
{
	int32_t i4Idx = 0;

	/* In the case of Channel Set = 155 / 42 with bitmap = 0x0f,
	 * prefer bit0 for ch149 / ch 36
	 */

	if (IS_6G_OP_CLASS(ucOperatingClass)) {
		if (g_fgNanUseR4AvailAttr)
			DBGLOG(NAN, WARN, "FIXME, OC=%u, PriChnlBitmap=0x%02x",
			       ucOperatingClass, ucPriChnlBitmap);
		else
			i4Idx = 1; /* channel 5, 101 */
	}

	for ( ; i4Idx < 8; i4Idx++) {
		if (ucPriChnlBitmap & BIT(i4Idx))
			return i4Idx;
	}

	return 0;
}

uint8_t nanRegGetChannelByOrder(uint8_t ucOperatingClass,
				uint16_t *pu2ChnlBitmap)
{
	uint32_t i;
	uint32_t j;
	uint32_t u4MaxChnlBitmap = REG_2G_5G_MAX_SUPPORT_CHANNEL;
	uint8_t *pucBuf = (uint8_t *)pu2ChnlBitmap;
#if (CFG_SUPPORT_NAN_6G == 1)
	uint8_t aucSupportChnlList[ALIGN_8(REG_MAX_SUPPORT_CHANNEL) / 8];
	uint8_t uc6gStartChnl = 0;
	uint8_t uc6gChnlNum = 0;
#endif

#if (CFG_SUPPORT_NAN_6G == 1)
	if (IS_6G_OP_CLASS(ucOperatingClass)) {
		if (g_fgNanUseR4AvailAttr) {
			kalMemZero(aucSupportChnlList,
				   sizeof(aucSupportChnlList));
			uc6gStartChnl = pucBuf[0];
			uc6gChnlNum = pucBuf[1];

			nanRegConvert6gChannelBitmap(ucOperatingClass,
						     pu2ChnlBitmap,
						     aucSupportChnlList);
			pucBuf = aucSupportChnlList;
		}

		u4MaxChnlBitmap = REG_6G_MAX_SUPPORT_CHANNEL;
	}
#endif

	i = nanRegFindRecordIdx(ucOperatingClass);
	if (i == REG_DB_ENTRY_NOT_FOUND)
		return REG_INVALID_INFO;

	for (j = 0; j < u4MaxChnlBitmap; j++) {

		if (g_rNanRegInfo[i].aucSupportChnlList[j] == 0)
			break; /* end of item in table */

		if ((pucBuf[j / 8] & BIT(j % 8)) == 0)
			continue;

		/* Clear rightmost bit for each function invocation */
		pucBuf[j / 8] &= ~BIT(j % 8);

#if (CFG_SUPPORT_NAN_6G == 1)
		if (IS_6G_OP_CLASS(ucOperatingClass) && g_fgNanUseR4AvailAttr) {
			uint8_t nxt = 0;

			if (j < u4MaxChnlBitmap - 1)
				nxt = g_rNanRegInfo[i].aucSupportChnlList[j+1];

			if (nxt != 0 && uc6gChnlNum > 1) {
				uc6gStartChnl = nxt;
				uc6gChnlNum--;
				*pu2ChnlBitmap =
					uc6gChnlNum << 8 | uc6gStartChnl;
				DBGLOG(NAN, LOUD,
					"New 6g s:%02x, c:%02x, %02x\n",
					uc6gChnlNum,
					uc6gStartChnl,
					*pu2ChnlBitmap);
			}
		}
#endif

		return g_rNanRegInfo[i].aucSupportChnlList[j];
	}

	return REG_INVALID_INFO;
}

uint32_t
nanRegGetChannelBitmap(uint8_t ucOperatingClass, uint8_t ucChannel,
		       uint16_t *pu2ChnlBitmap)
{
	uint8_t i, j;
	uint8_t *pucBuf;

	pucBuf = (uint8_t *)pu2ChnlBitmap;
	i = nanRegFindRecordIdx(ucOperatingClass);

#if (CFG_SUPPORT_NAN_6G == 1)
	if (IS_6G_OP_CLASS(ucOperatingClass) && g_fgNanUseR4AvailAttr) {
		pucBuf[0] = ucChannel;
		pucBuf[1] = 1;
		return WLAN_STATUS_SUCCESS;
	}
#endif

	if (i != REG_DB_ENTRY_NOT_FOUND) {
		for (j = 0; j < REG_MAX_SUPPORT_CHANNEL; j++) {
			if (g_rNanRegInfo[i].aucSupportChnlList[j] == ucChannel)
				pucBuf[j / 8] |= BIT(j % 8);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

uint16_t
nanRegGetBw(uint8_t ucOperatingClass)
{
	int i;

	i = nanRegFindRecordIdx(ucOperatingClass);
	if (i != REG_DB_ENTRY_NOT_FOUND)
		return g_rNanRegInfo[i].u2Bw;

	return REG_INVALID_INFO;
}

enum ENUM_CHNL_EXT
nanRegGetSco(uint8_t ucOperatingClass)
{
	int i;

	i = nanRegFindRecordIdx(ucOperatingClass);
	if (i != REG_DB_ENTRY_NOT_FOUND)
		return g_rNanRegInfo[i].eSco;

	return REG_INVALID_INFO;
}

uint8_t
nanRegGetPrimaryChannel(uint8_t ucChannel, uint16_t u2Bw, uint8_t ucNonContBw,
			uint8_t ucPriChnlIdx, uint8_t ucOperatingClass)
{
	uint8_t ucIs6gChnl = IS_6G_OP_CLASS(ucOperatingClass);

	if ((u2Bw == 20) || ((u2Bw == 40) && !ucIs6gChnl))
		return ucChannel;
	else if ((u2Bw == 160) && (ucNonContBw == 0))
		ucChannel = ucChannel - 14 + (ucPriChnlIdx * 4);
	else if ((u2Bw == 320) && (ucNonContBw == 0))
		ucChannel = ucChannel - 30 + (ucPriChnlIdx * 4);
	else if ((u2Bw == 40) && ucIs6gChnl)
		ucChannel = ucChannel - 2 + (ucPriChnlIdx * 4);
	else
		ucChannel = ucChannel - 6 + (ucPriChnlIdx * 4);

	return ucChannel;
}

/**
 * nanRegGetPrimaryChannelByOrder() - Get one available Primary Channel
 * @ucOperatingClass: Operating Class defined in 802.11 specification E4
 * @u2ChnlBitmap: channel bitmap defined in Wi-Fi Aware R4 Table 100
 * @ucNonContBw: Non-contiguous bandwidth flag in Wi-Fi Aware R4 Table
 * @ucPriChnlBitmap: primary channel bitmap defined in Wi-Fi Aware R4 Table 100
 *
 * Longer description
 *
 * Channel bitmep:
 *   if Operating Class less than 131:
 *	Bit i of the Channel Bitmap is set to 1 when the i-th Channel, in
 *	increasing numberical oerder, of the possible channels within the
 *	Operating Class is selected, and set to 0 otherwise
 *   else:
 *	Bits[7:0]: start channel number (indicated by channel center freq index
 *	column/Channel Set column) within the Operating Class selected
 *	Bits[15:8]: Number of channels including start channel number indicated
 *	by bits[7:0] and following channels (indicated in channel center
 *	frequency index column/Channel set column) within the Operating Class
 *	selected
 * Primary channel bitmap:
 *   if exactly one bit is set in the Channel Bitmap subfield, then this field
 *   indicates the set of selected preferered primary channels. It is reserved
 *   otherwise. The detailed settings of Primary Channel Bitmap is shown in
 *   Table 101.
 *
 * Context:
 * This function is expected to called repeatedly until ChnlBitmap is empty
 *
 * Return: Primary channel of the LSB in pu2ChnlBitmap, with ucOperatingClass.
 *	   if all bits in u2ChnlBitmap has been cleared,
 *	   return REG_INVALID_INFO to end the loop in the caller.
 */
uint8_t nanRegGetPrimaryChannelByOrder(uint8_t ucOperatingClass,
				       uint16_t *pu2ChnlBitmap,
				       uint8_t ucNonContBw,
				       uint8_t ucPriChnlBitmap)
{
	uint32_t i, j;
	uint32_t u4MaxChnlBitmap = REG_2G_5G_MAX_SUPPORT_CHANNEL;
	uint8_t *pucBuf = (uint8_t *)pu2ChnlBitmap;
#if (CFG_SUPPORT_NAN_6G == 1)
	uint8_t aucSupportChnlList[ALIGN_8(REG_MAX_SUPPORT_CHANNEL) / 8];
	uint8_t uc6gStartChnl = 0;
	uint8_t uc6gChnlNum = 0;
#endif

#if (CFG_SUPPORT_NAN_6G == 1)
	if (IS_6G_OP_CLASS(ucOperatingClass) && g_fgNanUseR4AvailAttr) {
		kalMemZero(aucSupportChnlList, sizeof(aucSupportChnlList));
		uc6gStartChnl = pucBuf[0];
		uc6gChnlNum = pucBuf[1];

		nanRegConvert6gChannelBitmap(ucOperatingClass, pu2ChnlBitmap,
					     aucSupportChnlList);
		pucBuf = aucSupportChnlList;
		u4MaxChnlBitmap = REG_6G_MAX_SUPPORT_CHANNEL;

		nanUtilDump(NULL, "[6g Prim Chnl Map]",
			    pucBuf, sizeof(aucSupportChnlList));
	}
#endif

	i = nanRegFindRecordIdx(ucOperatingClass);
	if (i == REG_DB_ENTRY_NOT_FOUND)
		return REG_INVALID_INFO;

	DBGLOG(NAN, LOUD, "find oc=%u, i=%u, u2ChnlBitmap=0x%04x",
	       ucOperatingClass, i, *pu2ChnlBitmap);

	for (j = 0; j < u4MaxChnlBitmap; j++) {

		if (g_rNanRegInfo[i].aucSupportChnlList[j] == 0)
			break; /* end of item in table */

		if ((pucBuf[j / 8] & BIT(j % 8)) == 0)
			continue;

		/* Clear rightmost bit for each function invocation */
		pucBuf[j / 8] &= ~BIT(j % 8);

#if (CFG_SUPPORT_NAN_6G == 1)
		/*
		 * Because there's a while loop in caller,
		 * here should clear bitmap once the channel has been selected
		 * Save the updated bitmap pointed by pu2ChnlBitmap.
		 */
		if (IS_6G_OP_CLASS(ucOperatingClass) && g_fgNanUseR4AvailAttr) {
			uint8_t nxt = 0;

			if (j < u4MaxChnlBitmap - 1)
				nxt = g_rNanRegInfo[i].aucSupportChnlList[j+1];

			if (nxt != 0 && uc6gChnlNum > 1) {
				uc6gStartChnl = nxt;
				uc6gChnlNum--;
				*pu2ChnlBitmap =
					uc6gChnlNum << 8 | uc6gStartChnl;
			} else {
				*pu2ChnlBitmap = 0;
			}
			DBGLOG(NAN, LOUD,
				"ReNew 6g chnl s:%d, c:%d, 0x%04x\n",
				uc6gStartChnl, uc6gChnlNum, *pu2ChnlBitmap);
		}
#endif

		return nanRegGetPrimaryChannel(
				g_rNanRegInfo[i].aucSupportChnlList[j],
				nanRegGetBw(ucOperatingClass),
				ucNonContBw,
				nanRegGet20MHzPrimaryChnlIndex(ucOperatingClass,
							       ucPriChnlBitmap),
				ucOperatingClass);
	}

	return REG_INVALID_INFO;
}

uint8_t nanRegGetCenterChnlByPriChnl(uint8_t ucOperatingClass,
				     uint8_t ucPrimaryChnl)
{
	uint32_t i;
	uint32_t j = 0;
	uint16_t u2Bw;
	uint8_t ucRang = 0;
	uint8_t ucCenterChnl;
	uint8_t ucChnl;

	ucCenterChnl = REG_INVALID_INFO;

	i = nanRegFindRecordIdx(ucOperatingClass);
	if (i != REG_DB_ENTRY_NOT_FOUND) {
		u2Bw = g_rNanRegInfo[i].u2Bw;
		if (u2Bw == 20)
			ucRang = 0;
#if (CFG_SUPPORT_NAN_6G == 1)
		else if ((u2Bw == 40) && IS_6G_OP_CLASS(ucOperatingClass))
			ucRang = 2;
#endif
		else if (u2Bw == 40)
			ucRang = 0;
		else if (u2Bw == 80)
			ucRang = 6;
		else if (u2Bw == 160)
			ucRang = 14;
		else if (u2Bw == 320)
			ucRang = 30;

		for (j = 0; j < REG_MAX_SUPPORT_CHANNEL; j++) {
			ucChnl = g_rNanRegInfo[i].aucSupportChnlList[j];
			if (ucChnl == 0)
				break;

			if (((ucChnl - ucRang) <= ucPrimaryChnl) &&
			    (ucPrimaryChnl <= (ucChnl + ucRang)))
				break;
		}

		if (ucChnl != 0) {
			if (u2Bw == 40) {
				if (nanRegGetSco(ucOperatingClass) ==
				    CHNL_EXT_SCA)
					ucCenterChnl = ucChnl + 2;
				else if (nanRegGetSco(ucOperatingClass) ==
					 CHNL_EXT_SCB)
					ucCenterChnl = ucChnl - 2;
#if (CFG_SUPPORT_NAN_6G == 1)
				else
					ucCenterChnl = ucChnl;
#endif
			} else {
				ucCenterChnl = ucChnl;
			}
		}
	}

	DBGLOG(NAN, TEMP,
	       "ucOperatingClass=%u, ucPrimaryChnl=%u, ucCenterChnl=%u, i=%u, j=%u\n",
	       ucOperatingClass, ucPrimaryChnl, ucCenterChnl, i, j);
	return ucCenterChnl;
}

uint8_t nanRegGetOperatingClass(uint16_t u2Bw, uint8_t ucChannel,
				enum ENUM_CHNL_EXT eSco, enum ENUM_BAND eBand)
{
	int i, j;
#if (CFG_SUPPORT_NAN_6G == 1)
	uint8_t ucIs6gChnl = (eBand == BAND_6G) ? TRUE : FALSE;
#endif

	for (i = 0; i < ARRAY_SIZE(g_rNanRegInfo); i++) {
#if (CFG_SUPPORT_NAN_6G == 1)
		if (ucIs6gChnl !=
		    IS_6G_OP_CLASS(g_rNanRegInfo[i].ucOperatingClass))
			continue;
#endif
		if ((g_rNanRegInfo[i].u2Bw == u2Bw) &&
		    (g_rNanRegInfo[i].eSco == eSco)) {
			for (j = 0; j < REG_MAX_SUPPORT_CHANNEL; j++) {
				if (g_rNanRegInfo[i].aucSupportChnlList[j] ==
				    ucChannel)
					return g_rNanRegInfo[i]
						.ucOperatingClass;
				else if (g_rNanRegInfo[i]
						 .aucSupportChnlList[j] == 0)
					break;
			}
		}
	}

	return REG_INVALID_INFO;
}

union _NAN_BAND_CHNL_CTRL nanRegGenNanChnlInfo(uint8_t ucPriChannel,
				enum ENUM_CHANNEL_WIDTH eChannelWidth,
				enum ENUM_CHNL_EXT eSco,
				uint8_t ucChannelS1, uint8_t ucChannelS2,
				enum ENUM_BAND eBand)
{
	union _NAN_BAND_CHNL_CTRL rChnlInfo;
	uint8_t ucOperatingClass = REG_INVALID_INFO;

	rChnlInfo.u4RawData = 0;

	if (eChannelWidth > CW_320_1MHZ) {
		DBGLOG(NAN, ERROR, "eChannelWidth is over!\n");
		return rChnlInfo;
	}
	if (eSco == CHNL_EXT_RES) {
		DBGLOG(NAN, ERROR, "eSco equals CHNL_EXT_RES!\n");
		return rChnlInfo;
	}
	switch (eChannelWidth) {
	case CW_20_40MHZ:
		if (eSco == CHNL_EXT_SCN) {
#if (CFG_SUPPORT_NAN_6G == 1)
			if ((eBand == BAND_6G) && (ucChannelS1 != 0))
				ucOperatingClass =
					nanRegGetOperatingClass(40,
						ucPriChannel, eSco, eBand);
			else
#endif
				ucOperatingClass =
					nanRegGetOperatingClass(20,
						ucPriChannel, eSco, eBand);
		}
		else if ((eSco == CHNL_EXT_SCA) || (eSco == CHNL_EXT_SCB))
			ucOperatingClass = nanRegGetOperatingClass(40,
						ucPriChannel, eSco, eBand);
		break;

	case CW_80MHZ:
		ucOperatingClass =
			nanRegGetOperatingClass(80, ucChannelS1, eSco, eBand);
		break;

	case CW_160MHZ:
		ucOperatingClass =
			nanRegGetOperatingClass(160, ucChannelS1, eSco, eBand);
		break;

	case CW_320_1MHZ:
		ucOperatingClass =
			nanRegGetOperatingClass(320, ucChannelS1, eSco, eBand);
		break;

	case CW_80P80MHZ:
		/* Fixme */
		DBGLOG(NAN, ERROR, "Not support 80+80!!\n");
		break;

	default:
		break;
	}

	if (ucOperatingClass != REG_INVALID_INFO) {
		rChnlInfo.u4Type = NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
		rChnlInfo.u4OperatingClass = ucOperatingClass;
		rChnlInfo.u4PrimaryChnl = ucPriChannel;
		rChnlInfo.u4AuxCenterChnl = 0; /* Fixme */
	}

	return rChnlInfo;
}

/**
 * nanRegGenNanChnlInfoByPriChannel() - Get the channel info
 * @ucPriChannel: primary channel number
 * @u2Bw: bandwidth
 * @eBand: Band of the queried channel information
 *
 * Return: channel information
 */
union _NAN_BAND_CHNL_CTRL nanRegGenNanChnlInfoByPriChannel(uint8_t ucPriChannel,
							   uint16_t u2Bw,
							   enum ENUM_BAND eBand)
{
	uint32_t u4Idx;
	enum ENUM_CHANNEL_WIDTH eChannelWidth;
	enum ENUM_CHNL_EXT eSco;
	uint8_t ucChannelS1;
	uint8_t ucChannelS2;
	unsigned char fgFound = FALSE;
	uint8_t ucCenterChnl = 0;
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucIs6gChnl = eBand == BAND_6G;
#endif

	/* Look up the OC table by matching BW and primary channel */
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(g_rNanRegInfo); u4Idx++) {
		if (g_rNanRegInfo[u4Idx].u2Bw != u2Bw)
			continue;

#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
		if (ucIs6gChnl !=
		    IS_6G_OP_CLASS(g_rNanRegInfo[u4Idx].ucOperatingClass))
			continue;
#endif

		ucCenterChnl = nanRegGetCenterChnlByPriChnl(
			g_rNanRegInfo[u4Idx].ucOperatingClass, ucPriChannel);
		if (ucCenterChnl != REG_INVALID_INFO) {
			fgFound = TRUE;
			break;
		}
	}
	if (!fgFound)
		return g_rNullChnl;

	DBGLOG(NAN, DEBUG,
	       "fgFound=%u, check idx=%u, oc=%u, bw=%u, ucCenterChnl=%u\n",
	       fgFound, u4Idx, g_rNanRegInfo[u4Idx].ucOperatingClass,
	       g_rNanRegInfo[u4Idx].u2Bw, ucCenterChnl);

	eSco = nanRegGetSco(g_rNanRegInfo[u4Idx].ucOperatingClass);

	ucChannelS1 = ucChannelS2 = 0;
	if ((u2Bw == 20) || (u2Bw == 40)) {
		eChannelWidth = CW_20_40MHZ;
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
		if (ucIs6gChnl && (u2Bw == 40))
			ucChannelS1 = ucCenterChnl;
#endif
	} else if (u2Bw == 80) {
		eChannelWidth = CW_80MHZ;
		ucChannelS1 = ucCenterChnl;
	} else if (u2Bw == 320) {
		eChannelWidth = CW_320_1MHZ;
		ucChannelS1 = ucCenterChnl;
	} else { /* 160 */
		eChannelWidth = CW_160MHZ;
		ucChannelS1 = ucCenterChnl;
	}

	/* get the channel info structure with determined channel parameters */
	return nanRegGenNanChnlInfo(ucPriChannel, eChannelWidth, eSco,
				    ucChannelS1, ucChannelS2, eBand);
}

uint32_t
nanRegConvertNanChnlInfo(union _NAN_BAND_CHNL_CTRL rChnlInfo,
			 uint8_t *pucPriChannel,
			 enum ENUM_CHANNEL_WIDTH *peChannelWidth,
			 enum ENUM_CHNL_EXT *peSco, uint8_t *pucChannelS1,
			 uint8_t *pucChannelS2) {
	uint16_t u2Bw;

	if (!pucPriChannel || !peChannelWidth || !peSco || !pucChannelS1 ||
	    !pucChannelS2)
		return WLAN_STATUS_FAILURE;

	u2Bw = nanRegGetBw(rChnlInfo.u4OperatingClass);
	if (u2Bw == REG_INVALID_INFO)
		return WLAN_STATUS_FAILURE;

	*pucPriChannel = rChnlInfo.u4PrimaryChnl;

	*peSco = nanRegGetSco(rChnlInfo.u4OperatingClass);

	*pucChannelS1 = *pucChannelS2 = 0;
	if ((u2Bw == 20) || (u2Bw == 40)) {
		*peChannelWidth = CW_20_40MHZ;
#if (CFG_SUPPORT_NAN_6G == 1)
		if (IS_6G_OP_CLASS(rChnlInfo.u4OperatingClass)) {
			*pucChannelS1 = nanRegGetCenterChnlByPriChnl(
				rChnlInfo.u4OperatingClass,
				rChnlInfo.u4PrimaryChnl);
			}
#endif
	} else if ((u2Bw == 80) && (rChnlInfo.u4AuxCenterChnl == 0)) {
		*peChannelWidth = CW_80MHZ;
		*pucChannelS1 = nanRegGetCenterChnlByPriChnl(
			rChnlInfo.u4OperatingClass,
			rChnlInfo.u4PrimaryChnl);
	} else if (u2Bw == 160) {
		*peChannelWidth = CW_160MHZ;
		*pucChannelS1 = nanRegGetCenterChnlByPriChnl(
			rChnlInfo.u4OperatingClass,
			rChnlInfo.u4PrimaryChnl);
	} else if (u2Bw == 320) {
		*peChannelWidth = CW_320_1MHZ;
		*pucChannelS1 = nanRegGetCenterChnlByPriChnl(
			rChnlInfo.u4OperatingClass,
			rChnlInfo.u4PrimaryChnl);
	} else if ((u2Bw == 80) && (rChnlInfo.u4AuxCenterChnl != 0)) {
		*peChannelWidth = CW_80P80MHZ;
		*pucChannelS1 = nanRegGetCenterChnlByPriChnl(
			rChnlInfo.u4OperatingClass,
			rChnlInfo.u4PrimaryChnl);
		*pucChannelS2 = rChnlInfo.u4AuxCenterChnl;
	}

	return WLAN_STATUS_SUCCESS;
}

enum ENUM_BAND
nanRegGetNanChnlBand(union _NAN_BAND_CHNL_CTRL rNanChnlInfo)
{
	enum ENUM_BAND eBand = BAND_NULL;
	uint32_t u4BandIdMask;

	if (rNanChnlInfo.u4Type == NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL) {
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
		if (IS_6G_OP_CLASS(rNanChnlInfo.u4OperatingClass))
			eBand = BAND_6G;
		else
#endif
			if (rNanChnlInfo.u4PrimaryChnl < 36)
				eBand = BAND_2G4;
			else
				eBand = BAND_5G;
	} else {
		u4BandIdMask = rNanChnlInfo.u4BandIdMask;

		if (u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_2P4G))
			eBand = BAND_2G4;
		else if (u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_5G))
			eBand = BAND_5G;
#if (CFG_SUPPORT_NAN_6G == 1) || (CFG_SUPPORT_WIFI_6G == 1)
		else if (u4BandIdMask & BIT(NAN_SUPPORTED_BAND_ID_6G))
			eBand = BAND_6G;
#endif
	}

	return eBand;
}

u_int8_t nanRegNanChnlBandIsEht(union _NAN_BAND_CHNL_CTRL rNanChnlInfo)
{
	if (rNanChnlInfo.u4Type != NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL)
		return FALSE;

	if (IS_EHT_OP_CLASS(rNanChnlInfo.u4OperatingClass))
		return TRUE;

	return FALSE;
}

#if (CFG_SUPPORT_NAN_6G == 1)
/**
 * nanRegConvert6gChannelBitmap() - Convert 6G channel bitmap to legacy format
 * @ucOperatingClass: Operating Class defined in 802.11 E.4
 * @pu2ChnlBitmap: 6G channel bitmap
 * @pucNewChnlBitmap: output buffer to hold the converted result
 */
uint32_t nanRegConvert6gChannelBitmap(uint8_t ucOperatingClass,
				      uint16_t *pu2ChnlBitmap,
				      uint8_t *pucNewChnlBitmap)
{
	int i, j, u4StartIdx = 0;
	uint8_t *pucBuf;
	uint8_t uc6gStartChnl, uc6gChnlNum;

	pucBuf = (uint8_t *)pu2ChnlBitmap;

	i = nanRegFindRecordIdx(ucOperatingClass);
	if (i == REG_DB_ENTRY_NOT_FOUND)
		return WLAN_STATUS_NOT_ACCEPTED;

	if (!IS_6G_OP_CLASS(ucOperatingClass))
		return WLAN_STATUS_NOT_ACCEPTED;

	uc6gStartChnl = pucBuf[0];
	uc6gChnlNum = pucBuf[1];

	DBGLOG(NAN, LOUD, "6g s:%d, c:%d, 0x%04x\n",
		uc6gStartChnl, uc6gChnlNum, *pu2ChnlBitmap);

	for (j = 0; j < REG_MAX_SUPPORT_CHANNEL; j++) {
		if (g_rNanRegInfo[i].aucSupportChnlList[j] == uc6gStartChnl)
			u4StartIdx = j;

		if (g_rNanRegInfo[i].aucSupportChnlList[j] >= uc6gStartChnl &&
		    (j - u4StartIdx) < uc6gChnlNum)
			pucNewChnlBitmap[j / 8] |= BIT(j % 8);
	}

	return WLAN_STATUS_SUCCESS;
}

/**
 * Modify u2ChannelBitmap
 * R3: Bit(i) is set if the i-th chanenl in the OC is selected
 * R4: BITS(0,7): the start chanenl number
 *     BITS(8,15): number of channels including the start channel
 */
void nanChannelBitmapR4ToR3(void *pBuf)
{
	struct _NAN_CHNL_ENTRY_T *prChannelEntry = pBuf;
	uint16_t u2ChannelBitmap = 0;
	uint8_t ucChannelStart = prChannelEntry->ucChannelStart;
	uint8_t ucChannelNum = prChannelEntry->ucChannelNum;

	if (prChannelEntry->ucOperatingClass < 131) /* same format */
		return;

	nanRegConvert6gChannelBitmap(prChannelEntry->ucOperatingClass,
				     &prChannelEntry->u2ChannelBitmap,
				     (uint8_t *)&u2ChannelBitmap);

	DBGLOG(NAN, TRACE, "R4->R3, OC %u, start=%u, num=%u, 0x%04x -> 0x%04x",
	       prChannelEntry->ucOperatingClass,
	       ucChannelStart, ucChannelNum,
	       prChannelEntry->u2ChannelBitmap, u2ChannelBitmap);

	prChannelEntry->u2ChannelBitmap = u2ChannelBitmap;
}

void nanSetNanUseR4AvailAttr(uint8_t ucEnable)
{
	g_fgNanUseR4AvailAttr = !!(ucEnable & BIT(NAN_AVAIL_BIT));
	DBGLOG(NAN, INFO, "R4 6G channel map (%u)\n", ucEnable);
}
#endif

u_int8_t nanIsNanActionFrame(struct WLAN_MAC_HEADER *pHeader,
			     uint16_t u2FrameLength)
{
	struct _NAN_ACTION_FRAME_T *prNanAction;
	const uint8_t aucWfaOui[] = NAN_OUI;

	if (!DBG_IS_LEVEL_SET(NAN, DEBUG))
		return FALSE;

	if (pHeader->b4SubType != (MAC_FRAME_ACTION >> 4))
		return FALSE;

	if (u2FrameLength < sizeof(*prNanAction))
		return FALSE;

	prNanAction = (struct _NAN_ACTION_FRAME_T *)pHeader;

	/* Category == 4 Public Action frame ||
	 *	       9 Protected Dual of Public Action Frame
	 * Action == 9 Public Action frame Vendor Specific
	 * OUI == 0x50-6F-9A
	 * OUI Type = 0x13 (Service Discovery) || 0x18 (Action)
	 */
	return (prNanAction->ucCategory == CATEGORY_PUBLIC_ACTION ||
		prNanAction->ucCategory ==
			CATEGORY_PROTECTED_DUAL_OF_PUBLIC_ACTION) &&
		prNanAction->ucAction == ACTION_PUBLIC_VENDOR_SPECIFIC &&
		!kalMemCmp(prNanAction->aucOUI, aucWfaOui, sizeof(aucWfaOui)) &&
		(prNanAction->ucOUItype == VENDOR_OUI_TYPE_NAN_SDF ||
		 prNanAction->ucOUItype == VENDOR_OUI_TYPE_NAN_NAF);
}

#endif /* CFG_SUPPORT_NAN */
