/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __CONNFEM_SKU_H__
#define __CONNFEM_SKU_H__

/*******************************************************************************
 *				M A C R O S
 ******************************************************************************/
#define DT_SKUS_HW_PROP_SIZE		4
#define DT_SKUS_HW_PROP_NAME_SIZE	64

#define CONNFEM_SKU_INVALID_IDX		0xFFFFFFFF
#define CONNFEM_SKU_LOG_SIZE		256

/*******************************************************************************
 *			    D A T A   T Y P E S
 ******************************************************************************/
enum cfm_array_type {
	CFM_ARRAY_TYPE_UINT,
	CFM_ARRAY_TYPE_CHAR,
	CFM_ARRAY_TYPE_NUM
};

/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/

/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/

extern int cfm_sku_fem_info_populate(struct device_node *np,
		struct connfem_sku_fem_info *fem_info);

extern int cfm_sku_fem_ttbl_populate(struct device_node *np,
		struct connfem_sku_fem_truth_table *tt);

extern int cfm_sku_fem_ctrl_pin_populate(struct device_node *np,
		struct connfem_sku_fem_ctrlpin *ctrl_pin);

extern int cfm_sku_fem_ttbl_usg_populate(struct device_node *np,
		struct connfem_sku_fem_truth_table_usage *tt_usage);

extern int cfm_dt_prop_substr_np_fetch(struct device_node *np,
		const char *prop_name, const char *needle,
		struct device_node **matched_np);

extern int cfm_sku_fem_layout_populate(struct device_node *np,
		struct connfem_sku *sku,
		struct connfem_sku_layout *layout);

extern void cfm_skus_pininfo_dump(unsigned int pin_count,
		struct connfem_sku_pinmap *pinmap);

extern int cfm_sku_generic_layout_hdl(struct device_node *np,
		unsigned int *pin_cnt,
		struct connfem_sku_pinmap *pinmap);

extern int cfm_sku_prop_val_get(struct device_node *np,
		const char* propname,
		unsigned int *res_val);

extern int cfm_sku_ttbl_usg_hdl(struct device_node *np,
		struct connfem_sku *sku);

extern int cfm_sku_flags_config_get(void* ctx,
		struct cfm_epaelna_flags_config** flags_config);

extern int cfm_sku_available_get(void* ctx, bool *avail);

extern void cfm_sku_data_dump(struct connfem_sku *sku);

#endif /* __CONNFEM_SKU_H__ */
