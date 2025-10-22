// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
//#include <linux/err.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include "connfem.h"

/*******************************************************************************
 *				M A C R O S
 ******************************************************************************/
#define CFM_DT_PROP_VID		"vid"
#define CFM_DT_PROP_PID		"pid"
#define CFM_DT_PROP_FLAG	"flag"

/*******************************************************************************
 *			    D A T A   T Y P E S
 ******************************************************************************/
/*
 * When CONFIG_OF is not set, for_each_property_of_node() is not defined,
 * we need to define it to avoid build break.
 */
#ifndef CONFIG_OF
#define for_each_property_of_node(dn, pp) \
	for (pp = dn->properties; pp != NULL; pp = pp->next)
#endif

struct cfm_sku_tt_usage_mapping {
	const char* node_name;
	struct connfem_sku_fem_truth_table_usage*
		(*get_tt_usg)(struct connfem_sku*, unsigned int);
};

/*******************************************************************************
 *		    F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************/
static int cfm_sku_pinctrl_mapping_populate(
		struct device_node *dt,
		unsigned int mappings,
		unsigned int *pin_cnt,
		struct connfem_sku_pinmap *result);

static int cfm_sku_prop_phdl_fetch(
		struct device_node *np,
		const char* propname,
		struct device_node **phdl);

static int cfm_sku_fem_state_populate(struct device_node *np,
		struct connfem_sku_fem_logic_cat *cat);

static int cfm_sku_ttbl_usg_subsys_hdl(struct device_node *np,
		struct connfem_sku *sku);

static int cfm_sku_tt_usg_subsys_entry(struct connfem_sku *sku,
		const char *fem_name, const char *subsys_name,
		struct connfem_sku_fem_truth_table_usage **tt_usg);

static struct connfem_sku_fem_truth_table_usage* cfm_sku_get_tt_usage_wf(
		struct connfem_sku* sku, unsigned int idx);

static struct connfem_sku_fem_truth_table_usage* cfm_sku_get_tt_usage_bt(
		struct connfem_sku* sku, unsigned int idx);

static void cfm_sku_array_log_helper(char *str, size_t size,
		void *arr, size_t count, enum cfm_array_type type);

static void cfm_sku_tt_tbl_log_dump(struct connfem_sku_fem_truth_table *tbl,
		char *str, size_t size);

/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/

/*******************************************************************************
 *			   P R I V A T E   D A T A
 ******************************************************************************/
/* Must end in {NULL, NULL} */
static struct cfm_sku_tt_usage_mapping cfm_sku_tt_usage_mappings[] = {
	{CFM_DT_NODE_WIFI,	cfm_sku_get_tt_usage_wf},
	{CFM_DT_NODE_BT,	cfm_sku_get_tt_usage_bt},
	{NULL, NULL}
};

/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
/**
 * cfm_sku_tt_usg_subsys_entry
 *	Iterate phandles of used states and record them in the
 *	ttbl_usg.
 *
 * Parameters
 *	sku	: has fem_count and ttbl usg to be populated.
 *	tt_usg	: acquired corresponding structure.
 *
 * Return value
 *	0	: Success
 *	-EINVAL : Error
 *	-ENODATA: Cannot find property
 *
 */
static int cfm_sku_tt_usg_subsys_entry(struct connfem_sku *sku,
		const char *fem_name, const char *subsys_name,
		struct connfem_sku_fem_truth_table_usage **tt_usg)
{
	unsigned int i;
	struct cfm_sku_tt_usage_mapping *map = cfm_sku_tt_usage_mappings;

	if (!sku || !subsys_name || !fem_name || !tt_usg) {
		pr_info("tt usage getter no input");
		return -EINVAL;
	}

	if (sku->fem_count > CONNFEM_SKU_FEM_COUNT) {
		pr_info("%s: fem count '%d' over max '%d'",
			__func__, sku->fem_count, CONNFEM_SKU_FEM_COUNT);
		return -EINVAL;
	}

	/* Match name with fem name in sku */
	for (i = 0; i < sku->fem_count; i++) {
		if (strncmp(fem_name,
			sku->fem[i].info.name,
			sizeof(sku->fem[i].info.name)) == 0) {
			break;
		}
	}

	if (i >= sku->fem_count) {
		pr_info("fem '%s' not found in sku",
			fem_name);
		return -EINVAL;
	}

	while (map && map->node_name) {
		if (strncmp(subsys_name, map->node_name,
			sizeof(map->node_name) - 1) == 0) {
			*tt_usg = map->get_tt_usg(sku, i);
			return 0;
		}
		map++;
	}

	pr_info("%s: cannot find tt_usage for node '%s'",
		__func__, subsys_name);
	*tt_usg = NULL;
	return -EINVAL;
}

static struct connfem_sku_fem_truth_table_usage* cfm_sku_get_tt_usage_wf(
		struct connfem_sku* sku, unsigned int idx)
{
    return &sku->fem[idx].tt_usage_wf;
}

static struct connfem_sku_fem_truth_table_usage* cfm_sku_get_tt_usage_bt(
		struct connfem_sku* sku, unsigned int idx)
{
    return &sku->fem[idx].tt_usage_bt;
}

