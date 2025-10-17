// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/firmware.h>
#include <linux/slab.h>
#include "connfem.h"

/*******************************************************************************
 *				M A C R O S
 ******************************************************************************/
#define CFM_CFG_FEMID_A_PID		0
#define CFM_CFG_FEMID_G_PID		1
#define CFM_CFG_FEMID_VID		2
#define CFM_CFG_FEMID_GET_G_VID(x)	(((x) >> 4) & 0x0F)
#define CFM_CFG_FEMID_GET_A_VID(x)	((x) & 0x0F)

/* Magic header types used in CFM configuration files */
#define CFM_CFG_HDR_CPY_SKU		"CFM:cpy-sku"
#define CFM_CFG_HDR_LEGACY		"CFM:legacy"
#define CFM_CFG_HDR_HW_NAME		"CFM:hw-name"
#define CFM_CFG_HDR_HWID		"CFM:hwid"
#define CFM_CFG_HDR_STR_SIZE		16

/*******************************************************************************
 *			    D A T A   T Y P E S
 ******************************************************************************/
enum CONNFEM_CFG {
	CONNFEM_CFG_ID			= 1,
	CONNFEM_CFG_FEMID		= 2,
	CONNFEM_CFG_PIN_INFO		= 3,
	CONNFEM_CFG_FLAGS_BT		= 4,
	CONNFEM_CFG_FLAGS_CM		= 5,
	CONNFEM_CFG_FLAGS_WIFI		= 6,
	CONNFEM_CFG_G_PART_NAME		= 7,
	CONNFEM_CFG_A_PART_NAME		= 8,
	CONNFEM_CFG_BT_FEMID		= 9,
	CONNFEM_CFG_BT_G_PART_NAME	= 10,
	CONNFEM_CFG_BT_A_PART_NAME	= 11,
	CONNFEM_CFG_HW_NAME		= 12,
	CONNFEM_CFG_NUM
};

struct cfm_cfg_tlv {
	unsigned short tag;
	unsigned short length;
	unsigned char data[];
};

/* 32-bytes is reserved for a header */
struct cfm_cfg_header {
	char type[CFM_CFG_HDR_STR_SIZE];
	unsigned int pl_id;
	unsigned int reserved1;
	unsigned int reserved2;
	unsigned int reserved3;
};

enum cfm_header_type {
	CONNFEM_HEADER_NONE = 0,
	CONNFEM_HEADER_COPY_SKU = 1,	/* CFM_CFG_HDR_CPY_SKU */
	CONNFEM_HEADER_LEGACY = 2,	/* CFM_CFG_HDR_LEGACY */
	CONNFEM_HEADER_HW_NAME = 3,	/* CFM_CFG_HDR_HW_NAME */
	CONNFEM_HEADER_HWID = 4,	/* CFM_CFG_HDR_HWID */
	CONNFEM_HEADER_NUM
};

struct cfm_header_mapping {
	const char *str;
	enum cfm_header_type type;
};

/*******************************************************************************
 *		    F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************/
static int cfm_cfg_parse_id(void *cfm, struct cfm_cfg_tlv *tlv);
static int cfm_cfg_parse_femid(struct connfem_epaelna_fem_info *fem_info,
				struct cfm_cfg_tlv *tlv);
static int cfm_cfg_parse_pin_info(struct cfm_epaelna_config *cfg,
					struct cfm_cfg_tlv *tlv);
static int cfm_cfg_parse_flags(enum connfem_subsys subsys,
				void *ctx,
				struct cfm_cfg_tlv *tlv);
static int cfm_cfg_parse(enum cfm_header_type hdr_type,
			struct connfem_epa_context *ctx,
			const struct firmware *data);
static int cfm_cfg_parse_flags_helper(int count,
				struct connfem_epaelna_subsys_cb *subsys_cb,
				struct cfm_cfg_tlv *tlv,
				struct cfm_container **result_out);
static int cfm_cfg_parse_part_name(enum connfem_rf_port port,
				struct connfem_epaelna_fem_info *fem_info,
				struct cfm_cfg_tlv *tlv);
