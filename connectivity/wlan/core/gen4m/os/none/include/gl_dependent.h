/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_dependency.h
 * \brief  List the os-dependent structure/API that need to implement
 * to align with common part
 *
 * not to modify *.c while put current used linux-type strucutre/API here
 * For porting to new OS, the API listed in this file needs to be
 * implemented
 */

#ifndef _GL_DEPENDENT_H
#define _GL_DEPENDENT_H

#ifndef IS_ENABLED
#define IS_ENABLED(_val) defined(_val)
#endif
/*
 * TODO: implement defined structure to let
 * other OS aligned to linux style for common part logic workaround
 */
/*
 * TODO: os-related?, CHN_DIRTY_WEIGHT_UPPERBOUND
 * should we remove gl_*_ioctl.h in os/none?
 * defined in os/linux/include/gl_p2p_ioctl.h
 * used in
 * 1) os/linux/gl_p2p_cfg80211.c
 * 2) mgmt/cnm.c
 * > is it related to ioctl?, or should it put in wlan_p2p.h
 */
#if 0
#define CHN_DIRTY_WEIGHT_UPPERBOUND     4
#endif

/* some arch's have a small-data section that can be accessed register-relative
 * but that can only take up to, say, 4-byte variables. jiffies being part of
 * an 8-byte variable may not be correctly accessed unless we force the issue
 * #define __jiffy_data  __attribute__((section(".data")))
 *
 * The 64-bit value is not atomic - you MUST NOT read it
 * without sampling the sequence number in jiffies_lock.

 * extern u64 __jiffy_data jiffies_64;
 * extern unsigned long volatile __jiffy_data jiffies;
 * TODO: no idea how to implement jiffies here
 */
#ifndef HZ
#define HZ (1000)
#endif
/*
 * comment: cipher type should not related to os
 * defined in os/linux/gl_wext.h,
 * while it looks like also defined in linux/wireless.h
 * are we trying to be an self-fulfilled driver then
 * should not put it under os folder? or we should just include from linux
 */
/* IW_AUTH_PAIRWISE_CIPHER and IW_AUTH_GROUP_CIPHER values (bit field) */
#define IW_AUTH_CIPHER_NONE     0x00000001
#define IW_AUTH_CIPHER_WEP40    0x00000002
#define IW_AUTH_CIPHER_TKIP     0x00000004
#define IW_AUTH_CIPHER_CCMP     0x00000008
#define IW_AUTH_CIPHER_WEP104   0x00000010

/*
 * comment:
 * 1) access GlueInfo member from core logic
 * 2) IW_AUTH_WPA_VERSION_DISABLED is defined in os/linux/include/gl_wext.h
 * > is the an os-dependent value/ protocol value.
 * should implement depends on OS/ its WiFi
 * needed by,
 * mgmt/ais_fsm.c
 * mgmt/assoc.c
 */
/* IW_AUTH_WPA_VERSION values (bit field) */
#define IW_AUTH_WPA_VERSION_DISABLED    0x00000001
#define IW_AUTH_WPA_VERSION_WPA         0x00000002
#define IW_AUTH_WPA_VERSION_WPA2        0x00000004

#define IW_AUTH_ALG_FT			0x00000008
#define IW_AUTH_ALG_SAE			0x00000010
#define IW_AUTH_ALG_FILS_SK		0x00000020
#define IW_AUTH_ALG_FILS_SK_PFS		0x00000040

#define IW_PMKID_LEN        16

#define SUITE(oui, id)	(((oui) << 8) | (id))
#define WLAN_AKM_SUITE_SAE			SUITE(0x000FAC, 8)
#define WLAN_EID_BSS_MAX_IDLE_PERIOD 90
#define WLAN_EID_VENDOR_SPECIFIC 221

/*
 * this highly depends on kernel version
 * why can't we just use kalGetTimeTick (?)
 * needed by
 * wmm.c
 */
#if 0
struct timespec {
	__kernel_time_t	tv_sec;			/* seconds */
	long tv_nsec;		/* nanoseconds */
};
#endif
/*
 * needed by cmm_asic_connac.c
 *	Potential risk in this function
 *	#ifdef CONFIG_PHYS_ADDR_T_64BIT
 *	typedef u64 phys_addr_t;
 *	#else
 *	typedef u32 phys_addr_t;
 *	#endif
 *	while anyway the rDmaAddr is transfer to u8Addr still u64
 *	, and filled into u4Ptr0 which is uint32_t (?)
 */
#define phys_addr_t uint32_t

/*
 * needed by nic_tx.h
 * defined in linux/types.h
 * #ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
 * typedef u64 dma_addr_t;
 * #else
 * typedef u32 dma_addr_t;
 * #endif
 */
#define dma_addr_t uint32_t

/* needed by:
 * common/debug.c
 * source/include/linux/printk.h
 */
enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_ADDRESS,
	DUMP_PREFIX_OFFSET
};

/*
 * needed by mgmt/tdls.c
 */
enum gfp_t {
	GFP_KERNEL,
	GFP_ATOMIC,
	__GFP_HIGHMEM,
	__GFP_HIGH
};
/* need by include/hal.h, halDeAggRxPktWorker
 * comment: use os-related structure directly outside headers of gl layer
 * while the implementation is in os/linux/hif*
 * possible actions:
 * 1) should we just move the function prototype to os gl layer?
 */
struct work_struct {
	/* atomic_long_t data; */
	/* struct list_head entry; */
	/* work_func_t func; */
};
/*
 * TODO: Functions need implementation
 */

/****************************************************************************
 * TODO: Functions prototype, which could be realized as follows
 * 1) inline function
 * 2) os API with same functionality
 * 3) implemented in gl_dependent.c
 ****************************************************************************
 */
/*
 * KAL_NEED_IMPLEMENT: wrapper to caution user to implement func when porting
 * @file: from which file
 * @func: called by which func
 * @line: at which line
 */
long KAL_NEED_IMPLEMENT(const char *file, const char *func, int line, ...);
#define pr_info(fmt, ...) printf(fmt)

/*	implemented: os/linux/gl_wext.c
 *	used: common/wlan_oid.c, wlanoidSetWapiAssocInfo
 *	why function called "search WPAIIE" has been implement under wext
 *	first used in wext_get_scan. Should it be part of wlan_oid?
 */
#if CFG_SUPPORT_WAPI
#define wextSrchDesiredWAPIIE(_pucIEStart, _i4TotalIeLen, \
	_ppucDesiredIE) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
#endif
#endif
