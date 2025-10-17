/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __CONNFEM_API_H__
#define __CONNFEM_API_H__

/*******************************************************************************
 *				M A C R O S
 ******************************************************************************/
#define CONNFEM_API_VERSION		2

#define CONNFEM_PART_NAME_SIZE		64
#define CONNFEM_FLAG_NAME_SIZE		32

/*
 * ConnFem v1 - CONNFEM_TYPE_EPAELNA
 */
#define CONNFEM_EPAELNA_PIN_COUNT	32
#define CONNFEM_EPAELNA_LAA_PIN_COUNT	8

/*
 * ConnFem v2 - CONNFEM_TYPE_SKU
 */
#define CONNFEM_SKU_FEM_COUNT		8
#define CONNFEM_FEM_PIN_COUNT		8
#define CONNFEM_FEM_LOGIC_COUNT		32
#define CONNFEM_FEM_LOGIC_CAT_COUNT	8

#define CONNFEM_SKU_LAYOUT_COUNT	16
#define CONNFEM_SKU_LAYOUT_PIN_COUNT	16

/*******************************************************************************
 *			    D A T A   T Y P E S
 ******************************************************************************/
enum connfem_type {
	CONNFEM_TYPE_NONE = 0,
	CONNFEM_TYPE_EPAELNA = 1,	/* !defined(CONNFEM_API_VERSION) */
	CONNFEM_TYPE_SKU = 2,		/* CONNFEM_API_VERSION == 2 */
	CONNFEM_TYPE_NUM
};

/*
 * ConnFem v1 - CONNFEM_TYPE_EPAELNA
 */
enum connfem_subsys {
	CONNFEM_SUBSYS_NONE = 0,
	CONNFEM_SUBSYS_WIFI = 1,
	CONNFEM_SUBSYS_BT = 2,
	CONNFEM_SUBSYS_NUM
};

enum connfem_rf_port {
	CONNFEM_PORT_BT = 0,
	CONNFEM_PORT_WFG = CONNFEM_PORT_BT,
	CONNFEM_PORT_WFA = 1,
	CONNFEM_PORT_NUM
};

struct connfem_part {
	unsigned char vid;
	unsigned char pid;
};

struct connfem_epaelna_pin {
	unsigned char antsel;
	unsigned char fem;
	bool polarity;
};

struct connfem_epaelna_fem_info {
	unsigned int id;
	struct connfem_part part[CONNFEM_PORT_NUM];
	char part_name[CONNFEM_PORT_NUM][CONNFEM_PART_NAME_SIZE];
};

struct connfem_epaelna_pin_info {
	unsigned int count;
	struct connfem_epaelna_pin pin[CONNFEM_EPAELNA_PIN_COUNT];
};

struct connfem_epaelna_laa_pin {
	unsigned short gpio;
	unsigned char wf_mode;
	unsigned char md_mode;
};

struct connfem_epaelna_laa_pin_info {
	unsigned int chip_id;
	unsigned int count;
	struct connfem_epaelna_laa_pin pin[CONNFEM_EPAELNA_LAA_PIN_COUNT];
};

struct connfem_epaelna_flags_common {
	unsigned char rxmode;
	unsigned char fe_ant_cnt;
	unsigned char fe_main_bt_share_lp2g;
	unsigned char fe_conn_spdt;
	unsigned char fe_reserved;
	unsigned char bd_type;
	unsigned char fe_conn_dpdt_sp3t;
	unsigned char fe_bt_wf_usage;
	unsigned char fe_conn_spdt_2;
};

struct connfem_epaelna_flags_wifi {
	bool open_loop;
	bool laa;
	unsigned char epa_option;
	bool only_2g;
	unsigned char nv_attr;
};

struct connfem_epaelna_flags_bt {
	bool bypass;
	bool epa_elna;
	bool epa;
	bool elna;
	unsigned char efem_mode; /* 3:epa_elna, 2:epa, 1:elna, 0:bypass */
	unsigned char rx_mode;
};

struct connfem_epaelna_flag_tbl_entry {
	const char *name;
	unsigned char *addr;
};

