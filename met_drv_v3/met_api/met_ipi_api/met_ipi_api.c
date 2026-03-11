// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/module.h>
#include "met_api.h"

#if defined(MET_MCUPM) || defined(MET_GPUEB) /* anyone of the ipi users */

extern long met_ipi_api_ready;

#include "mtk_tinysys_ipi.h"

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_LINK
#include "met_ipi_api/met_ipi_api.h"

#endif /* MET_MCUPM || MET_GPUEB (anyone of the ipi users) */

#ifdef MET_MCUPM

extern int met_mcupm_log_init(void);
extern int met_mcupm_log_uninit(void);
extern int met_ondiemet_attr_init_mcupm(void);
extern int met_ondiemet_attr_uninit_mcupm(void);

#endif /* MET_MCUPM */

#ifdef MET_GPUEB

extern int met_gpueb_log_init(void);
extern int met_gpueb_log_uninit(void);
extern int met_ondiemet_attr_init_gpueb(void);
extern int met_ondiemet_attr_uninit_gpueb(void);

#endif /* MET_GPUEB */

static int __init met_api_init(void)
{
#if defined(MET_MCUPM) || defined(MET_GPUEB) /* anyone of the ipi users */

    met_ipi_api_ready = 1;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_INIT
#include "met_ipi_api/met_ipi_api.h"

#endif /* MET_MCUPM || MET_GPUEB (anyone of the ipi users) */

#ifdef MET_MCUPM

    met_mcupm_log_init();
    met_ondiemet_attr_init_mcupm();

#endif /* MET_MCUPM */

#ifdef MET_GPUEB

    met_gpueb_log_init();
    met_ondiemet_attr_init_gpueb();

#endif /* MET_GPUEB */

    return 0;
}

static void __exit met_api_exit(void)
{
#ifdef MET_MCUPM

    met_mcupm_log_uninit();
    met_ondiemet_attr_uninit_mcupm();

#endif /* MET_MCUPM */

#ifdef MET_GPUEB

    met_gpueb_log_uninit();
    met_ondiemet_attr_uninit_gpueb();

#endif /* MET_GPUEB */

#if defined(MET_MCUPM) || defined(MET_GPUEB) /* anyone of the ipi users */

    met_ipi_api_ready = 0;

#define EXTERNAL_SYMBOL_FUNC_MODE EXTERNAL_SYMBOL_FUNC_MODE_MOD_EXIT
#include "met_ipi_api/met_ipi_api.h"

#endif /* MET_MCUPM || MET_GPUEB (anyone of the ipi users) */
}

module_init(met_api_init);
module_exit(met_api_exit);

MODULE_AUTHOR("DT_DM5");
MODULE_DESCRIPTION("MET_CORE");
MODULE_LICENSE("GPL");