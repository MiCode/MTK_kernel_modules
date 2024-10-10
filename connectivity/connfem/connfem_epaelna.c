// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
//#include <linux/err.h>
#include <linux/string.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include "connfem.h"

/*******************************************************************************
 *				M A C R O S
 ******************************************************************************/
#define CFM_DT_PROP_VID		"vid"
#define CFM_DT_PROP_PID		"pid"

#define FEMID_VID	0
#define FEMID_PID	1
#define FEMID_ITEMS	2

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

/*******************************************************************************
 *		    F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************/
static int cfm_epaelna_feminfo_part_populate(struct device_node *np,
					     struct connfem_part *result_out);

static int cfm_epaelna_pincfg_mapping_populate(
		struct cfm_dt_epaelna_pctl_data_context *pctl_data,
		struct connfem_epaelna_pin_info *result_out);

static int cfm_epaelna_pincfg_laa_pinmux_populate(
		struct cfm_dt_epaelna_pctl_data_context *pctl_data,
		struct connfem_epaelna_laa_pin_info *result_out);

static int cfm_epaelna_flags_subsys_populate(
		struct device_node *np,
		struct connfem_epaelna_flag_tbl_entry *tbl,
		struct cfm_container **result_out);

/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/
char *cfm_subsys_name[CONNFEM_SUBSYS_NUM] = {
	/* [CONNFEM_SUBSYS_NONE] */ CFM_DT_NODE_COMMON,
	/* [CONNFEM_SUBSYS_WIFI] */ CFM_DT_NODE_WIFI,
	/* [CONNFEM_SUBSYS_BT]   */ CFM_DT_NODE_BT
};

/*******************************************************************************
 *			   P R I V A T E   D A T A
 ******************************************************************************/
/* Align with FemNo coding in firmware
 *   0xAABCDDEE:
 *	[31:24] AA: reserved
 *	[23:20]  B: G-Band VID
 *	[19:16]  C: A-Band VID
 *	[15:8]  DD: G-Band PID
 *	[ 7:0]  EE: A-Band PID
 */
static int fem_id_shift[CONNFEM_PORT_NUM][FEMID_ITEMS] = {
	/* [0]CONNFEM_PORT_WFG */
	{
		/* [0]FEMID_VID */ 20,
		/* [1]FEMID_PID */ 8
	},

	/* [1]CONNFEM_PORT_WFA */
	{
		/* [0]FEMID_VID */ 16,
		/* [1]FEMID_PID */ 0
	}
};

static struct connfem_epaelna_subsys_cb *subsys_cb[CONNFEM_SUBSYS_NUM] = {
	/* [CONNFEM_SUBSYS_NONE] */ &cfm_cm_epaelna_cb,
	/* [CONNFEM_SUBSYS_WIFI] */ &cfm_wf_epaelna_cb,
	/* [CONNFEM_SUBSYS_BT]   */ &cfm_bt_epaelna_cb
};

/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
void cfm_epaelna_config_free(struct cfm_epaelna_config *cfg, bool free_all)
{
	if (!cfg)
		return;

	/* In case we need to keep some critical information for later use,
	 * the data related to FEM info and flags are especially important,
	 * these should only be freed if "free_all" is specified.
	 *
	 * In another word, if free_all == false, the only thing we could
	 * clean up is the PIN related info.
	 */
	if (!free_all) {
		memset(&cfg->pin_cfg, 0, sizeof(cfg->pin_cfg));
		return;
	}

	cfm_epaelna_flags_free(cfg->flags_cfg);

	memset(cfg, 0, sizeof(*cfg));
}

void cfm_epaelna_flags_free(struct cfm_epaelna_flags_config *flags)
{
	if (!flags)
		return;

	if (flags->pairs) {
		cfm_container_free(flags->pairs);
		flags->pairs = NULL;
	}
}

int cfm_epaelna_feminfo_populate(struct device_node **parts_np,
				  struct connfem_epaelna_fem_info *result_out)
{
	struct connfem_epaelna_fem_info result;
	struct device_node *np;
	int i;

	memset(&result, 0, sizeof(result));

