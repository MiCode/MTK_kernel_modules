/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2018-2022 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef _MALI_FOURCC_H_
#define _MALI_FOURCC_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <drm.h>
#include <drm_fourcc.h>
#pragma GCC diagnostic pop

/* Including stddef.h because libdrm version we use doesn't define size_t
 * like versions after 1a2a42c8bfa25.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#if defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC
#include "mali_private_fourcc.h"
#endif /* defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC */

/* FOURCCs for formats that exist upstream, but may not be in the drm_fourcc.h header included above.
 *
 * Below we define DRM FOURCC formats that are upstreamed, but may not be in the drm_fourcc.h header that we include
 * above, merely because that header is too old. As drm_fourcc.h is an external header that we cannot control, the best
 * we can do is to define here the missing formats.
 */
#ifndef DRM_FORMAT_R8
#define DRM_FORMAT_R8 fourcc_code('R', '8', ' ', ' ')
#endif

#ifndef DRM_FORMAT_INVALID
#define DRM_FORMAT_INVALID 0
#endif

#ifndef DRM_FORMAT_P010
#define DRM_FORMAT_P010 fourcc_code('P', '0', '1', '0')
#endif

#ifndef DRM_FORMAT_P016
#define DRM_FORMAT_P016 fourcc_code('P', '0', '1', '6')
#endif

#ifndef DRM_FORMAT_Y0L2
#define DRM_FORMAT_Y0L2 fourcc_code('Y', '0', 'L', '2')
#endif

#ifndef DRM_FORMAT_P210
#define DRM_FORMAT_P210 fourcc_code('P', '2', '1', '0')
#endif

#ifndef DRM_FORMAT_Y210
#define DRM_FORMAT_Y210 fourcc_code('Y', '2', '1', '0')
#endif

#ifndef DRM_FORMAT_Y410
#define DRM_FORMAT_Y410 fourcc_code('Y', '4', '1', '0')
#endif

#ifndef DRM_FORMAT_YUV420_8BIT
#define DRM_FORMAT_YUV420_8BIT fourcc_code('Y', 'U', '0', '8')
#endif

#ifndef DRM_FORMAT_YUV420_10BIT
#define DRM_FORMAT_YUV420_10BIT fourcc_code('Y', 'U', '1', '0')
#endif

#ifndef DRM_FORMAT_ABGR16161616F
#define DRM_FORMAT_ABGR16161616F fourcc_code('A', 'B', '4', 'H')
#endif

#ifndef DRM_FORMAT_R16
#define DRM_FORMAT_R16 fourcc_code('R', '1', '6', ' ')
#endif

#ifndef DRM_FORMAT_GR1616
#define DRM_FORMAT_GR1616 fourcc_code('G', 'R', '3', '2')
#endif

/*
 * 2 plane YCbCr
 * index 0 = Y plane, [39:0] Y3:Y2:Y1:Y0 little endian
 * index 1 = Cr:Cb plane, [39:0] Cr1:Cb1:Cr0:Cb0 little endian
 */
#ifndef DRM_FORMAT_NV15
#define DRM_FORMAT_NV15 fourcc_code('N', 'V', '1', '5') /* 2x2 subsampled Cr:Cb plane */
#endif

/*
 * 3 plane non-subsampled (444) YCbCr
 * 16 bits per component, but only 10 bits are used and 6 bits are padded
 * index 0: Y plane, [15:0] Y:x [10:6] little endian
 * index 1: Cb plane, [15:0] Cb:x [10:6] little endian
 * index 2: Cr plane, [15:0] Cr:x [10:6] little endian
 */
#ifndef DRM_FORMAT_Q410
#define DRM_FORMAT_Q410 fourcc_code('Q', '4', '1', '0')
#endif

/*
 * 3 plane non-subsampled (444) YCrCb
 * 16 bits per component, but only 10 bits are used and 6 bits are padded
 * index 0: Y plane, [15:0] Y:x [10:6] little endian
 * index 1: Cr plane, [15:0] Cr:x [10:6] little endian
 * index 2: Cb plane, [15:0] Cb:x [10:6] little endian
 */
#ifndef DRM_FORMAT_Q401
#define DRM_FORMAT_Q401 fourcc_code('Q', '4', '0', '1')
#endif

/*
 * RGBA format with 10-bit components packed in 64-bit per pixel, with 6 bits of unused padding per component:
 * [63:0] A:x:B:x:G:x:R:x 10:6:10:6:10:6:10:6 little endian
 */
#ifndef DRM_FORMAT_AXBXGXRX106106106106
#define DRM_FORMAT_AXBXGXRX106106106106 fourcc_code('A', 'B', '1', '0')
#endif