static int cfm_cfg_parse_sku(struct connfem_sku_context *ctx,
				const struct firmware *data);
static int cfm_cfg_sku_context_copy(void *ctx,
				const struct firmware *data);
static int cfm_cfg_sku_data_verify(struct connfem_sku_context *cfm);
static int cfm_cfg_parse_hw_name(const struct firmware *data);
static enum cfm_header_type cfm_cfg_header_type_get(const char *hd_str);
static int cfm_cfg_copy_sku_hdl(struct connfem_sku_context *cfm,
				const struct firmware *data,
				const struct cfm_cfg_header *header);
static int cfm_cfg_legacy_hdl(struct connfem_epa_context *cfm,
				enum cfm_header_type hdr_type,
				const struct firmware *data,
				const struct cfm_cfg_header *header);
static int cfm_cfg_parse_hwid(const struct firmware *data);
static int cfm_cfg_parse_hw_name_tlv(struct cfm_cfg_tlv *tlv);

/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/

/*******************************************************************************
 *			   P R I V A T E   D A T A
 ******************************************************************************/
static const struct cfm_header_mapping header_table[] = {
	{CFM_CFG_HDR_CPY_SKU,	CONNFEM_HEADER_COPY_SKU},
	{CFM_CFG_HDR_LEGACY,	CONNFEM_HEADER_LEGACY},
	{CFM_CFG_HDR_HW_NAME,	CONNFEM_HEADER_HW_NAME},
	{CFM_CFG_HDR_HWID,	CONNFEM_HEADER_HWID},
};

/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
void cfm_cfg_process(char *filename)
{
	int ret = 0;
	const struct firmware *data = NULL;
	struct connfem_context_ops *ops = NULL;
	const struct cfm_cfg_header *header = NULL;
	enum cfm_header_type hdr_type = CONNFEM_HEADER_NONE;

	if (!filename)
		return;

	ret = request_firmware_direct(&data, filename, NULL);
	if (ret != 0) {
		pr_info("request_firmware_direct() fail (%s:%d)",
				filename,
				ret);
		return;
	}

	if (connfem_ctx) {
		/* If cfm_cfg_process() is not called by connfem_mod_init()
		 * only in the future, we may need to consider context
		 * concurrency when Wifi and BT driver is calling ConnFem API
		 */
		ops = (struct connfem_context_ops *)connfem_ctx;
		ops->free(connfem_ctx);
		connfem_ctx = NULL;
	}

	/* Double check whether acquired data does not exist */
	if (!data) {
		pr_info("firmware load failed (%s:%d)", filename, ret);
		return;
	} else if (!data->data || data->size == 0) {
		pr_info("cfg without data (%s:%d)", filename, ret);
		release_firmware(data);
		return;
	}
	pr_info("Get filename(%s) size(%zu)", filename, data->size);

	/* Check if magic str is caught in the beginning of a file
	 * and get type of the binary file.
	*/
	if (data->size >= sizeof(struct cfm_cfg_header)) {
		header = (const struct cfm_cfg_header *)data->data;
		hdr_type = cfm_cfg_header_type_get(header->type);
	}

	if (hdr_type == CONNFEM_HEADER_NONE) {
		header = NULL;
	}
	pr_info("%s: header: %d", __func__, hdr_type);

	switch (hdr_type) {
	case CONNFEM_HEADER_COPY_SKU:
		connfem_ctx = cfm_ctx[CONNFEM_TYPE_SKU];
		cfm_cfg_copy_sku_hdl((struct connfem_sku_context *)connfem_ctx,
					data, header);
		break;
	case CONNFEM_HEADER_HW_NAME:
		cfm_cfg_parse_hw_name(data);
		break;

	case CONNFEM_HEADER_HWID:
		cfm_cfg_parse_hwid(data);
		break;
	case CONNFEM_HEADER_LEGACY:
	case CONNFEM_HEADER_NONE:
	default:
		connfem_ctx = cfm_ctx[CONNFEM_TYPE_EPAELNA];
		cfm_cfg_legacy_hdl((struct connfem_epa_context *)connfem_ctx,
				hdr_type, data, header);
		break;
	}

	release_firmware(data);
}