	/* Retrieves FEM info from each of the parts nodes
	 * We shall keep as much info as possible, so FW has the chance to
	 * know if a FEM does exist, and could do some FEM protection logic.
	 */
	for (i = 0; i < CONNFEM_PORT_NUM; i++) {
		np = parts_np[i];

		/* VID, PID */
		cfm_epaelna_feminfo_part_populate(np, &result.part[i]);

		/* Encode FEM ID for firmware (FEM Number) */
		result.id = result.id |
			(result.part[i].vid << fem_id_shift[i][FEMID_VID]) |
			(result.part[i].pid << fem_id_shift[i][FEMID_PID]);

		/* Part name */
		strncpy(result.part_name[i], np->name,
			sizeof(result.part_name[i]) - 1);
		result.part_name[i][sizeof(result.part_name[i]) - 1] = 0;
	}

	/* Update output parameter */
	memcpy(result_out, &result,
	       sizeof(result));

	cfm_epaelna_feminfo_dump(result_out);

	return 0;
}

/**
 * cfm_epaelna_feminfo_part_populate
 *	Extract info from a single part node
 *
 * Parameters
 *	np	  : Pointer to the part node containing 'vid'/'pid'
 *	result_out: the result output
 *
 * Return value
 *	0	: Success, result will contain valid value
 *	-EINVAL : Error
 *
 */
static int cfm_epaelna_feminfo_part_populate(struct device_node *np,
					     struct connfem_part *result_out)
{
	int err = 0;
	struct connfem_part result;
	unsigned int value;

	memset(&result, 0, sizeof(result));

	/* VID */
	value = 0;
	err = of_property_read_u32(np, CFM_DT_PROP_VID, &value);
	if (err < 0 || value > 0xFF) {
		pr_info("%s.%s: %d missing or not a 8-bit value, err %d",
			np->name, CFM_DT_PROP_VID, value, err);
		value = 0;
	}
	result.vid = (unsigned char)value;

	/* PID */
	value = 0;
	err = of_property_read_u32(np, CFM_DT_PROP_PID, &value);
	if (err < 0 || value > 0xFF) {
		pr_info("%s.%s: %d missing or not a 8-bit value, err %d",
			np->name, CFM_DT_PROP_PID, value, err);
		value = 0;
	}
	result.pid = (unsigned char)value;

	/* Update output parameter */
	memcpy(result_out, &result, sizeof(result));

	return 0;
}

/**
 * cfm_epaelna_pincfg_populate
 *	Populate pin config from the pinctrl data node.
 *
 *	NOTE: the result will be APPENDED in the output, instead of
 *	overwriting the whole structure as other API does.
 *
 * Parameters
 *	pctl_data: Pointer to the pinctrl data dt context
 *
 * Return value
 *	0	: Success, result output will be valid
 *	-EINVAL : Error
 *
 */
int cfm_epaelna_pincfg_populate(
		struct cfm_dt_epaelna_pctl_data_context *pctl_data,
		struct cfm_epaelna_pin_config *result_out)
{
	int err = 0;

	err = cfm_epaelna_pincfg_mapping_populate(pctl_data,
						  &result_out->pin_info);
	if (err < 0)
		return -EINVAL;

	err = cfm_epaelna_pincfg_laa_pinmux_populate(pctl_data,
						     &result_out->laa_pin_info);
	if (err < 0)
		return -EINVAL;

	return 0;
}

/**
 * cfm_epaelna_pincfg_mapping_populate
 *	Populate ANTSEL & FEM pin mapping from the pinctrl data node
 *
 *	NOTE: the result will be APPENDED in the output, instead of
 *	overwriting the whole structure as other API does.
 *
 * Parameters
 *	pctl_data: Pointer to the pinctrl data dt context
 *
 * Return value
 *	0	: Success, result output will be valid
 *	-EINVAL : Error
 *
 */
static int cfm_epaelna_pincfg_mapping_populate(
		struct cfm_dt_epaelna_pctl_data_context *pctl_data,
		struct connfem_epaelna_pin_info *result_out)
{
	int err = 0;
	int p, i, start;
	unsigned int p_ofst;
	unsigned int value = 0;
	unsigned char *addr;
	struct connfem_epaelna_pin_info result;

	/* Picking up from where we left off */
	memcpy(&result, result_out, sizeof(result));

	/* Check if we still have enough storage for new pins */
	if (pctl_data->pin_cnt + result.count > CONNFEM_EPAELNA_PIN_COUNT) {
		pr_info("Too many ANTSEL PINs! Not enough space for %d",
			pctl_data->pin_cnt);
		cfm_epaelna_pininfo_dump(&result);
		return -ENOMEM;
	}