/**
 * cfm_sku_ttbl_usg_subsys_hdl
 *	Iterate phandles of used states and record them in the
 *	ttbl_usg.
 *
 * Parameters
 *	np	: subsys node(s).
 *	sku	: has fem_count and ttbl usg to be populated.
 *
 * Return value
 *	0	: Success
 *	-EINVAL : Error
 *	-ENODATA: Cannot find property
 *
 */
static int cfm_sku_ttbl_usg_subsys_hdl(struct device_node *np,
		struct connfem_sku *sku)
{
	int err = 0, i = 0;
	int lerr = 0;
	struct device_node *tmp_np = NULL;
	struct of_phandle_iterator it;
	struct property *prop = NULL;
	struct connfem_sku_fem_truth_table_usage *tt_usg;

	if (!np || !sku) {
		pr_info("ttbl usg subsys hdl no input");
		return -EINVAL;
	}

	/* Errno type explanation:
	 * -ENODATA:	could not find property.
	 * We do not add WARN messages here, as it is not an error.
	 */
	prop = of_find_property(np, CFM_DT_PROP_USING_STATES, NULL);
	if (!prop) {
		pr_info("Property '%s' not found when coping with node '%s'",
			CFM_DT_PROP_USING_STATES, np->name);
		return -ENODATA;
	}

	/* Notice that of_for_each_phandle API implictly assigns -ENOENT
	 * after the loop is over. It is hard to detect this mechanism.
	 * Hence, if we uniformly utilize the same error variable
	 * to record error situation, the function will end in error.
	 * To clarify the error situation, we use variable "lerr" to
	 * detect the loop error, and "err" is to check whether the
	 * function flow is normal or not.
	*/
	of_for_each_phandle(&it, lerr, np, CFM_DT_PROP_USING_STATES, NULL, 0) {
		if (lerr < 0) {
			pr_info("%s: Invalid node at index %d",
				CFM_DT_PROP_USING_STATES, i);
			err = -EINVAL;
			break;
		}

		/* the name of tmp_np is fem name */
		err = cfm_sku_prop_phdl_fetch(it.node,
						CFM_DT_PROP_FEM,
						&tmp_np);
		if (err < 0) {
			err = -EINVAL;
			break;
		}

		err = cfm_sku_tt_usg_subsys_entry(sku, tmp_np->name,
						np->name, &tt_usg);
		if (err < 0) {
			pr_info("cfm_sku_tt_usg_subsys_entry failed");
			err = -EINVAL;
			break;
		}

		err = cfm_sku_fem_ttbl_usg_populate(it.node, tt_usg);
		if (err < 0) {
			pr_info("ttbl usg failed on node '%s'",
				np->name);
			err = -EINVAL;
			break;
		}

		i++;
		CFM_DT_PUT_NP(tmp_np);
	}

	CFM_DT_PUT_NP(tmp_np);
	of_node_put(it.node);
	return err;
}

/**
 * cfm_sku_ttbl_usg_hdl
 *	Fetches and populates the truth table usage based on
 *	the provided device tree node and fem name.
 *
 * Parameters
 *	np	: pointer to node containing subsys node(s).
 *	sku	: has fem_count and ttbl usg to be populated.
 *
 * Return value
 *	0	: Success
 *	-EINVAL : Error
 *
 */
int cfm_sku_ttbl_usg_hdl(struct device_node *np,
		struct connfem_sku *sku)
{
	struct device_node *tmp_np = NULL;
	int err = 0;

	if (!np || !sku) {
		pr_info("ttbl usg hdl no input");
		return -EINVAL;
	}

	/* Wifi node and prop using-state must exist */
	tmp_np = of_find_node_by_name(np, CFM_DT_NODE_WIFI);
	if (!tmp_np) {
		pr_info("Cannot find %s node under %s node",
			CFM_DT_NODE_WIFI,
			np->name);
		return -EINVAL;
	}

	err = cfm_sku_ttbl_usg_subsys_hdl(tmp_np, sku);
	if (err < 0) {
		pr_info("cfm_sku_ttbl_usg_subsys_hdl wifi failed");
		err = -EINVAL;
		goto dt_sku_ttbl_usg;
	}
	err = 0;
	CFM_DT_PUT_NP(tmp_np);

	/* BT node: must exist but prop using-state may not exist */
	tmp_np = of_find_node_by_name(np, CFM_DT_NODE_BT);
	if (!tmp_np) {
		pr_info("Cannot find %s node under %s node",
			CFM_DT_NODE_BT,
			np->name);
		err = -EINVAL;
		goto dt_sku_ttbl_usg;
	}

	err = cfm_sku_ttbl_usg_subsys_hdl(tmp_np, sku);
	if (err < 0 && err != -ENODATA) {
		pr_info("cfm_sku_ttbl_usg_subsys_hdl bt failed");
		err = -EINVAL;
		goto dt_sku_ttbl_usg;
	}
	err = 0;

	/* Success and fail both reach here */
dt_sku_ttbl_usg:
	CFM_DT_PUT_NP(tmp_np);
	return err;
}

 /**
 * cfm_sku_generic_layout_hdl
 *	Collects all info of layout-spdt.
 *
 * Parameters
 *	np	: a phandle node from "layout-spdt" or "layout" property
 *	pin_cnt	: pin_count is input and output at the same time.
 *		  After each cfm_sku_pinctrl_mapping_populate, it
 *		  will be updated so we have chance to append elements
 *		  directly to the unused section at the end of the array.
 *	pinmap	: pinmap will be populated.
 *
 */