static int cfm_cfg_legacy_hdl(struct connfem_epa_context *cfm,
				enum cfm_header_type hdr_type,
				const struct firmware *data,
				const struct cfm_cfg_header *header)
{
	struct connfem_context_ops *ops = NULL;

	if (!cfm || !data) {
		pr_info("%s: input is invalid", __func__);
		connfem_ctx = NULL;
		return -EINVAL;
	}

	ops = (struct connfem_context_ops *)cfm;
	if (header) {
		ops->id = header->pl_id;
	}
	pr_info("%s: ops->id: 0x%08x", __func__, ops->id);

	if (cfm_cfg_parse(hdr_type, cfm, data) == 0) {
		ops->src = CFM_SRC_CFG_FILE;
	} else {
		goto cfm_cfg_legacy_hdl_err;
	}

	return 0;

cfm_cfg_legacy_hdl_err:
	ops->free(cfm);
	connfem_ctx = NULL;

	return -EINVAL;
}

static int cfm_cfg_copy_sku_hdl(struct connfem_sku_context *cfm,
				const struct firmware *data,
				const struct cfm_cfg_header *header)
{
	int err = 0;
	struct connfem_context_ops *ops = NULL;

	if (!cfm || !data) {
		pr_info("%s: input is invalid", __func__);
		connfem_ctx = NULL;
		return -EINVAL;
	}

	ops = (struct connfem_context_ops *)cfm;
	err = cfm_cfg_sku_context_copy(cfm, data);
	if (err < 0) {
		pr_info("%s: sku copy fail", __func__);
		goto cfm_cfg_copy_sku_hdl_err;
	}

	err = cfm_cfg_sku_data_verify(cfm);
	if (err < 0) {
		goto cfm_cfg_copy_sku_hdl_err;
	}

	err = cfm_cfg_parse_sku(cfm, data);
	if (err < 0) {
		pr_info("%s: cfg sku tlv data failed", __func__);
		goto cfm_cfg_copy_sku_hdl_err;
	}

	if (header) {
		ops->id = header->pl_id;
	}
	pr_info("%s: ops->id: 0x%08x", __func__, ops->id);
	ops->src = CFM_SRC_CFG_FILE;
	cfm->available = true;

	cfm_sku_data_dump(&cfm->sku);
	return 0;

cfm_cfg_copy_sku_hdl_err:
	ops->free(cfm);
	connfem_ctx = NULL;

	return -EINVAL;
}

static enum cfm_header_type cfm_cfg_header_type_get(const char *hd_str)
{
	int i;

	if (!hd_str) {
		pr_info("%s: input is null", __func__);
		return CONNFEM_HEADER_NONE;
	}

	for (i = 0; i < sizeof(header_table) / sizeof(header_table[0]); i++) {
		if (strncmp(hd_str,
			header_table[i].str,
			CFM_CFG_HDR_STR_SIZE) == 0) {
			pr_info("%s: type '%s' matched", __func__, hd_str);
			return header_table[i].type;
		}
	}

	return CONNFEM_HEADER_NONE;
}

static int cfm_cfg_parse_hw_name(const struct firmware *data)
{
	unsigned int offset = sizeof(struct cfm_cfg_header);

	if (!data || !data->data) {
		pr_info("%s, data, or data->data is NULL", __func__);
		return -EINVAL;
	}

	if (data->size < offset) {
		pr_info("%s, data->size(%zu) < offset (%d)",
			__func__, data->size, offset);
		return -EINVAL;
	}

	return cfm_param_hw_name_set(data->data + offset, data->size - offset);
}

