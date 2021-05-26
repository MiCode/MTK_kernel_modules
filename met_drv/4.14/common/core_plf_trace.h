/*
 * Copyright (C) 2019 MediaTek Inc.
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

#ifndef _CORE_PLF_TRACE_H_
#define _CORE_PLF_TRACE_H_

#define	HVALUE_SIZE	9	/* 8 chars (max value ffffffff) + 1 char (',' or NULL) */
#define	DVALUE_SIZE	12	/* 10 chars (max value 4,294,967,295) + 1 char (',' or NULL) */

char *ms_formatH(char *__restrict__ buf, unsigned char cnt, unsigned int *__restrict__ value);
char *core_ms_formatD(char *__restrict__ buf, unsigned char cnt, unsigned int *__restrict__ value);
char *ms_formatH_ulong(char *__restrict__ buf, unsigned char cnt,
		       unsigned long *__restrict__ value);
char *ms_formatD_ulong(char *__restrict__ buf, unsigned char cnt,
		       unsigned long *__restrict__ value);
char *ms_formatH_EOL(char *__restrict__ buf, unsigned char cnt, unsigned int *__restrict__ value);
char *ms_formatD_EOL(char *__restrict__ buf, unsigned char cnt, unsigned int *__restrict__ value);

#endif	/* _CORE_PLF_TRACE_H_ */
