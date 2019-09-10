/*
 *  Copyright (c) 2016,2017 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __BTMTK_DEFINE_H__
#define __BTMTK_DEFINE_H__

#include "btmtk_config.h"

/**
 * Type definition
 */
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#ifndef UNUSED
	#define UNUSED(x) (void)(x)
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * Log level definition
 */
#define BTMTK_LOG_LEVEL_ERROR		1
#define BTMTK_LOG_LEVEL_WARNING		2
#define BTMTK_LOG_LEVEL_INFO		3
#define BTMTK_LOG_LEVEL_DEBUG		4
#define BTMTK_LOG_LEVEL_DEFAULT		BTMTK_LOG_LEVEL_INFO	/* default setting */

#if defined(CONFIG_DEBUG_FS) && (CONFIG_DEBUG_FS == 1)
extern u8 btmtk_log_lvl;

#define BTSDIO_RAW_PR_INFO(p, l, fmt, ...)						\
	do {									\
		{			\
			int raw_count = 0;					\
			const unsigned char *ptr = p;				\
			pr_info("[btmtk_info] "fmt, ##__VA_ARGS__);		\
			for (raw_count = 0; raw_count <= l; ++raw_count) {	\
				pr_info(" %02X", ptr[raw_count]);		\
			}							\
			pr_info("\n");						\
		}								\
	} while (0)

#define BTSDIO_RAW_PR_DEBUG(p, l, fmt, ...)						\
	do {									\
		{			\
			int raw_count = 0;					\
			const unsigned char *ptr = p;				\
			pr_debug("[btmtk_info] "fmt, ##__VA_ARGS__);		\
			for (raw_count = 0; raw_count <= l; ++raw_count) {	\
				pr_debug(" %02X", ptr[raw_count]);		\
			}							\
			pr_debug("\n");						\
		}								\
	} while (0)
#endif /* CONFIG_DEBUG_FS */

/**
 *
 * HCI packet type
 */
#define MTK_HCI_COMMAND_PKT	 0x01
#define MTK_HCI_ACLDATA_PKT		0x02
#define MTK_HCI_SCODATA_PKT		0x03
#define MTK_HCI_EVENT_PKT		0x04

/**
 * Log file path & name, the default path is /sdcard
 */
#define SYSLOG_FNAME			"bt_sys_log"
#define FWDUMP_FNAME			"bt_fw_dump"
#ifdef BTMTK_LOG_PATH
	#define SYS_LOG_FILE_NAME	(BTMTK_LOG_PATH SYSLOG_FNAME)
	#define FW_DUMP_FILE_NAME	(BTMTK_LOG_PATH FWDUMP_FNAME)
#else
	#define SYS_LOG_FILE_NAME	"/sdcard/"SYSLOG_FNAME
	#define FW_DUMP_FILE_NAME	"/sdcard/"FWDUMP_FNAME
#endif /* FW_DUMP_FILE_NAME */

/**
 * Maximum rom patch file name length
 */
#define MAX_BIN_FILE_NAME_LEN 32



#endif /* __BTMTK_DEFINE_H__ */
