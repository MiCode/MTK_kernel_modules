// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>

#if (CFG_SUPPORT_CONNFEM == 1 && CFG_CONNFEM_DEFAULT == 1)
#include "connfem_api.h"
#include "gl_os.h"

/*******************************************************************************
 *				M A C R O S
 ******************************************************************************/
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "["KBUILD_MODNAME"][CONNFEM]" fmt

/*******************************************************************************
 *			    D A T A   T Y P E S
 ******************************************************************************/
struct cfm_default_data {
	struct connfem_sku *sku;
	struct connfem_epaelna_flag_pair *flags[CONNFEM_SUBSYS_NUM];
};

/*******************************************************************************
 *		    F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************/
static void cfm_sku_dump(const struct connfem_sku *sku);


/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/

/*******************************************************************************
 *			   P R I V A T E   D A T A
 ******************************************************************************/
static struct connfem_epaelna_flag_pair cfm_data_flags_cm_sku1[] = {
	{"fe-ant-cnt",		0x96},
	{"fe-conn-dpdt-sp3t",	0x80},
	{"fe-conn-spdt",	0x98},
	{"fe-bt-wf-usage",	0x98},
	{"fe-conn-spdt-2",	0x80},
	{"",			0x00}
};

static struct connfem_epaelna_flag_pair cfm_data_flags_bt_sku1[] = {
	{"efem-mode",	0x03},
	{"rx-mode",	0x34},
	{"",		0x00}
};

static struct connfem_sku cfm_data_sku1 = {
	.fem_count = 2,
	.fem = {
		{
			.info = {
				.vid = 0x03,
				.pid = 0x05,
				.flag = 0x00000000,
				.name = "qm42655"
			},
			.ctrl_pin = {
				.count = 4,
				.id = {0x03, 0x02, 0x04, 0x0e}
			},
			.tt = {
				.logic_count = 15,
				.logic = {
					{0x00, 0x08},
					{0x10, 0x00},
					{0x11, 0x0a},
					{0x12, 0x02},
					{0x20, 0x07},
					{0x21, 0x09},
					{0x22, 0x05},
					{0x23, 0x0b},
					{0x24, 0x03},
					{0x25, 0x0d},
					{0x26, 0x01},
					{0x9f, 0x0c},
					{0x90, 0x04},
					{0x91, 0x0e},
					{0x92, 0x06}
				}
			},
			.tt_usage_wf = {
				.cat_count = 3,
				.cat = {
					{
						.id = 0x00,
						.op_count = 1,
						.op = {0x10}
					},
					{
						.id = 0x01,
						.op_count = 2,
						.op = {0x10, 0x11}
					},
					{
						.id = 0x02,
						.op_count = 7,
						.op = {
							0x20,
							0x21,
							0x22,
							0x23,
							0x24,
							0x25,
							0x26
						}
					}
				}
			}
		},
		{
			.info = {
				.vid = 0x03,
				.pid = 0x06,
				.flag = 0x00000000,
				.name = "qm45655"
			},
			.ctrl_pin = {
				.count = 4,
				.id = {0x0a, 0x0b, 0x0c, 0x01}
			},
			.tt = {
				.logic_count = 12,
				.logic = {
					{0x00, 0x00},
					{0x10, 0x0a},
					{0x11, 0x08},
					{0x12, 0x02},
					{0x20, 0x07},
					{0x21, 0x04},
					{0x22, 0x05},
					{0x23, 0x03},
					{0x24, 0x06},
					{0x25, 0x01},
					{0x90, 0x0f},
					{0x91, 0x03}
				}
			},
			.tt_usage_wf = {
				.cat_count = 3,
				.cat = {
					{
						.id = 0x00,
						.op_count = 1,
						.op = {0x00}
					},
					{
						.id = 0x01,
						.op_count = 2,
						.op = {0x10, 0x11}
					},
					{
						.id = 0x02,
						.op_count = 6,
						.op = {
							0x20,
							0x21,
							0x22,
							0x23,
							0x24,
							0x25
						}
					}
				}
			}
		}
	},

