// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/module.h>
#include "met_api.h"

#ifdef MET_SSPM

extern long met_sspm_api_ready;

#include "sspm_reservedmem.h"
#include "met_drv.h"

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_LINK
#include "met_sspm_api/met_sspm_api.h"

int met_register(struct metdevice *met);
int met_deregister(struct metdevice *met);
extern struct metdevice met_sspm_emi;
#ifdef MET_PLF_EXIST
extern struct metdevice met_emi;
#endif
extern int met_sspm_log_init(void);
extern int met_sspm_log_uninit(void);
extern int met_ondiemet_attr_init_sspm(void);
extern int met_ondiemet_attr_uninit_sspm(void);

#endif /* MET_SSPM */

static int __init met_api_init(void)
{
#ifdef MET_SSPM
    int met_sspm_emi_ret;
#ifdef MET_PLF_EXIST
    int met_emi_ret;
#endif

    met_sspm_emi_ret = met_deregister(&met_sspm_emi);
#ifdef MET_PLF_EXIST
    met_emi_ret = met_deregister(&met_emi);
#endif

    met_sspm_api_ready = 1;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_INIT
#include "met_sspm_api/met_sspm_api.h"

    met_sspm_log_init();
    met_ondiemet_attr_init_sspm();

    if (!met_sspm_emi_ret) met_register(&met_sspm_emi);
#ifdef MET_PLF_EXIST
    if (!met_emi_ret) met_register(&met_emi);
#endif

#endif /* MET_SSPM */
    return 0;
}

static void __exit met_api_exit(void)
{
#ifdef MET_SSPM
    int met_sspm_emi_ret;
#ifdef MET_PLF_EXIST
    int met_emi_ret;
#endif

    met_sspm_emi_ret = met_deregister(&met_sspm_emi);
#ifdef MET_PLF_EXIST
    met_emi_ret = met_deregister(&met_emi);
#endif

    met_sspm_log_uninit();
    met_ondiemet_attr_uninit_sspm();

    met_sspm_api_ready = 0;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_EXIT
#include "met_sspm_api/met_sspm_api.h"

    if (!met_sspm_emi_ret) met_register(&met_sspm_emi);
#ifdef MET_PLF_EXIST
    if (!met_emi_ret) met_register(&met_emi);
#endif

#endif /* MET_SSPM */
}

module_init(met_api_init);
module_exit(met_api_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_CORE");
MODULE_LICENSE("GPL");