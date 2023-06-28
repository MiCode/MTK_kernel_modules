/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "met_api.h"

/* define EXTERNAL_SYMBOL_FUNC_MODE to followings to decide using which mode
 * EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DECLARE, or no define: for declaration in core_plf_init.h
 * EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE: for variable definition in core_plf_init.c
 * EXTERNAL_SYMBOL_FUNC_MODE_MOD_LINK: for link external in met_xxx_api.c
 * EXTERNAL_SYMBOL_FUNC_MODE_MOD_INIT: for assign in met_xxx_api.c module_init
 * EXTERNAL_SYMBOL_FUNC_MODE_MOD_EXIT: for assign in met_xxx_api.c module_exit
 */
#ifndef EXTERNAL_SYMBOL_FUNC_MODE
#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DECLARE
#endif

#ifdef EXTERNAL_SYMBOL_FUNC
#undef EXTERNAL_SYMBOL_FUNC
#endif

#if EXTERNAL_SYMBOL_FUNC_MODE == EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DECLARE

#define EXTERNAL_SYMBOL_FUNC(ret,name,param...) \
    extern ret (*name ## _symbol)(param);

#elif EXTERNAL_SYMBOL_FUNC_MODE == EXTERNAL_SYMBOL_FUNC_MODE_SYMBOL_DEFINE

#define EXTERNAL_SYMBOL_FUNC(ret,name,param...) \
    ret (*name ## _symbol)(param) = NULL; \
    EXPORT_SYMBOL(name ## _symbol);

#elif EXTERNAL_SYMBOL_FUNC_MODE == EXTERNAL_SYMBOL_FUNC_MODE_MOD_LINK

#define EXTERNAL_SYMBOL_FUNC(ret,name,param...) \
    extern ret name(param); \
    extern ret (*name ## _symbol)(param);

#elif EXTERNAL_SYMBOL_FUNC_MODE == EXTERNAL_SYMBOL_FUNC_MODE_MOD_INIT

#define EXTERNAL_SYMBOL_FUNC(ret,name,param...) \
    name ## _symbol = name;

#elif EXTERNAL_SYMBOL_FUNC_MODE == EXTERNAL_SYMBOL_FUNC_MODE_MOD_EXIT

#define EXTERNAL_SYMBOL_FUNC(ret,name,param...) \
    name ## _symbol = NULL;

#endif /* EXTERNAL_SYMBOL_FUNC_MODE */

#ifdef EXTERNAL_SYMBOL_FUNC_MODE
#undef EXTERNAL_SYMBOL_FUNC_MODE
#endif