	.layout_flag = 0x00000000,
	.layout_count = 5,
	.layout = {
		{
			.fem_idx = 0,
			.bandpath = {0x00, 0x00, 0x50},
			.pin_count = 4,
			.pinmap = {
				{4, 0x03, 0},
				{5, 0x02, 0},
				{6, 0x04, 0},
				{7, 0x0e, 0}
			}
		},
		{
			.fem_idx = 0,
			.bandpath = {0x00, 0x01, 0x51},
			.pin_count = 4,
			.pinmap = {
				{18, 0x03, 0},
				{19, 0x02, 0},
				{20, 0x04, 0},
				{21, 0x0e, 0}
			}
		},
		{
			.fem_idx = 1,
			.bandpath = {0x00, 0x10, 0xff},
			.pin_count = 4,
			.pinmap = {
				{8, 0x0a, 0},
				{9, 0x0b, 0},
				{10, 0x0c, 0},
				{11, 0x01, 0}
			}
		},
		{
			.fem_idx = 1,
			.bandpath = {0x00, 0x11, 0xff},
			.pin_count = 4,
			.pinmap = {
				{14, 0x0a, 0},
				{15, 0x0b, 0},
				{16, 0x0c, 0},
				{17, 0x01, 0}
			}
		},
		{
			.fem_idx = 1,
			.bandpath = {0x00, 0x12, 0x60},
			.pin_count = 4,
			.pinmap = {
				{0, 0x0a, 0},
				{1, 0x0b, 0},
				{2, 0x0c, 0},
				{3, 0x01, 0}
			}
		}
	},

	.spdt = {
		.pin_count = 2,
		.pinmap = {
			{12, 0x07, 0},
			{13, 0x09, 0}
		}
	}
};

#ifdef CFG_COMBO_SLT_DUT
static struct connfem_epaelna_flag_pair cfm_data_flags_cm_slt2[] = {
	{"fe-ant-cnt",		0x96},
	{"fe-conn-dpdt-sp3t",	0x80},
	{"fe-conn-spdt",	0x88},
	{"fe-bt-wf-usage",	0x98},
	{"fe-conn-spdt-2",	0x80},
	{"",			0x00}
};

static struct connfem_epaelna_flag_pair cfm_data_flags_bt_slt2[] = {
	{"efem-mode",	0x03},
	{"rx-mode",	0x10},
	{"",		0x00}
};

static struct connfem_sku cfm_data_slt2 = {
	.fem_count = 1,
	.fem = {
		{
			.info = {
				.vid = 0x03,
				.pid = 0x06,
				.flag = 0x00000000,
				.name = "qm45655"
			},
			.ctrl_pin = {
				.count = 4,
				.id = {0x0a, 0x0b, 0x0c, 0x01}
			},
			.tt = {
				.logic_count = 12,
				.logic = {
					{0x00, 0x00},
					{0x10, 0x0a},
					{0x11, 0x08},
					{0x12, 0x02},
					{0x20, 0x07},
					{0x21, 0x04},
					{0x22, 0x05},
					{0x23, 0x03},
					{0x24, 0x06},
					{0x25, 0x01},
					{0x90, 0x0f},
					{0x91, 0x03}
				}
			},
			.tt_usage_wf = {
				.cat_count = 3,
				.cat = {
					{
						.id = 0x00,
						.op_count = 1,
						.op = {0x00}
					},
					{
						.id = 0x01,
						.op_count = 2,
						.op = {0x10, 0x11}
					},
					{
						.id = 0x02,
						.op_count = 6,
						.op = {
							0x20,
							0x21,
							0x22,
							0x23,
							0x24,
							0x25
						}
					}
				}
			}
		}
	},

	.layout_flag = 0x00000000,
	.layout_count = 3,
	.layout = {
		{
			.fem_idx = 0,
			.bandpath = {0x00, 0x10, 0xff},
			.pin_count = 4,
			.pinmap = {
				{8, 0x0a, 0},
				{9, 0x0b, 0},
				{10, 0x0c, 0},
				{11, 0x01, 0}
			}
		},
		{
			.fem_idx = 0,
			.bandpath = {0x00, 0x11, 0xff},
			.pin_count = 4,
			.pinmap = {
				{14, 0x0a, 0},
				{15, 0x0b, 0},
				{16, 0x0c, 0},
				{17, 0x01, 0}
			}
		},
		{
			.fem_idx = 0,
			.bandpath = {0x00, 0x12, 0x60},
			.pin_count = 4,
			.pinmap = {
				{0, 0x0a, 0},
				{1, 0x0b, 0},
				{2, 0x0c, 0},
				{3, 0x01, 0}
			}
		}
	},