/* FOURCCs for formats that do not currently exist upstream, but should.
 *
 * Formats defined in this file MUST have at least one user via the winsys interface. For formats only used internally
 * (e.g formats only used for PBuffers or YUV intermediate formats), the cobj_surface_format interface should be used
 * directly.
 *
 * Formats defined here _should_ be in the process of being upstreamed.
 *
 * Temporary FOURCCs have the pattern [A][r][m][0-9A-Za-z].
 * The corresponding macros must have the form ARM_DRM_FORMAT_xxx
 * The following temporary FOURCCs have already been used:
 *     Arm0: ARM_DRM_FORMAT_NV15
 *     Arm1: ARM_DRM_FORMAT_Q410
 *     Arm2: ARM_DRM_FORMAT_Q401
 *     Arm3: ARM_DRM_FORMAT_AXBXGXRX106106106106
 *
 * Once upstreamed the ARM_DRM_FORMAT_xxxx definition below should be removed.
 * These temporary FOURCCs should not be re-used. For example, Arm0 should only be used for
 * ARM_DRM_FORMAT_NV15 and must not be used for a different format, even after ARM_DRM_FORMAT_NV15
 * is upstreamed and its definition is removed from this file.
 *
 * WARNING: Both the macro name and the FOURCC value are very likely to change when these formats are upstreamed. As
 * such, there is *no* API or ABI stability guarantee for formats defined in this file.
 */

/* DRM FOURCC modifiers that exist upstream, but may not be in a drm_fourcc.h
 *
 * These values can be considered stable, but are defined here if required to support building against older
 * drm_fourcc.h.
 *
 * Initial support for AFBC modifiers included in Linux 4.19-rc1: ce6058039
 */
#ifndef DRM_FORMAT_MOD_VENDOR_ARM
#define DRM_FORMAT_MOD_VENDOR_ARM 0x08
#endif

#ifndef DRM_FORMAT_MOD_VENDOR_NONE
#define DRM_FORMAT_MOD_VENDOR_NONE 0
#endif

#ifndef DRM_FORMAT_RESERVED
#define DRM_FORMAT_RESERVED ((1ULL << 56) - 1)
#endif

#ifndef DRM_FORMAT_MOD_INVALID
#define DRM_FORMAT_MOD_INVALID fourcc_mod_code(NONE, DRM_FORMAT_RESERVED)
#endif

#ifndef fourcc_mod_code
#define fourcc_mod_code(vendor, val) ((((__u64)DRM_FORMAT_MOD_VENDOR_##vendor) << 56) | ((val)&0x00ffffffffffffffULL))
#endif

/*
 * The top 4 bits (out of the 56 bits alloted for specifying vendor specific
 * modifiers) denote the category for modifiers. Currently we have three
 * categories of modifiers ie AFBC, MISC and AFRC. We can have a maximum of
 * sixteen different categories.
 */
#ifndef DRM_FORMAT_MOD_ARM_CODE
#define DRM_FORMAT_MOD_ARM_CODE(__type, __val) \
	fourcc_mod_code(ARM, ((((__u64)(__type)) << 52) | ((__val)&0x000fffffffffffffULL)))
#endif

#ifndef DRM_FORMAT_MOD_ARM_TYPE_MASK
#define DRM_FORMAT_MOD_ARM_TYPE_MASK 0xf
#endif

#ifndef DRM_FORMAT_MOD_ARM_TYPE_AFBC
#define DRM_FORMAT_MOD_ARM_TYPE_AFBC 0x00
#endif

#ifndef DRM_FORMAT_MOD_ARM_TYPE_MISC
#define DRM_FORMAT_MOD_ARM_TYPE_MISC 0x01
#endif

#ifndef DRM_FORMAT_MOD_ARM_TYPE_AFRC
#define DRM_FORMAT_MOD_ARM_TYPE_AFRC 0x02
#endif

/*
 * Arm 16x16 Block U-Interleaved modifier
 *
 * This is copied from linux kernel. It divides the image
 * into 16x16 pixel blocks. Blocks are stored linearly in order, but pixels
 * in the block are reordered.
 */
#ifndef DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED
#define DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED DRM_FORMAT_MOD_ARM_CODE(DRM_FORMAT_MOD_ARM_TYPE_MISC, 1ULL)
#endif

/*
 * Tiled, 16x16 block linear modifier.
 *
 * This fourcc modifier is an alias to an existing fourcc modifier
 * that is already upstreamed however this one has vendor neutral naming.
 *
 * This is a simple tiled layout using tiles of 16x16 pixels in a row-major
 * layout. For YCbCr formats Cb/Cr components are taken in such a way that
 * they correspond to their 16x16 luma block.
 */
#ifndef DRM_FORMAT_MOD_GENERIC_16_16_TILE
#define DRM_FORMAT_MOD_GENERIC_16_16_TILE ((((__u64)0x04) << 56) | (2 & 0x00ffffffffffffffULL))
#endif