static int cfm_cfg_parse_hwid(const struct firmware *data)
{
	unsigned int offset = sizeof(struct cfm_cfg_header);
	size_t cpy_sz = 0;
	size_t actual_data_sz = 0;
	unsigned int *hwid = cfm_param_hwid();

	if (!data || !data->data) {
		pr_info("%s, data, or data->data is NULL", __func__);
		return -EINVAL;
	}

	if (data->size < offset) {
		pr_info("%s, data->size(%zu) < offset (%d)",
			__func__, data->size, offset);
		return -EINVAL;
	}
	actual_data_sz = data->size - offset;

	if (actual_data_sz <= sizeof(*hwid)) {
		cpy_sz = actual_data_sz;
	} else {
		cpy_sz = sizeof(*hwid);
	}

	if (cpy_sz == 0) {
		return 0;
	}

	/* It is necessary to initial hwid to be 0 if copy size > 0
	 * or some residual values make our copy results not as expected.
	*/
	memset(hwid, 0, sizeof(*hwid));
	memcpy(hwid, data->data + offset, cpy_sz);
	pr_info("%s, hwid: %u(0x%08x)", __func__, *hwid, *hwid);

	return 0;
}

static int cfm_cfg_sku_context_copy(void *ctx, const struct firmware *data)
{
	size_t size = 0;
	unsigned int offset = sizeof(struct cfm_cfg_header);
	size_t expected_size = offsetof(struct connfem_sku_context, flags) -
				sizeof(struct connfem_context_ops);
	const unsigned char *src = NULL;
	const unsigned char *dest = NULL;

	if (!ctx || !data || !data->data) {
		pr_info("%s, ctx, data, or data->data is NULL", __func__);
		return -EINVAL;
	}

	/* Real size of the data after the magic struct */
	size = data->size - offset;

	/* binary file layout maybe:
	 * (1) |  header   |  or (2) |  header  |
	 *     | SKU data  |         | SKU data |
	 *     | flag data |
	 * We have no other information about the binary file layout.
	 * If the data size is larger than the expected size, we simply
	 * copy the data to the context. We will then verify its
	 * correctness later.
	*/
	if (size >= expected_size) {
		src = data->data + offset;
		dest = (char*)ctx + sizeof(struct connfem_context_ops);
		memcpy((void *)dest, (void *)src, expected_size);
		pr_info("%s, copy mem size (%zu) success (total sz: %zu)",
			__func__, expected_size, size);
		return 0;
	} else {
		pr_info("%s, size < expected_size (%zu<%zu)",
			__func__, size, expected_size);
		return -EINVAL;
	}
}

static int cfm_cfg_parse_hw_name_tlv(struct cfm_cfg_tlv *tlv)
{
	if (tlv->length == 0) {
		pr_info("invalid hw_name length (%d)", tlv->length);
		return -EINVAL;
	}

	return cfm_param_hw_name_set(tlv->data, tlv->length);
}

static int cfm_cfg_parse_sku(struct connfem_sku_context *ctx,
				const struct firmware *data)
{
	size_t copied_size = offsetof(struct connfem_sku_context, flags) -
				sizeof(struct connfem_context_ops);
	unsigned int offset = sizeof(struct cfm_cfg_header) + copied_size;
	struct cfm_cfg_tlv *tlv = NULL;
	int ret = 0;

	if (!ctx || !data || !data->data) {
		pr_info("%s,ctx, data, or data->data is NULL", __func__);
		return -EINVAL;
	}

	while (offset < data->size) {
		tlv = (struct cfm_cfg_tlv *) (data->data + offset);
		/* Because tlv->length may give tlv->data wrong size,
		 * tlv->data may be larger than tlv->length specified.
		 * However, the string have already been written. Under
		 * this circumstance, we need to intialize hw_name as null
		 * string.
		*/
		if (offset + sizeof(struct cfm_cfg_tlv) + tlv->length > data->size) {
			cfm_param_hw_name_set(NULL, 0);
			pr_info("%s,tlv->length > data->size (%zu>%zu)",
				__func__,
				(offset + sizeof(struct cfm_cfg_tlv) + tlv->length),
				data->size);
			return -EINVAL;
		}
		pr_info("%s, tag:%hu,len:%hu,offset:%u",
				__func__,
				tlv->tag,
				tlv->length,
				offset);

		switch (tlv->tag) {
		case CONNFEM_CFG_FLAGS_BT:
			ret = cfm_cfg_parse_flags(CONNFEM_SUBSYS_BT,
						(void*)ctx, tlv);
			break;
		case CONNFEM_CFG_FLAGS_CM:
			ret = cfm_cfg_parse_flags(CONNFEM_SUBSYS_NONE,
						(void*)ctx, tlv);
			break;
		case CONNFEM_CFG_FLAGS_WIFI:
			ret = cfm_cfg_parse_flags(CONNFEM_SUBSYS_WIFI,
						(void*)ctx, tlv);
			break;
		case CONNFEM_CFG_HW_NAME:
			ret = cfm_cfg_parse_hw_name_tlv(tlv);
			break;
		default:
			pr_info("%s, unknown tag = %d",
				__func__,
				tlv->tag);
			break;
		}

		if (ret != 0) {
			cfm_param_hw_name_set(NULL, 0);
			return ret;
		}

		offset += sizeof(struct cfm_cfg_tlv) + tlv->length;
	}

	return ret;
}