	.spdt = {
		.pin_count = 1,
		.pinmap = {
			{12, 0x07, 0}
		}
	}
};
#endif  /* CFG_COMBO_SLT_DUT */

static struct cfm_default_data cfm_default[] = {
	{
		.sku = &cfm_data_sku1,
		.flags = {
			/* [CONNFEM_SUBSYS_NONE] */ cfm_data_flags_cm_sku1,
			/* [CONNFEM_SUBSYS_WIFI] */ NULL,
			/* [CONNFEM_SUBSYS_BT] */ cfm_data_flags_bt_sku1
		}
	},
#ifdef CFG_COMBO_SLT_DUT
	{
		.sku = &cfm_data_slt2,
		.flags = {
			/* [CONNFEM_SUBSYS_NONE] */ cfm_data_flags_cm_slt2,
			/* [CONNFEM_SUBSYS_WIFI] */ NULL,
			/* [CONNFEM_SUBSYS_BT] */ cfm_data_flags_bt_slt2
		}
	},
#endif
};

static const unsigned int cfm_default_count = ARRAY_SIZE(cfm_default);

/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
bool connfem_is_available(enum connfem_type fem_type)
{
	bool res;

	res = (fem_type == CONNFEM_TYPE_SKU);
	pr_info("%s, Default setting: %d", __func__, res);
	return res;
}

int connfem_epaelna_get_fem_info(struct connfem_epaelna_fem_info *fem_info)
{
	pr_info("%s, Default setting: Not support", __func__);
	memset(fem_info, 0, sizeof(*fem_info));
	return -EOPNOTSUPP;
}

int connfem_epaelna_get_bt_fem_info(struct connfem_epaelna_fem_info *fem_info)
{
	pr_info("%s, Default setting: Not support", __func__);
	memset(fem_info, 0, sizeof(*fem_info));
	return -EOPNOTSUPP;
}

int connfem_epaelna_get_pin_info(struct connfem_epaelna_pin_info *pin_info)
{
	pr_info("%s, Default setting: Not support", __func__);
	memset(pin_info, 0, sizeof(*pin_info));
	return -EOPNOTSUPP;
}

int connfem_epaelna_laa_get_pin_info(
		struct connfem_epaelna_laa_pin_info *laa_pin_info)
{
	pr_info("%s, Default setting: Not support", __func__);
	memset(laa_pin_info, 0, sizeof(*laa_pin_info));
	return -EOPNOTSUPP;
}

int connfem_epaelna_get_flags(enum connfem_subsys subsys, void *flags)
{
	pr_info("%s, Default setting: Not support", __func__);
	return -EOPNOTSUPP;
}

int connfem_sku_data(const struct connfem_sku **sku)
{
	unsigned int connfem_id;

	connfem_id = (unsigned int)wlanConnFemGetId();

	if (!sku) {
		pr_info("%s, No sku ptr input", __func__);
		return -EINVAL;
	}

	if (connfem_id >= cfm_default_count) {
		pr_info("%s, ConnfemId %d exceeds maximum value %d",
			__func__, connfem_id, cfm_default_count - 1);
		return -EINVAL;
	}

	*sku = cfm_default[connfem_id].sku;

	cfm_sku_dump(*sku);

	return 0;
}

int connfem_sku_flag_u8(enum connfem_subsys subsys,
			const char *name,
			unsigned char *value)
{
	unsigned int connfem_id;
	struct connfem_epaelna_flag_pair *flags = NULL;

	connfem_id = (unsigned int)wlanConnFemGetId();

	if (!value || !name) {
		pr_info("%s, input parameter is NULL", __func__);
		return -EINVAL;
	}

	if (subsys >= CONNFEM_SUBSYS_NUM) {
		pr_info("%s, Invalid subsys %d", __func__, subsys);
		return -EINVAL;
	}

	if (connfem_id >= cfm_default_count) {
		pr_info("%s, ConnfemId %d exceeds maximum value %d",
			__func__, connfem_id, cfm_default_count - 1);
		return -EINVAL;
	}

	flags = cfm_default[connfem_id].flags[subsys];
	if (!flags) {
		pr_info("%s, Default setting: Unsupported subsys %d",
			__func__, subsys);
	}

	while (flags && flags->name[0] != 0) {
		if (strncasecmp(name, flags->name, sizeof(flags->name)) == 0) {
			pr_info("%s, Default setting: Subsys %d '%s': 0x%02x",
				__func__, subsys, name, flags->value);
			*value = flags->value;
			return 0;
		}
		flags++;
	}

	pr_info("%s, Default setting: Subsys %d does not have '%s'",
		__func__, subsys, name);

	return -ENOENT;
}