	p_ofst = result.count;

	p = i = start = 0;
	for (p = 0; p < pctl_data->pin_cnt; p++) {
		start = p * CFM_DT_MAPPING_SIZE;

		for (i = 0; i < CFM_DT_MAPPING_SIZE; i++) {
			err = of_property_read_u32_index(pctl_data->np,
							 CFM_DT_PROP_MAPPING,
							 start + i,
							 &value);
			if (err < 0)
				goto mapping_populate_read_err;

			if (i == CFM_DT_MAPPING_POLARITY_INDEX && value > 1) {
				err = -EINVAL;
				goto mapping_populate_polarity_err;
			}

			if (value > 0xFF) {
				err = -EINVAL;
				goto mapping_populate_outofbound_err;
			}

			addr = (unsigned char *)(&result.pin[p + p_ofst]);
			*(addr + i) = (unsigned char)value;
		}
	}
	result.count += pctl_data->pin_cnt;

	memcpy(result_out, &result, sizeof(result));

	return 0;

mapping_populate_read_err:
	pr_info("%s,pin:%d idx:%d,real idx:%d,Read err %d",
		CFM_DT_PROP_MAPPING,
		p, i, (start + i), err);
	return err;

mapping_populate_polarity_err:
	pr_info("%s,pin:%d idx:%d,real idx:%d,Polarity %d is not 0 or 1",
		CFM_DT_PROP_MAPPING,
		p, i, (start + i), value);
	return err;

mapping_populate_outofbound_err:
	pr_info("%s,pin:%d idx:%d,real idx:%d,Value %d exceeds 8-bit",
		CFM_DT_PROP_MAPPING,
		p, i, (start + i), value);
	return err;
}

/**
 * cfm_epaelna_pincfg_laa_pinmux_populate
 *	Populate LAA pinmux from the pinctrl data node
 *
 *	NOTE: the result will be APPENDED in the output, instead of
 *	overwriting the whole structure as other API does.
 *
 * Parameters
 *	pctl_data: Pointer to the pinctrl data dt context
 *
 * Return value
 *	0	: Success, result output will be valid
 *	-EINVAL : Error
 *
 */
static int cfm_epaelna_pincfg_laa_pinmux_populate(
		struct cfm_dt_epaelna_pctl_data_context *pctl_data,
		struct connfem_epaelna_laa_pin_info *result_out)
{
	int err = 0;
	int p, i, start;
	unsigned int p_ofst;
	unsigned int value[CFM_DT_LAA_PINMUX_SIZE] = {0};
	struct connfem_epaelna_laa_pin_info result;

	/* Picking up from where we left off */
	memcpy(&result, result_out, sizeof(result));

	/* Check if we still have enough storage for new pins */
	if (pctl_data->laa_cnt + result.count >
	    CONNFEM_EPAELNA_LAA_PIN_COUNT) {
		pr_info("Too many LAA PINs! Not enough space for %d",
			pctl_data->laa_cnt);
		cfm_epaelna_laainfo_dump(&result);
		return -ENOMEM;
	}

	p_ofst = result.count;

	p = i = start = 0;
	for (p = 0; p < pctl_data->laa_cnt; p++) {
		start = p * CFM_DT_LAA_PINMUX_SIZE;

		for (i = 0; i < CFM_DT_LAA_PINMUX_SIZE; i++) {
			err = of_property_read_u32_index(pctl_data->np,
							 CFM_DT_PROP_LAA_PINMUX,
							 start + i,
							 &value[i]);
			if (err < 0)
				goto laa_pinmux_populate_read_err;

			if (CFM_DT_GET_PIN_NO(value[i]) > 0xFFFF) {
				err = -EINVAL;
				goto laa_pinmux_populate_pin_outofbound_err;
			}
		}

		if (CFM_DT_GET_PIN_NO(value[CFM_DT_LAA_PINMUX_WF_INDEX]) !=
		    CFM_DT_GET_PIN_NO(value[CFM_DT_LAA_PINMUX_MD_INDEX])) {
			err = -EINVAL;
			goto laa_pinmux_populate_pin_mismatch_err;
		}

		result.pin[p + p_ofst].gpio =
			CFM_DT_GET_PIN_NO(value[CFM_DT_LAA_PINMUX_WF_INDEX]);

		result.pin[p + p_ofst].wf_mode =
			CFM_DT_GET_PIN_FUNC(value[CFM_DT_LAA_PINMUX_WF_INDEX]);

		result.pin[p + p_ofst].md_mode =
			CFM_DT_GET_PIN_FUNC(value[CFM_DT_LAA_PINMUX_MD_INDEX]);
	}
	result.count += pctl_data->laa_cnt;

