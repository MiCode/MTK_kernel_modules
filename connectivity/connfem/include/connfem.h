/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*
 * The header is provided for connfem kernel module internal used.
 */

#ifndef __CONNFEM_H__
#define __CONNFEM_H__

#include <linux/version.h>
#include "connfem_api.h"
#include "connfem_container.h"
#include "connfem_dt.h"
#include "connfem_epaelna.h"
#include "connfem_cfg.h"
#include "connfem_sku.h"

/*******************************************************************************
 *				M A C R O S
 ******************************************************************************/
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#define CFM_IOC_MAGIC		0xCF

#define CFM_IOC_IS_AVAILABLE	_IOR(CFM_IOC_MAGIC, \
				     0x00, struct cfm_ioc_is_available)

/* Query flags names entries/size stat */
#define CFM_IOC_EPA_FN_STAT	_IOR(CFM_IOC_MAGIC, \
				     0x01, struct cfm_ioc_epa_fn_stat)

/* Get flags names */
#define CFM_IOC_EPA_FN		_IOR(CFM_IOC_MAGIC, \
				     0x02, struct cfm_ioc_epa_fn)

/* Get eFEM info */
#define CFM_IOC_EPA_INFO	_IOR(CFM_IOC_MAGIC, \
				     0x03, struct cfm_ioc_epa_info)

/* Query flags names & values entries/size stat */
#define CFM_IOC_EPA_FLAGS_STAT	_IOR(CFM_IOC_MAGIC, \
				     0x04, struct cfm_ioc_epa_flags_stat)

/* Get flags names & values */
#define CFM_IOC_EPA_FLAGS	_IOR(CFM_IOC_MAGIC, \
				     0x05, struct cfm_ioc_epa_flags)

/* Get HW name */
#define CFM_IOC_SKU_HW_NAME	_IOR(CFM_IOC_MAGIC, \
				     0x06, struct cfm_ioc_sku_hw_name)

#define CFM_PARAM_EPAELNA_HWID_INVALID	0xFFFFFFFF
#define CFM_PARAM_HWID_INVALID		0xFFFFFFFF

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
#define CFG_HWID_PMIC_SUPPORT 1
#endif

#define CONNFEM_HW_NAME_SIZE		64

/* Reduce the effort of porting, the following macros will be filled
 * in struct connfem_epa_context and struct connfem_sku_context.
 */
#define CFM_OPS_SKU \
	.free = cfm_sku_context_free, \
	.parse = cfm_dt_sku_parse, \
	.get_flags_config = cfm_sku_flags_config_get, \
	.get_available = cfm_sku_available_get, \
	.ctx_type = CONNFEM_TYPE_SKU

#define CFM_OPS_EPAELNA \
	.free = cfm_epa_context_free, \
	.parse = cfm_dt_epa_parse, \
	.get_flags_config = cfm_epaelna_flags_config_get, \
	.get_available = cfm_epaelna_available_get, \
	.ctx_type = CONNFEM_TYPE_EPAELNA

/*******************************************************************************
 *			    D A T A   T Y P E S
 ******************************************************************************/
enum cfm_src {
	CFM_SRC_DEVICE_TREE = 0,
	CFM_SRC_CFG_FILE = 1,
	CFM_SRC_NUM
};

struct connfem_context_ops {
	unsigned int id;
	struct platform_device *pdev;

	enum connfem_type ctx_type;
	enum cfm_src src;

	/* At the end of the program or when an exception occurs,
	 * the memory and the device nodes should be returned.
	 */
	void (*free)(void *cfm);

	/* Different parse DTS methods for different platforms */
	int (*parse)(void *cfm);

	/* Locate flags and config struct */
	int (*get_flags_config)(void *cfm,
			struct cfm_epaelna_flags_config **flags_config);

	/* Find out if eFEM is available for the current platform */
	int (*get_available)(void *cfm, bool *avail);
};

struct connfem_epa_context {
	struct connfem_context_ops ops;
	struct cfm_dt_context dt;
	struct cfm_epaelna_config epaelna;
};

/* Note that hwid, available, and sku will be copied in
 * cfm_cfg_sku_context_copy via offsetof. Be careful when
 * modifying this struct here. Config file may correspondingly
 * need to be updated.
*/
struct connfem_sku_context {
	struct connfem_context_ops ops;
	unsigned int hwid;	/* hardware description */
	bool available;

	struct connfem_sku sku;

	struct cfm_dt_epaelna_flags_context flags;
	struct cfm_epaelna_flags_config flags_cfg[CONNFEM_SUBSYS_NUM];
};

struct cfm_ioc_is_available {
	/* IN */
	enum connfem_type fem_type;
	enum connfem_subsys subsys;

	/* OUT */
	bool is_available;
};

struct cfm_ioc_epa_fn_stat {
	/* IN */
	enum connfem_subsys subsys;

	/* OUT */
	unsigned int cnt;
	unsigned int entry_sz;
};

struct cfm_ioc_epa_fn {
	/* IN */
	enum connfem_subsys subsys;

	/* IN/OUT */
	/* struct cfm_container*
	 *   entry: char[CONNFEM_FLAG_NAME_SIZE], zero-terminated
	 */
	uint64_t names;
};

struct cfm_ioc_epa_info {
	/* OUT - Aligned with connfem_epaelna_fem_info */
	unsigned int id;
	struct connfem_part part[CONNFEM_PORT_NUM];
	char part_name[CONNFEM_PORT_NUM][CONNFEM_PART_NAME_SIZE];
};

struct cfm_ioc_epa_flags_stat {
	/* IN */
	enum connfem_subsys subsys;

	/* OUT */
	unsigned int cnt;
	unsigned int entry_sz;
};

struct cfm_ioc_epa_flags {
	/* IN */
	enum connfem_subsys subsys;

	/* IN/OUT */
	/* struct cfm_container*
	 *   entry: connfem_epaelna_flag_pair
	 */
	uint64_t pairs;
};

struct cfm_ioc_sku_hw_name {
	/* OUT */
	/* len: count does not include zero terminator
	*/
	unsigned int len;
	char name[CONNFEM_HW_NAME_SIZE];
};

/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/
extern void *connfem_ctx;
extern void *cfm_ctx[CONNFEM_TYPE_NUM];

extern char *cfm_subsys_name[CONNFEM_SUBSYS_NUM];

/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
extern bool connfem_is_internal(void);
extern void cfm_epa_context_free(void *ctx);
extern void cfm_sku_context_free(void *ctx);
extern unsigned int cfm_param_epaelna_hwid(void);
extern unsigned int *cfm_param_hwid(void);
extern const char *cfm_param_hw_name(void);
extern int cfm_param_hw_name_set(const char *str, size_t sz);

#endif /* __CONNFEM_H__ */