#ifndef DRM_FORMAT_MOD_ARM_AFBC
#define DRM_FORMAT_MOD_ARM_AFBC(__afbc_mode) DRM_FORMAT_MOD_ARM_CODE(DRM_FORMAT_MOD_ARM_TYPE_AFBC, __afbc_mode)
#endif

#ifndef DRM_FORMAT_MOD_ARM_AFRC
#define DRM_FORMAT_MOD_ARM_AFRC(__afrc_mode) DRM_FORMAT_MOD_ARM_CODE(DRM_FORMAT_MOD_ARM_TYPE_AFRC, __afrc_mode)
#endif

/* AFBC superblock size */
#ifndef AFBC_FORMAT_MOD_BLOCK_SIZE_16x16
#define AFBC_FORMAT_MOD_BLOCK_SIZE_16x16 ((__u64)0x1)
#endif

#ifndef AFBC_FORMAT_MOD_BLOCK_SIZE_32x8
#define AFBC_FORMAT_MOD_BLOCK_SIZE_32x8 ((__u64)0x2)
#endif

#ifndef AFBC_FORMAT_MOD_BLOCK_SIZE_MASK
#define AFBC_FORMAT_MOD_BLOCK_SIZE_MASK ((__u64)0xf)
#endif

/* AFBC lossless transform */
#ifndef AFBC_FORMAT_MOD_YTR
#define AFBC_FORMAT_MOD_YTR (((__u64)1) << 4)
#endif

/* AFBC block-split */
#ifndef AFBC_FORMAT_MOD_SPLIT
#define AFBC_FORMAT_MOD_SPLIT (((__u64)1) << 5)
#endif

/* AFBC sparse layout */
#ifndef AFBC_FORMAT_MOD_SPARSE
#define AFBC_FORMAT_MOD_SPARSE (((__u64)1) << 6)
#endif

/* AFBC copy-block restrict */
#ifndef AFBC_FORMAT_MOD_CBR
#define AFBC_FORMAT_MOD_CBR (((__u64)1) << 7)
#endif

/* AFBC tiled layout */
#ifndef AFBC_FORMAT_MOD_TILED
#define AFBC_FORMAT_MOD_TILED (((__u64)1) << 8)
#endif

/* AFBC solid color blocks */
#ifndef AFBC_FORMAT_MOD_SC
#define AFBC_FORMAT_MOD_SC (((__u64)1) << 9)
#endif

/* AFBC 1.3 block sizes */
#ifndef AFBC_FORMAT_MOD_BLOCK_SIZE_64x4
#define AFBC_FORMAT_MOD_BLOCK_SIZE_64x4 ((__u64)0x3)
#endif

#ifndef AFBC_FORMAT_MOD_BLOCK_SIZE_32x8_64x4
#define AFBC_FORMAT_MOD_BLOCK_SIZE_32x8_64x4 ((__u64)0x4)
#endif

/* AFBC double-buffer */
#ifndef AFBC_FORMAT_MOD_DB
#define AFBC_FORMAT_MOD_DB (((__u64)1) << 10)
#endif

/* AFBC buffer content hints */
#ifndef AFBC_FORMAT_MOD_BCH
#define AFBC_FORMAT_MOD_BCH (((__u64)1) << 11)
#endif

/* AFBC uncompressed storage mode */
#ifndef AFBC_FORMAT_MOD_USM
#define AFBC_FORMAT_MOD_USM (((__u64)1) << 12)
#endif

#ifndef DRM_FORMAT_MOD_LINEAR
#define DRM_FORMAT_MOD_LINEAR 0
#endif

/*
 * AFRC coding unit size.
 *
 * Indicates the coding unit size in bytes for one or more planes in an AFRC
 * encoded buffer. The coding unit size for chrominance is the same for both
 * Cb and Cr, which may be stored in separate planes.
 *
 * For RGBA formats, AFRC_FORMAT_MOD_CU_SIZE_P0 must be specified.
 * For YUV formats, both AFRC_FORMAT_MOD_CU_SIZE_P0 and
 * AFRC_FORMAT_MOD_CU_SIZE_P12 must be specified.
 */
#ifndef AFRC_FORMAT_MOD_CU_SIZE_MASK
#define AFRC_FORMAT_MOD_CU_SIZE_MASK 0xf
#endif

#ifndef AFRC_FORMAT_MOD_CU_SIZE_16
#define AFRC_FORMAT_MOD_CU_SIZE_16 (1ULL)
#endif

#ifndef AFRC_FORMAT_MOD_CU_SIZE_24
#define AFRC_FORMAT_MOD_CU_SIZE_24 (2ULL)
#endif

#ifndef AFRC_FORMAT_MOD_CU_SIZE_32
#define AFRC_FORMAT_MOD_CU_SIZE_32 (3ULL)
#endif