struct connfem_epaelna_flag_pair {
	char name[CONNFEM_FLAG_NAME_SIZE]; /* zero-terminated */
	unsigned char value;
};

struct connfem_epaelna_subsys_cb {
	void*(*flags_get)(void);
	struct connfem_epaelna_flag_tbl_entry*(*flags_tbl_get)(void);
	unsigned int (*flags_cnt)(void);
};

/*
 * ConnFem v2 - CONNFEM_TYPE_SKU
 */
/* FEM Basic Info */
struct connfem_sku_fem_info {
	unsigned short vid;
	unsigned short pid;
	unsigned int flag;
	char name[CONNFEM_PART_NAME_SIZE];
};

/* FEM Control PIN */
struct connfem_sku_fem_ctrlpin {
	unsigned int count;
	unsigned char id[CONNFEM_FEM_PIN_COUNT];
};

/* Logic Truth Table */
struct connfem_sku_fem_logic {
	unsigned int op;
	unsigned int binary;
};

struct connfem_sku_fem_truth_table {
	unsigned int logic_count;
	struct connfem_sku_fem_logic logic[CONNFEM_FEM_LOGIC_COUNT];
};

/* Truth Table Usage */
struct connfem_sku_fem_logic_cat {
	unsigned int id;
	unsigned int op_count;
	unsigned int op[CONNFEM_FEM_LOGIC_COUNT];
};

struct connfem_sku_fem_truth_table_usage {
	unsigned int cat_count;
	struct connfem_sku_fem_logic_cat cat[CONNFEM_FEM_LOGIC_CAT_COUNT];
};

/* Keep all sku information */
struct connfem_sku_fem {
	unsigned int magic_num;	/* CONNFEM_FEM_MAGIC_NUMBER */
	struct connfem_sku_fem_info info;
	struct connfem_sku_fem_ctrlpin ctrl_pin;
	struct connfem_sku_fem_truth_table tt;
	struct connfem_sku_fem_truth_table_usage tt_usage_wf;
	struct connfem_sku_fem_truth_table_usage tt_usage_bt; /* Reserved */
};

struct connfem_sku_pinmap {
	unsigned char pin1;	/* Antsel */
	unsigned char pin2;	/* FEM Control PIN, or MD BPI PIN for LAA 4x4 */
	unsigned char flag;	/* Polarity, or reserved for PIN mapping attribute */
};

struct connfem_sku_layout {
	unsigned int fem_idx;

	unsigned char bandpath[CONNFEM_SUBSYS_NUM];

	unsigned int pin_count;
	struct connfem_sku_pinmap pinmap[CONNFEM_SKU_LAYOUT_PIN_COUNT];
};

struct connfem_sku_spdt {
	unsigned int magic_num;	/* CONNFEM_SPDT_MAGIC_NUMBER */
	unsigned int pin_count;
	struct connfem_sku_pinmap pinmap[CONNFEM_SKU_LAYOUT_PIN_COUNT];
};

struct connfem_sku {
	unsigned int fem_count;
	struct connfem_sku_fem fem[CONNFEM_SKU_FEM_COUNT];

	unsigned int layout_flag;

	unsigned int layout_count;
	struct connfem_sku_layout layout[CONNFEM_SKU_LAYOUT_COUNT];

	struct connfem_sku_spdt spdt;
};

/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/


/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
extern bool connfem_is_available(enum connfem_type fem_type);

extern int connfem_epaelna_get_fem_info(
			struct connfem_epaelna_fem_info *fem_info);

extern int connfem_epaelna_get_bt_fem_info(
			struct connfem_epaelna_fem_info *fem_info);

extern int connfem_epaelna_get_pin_info(
			struct connfem_epaelna_pin_info *pin_info);

extern int connfem_epaelna_laa_get_pin_info(
			struct connfem_epaelna_laa_pin_info *laa_pin_info);

extern int connfem_epaelna_get_flags(enum connfem_subsys subsys, void *flags);

extern int connfem_sku_data(const struct connfem_sku **sku);

extern int connfem_sku_flag_u8(enum connfem_subsys subsys,
			const char *name,
			unsigned char *value);

#endif /* __CONNFEM_API_H__ */
