// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include "connfem.h"

/*******************************************************************************
 *				M A C R O S
 ******************************************************************************/
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "["KBUILD_MODNAME"][API]" fmt

/*******************************************************************************
 *			    D A T A   T Y P E S
 ******************************************************************************/


/*******************************************************************************
 *		    F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************/


/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/


/*******************************************************************************
 *			   P R I V A T E   D A T A
 ******************************************************************************/


/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
bool connfem_is_available(enum connfem_type fem_type)
{
	int err;
	struct connfem_context_ops *ops;
	bool available;

	if (!connfem_ctx) {
		pr_info("%s, No ConnFem context", __func__);
		return false;
	}

	ops = (struct connfem_context_ops *)connfem_ctx;

	if (ops->ctx_type != fem_type) {
		/*
		pr_info("%s: actual FEM type: %d, input FEM type: %d",
			__func__, ops->ctx_type, fem_type);
		*/
		return false;
	}

	err = ops->get_available(connfem_ctx, &available);

	if (err < 0 || available == false) {
		pr_info("%s: FEM is not available",
			__func__);
		return false;
	}

	return true;
}
EXPORT_SYMBOL(connfem_is_available);

int connfem_epaelna_get_fem_info(struct connfem_epaelna_fem_info *fem_info)
{
	struct connfem_epa_context *cfm;

	if (!fem_info) {
		pr_info("%s, input parameter is NULL", __func__);
		return -EINVAL;
	}

	memset(fem_info, 0, sizeof(*fem_info));

	if (!connfem_is_available(CONNFEM_TYPE_EPAELNA)) {
		pr_info("%s, Not support", __func__);
		return -EOPNOTSUPP;
	}

	cfm = (struct connfem_epa_context *)connfem_ctx;

	memcpy(fem_info, &cfm->epaelna.fem_info, sizeof(*fem_info));

	pr_info("GetFemInfo");
	cfm_epaelna_feminfo_dump(fem_info);

	return 0;
}
EXPORT_SYMBOL(connfem_epaelna_get_fem_info);

int connfem_epaelna_get_bt_fem_info(struct connfem_epaelna_fem_info *fem_info)
{
	struct connfem_epa_context *cfm;

	if (!fem_info) {
		pr_info("%s, input parameter is NULL", __func__);
		return -EINVAL;
	}

	memset(fem_info, 0, sizeof(*fem_info));

	if (!connfem_is_available(CONNFEM_TYPE_EPAELNA)) {
		pr_info("%s, Not support", __func__);
		return -EOPNOTSUPP;
	}

	cfm = (struct connfem_epa_context *)connfem_ctx;

	memcpy(fem_info, &cfm->epaelna.bt_fem_info, sizeof(*fem_info));

	pr_info("GetFemInfo");
	cfm_epaelna_feminfo_dump(fem_info);

	return 0;
}
EXPORT_SYMBOL(connfem_epaelna_get_bt_fem_info);


int connfem_epaelna_get_pin_info(struct connfem_epaelna_pin_info *pin_info)
{
	int total_pin_count = 0;
	int pin_count = 0;
	struct connfem_epa_context *cfm;

	if (!pin_info) {
		pr_info("%s, input parameter is NULL", __func__);
		return -EINVAL;
	}

	memset(pin_info, 0, sizeof(*pin_info));

	if (!connfem_is_available(CONNFEM_TYPE_EPAELNA)) {
		pr_info("%s, Not support", __func__);
		return -EOPNOTSUPP;
	}

	cfm = (struct connfem_epa_context *)connfem_ctx;

	total_pin_count = cfm->epaelna.pin_cfg.pin_info.count +
		cfm->epaelna.bt_pin_cfg.pin_info.count;

	if (total_pin_count >= CONNFEM_EPAELNA_PIN_COUNT) {
		pr_info("%s, pin count:%d > %d",
			__func__,
			total_pin_count,
			CONNFEM_EPAELNA_PIN_COUNT);
		return -EOPNOTSUPP;
	}

	pin_info->count = total_pin_count;
	pin_count = cfm->epaelna.pin_cfg.pin_info.count;

	if (cfm->epaelna.pin_cfg.pin_info.count > 0) {
		memcpy(pin_info->pin,
			&cfm->epaelna.pin_cfg.pin_info.pin,
			cfm->epaelna.pin_cfg.pin_info.count *
			sizeof(struct connfem_epaelna_pin));
	}

	if (cfm->epaelna.bt_pin_cfg.pin_info.count > 0) {
		memcpy(pin_info->pin + pin_count,
			&cfm->epaelna.bt_pin_cfg.pin_info.pin,
			cfm->epaelna.bt_pin_cfg.pin_info.count *
			sizeof(struct connfem_epaelna_pin));
	}

	pr_info("GetPinInfo");
	cfm_epaelna_pininfo_dump(pin_info);

	return 0;
}
EXPORT_SYMBOL(connfem_epaelna_get_pin_info);

int connfem_epaelna_laa_get_pin_info(
		struct connfem_epaelna_laa_pin_info *laa_pin_info)
{
	struct connfem_epa_context *cfm;
	struct connfem_context_ops *ops;