#ifndef AFRC_FORMAT_MOD_CU_SIZE_P0
#define AFRC_FORMAT_MOD_CU_SIZE_P0(__afrc_cu_size) (__afrc_cu_size)
#endif

#ifndef AFRC_FORMAT_MOD_CU_SIZE_P12
#define AFRC_FORMAT_MOD_CU_SIZE_P12(__afrc_cu_size) ((__afrc_cu_size) << 4)
#endif

/*
 * AFRC scanline memory layout.
 *
 * Indicates scanline memory layout is in use for an AFRC encoded
 * buffer. The memory layout is the same for all planes.
 */
#ifndef AFRC_FORMAT_MOD_LAYOUT_SCAN
#define AFRC_FORMAT_MOD_LAYOUT_SCAN ((1ULL) << 8)
#endif

/* DRM FOURCC modifiers that do not currently exist upstream, but should.
 *
 * Format modifiers defined here _should_ be in the process of being upstreamed.
 *
 * WARNING: Both the macro name and the FOURCC modifier value are very likely
 * to change when these format modifiers are upstreamed. As such, there is *no*
 * API or ABI stability guarantee for format modifiers defined in this file.
 */


/* HyFBC format. */
#ifndef DRM_FORMAT_MOD_HYFBC

/* Check with HyFBC about the vendor specific value */
#ifndef DRM_FORMAT_MOD_VENDOR_HYFBC
#define DRM_FORMAT_MOD_VENDOR_HYFBC 0x09
#endif

/* Modifier for HyFBC format */
#define DRM_FORMAT_MOD_CUSTOM_HYFBC ((__u64)1 << 12)

#define DRM_FORMAT_MOD_HYFBC(__hyfbc_mode) fourcc_mod_code(HYFBC, ((__hyfbc_mode) | DRM_FORMAT_MOD_CUSTOM_HYFBC))
#endif /* DRM_FORMAT_MOD_HYFBC */

/* Below here are utilities to aid use of the drm_fourcc values - there is no intention to
 * upstream these */

#define fourcc_mod_code_get_vendor(val) ((val) >> 56)
#define fourcc_mod_code_get_type(val) (((val) >> 52) & DRM_FORMAT_MOD_ARM_TYPE_MASK)

/* Returns true if the modifier describes an AFBC format. */
static inline bool drm_fourcc_modifier_is_afbc(uint64_t modifier)
{
	uint32_t vendor = fourcc_mod_code_get_vendor(modifier);
	uint32_t type = fourcc_mod_code_get_type(modifier);
	return DRM_FORMAT_MOD_VENDOR_ARM == vendor && DRM_FORMAT_MOD_ARM_TYPE_AFBC == type;
}

static inline bool drm_fourcc_modifier_is_hyfbc(uint64_t modifier)
{
	uint32_t vendor = fourcc_mod_code_get_vendor(modifier);
	return DRM_FORMAT_MOD_VENDOR_HYFBC == vendor;
}

/* Returns true if the modifier describes an AFRC format. */
static inline bool drm_fourcc_modifier_is_afrc(uint64_t modifier)
{
	uint32_t vendor = fourcc_mod_code_get_vendor(modifier);
	uint32_t type = fourcc_mod_code_get_type(modifier);
	return DRM_FORMAT_MOD_VENDOR_ARM == vendor && DRM_FORMAT_MOD_ARM_TYPE_AFRC == type;
}

/* Returns true if the modifier describes an 16x16 block linear YUV format. */
static inline bool drm_fourcc_modifier_is_block_linear_yuv(uint64_t modifier)
{
	return modifier == DRM_FORMAT_MOD_GENERIC_16_16_TILE;
}

