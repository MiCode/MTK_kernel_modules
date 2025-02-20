// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/iio/consumer.h>
#include <linux/slab.h>
#include "connfem.h"

/*******************************************************************************
 *				M A C R O S
 ******************************************************************************/
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "["KBUILD_MODNAME"][DT]" fmt

/*******************************************************************************
 *			    D A T A   T Y P E S
 ******************************************************************************/


/*******************************************************************************
 *		    F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************/
static int cfm_dt_epaelna_parse(struct device_node *np,
		struct connfem_epa_context *cfm);

static void cfm_dt_epaelna_free(struct cfm_dt_epaelna_context *dt,
				bool free_all);

static int cfm_dt_epaelna_hwid_parse(struct device_node *np,
		struct platform_device *pdev,
		unsigned int *hwid);

static int cfm_dt_epaelna_hwid_gpio_parse(struct device_node *np,
		unsigned int *hwid_out,
		unsigned int *nbits_out);

#if (CFG_HWID_PMIC_SUPPORT == 1)
static int cfm_dt_epaelna_hwid_pmic_parse(struct device_node *np,
		struct platform_device *pdev,
		unsigned int *hwid_out,
		unsigned int *nbits_out);

static int cfm_dt_epaelna_hwid_pmic_match(struct device_node *pmic_node,
		char *range_n,
		unsigned int pmic_value,
		unsigned int *hwid_out,
		unsigned int *nbits_out);

static int cfm_dt_epaelna_hwid_pmic_read(struct platform_device *pdev,
		const char *channel_name,
		unsigned int *value_out);
#endif

static int cfm_dt_epaelna_parts_parse(
		struct device_node *np,
		const char *prop_name,
		unsigned int hwid,
		struct device_node **parts_np);

static int cfm_dt_epaelna_pctl_pinmux_apply(
		struct platform_device *pdev,
		struct cfm_dt_epaelna_pctl_context *pctl);

static int cfm_dt_epaelna_pctl_state_parse(
		struct device_node *dn,
		struct connfem_epaelna_fem_info *fem_info,
		struct cfm_dt_epaelna_pctl_state_context *pstate);

static void cfm_dt_epaelna_pctl_free(struct cfm_dt_epaelna_pctl_context *pctl);

static void cfm_dt_epaelna_pctl_state_free(
		struct cfm_dt_epaelna_pctl_state_context *pstate);

static int cfm_dt_epaelna_pctl_state_find(
		const struct device_node *dn,
		char *state_name,
		unsigned int *state_index);

static int cfm_dt_epaelna_pctl_state_get(
		struct device_node *dn,
		struct connfem_epaelna_fem_info *fem_info,
		char symbol,
		struct cfm_dt_epaelna_pctl_state_context *pstate);

static bool cfm_dt_epaelna_pctl_exists(
		struct cfm_dt_epaelna_pctl_context *pctl);

static int cfm_dt_epaelna_pctl_walk(
		struct cfm_dt_epaelna_pctl_context *pctl,
		struct cfm_epaelna_pin_config *result);

static int cfm_dt_epaelna_pctl_data_parse(
		struct device_node *np,
		struct cfm_dt_epaelna_pctl_data_context *pctl_data);

static int cfm_dt_epaelna_pctl_data_mapping_parse(
		struct device_node *np,
		unsigned int *pin_cnt);

static int cfm_dt_epaelna_pctl_data_laa_pinmux_parse(
		struct device_node *np,
		unsigned int *laa_cnt);

static int cfm_dt_epaelna_flags_parse(
		struct device_node *np,
		const char *hw_name,
		struct cfm_dt_epaelna_flags_context *flags_out);

static int cfm_dt_epaelna_pin_mapping_get(
		struct device_node *dn,
		struct cfm_dt_epaelna_pctl_state_context *pstate,
		struct cfm_epaelna_pin_config *result_out);

static int cfm_dt_cfg_apply_pctl(
		struct platform_device *pdev,
		struct connfem_epaelna_fem_info *fem_info,
		struct cfm_epaelna_pin_config *pin_cfg,
		struct cfm_dt_epaelna_pctl_context *pctl);

static int cfm_dt_cfg_is_antsel_match(
		struct connfem_epaelna_pin_info *dp_info,
		struct connfem_epaelna_pin_info *cp_info);

struct device_node* cfm_dt_child_find(struct device_node *dn,
		const char** node_names);

static struct property* cfm_dt_prop_find (
		struct device_node *dn, ...);

static int cfm_dt_sku_data_parse(struct device_node *np,
		struct connfem_sku_context *cfm);

static int cfm_dt_sku_hw_np_fetch(
		struct device_node *np,	unsigned int hwid,
		struct device_node **np_out);

static int cfm_dt_epaelna_flags_name_get(
		unsigned int hwid,
		size_t name_out_size,
		char *name_out);

static int cfm_dt_sku_fem_data_hdl(struct device_node *np,
				struct connfem_sku *cfm_sku);

static int cfm_dt_sku_layout_data_hdl(struct device_node *np,
				struct connfem_sku *cfm_sku);

static struct device_node* cfm_dt_node_parse_helper(void *cfm,
		const char **internal_node_names, const char **node_names);

/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/


/*******************************************************************************
 *			   P R I V A T E   D A T A
 ******************************************************************************/
/* The following arrays must end in NULL */
const char *cfm_sku_nodenames[]		= {CFM_DT_NODE_SKU};
const char *cfm_sku_mtk_nodenames[]	= {CFM_DT_NODE_SKU_MTK};
const char *cfm_epaelna_nodenames[]	= {CFM_DT_NODE_EPAELNA};
const char *cfm_epaelna_mtk_nodenames[]	= {CFM_DT_NODE_EPAELNA_MTK};

/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
static struct device_node* cfm_dt_node_parse_helper(void *cfm,
		const char **internal_node_names, const char **node_names)
{
	struct device_node *dn;
	struct device_node *np = NULL;

	if (!cfm || !internal_node_names || !node_names) {
		pr_info("%s: Missing input", __func__);
		return NULL;
	}

	dn = ((struct connfem_context_ops *)cfm)->pdev->dev.of_node;

	/* Search internal node ... */
	if (connfem_is_internal()) {
		np = cfm_dt_child_find(dn, internal_node_names);
	}

	/* If no internal node, search another node ... */
	if (!np) {
		np = cfm_dt_child_find(dn, node_names);
	}

	return np;
}

int cfm_dt_epa_parse(void *cfm)
{
	int err = 0;
	struct device_node *np = NULL;

	if (!cfm) {
		pr_info("%s: Missing input", __func__);
		return -EINVAL;
	}

	np = cfm_dt_node_parse_helper(cfm,
				cfm_epaelna_mtk_nodenames,
				cfm_epaelna_nodenames);

	if (np) {
		pr_info("Find node '%s'", np->name);
		err = cfm_dt_epaelna_parse(np,
			(struct connfem_epa_context *)cfm);
		CFM_DT_PUT_NP(np);
	} else {
		pr_info("Missing epa elna node");
		err = -EINVAL;
	}

	return err;
}

int cfm_dt_sku_parse(void *cfm)
{
	int err = 0;
	struct device_node *np = NULL;

	if (!cfm) {
		pr_info("%s: Missing input", __func__);
		return -EINVAL;
	}

	np = cfm_dt_node_parse_helper(cfm,
				cfm_sku_mtk_nodenames,
				cfm_sku_nodenames);

	if (np) {
		pr_info("Find node '%s'", np->name);
		err = cfm_dt_sku_data_parse(np,
			(struct connfem_sku_context *)cfm);
		CFM_DT_PUT_NP(np);
	} else {
		pr_info("Missing sku node");
		err = -EINVAL;
	}

	return err;
}

void cfm_dt_free(struct cfm_dt_context *dt)
{
	cfm_dt_epaelna_free(&dt->epaelna, true);
}

int cfm_dt_cfg_ext(struct connfem_epa_context *cfm)
{
	struct platform_device *pdev = cfm->ops.pdev;
	struct cfm_epaelna_config *cfg = &cfm->epaelna;
	struct cfm_dt_epaelna_context *dt = &cfm->dt.epaelna;

	/* Apply pinctrl from device tree */
	if (0 > cfm_dt_cfg_apply_pctl(pdev, &cfg->fem_info,
				&cfg->pin_cfg, &dt->pctl)) {
		pr_info("Skip applying pinctrl");
	}

	if (0 > cfm_dt_cfg_apply_pctl(pdev, &cfg->bt_fem_info,
				&cfg->bt_pin_cfg, &dt->bt_pctl)) {
		pr_info("Skip applying bt pinctrl");
	}

	return 0;
}

