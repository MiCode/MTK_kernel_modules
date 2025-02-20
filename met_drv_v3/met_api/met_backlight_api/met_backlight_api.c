// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/module.h>
#include "met_api.h"

#ifdef MET_BACKLIGHT

extern long met_backlight_api_ready;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_LINK
#include "met_backlight_api/met_backlight_api.h"

#endif /* MET_BACKLIGHT */

static int __init met_api_init(void)
{
#ifdef MET_BACKLIGHT

    met_backlight_api_ready = 1;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_INIT
#include "met_backlight_api/met_backlight_api.h"

#endif /* MET_BACKLIGHT */
    return 0;
}

static void __exit met_api_exit(void)
{
#ifdef MET_BACKLIGHT

    met_backlight_api_ready = 0;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_EXIT
#include "met_backlight_api/met_backlight_api.h"

#endif /* MET_BACKLIGHT */
}

module_init(met_api_init);
module_exit(met_api_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_CORE");
MODULE_LICENSE("GPL");