static int cfm_cfg_sku_data_verify(struct connfem_sku_context *cfm)
{
	int i;
	struct connfem_sku *sku = NULL;
	struct connfem_sku_fem_ctrlpin *ctrl_pin;
	struct connfem_sku_fem_truth_table *tt;
	struct connfem_sku_fem_truth_table_usage *tt_usg_wf;
	struct connfem_sku_fem_truth_table_usage *tt_usg_bt;

	if (!cfm) {
		pr_info("%s, input is NULL", __func__);
		return -EINVAL;
	}
	sku = &cfm->sku;

	if (sku->fem_count > CONNFEM_SKU_FEM_COUNT) {
		goto sku_data_verify_err;
	}

	for (i = 0; i < sku->fem_count; i++) {
		ctrl_pin = &sku->fem[i].ctrl_pin;
		tt = &sku->fem[i].tt;
		tt_usg_wf = &sku->fem[i].tt_usage_wf;
		tt_usg_bt = &sku->fem[i].tt_usage_bt;

		if (sku->fem[i].magic_num != CONNFEM_FEM_MAGIC_NUMBER ||
			ctrl_pin->count > CONNFEM_FEM_PIN_COUNT ||
			tt->logic_count > CONNFEM_FEM_LOGIC_COUNT ||
			tt_usg_wf->cat_count > CONNFEM_FEM_LOGIC_CAT_COUNT ||
			tt_usg_bt->cat_count > CONNFEM_FEM_LOGIC_CAT_COUNT) {
			goto sku_data_verify_err;
		}
	}

	if (sku->layout_count > CONNFEM_SKU_LAYOUT_COUNT) {
		goto sku_data_verify_err;
	}

	for (i = 0; i < sku->layout_count; i++) {
		if (sku->layout[i].pin_count > CONNFEM_SKU_LAYOUT_PIN_COUNT) {
			goto sku_data_verify_err;
		}
	}

	if (sku->spdt.magic_num != CONNFEM_SPDT_MAGIC_NUMBER) {
		goto sku_data_verify_err;
	}

	pr_info("%s: verification pass", __func__);
	return 0;

sku_data_verify_err:
	pr_info("%s: verification fails, reset sku data", __func__);
	cfm_dt_sku_data_reset(cfm);
	return -EINVAL;
}

/**
 * cfm_cfg_parse_id
 *	Parse ID from config file
 *
 * Parameters
 *	ctx : Pointer to connfem context
 *	tlv	: Pointer to tlv data containing ID
 *
 * Return value
 *	0	: Success
 *	-EINVAL	: Error
 *
 */
static int cfm_cfg_parse_id(void *cfm, struct cfm_cfg_tlv *tlv)
{
	struct connfem_context_ops *ops = (struct connfem_context_ops *)cfm;

	if (tlv->length != sizeof(ops->id)) {
		pr_info(" id length (%d) should be (%zu)",
				tlv->length,
				sizeof(ops->id));
		return -EINVAL;
	}

	memcpy(&ops->id, tlv->data, tlv->length);

	pr_info("ctx->id: 0x%08x", ops->id);

	return 0;
}

/**
 * cfm_cfg_parse_femid
 *	Parse FEM ID from config file
 *
 * Parameters
 *	fem_id	: Pointer to fem_info or bt_fem_info
 *	tlv		: Pointer to tlv data containing FEM ID
 *
 * Return value
 *	0	: Success
 *	-EINVAL	: Error
 *
 */