/* Returns the number of planes represented by a fourcc format. */
static inline uint32_t drm_fourcc_format_get_num_planes(uint32_t format)
{
	switch (format)
	{
	default:
		return 0;

	case DRM_FORMAT_C8:
	case DRM_FORMAT_R8:
	case DRM_FORMAT_RGB332:
	case DRM_FORMAT_BGR233:
	case DRM_FORMAT_XRGB4444:
	case DRM_FORMAT_XBGR4444:
	case DRM_FORMAT_RGBX4444:
	case DRM_FORMAT_BGRX4444:
	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_BGRA4444:
	case DRM_FORMAT_XRGB1555:
	case DRM_FORMAT_XBGR1555:
	case DRM_FORMAT_RGBX5551:
	case DRM_FORMAT_BGRX5551:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_XBGR2101010:
	case DRM_FORMAT_RGBX1010102:
	case DRM_FORMAT_BGRX1010102:
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_AYUV:
	case DRM_FORMAT_ABGR16161616F:
	case DRM_FORMAT_AXBXGXRX106106106106:
	case DRM_FORMAT_R16:
	case DRM_FORMAT_GR1616:
	case DRM_FORMAT_Y410:
	case DRM_FORMAT_Y0L2:
	case DRM_FORMAT_Y210:
	case DRM_FORMAT_YUV420_8BIT:
	case DRM_FORMAT_YUV420_10BIT:
#if defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC
	case ARM_PRIV_DRM_FORMAT_AYUV2101010:
	case ARM_PRIV_DRM_FORMAT_YUYV10101010:
	case ARM_PRIV_DRM_FORMAT_VYUY10101010:
#endif /* defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC */
		return 1;

	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
	case DRM_FORMAT_P010:
	case DRM_FORMAT_P016:
	case DRM_FORMAT_P210:
	case DRM_FORMAT_NV15:
#if defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC
	case ARM_PRIV_DRM_FORMAT_Y_UV422_10BIT:
#endif /* defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC */
		return 2;

	case DRM_FORMAT_YUV410:
	case DRM_FORMAT_YVU410:
	case DRM_FORMAT_YUV411:
	case DRM_FORMAT_YVU411:
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YUV422:
	case DRM_FORMAT_YVU422:
	case DRM_FORMAT_YUV444:
	case DRM_FORMAT_YVU444:
	case DRM_FORMAT_Q410:
	case DRM_FORMAT_Q401:
		return 3;
	}
}

/**
 * @brief Returns the string representation of the given FOURCC.
 *
 * @param format A FOURCC format which is known by the DDK.
 *
 * @return The string representation of @p format, or "UNKNOWN-FOURCC" for formats not known by the DDK.
 */