static void cfm_dt_epaelna_free(struct cfm_dt_epaelna_context *dt,
				bool free_all)
{
	int i;

	if (!dt)
		return;

	/* PIN related info can always be freed without keeping */
	cfm_dt_epaelna_pctl_free(&dt->pctl);

	if (!free_all) {
		memset(&dt->pctl, 0, sizeof(dt->pctl));
		return;
	}

	/* In case we need to keep some critical information for later use,
	 * the data related to "flags" and "parts" are especially important,
	 * these should only be freed if "free_all" is specified.
	 */
	cfm_dt_epaelna_flags_free(&dt->flags);

	for (i = 0; i < CONNFEM_PORT_NUM; i++) {
		if (dt->parts_np[i]) {
			of_node_put(dt->parts_np[i]);
			dt->parts_np[i] = NULL;
		}
	}

	memset(dt, 0, sizeof(*dt));
}

static void cfm_dt_epaelna_pctl_free(struct cfm_dt_epaelna_pctl_context *pctl)
{
	if (!pctl)
		return;

	cfm_dt_epaelna_pctl_state_free(&pctl->state);
}

static void cfm_dt_epaelna_pctl_state_free(
		struct cfm_dt_epaelna_pctl_state_context *pstate)
{
	int i;

	if (!pstate)
		return;

	if (pstate->np) {
		for (i = 0; i < pstate->np_cnt; i++) {
			if (pstate->np[i]) {
				of_node_put(pstate->np[i]);
				pstate->np[i] = NULL;
			}
		}
		kfree(pstate->np);
		pstate->np = NULL;
	}

	pstate->np_cnt = 0;
}

void cfm_dt_epaelna_flags_free(
		struct cfm_dt_epaelna_flags_context *flags)
{
	int i;

	if (!flags)
		return;

	for (i = 0; i < CONNFEM_SUBSYS_NUM; i++) {
		if (flags->np[i]) {
			of_node_put(flags->np[i]);
			flags->np[i] = NULL;
		}
	}
}

static int cfm_dt_epaelna_parse(struct device_node *np,
		struct connfem_epa_context *cfm)
{
	int err = 0;
	struct device_node *dn = cfm->ops.pdev->dev.of_node;
	struct cfm_dt_epaelna_context *dt = &cfm->dt.epaelna;
	struct cfm_epaelna_config *result = &cfm->epaelna;
	unsigned int hwid;
	struct property *parts;

	/* Initialize */
	memset(dt, 0, sizeof(*dt));
	memset(result, 0, sizeof(*result));

	/* HWID property is optional, but must be valid if it exists */
	hwid = cfm_param_epaelna_hwid();
	if (hwid == CFM_PARAM_EPAELNA_HWID_INVALID) {
		err = cfm_dt_epaelna_hwid_parse(np, cfm->ops.pdev, &dt->hwid);
		if (err == -ENOENT)
			dt->hwid = 0;
		else if (err == -EAGAIN)
			return err;
		else if (err < 0)
			return -EINVAL;
	} else {
		dt->hwid = hwid;
		pr_info("Force HWID: %u", dt->hwid);
	}

	/* Parse parts property */
	err = cfm_dt_epaelna_parts_parse(np, CFM_DT_PROP_PARTS, dt->hwid,
			dt->parts_np);
	if (err < 0) {
		err = -EINVAL;
		goto dt_epaelna_err;
	}

	/* Populate FEM info */
	err = cfm_epaelna_feminfo_populate(dt->parts_np, &result->fem_info);
	if (err < 0) {
		err = -EINVAL;
		goto dt_epaelna_err;
	}

	/* Parse bt_parts property */
	parts = cfm_dt_prop_find(np, CFM_DT_PROP_BT_PARTS);
	if (!parts) {
		err = -EINVAL;
	} else {
		err = cfm_dt_epaelna_parts_parse(np, parts->name, 
				dt->hwid, dt->bt_parts_np);
	}

	if (err == 0) {
		/* Populate BT dedicated FEM info */
		err = cfm_epaelna_feminfo_populate(dt->bt_parts_np,
				&result->bt_fem_info);
	}

	/* Set available */
	if (result->fem_info.id)
		result->available = true;

	/* Get flag's node_name */
	err = cfm_dt_epaelna_flags_name_get(dt->hwid,
			sizeof(dt->flags.node_name),
			dt->flags.node_name);

	if (err < 0) {
		err = -EINVAL;
		goto dt_epaelna_err;
	}

	/* Parse subsys flags nodes */
	err = cfm_dt_epaelna_flags_parse(np, dt->flags.node_name, &dt->flags);
	if (err < 0) {
		err = -EINVAL;
		goto dt_epaelna_err;
	}

	/* Populate subsys flags */
	err = cfm_epaelna_flags_populate(&dt->flags, result->flags_cfg);
	if (err < 0) {
		err = -EINVAL;
		goto dt_epaelna_err;
	}

	/* Parse pinctrl state property */
	err = cfm_dt_epaelna_pctl_state_parse(dn,
					      &result->fem_info,
					      &dt->pctl.state);
	if (err == -ENOENT) {
		/* For combo chip, it doesn't need to apply PINMUX.
		 * We should collect pin mapping from corresponding device node
		 */
		err = cfm_dt_epaelna_pin_mapping_get(dn,
						  &dt->pctl.state,
						  &result->pin_cfg);
	}
	if (err < 0) {
		err = -EINVAL;
		goto dt_epaelna_err;
	}

	/* Parse pinctrl state property for BT dedicate FEM */
	if (result->bt_fem_info.id != 0) {
		err = cfm_dt_epaelna_pctl_state_parse(dn,
						      &result->bt_fem_info,
						      &dt->bt_pctl.state);
		if (err == -ENOENT) {
			/* For combo chip, it doesn't need to apply PINMUX.
			 * We should collect pin mapping from corresponding
			 * device node
			 */
			err = cfm_dt_epaelna_pin_mapping_get(dn,
							  &dt->bt_pctl.state,
							  &result->bt_pin_cfg);
		}
	}

	/* Walk through all pinctrl nodes to parse and populate */
	if (cfm_dt_epaelna_pctl_exists(&dt->pctl)) {
		err = cfm_dt_epaelna_pctl_walk(&dt->pctl, &result->pin_cfg);
		if (err < 0) {
			err = -EINVAL;
			goto dt_epaelna_err;
		}
	} else {
		pr_info("Skip ANTSEL pin info collection from pinctrl");
	}

	/* Apply PINMUX only if device tree successfully parsed */
	if (cfm_dt_epaelna_pctl_exists(&dt->pctl)) {
		err = cfm_dt_epaelna_pctl_pinmux_apply(cfm->ops.pdev, &dt->pctl);
		if (err < 0) {
			err = -EINVAL;
			goto dt_epaelna_err;
		}
	} else {
		pr_info("Skip applying GPIO PINMUX");
	}

	/* Walk through all pinctrl nodes to parse and populate
	 * for BT dedicate FEM
	 */
	if (cfm_dt_epaelna_pctl_exists(&dt->bt_pctl)) {
		err = cfm_dt_epaelna_pctl_walk(&dt->bt_pctl,
				&result->bt_pin_cfg);
		if (err < 0) {
			err = -EINVAL;
			goto dt_epaelna_err;
		}
	} else {
		pr_info("Skip ANTSEL pin info collection from pinctrl for BT dedicate FEM");
	}

	/* Apply PINMUX only if device tree successfully parsed */
	if (cfm_dt_epaelna_pctl_exists(&dt->bt_pctl)) {
		err = cfm_dt_epaelna_pctl_pinmux_apply(cfm->ops.pdev, &dt->bt_pctl);
		if (err < 0) {
			err = -EINVAL;
			goto dt_epaelna_err;
		}
	} else {
		pr_info("Skip applying GPIO PINMUX for BT dedicate FEM");
	}


	return 0;

dt_epaelna_err:
	cfm_dt_epaelna_free(dt, false);
	cfm_epaelna_config_free(result, false);

	return err;
}

/**
 * cfm_dt_epaelna_hwid_parse
 *	Parses and retrieves type of hardware based on info in hwid node.
 *
 * Parameters
 *	np	: Pointer to the node containing the 'hwid' node.
 *	pdev	: Pointer to the platform_device
 *	hwid	: Output parameter storing the final HW ID
 *
 * Return value
 *	0		: Success, hwid will contain valid result
 *	-ENOENT	: HW ID not defined in device tree, hwid will set to 0
 *	-EINVAL	: Failed to parse hwid property
 *	-EAGAIN	: PMIC is not ready for probe, return immediately and
 *					  wait for next probe() callback
 */