static int cfm_cfg_parse_femid(struct connfem_epaelna_fem_info *fem_info,
				struct cfm_cfg_tlv *tlv)
{
	if (tlv->length != sizeof(fem_info->id)) {
		pr_info("fem id length (%d) should be (%zu)",
				tlv->length,
				sizeof(fem_info->id));
		return -EINVAL;
	}

	memcpy(&fem_info->id, tlv->data, tlv->length);
	fem_info->part[CONNFEM_PORT_WFG].vid =
			CFM_CFG_FEMID_GET_G_VID(tlv->data[CFM_CFG_FEMID_VID]);
	fem_info->part[CONNFEM_PORT_WFA].vid =
			CFM_CFG_FEMID_GET_A_VID(tlv->data[CFM_CFG_FEMID_VID]);
	fem_info->part[CONNFEM_PORT_WFG].pid =
			tlv->data[CFM_CFG_FEMID_G_PID];
	fem_info->part[CONNFEM_PORT_WFA].pid =
			tlv->data[CFM_CFG_FEMID_A_PID];

	pr_info("fem_info->id: 0x%08x", fem_info->id);

	return 0;
}

/**
 * cfm_cfg_parse_pin_info
 *	Parse pin info from config file
 *
 * Parameters
 *	cfg : Pointer to epaelna config
 *	tlv	: Pointer to tlv data containing pin info
 *
 * Return value
 *	0	: Success
 *	-EINVAL	: Error
 *
 */
static int cfm_cfg_parse_pin_info(struct cfm_epaelna_config *cfg,
				struct cfm_cfg_tlv *tlv)
{
	if (tlv->length == 0 ||
		tlv->length > sizeof(cfg->pin_cfg.pin_info.pin)) {
		pr_info("invalid pin info length (%d)", tlv->length);
		return -EINVAL;
	}

	if ((tlv->length % sizeof(struct connfem_epaelna_pin)) != 0) {
		pr_info("pin info length needs to be multiple of %zu, currently %d",
			sizeof(struct connfem_epaelna_pin),
			tlv->length);
		return -EINVAL;
	}

	memset(&cfg->pin_cfg.pin_info, 0, sizeof(cfg->pin_cfg.pin_info));
	cfg->pin_cfg.pin_info.count =
			tlv->length / sizeof(struct connfem_epaelna_pin);
	memcpy(&cfg->pin_cfg.pin_info.pin,
			tlv->data,
			tlv->length);

	return 0;
}

/**
 * cfm_cfg_parse_flags
 *	Parse common/wifi/BT flags from config file
 *
 * Parameters
 *	subsys	: connfem_subsys
 *	ctx		: Pointer to connfem context
 *	tlv		: Pointer to tlv data containing common/wifi/BT flags
 *
 * Return value
 *	0	: Success
 *	-EINVAL	: Error
 *
 */
static int cfm_cfg_parse_flags(enum connfem_subsys subsys,
				void *ctx, struct cfm_cfg_tlv *tlv)
{
	int err = 0;
	struct connfem_epaelna_subsys_cb *subsys_cb = NULL;
	struct connfem_context_ops *ops = NULL;
	struct cfm_epaelna_flags_config *flg_cfg;

	if (tlv->length == 0 ) {
		pr_info("invalid flag pair length (%d)", tlv->length);
		return -EINVAL;
	}

	if ((tlv->length % sizeof(struct connfem_epaelna_flag_pair)) != 0) {
		pr_info("flag pair length needs to be multiple of %zu, currently %d",
			sizeof(struct connfem_epaelna_flag_pair),
			tlv->length);
		return -EINVAL;
	}

	subsys_cb = cfm_epaelna_flags_subsys_cb_get(subsys);
	if (subsys_cb == NULL)
		return -EINVAL;

	ops = (struct connfem_context_ops *)ctx;
	err = ops->get_flags_config(connfem_ctx, &flg_cfg);
	if (err < 0) {
		pr_info("%s, cannot get flags config", __func__);
		return -EINVAL;
	}

	err = cfm_cfg_parse_flags_helper(tlv->length/sizeof(struct connfem_epaelna_flag_pair),
					subsys_cb,
					tlv,
					&flg_cfg[subsys].pairs);
	if (err < 0)
		return -EINVAL;

	cfm_epaelna_flags_pairs_dump(subsys, flg_cfg[subsys].pairs);

	flg_cfg[subsys].obj = subsys_cb->flags_get();
	if (!flg_cfg[subsys].obj) {
		pr_info("%s flags structure is NULL",
			cfm_subsys_name[subsys]);
		return -EINVAL;
	}

	return 0;
}

