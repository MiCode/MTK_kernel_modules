/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _SRC_UTIL_H_
#define _SRC_UTIL_H_

/* #define FILELOG 1 */

#ifdef FILELOG
void filelog(char *str);
#else
#define filelog(str)
#endif

#endif				/* _SRC_UTIL_H_ */