static int cfm_dt_epaelna_hwid_parse(struct device_node *np,
				     struct platform_device *pdev,
				     unsigned int *hwid_out)
{
	unsigned int hwid = 0;
	struct device_node *np_hwid;
	unsigned int hwid_tmp = 0;
	unsigned int nbits = 0;
	unsigned int bits = 0;
	int ret = 0;

	/* Get node of HWID if it is exists */
	np_hwid = of_get_child_by_name(np, CFM_DT_NODE_HWID);
	if (!np_hwid)
		return -ENOENT;

	/* Retrieve hwid by gpio */
	ret = cfm_dt_epaelna_hwid_gpio_parse(np_hwid, &hwid_tmp, &bits);
	if (ret < 0 && ret != -ENOENT) {
		return -EINVAL;
	} else if (ret != -ENOENT) {
		hwid |= (hwid_tmp << nbits);
		nbits += bits;
	}

#if (CFG_HWID_PMIC_SUPPORT == 1)
	/* Retrieve hwid by pmic */
	hwid_tmp = 0;
	bits = 0;
	ret = cfm_dt_epaelna_hwid_pmic_parse(np_hwid, pdev,
			&hwid_tmp, &bits);
	if (ret == -EAGAIN) {
		return ret;
	} else if (ret < 0 && ret != -ENOENT) {
		return -EINVAL;
	} else if (ret != -ENOENT) {
		hwid |= (hwid_tmp << nbits);
		nbits += bits;
	}
#endif

	/* Update output parameter */
	*hwid_out = hwid;

	pr_info("hwid: %d, nbits: %d", hwid, nbits);

	return 0;
}

/**
 * cfm_dt_epaelna_hwid_gpio_parse
 *	Parses hwid value based on GPIO property.
 *
 * Parameters
 *	np	: Pointer to the node containing the 'gpio' property.
 *	hwid_out	: Output parameter storing the hwid calculated
 *				by gpio property
 *	nbits_out	: Output parameter storing the value
 *				how many bits to present hwid
 *
 * Return value
 *	0		: Success, hwid will contain valid result
 *	-ENOENT	: gpio is not defined in device tree, hwid will set to 0
 *	-EINVAL : Failed to parse gpio property
 *
 */
static int cfm_dt_epaelna_hwid_gpio_parse(struct device_node *np,
				     unsigned int *hwid_out,
				     unsigned int *nbits_out)
{
	int i, cnt = 0, gpio_num;
	unsigned int hwid = 0;
	unsigned int consumed_bits = 0;
	unsigned int gpio_value = 0;

	/* CFM_DT_PROP_GPIO_CELLS (#gpio-cells) is defined in mtXXXX.dts
	 * In the normal case, it should be 2. Hence, dts needs to be set
	 * up in the form like gpio = <&pio 132 0>;
	 */
	cnt = of_count_phandle_with_args(np, CFM_DT_PROP_GPIO,
					CFM_DT_PROP_GPIO_CELLS);
	if (cnt <= 0 && cnt != -ENOENT) {
		pr_info("Invalid '%s' property", CFM_DT_PROP_GPIO);
		return -EINVAL;
	}

	pr_info("Number of HWID GPIO PINs: %d",
		(cnt == -ENOENT ? 0 : cnt));

	if (cnt <= 0)
		return -ENOENT;

	/* Retrieve hardware ID based on GPIO PIN(s) value */
	for (i = 0; i < cnt; i++) {
		/* Retrieve GPIO PIN number */
		gpio_num = of_get_named_gpio(np, CFM_DT_PROP_GPIO, i);

		/* Abort if it's not valid */
		if (!gpio_is_valid(gpio_num)) {
			pr_info("Invalid '%s' property: idx(%d)err(%d)",
				CFM_DT_NODE_HWID,
				i, gpio_num);
			hwid = 0;
			return -EINVAL;
		}

		/* Read GPIO PIN */
		gpio_value = gpio_get_value(gpio_num);
		hwid |= ((gpio_value & 0x1) << i);
		pr_info("%s[%d]: GPIO(%d)=%d, hwid(%u)",
			CFM_DT_NODE_HWID, i,
			gpio_num, gpio_value, hwid);

		consumed_bits++;
	}

	*hwid_out = hwid;
	*nbits_out = consumed_bits;

	return 0;
}

#if (CFG_HWID_PMIC_SUPPORT == 1)
/**
 * cfm_dt_epaelna_hwid_pmic_parse
 *	Parses hwid value based on PMIC device node.
 *
 * Parameters
 *	np		: Pointer to the node containing the 'pmic' device node.
 *	pdev	: Pointer to the platform_device
 *	hwid_out	: Output parameter storing the hwid
 *					calculated by pmic device node
 *	nbits_out	: Output parameter storing the value
 *					how many bits to present hwid
 *
 * Return value
 *	0		: Success, hwid will contain valid result
 *	-ENOENT	: pmic is not defined in device tree, hwid will set to 0
 *	-EINVAL : Failed to parse pmic device node
 *	-EAGAIN	: PMIC is not ready for probe, return immediately
 *					and wait for next probe() callback
 *
 */
static int cfm_dt_epaelna_hwid_pmic_parse(
		struct device_node *np,
		struct platform_device *pdev,
		unsigned int *hwid_out,
		unsigned int *nbits_out)
{
	struct device_node *pmic_node;
	int i, err, cnt, bits;
	const char *channel_name;
	char range_n[sizeof(CFM_DT_PROP_RANGE_PREFIX) + 10 + 1];
	unsigned int pmic_value = 0;
	unsigned int hwid = 0;
	unsigned int nbits = 0;
	unsigned int hwid_tmp = 0;
	struct property *prop_ch_name;

	/* Get node of pmic if it is exists */
	pmic_node = of_get_child_by_name(np, CFM_DT_NODE_PMIC);
	if (!pmic_node)
		return -ENOENT;

	prop_ch_name = cfm_dt_prop_find(pmic_node, CFM_DT_PROP_CHANNEL_NAME);
	if (!prop_ch_name) {
		pr_info("Fail to parse prop ch name");
		return -EINVAL;
	}

	/* Retrieve name number from pmic node */
	cnt = of_property_count_strings(pmic_node, prop_ch_name->name);
	if (cnt <= 0) {
		pr_info("Missing prop ch name, cnt: %d",	cnt);
		return -EINVAL;
	}

	/* Retrieve hardware ID based on PMIC PIN(s) value */
	for (i = 0; i < cnt; i++) {
		/* Retrieve name */
		err = of_property_read_string_index(pmic_node,
				prop_ch_name->name,
				i,
				&channel_name);
		if (err < 0) {
			pr_info("Invalid '%s' property: idx(%d)err(%d)",
				prop_ch_name->name,
				i,
				err);
			return -EINVAL;
		}
		pr_info("Channel_name: %s, idx: %d", channel_name, i);

		err = of_property_match_string(pdev->dev.of_node, CFM_DT_PROP_IO_CHANNEL_NAMES, channel_name);
		if (err < 0) {
			pr_info("'%s' not found, err(%d)",
				channel_name,
				err);
			return -EINVAL;
		}

		snprintf(range_n, sizeof(range_n), "%s%d",
			CFM_DT_PROP_RANGE_PREFIX,
			i);

		/* Retrieve value from PMIC channel name */
		pmic_value = 0;
		err = cfm_dt_epaelna_hwid_pmic_read(pdev,
					 channel_name, &pmic_value);
		if (err == -EAGAIN)
			return err;
		else if (err < 0)
			return -EINVAL;

		err = cfm_dt_epaelna_hwid_pmic_match(pmic_node,
					range_n,
					pmic_value,
					&hwid_tmp,
					&bits);

		if (err == -EAGAIN)
			return err;
		else if (err < 0)
			return -EINVAL;

		hwid |= (hwid_tmp << nbits);
		nbits += bits;

		pr_info("After apply '%s', hwid set to %d", channel_name, hwid);
	}

	*hwid_out = hwid;
	*nbits_out = nbits;

	return 0;
}

/**
 * cfm_dt_epaelna_hwid_pmic_match
 *	Determine hwid based on ranges and pmic value.
 *
 * Parameters
 *	pmic_node	: Pointer to the 'pmic' HWID device node.
 *	range_n		: Pointer to the range-n containing pmic ranges
 *	pmic_value	: PMIC value read from specific io-channel
 *	hwid_out	: Output parameter storing the hwid
 *					calculated by pmic device node
 *	nbits_out	: Output parameter storing the value
 *					how many bits to present hwid
 *
 * Return value
 *	0		: Success, hwid will contain valid result
 *	-ENOENT	: pmic is not defined in device tree, hwid will set to 0
 *	-EINVAL : Failed to parse pmic device node
 *	-EAGAIN	: PMIC is not ready for probe, return immediately
 *					and wait for next probe() callback
 *
 */