int cfm_sku_generic_layout_hdl(struct device_node *np,
		unsigned int *pin_cnt, struct connfem_sku_pinmap *pinmap)
{
	int cnt = 0, err = 0;
	unsigned int mappings;

	if (!np || !pin_cnt || !pinmap) {
		pr_info("layout no input");
		return -EINVAL;
	}

	/* Get mapping
	 * mappings can be regarded as pin count.
	 */
	cnt = of_property_count_u32_elems(np, CFM_DT_PROP_MAPPING);
	if (cnt <= 0) {
		pr_info("Invalid or missing '%s' property, "
			"err %d",
			CFM_DT_PROP_MAPPING, cnt);
		return -EINVAL;
	}

	mappings = (unsigned int)cnt;

	/* Validate the number of entries in mapping */
	if ((mappings % CFM_DT_MAPPING_SIZE) != 0) {
		pr_info("'%s' needs to be multiple of %d, currently %d",
			CFM_DT_PROP_MAPPING,
			CFM_DT_MAPPING_SIZE,
			mappings);
		return -EINVAL;
	}

	mappings /= CFM_DT_MAPPING_SIZE;

	err = cfm_sku_pinctrl_mapping_populate(np, mappings,
						pin_cnt, pinmap);

	if (err < 0) {
		pr_info("mapping populate failed");
		return -EINVAL;
	}

	return 0;
}

/**
 * cfm_sku_fem_layout_populate
 *	Collects all info of layout(s).
 *
 * Parameters
 *	np	: a phandle node from "layout" property
 *	sku	: a data with fem-related information
 *	layout	: a layout of a fem will be populated
 *
 */
int cfm_sku_fem_layout_populate(struct device_node *np,
		struct connfem_sku *sku,
		struct connfem_sku_layout *layout)
{
	int err = 0, i;
	struct device_node *fem_np = NULL;
	unsigned int value = 0;
	unsigned int bk_cnt = 0, bk_idx = 0;

	if (!np || !sku || !layout) {
		pr_info("layout no input");
		return -EINVAL;
	}

	bk_cnt = layout->pin_count;
	bk_idx = layout->fem_idx;

	/* Populate fem_idx
	 * Check just in case, check if there is wrongly usage of dts
	 * e.g.,
	 * O: fem = <&qm42195>;
	 * X: fem = <&qm42195>, <&qm42639>; <-- over one element
	 */
	err = cfm_sku_prop_phdl_fetch(np, CFM_DT_PROP_FEM, &fem_np);

	if (err < 0) {
		return -EINVAL;
	}

	for (i = 0; i < sku->fem_count; i++) {
		if(strncmp(fem_np->name,
			sku->fem[i].info.name,
			CONNFEM_PART_NAME_SIZE) == 0) {
			break;
		}
		layout->fem_idx++;
	}

	/* Populate CFM_DT_PROP_BAND_PATH_WIFI */
	err = cfm_sku_prop_val_get(np, CFM_DT_PROP_BAND_PATH_WIFI, &value);
	if (err < 0) {
		err = -EINVAL;
		goto dt_fem_layout_err;
	}
	layout->bandpath[CONNFEM_SUBSYS_WIFI] = (unsigned char)value;

	/* Populate CFM_DT_PROP_BAND_PATH_BT */
	err = cfm_sku_prop_val_get(np, CFM_DT_PROP_BAND_PATH_BT, &value);
	if (err < 0) {
		err = -EINVAL;
		goto dt_fem_layout_err;
	}
	layout->bandpath[CONNFEM_SUBSYS_BT] = (unsigned char)value;

	err = cfm_sku_generic_layout_hdl(np,
				&layout->pin_count,
				layout->pinmap);
	if (err < 0) {
		pr_info("cfm_sku_generic_layout_hdl failed");
		err = -EINVAL;
		goto dt_fem_layout_err;
	}

	CFM_DT_PUT_NP(fem_np);
	return 0;

dt_fem_layout_err:
	CFM_DT_PUT_NP(fem_np);
	layout->pin_count = bk_cnt;
	layout->fem_idx = bk_idx;
	return -EINVAL;
}

/**
 * cfm_sku_pinctrl_mapping_populate
 *	Populate ANTSEL & FEM pin mapping from the pinctrl data property
 *
 *	NOTE: the result will be APPENDED in the output, instead of
 *	overwriting the whole structure as other API does.
 *
 * Parameters
 *	np	: a node includes "mapping" property.
 *	mappings: the number of sets of mappings.
 *	pin_cnt	: previous pin count. In the end of the function,
 *		  it will be updated to be the current pin count.
 *
 * Return value
 *	0	: Success, result output will be valid
 *	-EINVAL : Error
 *
 */
static int cfm_sku_pinctrl_mapping_populate(
		struct device_node *np,
		unsigned int mappings,
		unsigned int *pin_cnt,
		struct connfem_sku_pinmap *result)
{
	int err = 0;
	int p, i, start;
	unsigned int p_ofst;
	unsigned int value = 0;
	unsigned char *addr;