	if (!laa_pin_info) {
		pr_info("%s, input parameter is NULL", __func__);
		return -EINVAL;
	}

	memset(laa_pin_info, 0, sizeof(*laa_pin_info));

	if (!connfem_is_available(CONNFEM_TYPE_EPAELNA)) {
		pr_info("%s, Not support", __func__);
		return -EOPNOTSUPP;
	}

	cfm = (struct connfem_epa_context *)connfem_ctx;
	ops = (struct connfem_context_ops *)connfem_ctx;

	memcpy(laa_pin_info, &cfm->epaelna.pin_cfg.laa_pin_info,
	       sizeof(*laa_pin_info));

	/* Assign chip id to connfem_epaelna_laa_pin_info
	 * due to FW (COEX LAA 4x4 module) need this information.
	 */
	laa_pin_info->chip_id = ops->id;

	pr_info("GetLaaPinInfo");
	cfm_epaelna_laainfo_dump(laa_pin_info);

	return 0;
}
EXPORT_SYMBOL(connfem_epaelna_laa_get_pin_info);

int connfem_epaelna_get_flags(enum connfem_subsys subsys, void *flags)
{
	struct connfem_epa_context *cfm;

	if (subsys >= CONNFEM_SUBSYS_NUM) {
		pr_info("%s, invalid subsys %d",
			__func__, subsys);
		return -EINVAL;
	}

	if (!flags) {
		pr_info("%s, input parameter is NULL", __func__);
		return -EINVAL;
	}

	if (!connfem_is_available(CONNFEM_TYPE_EPAELNA)) {
		pr_info("%s, Not support", __func__);
		return -EOPNOTSUPP;
	}

	cfm = (struct connfem_epa_context *)connfem_ctx;

	if (!cfm->epaelna.flags_cfg[subsys].obj) {
		pr_info("%s, subsys %d '%s' flags is NULL",
			__func__, subsys, cfm_subsys_name[subsys]);
		return -EINVAL;
	}

	switch (subsys) {
	case CONNFEM_SUBSYS_NONE:
		memcpy(flags, cfm->epaelna.flags_cfg[subsys].obj,
		       sizeof(struct connfem_epaelna_flags_common));
		break;

	case CONNFEM_SUBSYS_WIFI:
		memcpy(flags, cfm->epaelna.flags_cfg[subsys].obj,
		       sizeof(struct connfem_epaelna_flags_wifi));
		break;

	case CONNFEM_SUBSYS_BT:
		memcpy(flags, cfm->epaelna.flags_cfg[subsys].obj,
		       sizeof(struct connfem_epaelna_flags_bt));
		break;

	default:
		pr_info("%s, unknown subsys: %d", __func__, subsys);
		return -EINVAL;
	}

	pr_info("GetFlags");
	cfm_epaelna_flags_obj_dump(subsys, flags);

	return 0;
}
EXPORT_SYMBOL(connfem_epaelna_get_flags);

int connfem_sku_data(const struct connfem_sku **sku)
{
	if (!sku) {
		pr_info("%s, No sku ptr input", __func__);
		return -EINVAL;
	}

	if (!connfem_is_available(CONNFEM_TYPE_SKU)) {
		pr_info("%s, SKU is not supported", __func__);
		return -EOPNOTSUPP;
	}

	*sku = &((struct connfem_sku_context *)connfem_ctx)->sku;

	return 0;
}
EXPORT_SYMBOL(connfem_sku_data);

int connfem_sku_flag_u8(enum connfem_subsys subsys,
			const char *name,
			unsigned char *value)
{
	struct connfem_context_ops *ops;
	struct cfm_epaelna_flags_config *flg_cfg;
	struct cfm_container *pairs;
	struct connfem_epaelna_flag_pair *pair;
	int err = 0;
	int i;

	if (!value || !name) {
		pr_info("%s, input parameter is NULL", __func__);
		return -EINVAL;
	}

	if (!connfem_ctx) {
		pr_info("%s, No ConnFem context", __func__);
		return -EOPNOTSUPP;
	}

	ops = (struct connfem_context_ops *)connfem_ctx;

	err = ops->get_flags_config(connfem_ctx, &flg_cfg);
	if (err < 0) {
		pr_info("%s, cannot get flags config", __func__);
		return -EINVAL;
	}
	pairs = flg_cfg[subsys].pairs;

	if (!pairs) {
		pr_info("%s, subsys %d '%s' pairs is NULL",
			__func__, subsys, cfm_subsys_name[subsys]);
		return -ENOENT;
	}

	for (i = 0; i < pairs->cnt; i++) {
		pair = (struct connfem_epaelna_flag_pair*)
			cfm_container_entry(pairs, i);
		if (pair && strncasecmp(pair->name,
				name,
				CONNFEM_FLAG_NAME_SIZE) == 0) {
			*value = (unsigned char)pair->value;
			return 0;
		}
	}

	return -ENOENT;
}
EXPORT_SYMBOL(connfem_sku_flag_u8);