	memcpy(result_out, &result, sizeof(result));
	return 0;

laa_pinmux_populate_read_err:
	pr_info("%s,pin:%d idx:%d,real idx:%d,Read err %d",
		CFM_DT_PROP_LAA_PINMUX,
		p, i, (start + i), err);
	return err;

laa_pinmux_populate_pin_outofbound_err:
	pr_info("%s,pin:%d idx:%d,real idx:%d,Value 0x%x,GPIO exceed %d",
		CFM_DT_PROP_LAA_PINMUX,
		p, i, (start + i),
		value[i], 0xFFFF);
	return err;

laa_pinmux_populate_pin_mismatch_err:
	pr_info("%s,pin:%d idx:%d,GPIO mismatch WF:%d/MD:%d",
		CFM_DT_PROP_LAA_PINMUX,
		p, start,
		CFM_DT_GET_PIN_NO(value[CFM_DT_LAA_PINMUX_WF_INDEX]),
		CFM_DT_GET_PIN_NO(value[CFM_DT_LAA_PINMUX_MD_INDEX]));
	return err;
}

/**
 * cfm_epaelna_flags_populate
 *	Populate subsys' flags from the parsed flags node
 *
 * Parameters
 *	dt_flags: Pointer to the flags dt context
 *	result  : Pointer to an array of size CONNFEM_SUBSYS_NUM, and
 *		  element type of struct cfm_epaelna_flags_config
 *
 * Return value
 *	0	: Success, result output will be valid
 *	-ENOMEM : Out of memory
 *	-EINVAL : Error
 *
 */
int cfm_epaelna_flags_populate(
	struct cfm_dt_epaelna_flags_context *dt_flags,
	struct cfm_epaelna_flags_config *result_out)
{
	int err = 0;
	int s;
	struct connfem_epaelna_flag_tbl_entry *tbl = NULL;
	struct cfm_epaelna_flags_config result[CONNFEM_SUBSYS_NUM];

	memset(&result, 0, sizeof(result));

	for (s = 0; s < CONNFEM_SUBSYS_NUM; s++) {
		if (!dt_flags->np[s] || !subsys_cb[s])
			continue;

		if (!subsys_cb[s]->flags_get) {
			pr_info("Undefined %s.flags_get",
				cfm_subsys_name[s]);
			continue;
		}

		if (!subsys_cb[s]->flags_tbl_get) {
			pr_info("Undefined %s.flags_tbl_get",
				cfm_subsys_name[s]);
			continue;
		}

		tbl = subsys_cb[s]->flags_tbl_get();
		if (!tbl) {
			pr_info("%s flags mapping table is NULL",
				cfm_subsys_name[s]);
			continue;
		}

		err = cfm_epaelna_flags_subsys_populate(dt_flags->np[s],
							tbl,
							&result[s].pairs);
		if (err < 0)
			break;

		cfm_epaelna_flags_pairs_dump(s, result[s].pairs);

		result[s].obj = subsys_cb[s]->flags_get();
		if (!result[s].obj) {
			/* If subsys doesn't want to return its flags struct,
			 * doesn't harm us, so continue normally...
			 */
			pr_info("%s flags structure is NULL, continue..",
				cfm_subsys_name[s]);
		}
	}

	if (err < 0) {
		pr_info("Error while populating %s '%s'",
			cfm_subsys_name[s], dt_flags->node_name);

		for (s = 0; s < CONNFEM_SUBSYS_NUM; s++)
			cfm_epaelna_flags_free(&result[s]);
	} else {
		memcpy(result_out, &result, sizeof(result));
	}

	return err;
}