	/* Check if we still have enough storage for pins */
	if (*pin_cnt + mappings > CONNFEM_SKU_LAYOUT_PIN_COUNT) {
		pr_info("Too many ANTSEL PINs (over %d)! "
			"Not enough space for mappings: %d pincnt: '%d'",
			CONNFEM_SKU_LAYOUT_PIN_COUNT, mappings, *pin_cnt);
		return -ENOMEM;
	}

	p_ofst = *pin_cnt;

	p = i = start = 0;
	for (p = 0; p < mappings; p++) {
		start = p * CFM_DT_MAPPING_SIZE;

		for (i = 0; i < CFM_DT_MAPPING_SIZE; i++) {
			err = of_property_read_u32_index(np,
							CFM_DT_PROP_MAPPING,
							start + i,
							&value);
			if (err < 0)
				goto skus_mapping_populate_read_err;

			if (i == CFM_DT_MAPPING_POLARITY_INDEX && value > 1) {
				err = -EINVAL;
				goto skus_mapping_populate_polarity_err;
			}

			if (value > 0xFF) {
				err = -EINVAL;
				goto skus_mapping_populate_outofbound_err;
			}

			addr = (unsigned char *)(&result[p + p_ofst]);
			*(addr + i) = (unsigned char)value;
		}
	}

	*pin_cnt += mappings;

	return 0;

skus_mapping_populate_read_err:
	pr_info("%s,pin:%d idx:%d,real idx:%d,Read err %d",
		CFM_DT_PROP_MAPPING,
		p, i, (start + i), err);
	return err;

skus_mapping_populate_polarity_err:
	pr_info("%s,pin:%d idx:%d,real idx:%d,"
		"Polarity %d is not 0 or 1",
		CFM_DT_PROP_MAPPING,
		p, i, (start + i), value);
	return err;

skus_mapping_populate_outofbound_err:
	pr_info("%s,pin:%d idx:%d,real idx:%d,"
		"Value %d exceeds 8-bit",
		CFM_DT_PROP_MAPPING,
		p, i, (start + i), value);
	return err;
}

void cfm_skus_pininfo_dump(unsigned int pin_count,
		struct connfem_sku_pinmap *pinmap)
{
	int i, c;
	/* Multiply by 5 to align the maximum size like "0xcc," in fem log */
	char plog1[(CONNFEM_SKU_LAYOUT_PIN_COUNT * 5) + 1] = {0};
	char plog2[(CONNFEM_SKU_LAYOUT_PIN_COUNT * 5) + 1] = {0};
	char fg_log[(CONNFEM_SKU_LAYOUT_PIN_COUNT * 5) + 1] = {0};
	int p1_pos = 0;
	int p2_pos = 0;
	int fg_pos = 0;

	if (!pinmap) {
		pr_info("Sku pinmap, (null)");
		return;
	}

	pr_info("Sku PinInfo, count:%d, max:%d",
		pin_count, CONNFEM_EPAELNA_PIN_COUNT);

	for (i = 0; i < pin_count; i++) {
		if (p1_pos >= sizeof(plog1) - 1) {
			pr_info("%s: p1_pos:%d >= plog1 size:%zu",
				__func__, p1_pos,
				sizeof(plog1) - 1);
				break;
		}
		c = snprintf(plog1 + p1_pos, sizeof(plog1) - p1_pos,
			"%4d,",
			pinmap[i].pin1);
		if (c < 0 || c >= sizeof(plog1) - p1_pos) {
			pr_info("%s: c:%d,plog1 size:%zu",
				__func__, c,
				sizeof(plog1));
				break;
		} else {
			p1_pos += c;
		}

		if (p2_pos >= sizeof(plog2) - 1) {
			pr_info("%s: p2_pos:%d > plog2 size:%zu",
				__func__, p2_pos,
				sizeof(plog2) - 1);
				break;
		}
		c = snprintf(plog2 + p2_pos, sizeof(plog2) - p2_pos,
			"0x%02x,",
			pinmap[i].pin2);
		if (c < 0 || c >= sizeof(plog2) - p2_pos) {
			pr_info("%s: c:%d,plog2 size:%zu",
				__func__, c,
				sizeof(plog2));
				break;
		} else {
			p2_pos += c;
		}

		if (fg_pos >= sizeof(fg_log) - 1) {
			pr_info("%s: fg_pos:%d > fg_log size:%zu",
				__func__, fg_pos,
				sizeof(fg_log) - 1);
				break;
		}
		c = snprintf(fg_log + fg_pos, sizeof(fg_log) - fg_pos,
			"%4d,",
			pinmap[i].flag);
		if (c < 0 || c >= sizeof(fg_log) - fg_pos) {
			pr_info("%s: c:%d,fg_log size:%zu",
				__func__, c,
				sizeof(fg_log));
				break;
		} else {
			fg_pos += c;
		}
	}

	if (pin_count > 0) {
		pr_info("plog1:%s", plog1);
		pr_info("plog2:%s", plog2);
		pr_info("fg_log:%s", fg_log);
	}
}

/**
 * cfm_sku_fem_ttbl_usg_populate
 *	Collects all info of truth table usage(s).
 *
 * Parameters
 *	np	: a node includes "cat-names"
 *	tt_usage: a truth table usage of a fem will be populated.
 *
 */
