/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __UT_FS_TEST_H__
#define __UT_FS_TEST_H__


#include <stdio.h>


/******************************************************************************/
// CMD printf color
/******************************************************************************/
#define NONE           "\033[m"
#define RED            "\033[0;32;31m"
#define LIGHT_RED      "\033[1;31m"
#define GREEN          "\033[0;32;32m"
#define LIGHT_GREEN    "\033[1;32m"
#define BLUE           "\033[0;32;34m"
#define LIGHT_BLUE     "\033[1;34m"
#define DARY_GRAY      "\033[1;30m"
#define CYAN           "\033[0;36m"
#define LIGHT_CYAN     "\033[1;36m"
#define PURPLE         "\033[0;35m"
#define LIGHT_PURPLE   "\033[1;35m"
#define BROWN          "\033[0;33m"
#define YELLOW         "\033[1;33m"
#define LIGHT_GRAY     "\033[0;37m"
#define WHITE          "\033[1;37m"
/******************************************************************************/


/******************************************************************************
 * unit test printf
 *****************************************************************************/
#define UT_INF(format, args...) \
	printf(LIGHT_GREEN PFX "[%s] " format NONE, __func__, ##args)

#define UT_ERR(format, args...) \
	printf(LIGHT_RED PFX "[%s] " format NONE, __func__, ##args)


#define UT_FUNC_START() \
do { \
	UT_INF("\n"); \
	UT_INF(">>>>>>>>>>>>>>>>>>>>==================<<<<<<<<<<<<<<<<<<<<\n"); \
	UT_INF("\n"); \
} while (0) \

#define UT_FUNC_END() \
do { \
	UT_INF("\n"); \
	UT_INF(">>>>>>>>>>>>>>>>>>>>==================<<<<<<<<<<<<<<<<<<<<\n"); \
	UT_INF("\n"); \
	printf("\n\n\n"); \
} while (0) \


#endif
