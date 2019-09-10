/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
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