/**
 * cfm_epaelna_flags_subsys_populate
 *	Function traverses through all props in the give subsys' flags node,
 *	and updates subsys' flags structure through mapping table,
 *	and collect flag name/value pairs into the ConnFem container.
 *
 *	On success, the container will be allocated, caller needs to release it
 *	via cfm_container_free().
 *
 * Parameters
 *	np	  : Pointer to subsys' flags device tree node
 *	tbl	  : Pointer to subsys' flags mapping table
 *	result_out: Pointer to ConnFem container for storing flags name/value
 *
 * Return value
 *	0	: Success, result output will be valid
 *	-ENOMEM : Out of memory
 *
 */
static int cfm_epaelna_flags_subsys_populate(
		struct device_node *np,
		struct connfem_epaelna_flag_tbl_entry *tbl,
		struct cfm_container **result_out)
{
	unsigned int i, len;
	struct property *prop = NULL;
	struct connfem_epaelna_flag_tbl_entry *entry;
	struct cfm_container *result = NULL;
	struct connfem_epaelna_flag_pair *pair;

	/* Prepare flags pairs container storage */
	i = 0;
	for_each_property_of_node(np, prop) {
		i++;
	}

	result = cfm_container_alloc(i, sizeof(struct connfem_epaelna_flag_pair));
	if (!result)
		return -ENOMEM;

	/* Retrieves flags name from device tree */
	i = 0;
	for_each_property_of_node(np, prop) {
		/* Skip built-in property 'name' */
		if (strcmp(prop->name, "name") == 0)
			continue;

		/* Skip if not supported by subsys */
		entry = cfm_epaelna_flags_subsys_find(prop->name, tbl);
		if (!entry)
			continue;

		/* Discard flag with name exceeding length limit */
		len = strlen(prop->name) + 1;
		if (len > CONNFEM_FLAG_NAME_SIZE) {
			pr_info("Drop '%s' prop, len %u > %u",
				prop->name,
				len - 1,
				CONNFEM_FLAG_NAME_SIZE - 1);
			continue;
		}

		/* Double check if container is big enough to keep this flag */
		pair = cfm_container_entry(result, i);
		if (i >= result->cnt || !pair) {
			pr_info("[ERR] Drop '%s' prop, too many flags %d > %d",
				prop->name, i + 1, result->cnt);
			continue;
		}

		/* Update subsys' flags value */
		if (prop->length == 0) {
			/* always enable boolean flag */
			*(entry->addr) = true;

		} else if (prop->length == 1 && prop->value) {
			/* assign the specified 1 byte value */
			*(entry->addr) = *((unsigned char*)prop->value);

		} else {
			/* skip property with multi-bytes value */
			pr_info("Drop '%s' prop, multi-bytes(%d) value unsupported",
				prop->name,
				prop->length);
			continue;
		}

		/* Update subsys' flags container entry */
		memcpy(pair->name, prop->name, len);
		pair->value = *(entry->addr);
		i++;
	}

	/* Updates container size, the end result could be smaller,
	 * due to removal of long name and built-in property.
	 */
	result->cnt = i;

	*result_out = result;

	return 0;
}

/**
 * connfem_epaelna_flag_tbl_entry
 *	Function traverses through all props in the give subsys' flags node,
 *	and updates subsys' flags structure through mapping table,
 *	and collect flag name/value pairs into the ConnFem container.
 *
 *	On success, the container will be allocated, caller needs to release it
 *	via cfm_container_free().
 *
 * Parameters
 *	name: Name of the flag to search
 *	tbl : Pointer to subsys' flags mapping table
 *
 * Return value
 *	Pointer to the subsys' flag table entry, or NULL if can't be found.
 *
 */
struct connfem_epaelna_flag_tbl_entry*
	cfm_epaelna_flags_subsys_find(
		char *name,
		struct connfem_epaelna_flag_tbl_entry *tbl)
{
	struct connfem_epaelna_flag_tbl_entry *iter = tbl;

	while (iter && (iter->name != NULL && iter->addr != NULL)) {
		if (strncmp(name, iter->name, CONNFEM_FLAG_NAME_SIZE) == 0)
			return iter;
		iter++;
	}
	pr_info("Unsupported flag '%s'", name);
	return NULL;
}

struct connfem_epaelna_subsys_cb*
	cfm_epaelna_flags_subsys_cb_get(
		enum connfem_subsys subsys)
{
	if (subsys >= CONNFEM_SUBSYS_NUM) {
		pr_info("%s, invalid subsys = %d", __func__, subsys);
		return NULL;
	}