static int cfm_dt_epaelna_hwid_pmic_match(
		struct device_node *pmic_node,
		char *range_n,
		unsigned int pmic_value,
		unsigned int *hwid_out,
		unsigned int *nbits_out)
{
	unsigned int prev_range = 0;
	unsigned int curr_range = 0;
	int i, err, tmp, range_count = 0;
	unsigned int hwid = 0;
	unsigned int nbits = 0;

	/* Retrieve count of range_n */
	range_count =
			of_property_count_u32_elems(pmic_node, range_n);
	if (range_count <= 0) {
		pr_info("Can not find '%s' property, err %d",
			range_n,
			range_count);
		return -EINVAL;
	}

	prev_range = 0;
	for (i = 0; i < range_count; i++) {
		/* Retrieve values from range */
		err = of_property_read_u32_index(pmic_node,
			 range_n, i, &curr_range);
		if (err < 0) {
			pr_info("Can not get value from '%s' property, err %d",
				range_n,
				range_count);
			return -EINVAL;
		}

		pr_info("property: %s, [%d/%d], curr_range: %d, prev_range: %d",
				range_n,
				i,
				range_count,
				curr_range,
				prev_range);

		if (prev_range >= curr_range) {
			pr_info("'%s' values are not from small to large",
				range_n);
			return -EINVAL;
		}

		/* Compare value between pmic and range and determine hwid */
		if (prev_range <= pmic_value &&
				   pmic_value < curr_range) {
			break;
		}

		prev_range = curr_range;
	}
	hwid = i;

	/* Count total bits */
	tmp = range_count;
	while (tmp) {
		nbits++;
		tmp >>= 1;
	}

	pr_info("hwid set to: %d, consumed bits for '%s': %d",
			hwid,
			range_n,
			nbits);

	*hwid_out = hwid;
	*nbits_out = nbits;

	return 0;
}

/**
 * cfm_dt_epaelna_hwid_pmic_read
 *	Read pmic value based on PMIC channel.
 *
 * Parameters
 *	pdev			: Pointer to the platform_device
 *	channel_name	: Get the PMIC value based on this channel_name
 *	value_out		: Output parameter storing the value read
 *					from PMIC
 *
 * Return value
 *	0		: Success, hwid will contain valid result
 *	-EAGAIN	: PMIC is not ready for probe, return immediately
 *					and wait for next probe() callback
 *
 */
static int cfm_dt_epaelna_hwid_pmic_read(
		struct platform_device *pdev,
		const char *channel_name,
		unsigned int *value_out)
{
	struct iio_channel *chan_iio_channel;
	int ret = 0;
	int val = 0;

	chan_iio_channel = devm_iio_channel_get(&pdev->dev, channel_name);
	ret = PTR_ERR_OR_ZERO(chan_iio_channel);

	if (ret) {
		pr_info("PMIC '%s' get fail, err: %d", channel_name, ret);
		/* if error code is EPROBE_DEFER (517),
		 * probe function will be called again
		 */
		if (ret == -EPROBE_DEFER) {
			pr_info("Got EPROBE_DEFER, err: %d", ret);
			return -EAGAIN;
		}
	} else {
		ret = iio_read_channel_processed(chan_iio_channel, &val);
		if (ret < 0) {
			pr_info("'%s' read fail, err: %d",
					channel_name,
					ret);
			return ret;
		} else {
			pr_info("'%s' read success, val: %d",
					channel_name,
					val);
		}
	}

	*value_out = val;

	return ret;
}
#endif

/**
 * cfm_dt_epaelna_parts_parse
 *	Locate the correct parts based on hwid, and collect parts nodes.
 *
 * Parameters
 *	np	: Pointer to the node containing the 'parts' property.
 *	prop_name: CFM_DT_PROP_PARTS or CFM_DT_PROP_BT_PARTS
 *	hwid	: Indication on which parts group to be parsed
 *		  Each parts group must contain CONNFEM_PORT_NUM of phandle
 *	parts_np: struct device_node *parts_np[CONNFEM_PORT_NUM]
 *		  for storing parts node pointers.
 *
 * Return value
 *	0	: Success, part & name will contain valid result
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_parts_parse(
		struct device_node *np,
		const char *prop_name,
		unsigned int hwid,
		struct device_node **parts_np_out)
{
	int err = 0;
	int cnt, start, i;
	struct device_node *parts_np[CONNFEM_PORT_NUM];

	/* 'parts' property must exist */
	cnt = of_property_count_u32_elems(np, prop_name);
	if (cnt <= 0) {
		pr_info("Missing '%s' property", prop_name);
		return -EINVAL;
	}

	/* Ensure 'parts' property is valid */
	if ((cnt % CONNFEM_PORT_NUM) != 0) {
		pr_info("%s has %d items, must be multiple of %d",
			prop_name,
			cnt, CONNFEM_PORT_NUM);
		return -EINVAL;
	}

	if (((hwid + 1) * CONNFEM_PORT_NUM) > cnt) {
		pr_info("%s has %dx%d items, need %dx%d for hwid %d",
			prop_name,
			(cnt / 2), CONNFEM_PORT_NUM,
			(hwid + 1), CONNFEM_PORT_NUM,
			hwid);
		return -EINVAL;
	}

	/* Collect node pointer to each of the required parts */
	memset(parts_np, 0, sizeof(parts_np));
	start = hwid * CONNFEM_PORT_NUM;
	for (i = 0; i < CONNFEM_PORT_NUM; i++) {
		parts_np[i] =
			of_parse_phandle(np, prop_name, start + i);

		if (!parts_np[i]) {
			pr_info("%s[%d][%d]: Invalid node at index %d",
				prop_name,
				hwid, i,
				start + i);
			err = -EINVAL;
			break;
		}

		pr_info("Found %s[%d][%d]: %s",
			prop_name,
			hwid, i,
			parts_np[i]->name);
	}

	/* Update DT context if parsing completes */
	if (!err) {
		memcpy(parts_np_out, parts_np, sizeof(parts_np));
		return 0;
	}

	/* Release parts node if error */
	for (i = 0; i < CONNFEM_PORT_NUM; i++) {
		if (parts_np[i])
			of_node_put(parts_np[i]);
	}

	return err;
}

/**
 * cfm_dt_epaelna_pctl_pinmux_apply
 *	Apply pinctrl pinmux
 *
 * Parameters
 *
 * Return value
 *	0	: Pinctrl pinmux successfully applied
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_pctl_pinmux_apply(
		struct platform_device *pdev,
		struct cfm_dt_epaelna_pctl_context *pctl)
{
	struct pinctrl *pinctrl = NULL;
	struct pinctrl_state *pin_state = NULL;
	int err;

	pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pinctrl)) {
		pr_info("Unable to get pinctrl: %ld", PTR_ERR(pinctrl));
		return -EINVAL;
	}

	pin_state = pinctrl_lookup_state(pinctrl, pctl->state.name);
	if (IS_ERR(pin_state)) {
		pr_info("Unable to find state '%s': %ld",
			pctl->state.name,
			PTR_ERR(pin_state));

		devm_pinctrl_put(pinctrl);
		return -EINVAL;
	}

	err = pinctrl_select_state(pinctrl, pin_state);
	if (err < 0) {
		pr_info("Failed to apply pinctrl for state '%s', err %d",
			pctl->state.name, err);
		err = -EINVAL;
	} else {
		pr_info("'%s' pinmux successfully applied",
			pctl->state.name);
		err = 0;
	}

	devm_pinctrl_put(pinctrl);
	return err;
}

/**
 * cfm_dt_epaelna_pctl_state_parse
 *	Collect pinctrl nodes associated to the FEM Info.
 *
 * Parameters
 *	np	: Pointer to the node containing the 'pinctrl-names' property.
 *
 * Return value
 *	0	: Success, part & name will contain valid result
 *	-ENOENT : State could not be found
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_pctl_state_parse(
		struct device_node *dn,
		struct connfem_epaelna_fem_info *fem_info,
		struct cfm_dt_epaelna_pctl_state_context *pstate)
{
	int err = 0;
	int i, c;
	char list[] = {CFM_DT_PCTL_STATE_SYMBOL};
	int len = sizeof(list) / sizeof(char);

	/* Find mapping symbol in pinctrl state */
	for (i = 0; i < len; i++) {
		err = cfm_dt_epaelna_pctl_state_get(dn, fem_info, list[i], pstate);
		if (err == 0) {
			break;
		}
	}

	if (err < 0) {
		pr_info("pctl state name not found, err %d", err);
		return err;	/* -ENOENT, -EINVAL */
	}

	/* Collect pinctrl nodes */
	c = snprintf(pstate->prop_name, sizeof(pstate->prop_name),
		 "%s%d",
		 CFM_DT_PROP_PINCTRL_PREFIX, pstate->index);
	if (c < 0 || c >= sizeof(pstate->prop_name)) {
		pr_info("pinctrl prop name error %d, sz %u, '%s%d'",
			c,
			(unsigned int)sizeof(pstate->prop_name),
			CFM_DT_PROP_PINCTRL_PREFIX,
			pstate->index);
		return -EINVAL;
	}

	err = of_property_count_u32_elems(dn, pstate->prop_name);
	if (err <= 0) {
		pr_info("'%s' is at %d, invalid '%s' prop, err %d",
			pstate->name,
			pstate->index,
			pstate->prop_name,
			err);
		return -EINVAL;
	}
	pstate->np_cnt = (unsigned int)err;

	pr_info("Found pinctrl state '%s', prop '%s', with %d nodes",
		pstate->name, pstate->prop_name, pstate->np_cnt);

	pstate->np = kcalloc(pstate->np_cnt,
			     sizeof(*(pstate->np)),
			     GFP_KERNEL);

	err = 0;
	for (i = 0; i < pstate->np_cnt; i++) {
		pstate->np[i] = of_parse_phandle(dn, pstate->prop_name, i);
		if (!pstate->np[i]) {
			pr_info("Unable to get pinctrl node %s[%d]",
				pstate->prop_name, i);
			err = -EINVAL;
			break;
		}
		/* pr_info("Collected [%d]'%s'", i, pstate->np[i]->name); */
	}

	/* Release the partially collected pinctrl nodes in case of error */
	if (err < 0)
		cfm_dt_epaelna_pctl_state_free(pstate);
	return err;
}