int cfm_sku_fem_ttbl_usg_populate(struct device_node *np,
		struct connfem_sku_fem_truth_table_usage *tt_usage)
{
	int err = 0;
	struct device_node *tmp_np = NULL;
	const char *cat_str;
	struct property *prop;
	struct connfem_sku_fem_logic_cat *cat_ptr;
	unsigned int bk_cnt = 0;

	if (!np || !tt_usage) {
		pr_info("truth table usage populate no input");
		return -EINVAL;
	}

	prop = of_find_property(np, CFM_DT_PROP_CAT_NAMES, NULL);
	if (!prop) {
		pr_info("Property '%s' not found",
			CFM_DT_PROP_CAT_NAMES);
		return -EINVAL;
	}

	bk_cnt = tt_usage->cat_count;

	of_property_for_each_string(np, CFM_DT_PROP_CAT_NAMES, prop, cat_str) {
		if (!cat_str) {
			pr_info("Error when "
				"iterating property '%s' under node '%s'",
				CFM_DT_PROP_CAT_NAMES, np->name);
			goto dt_fem_ttble_usg_err;
		}

		/* When parsing strings under a certain property,
		 * it automatically adds a "" at the END of the string
		 * array. (not in DTS) Hence, it is neccessary to check
		 * if the acquired string is "".
		 * Besides, we anticipate that there is no any "" in DTS.
		 */
		if (cat_str[0] == '\0') {
			break;
		}

		if (tt_usage->cat_count >= CONNFEM_FEM_LOGIC_CAT_COUNT) {
			pr_info("'%d' over max logic categories '%d'",
				tt_usage->cat_count,
				CONNFEM_FEM_LOGIC_CAT_COUNT);
			goto dt_fem_ttble_usg_err;
		}

		/* Fetch a node whose name is cat_str */
		tmp_np = of_find_node_by_name(np, cat_str);

		if (!tmp_np) {
			pr_info("Invalid node named '%s'", cat_str);
			goto dt_fem_ttble_usg_err;
		}

		/* Populate id */
		cat_ptr = &tt_usage->cat[tt_usage->cat_count];

		err = cfm_sku_fem_state_populate(tmp_np, cat_ptr);
		if (err < 0) {
			pr_info("cfm_sku_fem_state_populate failed");
			goto dt_fem_ttble_usg_err;
		}

		tt_usage->cat_count++;
		CFM_DT_PUT_NP(tmp_np);
	}

	return 0;

dt_fem_ttble_usg_err:
	CFM_DT_PUT_NP(tmp_np);
	tt_usage->cat_count = bk_cnt;
	return -EINVAL;
}

static int cfm_sku_fem_state_populate(struct device_node *np,
		struct connfem_sku_fem_logic_cat *cat)
{
	int err = 0;
	unsigned int value = 0;
	const __be32 *p;
	struct property *prop;
	unsigned int bk_cnt = 0;

	if (!np || !cat) {
		pr_info("fem state populate no input");
		return -EINVAL;
	}

	bk_cnt = cat->op_count;

	/* Check just in case, check if there is wrongly usage of dts
		* e.g.,
		* O: cat-id = <1>;
		* X: cat-id = <1>, <2>; <-- over one element
		*/
	err = cfm_sku_prop_val_get(np, CFM_DT_PROP_CAT_ID, &value);
	if (err < 0) {
		goto dt_fem_states_err;
	}
	cat->id = value;

	prop = of_find_property(np, CFM_DT_PROP_STATES, NULL);
	if (!prop) {
		pr_info("Property '%s' not found", CFM_DT_PROP_STATES);
		goto dt_fem_states_err;
	}

	of_property_for_each_u32(np, CFM_DT_PROP_STATES, prop, p, value) {
		if (!p) {
			pr_info("Error when iterating property '%s'",
				CFM_DT_PROP_STATES);
			goto dt_fem_states_err;
		}

		cat->op[cat->op_count] = value;
		cat->op_count++;
	}

	return 0;

dt_fem_states_err:
	cat->op_count = bk_cnt;
	return -EINVAL;
}


/**
 * cfm_sku_fem_ctrl_pin_populate
 *	Collects all info of pin ctrl(s).
 *
 * Parameters
 *	np	: a node (phandle) is from property "using-fem"
 *	ctrl_pin: a ctrl pin of a fem will be populated.
 *
 */
int cfm_sku_fem_ctrl_pin_populate(struct device_node *np,
		struct connfem_sku_fem_ctrlpin *ctrl_pin)
{
	unsigned int value = 0;
	const __be32 *p;
	struct property *prop;
	unsigned int bk_cnt = 0;

	if (!np || !ctrl_pin) {
		pr_info("ctrl pin populate no input");
		return -EINVAL;
	}
	bk_cnt = ctrl_pin->count;

	prop = of_find_property(np, CFM_DT_PROP_PINS, NULL);
	if (!prop) {
		pr_info("Property '%s' not found", CFM_DT_PROP_PINS);
		goto dt_fem_ctrl_pin_err;
	}

	/* Iterate pins elements and populate */
	of_property_for_each_u32(np, CFM_DT_PROP_PINS, prop, p, value) {
		if (!p) {
			pr_info("Error when iterating property '%s', "
				"idx '%d'",
				CFM_DT_PROP_PINS,
				ctrl_pin->count);
			goto dt_fem_ctrl_pin_err;
		}

		if (ctrl_pin->count >= CONNFEM_FEM_PIN_COUNT) {
			pr_info("Over max supportable number %d of %s",
				CONNFEM_FEM_PIN_COUNT,
				CFM_DT_PROP_PINS);
			goto dt_fem_ctrl_pin_err;
		}

		/* Get PIN ID */
		ctrl_pin->id[ctrl_pin->count] = (unsigned char)value;
		ctrl_pin->count++;
	}

