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
	CONNFEM_CFG_NUM
};

struct cfm_cfg_tlv {
	unsigned short tag;
	unsigned short length;
	unsigned char data[0];
};

/*******************************************************************************
 *		    F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************/
static int cfm_cfg_parse_id(struct connfem_context *ctx,
					struct cfm_cfg_tlv *tlv);
static int cfm_cfg_parse_femid(struct connfem_epaelna_fem_info *fem_info,
				struct cfm_cfg_tlv *tlv);
static int cfm_cfg_parse_pin_info(struct cfm_epaelna_config *cfg,
					struct cfm_cfg_tlv *tlv);
static int cfm_cfg_parse_flags(enum connfem_subsys subsys,
					struct connfem_context *ctx,
					struct cfm_cfg_tlv *tlv);
static int cfm_cfg_parse(struct connfem_context *ctx,
					const struct firmware *data);
static int cfm_cfg_parse_flags_helper(int count,
				struct connfem_epaelna_subsys_cb *subsys_cb,
				struct cfm_cfg_tlv *tlv,
				struct cfm_container **result_out);
static int cfm_cfg_parse_part_name(enum connfem_rf_port port,
				struct connfem_epaelna_fem_info *fem_info,
				struct cfm_cfg_tlv *tlv);

/*******************************************************************************
 *			    P U B L I C   D A T A
 ******************************************************************************/

/*******************************************************************************
 *			   P R I V A T E   D A T A
 ******************************************************************************/

/*******************************************************************************
 *			      F U N C T I O N S
 ******************************************************************************/
void cfm_cfg_process(char *filename)
{
	int ret = 0;
	const struct firmware *data = NULL;

	if (!filename)
		return;

	ret = request_firmware(&data, filename, NULL);
	if (ret != 0) {
		pr_info("[WARN] request_firmware() fail (%s:%d)\n",
				filename,
				ret);
		return;
	}
	pr_info("get filename(%s) size(%zu)\n",
			filename,
			data->size);

	if (connfem_ctx) {
		/* If cfm_cfg_process() is not called by connfem_mod_init()
		 * only in the future, we may need to consider context
		 * concurrency when Wifi and BT driver is calling ConnFem API
		 */
		cfm_context_free(connfem_ctx);
		connfem_ctx = NULL;
	}

	connfem_ctx = kzalloc(
			sizeof(struct connfem_context),
			GFP_KERNEL);

	if (!connfem_ctx) {
		pr_info("[WARN] alloc mem for connfem_ctx fail, not parsing conf\n");
		release_firmware(data);
		return;
	}

	if (cfm_cfg_parse(connfem_ctx, data) == 0) {
		connfem_ctx->src = CFM_SRC_CFG_FILE;
	} else {
		pr_info("[WARN] conf parsing fail\n");
		if (connfem_ctx) {
			cfm_context_free(connfem_ctx);
			kfree(connfem_ctx);
			connfem_ctx = NULL;
		}
	}

	release_firmware(data);
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
static int cfm_cfg_parse_id(struct connfem_context *ctx,
				struct cfm_cfg_tlv *tlv)
{
	if (tlv->length != sizeof(ctx->id)) {
		pr_info("[WARN] id length (%d) should be (%zu)",
				tlv->length,
				sizeof(ctx->id));
		return -EINVAL;
	}

	memcpy(&ctx->id, tlv->data, tlv->length);

	pr_info("ctx->id: 0x%08x", ctx->id);

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
		pr_info("[WARN] fem id length (%d) should be (%zu)",
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
		pr_info("[WARN] invalid pin info length (%d)", tlv->length);
		return -EINVAL;
	}

	if ((tlv->length % sizeof(struct connfem_epaelna_pin)) != 0) {
		pr_info("[WARN] pin info length needs to be multiple of %zu, currently %d",
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
				struct connfem_context *ctx,
				struct cfm_cfg_tlv *tlv)
{
	int err = 0;
	struct connfem_epaelna_subsys_cb *subsys_cb = NULL;

	if (tlv->length == 0 ) {
		pr_info("[WARN] invalid flag pair length (%d)", tlv->length);
		return -EINVAL;
	}

	if ((tlv->length % sizeof(struct connfem_epaelna_flag_pair)) != 0) {
		pr_info("[WARN] flag pair length needs to be multiple of %zu, currently %d",
			sizeof(struct connfem_epaelna_flag_pair),
			tlv->length);
		return -EINVAL;
	}

	subsys_cb = cfm_epaelna_flags_subsys_cb_get(subsys);
	if (subsys_cb == NULL)
		return -EINVAL;

	err = cfm_cfg_parse_flags_helper(tlv->length/sizeof(struct connfem_epaelna_flag_pair),
			subsys_cb,
			tlv,
			&ctx->epaelna.flags_cfg[subsys].pairs);
	if (err < 0)
		return -EINVAL;

	cfm_epaelna_flags_pairs_dump(subsys, ctx->epaelna.flags_cfg[subsys].pairs);

	ctx->epaelna.flags_cfg[subsys].obj = subsys_cb->flags_get();
	if (!ctx->epaelna.flags_cfg[subsys].obj) {
		pr_info("[WARN] %s flags structure is NULL",
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
		pr_info("[WARN] invalid port (%d)", port);
		return -EINVAL;
	}

	if (tlv->length == 0 ||
		tlv->length > sizeof(fem_info->part_name[port])) {
		pr_info("[WARN] invalid part name length (%d)", tlv->length);
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
static int cfm_cfg_parse(struct connfem_context *ctx,
				const struct firmware *data)
{
	unsigned int offset = 0;
	struct cfm_cfg_tlv *tlv = NULL;
	int ret = 0;

	if (!ctx || !data || !data->data) {
		pr_info("%s,[WARN] ctx, data, or data->data is NULL", __func__);
		return -EINVAL;
	}

	while (offset < data->size) {
		tlv = (struct cfm_cfg_tlv *) (data->data + offset);
		if (offset + sizeof(struct cfm_cfg_tlv) + tlv->length > data->size) {
			pr_info("%s,[WARN] tlv->length > data->size (%zu>%zu)",
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
			ret = cfm_cfg_parse_flags(CONNFEM_SUBSYS_BT, ctx, tlv);
			break;
		case CONNFEM_CFG_FLAGS_CM:
			ret = cfm_cfg_parse_flags(CONNFEM_SUBSYS_NONE, ctx, tlv);
			break;
		case CONNFEM_CFG_FLAGS_WIFI:
			ret = cfm_cfg_parse_flags(CONNFEM_SUBSYS_WIFI, ctx, tlv);
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
		pr_info("[WARN] invalid flags count(%d), max(%d)",
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
			pr_info("%s,[WARN] tlv->data > tlv->length (%zu>%hu)",
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
			pr_info("[ERR] Drop '%s' prop, too many flags %d > %d",
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