	return subsys_cb[subsys];
}

void cfm_epaelna_config_dump(struct cfm_epaelna_config *cfg)
{
	int i;

	if (!cfg) {
		pr_info("ePAeLNA Config, (null)");
		return;
	}

	pr_info("ePAeLNA Config, available:%d", cfg->available);

	cfm_epaelna_feminfo_dump(&cfg->fem_info);
	cfm_epaelna_feminfo_dump(&cfg->bt_fem_info);
	cfm_epaelna_pininfo_dump(&cfg->pin_cfg.pin_info);
	cfm_epaelna_pininfo_dump(&cfg->bt_pin_cfg.pin_info);
	cfm_epaelna_laainfo_dump(&cfg->pin_cfg.laa_pin_info);

	for (i = 0; i < CONNFEM_SUBSYS_NUM; i++)
		cfm_epaelna_flags_dump(i, &cfg->flags_cfg[i]);
}

void cfm_epaelna_feminfo_dump(struct connfem_epaelna_fem_info *fem_info)
{
	int i;

	if (!fem_info) {
		pr_info("FemInfo, (null)");
		return;
	}

	pr_info("FemInfo, id:0x%08x", fem_info->id);

	for (i = 0; i < CONNFEM_PORT_NUM; i++) {
		pr_info("FemInfo, [%d]vid:0x%02x,pid:0x%02x,name:'%s'",
			i,
			fem_info->part[i].vid,
			fem_info->part[i].pid,
			fem_info->part_name[i]);
	}
}

void cfm_epaelna_pininfo_dump(struct connfem_epaelna_pin_info *pin_info)
{
	int i, c;
	/* Multiply by 5 to align the maximum size like "0xcc," in fem log */
	char ant_log[(CONNFEM_EPAELNA_PIN_COUNT * 5) + 1] = {0};
	char fem_log[(CONNFEM_EPAELNA_PIN_COUNT * 5) + 1] = {0};
	char pol_log[(CONNFEM_EPAELNA_PIN_COUNT * 5) + 1] = {0};
	int ant_pos = 0;
	int fem_pos = 0;
	int pol_pos = 0;

	if (!pin_info) {
		pr_info("PinInfo, (null)");
		return;
	}

	pr_info("PinInfo, count:%d, max:%d",
		pin_info->count, CONNFEM_EPAELNA_PIN_COUNT);

	for (i = 0; i < pin_info->count; i++) {
		if (ant_pos >= sizeof(ant_log) - 1) {
			pr_info("ant_pos:%d >= ant_log size:%zu",
				ant_pos,
				sizeof(ant_log) - 1);
				break;
		}
		c = snprintf(ant_log + ant_pos, sizeof(ant_log) - ant_pos,
			"%4d,",
			pin_info->pin[i].antsel);
		if (c < 0 || c >= sizeof(ant_log) - ant_pos) {
			pr_info("c:%d,ant_log size:%zu",
				c,
				sizeof(ant_log));
				break;
		} else {
			ant_pos += c;
		}

		if (fem_pos >= sizeof(fem_log) - 1) {
			pr_info("fem_pos:%d > fem_log size:%zu",
				fem_pos,
				sizeof(fem_log) - 1);
				break;
		}
		c = snprintf(fem_log + fem_pos, sizeof(fem_log) - fem_pos,
			"0x%02x,",
			pin_info->pin[i].fem);
		if (c < 0 || c >= sizeof(fem_log) - fem_pos) {
			pr_info("c:%d,fem_log size:%zu",
				c,
				sizeof(fem_log));
				break;
		} else {
			fem_pos += c;
		}

		if (pol_pos >= sizeof(pol_log) - 1) {
			pr_info("pol_pos:%d > pol_log size:%zu",
				pol_pos,
				sizeof(pol_log) - 1);
				break;
		}
		c = snprintf(pol_log + pol_pos, sizeof(pol_log) - pol_pos,
			"%4d,",
			pin_info->pin[i].polarity);
		if (c < 0 || c >= sizeof(pol_log) - pol_pos) {
			pr_info("c:%d,pol_log size:%zu",
				c,
				sizeof(pol_log));
				break;
		} else {
			pol_pos += c;
		}
	}

	if (pin_info->count > 0) {
		pr_info("ant:%s", ant_log);
		pr_info("fem:%s", fem_log);
		pr_info("pol:%s", pol_log);
	}
}