/**
 * cfm_dt_epaelna_pctl_state_find
 *	Find the pinctrl state's index.
 *
 * Parameters
 *	np	   : Pointer to the node containing 'pinctrl-names' prop.
 *	state_name : Pinctrl state name to be located.
 *	state_index: Output parameter storing the index found
 *
 * Return value
 *	0	: Success, state_index will contain valid result
 *	-ENOENT : State could not be found
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_pctl_state_find(
		const struct device_node *dn,
		char *state_name,
		unsigned int *state_index)
{
	struct property *p;
	const char *name = NULL;
	int i = 0;

	of_property_for_each_string(dn, "pinctrl-names", p, name) {
		if (strncmp(name, state_name,
			    CFM_DT_PCTL_STATE_NAME_SIZE) == 0) {
			break;
		}
		i++;
	}

	if (!name) {
		pr_info("pinctrl-names does not have '%s' state",
			state_name);
		return -ENOENT;
	}

	*state_index = i;
	return 0;
}

/**
 * cfm_dt_epaelna_pctl_state_get
 *	Get the pinctrl state's name and index with matched symbol
 *
 * Parameters
 *	np	   : Pointer to the node containing 'pinctrl-names' prop.
 *	symbol : Symbol in state name, ex "_" or "-"
 *
 * Return value
 *	0	: Success, state name and index will contain valid result
 *	-ENOENT : State could not be found
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_pctl_state_get(
		struct device_node *dn,
		struct connfem_epaelna_fem_info *fem_info,
		char symbol,
		struct cfm_dt_epaelna_pctl_state_context *pstate)
{
	int err = 0;
	int index, c;
	char name[CFM_DT_PCTL_STATE_NAME_SIZE];

	/* Locate pinctrl state index */
	c = snprintf(name, sizeof(name),
			"%s%c%s",
			fem_info->part_name[CONNFEM_PORT_WFG],
			symbol,
			fem_info->part_name[CONNFEM_PORT_WFA]);
	if (c < 0 || c >= sizeof(name)) {
		pr_info("pinctrl state name error %d, sz %u, '%s'%c'%s'",
			c,
			(unsigned int)sizeof(name),
			fem_info->part_name[CONNFEM_PORT_WFG],
			symbol,
			fem_info->part_name[CONNFEM_PORT_WFA]);
		return -EINVAL;
	}

	if (NULL != of_get_child_by_name(dn, name)) {
		memcpy(pstate->name, name, sizeof(name));
	}

	err = cfm_dt_epaelna_pctl_state_find(dn, name, &index);
	if (err < 0)
		return err;	/* -ENOENT, -EINVAL */

	/* "pinctrl-names" is existed */
	memcpy(pstate->name, name, sizeof(name));
	pstate->index = index;
	return 0;
}

/**
 * cfm_dt_epaelna_pctl_exists
 *	Returns true/false indicating if pinctrl nodes exist
 *
 * Parameters
 *
 * Return value
 *
 */
static bool cfm_dt_epaelna_pctl_exists(
		struct cfm_dt_epaelna_pctl_context *pctl)
{
	return pctl && (pctl->state.np_cnt > 0);
}

/**
 * cfm_dt_epaelna_pctl_walk
 *	Walk through pinctrl nodes and populate pin config result
 *
 * Parameters
 *
 * Return value
 *	0	: Success, pin config output will be valid
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_pctl_walk(
		struct cfm_dt_epaelna_pctl_context *pctl,
		struct cfm_epaelna_pin_config *result_out)
{
	int err = 0;
	int i;
	struct device_node *np;
	struct cfm_dt_epaelna_pctl_data_context pctl_data;
	struct cfm_epaelna_pin_config result;

	memset(&result, 0, sizeof(result));

	for (i = 0; i < pctl->state.np_cnt; i++) {
		for_each_child_of_node(pctl->state.np[i], np) {
			pr_info("Checking [%d]%s.%s",
				i, pctl->state.np[i]->name, np->name);

			/* Aligned with Kernel pinctrl's expectation on the
			 * existence of 'pinmux' property, this check prevents
			 * us from mis-collecting the PIN mapping and the real
			 * pinmux setting.
			 *
			 * Kernel pinctrl in fact will log runtime error, and
			 * our probe callback does not get called in this case.
			 *
			 * So we probably wont need this check, but leave it
			 * there in case Kernel behaviour changes in future.
			 *
			 */
			if (!of_get_property(np, CFM_DT_PROP_PINMUX, NULL)) {
				pr_info("Missing '%s' property",
					CFM_DT_PROP_PINMUX);
				of_node_put(np);
				continue;
			}

			err = cfm_dt_epaelna_pctl_data_parse(np, &pctl_data);
			if (err < 0)
				break;

			/* As we're iterating through each of the pinctrl
			 * nodes, the previous result has to be kept, and
			 * new info should be appended instead.
			 */
			err = cfm_epaelna_pincfg_populate(&pctl_data, &result);
			if (err < 0)
				break;

			of_node_put(np);
		}
		if (err < 0)
			break;
	}

	if (err < 0) {
		pr_info("Error while parsing [%d]%s.%s",
			i, pctl->state.np[i]->name,
			(np ? np->name : "(null)"));
		of_node_put(np);
	} else {
		memcpy(result_out, &result, sizeof(result));

		cfm_epaelna_pininfo_dump(&result_out->pin_info);
		cfm_epaelna_laainfo_dump(&result_out->laa_pin_info);

	}

	return err;
}

/**
 * cfm_dt_epaelna_pctl_data_parse
 *	Parses the required properties in the pinctrl data node
 *
 * Parameters
 *	np: Pointer to the node containing the 'pinmux' property.
 *
 * Return value
 *	0	: Success, pin config output will be valid
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_pctl_data_parse(
		struct device_node *np,
		struct cfm_dt_epaelna_pctl_data_context *pctl_data_out)
{
	int err = 0;
	struct cfm_dt_epaelna_pctl_data_context pctl_data;

	memset(&pctl_data, 0, sizeof(pctl_data));
	pctl_data.np = np;

	err = cfm_dt_epaelna_pctl_data_mapping_parse(np, &pctl_data.pin_cnt);
	if (err < 0)
		return err;

	err = cfm_dt_epaelna_pctl_data_laa_pinmux_parse(np,
							 &pctl_data.laa_cnt);
	if (err < 0)
		return err;

	memcpy(pctl_data_out, &pctl_data,
	       sizeof(pctl_data));

	return 0;
}

/**
 * cfm_dt_epaelna_pctl_data_mapping_parse
 *	Parses the 'mapping' property in the pinctrl data node
 *
 * Parameters
 *	np: Pointer to the node containing the 'mapping' property.
 *
 * Return value
 *	0	: Success, pin config output will be valid
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_pctl_data_mapping_parse(
		struct device_node *np,
		unsigned int *pin_cnt_out)
{
	int err = 0;
	unsigned int pin_cnt;
	unsigned int mappings;

	/* Get number of entries in pinmux and mapping */
	err = of_property_count_u32_elems(np, CFM_DT_PROP_PINMUX);
	if (err <= 0) {
		pr_info("Invalid or missing '%s' property, err %d",
			CFM_DT_PROP_PINMUX, err);
		return -EINVAL;
	}
	pin_cnt = (unsigned int)err;
	err = 0;

	err = of_property_count_u32_elems(np, CFM_DT_PROP_MAPPING);
	if (err <= 0) {
		pr_info("Invalid or missing '%s' property, err %d",
			CFM_DT_PROP_MAPPING, err);
		return -EINVAL;
	}
	mappings = (unsigned int)err;
	err = 0;

	/* Validate the number of entries in pinmux and mapping */
	if (pin_cnt > CONNFEM_EPAELNA_PIN_COUNT) {
		pr_info("'%s' exceeds limit of %d PINs, currently %d",
			CFM_DT_PROP_PINMUX,
			CONNFEM_EPAELNA_PIN_COUNT,
			pin_cnt);
		return -EINVAL;
	}

	if ((mappings % CFM_DT_MAPPING_SIZE) != 0) {
		pr_info("'%s' needs to be multiple of %d, currently %d",
			CFM_DT_PROP_MAPPING, CFM_DT_MAPPING_SIZE, mappings);
		return -EINVAL;
	}

	mappings /= CFM_DT_MAPPING_SIZE;

	if (pin_cnt != mappings) {
		pr_info("Unequal number of entries: '%s':%d,'%s':%d",
			CFM_DT_PROP_PINMUX, pin_cnt,
			CFM_DT_PROP_MAPPING, mappings);
		return -EINVAL;
	}

	*pin_cnt_out = pin_cnt;

	return 0;
}