/**
 * cfm_cfg_parse_part_name
 *	Parse A/G band part name from config file
 *
 * Parameters
 *	port	: connfem_rf_port
 *	fem_id	: Pointer to fem_info or bt_fem_info
 *	tlv		: Pointer to tlv data containing part name
 *
 * Return value
 *	0	: Success
 *	-EINVAL	: Error
 *
 */
static int cfm_cfg_parse_part_name(enum connfem_rf_port port,
				struct connfem_epaelna_fem_info *fem_info,
				struct cfm_cfg_tlv *tlv)
{
	if (port >= CONNFEM_PORT_NUM) {
		pr_info("invalid port (%d)", port);
		return -EINVAL;
	}

	if (tlv->length == 0 ||
		tlv->length > sizeof(fem_info->part_name[port])) {
		pr_info("invalid part name length (%d)", tlv->length);
		return -EINVAL;
	}

	memset(fem_info->part_name[port], 0, sizeof(fem_info->part_name[port]));
	memcpy(fem_info->part_name[port], tlv->data, tlv->length);
	fem_info->part_name[port][sizeof(fem_info->part_name[port])-1] = '\0';

	return 0;
}

/**
 * cfm_cfg_parse
 *	Parse config file
 *
 * Parameters
 *	ctx	: Pointer to connfem context
 *	data: Pointer to data containing config file
 *
 * Return value
 *	0	: Success
 *	-EINVAL	: Error
 *
 */
static int cfm_cfg_parse(enum cfm_header_type hdr_type,
			struct connfem_epa_context *ctx,
			const struct firmware *data)
{
	unsigned int offset = 0;
	struct cfm_cfg_tlv *tlv = NULL;
	int ret = 0;

	if (!ctx || !data || !data->data) {
		pr_info("%s,ctx, data, or data->data is NULL", __func__);
		return -EINVAL;
	}

	if (hdr_type == CONNFEM_HEADER_LEGACY) {
		offset = sizeof(struct cfm_cfg_header);
	}

	while (offset < data->size) {
		tlv = (struct cfm_cfg_tlv *) (data->data + offset);
		if (offset + sizeof(struct cfm_cfg_tlv) + tlv->length > data->size) {
			pr_info("%s,tlv->length > data->size (%zu>%zu)",
				__func__,
				(offset + sizeof(struct cfm_cfg_tlv) + tlv->length),
				data->size);
			return -EINVAL;
		}
		pr_info("%s, tag:%hu,len:%hu,offset:%u",
				__func__,
				tlv->tag,
				tlv->length,
				offset);

		switch (tlv->tag) {
		case CONNFEM_CFG_ID:
			ret = cfm_cfg_parse_id(ctx, tlv);
			break;
		case CONNFEM_CFG_FEMID:
			ret = cfm_cfg_parse_femid(&ctx->epaelna.fem_info, tlv);
			break;
		case CONNFEM_CFG_PIN_INFO:
			ret = cfm_cfg_parse_pin_info(&ctx->epaelna, tlv);
			break;
		case CONNFEM_CFG_FLAGS_BT:
			ret = cfm_cfg_parse_flags(CONNFEM_SUBSYS_BT,
						(void *)ctx, tlv);
			break;
		case CONNFEM_CFG_FLAGS_CM:
			ret = cfm_cfg_parse_flags(CONNFEM_SUBSYS_NONE,
						(void *)ctx, tlv);
			break;
		case CONNFEM_CFG_FLAGS_WIFI:
			ret = cfm_cfg_parse_flags(CONNFEM_SUBSYS_WIFI,
						(void *)ctx, tlv);
			break;
		case CONNFEM_CFG_G_PART_NAME:
			ret = cfm_cfg_parse_part_name(CONNFEM_PORT_WFG,
					&ctx->epaelna.fem_info, tlv);
			break;
		case CONNFEM_CFG_A_PART_NAME:
			ret = cfm_cfg_parse_part_name(CONNFEM_PORT_WFA,
					&ctx->epaelna.fem_info, tlv);
			break;
		case CONNFEM_CFG_BT_FEMID:
			ret = cfm_cfg_parse_femid(&ctx->epaelna.bt_fem_info,
					tlv);
			break;
		case CONNFEM_CFG_BT_G_PART_NAME:
			ret = cfm_cfg_parse_part_name(CONNFEM_PORT_WFG,
					&ctx->epaelna.bt_fem_info, tlv);
			break;
		case CONNFEM_CFG_BT_A_PART_NAME:
			ret = cfm_cfg_parse_part_name(CONNFEM_PORT_WFA,
					&ctx->epaelna.bt_fem_info, tlv);
			break;
		default:
			pr_info("%s, unknown tag = %d",
					__func__,
					tlv->tag);
			break;
		}

		if (ret != 0)
			return ret;

		offset += sizeof(struct cfm_cfg_tlv) + tlv->length;
	}

	if (ret == 0 && ctx->epaelna.fem_info.id != 0) {
		ctx->epaelna.available = true;
		cfm_epaelna_config_dump(&ctx->epaelna);
	}

	return ret;
}

