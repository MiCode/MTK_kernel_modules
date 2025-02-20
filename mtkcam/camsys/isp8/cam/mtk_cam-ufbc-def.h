/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Hung-Wen Hsieh <hung-wen.hsieh@mediatek.com>
 */

#ifndef __UFBC_DEF_H__
#define __UFBC_DEF_H__


/**
 * Describe Mediatek UFBC (Universal Frame Buffer Compression) buffer header.
 * Mediatek UFBC supports 1 plane Bayer and 2 planes Y/UV image formats.
 * Caller must follow the formulation to calculate the bit stream buffer size
 * and length table buffer size.
 *
 * Header Size
 *
 * Fixed size of 4096 bytes. Reserved bytes will be used by Mediatek
 * ISP driver. Caller SHOULD NOT edit it.
 *
 * Bit Stream Size
 *
 *  @code
 *    // for each plane
 *    size = ((width + 63) / 64) * 64;      // width must be aligned to 64 pixel
 *    size = (size * bitsPerPixel + 7) / 8; // convert to bytes
 *    size = size * height;
 *  @endcode
 *
 * Table Size
 *
 *  @code
 *    // for each plane
 *    size = (width + 63) / 64;
 *    size = size * height;
 *  @endcode
 *
 * And the memory layout should be followed as
 *
 *  @code
 *           Bayer                  YUV2P
 *    +------------------+  +------------------+
 *    |      Header      |  |      Header      |
 *    +------------------+  +------------------+
 *    |                  |  |     Y Plane      |
 *    | Bayer Bit Stream |  |    Bit Stream    |
 *    |                  |  |                  |
 *    +------------------+  +------------------+
 *    |   Length Table   |  |     UV Plane     |
 *    +------------------+  |    Bit Stream    |
 *                          |                  |
 *                          +------------------+
 *                          |     Y Plane      |
 *                          |   Length Table   |
 *                          +------------------+
 *                          |     UV Plane     |
 *                          |   Length Table   |
 *                          +------------------+
 *  @endcode
 *
 *  @note Caller has responsibility to fill all the fields according the
 *        real buffer layout.
 */

struct UfbcBufferHeader {
	uint8_t Rsv[4096];
};

/******************************************************************************
 * @UFBC format meta info
 *
 ******************************************************************************/
struct UFD_META_INFO {
	unsigned int bUF;
	unsigned int UFD_BITSTREAM_OFST_ADDR[4];
	unsigned int UFD_BS_AU_START[4];
	unsigned int UFD_AU2_SIZE[4];
	unsigned int UFD_BOND_MODE[4];
};

struct UFD_HW_META_INFO {
	unsigned int Fh0[32];
	unsigned int Fh1[32];
};

union UFDStruct {
	struct UFD_META_INFO UFD;
	struct UFD_HW_META_INFO HWUFD;
};

struct UFO_META_INFO {
	unsigned int AUWriteBySW;
	unsigned int Rsv[15];
	union UFDStruct UFD;
};


struct YUFD_META_INFO {
	unsigned int bYUF;
	unsigned int YUFD_BITSTREAM_OFST_ADDR[4];
	unsigned int YUFD_BS_AU_START[4];
	unsigned int YUFD_AU2_SIZE[4];
	unsigned int YUFD_BOND_MODE[4];
};

struct YUFD_HW_META_INFO {
	unsigned int Fh0[32];
	unsigned int Fh1[32];
};

union YUFDStruct {
	struct YUFD_META_INFO YUFD;
	struct YUFD_HW_META_INFO HWYUFD;
};

struct YUFO_META_INFO {
	unsigned int AUWriteBySW;
	unsigned int Rsv[15];
	union YUFDStruct YUFD;
};

#endif // __UFBC_DEF_H__