static void cfm_sku_dump(const struct connfem_sku *sku)
{
	int f, i, j;
	const struct connfem_sku_fem_truth_table_usage *usage = NULL;

	if (!sku)
		return;

	pr_info("fem_count: %d\n", sku->fem_count);
	for (f = 0; f < sku->fem_count; f++) {
		pr_info("fem[%d]\n", f);
		pr_info("\tinfo\n");
		pr_info("\t\tvid : 0x%02x\n", sku->fem[f].info.vid);
		pr_info("\t\tpid : 0x%02x\n", sku->fem[f].info.pid);
		pr_info("\t\tflag: 0x%08x\n", sku->fem[f].info.flag);
		pr_info("\t\tname: '%s'\n", sku->fem[f].info.name);

		pr_info("\tctrl_pin\n");
		pr_info("\t\tcount: %d\n", sku->fem[f].ctrl_pin.count);
		for (i = 0; i < sku->fem[f].ctrl_pin.count; i++) {
			pr_info("\t\tid[%d]: 0x%02x\n",
				i, sku->fem[f].ctrl_pin.id[i]);
		}

		pr_info("\ttt\n");
		pr_info("\t\tlogic_count: %d\n", sku->fem[f].tt.logic_count);
		for (i = 0; i < sku->fem[f].tt.logic_count; i++) {
			pr_info("\t\tlogic[%d]: {op:0x%02x, binary:0x%02x}\n",
				i,
				sku->fem[f].tt.logic[i].op,
				sku->fem[f].tt.logic[i].binary);
		}

		pr_info("\ttt_usage_wf\n");
		usage = &sku->fem[f].tt_usage_wf;

		pr_info("\t\tcat_count: %d\n",
			usage->cat_count);

		for (i = 0; i < usage->cat_count; i++) {
			pr_info("\t\tcat[%d]: {id:0x%02x, op_count:%d}\n",
				i,
				usage->cat[i].id,
				usage->cat[i].op_count);

			for (j = 0; j < usage->cat[i].op_count; j++) {
				pr_info("\t\t\top[%d]: 0x%02x\n",
					j,
					usage->cat[i].op[j]);
			}
		}
	}

	pr_info("layout_flag: 0x%08x\n", sku->layout_flag);
	pr_info("layout_count: %d\n", sku->layout_count);
	for (i = 0; i < sku->layout_count; i++) {
		pr_info("layout[%d]\n", i);
		if (sku->layout[i].fem_idx >= sku->fem_count) {
			pr_info("\t\tfem_idx: %d => ERROR: Only %d FEMs defined\n",
				sku->layout[i].fem_idx,
				sku->fem_count);
		} else {
			pr_info("\t\tfem_idx: %d ('%s')\n",
				sku->layout[i].fem_idx,
				sku->fem[sku->layout[i].fem_idx].info.name);
		}

		pr_info("\t\tbandpath: {WF:0x%02x, BT:0x%02x}\n",
			sku->layout[i].bandpath[CONNFEM_SUBSYS_WIFI],
			sku->layout[i].bandpath[CONNFEM_SUBSYS_BT]);

		pr_info("\t\tpin_count: %d\n", sku->layout[i].pin_count);
		for (j = 0; j < sku->layout[i].pin_count; j++) {
			pr_info("\t\tpinmap[%d]: {%d, 0x%02x, 0x%02x}\n",
			j,
			sku->layout[i].pinmap[j].pin1,
			sku->layout[i].pinmap[j].pin2,
			sku->layout[i].pinmap[j].flag);
		}
	}

	pr_info("spdt\n");
	pr_info("\tpin_count: %d\n", sku->spdt.pin_count);
	for (j = 0; j < sku->spdt.pin_count; j++) {
		pr_info("\tpinmap[%d]: {%d, 0x%02x, 0x%02x}\n",
			j,
			sku->spdt.pinmap[j].pin1,
			sku->spdt.pinmap[j].pin2,
			sku->spdt.pinmap[j].flag);
	}
}
#endif  /* (CFG_SUPPORT_CONNFEM == 1 && CFG_CONNFEM_DEFAULT == 1) */
