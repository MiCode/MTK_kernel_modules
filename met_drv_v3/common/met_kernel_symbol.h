/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef MET_KERNEL_SYMBOL
#define MET_KERNEL_SYMBOL

/*lookup symbol*/
#include <asm/cpu.h>
#include <linux/kallsyms.h>
#include <linux/perf_event.h>
#include <linux/kthread.h>


#define _MET_SYMBOL_GET(_func_name_) ({\
	int ret = 0; \
	do { \
		_func_name_##_symbol = (void *)(&_func_name_); \
		if (_func_name_##_symbol == NULL) { \
			pr_debug("MET ext. symbol : %s is not found!\n", #_func_name_); \
			PR_BOOTMSG_ONCE("MET ext. symbol : %s is not found!\n", #_func_name_); \
			ret = -1; \
		} \
	} while (0); \
	ret; \
	})

#define _MET_SYMBOL_PUT(_func_name_) { \
		if (_func_name_##_symbol) { \
			_func_name_##_symbol = NULL; \
		} \
	}

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#ifndef MET_SCMI
#ifdef SSPM_VERSION_V2
extern struct mtk_ipi_device sspm_ipidev;
extern struct mtk_ipi_device *sspm_ipidev_symbol;
#endif /* SSPM_VERSION_V2 */
#endif /* MET_SCMI */
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

extern unsigned int mt_get_chip_id(void);
extern unsigned int (*mt_get_chip_id_symbol)(void);

#endif	/* MET_KERNEL_SYMBOL */
