/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) 2024 MediaTek Inc.

#ifndef __MTK_CAM_SENINF_UTILS_H__
#define __MTK_CAM_SENINF_UTILS_H__

#include <media/v4l2-subdev.h>
#include <uapi/linux/media-bus-format.h>


#define to_std_fmt_code(code) \
	((code) & 0xFFFF)

#define get_scenario_from_fmt_code(code) \
({ \
	int __val = 0; \
	__val = ((code) >> 16) & 0xFF; \
	__val; \
})


static inline int chk_subdev_ops_command_exist(struct v4l2_subdev *sd)
{
	return (sd && sd->ops && sd->ops->core && sd->ops->core->command)
		? 1 : 0;
}

static inline unsigned int dt_remap_to_mipi_dt(unsigned int dt_remap)
{
	switch (dt_remap) {
	case MTK_MBUS_FRAME_DESC_REMAP_TO_RAW12:
		return 0x2c;
	case MTK_MBUS_FRAME_DESC_REMAP_TO_RAW14:
		return 0x2d;
	case MTK_MBUS_FRAME_DESC_REMAP_TO_RAW10:
	default:
		return 0x2b;
	}
}

static inline unsigned int get_code2dt(unsigned int code)
{
	switch (code) {
	case MEDIA_BUS_FMT_SBGGR8_1X8:
	case MEDIA_BUS_FMT_SGBRG8_1X8:
	case MEDIA_BUS_FMT_SGRBG8_1X8:
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		return 0x2a;

	case MEDIA_BUS_FMT_SBGGR12_1X12:
	case MEDIA_BUS_FMT_SGBRG12_1X12:
	case MEDIA_BUS_FMT_SGRBG12_1X12:
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		return 0x2c;

	case MEDIA_BUS_FMT_SBGGR14_1X14:
	case MEDIA_BUS_FMT_SGBRG14_1X14:
	case MEDIA_BUS_FMT_SGRBG14_1X14:
	case MEDIA_BUS_FMT_SRGGB14_1X14:
		return 0x2d;
	case MEDIA_BUS_FMT_YUYV8_2X8:  //0x2008
	case MEDIA_BUS_FMT_UYVY8_2X8:  //0x2006
	case MEDIA_BUS_FMT_VYUY8_2X8:  //0x2007
	case MEDIA_BUS_FMT_YVYU8_2X8:  //0x2009
		return 0x1e;

	case MEDIA_BUS_FMT_SBGGR10_ALAW8_1X8:
	case MEDIA_BUS_FMT_SGBRG10_ALAW8_1X8:
	case MEDIA_BUS_FMT_SGRBG10_ALAW8_1X8:
	case MEDIA_BUS_FMT_SRGGB10_ALAW8_1X8:
	case MEDIA_BUS_FMT_SBGGR10_DPCM8_1X8:
	case MEDIA_BUS_FMT_SGBRG10_DPCM8_1X8:
	case MEDIA_BUS_FMT_SGRBG10_DPCM8_1X8:
	case MEDIA_BUS_FMT_SRGGB10_DPCM8_1X8:
	case MEDIA_BUS_FMT_SBGGR10_2X8_PADHI_BE:
	case MEDIA_BUS_FMT_SBGGR10_2X8_PADHI_LE:
	case MEDIA_BUS_FMT_SBGGR10_2X8_PADLO_BE:
	case MEDIA_BUS_FMT_SBGGR10_2X8_PADLO_LE:
	case MEDIA_BUS_FMT_SBGGR10_1X10:
	case MEDIA_BUS_FMT_SGBRG10_1X10:
	case MEDIA_BUS_FMT_SGRBG10_1X10:
	case MEDIA_BUS_FMT_SRGGB10_1X10:
	default:
		return 0x2b;
	}
}

static inline int get_mbus_format_by_dt(int dt, int remap_type)
{
	int remap_dt = (remap_type == MTK_MBUS_FRAME_DESC_REMAP_NONE)
		? dt : dt_remap_to_mipi_dt(remap_type);

	switch (remap_dt) {
	case 0x2a:
		return MEDIA_BUS_FMT_SBGGR8_1X8;
	case 0x2b:
		return MEDIA_BUS_FMT_SBGGR10_1X10;
	case 0x2c:
		return MEDIA_BUS_FMT_SBGGR12_1X12;
	case 0x2d:
		return MEDIA_BUS_FMT_SBGGR14_1X14;
	case 0x24:
		return MEDIA_BUS_FMT_RGB888_1X24;
	case 0x1e:
		return MEDIA_BUS_FMT_YUYV8_2X8;
	default:
		/* default raw8 for other data types */
		return MEDIA_BUS_FMT_SBGGR8_1X8;
	}
}

static inline u16 conv_ebd_hsize_raw14(u16 exp_hsize, u8 ebd_parsing_type)
{
	u16 result = exp_hsize;

	// roundup to 8x
	switch (ebd_parsing_type) {
	case MTK_EBD_PARSING_TYPE_MIPI_RAW10:
		result = (result * 10 + 13) / 14;
		//result = (result + 7) & (~0x7);
		break;
	case MTK_EBD_PARSING_TYPE_MIPI_RAW12:
		result = (result * 12 + 13) / 14;
		//result = (result + 7) & (~0x7);
		break;
	case MTK_EBD_PARSING_TYPE_MIPI_RAW14:
		// do nothing
		break;
	default: // 8
		result = (result * 8 + 13) / 14;
		//result = (result + 7) & (~0x7);
		break;
	}

	return result;
}

#endif /* __MTK_CAM_SENINF_UTILS_H__ */