void cfm_epaelna_laainfo_dump(struct connfem_epaelna_laa_pin_info *laa)
{
	int i;

	if (!laa) {
		pr_info("LaaInfo, (null)");
		return;
	}

	pr_info("LaaInfo, count:%d, max:%d, id:0x%x",
		laa->count, CONNFEM_EPAELNA_LAA_PIN_COUNT, laa->chip_id);

	for (i = 0; i < laa->count; i++) {
		pr_info("LaaInfo, [%d]gpio:%d,wf:%d,md:%d",
			i,
			laa->pin[i].gpio,
			laa->pin[i].wf_mode,
			laa->pin[i].md_mode);
	}
}

/* Dump a single subsys' flags config */
void cfm_epaelna_flags_dump(enum connfem_subsys subsys,
			    struct cfm_epaelna_flags_config *flags)
{
	if (subsys >= CONNFEM_SUBSYS_NUM)
		return;

	if (!flags) {
		pr_info("Flags, %s (null)", cfm_subsys_name[subsys]);
		return;
	}

	cfm_epaelna_flags_obj_dump(subsys, flags->obj);

	cfm_epaelna_flags_pairs_dump(subsys, flags->pairs);
}

void cfm_epaelna_flags_obj_dump(enum connfem_subsys subsys,
				void *flags_obj)
{
	struct connfem_epaelna_flags_common *cm_flags = NULL;
	struct connfem_epaelna_flags_wifi *wf_flags = NULL;
	struct connfem_epaelna_flags_bt *bt_flags = NULL;

	if (!flags_obj) {
		pr_info("FlagsObj, %s (null)", cfm_subsys_name[subsys]);
		return;
	}

	switch (subsys) {
	case CONNFEM_SUBSYS_NONE:
		cm_flags = (struct connfem_epaelna_flags_common *)flags_obj;
		pr_info("CmFlags.rxmode: 0x%02x", cm_flags->rxmode);
		pr_info("CmFlags.fe_ant_cnt: 0x%02x", cm_flags->fe_ant_cnt);
		pr_info("CmFlags.fe_main_bt_share_lp2g: 0x%02x", cm_flags->fe_main_bt_share_lp2g);
		pr_info("CmFlags.fe_conn_spdt: 0x%02x", cm_flags->fe_conn_spdt);
		pr_info("CmFlags.fe_reserved: 0x%02x", cm_flags->fe_reserved);
		pr_info("CmFlags.bd_type: 0x%02x", cm_flags->bd_type);
		break;

	case CONNFEM_SUBSYS_WIFI:
		wf_flags = (struct connfem_epaelna_flags_wifi *)flags_obj;
		pr_info("WfFlags.open_loop: %d", wf_flags->open_loop);
		pr_info("WfFlags.laa: %d", wf_flags->laa);
		break;

	case CONNFEM_SUBSYS_BT:
		bt_flags = (struct connfem_epaelna_flags_bt *)flags_obj;
		pr_info("BtFlags.bypass: %d", bt_flags->bypass);
		pr_info("BtFlags.epa_elna: %d", bt_flags->epa_elna);
		pr_info("BtFlags.elna: %d", bt_flags->elna);
		pr_info("BtFlags.epa: %d", bt_flags->epa);
		break;

	default:
		break;
	}
}

void cfm_epaelna_flags_pairs_dump(enum connfem_subsys subsys,
				  struct cfm_container *pairs)
{
	int i;
	struct connfem_epaelna_flag_pair *pair;

	if (!pairs) {
		pr_info("[%s]FlagPairs, (null)", cfm_subsys_name[subsys]);
		return;
	}

	pr_info("[%s]FlagPairs, count:%d, entry_sz:%d",
		cfm_subsys_name[subsys], pairs->cnt, pairs->entry_sz);

	for (i = 0; i < pairs->cnt; i++) {
		pair = (struct connfem_epaelna_flag_pair*)
			cfm_container_entry(pairs, i);
		if (pair) {
			pr_info("[%s]FlagPairs, [%d]'%s':0x%02x",
				cfm_subsys_name[subsys],
				i,
				pair->name,
				pair->value);
		} else {
			pr_info("[%s]FlagPairs, [%d]pair = NULL",
				cfm_subsys_name[subsys],
				i);
		}
	}
}