	return 0;

dt_fem_ctrl_pin_err:
	ctrl_pin->count = bk_cnt;
	return -EINVAL;
}

/**
 * cfm_sku_fem_ttbl_populate
 *	Collects all info of truth table(s).
 *
 * Parameters
 *	np	: a node (phandle) is from property "using-fem"
 *	tt: a truth table of a fem will be populated.
 *
 * Return value
 *	0	: Success, tt is successfully populated.
 *	-EINVAL : Error
 *
 */
int cfm_sku_fem_ttbl_populate(struct device_node *np,
		struct connfem_sku_fem_truth_table *tt)
{
	int i, j, cnt;
	int err = 0;
	unsigned int bk_cnt = 0;

	if (!np || !tt) {
		pr_info("truth table populate no input");
		return -EINVAL;
	}

	bk_cnt = tt->logic_count;
	cnt = of_property_count_u32_elems(np, CFM_DT_PROP_TRUTH_TABLE);

	if (cnt <= 0) {
		pr_info("Missing '%s' property",
			CFM_DT_PROP_TRUTH_TABLE);
		goto dt_fem_ttbl_err;
	}

	if ((cnt % CFM_DT_TRUTH_TABLE_ELEMENT_SIZE) != 0) {
		pr_info("%s has %d items, must be multiple of %d",
			CFM_DT_PROP_TRUTH_TABLE,
			cnt, CFM_DT_TRUTH_TABLE_ELEMENT_SIZE);
		goto dt_fem_ttbl_err;
	}

	/* Get how many logic counts
	 * e.g., truth-table = <OP_1 0x00>,
	 *                     <OP_2 0x01>;
	 * The cnt is 4. And the logic count above is 2.
	 */
	tt->logic_count = cnt / CFM_DT_TRUTH_TABLE_ELEMENT_SIZE;

	if (tt->logic_count > CONNFEM_FEM_LOGIC_COUNT) {
		pr_info("Logic count %d is not supported (limit %d)",
			tt->logic_count,
			CONNFEM_FEM_LOGIC_COUNT);
		goto dt_fem_ttbl_err;
	}

	/* Iterate truth-table elements */
	for (i = 0, j = 0; i < tt->logic_count;	i++) {
		j = i * CFM_DT_TRUTH_TABLE_ELEMENT_SIZE;
		/* Get operation mode number */
		err = of_property_read_u32_index(np,
			CFM_DT_PROP_TRUTH_TABLE, j, &tt->logic[i].op);

		if (err < 0) {
			pr_info("Can not get value from '%s' "
				"property, group %d, idx %d",
				CFM_DT_PROP_TRUTH_TABLE,
				i, j);
			goto dt_fem_ttbl_err;
		}

		/* Get binary number */
		err = of_property_read_u32_index(np,
			CFM_DT_PROP_TRUTH_TABLE, j + 1, &tt->logic[i].binary);

		if (err < 0) {
			pr_info("Can not get value from '%s' "
				"property, group %d, idx %d",
				CFM_DT_PROP_TRUTH_TABLE,
				i, j + 1);
			goto dt_fem_ttbl_err;
		}
	}

	return 0;

dt_fem_ttbl_err:
	tt->logic_count = bk_cnt;
	return -EINVAL;
}

/**
 * cfm_sku_fem_info_populate
 *	Collects all info of fem(s).
 *
 * Parameters
 *	np	: a node (phandle) is from property "using-fem"
 *	fem_info: fem info will be populated.
 *
 * Return value
 *	0	: Success, fem_info is successfully populated.
 *	-EINVAL : Error
 */
int cfm_sku_fem_info_populate(struct device_node *np,
		struct connfem_sku_fem_info *fem_info)
{
	int err = 0;
	unsigned int value = 0;

	if (!np || !fem_info) {
		pr_info("fem info populate no input");
		return -EINVAL;
	}

	/* Populate name */
	strncpy(fem_info->name, np->name,
		sizeof(fem_info->name) - 1);

	fem_info->name[sizeof(fem_info->name) - 1] = 0;

	/* Populate vid */
	err = of_property_read_u32(np, CFM_DT_PROP_VID, &value);
	if (err < 0) {
		goto dt_fem_info_err;
	}

	fem_info->vid = (unsigned short)value;

	/* Populate pid */
	err = of_property_read_u32(np, CFM_DT_PROP_PID, &value);
	if (err < 0) {
		goto dt_fem_info_err;
	}

	fem_info->pid = (unsigned short)value;

	/* Populate flag
	 * Flag property is reserved for future use.
	 * It is not necessary. Hence, we do not try to implement
	 * error handling mechanism here.
	 */
	err = of_property_read_u32(np, CFM_DT_PROP_FLAG, &value);
	if (err < 0) {
		pr_info("[REMIND] Cannot find %s", CFM_DT_PROP_FLAG);
		fem_info->flag = 0;
	} else {
		fem_info->flag = value;
	}