static int cfm_cfg_parse_flags_helper(int count,
				struct connfem_epaelna_subsys_cb *subsys_cb,
				struct cfm_cfg_tlv *tlv,
				struct cfm_container **result_out)
{
	unsigned int i = 0, len = 0, offset = 0, tbl_size = 0;
	struct connfem_epaelna_flag_tbl_entry *entry;
	struct cfm_container *result = NULL;
	struct connfem_epaelna_flag_pair *pair;
	struct connfem_epaelna_flag_pair pairTlv;

	tbl_size = subsys_cb->flags_cnt();

	if (count == 0 || count > tbl_size) {
		pr_info("invalid flags count(%d), max(%d)",
			count,
			tbl_size);
		return -EINVAL;
	}

	result = cfm_container_alloc(count, sizeof(struct connfem_epaelna_flag_pair));
	if (!result)
		return -ENOMEM;

	/* Retrieves flags name from tlv */
	while (offset < tlv->length) {
		if (offset + sizeof(struct connfem_epaelna_flag_pair) > tlv->length) {
			pr_info("%s,tlv->data > tlv->length (%zu>%hu)",
				__func__,
				offset + sizeof(struct connfem_epaelna_flag_pair),
				tlv->length);
			cfm_container_free(result);
			return -EINVAL;
		}
		memset(&pairTlv, 0, sizeof(struct connfem_epaelna_flag_pair));
		memcpy(&pairTlv, tlv->data+offset, sizeof(struct connfem_epaelna_flag_pair));
		pairTlv.name[CONNFEM_FLAG_NAME_SIZE-1] = '\0';

		/* Skip if not supported by subsys */
		entry = cfm_epaelna_flags_subsys_find(pairTlv.name,
			subsys_cb->flags_tbl_get());
		if (!entry) {
			offset += sizeof(struct connfem_epaelna_flag_pair);
			continue;
		}

		/* Double check if container is big enough to keep this flag */
		pair = cfm_container_entry(result, i);
		if (i >= result->cnt || !pair) {
			pr_info("Drop '%s' prop, too many flags %d > %d",
				pairTlv.name, i + 1, result->cnt);
			offset += sizeof(struct connfem_epaelna_flag_pair);
			continue;
		}

		/* Update subsys' flags value */
		*(entry->addr) = pairTlv.value;

		/* Update subsys' flags container entry */
		len = strlen(pairTlv.name) + 1;
		memcpy(pair->name, pairTlv.name, len);
		pair->value = *(entry->addr);
		i++;

		offset += sizeof(struct connfem_epaelna_flag_pair);
	}

	result->cnt = i;

	*result_out = result;

	return 0;
}
