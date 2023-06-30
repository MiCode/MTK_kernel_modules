/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _TRACE_H_
#define _TRACE_H_


extern void (*mp_cp_ptr)(unsigned long long timestamp,
	       struct task_struct *task,
	       unsigned long program_counter,
	       unsigned long dcookie,
	       unsigned long offset,
	       unsigned char cnt, unsigned int *value);

#define MP_FMT1	"%x\n"
#define MP_FMT2	"%x,%x\n"
#define MP_FMT3	"%x,%x,%x\n"
#define MP_FMT4	"%x,%x,%x,%x\n"
#define MP_FMT5	"%x,%x,%x,%x,%x\n"
#define MP_FMT6	"%x,%x,%x,%x,%x,%x\n"
#define MP_FMT7	"%x,%x,%x,%x,%x,%x,%x\n"
#define MP_FMT8	"%x,%x,%x,%x,%x,%x,%x,%x\n"
#define MP_FMT9	"%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define MP_FMT10 "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define MP_FMT11 "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define MP_FMT12 "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define MP_FMT13 "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define MP_FMT14 "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
#define MP_FMT15 "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"

#define MET_GENERAL_PRINT(FUNC, count, value) \
do { \
	switch (count) { \
	case 1: { \
		FUNC(MP_FMT1, value[0]); \
		} \
		break; \
	case 2: { \
		FUNC(MP_FMT2, value[0], value[1]); \
		} \
		break; \
	case 3: { \
		FUNC(MP_FMT3, value[0], value[1], value[2]); \
		} \
		break; \
	case 4: { \
		FUNC(MP_FMT4, value[0], value[1], value[2], value[3]); \
		} \
		break; \
	case 5: { \
		FUNC(MP_FMT5, value[0], value[1], value[2], value[3], value[4]); \
		} \
		break; \
	case 6: { \
		FUNC(MP_FMT6, value[0], value[1], value[2], value[3], value[4], value[5]); \
		} \
		break; \
	case 7: { \
		FUNC(MP_FMT7, value[0], value[1], value[2], value[3], value[4], value[5], value[6]); \
		} \
		break; \
	case 8: { \
		FUNC(MP_FMT8, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7]); \
		} \
		break; \
	case 9: { \
		FUNC(MP_FMT9, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], \
				value[8]); \
		} \
		break; \
	case 10: { \
		FUNC(MP_FMT10, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], \
				value[8], value[9]); \
		} \
		break; \
	case 11: { \
		FUNC(MP_FMT11, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], \
				value[8], value[9], value[10]); \
		} \
		break; \
	case 12: { \
		FUNC(MP_FMT12, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], \
				value[8], value[9], value[10], value[11]); \
		} \
		break; \
	case 13: { \
		FUNC(MP_FMT13, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], \
				value[8], value[9], value[10], value[11], value[12]); \
		} \
		break; \
	case 14: { \
		FUNC(MP_FMT14, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], \
				value[8], value[9], value[10], value[11], value[12], value[13]); \
		} \
		break; \
	case 15: { \
		FUNC(MP_FMT15, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], \
				value[8], value[9], value[10], value[11], value[12], value[13], value[14]); \
		} \
		break; \
	} \
} while (0)
#endif /* _TRACE_H_ */