	return 0;

dt_fem_info_err:
	pr_info("Can't find %s or %s", CFM_DT_PROP_VID, CFM_DT_PROP_PID);
	return -EINVAL;
}

/**
 * cfm_sku_prop_val_get
 *	This function is generic, which can detect if the number
 *	of a value in a given property is only 1. If it is valid,
 *	return the value back in the parameter.
 *
 * Parameters
 *	np	: a node includes property
 *	prop_name	: the property we are checking
 *	res_val	: the value under the property
 *
 * Return value
 *	0	: Success, res_val is successfully got.
 *	-EINVAL	: Error
 */
int cfm_sku_prop_val_get(
		struct device_node *np,
		const char *propname,
		unsigned int *res_val)
{
	int err = 0;
	int check_cnt = 0;
	u32 val = 0;

	if (!np || !propname || !res_val) {
		pr_info("%s has no input", __func__);
		return -EINVAL;
	}

	check_cnt = of_property_count_u32_elems(np, propname);
	if (check_cnt != 1) {
		pr_info("Invalid %s property: expected one value, "
			"found %d",
			propname,
			check_cnt);
		return -EINVAL;
	}

	err = of_property_read_u32(np, propname, &val);
	if (err < 0) {
		pr_info("Failed to read %s property",
			propname);
		return -EINVAL;
	}

	*res_val = (unsigned int)val;

	return 0;
}

/**
 * cfm_sku_prop_phdl_fetch
 *	This function is generic, which can detect if the number
 *	of a phandle in a given property is only 1. If it is
 *	valid, return the phandle back in the parameter.
 *
 * Parameters
 * 	np	: a node includes property
 *	prop_name: the property we are checking
 *	phdl	: the phandle under the property
 *
 * Return value
 *	0	: Success, phdl is successfully fetched.
 *	-EINVAL	: Error
 */
static int cfm_sku_prop_phdl_fetch(
		struct device_node *np,
		const char *propname,
		struct device_node **phdl)
{
	int check_cnt = 0;

	if (!np || !propname) {
		pr_info("%s has no input", __func__);
		return -EINVAL;
	}

	check_cnt = of_property_count_u32_elems(np, propname);
	if (check_cnt != 1) {
		pr_info("Invalid %s property: expected one value, "
			"found %d",
			propname,
			check_cnt);
		return -EINVAL;
	}

	*phdl = of_parse_phandle(np, propname, 0);
	if (!(*phdl)) {
		pr_info("%s: Invalid node under %s",
			propname, propname);
		return -EINVAL;
	}

	return 0;
}

int cfm_sku_flags_config_get(void* ctx,
		struct cfm_epaelna_flags_config** flags_config)
{
	struct connfem_sku_context *cfm = NULL;

	if (!ctx || !flags_config)
		return -EINVAL;

	cfm = (struct connfem_sku_context *) ctx;

	*flags_config = cfm->flags_cfg;
	return 0;
}

int cfm_sku_available_get(void* ctx, bool *avail)
{
	struct connfem_sku_context *cfm = NULL;

	if (!ctx || !avail)
		return -EINVAL;

	cfm = (struct connfem_sku_context *) ctx;

	*avail = cfm->available;
	return 0;
}

/**
 * cfm_sku_data_dump
 *	This function is to print out the sku data.
 *
 * Parameters
 * 	sku	: the data structure to be printed out
 *
 */
void cfm_sku_data_dump(struct connfem_sku *sku)
{
	unsigned int i, j;
	char log[CONNFEM_SKU_LOG_SIZE] = {0};
	struct connfem_sku_fem_info *info;
	struct connfem_sku_fem_ctrlpin *ctrl_pin;
	struct connfem_sku_fem_truth_table *tt;
	struct connfem_sku_fem_truth_table_usage *usg_wf;
	struct connfem_sku_fem_truth_table_usage *usg_bt;
	struct connfem_sku_layout *layout;

	pr_info("FEM Count: %u, Layout Count: %u, Layout Flag: 0x%08x",
		sku->fem_count, sku->layout_count, sku->layout_flag);

	for (i = 0; i < sku->fem_count; i++) {
		info = &sku->fem[i].info;
		ctrl_pin = &sku->fem[i].ctrl_pin;
		tt = &sku->fem[i].tt;
		usg_wf = &sku->fem[i].tt_usage_wf;
		usg_bt = &sku->fem[i].tt_usage_bt;

		/* Dump FEM info */
		pr_info("FEM[%u]: vid: %hu, pid: %hu, flag: 0x%08x, name: '%s'",
			i, info->vid, info->pid, info->flag, info->name);

		/* Dump ctrlpin info:
		 * Notice type is CFM_ARRAY_TYPE_CHAR.
		*/
		cfm_sku_array_log_helper(log, sizeof(log),
					(void *)ctrl_pin->id,
					ctrl_pin->count,
					CFM_ARRAY_TYPE_CHAR);
		pr_info("ctrlpin cnt: %u, id(hex): '%s'",
			ctrl_pin->count, log);

		/* Dump truth table info */
		pr_info("truth table cnt: %u", tt->logic_count);
		cfm_sku_tt_tbl_log_dump(tt, log, sizeof(log));

		/* Dump truth table usage */
		pr_info("WF truth tbl usg cat_cnt: %u", usg_wf->cat_count);
		for (j = 0; j < usg_wf->cat_count; j++) {
			cfm_sku_array_log_helper(log, sizeof(log),
					(void *)usg_wf->cat[j].op,
					usg_wf->cat[j].op_count,
					CFM_ARRAY_TYPE_UINT);
			pr_info("WF usg[%u] cat_id: 0x%x, cat op count: %u",
				j, usg_wf->cat[j].id,
				usg_wf->cat[j].op_count);
			pr_info("WF usg[%u] logic cat op(hex): '%s'", j, log);
		}

		pr_info("BT truth tbl usg cat_cnt: %u", usg_bt->cat_count);
		for (j = 0; j < usg_bt->cat_count; j++) {
			cfm_sku_array_log_helper(log, sizeof(log),
					(void *)usg_bt->cat[j].op,
					usg_bt->cat[j].op_count,
					CFM_ARRAY_TYPE_UINT);
			pr_info("BT usg[%u] cat_id: 0x%x, cat op count: %u",
				j, usg_bt->cat[j].id,
				usg_bt->cat[j].op_count);
			pr_info("BT usg[%u] logic cat op(hex): '%s'", j, log);
		}
	}

	for (i = 0; i < sku->layout_count; i++) {
		layout = &sku->layout[i];
		pr_info("Layout[%u]: fem_idx: %u, bandpath(wf): 0x%x, "
			"bandpath(bt): 0x%x, pin_cnt: %u",
			i,
			layout->fem_idx,
			layout->bandpath[CONNFEM_SUBSYS_WIFI],
			layout->bandpath[CONNFEM_SUBSYS_BT],
			layout->pin_count);
		cfm_skus_pininfo_dump(layout->pin_count, layout->pinmap);
	}

	cfm_skus_pininfo_dump(sku->spdt.pin_count, sku->spdt.pinmap);
}