static inline const char *drm_fourcc_format_to_string(uint32_t format)
{
	switch (format)
	{
	case DRM_FORMAT_C8:
		return "DRM_FORMAT_C8";
	case DRM_FORMAT_R8:
		return "DRM_FORMAT_R8";
	case DRM_FORMAT_RGB332:
		return "DRM_FORMAT_RGB332";
	case DRM_FORMAT_BGR233:
		return "DRM_FORMAT_BGR233";
	case DRM_FORMAT_XRGB4444:
		return "DRM_FORMAT_XRGB4444";
	case DRM_FORMAT_XBGR4444:
		return "DRM_FORMAT_XBGR4444";
	case DRM_FORMAT_RGBX4444:
		return "DRM_FORMAT_RGBX4444";
	case DRM_FORMAT_BGRX4444:
		return "DRM_FORMAT_BGRX4444";
	case DRM_FORMAT_ARGB4444:
		return "DRM_FORMAT_ARGB4444";
	case DRM_FORMAT_ABGR4444:
		return "DRM_FORMAT_ABGR4444";
	case DRM_FORMAT_RGBA4444:
		return "DRM_FORMAT_RGBA4444";
	case DRM_FORMAT_BGRA4444:
		return "DRM_FORMAT_BGRA4444";
	case DRM_FORMAT_XRGB1555:
		return "DRM_FORMAT_XRGB1555";
	case DRM_FORMAT_XBGR1555:
		return "DRM_FORMAT_XBGR1555";
	case DRM_FORMAT_RGBX5551:
		return "DRM_FORMAT_RGBX5551";
	case DRM_FORMAT_BGRX5551:
		return "DRM_FORMAT_BGRX5551";
	case DRM_FORMAT_ARGB1555:
		return "DRM_FORMAT_ARGB1555";
	case DRM_FORMAT_ABGR1555:
		return "DRM_FORMAT_ABGR1555";
	case DRM_FORMAT_RGBA5551:
		return "DRM_FORMAT_RGBA5551";
	case DRM_FORMAT_BGRA5551:
		return "DRM_FORMAT_BGRA5551";
	case DRM_FORMAT_RGB565:
		return "DRM_FORMAT_RGB565";
	case DRM_FORMAT_BGR565:
		return "DRM_FORMAT_BGR565";
	case DRM_FORMAT_RGB888:
		return "DRM_FORMAT_RGB888";
	case DRM_FORMAT_BGR888:
		return "DRM_FORMAT_BGR888";
	case DRM_FORMAT_XRGB8888:
		return "DRM_FORMAT_XRGB8888";
	case DRM_FORMAT_XBGR8888:
		return "DRM_FORMAT_XBGR8888";
	case DRM_FORMAT_RGBX8888:
		return "DRM_FORMAT_RGBX8888";
	case DRM_FORMAT_BGRX8888:
		return "DRM_FORMAT_BGRX8888";
	case DRM_FORMAT_ARGB8888:
		return "DRM_FORMAT_ARGB8888";
	case DRM_FORMAT_ABGR8888:
		return "DRM_FORMAT_ABGR8888";
	case DRM_FORMAT_RGBA8888:
		return "DRM_FORMAT_RGBA8888";
	case DRM_FORMAT_BGRA8888:
		return "DRM_FORMAT_BGRA8888";
	case DRM_FORMAT_XRGB2101010:
		return "DRM_FORMAT_XRGB2101010";
	case DRM_FORMAT_XBGR2101010:
		return "DRM_FORMAT_XBGR2101010";
	case DRM_FORMAT_RGBX1010102:
		return "DRM_FORMAT_RGBX1010102";
	case DRM_FORMAT_BGRX1010102:
		return "DRM_FORMAT_BGRX1010102";
	case DRM_FORMAT_ARGB2101010:
		return "DRM_FORMAT_ARGB2101010";
	case DRM_FORMAT_ABGR2101010:
		return "DRM_FORMAT_ABGR2101010";
	case DRM_FORMAT_RGBA1010102:
		return "DRM_FORMAT_RGBA1010102";
	case DRM_FORMAT_BGRA1010102:
		return "DRM_FORMAT_BGRA1010102";
	case DRM_FORMAT_YUYV:
		return "DRM_FORMAT_YUYV";
	case DRM_FORMAT_YVYU:
		return "DRM_FORMAT_YVYU";
	case DRM_FORMAT_UYVY:
		return "DRM_FORMAT_UYVY";
	case DRM_FORMAT_VYUY:
		return "DRM_FORMAT_VYUY";
	case DRM_FORMAT_AYUV:
		return "DRM_FORMAT_AYUV";
	case DRM_FORMAT_ABGR16161616F:
		return "DRM_FORMAT_ABGR16161616F";
	case DRM_FORMAT_R16:
		return "DRM_FORMAT_R16";
	case DRM_FORMAT_GR1616:
		return "DRM_FORMAT_GR1616";
	case DRM_FORMAT_Y410:
		return "DRM_FORMAT_Y410";
	case DRM_FORMAT_Y0L2:
		return "DRM_FORMAT_Y0L2";
	case DRM_FORMAT_Y210:
		return "DRM_FORMAT_Y210";
	case DRM_FORMAT_YUV420_8BIT:
		return "DRM_FORMAT_YUV420_8BIT";
	case DRM_FORMAT_YUV420_10BIT:
		return "DRM_FORMAT_YUV420_10BIT";
	case DRM_FORMAT_NV12:
		return "DRM_FORMAT_NV12";
	case DRM_FORMAT_NV21:
		return "DRM_FORMAT_NV21";
	case DRM_FORMAT_NV16:
		return "DRM_FORMAT_NV16";
	case DRM_FORMAT_NV61:
		return "DRM_FORMAT_NV61";
	case DRM_FORMAT_P010:
		return "DRM_FORMAT_P010";
	case DRM_FORMAT_P016:
		return "DRM_FORMAT_P016";
	case DRM_FORMAT_P210:
		return "DRM_FORMAT_P210";
	case DRM_FORMAT_YUV410:
		return "DRM_FORMAT_YUV410";
	case DRM_FORMAT_YVU410:
		return "DRM_FORMAT_YVU410";
	case DRM_FORMAT_YUV411:
		return "DRM_FORMAT_YUV411";
	case DRM_FORMAT_YVU411:
		return "DRM_FORMAT_YVU411";
	case DRM_FORMAT_YUV420:
		return "DRM_FORMAT_YUV420";
	case DRM_FORMAT_YVU420:
		return "DRM_FORMAT_YVU420";
	case DRM_FORMAT_YUV422:
		return "DRM_FORMAT_YUV422";
	case DRM_FORMAT_YVU422:
		return "DRM_FORMAT_YVU422";
	case DRM_FORMAT_YUV444:
		return "DRM_FORMAT_YUV444";
	case DRM_FORMAT_YVU444:
		return "DRM_FORMAT_YVU444";
	case DRM_FORMAT_NV15:
		return "DRM_FORMAT_NV15";
	case DRM_FORMAT_Q410:
		return "DRM_FORMAT_Q410";
	case DRM_FORMAT_Q401:
		return "DRM_FORMAT_Q401";
	case DRM_FORMAT_AXBXGXRX106106106106:
		return "DRM_FORMAT_AXBXGXRX106106106106";
#if defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC
	case ARM_PRIV_DRM_FORMAT_AYUV2101010:
		return "ARM_PRIV_DRM_FORMAT_AYUV2101010";
	case ARM_PRIV_DRM_FORMAT_YUYV10101010:
		return "ARM_PRIV_DRM_FORMAT_YUYV10101010";
	case ARM_PRIV_DRM_FORMAT_VYUY10101010:
		return "ARM_PRIV_DRM_FORMAT_VYUY10101010";
	case ARM_PRIV_DRM_FORMAT_Y_UV422_10BIT:
		return "ARM_PRIV_DRM_FORMAT_Y_UV422_10BIT";
#endif /* defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC */
	case DRM_FORMAT_INVALID:
		return "DRM_FORMAT_INVALID";
	default:
		return "UNKNOWN-FOURCC";
	}
}