/**
 * cfm_dt_epaelna_pctl_data_laa_pinmux_parse
 *	Parses the 'laa-mapping' property in the pinctrl data node
 *
 * Parameters
 *	np: Pointer to the node containing the 'mapping' property.
 *
 * Return value
 *	0	: Success, pin config output will be valid
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_pctl_data_laa_pinmux_parse(
		struct device_node *np,
		unsigned int *laa_cnt_out)
{
	int err = 0;
	unsigned int laa_cnt;

	/* laa-pinmux property is optional */
	err = of_property_count_u32_elems(np, CFM_DT_PROP_LAA_PINMUX);
	if (err <= 0)
		laa_cnt = 0;
	else
		laa_cnt = (unsigned int)err;

	/* Validate the number of entries in laa-pinmux */
	if ((laa_cnt % CFM_DT_LAA_PINMUX_SIZE) != 0) {
		pr_info("'%s' needs to be multiple of %d, currently %d",
			CFM_DT_PROP_LAA_PINMUX,
			CFM_DT_LAA_PINMUX_SIZE,
			laa_cnt);
		return -EINVAL;
	}

	laa_cnt /= CFM_DT_LAA_PINMUX_SIZE;

	if (laa_cnt > CONNFEM_EPAELNA_LAA_PIN_COUNT) {
		pr_info("'%s' exceeds limit of %d PINs, currently %d",
			CFM_DT_PROP_LAA_PINMUX,
			CONNFEM_EPAELNA_LAA_PIN_COUNT,
			laa_cnt);
		return -EINVAL;
	}

	*laa_cnt_out = laa_cnt;

	return 0;
}

/**
 * cfm_dt_epaelna_flags_parse
 *	Locate & collect the flag nodes for each subsys based on hwid.
 *
 *	Note that for a given hwid:
 *	1. Subsys does not necessary need to define its flags node.
 *		- Subsys' flags np in the flags context will set to NULL.
 *	2. Flags node is defined, but is empty
 *		- Subsys' flags np in the flags context will be valid, caller
 *		  needs to decide how to deal with.
 *
 *	On success, flags context needs to be released via
 *	cfm_dt_epaelna_flags_free() or cfm_dt_epaelna_free();
 *
 * Parameters
 *	np	 : Pointer to the epa_elna/_mtk node containing the subsys node.
 *	hw_name	 : Indication on which flag node to be selected. (flags-N or
 *		   flags)
 *	flags_out: Pointer to the flags context for storing the parsed states.
 *
 * Return value
 *	0	: Success, output parameter will be valid
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_flags_parse(
		struct device_node *np,
		const char *hw_name,
		struct cfm_dt_epaelna_flags_context *flags_out)
{
	int i;
	struct device_node *subsys_np;
	struct cfm_dt_epaelna_flags_context flags;

	memset(&flags, 0, sizeof(flags));

	strncpy(flags.node_name, hw_name,
		sizeof(flags.node_name) - 1);

	flags.node_name[sizeof(flags.node_name) - 1] = 0;

	/* Collect subsys' flags node if valid */
	for (i = 0; i < CONNFEM_SUBSYS_NUM; i++) {
		if (cfm_subsys_name[i] == NULL)
			continue;

		subsys_np = of_get_child_by_name(np, cfm_subsys_name[i]);
		if (!subsys_np) {
			pr_info("Skip %s flags, missing '%s' node",
				cfm_subsys_name[i],
				cfm_subsys_name[i]);
			continue;
		}

		flags.np[i] = of_get_child_by_name(subsys_np, flags.node_name);
		if (!flags.np[i]) {
			pr_info("Skip %s flags, missing '%s' node",
				cfm_subsys_name[i],
				flags.node_name);
			continue;
		}

		of_node_put(subsys_np);
	}

	memcpy(flags_out, &flags, sizeof(flags));

	return 0;
}

/**
 * cfm_dt_epaelna_pin_mapping_get
 *	Collect the pin mapping info associated to the FEM info
 *
 * Parameters
 *	dn		: Pointer to the node containing device node associated
 *				to the FEM info
 *	fem_info	: Point to the 'fem_info' containing part name
 *	result_out	: Pointer to the 'pin_config' containing pin mapping
 *
 * Return value
 *	0	: Success, output parameter will be valid
 *	-EINVAL : Error
 *
 */
static int cfm_dt_epaelna_pin_mapping_get(
		struct device_node *dn,
		struct cfm_dt_epaelna_pctl_state_context *pstate,
		struct cfm_epaelna_pin_config *result_out)
{
	int err = 0;
	unsigned int mappings;
	struct device_node *parts_np;
	struct cfm_dt_epaelna_pctl_data_context pctl_data;
	struct cfm_epaelna_pin_config result;

	memset(&result, 0, sizeof(result));
	memset(&pctl_data, 0, sizeof(pctl_data));

	parts_np = of_get_child_by_name(dn, pstate->name);
	if (!parts_np) {
		pr_info("can not get device node from %s", pstate->name);
		return -EINVAL;
	}

	err = of_property_count_u32_elems(parts_np, CFM_DT_PROP_MAPPING);
	if (err <= 0) {
		pr_info("Invalid or missing '%s' property, err %d",
			CFM_DT_PROP_MAPPING, err);
		return -EINVAL;
	}
	mappings = (unsigned int)err;
	err = 0;

	/* Validate the number of entries in mapping */
	if ((mappings % CFM_DT_MAPPING_SIZE) != 0) {
		pr_info("'%s' needs to be multiple of %d, currently %d",
			CFM_DT_PROP_MAPPING, CFM_DT_MAPPING_SIZE, mappings);
		return -EINVAL;
	}

	mappings /= CFM_DT_MAPPING_SIZE;

	pctl_data.np = parts_np;
	pctl_data.pin_cnt = mappings;
	err = cfm_epaelna_pincfg_populate(&pctl_data, &result);
	if (err < 0)
		return -EINVAL;

	memcpy(result_out, &result, sizeof(result));

	cfm_epaelna_pininfo_dump(&result_out->pin_info);

	return 0;
}

/**
 * cfm_dt_cfg_apply_pctl
 *	The main purpose is to apply pctl PINMUX
 *
 * Parameters
 *	pdev		: Pointer to the platform device
 *
 * Return value
 *	0	: Success, output parameter will be valid
 *	-EINVAL : Error
 *
 */
static int cfm_dt_cfg_apply_pctl(
		struct platform_device *pdev,
		struct connfem_epaelna_fem_info *fem_info,
		struct cfm_epaelna_pin_config *pin_cfg,
		struct cfm_dt_epaelna_pctl_context *pctl)
{
	int err = 0;
	struct device_node *dn = pdev->dev.of_node;
	struct cfm_epaelna_pin_config dt_pin;

	/* Parse pinctrl state property */
	err = cfm_dt_epaelna_pctl_state_parse(dn, fem_info, &pctl->state);

	/* For combo chip, pin mapping should be applied ConnFem cfg */
	if (err < 0) {
		pr_info("Skip applying GPIO PINMUX, err %d", err);
		return err;
	}

	/* Apply PINMUX only if device tree successfully parsed */
	if (cfm_dt_epaelna_pctl_exists(pctl)) {
		if (0 > cfm_dt_epaelna_pctl_walk(pctl, &dt_pin)) {
			return -EINVAL;
		}

		/*
		* Because PINMUX can not be setting by config file,
		* make sure the APSoC Antsel/eFEM PIN mapping is the
		* same with DTS and config file
		*/
		if (0 > cfm_dt_cfg_is_antsel_match(&dt_pin.pin_info,
						&pin_cfg->pin_info)) {
			return -EINVAL;
		}

		if (0 > cfm_dt_epaelna_pctl_pinmux_apply(pdev, pctl)) {
			return -EINVAL;
		}
	}