/**
 * cfm_sku_array_log_helper
 *	This function converts an array of unsigned integers into a hex
 *	string. Each integer is converted into a hex with variable
 *	length (2, 4, 6, or 8 characters), depending on its value. The
 *	hex strings are separated by a comma. The converted string is
 *	limited by the provided buffer size.
 *
 * Parameters
 *	str	: A pointer to the buffer that the resulting string is
 *		stored.
 *	size	: The size of the provided buffer. The resulting string
 *		will not exceed this size.
 *	arr	: A pointer to the array of unsigned integers to be
 *		converted.
 *	count	: The number of elements in the array to be converted.
 *	type	: to determine
 *
 * Note: This function will not overflow the provided buffer as it
 * 	checks the available size before writing. If the remaining
 * 	buffer size is not enough, it will print out a warning message
 * 	and stop the conversion.
 */
static void cfm_sku_array_log_helper(char *str, size_t size,
		void *arr, size_t count, enum cfm_array_type type)
{
	int sz, avail_sz;
	size_t i;
	unsigned int val = 0;
	const char *fmt;

	if (!str || !arr) {
		pr_info("%s: input is null", __func__);
		return;
	}

	memset(str, 0, size);

	for (i = 0; i < count; i++) {
		switch (type) {
		case CFM_ARRAY_TYPE_UINT:
			val = ((unsigned int *)arr)[i];
			break;
		case CFM_ARRAY_TYPE_CHAR:
			val = ((unsigned char *)arr)[i];
			break;
		default:
			pr_info("%s: wrong type", __func__);
			return;
		}

		if (val <= 0xFF)
			fmt = "%02x,";
		else if (val <= 0xFFFF)
			fmt = "%04x,";
		else if (val <= 0xFFFFFF)
			fmt = "%06x,";
		else
			fmt = "%08x,";

		/* Pre-calculate how many char we will write in buffer */
		sz = snprintf(NULL, 0, fmt, val);

		/* For avail_sz, we reserved a char space for '\0'.
		 * strlen counts the number of string without '\0'.
		 */
		avail_sz = size - strlen(str) - 1;

		if (sz > avail_sz) {
			pr_info("%s: remaining buffer is not enough (%d>%d)",
				__func__, sz, avail_sz);
			break;
		}

		if (snprintf(str + strlen(str), avail_sz, fmt, val) < 0) {
			pr_info("%s: error when writing in str (%d/%d)",
				__func__, sz, avail_sz);
			break;
		}
	}
}

static void cfm_sku_tt_tbl_log_dump(struct connfem_sku_fem_truth_table *tbl,
		char *str, size_t size)
{
	size_t i;
	unsigned int tmp_arr[CONNFEM_FEM_LOGIC_COUNT] = {0};

	for (i = 0; i < tbl->logic_count; ++i) {
		tmp_arr[i] = tbl->logic[i].op;
	}

	cfm_sku_array_log_helper(str, size, tmp_arr,
				tbl->logic_count, CFM_ARRAY_TYPE_UINT);

	pr_info("ttbl op (hex): '%s'", str);

	/* It's not necessary to initialize tmp_arr to be 0
	 * since we have the same range of processing array.
	*/
	for (i = 0; i < tbl->logic_count; ++i) {
		tmp_arr[i] = tbl->logic[i].binary;
	}

	cfm_sku_array_log_helper(str, size, tmp_arr,
				tbl->logic_count, CFM_ARRAY_TYPE_UINT);

	pr_info("ttbl bin(hex): '%s'", str);
}
