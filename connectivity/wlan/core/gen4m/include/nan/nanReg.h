/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _NAN_REG_H_
#define _NAN_REG_H_

#if CFG_SUPPORT_NAN

#define REG_INVALID_INFO 0xFF

extern u_int8_t g_fgNanUseR4AvailAttr;

uint16_t nanRegGetBw(uint8_t ucOperatingClass);
uint32_t nanRegGetChannelBitmap(uint8_t ucOperatingClass,
		uint8_t ucChannel, uint16_t *pu2ChnlBitmap);
uint8_t nanRegGetChannelByOrder(uint8_t ucOperatingClass,
		uint16_t *pu2ChnlBitmap);
uint8_t nanRegGetPrimaryChnlBehavior(uint8_t ucOperatingClass);
uint8_t nanRegGetPrimaryChannel(uint8_t ucChannel, uint16_t u2Bw,
			       uint8_t ucNonContBw,
			       uint8_t ucPriChnlIdx,
			       uint8_t ucOperatingClass);

uint8_t nanRegGetPrimaryChannelByOrder(uint8_t ucOperatingClass,
				      uint16_t *pu2ChnlBitmap,
				      uint8_t ucNonContBw,
				      uint8_t ucPriChnlBitmap);

uint32_t nanRegConvertNanChnlInfo(union _NAN_BAND_CHNL_CTRL rChnlInfo,
				  uint8_t *pucPriChannel,
				  enum ENUM_CHANNEL_WIDTH *peChannelWidth,
				  enum ENUM_CHNL_EXT *peSco,
				  uint8_t *pucChannelS1, uint8_t *pucChannelS2);

union _NAN_BAND_CHNL_CTRL
nanRegGenNanChnlInfo(uint8_t ucPriChannel,
		enum ENUM_CHANNEL_WIDTH eChannelWidth,
		enum ENUM_CHNL_EXT eSco, uint8_t ucChannelS1,
		uint8_t ucChannelS2, enum ENUM_BAND eBand);

union _NAN_BAND_CHNL_CTRL
nanRegGenNanChnlInfoByPriChannel(uint8_t ucPriChannel,
		uint16_t u2Bw, enum ENUM_BAND eBand);

enum ENUM_BAND
nanRegGetNanChnlBand(union _NAN_BAND_CHNL_CTRL rNanChnlInfo);

u_int8_t nanRegNanChnlBandIsEht(union _NAN_BAND_CHNL_CTRL rNanChnlInfo);

uint8_t
nanRegGetCenterChnlByPriChnl(uint8_t ucOperatingClass, uint8_t ucPrimaryChnl);

uint32_t
nanRegConvert6gChannelBitmap(uint8_t ucOperatingClass,
	uint16_t *pu2ChnlBitmap,
	uint8_t *pucNewChnlBitmap);

void nanChannelBitmapR4ToR3(void *pBuf);

void nanSetNanUseR4AvailAttr(uint8_t ucEnable);

u_int8_t nanIsNanActionFrame(struct WLAN_MAC_HEADER *pHeader,
			     uint16_t u2FrameLength);

#endif

#endif /* _NAN_REG_H_ */