	return err;
}

/**
 * cfm_dt_cfg_is_antsel_match
 * Check the antsel pin in DTS is the same with config.
 *
 * Parameters
 * dp_info : (IN) Pointer to DTS pin info.
 * cp_info: (IN) Pointer to Config pin info.
 *
 * Return value
 * 0 : Success, node/property name is existed.
 * -EINVAL : Fail to find the mapping name.
 *
 */
static int cfm_dt_cfg_is_antsel_match(
		struct connfem_epaelna_pin_info *dp_info,
		struct connfem_epaelna_pin_info *cp_info)
{
	int i, j;
	struct connfem_epaelna_pin *dt_pin = dp_info->pin;
	struct connfem_epaelna_pin *cfg_pin = cp_info->pin;

	if (dp_info->count != cp_info->count) {
		pr_info("The pin_info count is not the same");
		return -EINVAL;
	}

	for (i = 0; i < dp_info->count; i++) {
		for (j = 0; j < cp_info->count; j++) {
			if (dt_pin[i].antsel == cfg_pin[j].antsel) {
				break;
			}
		}

		if (j == cp_info->count) {
			pr_info("Miss DTS antsel pin %d in cfg file", dt_pin[i].antsel);
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * cfm_dt_child_find
 * 1. First to find the valid name in DTS.
 * 2. Try to find legacy name to backward compatible.
 *
 * Parameters
 * dn	: (IN) Pointer to parent device node.
 * node_names: (IN) Support name list.
 *
 * Return value
 * 0	: Success, node/property name is existed.
 * -EINVAL : Fail to find the mapping name.
 *
 */
struct device_node* cfm_dt_child_find(struct device_node *dn,
		const char** node_names)
{
	const char **name = node_names;
	struct device_node *np = NULL;
	if (!dn || !node_names) {
		return NULL;
	}

	while (*name && !(np = of_get_child_by_name(dn, *name))) {
		name++;
	}
	return np;
}

/**
 * cfm_dt_prop_find
 * 1. First to find the valid prop name in DTS.
 * 2. Try to find legacy prop name to backward compatible.
 *
 * Parameters
 * dn : (IN) Pointer to parent device node.
 * name_list: (IN) Support name list.
 * out_name : (OUTPUT) Return the mapping name.
 *
 * Return value
 * NULL : Fail to find the mapping name.
 *
 */
static struct property* cfm_dt_prop_find (
		struct device_node *dn, ...)
{
	char *idx;
	struct property *np;
	va_list args;

	va_start(args, dn);
	while ((idx = va_arg(args, char *)) != NULL) {
		if ((np = of_find_property(dn, idx, NULL)) != NULL) {
			pr_info("Find property '%s'", np->name);
			va_end(args);
			return np;
		}
	}

	va_end(args);
	return NULL;
}

static int cfm_dt_sku_data_parse(struct device_node *np,
		struct connfem_sku_context *cfm)
{
	int err = 0, lerr = 0, i = 0;
	struct connfem_sku *sku = &cfm->sku;
	struct cfm_dt_epaelna_flags_context *flags = &cfm->flags;
	struct cfm_epaelna_flags_config *flags_cfg = cfm->flags_cfg;
	unsigned int hwid;
	struct device_node *hw_np = NULL;
	struct of_phandle_iterator it;
	const char *hw_name = cfm_param_hw_name();

	cfm_dt_sku_data_reset(cfm);

	/* Parse HWID
	 * We have a priority hierarchy for handling the hardware ID (hwid),
	 * which is as follows from high to low:
	 * (1) hwid (from config or insmod)
	 * (2) hw_names (from config or insmod)
	 * (3) GPIO.
	 */
	hwid = *cfm_param_hwid();
	/* Check if hw_name has content matching any names
	* specified in DT tree. If hw_name is matched with name
	* defined in DTS, hwid will be updated. However, if prop
	* hw_names cannot be found or name cannot be matched, we
	* do not regard this as an error.
	*/
	if (hwid == CFM_PARAM_HWID_INVALID) {
		if (hw_name[0] != '\0') {
			err = of_property_match_string(np,
							CFM_DT_PROP_HW_NAMES,
							hw_name);
			if (err >= 0) {
				hwid = (unsigned int)err;
				pr_info("hw_name matched. HWID update: %d",
					hwid);
			}
		}
	}
	if (hwid == CFM_PARAM_HWID_INVALID) {
		err = cfm_dt_epaelna_hwid_parse(np, cfm->ops.pdev, &hwid);
		if (err == -ENOENT)
			hwid = 0;
		else if (err == -EAGAIN)
			return err;
		else if (err < 0)
			return -EINVAL;
	}
	if (hwid == CFM_PARAM_HWID_INVALID) {
		hwid = 0;
		pr_info("hwid is set to default value: %d", hwid);
	}
	cfm->hwid = hwid;

	/* Note that we need to re-get hw_name if hw_name is null string
	 * via hwid because hw_name is not only acquired through config
	 * mechanism.
	*/
	if (hw_name[0] == '\0') {
		err = of_property_read_string_index(np,
						CFM_DT_PROP_HW_NAMES,
						cfm->hwid,
						&hw_name);
		if (err < 0) {
			pr_info("Error (%d) when parsing '%s' property",
				err, CFM_DT_PROP_HW_NAMES);
			return -EINVAL;
		}

		err = cfm_param_hw_name_set(hw_name, strlen(hw_name) + 1);
		if (err < 0) {
			pr_info("Error (%d) when setting hw name", err);
			return -EINVAL;
		}
	}

	/* Get hw-N node (N is hwid)
	 * We distinguish current hw setting via hwid.
	 */
	err = cfm_dt_sku_hw_np_fetch(np, cfm->hwid, &hw_np);
	if (err < 0) {
		pr_info("cfm_dt_sku_hw_np_fetch failed");
		err = -EINVAL;
		goto dt_sku_err;
	}

	/* We have successfully fetched the hw-N node.
	 * Moving forward, we will parse the FEM data with hw_np
	 * and populate variables in struct connfem_sku.
	*/
	err = cfm_dt_sku_fem_data_hdl(hw_np, sku);
	if (err < 0) {
		pr_info("cfm_dt_sku_fem_data_hdl failed");
		err = -EINVAL;
		goto dt_sku_err;
	}

	cfm->available = true;

	/* Parse layout-flag */
	err = cfm_sku_prop_val_get(hw_np,
				CFM_DT_PROP_LAYOUT_FLAG,
				&sku->layout_flag);

	if (err < 0) {
		pr_info("[REMIND] Get value of '%s' failed",
			CFM_DT_PROP_LAYOUT_FLAG);
		sku->layout_flag = 0;
	}

	/* Parse layout */
	err = cfm_dt_sku_layout_data_hdl(hw_np, sku);

	if (err < 0) {
		pr_info("cfm_dt_sku_layout_data_hdl failed");
		err = -EINVAL;
		goto dt_sku_err;
	}

	/* Parse spdt layout */
	of_for_each_phandle(&it, lerr, hw_np,
			CFM_DT_PROP_LAYOUT_SPDT, NULL, 0) {
		if (lerr < 0) {
			pr_info("%s[%d]: Invalid node at index %d",
				CFM_DT_PROP_LAYOUT_SPDT,
				i, i);
			err = -EINVAL;
			break;
		}

		err = cfm_sku_generic_layout_hdl(it.node,
						&sku->spdt.pin_count,
						sku->spdt.pinmap);

		if (err < 0) {
			pr_info("cfm_sku_generic_layout_hdl failed");
			err = -EINVAL;
			break;
		}

		i++;
	}
	of_node_put(it.node);

	if (err < 0) {
		pr_info("spdt layout parse failed");
		goto dt_sku_err;
	}

	sku->spdt.magic_num = CONNFEM_SPDT_MAGIC_NUMBER;

	/* Parse subsys flags nodes */
	err = cfm_dt_epaelna_flags_parse(hw_np, CFM_DT_NODE_FLAGS, flags);

	if (err < 0) {
		pr_info("cfm_dt_epaelna_flags_parse failed");
		err = -EINVAL;
		goto dt_sku_err;
	}

	/* Populate subsys flags:
	 * Here, we do not add a new function to deal with sku flags.
	 * We reuse cfm_epaelna_flags_populate and populate data into
	 * epaelna-related structure.
	*/
	err = cfm_epaelna_flags_populate(flags, flags_cfg);

	if (err < 0) {
		pr_info("cfm_epaelna_flags_populate failed");
		err = -EINVAL;
		goto dt_sku_err;
	}

	cfm_sku_data_dump(sku);

	CFM_DT_PUT_NP(hw_np);
	return 0;

dt_sku_err:
	CFM_DT_PUT_NP(hw_np);
	cfm_dt_sku_data_reset(cfm);
	return err;
}


/**
 * cfm_dt_sku_hw_np_fetch
 *	get hw-N node
 *
 * Parameters
 *	np	: Pointer to the node containing the 'hw-' node.
 *	hwid	: indicate which node we'll catch.
 *
 * Return value
 *	0	: Success
 *	-EINVAL	: Error
 *
 */
static int cfm_dt_sku_hw_np_fetch(
		struct device_node *np,	unsigned int hwid,
		struct device_node **hw_np)
{
	int c = 0;
	char name[sizeof(CFM_DT_NODE_HW_PREFIX) + 10 + 1];

	if (!np || !hw_np) {
		pr_info("No input node");
		return -EINVAL;
	}

	c = snprintf(name, sizeof(name), "%s%u", CFM_DT_NODE_HW_PREFIX, hwid);

	/* Error handling:
	 * c == size is not correct since '\0' should be
	 * taken into account.
	 */
	if (c < 0 || c >= sizeof(name)) {
		pr_info("Concatenate name error %d, sz %u, '%s'%u'",
			c,
			(unsigned int) sizeof(name),
			CFM_DT_NODE_HW_PREFIX,
			hwid);
		return -EINVAL;
	}

	*hw_np = of_get_child_by_name(np, name);
	if (!(*hw_np)) {
		pr_info("Cannot find node of name: %s", name);
		return -ENOENT;
	}

	pr_info("Get node of name: %s", name);

	return 0;
}

/**
 * cfm_dt_sku_fem_data_hdl
 * 	Parse information of a FEM, including:
 * 	(1) FEM info
 * 	(2) FEM truth table
 * 	(3) FEM control pin
 * 	(4) FEM truth table usage.
 *
 * Parameters
 *	np	: Pointer to the node containing properties as
 *		'using-fems', common node, wifi node, bt node.
 *	sku	: It will be populated with fem data.
 *
 * Return value
 *	0	: Success
 *	-EINVAL	: Error
 *
 */
static int cfm_dt_sku_fem_data_hdl(struct device_node *np,
				struct connfem_sku *sku)
{
	int err = 0;
	unsigned int i = 0;
	struct device_node *tmp_np = NULL;

	if(!np || !sku) {
		pr_info("%s: input is NULL", __func__);
		return -EINVAL;
	}

	err = of_property_count_u32_elems(np,
					CFM_DT_PROP_USING_FEMS);

	if (err <= 0) {
		pr_info("Missing '%s' property",
			CFM_DT_PROP_USING_FEMS);
		return -EINVAL;
	}

	sku->fem_count = (unsigned int)err;

	if (sku->fem_count > CONNFEM_SKU_FEM_COUNT) {
		pr_info("FEM count '%d' over '%d' of '%s' property",
			sku->fem_count,
			CONNFEM_SKU_FEM_COUNT,
			CFM_DT_PROP_USING_FEMS);
		return -EINVAL;
	}

	/* Per FEM data parsing	*/
	for (i = 0; i < sku->fem_count; i++) {
		sku->fem[i].magic_num = CONNFEM_FEM_MAGIC_NUMBER;
		tmp_np = of_parse_phandle(np, CFM_DT_PROP_USING_FEMS, i);

		if (!tmp_np) {
			pr_info("%s[%d]: Invalid node at index %d",
				CFM_DT_PROP_USING_FEMS,
				i, i);
			return -EINVAL;
		}

		err = cfm_sku_fem_info_populate(tmp_np,
						&sku->fem[i].info);
		if (err < 0) {
			pr_info("cfm_sku_fem_info_populate failed");
			err = -EINVAL;
			goto dt_sku_fem_data_hdl;
		}

		err = cfm_sku_fem_ctrl_pin_populate(tmp_np,
						&sku->fem[i].ctrl_pin);
		if (err < 0) {
			pr_info("cfm_sku_fem_ctrl_pin_populate failed");
			err = -EINVAL;
			goto dt_sku_fem_data_hdl;
		}

		err = cfm_sku_fem_ttbl_populate(tmp_np,
						&sku->fem[i].tt);
		if (err < 0) {
			pr_info("cfm_sku_fem_ttbl_populate failed");
			err = -EINVAL;
			goto dt_sku_fem_data_hdl;
		}

		CFM_DT_PUT_NP(tmp_np);
	}

	err = cfm_sku_ttbl_usg_hdl(np, sku);
	if (err < 0) {
		pr_info("cfm_sku_ttbl_usg_hdl failed");
		err = -EINVAL;
		goto dt_sku_fem_data_hdl;
	}

	/* Normal success procedure & error will reach here */
dt_sku_fem_data_hdl:
	CFM_DT_PUT_NP(tmp_np);
	return err;
}

/**
 * cfm_dt_sku_layout_data_hdl
 * 	Parse information of a FEM layout
 *
 * Parameters
 *	np	: Pointer to the node containing property
 *		"layout".
 *	connfem_sku: It will be populated with fem data.
 *
 * Return value
 *	0	: Success
 *	-EINVAL	: Error
 *
 */
static int cfm_dt_sku_layout_data_hdl(struct device_node *np,
				struct connfem_sku *cfm_sku)
{
	int err = 0, lerr = 0;
	unsigned int bk_cnt = 0;
	struct property *prop = NULL;
	struct of_phandle_iterator it;

	if(!np || !cfm_sku) {
		pr_info("%s: input is NULL", __func__);
		return -EINVAL;
	}

	bk_cnt = cfm_sku->layout_count;
	prop = of_find_property(np, CFM_DT_PROP_LAYOUT, NULL);

	if (!prop) {
		pr_info("Property '%s' not found", CFM_DT_PROP_LAYOUT);
		return -EINVAL;
	}

	of_for_each_phandle(&it, lerr, np, CFM_DT_PROP_LAYOUT, NULL, 0) {
		if (lerr < 0) {
			pr_info("%s: Invalid node at index %d",
				CFM_DT_PROP_LAYOUT,
				cfm_sku->layout_count);
			err = -EINVAL;
			break;
		}

		if (cfm_sku->layout_count >= CONNFEM_SKU_LAYOUT_COUNT) {
			pr_info("Layout count '%d' over '%d'",
				cfm_sku->layout_count,
				CONNFEM_SKU_LAYOUT_COUNT);
			err = -EINVAL;
			break;
		}

		err = cfm_sku_fem_layout_populate(it.node, cfm_sku,
				&cfm_sku->layout[cfm_sku->layout_count]);
		if (err < 0) {
			pr_info("cfm_sku_fem_layout_populate failed");
			err = -EINVAL;
			break;
		}

		cfm_sku->layout_count++;
	}
	of_node_put(it.node);

	if (err < 0) {
		cfm_sku->layout_count = bk_cnt;
		return -EINVAL;
	}

	return 0;
}

static int cfm_dt_epaelna_flags_name_get(
		unsigned int hwid,
		size_t name_out_size,
		char *name_out)
{
	int c;
	char name[sizeof(CFM_DT_PROP_FLAGS_PREFIX) + 10 + 1];
	size_t bytes_to_copy = 0;

	/* Flags node name is based on hwid, it's the same for all subsys */
	c = snprintf(name, sizeof(name),
		 "%s%u",
		 CFM_DT_PROP_FLAGS_PREFIX, hwid);
	if (c < 0 || c >= sizeof(name)) {
		pr_info("flag node name error %d, sz %u, '%s%u'",
			c,
			(unsigned int)sizeof(name),
			CFM_DT_PROP_FLAGS_PREFIX,
			hwid);
		return -EINVAL;
	}

	/* Prevent OOB */
	bytes_to_copy = sizeof(name) < name_out_size ?
			sizeof(name) : name_out_size;

	memcpy(name_out, name, bytes_to_copy);

	return 0;
}

void cfm_dt_sku_data_reset(struct connfem_sku_context *cfm)
{
	if (!cfm) {
		pr_info("Sku reset no input");
		return;
	}

	cfm_epaelna_flags_free(cfm->flags_cfg);
	cfm_dt_epaelna_flags_free(&cfm->flags);

	memset(&cfm->hwid, 0, sizeof(cfm->hwid));
	memset(&cfm->available, 0, sizeof(cfm->available));
	memset(&cfm->sku, 0, sizeof(cfm->sku));
	memset(&cfm->flags, 0, sizeof(cfm->flags));
	memset(cfm->flags_cfg, 0, sizeof(cfm->flags_cfg));
}