/* Returns the bits per pixel of the format. Supports only single plane non-YUV formats. */
static inline uint32_t drm_fourcc_format_get_bits_per_pixel(uint32_t format)
{
	switch (format)
	{
	default:
		return 0;

	case DRM_FORMAT_C8:
	case DRM_FORMAT_R8:
	case DRM_FORMAT_RGB332:
	case DRM_FORMAT_BGR233:
		return 8;

	case DRM_FORMAT_XRGB4444:
	case DRM_FORMAT_XBGR4444:
	case DRM_FORMAT_RGBX4444:
	case DRM_FORMAT_BGRX4444:
	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_BGRA4444:
	case DRM_FORMAT_XRGB1555:
	case DRM_FORMAT_XBGR1555:
	case DRM_FORMAT_RGBX5551:
	case DRM_FORMAT_BGRX5551:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
		return 16;

	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return 24;

	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_XBGR2101010:
	case DRM_FORMAT_RGBX1010102:
	case DRM_FORMAT_BGRX1010102:
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_BGRA1010102:
		return 32;

	case DRM_FORMAT_ABGR16161616F:
	case DRM_FORMAT_AXBXGXRX106106106106:
		return 64;
	}
}

/**
 * @brief Return whether a FOURCC format is YUV.
 *
 * @param format A FOURCC format known by the DDK.
 *
 * @return Whether @p format is a YUV format. This function returns @c false for FOURCCs not known by the DDK,
 *   i.e. those for which drm_fourcc_format_to_string() returns "UNKNOWN-FOURCC".
 */
static inline bool drm_fourcc_is_yuv(uint32_t format)
{
	switch (format)
	{
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_AYUV:
	case DRM_FORMAT_Y410:
	case DRM_FORMAT_Y0L2:
	case DRM_FORMAT_Y210:
	case DRM_FORMAT_YUV420_8BIT:
	case DRM_FORMAT_YUV420_10BIT:
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
	case DRM_FORMAT_P010:
	case DRM_FORMAT_P016:
	case DRM_FORMAT_P210:
	case DRM_FORMAT_YUV410:
	case DRM_FORMAT_YVU410:
	case DRM_FORMAT_YUV411:
	case DRM_FORMAT_YVU411:
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YUV422:
	case DRM_FORMAT_YVU422:
	case DRM_FORMAT_YUV444:
	case DRM_FORMAT_YVU444:
	case DRM_FORMAT_NV15:
	case DRM_FORMAT_Q410:
	case DRM_FORMAT_Q401:
#if defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC
	case ARM_PRIV_DRM_FORMAT_AYUV2101010:
	case ARM_PRIV_DRM_FORMAT_YUYV10101010:
	case ARM_PRIV_DRM_FORMAT_VYUY10101010:
	case ARM_PRIV_DRM_FORMAT_Y_UV422_10BIT:
#endif /* defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC */
		return true;
	default:
		return false;
	}
}

/**
 * @brief Returns YUV FOURCC subsampling rate.
 *
 * @param format A FOURCC format known by the DDK.
 *
 * @return Subsampling of a YUV @p format. This function returns 0 for formats not known by the DDK.
 */
static inline uint32_t drm_fourcc_get_yuv_subsampling(uint32_t format)
{
	switch (format)
	{
	case DRM_FORMAT_Q410:
	case DRM_FORMAT_Q401:
	case DRM_FORMAT_Y410:
	case DRM_FORMAT_AYUV:
	case DRM_FORMAT_YUV444:
	case DRM_FORMAT_YVU444:
	case DRM_FORMAT_NV24:
	case DRM_FORMAT_NV42:
#if defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC
	case ARM_PRIV_DRM_FORMAT_AYUV2101010:
#endif /* defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC */
		return 444;
	case DRM_FORMAT_Y210:
	case DRM_FORMAT_P210:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_NV61:
	case DRM_FORMAT_YUV422:
	case DRM_FORMAT_YVU422:
#if defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC
	case ARM_PRIV_DRM_FORMAT_YUYV10101010:
	case ARM_PRIV_DRM_FORMAT_VYUY10101010:
	case ARM_PRIV_DRM_FORMAT_Y_UV422_10BIT:
#endif /* defined(MALI_PRIVATE_FOURCC) && MALI_PRIVATE_FOURCC */
		return 422;
	case DRM_FORMAT_P010:
	case DRM_FORMAT_Y0L2:
	case DRM_FORMAT_YUV420_10BIT:
	case DRM_FORMAT_NV15:
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YUV420_8BIT:
		return 420;
	}

	return 0;
}

/**
 * @brief Returns the number of components for the given plane for AFRC.
 *
 * @param fourcc FOURCC value of the format.
 * @param plane Format's plane index
 *
 * @return Number of components for the given plane, -1 on failure.
 */
static int get_afrc_components_per_plane(uint32_t fourcc, int plane)
{
	switch (fourcc)
	{
	case DRM_FORMAT_BGR888:
		assert(plane == 0);
		return 3;
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_XBGR8888:
		assert(plane == 0);
		return 4;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_P010:
	case DRM_FORMAT_P210:
	case DRM_FORMAT_NV16:
		assert(plane <= 1);
		return plane == 0 ? 1 : 2;
	case DRM_FORMAT_Q410:
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YUV444:
		assert(plane <= 2);
		return 1;
	}

	return -1;
}

/**
 * @brief Returns the paging tile dimensions for AFRC.
 *
 * @param modifier Format's modifier value.
 * @param width The location where the width of the paging tile will be set
 * @param height The location where the height of the paging tile will be set
 *
 */
static void get_afrc_paging_tile_dimensions(uint64_t modifier, uint32_t *width, uint32_t *height)
{
	if (modifier & AFRC_FORMAT_MOD_LAYOUT_SCAN)
	{
		*width = 16;
		*height = 4;
	}
	else
	{
		*width = 8;
		*height = 8;
	}
}

/**
 * @brief Returns the clump dimensions of an AFRC tile block
 *
 * @param fourcc FOURCC value of the format.
 * @param modifier Modifier value.
 * @param plane Format's plane index.
 * @param width The location where the width of the clump dimension will be set
 * @param height The location where the height of the clump dimension will be set
 *
 * @return 0 on success, -1 on failure.
 */
static int get_afrc_clump_dimensions(uint32_t fourcc, uint64_t modifier, int plane, uint32_t *width, uint32_t *height)
{
	switch (get_afrc_components_per_plane(fourcc, plane))
	{
	case 1:
		/* If there is a single component in a plane, the tile clump dimensions match paging tile dimensions */
		get_afrc_paging_tile_dimensions(modifier, width, height);
		break;
	case 2:
		*width = 8;
		*height = 4;
		break;
	case 3:
	case 4:
		*width = 4;
		*height = 4;
		break;
	default:
		return -1;
	}

	return 0;
}

/**
 * @brief Returns the number of vertical samples in a single block for the given plane.
 *
 * @param fourcc FOURCC value of the format.
 * @param modifier Modifier value.
 * @param plane Format's plane index
 *
 * @return Number of vertical samples on success, -1 on failure.
 *
 * Note: implemented for uncompressed, AFRC and block linear formats only.
 */
static inline int drm_format_get_block_height(uint32_t fourcc, uint64_t modifier, int plane)
{
	if (drm_fourcc_modifier_is_block_linear_yuv(modifier))
	{
		int height = 16;
		if (plane > 0 && drm_fourcc_get_yuv_subsampling(fourcc) == 420)
		{
			height /= 2;
		}
		return height;
	}
	else if (drm_fourcc_modifier_is_afrc(modifier))
	{
		uint32_t block_w = 0, block_h = 0;
		if (get_afrc_clump_dimensions(fourcc, modifier, plane, &block_w, &block_h) == 0)
		{
			return (int)block_h;
		}

		return -1;
	}
	else if (modifier == DRM_FORMAT_MOD_LINEAR)
	{
		return fourcc == DRM_FORMAT_Y0L2 ? 2 : 1;
	}
	else
	{
		return -1;
	}
}

/**
 * @brief Returns the number of horizontal samples in a single block for the given plane.
 *
 * @param fourcc   FOURCC value of the format.
 * @param modifier Modifier value.
 * @param plane    Format's plane index
 *
 * @return Number of horizontal samples on success, -1 on failure.
 *
 * Note: implemented for uncompressed, AFRC and block linear formats only.
 */
static inline int drm_format_get_block_width(uint32_t fourcc, uint64_t modifier, int plane)
{
	int width = -1;
	if (drm_fourcc_modifier_is_block_linear_yuv(modifier))
	{
		width = 16;
		const int subsampling = drm_fourcc_get_yuv_subsampling(fourcc);
		if (plane > 0 && ((subsampling == 422) || (subsampling == 420)))
		{
			width /= 2;
		}
	}
	else if (modifier == DRM_FORMAT_MOD_LINEAR)
	{
		switch (fourcc)
		{
		case DRM_FORMAT_Y0L2:
			width = 2;
			break;
		case DRM_FORMAT_NV15:
			width = 4;
			if (plane > 0)
			{
				width = 2;
			}
			break;
		default:
			width = 1;
			break;
		}
	}
	else if (drm_fourcc_modifier_is_afrc(modifier))
	{
		uint32_t block_w = 0, block_h = 0;
		if (get_afrc_clump_dimensions(fourcc, modifier, plane, &block_w, &block_h) == 0)
		{
			width = (int)block_w;
		}
	}

	return width;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MALI_FOURCC_H_ */
