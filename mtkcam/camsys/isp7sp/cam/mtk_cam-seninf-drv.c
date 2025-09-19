// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/of_graph.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/component.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>
#include <uapi/linux/sched/types.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <soc/mediatek/mmdvfs_v3.h>

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include <linux/videodev2.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-async.h>
#include <media/v4l2-device.h>

#include "mtk_cam.h"
#include "mtk_cam-seninf-def.h"
#include "mtk_cam-seninf.h"
#include "mtk_cam-seninf-hw.h"
#include "mtk_cam-seninf-route.h"
#include "mtk_cam-seninf-tsrec.h"
#include "imgsensor-user.h"
#include "mtk_cam-seninf-ca.h"
#include "mtk_cam-seninf_control-7sp.h"

#define is_irq_ready 1

#define ESD_RESET_SUPPORT 1
#define V4L2_CID_MTK_SENINF_BASE	(V4L2_CID_USER_BASE | 0xf000)
#define V4L2_CID_MTK_TEST_STREAMON	(V4L2_CID_MTK_SENINF_BASE + 1)

#define sd_to_ctx(__sd) container_of(__sd, struct seninf_ctx, subdev)
#define notifier_to_ctx(__n) container_of(__n, struct seninf_ctx, notifier)
#define ctrl_hdl_to_ctx(__h) container_of(__h, struct seninf_ctx, ctrl_handler)
#define sizeof_u32(__struct_name__) (sizeof(__struct_name__) / sizeof(u32))
#define sizeof_u16(__struct_name__) (sizeof(__struct_name__) / sizeof(u16))

#define DEBUG_OPS_SHOW_LOG_SIZE 1024
static char debug_ops_show_log[DEBUG_OPS_SHOW_LOG_SIZE];

struct mtk_cam_seninf_ops *g_seninf_ops;

/* aov sensor use */
struct mtk_seninf_aov_param g_aov_param;
struct seninf_ctx *aov_ctx[AOV_SENINF_NUM];

#ifdef CSI_EFUSE_SET
#include <linux/nvmem-consumer.h>
#endif

#define USING_MAX_ISP_CLK
#define CSI_EFUSE_VERIFY_EN 1
#define CSI_EFUSE_VERIFY_GORDAN_TABLE_EN 0
#if CSI_EFUSE_VERIFY_GORDAN_TABLE_EN == 1
static int gordan_csi_efuse_table[32][6] = {
	{       0x0,        0x1,        0x2,        0x3,        0x4,        0x5},
	{ 0x8421085,  0x8421086,  0x8421087,  0x8421088,  0x8421089,  0x842108a},
	{0x1084210a, 0x1084210b, 0x1084210c, 0x1084210d, 0x1084210e, 0x1084210f},
	{0x18c6318f, 0x18c63181, 0x18c63182, 0x18c63183, 0x18c63184, 0x18c63185},
	{0x21084205, 0x21084206, 0x21084207, 0x21084208, 0x21084209, 0x2108420a},
	{0x294a528a, 0x294a528b, 0x294a528c, 0x294a528d, 0x294a528e, 0x294a528f},
	{0x318c630f, 0x318c6310, 0x318c6302, 0x318c6303, 0x318c6304, 0x318c6305},
	{0x39ce7385, 0x39ce7386, 0x39ce7387, 0x39ce7388, 0x39ce7389, 0x39ce738a},
	{0x4210840a, 0x4210840b, 0x4210840c, 0x4210840d, 0x4210840e, 0x4210840f},
	{0x4a52948f, 0x4a529490, 0x4a529491, 0x4a529483, 0x4a529484, 0x4a529485},
	{0x5294a505, 0x5294a506, 0x5294a507, 0x5294a508, 0x5294a509, 0x5294a50a},
	{0x5ad6b58a, 0x5ad6b58b, 0x5ad6b58c, 0x5ad6b58d, 0x5ad6b58e, 0x5ad6b58f},
	{0x6318c60f, 0x6318c610, 0x6318c611, 0x6318c612, 0x6318c604, 0x6318c605},
	{0x6b5ad685, 0x6b5ad686, 0x6b5ad687, 0x6b5ad688, 0x6b5ad689, 0x6b5ad68a},
	{0x739ce70a, 0x739ce70b, 0x739ce70c, 0x739ce70d, 0x739ce70e, 0x739ce70f},
	{0x7bdef78f, 0x7bdef790, 0x7bdef791, 0x7bdef792, 0x7bdef793, 0x7bdef785},
	{0x84210805, 0x84210806, 0x84210807, 0x84210808, 0x84210809, 0x8421080a},
	{0x8c63188a, 0x8c63188b, 0x8c63188c, 0x8c63188d, 0x8c63188e, 0x8c63188f},
	{0x94a5290f, 0x94a52910, 0x94a52911, 0x94a52912, 0x94a52913, 0x94a52914},
	{0x9ce73994, 0x9ce73986, 0x9ce73987, 0x9ce73988, 0x9ce73989, 0x9ce7398a},
	{0xa5294a0a, 0xa5294a0b, 0xa5294a0c, 0xa5294a0d, 0xa5294a0e, 0xa5294a0f},
	{0xad6b5a8f, 0xad6b5a90, 0xad6b5a91, 0xad6b5a92, 0xad6b5a93, 0xad6b5a94},
	{0xb5ad6b14, 0xb5ad6b15, 0xb5ad6b07, 0xb5ad6b08, 0xb5ad6b09, 0xb5ad6b0a},
	{0xbdef7b8a, 0xbdef7b8b, 0xbdef7b8c, 0xbdef7b8d, 0xbdef7b8e, 0xbdef7b8f},
	{0xc6318c0f, 0xc6318c10, 0xc6318c11, 0xc6318c12, 0xc6318c13, 0xc6318c14},
	{0xce739c94, 0xce739c95, 0xce739c96, 0xce739c88, 0xce739c89, 0xce739c8a},
	{0xd6b5ad0a, 0xd6b5ad0b, 0xd6b5ad0c, 0xd6b5ad0d, 0xd6b5ad0e, 0xd6b5ad0f},
	{0xdef7bd8f, 0xdef7bd90, 0xdef7bd91, 0xdef7bd92, 0xdef7bd93, 0xdef7bd94},
	{0xe739ce14, 0xe739ce15, 0xe739ce16, 0xe739ce17, 0xe739ce09, 0xe739ce0a},
	{0xef7bde8a, 0xef7bde8b, 0xef7bde8c, 0xef7bde8d, 0xef7bde8e, 0xef7bde8f},
	{0xf7bdef0f, 0xf7bdef10, 0xf7bdef11, 0xf7bdef12, 0xf7bdef13, 0xf7bdef14},
	{0xffffff94, 0xffffff95, 0xffffff96, 0xffffff97, 0xffffff98, 0xffffff8a}
};
#endif  /*CSI_EFUSE_VERIFY_GORDAN_TABLE_EN*/

static const char * const csi_phy_versions[] = {
	MTK_CSI_PHY_VERSIONS
};


static const char * const csi_port_names[] = {
	SENINF_CSI_PORT_NAMES
};

static const char * const clk_names[] = {
	SENINF_CLK_NAMES
};

static const char * const set_reg_names[] = {
	SET_REG_KEYS_NAMES
};

static const char * const eye_scan_names[] = {
	EYE_SCAN_KEYS_NAMES
};

static const char * const mux_range_name[] = {
	MUX_RANGE_NAMES
};

static const char * const muxvr_range_name[] = {
	MUXVR_RANGE_NAMES
};

static const char * const cammux_range_name[] = {
	CAMMUX_RANGE_NAMES
};

static const char * const clk_fmeter_names[] = {
	CLK_FMETER_NAMES
};

static const struct seninf_struct_map clk_fmeter_maps[] = {
	CLK_FMETER_MAPS
};

static const char * const cdphy_dvfs_step_name[] = {
	CDPHY_DVFS_STEP
};

static ssize_t status_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	return g_seninf_ops->_show_status(dev, attr, buf);
}

static DEVICE_ATTR_RO(status);

static ssize_t err_status_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	return g_seninf_ops->_show_err_status(dev, attr, buf);
}

static DEVICE_ATTR_RO(err_status);

static ssize_t debug_ops_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len, debug_ops_show_log);
	memset(debug_ops_show_log, '\0', sizeof(char)*DEBUG_OPS_SHOW_LOG_SIZE);
	return len;
}

enum REG_OPS_CMD {
	REG_OPS_CMD_ID,
	REG_OPS_CMD_CSI,
	REG_OPS_CMD_RG,
	REG_OPS_CMD_VAL,
	REG_OPS_CMD_PAD = REG_OPS_CMD_RG,
	REG_OPS_CMD_CAMTG = REG_OPS_CMD_VAL,
	REG_OPS_CMD_TAG,
	REG_OPS_CMD_MAX_NUM,
};

enum EYE_SCAN_OPS_CMD {
	EYE_SCAN_ID,
	EYE_SCAN_SENSORIDX,
	EYE_SCAN_CMD,
	EYE_SCAN_VAL,
};


static int parse_debug_csi_port(char *csi_str)
{
	char csi_names[20];
	int csi_port = -1;
	int ret, i;

	if (!csi_str)
		return -1;

	for (i = 0; i < CSI_PORT_MAX_NUM; i++) {
		memset(csi_names, 0, ARRAY_SIZE(csi_names));
		ret = snprintf(csi_names, 10, "csi-%s", csi_port_names[i]);
		if (ret < 0)
			break;

		if (!strcasecmp(csi_str, csi_names)) {
			csi_port = i;
			break;
		}
	}

	return csi_port;
}

static int get_sensor_idx(struct seninf_ctx *ctx)
{
	int val = 0;
	struct v4l2_subdev *sensor_sd = NULL;
	struct v4l2_ctrl *ctrl;

	if (!ctx->sensor_sd) {
		dev_info(ctx->dev, "no sensor subdev\n");
		return -EINVAL;
	}

	sensor_sd = ctx->sensor_sd;

	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler, V4L2_CID_MTK_SENSOR_IDX);
	if (!ctrl) {
		dev_info(ctx->dev, "no sensor idx in subdev %s\n", sensor_sd->name);
		return -EINVAL;
	}

	val = v4l2_ctrl_g_ctrl(ctrl);

	dev_info(ctx->dev, "[%s]: %d\n", __func__, val);

	return val;
}

static void dbg_deinit_chmux(struct seninf_ctx *ctx)
{
	if (!ctx)
		return;

	if (ctx->dbg_chmux_param) {
		kfree(ctx->dbg_chmux_param->settings);
		ctx->dbg_chmux_param->settings = NULL;
		ctx->dbg_chmux_param->num = 0;

		kfree(ctx->dbg_chmux_param);
		ctx->dbg_chmux_param = NULL;
	}
}

static void dbg_init_chmux(struct seninf_ctx *ctx)
{
	if (!ctx)
		return;

	dbg_deinit_chmux(ctx);

	ctx->dbg_chmux_param = kzalloc(sizeof(struct mtk_cam_seninf_mux_param),
				       GFP_KERNEL);
}

static void dbg_commit_chmux(struct seninf_ctx *ctx)
{
	if (!ctx)
		return;

	if (ctx->dbg_chmux_param)
		mtk_cam_seninf_streaming_mux_change(ctx->dbg_chmux_param);
}

static void dbg_set_camtg(struct seninf_ctx *ctx, int pad_id, int camtg, int tag_id)
{
	int num = 0;
	struct mtk_cam_seninf_mux_setting *old_settings, *new_settings;

	if (!ctx)
		return;

	if (ctx->dbg_chmux_param) {
		num = ctx->dbg_chmux_param->num + 1;
		if (num < 1) {
			dev_info(ctx->dev, "error: of dbg setting num %d\n", num);
			return;
		}

		new_settings = kzalloc(sizeof(struct mtk_cam_seninf_mux_setting) * num,
				       GFP_KERNEL);
		if (!new_settings)
			return;

		old_settings = ctx->dbg_chmux_param->settings;
		if (old_settings) {
			memcpy(new_settings, old_settings,
			       sizeof(struct mtk_cam_seninf_mux_setting) * (num - 1));
			kfree(old_settings);
		}

		new_settings[num - 1].seninf = &ctx->subdev;
		new_settings[num - 1].enable = 1;
		new_settings[num - 1].source = pad_id;
		new_settings[num - 1].camtg = camtg;
		new_settings[num - 1].tag_id = tag_id;

		ctx->dbg_chmux_param->settings = new_settings;
		ctx->dbg_chmux_param->num += 1;
	} else {
		mtk_cam_seninf_set_camtg_camsv(&ctx->subdev,
					       pad_id, camtg, tag_id);
	}
}

static ssize_t debug_ops_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	char delim[] = "\n\t\r\x0a\x0d ";
	char *token = NULL;
	char *sbuf = kzalloc(sizeof(char) * (count + 1), GFP_KERNEL);
	char *s = sbuf;
	int ret, i, csi_port, val_signed = -1, rg_idx = -1, eye_scan_rg_idx = -1;
	unsigned int num_para = 0;
	char *arg[REG_OPS_CMD_MAX_NUM];
	struct seninf_core *core = dev_get_drvdata(dev);
	struct seninf_ctx *ctx, *seninf_ctx = NULL;
	u32 pad_id, camtg, tag_id, sensor_idx;
	u64 val;
	char *eye_scan_log = NULL;
	bool find_sensor_subdev = 0;

	if (!sbuf)
		goto ERR_DEBUG_OPS_STORE;

	memcpy(sbuf, buf, count);

	token = strsep(&s, delim);
	while (token != NULL && num_para < REG_OPS_CMD_MAX_NUM) {
		if (strlen(token)) {
			arg[num_para] = token;
			num_para++;
		}

		token = strsep(&s, delim);
	}

	if (num_para < (REG_OPS_CMD_ID + 1)) {
		dev_info(dev, "Wrong command parameter number\n");
		goto ERR_DEBUG_OPS_STORE;
	}

	if (strncmp("SET_REG", arg[REG_OPS_CMD_ID], sizeof("SET_REG")) == 0) {
		if (num_para != (REG_OPS_CMD_VAL + 1)) {
			dev_info(dev, "Wrong command parameter number\n");
			goto ERR_DEBUG_OPS_STORE;
		}

		for (i = 0; i < REG_KEY_MAX_NUM; i++) {
			if (!strcasecmp(arg[REG_OPS_CMD_RG], set_reg_names[i]))
				rg_idx = i;
		}
		if (rg_idx < 0)
			goto ERR_DEBUG_OPS_STORE;

		ret = kstrtoull(arg[REG_OPS_CMD_VAL], 0, &val);
		if (ret)
			goto ERR_DEBUG_OPS_STORE;

		csi_port = parse_debug_csi_port(arg[REG_OPS_CMD_CSI]);
		if (csi_port < 0)
			goto ERR_DEBUG_OPS_STORE;

		// reg call
		mutex_lock(&core->mutex);

		list_for_each_entry(ctx, &core->list, list) {
			if (csi_port == ctx->port)
				g_seninf_ops->_set_reg(ctx, rg_idx, val);
		}

		mutex_unlock(&core->mutex);
	} else if (strncmp("CHMUX_BEGIN", arg[REG_OPS_CMD_ID], sizeof("CHMUX_BEGIN")) == 0) {
		if (num_para != (REG_OPS_CMD_CSI + 1)) {
			dev_info(dev, "Wrong command parameter number\n");
			goto ERR_DEBUG_OPS_STORE;
		}

		csi_port = parse_debug_csi_port(arg[REG_OPS_CMD_CSI]);
		if (csi_port < 0)
			goto ERR_DEBUG_OPS_STORE;

		// set call
		mutex_lock(&core->mutex);
		list_for_each_entry(ctx, &core->list, list) {
			if (csi_port == ctx->port) {
				seninf_ctx = ctx;
				break;
			}
		}
		mutex_unlock(&core->mutex);

		dev_info(dev, "seninf_ctx != NULL?  %d\n", (seninf_ctx != NULL));

		dbg_init_chmux(seninf_ctx);

	} else if (strncmp("CHMUX_END", arg[REG_OPS_CMD_ID], sizeof("CHMUX_END")) == 0) {
		if (num_para != (REG_OPS_CMD_CSI + 1)) {
			dev_info(dev, "Wrong command parameter number\n");
			goto ERR_DEBUG_OPS_STORE;
		}

		csi_port = parse_debug_csi_port(arg[REG_OPS_CMD_CSI]);
		if (csi_port < 0)
			goto ERR_DEBUG_OPS_STORE;

		// set call
		mutex_lock(&core->mutex);
		list_for_each_entry(ctx, &core->list, list) {
			if (csi_port == ctx->port) {
				seninf_ctx = ctx;
				break;
			}
		}
		mutex_unlock(&core->mutex);

		dbg_commit_chmux(seninf_ctx);
		dbg_deinit_chmux(seninf_ctx);

	} else if (strncmp("SET_CAMTG", arg[REG_OPS_CMD_ID], sizeof("SET_CAMTG")) == 0) {
		if (num_para != (REG_OPS_CMD_TAG + 1)) {
			dev_info(dev, "Wrong command parameter number\n");
			goto ERR_DEBUG_OPS_STORE;
		}

		csi_port = parse_debug_csi_port(arg[REG_OPS_CMD_CSI]);
		if (csi_port < 0)
			goto ERR_DEBUG_OPS_STORE;

		ret = kstrtouint(arg[REG_OPS_CMD_PAD], 0, &pad_id);
		if (ret)
			goto ERR_DEBUG_OPS_STORE;

		ret = kstrtouint(arg[REG_OPS_CMD_CAMTG], 0, &camtg);
		if (ret)
			goto ERR_DEBUG_OPS_STORE;

		ret = kstrtouint(arg[REG_OPS_CMD_TAG], 0, &tag_id);
		if (ret)
			goto ERR_DEBUG_OPS_STORE;

		// set call
		mutex_lock(&core->mutex);
		list_for_each_entry(ctx, &core->list, list) {
			if (csi_port == ctx->port) {
				seninf_ctx = ctx;
				break;
			}
		}
		mutex_unlock(&core->mutex);

		if (seninf_ctx)
			dbg_set_camtg(seninf_ctx, pad_id, camtg, tag_id);
	} else if (strncmp("EYE_SCAN", arg[EYE_SCAN_ID], sizeof("EYE_SCAN")) == 0){
		for (i = 0; i < EYE_SCAN_MAX_NUM; i++) {
			if (!strcasecmp(arg[EYE_SCAN_CMD], eye_scan_names[i])){
				eye_scan_rg_idx = i;
				break;
			}
		}
		if (eye_scan_rg_idx < 0){
			ret = snprintf(debug_ops_show_log, DEBUG_OPS_SHOW_LOG_SIZE,
				       "[EYE_SCAN FAIL] no such command\n");
			if (ret < 0)
				dev_info(dev, "failed to snprintf\n");
			dev_info(dev, "[%s] wrong eye_scan_rg_idx, line=%d\n", __func__, __LINE__);
			goto ERR_DEBUG_OPS_STORE;
		}

		if (!((eye_scan_rg_idx == EYE_SCAN_KEYS_GET_CRC_STATUS) ||\
			  (eye_scan_rg_idx == EYE_SCAN_KEYS_CDR_DELAY_DPHY_EN) ||\
			  (eye_scan_rg_idx == EYE_SCAN_KEYS_FLUSH_CRC_STATUS) ||\
			  (eye_scan_rg_idx == EYE_SCAN_KEYS_GET_EQ_DG0_EN) ||\
			  (eye_scan_rg_idx == EYE_SCAN_KEYS_GET_EQ_SR0) ||\
			  (eye_scan_rg_idx == EYE_SCAN_KEYS_GET_EQ_DG1_EN) ||\
			  (eye_scan_rg_idx == EYE_SCAN_KEYS_GET_EQ_SR1) ||\
			  (eye_scan_rg_idx == EYE_SCAN_KEYS_GET_EQ_BW) ||\
			  (eye_scan_rg_idx == EYE_SCAN_KEYS_GET_CDR_DELAY) ||\
			  (eye_scan_rg_idx == EYE_SCAN_KEYS_GET_EQ_OFFSET))) {
			ret = kstrtoint(arg[EYE_SCAN_VAL], 0, &val_signed);
			if (ret){
				ret = snprintf(debug_ops_show_log, DEBUG_OPS_SHOW_LOG_SIZE,
					       "[EYE_SCAN FAIL] decode value (str2int) fail\n");
				if (ret < 0)
					dev_info(dev, "failed to snprintf\n");
				dev_info(dev, "[%s] str2int fail, line=%d\n", __func__, __LINE__);
				goto ERR_DEBUG_OPS_STORE;
			}
		}
		ret = kstrtouint(arg[EYE_SCAN_SENSORIDX], 0, &sensor_idx);
		if (ret) {
			ret = snprintf(debug_ops_show_log, DEBUG_OPS_SHOW_LOG_SIZE,
				       "[EYE_SCAN FAIL] decode sensor_idx (str2int) fail\n");
			if (ret < 0)
				dev_info(dev, "failed to snprintf\n");
			dev_info(dev, "[%s] str2int fail, line=%d\n", __func__, __LINE__);
			goto ERR_DEBUG_OPS_STORE;
		}

		mutex_lock(&core->mutex);
		list_for_each_entry(ctx, &core->list, list) {
			dev_info(dev, "get_sensor_idx(ctx) = %d ctx->port=%d, sensor_idx=%d\n", get_sensor_idx(ctx),ctx->port,sensor_idx);
			if (sensor_idx == get_sensor_idx(ctx)) {
				find_sensor_subdev = 1;

				eye_scan_log = kzalloc(DEBUG_OPS_SHOW_LOG_SIZE + 1, GFP_KERNEL);
				if (eye_scan_log != NULL) {
					g_seninf_ops->_eye_scan(ctx, eye_scan_rg_idx, val_signed, eye_scan_log, (int)DEBUG_OPS_SHOW_LOG_SIZE );
					ret = snprintf(debug_ops_show_log, DEBUG_OPS_SHOW_LOG_SIZE, eye_scan_log);
					if (ret < 0)
						dev_info(dev, "failed to snprintf\n");
				}
				kfree(eye_scan_log);

			}
		}
		mutex_unlock(&core->mutex);

		if (!find_sensor_subdev) {
			dev_info(dev, "[EYE_SCAN FAIL] could not find sensor subdev\n");
			ret = snprintf(debug_ops_show_log, DEBUG_OPS_SHOW_LOG_SIZE,
				       "[EYE_SCAN FAIL] could not find sensor subdev\n");
			if (ret < 0)
				dev_info(dev, "failed to snprintf\n");
		}
	}

ERR_DEBUG_OPS_STORE:

	kfree(sbuf);

	return count;
}

static DEVICE_ATTR_RW(debug_ops);

static u32 seninf_vsync_debug;
module_param(seninf_vsync_debug, uint, 0644);
MODULE_PARM_DESC(seninf_vsync_debug, "seninf_vsync_debug");

static u32 seninf_dbg_log;
module_param(seninf_dbg_log, uint, 0644);
MODULE_PARM_DESC(seninf_dbg_log, "seninf_dbg_log");

#define SENINF_DVFS_READY
#ifdef SENINF_DVFS_READY
static int seninf_dfs_init(struct seninf_dfs *dfs, struct device *dev)
{
	int ret, i;
	struct dev_pm_opp *opp;
	unsigned long freq;

	dfs->dev = dev;

	ret = dev_pm_opp_of_add_table(dev);
	if (ret < 0) {
		dev_info(dev, "fail to init opp table: %d\n", ret);
		return ret;
	}

	dfs->cnt = dev_pm_opp_get_opp_count(dev);

	dfs->freqs = devm_kzalloc(dev,
				  sizeof(unsigned long) * dfs->cnt, GFP_KERNEL);
	dfs->volts = devm_kzalloc(dev,
				  sizeof(unsigned long) * dfs->cnt, GFP_KERNEL);

	if (!dfs->freqs || !dfs->volts) {
		dev_info(dev, "devm_kzalloc failed\n");
		return -ENOMEM;
	}

	i = 0;
	freq = 0;
	while (!IS_ERR(opp = dev_pm_opp_find_freq_ceil(dev, &freq))) {
		dfs->freqs[i] = freq;
		dfs->volts[i] = dev_pm_opp_get_voltage(opp);
		freq++;
		i++;
		dev_pm_opp_put(opp);
	}

	for (i = 0; i < dfs->cnt; i++) {
		dev_info(dev, "dfs[%d] freq %ld volt %ld\n",
			 i, dfs->freqs[i], dfs->volts[i]);
	}

	return 0;
}
#endif

static int seninf_dfs_exit(struct seninf_dfs *dfs)
{
	if (!dfs->cnt)
		return 0;

	dev_pm_opp_of_remove_table(dfs->dev);

	return 0;
}

static int __seninf_dfs_set(struct seninf_ctx *ctx, unsigned long freq)
{
	int i, ret;
	struct seninf_ctx *tmp;
	struct seninf_core *core = ctx->core;
	struct seninf_dfs *dfs = &core->dfs;
	unsigned long require = 0, old;

	if (!dfs->cnt)
		return 0;

	if (!core->clk[CLK_MMDVFS]) {
		dev_info(dfs->dev, "%s: mmdvfs_clk is not ready\n", __func__);
		return -EINVAL;
	}

	old = ctx->isp_freq;
	ctx->isp_freq = freq;

	list_for_each_entry(tmp, &core->list, list) {
		if (tmp->isp_freq > require)
			require = tmp->isp_freq;
	}

	for (i = 0; i < dfs->cnt; i++) {
		if (dfs->freqs[i] >= require)
			break;
	}

	if (i == dfs->cnt) {
		ctx->isp_freq = old;
		return -EINVAL;
	}

	ret = clk_set_rate(core->clk[CLK_MMDVFS], dfs->freqs[i]);

	if (ret < 0) {
		dev_info(ctx->dev, "[%s] clk_set_rate %lu failed\n",
		__func__, dfs->freqs[i]);
		return -EINVAL;
	}

	dev_info(ctx->dev, "freq %ld require %ld selected %ld\n",
		 freq, require, dfs->freqs[i]);

	return 0;
}

static int seninf_dfs_set(struct seninf_ctx *ctx, unsigned long freq)
{
	int ret = 0;
	struct seninf_core *core = ctx->core;

	if (core == NULL) {
		dev_info(ctx->dev, "%s core is NULL\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&core->mutex);
	if (ctx->allow_adjust_isp_en)
		ret = __seninf_dfs_set(ctx, freq);
	mutex_unlock(&core->mutex);

	return ret;
}

static int seninf_core_pm_runtime_enable(struct seninf_core *core)
{
	int i;

	core->pm_domain_cnt = of_count_phandle_with_args(core->dev->of_node,
					"power-domains",
					"#power-domain-cells");
	if (core->pm_domain_cnt == 1)
		pm_runtime_enable(core->dev);
	else if (core->pm_domain_cnt > 1) {
		core->pm_domain_devs = devm_kcalloc(core->dev, core->pm_domain_cnt,
					sizeof(*core->pm_domain_devs), GFP_KERNEL);
		if (!core->pm_domain_devs)
			return -ENOMEM;

		for (i = 0; i < core->pm_domain_cnt; i++) {
			core->pm_domain_devs[i] =
				dev_pm_domain_attach_by_id(core->dev, i);

			if (IS_ERR_OR_NULL(core->pm_domain_devs[i])) {
				dev_info(core->dev, "%s: fail to probe pm id %d\n",
					__func__, i);
				core->pm_domain_devs[i] = NULL;
			}
		}
	} else
		dev_info(core->dev, "core->pm_domain_cnt < 0\n");

	return 0;
}

static int seninf_core_pm_runtime_disable(struct seninf_core *core)
{
	int i;

	if (core->pm_domain_cnt == 1)
		pm_runtime_disable(core->dev);
	else if (core->pm_domain_cnt > 1) {
		if (!core->pm_domain_devs)
			return -EINVAL;

		for (i = 0; i < core->pm_domain_cnt; i++) {
			if (core->pm_domain_devs[i])
				dev_pm_domain_detach(core->pm_domain_devs[i], 1);
		}
	} else
		dev_info(core->dev, "core->pm_domain_cnt < 0\n");

	return 0;
}

static int seninf_core_pm_runtime_get_sync(struct seninf_core *core)
{
	int i;
	int ret = 0;

	if (core->pm_domain_cnt == 1) {
#ifndef REDUCE_KO_DEPENDANCY_FOR_SMT
		mtk_mmdvfs_enable_vcp(true, VCP_PWR_USR_SENIF);
#endif
		ret = pm_runtime_get_sync(core->dev);
		if (ret < 0) {
			dev_info(core->dev, "pm_runtime_get_sync(fail),ret(%d)\n", ret);
			return ret;
		}
	} else if (core->pm_domain_cnt > 1) {
		if (!core->pm_domain_devs)
			return -EINVAL;

		for (i = 0; i < core->pm_domain_cnt; i++) {
			if (core->pm_domain_devs[i] != NULL) {
#ifndef REDUCE_KO_DEPENDANCY_FOR_SMT
				mtk_mmdvfs_enable_vcp(true, VCP_PWR_USR_SENIF);
#endif
				ret = pm_runtime_get_sync(core->pm_domain_devs[i]);
				if (ret < 0) {
					dev_info(core->dev,
						"pm_runtime_get_sync(fail),ret(%d)\n",
						ret);
					return ret;
				}
			}
		}
	} else
		dev_info(core->dev, "core->pm_domain_cnt < 0\n");

	return 0;
}

static int seninf_core_pm_runtime_put(struct seninf_core *core)
{
	int i;
	int ret = 0;

	if (core->pm_domain_cnt == 1) {
		ret = pm_runtime_put_sync(core->dev);
		if (ret < 0)
			dev_info(core->dev, "pm_runtime_put_sync(fail),ret(%d)\n", ret);
#ifndef REDUCE_KO_DEPENDANCY_FOR_SMT
		mtk_mmdvfs_enable_vcp(false, VCP_PWR_USR_SENIF);
#endif
	} else if (core->pm_domain_cnt > 1) {
		if (!core->pm_domain_devs)
			return -ENOMEM;

		for (i = core->pm_domain_cnt - 1; i >= 0; i--) {
			if (core->pm_domain_devs[i] != NULL) {
				ret = pm_runtime_put_sync(core->pm_domain_devs[i]);
				if (ret < 0)
					dev_info(core->dev,
						"pm_runtime_put_sync(fail),ret(%d)\n",
						ret);
#ifndef REDUCE_KO_DEPENDANCY_FOR_SMT
				mtk_mmdvfs_enable_vcp(false, VCP_PWR_USR_SENIF);
#endif
			}
		}
	} else
		dev_info(core->dev, "core->pm_domain_cnt < 0\n");

	return 0;
}

#if is_irq_ready
static irqreturn_t mtk_irq_seninf(int irq, void *data)
{
	unsigned int wake_thread = 0;

	wake_thread = g_seninf_ops->_irq_handler(irq, data);
	return (wake_thread) ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static irqreturn_t mtk_thread_irq_seninf(int irq, void *data)
{
	g_seninf_ops->_thread_irq_handler(irq, data);
	return IRQ_HANDLED;
}
#endif

static int get_seninf_ops(struct device *dev, struct seninf_core *core)
{
	int i, ret, cam_type_cnt = 0;
	const char *ver;
	u32 read_prop_test;

	ret = of_property_read_string(dev->of_node, "mtk-csi-phy-ver", &ver);

	if (ret) {
		g_seninf_ops = &mtk_csi_phy_3_0;
		dev_info(dev, "%s: INFO: phy default mtk-csi-phy-3-0\n", __func__);
	} else {
		for (i = 0; i < SENINF_PHY_VER_NUM; i++) {
			if (!strcasecmp(ver, csi_phy_versions[i])) {
				// Support phy 3.0 & phy 3.1
				if (i == SENINF_PHY_3_1) {
					g_seninf_ops = &mtk_csi_phy_3_1;
					dev_info(dev, "%s: INFO: phy config mtk-csi-phy-3-1\n", __func__);
				} else {
					g_seninf_ops = &mtk_csi_phy_3_0;
					dev_info(dev, "%s: INFO: phy config mtk-csi-phy-3-0\n", __func__);
				}
			}
		}
	}

	of_property_read_u32(dev->of_node, "seninf-num",
		&g_seninf_ops->seninf_num);
	of_property_read_u32(dev->of_node, "mux-num",
		&g_seninf_ops->mux_num);
	of_property_read_u32(dev->of_node, "cam-mux-num",
		&g_seninf_ops->cam_mux_num);
	of_property_read_u32(dev->of_node, "pref-mux-num",
		&g_seninf_ops->pref_mux_num);
	ret = of_property_read_string(dev->of_node, "mtk-iomem-ver",
		&g_seninf_ops->iomem_ver);

	if (!ret) {
		dev_info(dev,
			"%s: NOTICE: read property:(mtk_iomem_ver) success, ret:%d, using special mapping order\n",
			__func__, ret);
	} else {
		dev_info(dev,
			"%s: NOTICE: read property:(mtk_iomem_ver) not found, ret:%d, using default mapping order\n",
			__func__, ret);
	}

	for (i = 0; i < TYPE_MAX_NUM; i++) {
		ret = of_property_read_u32_index(dev->of_node, mux_range_name[i],
						0, &read_prop_test);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: try get mux_range property '%s' not found with ret %d\n",
				__func__, mux_range_name[i], ret);
			continue;
		}
		cam_type_cnt++;
		dev_info(dev,
				"%s: INFO:  try get mux_range property '%s' is found, cam_type_cnt %d\n",
				__func__, mux_range_name[i], cam_type_cnt);

		ret = of_property_read_u32_index(dev->of_node, muxvr_range_name[i],
						0, &read_prop_test);
		if (ret == 0) {
			core->is_porting_muxvr_range = true;
			dev_info(dev,
				"%s: found muxvr_range proprety '%s' successfully, is_porting_muxvr_range %d\n",
				__func__, mux_range_name[i], core->is_porting_muxvr_range);

		} else {
			dev_info(dev,
				"%s: ERROR: try get muxvr_range property '%s' not found with ret %d\n",
				__func__, mux_range_name[i], ret);
		}

	}

	for (i = 0; i < cam_type_cnt; i++) {
		ret = of_property_read_u32_index(dev->of_node, mux_range_name[i],
						0, &core->mux_range[i].first);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(mux_range_name[%d] (first)) failed, not modify pointer, ret:%d\n",
				__func__, i, ret);
			return -1;
		}

		ret = of_property_read_u32_index(dev->of_node, mux_range_name[i],
						1, &core->mux_range[i].second);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(mux_range_name[%d] (second)) failed, not modify pointer, ret:%d\n",
				__func__, i, ret);
			return -1;
		}

		ret = of_property_read_u32_index(dev->of_node, cammux_range_name[i],
						0, &core->cammux_range[i].first);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(cammux_range_name[%d] (first)) failed, not modify pointer, ret:%d\n",
				__func__, i, ret);
			return -1;
		}

		ret = of_property_read_u32_index(dev->of_node, cammux_range_name[i],
						1, &core->cammux_range[i].second);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(cammux_range_name[%d] (second)) failed, not modify pointer, ret:%d\n",
				__func__, i, ret);
			return -1;
		}

		if (core->is_porting_muxvr_range) {
			ret = of_property_read_u32_index(dev->of_node, muxvr_range_name[i],
						0, &core->muxvr_range[i].first);
			if (ret) {
				dev_info(dev,
					"%s: ERROR: read property index:(muxvr_range_name[%d] (first)) failed, not modify pointer, ret:%d\n",
					__func__, i, ret);
				return -1;
			}

			ret = of_property_read_u32_index(dev->of_node, muxvr_range_name[i],
							1, &core->muxvr_range[i].second);
			if (ret) {
				dev_info(dev,
					"%s: ERROR: read property index:(muxvr_range_name[%d] (second)) failed, not modify pointer, ret:%d\n",
					__func__, i, ret);
				return -1;
			}
		}
	}

	for (i = CDPHY_DVFS_STEP_0; i < CDPHY_DVFS_STEP_MAX_NUM; i++) {
		/* cphy 4d1c data rate maximum */
		ret = of_property_read_u32_index(dev->of_node,
			cdphy_dvfs_step_name[i], 0,
			&core->cdphy_dvfs_step[i].cphy_data_rate.first);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(cdphy_dvfs_step_name[%d].cphy_data_rate.first) failed,ret:%d\n",
				__func__, i, ret);
			core->cdphy_dvfs_step[i].cphy_data_rate.first = 0;
		}
		/* cphy 2d1c data rate maximum */
		ret = of_property_read_u32_index(dev->of_node,
			cdphy_dvfs_step_name[i], 1,
			&core->cdphy_dvfs_step[i].cphy_data_rate.second);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(cdphy_dvfs_step_name[%d].cphy_data_rate.second) failed,ret:%d\n",
				__func__, i, ret);
			core->cdphy_dvfs_step[i].cphy_data_rate.second = 0;
		}
		/* dphy 4d1c data rate maximum */
		ret = of_property_read_u32_index(dev->of_node,
			cdphy_dvfs_step_name[i], 2,
			&core->cdphy_dvfs_step[i].dphy_data_rate.first);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(cdphy_dvfs_step_name[%d].dphy_data_rate.first) failed,ret:%d\n",
				__func__, i, ret);
			core->cdphy_dvfs_step[i].dphy_data_rate.first = 0;
		}
		/* dphy 2d1c data rate maximum */
		ret = of_property_read_u32_index(dev->of_node,
			cdphy_dvfs_step_name[i], 3,
			&core->cdphy_dvfs_step[i].dphy_data_rate.second);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(cdphy_dvfs_step_name[%d].dphy_data_rate.second) failed,ret:%d\n",
				__func__, i, ret);
			core->cdphy_dvfs_step[i].dphy_data_rate.second = 0;
		}
		/* cdphy csi clk according to vcore */
		ret = of_property_read_u32_index(dev->of_node,
			cdphy_dvfs_step_name[i], 4,
			&core->cdphy_dvfs_step[i].csi_clk);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(cdphy_dvfs_step_name[%d].csi_clk) failed,ret:%d\n",
				__func__, i, ret);
			core->cdphy_dvfs_step[i].csi_clk = 0;
		}
		/* cdphy 4d1c vcore minimum */
		ret = of_property_read_u32_index(dev->of_node,
			cdphy_dvfs_step_name[i], 5,
			&core->cdphy_dvfs_step[i].cdphy_voltage.first);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(cdphy_dvfs_step_name[%d].cdphy_voltage.first) failed,ret:%d\n",
				__func__, i, ret);
			core->cdphy_dvfs_step[i].cdphy_voltage.first = 0;
		}
		/* cdphy 2d1c vcore minimum */
		ret = of_property_read_u32_index(dev->of_node,
			cdphy_dvfs_step_name[i], 6,
			&core->cdphy_dvfs_step[i].cdphy_voltage.second);
		if (ret) {
			dev_info(dev,
				"%s: ERROR: read property index:(cdphy_dvfs_step_name[%d].cdphy_voltage.second) failed,ret:%d\n",
				__func__, i, ret);
			core->cdphy_dvfs_step[i].cdphy_voltage.second = 0;
		}
		dev_info(dev,
			"[%s]: step[%d].c/dphy_data_rate.first/second:%u/%u/%u/%u,csi_clk:%u,voltage.first/second:%u/%u,ret:%d\n",
			__func__, i,
			core->cdphy_dvfs_step[i].cphy_data_rate.first,
			core->cdphy_dvfs_step[i].cphy_data_rate.second,
			core->cdphy_dvfs_step[i].dphy_data_rate.first,
			core->cdphy_dvfs_step[i].dphy_data_rate.second,
			core->cdphy_dvfs_step[i].csi_clk,
			core->cdphy_dvfs_step[i].cdphy_voltage.first,
			core->cdphy_dvfs_step[i].cdphy_voltage.second, ret);
	}

	dev_info(dev,
		"%s: seninf_num = %d, mux_num = %d, cam_mux_num = %d, pref_mux_num =%d\n",
		__func__,
		g_seninf_ops->seninf_num,
		g_seninf_ops->mux_num,
		g_seninf_ops->cam_mux_num,
		g_seninf_ops->pref_mux_num);

	return 0;
}

static int seninf_core_probe(struct platform_device *pdev)
{
	int i, j, ret;
#if is_irq_ready
	int irq;
#endif
	struct resource *res;
	struct seninf_core *core;
	struct device *dev = &pdev->dev;
	const char *str = NULL;
	u32 tmp_no = 0;
	struct device_node *tmp_node;

	device_enable_async_suspend(dev);

	core = devm_kzalloc(&pdev->dev, sizeof(*core), GFP_KERNEL);
	if (!core) {
		dev_err(dev, "%s: kzalloc core failed\n", __func__);
		WRAP_AEE_EXCEPTION("seninf_core_probe", "Kzalloc");
		return -ENOMEM;
	}

	dev_set_drvdata(dev, core);
	core->dev = dev;
	mutex_init(&core->mutex);
	mutex_init(&core->cammux_page_ctrl_mutex);
	mutex_init(&core->seninf_top_mux_mutex);
	INIT_LIST_HEAD(&core->list);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "base");
	core->reg_if = devm_ioremap_resource(dev, res);
	if (IS_ERR(core->reg_if)) {
		dev_err(dev, "%s: ioremap core->reg_if failed\n", __func__);
		WRAP_AEE_EXCEPTION("seninf_core_probe", "ioremap reg_if");
		return PTR_ERR(core->reg_if);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ana-rx");
	core->reg_ana = devm_ioremap_resource(dev, res);
	if (IS_ERR(core->reg_ana)) {
		dev_err(dev, "%s: ioremap core->reg_ana failed\n", __func__);
		WRAP_AEE_EXCEPTION("seninf_core_probe", "ioremap reg_ana");
		return PTR_ERR(core->reg_ana);
	}

	ret = get_seninf_ops(dev, core);
	if (ret) {
		dev_err(dev, "%s: get_seninf_ops failed\n", __func__);
		WRAP_AEE_EXCEPTION("seninf_core_probe", "Get Seninf Ops");
		return ret;
	}
	mtk_cam_seninf_init_res(core);

	mtk_cam_seninf_tsrec_init(dev, core->reg_if);

	spin_lock_init(&core->spinlock_irq);

#if is_irq_ready
	/* Return: non-zero IRQ number on success, negative error number on failure. */
	irq = platform_get_irq_byname(pdev, "seninf-irq");
	if (irq <= 0) {
		dev_err(dev, "%s: failed to get seninf-irq number, ret:%d\n", __func__, irq);
		//return -ENODEV;
	} else {
		ret = devm_request_threaded_irq(dev, irq, mtk_irq_seninf,
					mtk_thread_irq_seninf, 0, dev_name(dev), core);
		if (ret) {
			dev_err(dev, "%s: Request seninf-irq failed\n", __func__);
			WRAP_AEE_EXCEPTION("seninf_core_probe", "Request seninf-irq");
			return ret;
		}
		dev_info(dev, "registered seninf-irq=%d\n", irq);
	}

	irq = platform_get_irq_byname(pdev, "tsrec-irq");
	if (irq <= 0) {
		dev_err(dev, "%s: failed to get tsrec-irq number, ret:%d\n", __func__, irq);
		//return -ENODEV;
	} else
		mtk_cam_seninf_tsrec_irq_init(core, irq);
#endif

	/* default platform properties */
	core->cphy_settle_delay_dt = SENINF_CPHY_SETTLE_DELAY_DT;
	core->dphy_settle_delay_dt = SENINF_DPHY_SETTLE_DELAY_DT;
	core->settle_delay_ck = SENINF_SETTLE_DELAY_CK;
	core->hs_trail_parameter = SENINF_HS_TRAIL_PARAMETER;

	/* read platform properties from device tree */
	of_property_read_u32(dev->of_node, "cphy-settle-delay-dt",
		&core->cphy_settle_delay_dt);
	of_property_read_u32(dev->of_node, "dphy-settle-delay-dt",
		&core->dphy_settle_delay_dt);
	of_property_read_u32(dev->of_node, "settle-delay-ck",
		&core->settle_delay_ck);
	of_property_read_u32(dev->of_node, "hs-trail-parameter",
		&core->hs_trail_parameter);
	of_property_read_u32(dev->of_node, "force-glp-enable",
		&core->force_glp_en);

	core->seninf_vsync_debug_flag = &seninf_vsync_debug;
	core->seninf_dbg_log = &seninf_dbg_log;

	core->aov_csi_clk_switch_flag = CSI_CLK_130;
	core->aov_abnormal_deinit_flag = 0;
	core->aov_abnormal_deinit_usr_fd_kill_flag = 0;
	core->aov_abnormal_init_flag = 0;
	core->aov_ut_debug_for_get_csi_param = 0;

	/* vcore node get */
	core->dvfsrc_vcore_power = devm_regulator_get_optional(dev, "dvfsrc-vcore");
	if (IS_ERR(core->dvfsrc_vcore_power)) {
		dev_err(dev, "%s: failed to get dvfsrc-vcore\n", __func__);
		core->dvfsrc_vcore_power = NULL;
	}

	/* clk node get */
	for (i = 0; i < CLK_MAXCNT; i++) {
		core->clk[i] = devm_clk_get(dev, clk_names[i]);
		if (IS_ERR(core->clk[i])) {
			dev_err(dev, "%s: failed to get %s\n", __func__, clk_names[i]);
			core->clk[i] = NULL;
			//return -EINVAL;
		}
	}

	// fmeter dbg property
	memset(core->fmeter, 0, sizeof(core->fmeter));
	for (i = 0; i < CLK_FMETER_MAX; i++) {
		str = NULL;
		tmp_node = of_find_node_by_name(dev->of_node, clk_fmeter_names[i]);
		if (tmp_node) {
			of_property_read_u32(tmp_node,
				"fmeter-no", &tmp_no);
			of_property_read_string(tmp_node,
				"fmeter-type", &str);
			of_node_put(tmp_node);
		} else {
			dev_info(dev, "[i:%d], tmp_node == NULL\n", i);
			continue;
		}
		if (str) {
			for (j = 0; j < ARRAY_SIZE(clk_fmeter_maps); j++) {
				if (strncmp(str, clk_fmeter_maps[j].key, strlen(str)) == 0) {
					core->fmeter[i].fmeter_type = clk_fmeter_maps[j].value;
					break;
				}
			}
			core->fmeter[i].fmeter_no = tmp_no;
		}

	}

	ret = of_platform_populate(dev->of_node, NULL, NULL, dev);
	if (ret) {
		dev_err(dev, "%s: create sub devices failed\n", __func__);
		WRAP_AEE_EXCEPTION("seninf_core_probe", "Create Sub Devices");
		return ret;
	}

#ifdef SENINF_DVFS_READY
	ret = seninf_dfs_init(&core->dfs, dev);
	if (ret) {
		dev_err(dev, "%s: failed to init dfs\n", __func__);
		//return ret;
	}
#endif

	ret = device_create_file(dev, &dev_attr_status);
	if (ret)
		dev_err(dev, "%s: failed to create sysfs status\n", __func__);

	ret = device_create_file(dev, &dev_attr_debug_ops);
	if (ret)
		dev_err(dev, "%s: failed to create sysfs debug ops\n", __func__);

	ret = device_create_file(dev, &dev_attr_err_status);
	if (ret)
		dev_err(dev, "%s: failed to create sysfs status\n", __func__);

	seninf_core_pm_runtime_enable(core);

	kthread_init_worker(&core->seninf_worker);
	core->seninf_kworker_task = kthread_run(kthread_worker_fn,
				&core->seninf_worker, "seninf_worker");
	if (IS_ERR(core->seninf_kworker_task)) {
		dev_err(dev, "%s: failed to start seninf kthread worker\n", __func__);
		core->seninf_kworker_task = NULL;
	} else {
		dev_info(dev, "%s: seninf kthread worker set prio to fifo\n", __func__);
		sched_set_fifo(core->seninf_kworker_task);
	}

	g_seninf_ops->_init_irq_fifo(core);

	return 0;
}

static int seninf_core_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct seninf_core *core = dev_get_drvdata(dev);

	seninf_dfs_exit(&core->dfs);
	of_platform_depopulate(dev);
	seninf_core_pm_runtime_disable(core);
	device_remove_file(dev, &dev_attr_status);
	device_remove_file(dev, &dev_attr_debug_ops);
	device_remove_file(dev, &dev_attr_err_status);

	mtk_cam_seninf_tsrec_uninit();

	if (core->seninf_kworker_task)
		kthread_stop(core->seninf_kworker_task);

	g_seninf_ops->_uninit_irq_fifo(core);

	return 0;
}

static const struct of_device_id seninf_core_of_match[] = {
	{.compatible = "mediatek,seninf-core"},
	{},
};
MODULE_DEVICE_TABLE(of, seninf_core_of_match);

struct platform_driver seninf_core_pdrv = {
	.probe	= seninf_core_probe,
	.remove	= seninf_core_remove,
	.driver	= {
		.name = "seninf-core",
		.of_match_table = seninf_core_of_match,
	},
};

static int get_csi_port(struct device *dev, int *port)
{
	int i, ret;
	const char *name;

	ret = of_property_read_string(dev->of_node, "csi-port", &name);
	if (ret)
		return ret;

	for (i = 0; i < CSI_PORT_MAX_NUM; i++) {
		if (!strcasecmp(name, csi_port_names[i])) {
			*port = i;
			return 0;
		}
	}

	return -1;
}

static int seninf_subscribe_event(struct v4l2_subdev *sd,
				  struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_CTRL:
		return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
	default:
		return -EINVAL;
	}
}

static void init_fmt(struct seninf_ctx *ctx)
{
	int i, j;

	for (i = 0 ; i < ARRAY_SIZE(ctx->fmt); i++) {
		ctx->fmt[i].format.code = MEDIA_BUS_FMT_SBGGR10_1X10;
		ctx->fmt[i].format.width = DEFAULT_WIDTH;
		ctx->fmt[i].format.height = DEFAULT_HEIGHT;
		ctx->fmt[i].format.field = V4L2_FIELD_NONE;
		ctx->fmt[i].format.colorspace = V4L2_COLORSPACE_SRGB;
		ctx->fmt[i].format.xfer_func = V4L2_XFER_FUNC_DEFAULT;
		ctx->fmt[i].format.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
		ctx->fmt[i].format.quantization = V4L2_QUANTIZATION_DEFAULT;
	}

	for (i = 0; i < ARRAY_SIZE(ctx->vcinfo.vc); i++)
		for(j = 0; j < MAX_DEST_NUM; j++)
			ctx->vcinfo.vc[i].dest[j].pix_mode = SENINF_DEF_PIXEL_MODE;
}
#ifdef CSI_EFUSE_SET
static int dev_read_csi_efuse(struct seninf_ctx *ctx)
{
	struct nvmem_cell *cell;
	size_t len = 0;
	u32 *buf;

	ctx->m_csi_efuse = 0x00000000;

	cell = nvmem_cell_get(ctx->dev, "rg_csi");
	dev_info(ctx->dev, "ctx->port = %d\n", ctx->port);
	if (IS_ERR(cell)) {
		if (PTR_ERR(cell) == -EPROBE_DEFER) {
			dev_info(ctx->dev, "read csi efuse returned with error cell %d\n",
				-EPROBE_DEFER-EPROBE_DEFER);
			return PTR_ERR(cell);
		}
		dev_info(ctx->dev, "read csi efuse returned with error cell %d\n", -1);
		return -1;
	}
	buf = (u32 *)nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);
	if (IS_ERR(buf)) {
		dev_info(ctx->dev, "read csi efuse returned with error buf\n");
		return PTR_ERR(buf);
	}
	ctx->m_csi_efuse = *buf;
	kfree(buf);
	dev_info(ctx->dev, "Efuse Data: 0x%08x\n", ctx->m_csi_efuse);

	return 0;
}

static int csi_port_switch_for_efuse(enum CSI_PORT csi_port_efuse)
{
	int sw_csi_port = CSI_PORT_0;

	switch (csi_port_efuse) {
	case CSI_PORT_0:
	case CSI_PORT_0A:
	case CSI_PORT_0B:
		sw_csi_port = CSI_PORT_0;
		break;
	case CSI_PORT_1:
	case CSI_PORT_1A:
	case CSI_PORT_1B:
		sw_csi_port = CSI_PORT_1;
		break;
	case CSI_PORT_2:
	case CSI_PORT_2A:
	case CSI_PORT_2B:
		sw_csi_port = CSI_PORT_2;
		break;
	case CSI_PORT_3:
	case CSI_PORT_3A:
	case CSI_PORT_3B:
		sw_csi_port = CSI_PORT_3;
		break;
	case CSI_PORT_4:
	case CSI_PORT_4A:
	case CSI_PORT_4B:
		sw_csi_port = CSI_PORT_4;
		break;
	case CSI_PORT_5:
	case CSI_PORT_5A:
	case CSI_PORT_5B:
		sw_csi_port = CSI_PORT_5;
		break;
	default:
		sw_csi_port = CSI_PORT_0;
		break;
	}
	return  sw_csi_port;
}

#if CSI_EFUSE_VERIFY_EN == 1
static int csi_efuse_value_verify(struct seninf_ctx *ctx)
{
	int checksum = 0;
	int csi_efuse_val = ctx->m_csi_efuse;
	int csi_port = csi_port_switch_for_efuse(ctx->port);
	int csi_verify_bit = (csi_efuse_val & 0x1f);
	int checksum2verify = 0;

	checksum += ((csi_efuse_val >> 27) & 0x1f);
	checksum += ((csi_efuse_val >> 22) & 0x1f);
	checksum += ((csi_efuse_val >> 17) & 0x1f);
	checksum += ((csi_efuse_val >> 12) & 0x1f);
	checksum += ((csi_efuse_val >> 7) & 0x1f);
	checksum += csi_port;
	dev_info(ctx->dev, "Efuse Data: 0x%08x, csi_port: %d, checksum: 0x%08x\n",
		ctx->m_csi_efuse, csi_port, checksum);

	if (checksum == csi_port) {
		dev_info(ctx->dev, "All value == 0 skip verify\n");
		return 0;
	}

	checksum2verify = (((checksum >> 4) & 0xf) + (checksum & 0xf)) & 0x1f;

	if (checksum2verify != csi_verify_bit) {
		dev_info(ctx->dev, "Efuse verify fail VerifyBit: 0x%08x, ChecksumToVerify: 0x%08x\n",
			csi_verify_bit, checksum2verify);
		return -1;
	} else {
		dev_info(ctx->dev, "Efuse verify pass VerifyBit: 0x%08x, ChecksumToVerify: 0x%08x\n",
			csi_verify_bit, checksum2verify);
	}
	return 0;
}
#endif  /*CSI_EFUSE_VERIFY_EN*/
#endif  /*CSI_EFUSE_SET*/

static const struct v4l2_mbus_framefmt fmt_default = {
	.code = MEDIA_BUS_FMT_SBGGR10_1X10,
	.width = DEFAULT_WIDTH,
	.height = DEFAULT_HEIGHT,
	.field = V4L2_FIELD_NONE,
	.colorspace = V4L2_COLORSPACE_SRGB,
	.xfer_func = V4L2_XFER_FUNC_DEFAULT,
	.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
	.quantization = V4L2_QUANTIZATION_DEFAULT,
};

static int mtk_cam_seninf_init_cfg(struct v4l2_subdev *sd,
				   struct v4l2_subdev_state *state)
{
	struct v4l2_mbus_framefmt *mf;
	unsigned int i;

	for (i = 0; i < sd->entity.num_pads; i++) {
#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
		mf = v4l2_subdev_state_get_format(state, i);
#else
		mf = v4l2_subdev_get_try_format(sd, state, i);
#endif
		*mf = fmt_default;
	}

	return 0;
}

static int mtk_cam_seninf_set_fmt(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
			  struct v4l2_subdev_format *fmt)
{
	struct seninf_ctx *ctx = sd_to_ctx(sd);
	struct v4l2_mbus_framefmt *format;
	char bSinkFormatChanged = 0;

	if (fmt->pad >= PAD_MAXCNT)
		return -EINVAL;

	format = &ctx->fmt[fmt->pad].format;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
		*v4l2_subdev_state_get_format(state, fmt->pad) = fmt->format;
#else
		*v4l2_subdev_get_try_format(sd, state, fmt->pad) = fmt->format;
#endif
		dev_dbg(ctx->dev, "s_fmt pad %d code/res 0x%x/%dx%d which %d=> 0x%x/%dx%d\n",
			fmt->pad,
			fmt->format.code,
			fmt->format.width,
			fmt->format.height,
			fmt->which,
			format->code,
			format->width,
			format->height);
	} else {
		/* NOTE: update vcinfo once the SINK format changed */
		if (fmt->pad == PAD_SINK)
			bSinkFormatChanged = 1;

		format->code = fmt->format.code;
		format->width = fmt->format.width;
		format->height = fmt->format.height;

		if (bSinkFormatChanged && !ctx->is_test_model && !ctx->streaming)
			mtk_cam_seninf_get_vcinfo(ctx);

		mtk_cam_sensor_get_vc_info_by_scenario(ctx, fmt->format.code);

		dev_info(ctx->dev, "s_fmt pad %d code/res 0x%x/%dx%d which %d=> 0x%x/%dx%d\n",
			fmt->pad,
			fmt->format.code,
			fmt->format.width,
			fmt->format.height,
			fmt->which,
			format->code,
			format->width,
			format->height);
	}

	return 0;
}

static int mtk_cam_seninf_get_fmt(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
			  struct v4l2_subdev_format *fmt)
{
	struct seninf_ctx *ctx = sd_to_ctx(sd);
	struct v4l2_mbus_framefmt *format;

	if (fmt->pad >= PAD_MAXCNT)
		return -EINVAL;

	format = &ctx->fmt[fmt->pad].format;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
		fmt->format = *v4l2_subdev_state_get_format(state, fmt->pad);
#else
		fmt->format = *v4l2_subdev_get_try_format(sd, state, fmt->pad);
#endif
	} else {
		fmt->format.code = format->code;
		fmt->format.width = format->width;
		fmt->format.height = format->height;
		fmt->format.field = format->field;
		fmt->format.colorspace = format->colorspace;
		fmt->format.xfer_func = format->xfer_func;
		fmt->format.ycbcr_enc = format->ycbcr_enc;
		fmt->format.quantization = format->quantization;
	}

	return 0;
}

static int set_aov_test_model_param(struct seninf_ctx *ctx,
	struct seninf_vc **vc, char enable, int vc_used)
{
	int i = 0;
	int ret = 0;

	pr_info("[%s]+\n", __func__);

	if (enable) {
		g_aov_param.sensor_idx = 5; // 5: test model
		aov_ctx[g_aov_param.sensor_idx] = ctx;
		g_aov_param.port = ctx->port;
		g_aov_param.portA = ctx->portA;
		g_aov_param.portB = ctx->portB;
		g_aov_param.is_4d1c = ctx->is_4d1c;
		g_aov_param.seninfIdx = ctx->seninfIdx;
		g_aov_param.cnt = vc_used;
		g_aov_param.is_test_model = ctx->is_aov_test_model;

		/* must enable mux(clk) before clk_set_parent
		 * pm_runtime_get_sync will call runtime_resume.
		 */
		ret = pm_runtime_get_sync(ctx->dev);
		if (ret < 0) {
			dev_info(ctx->dev, "%s pm_runtime_get_sync ret %d\n", __func__, ret);
			pm_runtime_put_noidle(ctx->dev);
			return ret;
		}

		g_aov_param.isp_freq = ISP_CLK_LOW;

		for (i = 0; i < vc_used; ++i) {
			vc[i]->enable = 1;
			vc[i]->dest[0].pix_mode = 2;

			vc[i]->dest_cnt = 1;
			vc[i]->dest[0].cam = 33;
			vc[i]->dest[0].mux = 5;
			vc[i]->dest[0].mux_vr = 33;

			dev_info(ctx->dev,
				"test mode mux %d, cam %d, pixel mode %d, vc = %d, dt = 0x%x\n",
				vc[i]->dest[0].mux, vc[i]->dest[0].cam, vc[i]->dest[0].pix_mode,
				vc[i]->vc, vc[i]->dt);

			g_aov_param.height = 480;
			g_aov_param.width = 640;
			g_aov_param.camtg = 33;

			udelay(40);
		}
	} else {
		pm_runtime_put_sync(ctx->dev);
		/* array size of aov_ctx[] is
		 * AOV_SENINF_NUM: most number of sensors support
		 */
		if (g_aov_param.sensor_idx < AOV_SENINF_NUM) {
			aov_ctx[g_aov_param.sensor_idx] = NULL;
			memset(&g_aov_param, 0, sizeof(struct mtk_seninf_aov_param));
		}
	}

	pr_info("[%s]-\n", __func__);

	return 0;
}

static int set_test_model(struct seninf_ctx *ctx, char enable)
{
	struct seninf_vc *vc[] = {NULL, NULL, NULL, NULL, NULL};
	int i = 0, vc_used = 0;
	struct seninf_mux *mux, *mux_by_camtype[TYPE_MAX_NUM] = {0};
	int vc_dt_filter = 1;
	struct seninf_dfs *dfs = &ctx->core->dfs;
	int ret = 0;

	pr_info("[%s]+\n", __func__);

	if (ctx->is_test_model == 1) {
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
	} else if (ctx->is_test_model == 2) {
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW1);
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW2);
	} else if (ctx->is_test_model == 3) {
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_PDAF0);
	} else if (ctx->is_test_model == 4) {
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW1);
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW2);
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_PDAF0);
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_PDAF1);
	} else if (ctx->is_test_model == 5) {
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
		vc[vc_used++] = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_W0);
	} else {
		dev_info(ctx->dev, "testmodel %d invalid\n", ctx->is_test_model);
		return -1;
	}

	for (; i < vc_used; ++i) {
		if (!vc[i]) {
			dev_info(ctx->dev, "vc not found\n");
			return -1;
		}
	}

	if (ctx->is_aov_test_model) {
		ctx->streaming = enable;
		return set_aov_test_model_param(ctx, &vc[0], enable, vc_used);
	}

	if (enable) {
		ret = pm_runtime_get_sync(ctx->dev);
		if (ret < 0) {
			dev_info(ctx->dev, "%s pm_runtime_get_sync ret %d\n", __func__, ret);
			pm_runtime_put_noidle(ctx->dev);
			return ret;
		}

		if (dfs->cnt)
			seninf_dfs_set(ctx, dfs->freqs[dfs->cnt - 1]);

		for (i = 0; i < vc_used; ++i) {
			vc[i]->dest_cnt = 1;
			vc[i]->dest[0].cam = ctx->pad2cam[vc[i]->out_pad][0];
			vc[i]->enable = 1;

			if (mux_by_camtype[vc[i]->dest[0].cam_type]) {
				mux = mux_by_camtype[vc[i]->dest[0].cam_type];
			} else {
				mux = mtk_cam_seninf_mux_get_by_type(ctx,
					     cammux2camtype(ctx, vc[i]->dest[0].cam));
				mux_by_camtype[vc[i]->dest[0].cam_type] = mux;
			}
			if (!mux)
				return -EBUSY;

			vc[i]->dest[0].mux = mux->idx;

			dev_info(ctx->dev,
				"test mode mux %d, cam %d, pixel mode %d, vc = %d, dt = 0x%x\n",
				vc[i]->dest[0].mux, vc[i]->dest[0].cam, vc[i]->dest[0].pix_mode,
				vc[i]->vc, vc[i]->dt);

			g_seninf_ops->_set_test_model(ctx,
					vc[i]->dest[0].mux, vc[i]->dest[0].cam, vc[i]->dest[0].pix_mode,
					vc_dt_filter, i, vc[i]->vc, vc[i]->dt, vc[i]->vc);
			if (vc[i]->out_pad == PAD_SRC_PDAF0)
				mdelay(40);
			else
				udelay(40);
		}
	} else {
		g_seninf_ops->_set_idle(ctx);
		mtk_cam_seninf_release_mux(ctx);
		mtk_cam_seninf_tsrec_n_reset(ctx->seninfIdx);
		if (dfs->cnt)
			seninf_dfs_set(ctx, 0);

		pm_runtime_put_sync(ctx->dev);
		ctx->is_test_model = 0;
		dev_info(ctx->dev, "[%s] test pattern off\n", __func__);
	}

	ctx->streaming = enable;

	pr_info("[%s]-\n", __func__);

	return 0;
}

static int config_hw_csi(struct seninf_ctx *ctx)
{
	int intf = ctx->seninfIdx;
	struct seninf_vcinfo *vcinfo = &ctx->vcinfo;
	struct seninf_glp_dt glpinfo;
#if AOV_GET_PARAM
	struct seninf_core *core = ctx->core;
#endif
	int ret = 0;

	memset(&glpinfo, 0, sizeof(struct seninf_glp_dt));

	mtk_cam_seninf_get_csi_param(ctx);
	mtk_cam_sensor_get_glp_dt(ctx, &glpinfo);

#if AOV_GET_PARAM
	if (!(core->aov_sensor_id < 0) &&
		!(core->current_sensor_id < 0) &&
		(core->current_sensor_id == core->aov_sensor_id)) {
		g_aov_param.port = ctx->port;
		g_aov_param.portA = ctx->portA;
		g_aov_param.portB = ctx->portB;
		g_aov_param.is_4d1c = ctx->is_4d1c;
		g_aov_param.seninfIdx = intf;
		g_aov_param.cnt = vcinfo->cnt;
		g_aov_param.seninf_dphy_settle_delay_dt =
			ctx->seninf_dphy_settle_delay_dt;
		g_aov_param.cphy_settle_delay_dt = ctx->cphy_settle_delay_dt;
		g_aov_param.dphy_settle_delay_dt = ctx->dphy_settle_delay_dt;
		g_aov_param.settle_delay_ck = ctx->settle_delay_ck;
		g_aov_param.hs_trail_parameter = ctx->hs_trail_parameter;
	}
#endif

	g_seninf_ops->_reset(ctx, intf);

	g_seninf_ops->_set_vc(ctx, intf, vcinfo, &glpinfo);

	ret = g_seninf_ops->_set_csi_mipi(ctx);
	if (ret) {
		dev_info(ctx->dev, "[%s][Error] _set_csi_mipi ret(%d)\n", __func__, ret);
		return ret;
	}

	ret = g_seninf_ops->_set_csi_afifo_pop(ctx);
	if (ret) {
		dev_info(ctx->dev, "[%s][Error] _set_csi_afifo_pop ret(%d)\n",
				__func__, ret);
		return ret;
	}

	return 0;
}

static int calc_buffered_pixel_rate(struct device *dev,
				    s64 width, s64 height,
				    s64 hblank, s64 vblank,
				    int fps_n, int fps_d, s64 *result)
{
	s64 buffered_pixel_rate;
	s64 orig_pixel_rate = *result;
	s64 pclk;
	s64 k;

	if (fps_d == 0 || width == 0 || hblank == 0 || ISP_CLK_LOW == 0) {
		dev_info(dev, "Prevent divided by 0, fps_d= %d, w= %llu, h= %llu, ISP_CLK= %d\n",
			fps_d, width, hblank, ISP_CLK_LOW);
		return 0;
	}

	/* calculate pclk */
	pclk = (width + hblank) * (height + vblank) * fps_n;
	do_div(pclk, fps_d);

	/* calculate buffered pixel_rate */
	buffered_pixel_rate = orig_pixel_rate * width;
	k = HW_BUF_EFFECT * orig_pixel_rate;
	do_div(k, ISP_CLK_LOW);
	do_div(buffered_pixel_rate, (width + hblank - k));
	*result = buffered_pixel_rate;

	dev_dbg(
		dev,
		"%s: w %llu h %llu hb %llu vb %llu fps %d/%d pclk %llu->%llu orig %llu k %llu hbe %d\n",
		__func__, width, height, hblank, vblank,
		fps_n, fps_d, pclk, buffered_pixel_rate, orig_pixel_rate, k, HW_BUF_EFFECT);

	return 0;
}

static int get_buffered_pixel_rate(struct seninf_ctx *ctx,
				   struct v4l2_subdev *sd, int sd_pad_idx,
				   s64 *result)
{
	int ret;
	struct v4l2_ctrl *ctrl;
	struct v4l2_subdev_format fmt;
	struct v4l2_subdev_frame_interval fi;
	s64 width, height, hblank, vblank;
#if AOV_GET_PARAM
	struct seninf_core *core = ctx->core;
#endif

	fmt.pad = sd_pad_idx;
	fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	ret = v4l2_subdev_call(sd, pad, get_fmt, NULL, &fmt);
	if (ret) {
		dev_info(ctx->dev, "no get_fmt in %s\n", sd->name);
		return ret;
	}

	width = fmt.format.width;
	height = fmt.format.height;

	memset(&fi, 0, sizeof(fi));
	fi.pad = sd_pad_idx;
	fi.reserved[0] = V4L2_SUBDEV_FORMAT_ACTIVE;
#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
	ret = v4l2_subdev_call_state_active(sd, pad, get_frame_interval, &fi);
#else
	ret = v4l2_subdev_call(sd, video, g_frame_interval, &fi);
#endif

	if (ret) {
		dev_info(ctx->dev, "no g_frame_interval in %s\n", sd->name);
		return ret;
	}

	ctrl = v4l2_ctrl_find(sd->ctrl_handler, V4L2_CID_HBLANK);
	if (!ctrl) {
		dev_info(ctx->dev, "no hblank in %s\n", sd->name);
		return -EINVAL;
	}

	hblank = v4l2_ctrl_g_ctrl(ctrl);

	ctrl = v4l2_ctrl_find(sd->ctrl_handler, V4L2_CID_VBLANK);
	if (!ctrl) {
		dev_info(ctx->dev, "no vblank in %s\n", sd->name);
		return -EINVAL;
	}

	vblank = v4l2_ctrl_g_ctrl(ctrl);

	/* update fps */
	ctx->fps_n = fi.interval.denominator;
	ctx->fps_d = fi.interval.numerator;

#if AOV_GET_PARAM
	if (!(core->aov_sensor_id < 0) &&
		!(core->current_sensor_id < 0) &&
		(core->current_sensor_id == core->aov_sensor_id)) {
		g_aov_param.width = width;
		g_aov_param.height = height;
		g_aov_param.hblank = hblank;
		g_aov_param.vblank = vblank;
		g_aov_param.fps_n = ctx->fps_n;
		g_aov_param.fps_d = ctx->fps_d;
	}
#endif

	return calc_buffered_pixel_rate(ctx->dev, width, height, hblank, vblank,
					ctx->fps_n, ctx->fps_d, result);
}

static int get_customized_pixel_rate(struct seninf_ctx *ctx, struct v4l2_subdev *sd,
			  s64 *result)
{
	struct v4l2_ctrl *ctrl;
#if AOV_GET_PARAM
	struct seninf_core *core = ctx->core;
#endif

	ctrl = v4l2_ctrl_find(sd->ctrl_handler, V4L2_CID_MTK_CUST_SENSOR_PIXEL_RATE);
	if (!ctrl) {
		dev_info(ctx->dev, "no cust pixel rate in subdev %s\n", sd->name);
		return -EINVAL;
	}

	*result = v4l2_ctrl_g_ctrl(ctrl);

#if AOV_GET_PARAM
	if (!(core->aov_sensor_id < 0) &&
		!(core->current_sensor_id < 0) &&
		(core->current_sensor_id == core->aov_sensor_id))
		g_aov_param.customized_pixel_rate = *result;
#endif

	return 0;
}

static int get_pixel_rate(struct seninf_ctx *ctx, struct v4l2_subdev *sd,
			  s64 *result)
{
	struct v4l2_ctrl *ctrl;
#if AOV_GET_PARAM
	struct seninf_core *core = ctx->core;
#endif

	ctrl = v4l2_ctrl_find(sd->ctrl_handler, V4L2_CID_PIXEL_RATE);
	if (!ctrl) {
		dev_info(ctx->dev, "no pixel rate in subdev %s\n", sd->name);
		return -EINVAL;
	}

	*result = v4l2_ctrl_g_ctrl_int64(ctrl);

#if AOV_GET_PARAM
	if (!(core->aov_sensor_id < 0) &&
		!(core->current_sensor_id < 0) &&
		(core->current_sensor_id == core->aov_sensor_id))
		g_aov_param.mipi_pixel_rate = *result;
#endif

	return 0;
}

static int get_mbus_config(struct seninf_ctx *ctx, struct v4l2_subdev *sd)
{
	struct v4l2_mbus_config cfg = {0};
	int ret;
#if AOV_GET_PARAM
	struct seninf_core *core = ctx->core;
#endif

	ret = v4l2_subdev_call(sd, pad, get_mbus_config, ctx->sensor_pad_idx, &cfg);
	if (ret) {
		dev_info(ctx->dev, "no g_mbus_config in %s\n",
			 sd->entity.name);
		return -1;
	}

	ctx->is_cphy = cfg.type == V4L2_MBUS_CSI2_CPHY;
	ctx->num_data_lanes = cfg.bus.mipi_csi2.num_data_lanes;

#if AOV_GET_PARAM
	if (!(core->aov_sensor_id < 0) &&
		!(core->current_sensor_id < 0) &&
		(core->current_sensor_id == core->aov_sensor_id)) {
		g_aov_param.is_cphy = ctx->is_cphy;
		g_aov_param.num_data_lanes = ctx->num_data_lanes;
	}
#endif

	dev_info(ctx->dev, "%s num_data_lanes %d\n", __func__, ctx->num_data_lanes);

	return 0;
}

//static int update_isp_clk(struct seninf_ctx *ctx)
int update_isp_clk(struct seninf_ctx *ctx)
{
	int i, ret;
	struct seninf_dfs *dfs = &ctx->core->dfs;
	struct seninf_core *core = ctx->core;

#ifndef USING_MAX_ISP_CLK
	int pixelmode;
	s64 pixel_rate = -1, dfs_freq;
	struct seninf_vc *vc;
#endif

	if (core == NULL) {
		dev_info(dfs->dev, "%s: core is NULL\n", __func__);
		return -EINVAL;
	}

	if (!ctx->allow_adjust_isp_en) {
		dev_info(ctx->dev, "%s adjust_isp_en %d, skip update isp clk flow\n",
			__func__, ctx->allow_adjust_isp_en);
		return 0;
	}

	if (!dfs->cnt) {
		dev_info(dfs->dev, "%s: dfs->cnt is NULL (can be ignore)\n", __func__);
		return 0;
	}


#ifdef USING_MAX_ISP_CLK
	/* always choose the highest freq index */
	i = dfs->cnt - 1;

#else
	vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
	if (!vc) {
		dev_info(ctx->dev, "failed to get vc SRC_RAW0, try EXT0\n");

		vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);
		if (!vc) {
			dev_info(ctx->dev, "failed to get vc SRC_RAW_EXT0\n");

			return -EINVAL;
		}
	}
	dev_info(ctx->dev,
		"%s dfs->cnt %d pixel mode %d customized_pixel_rate %lld, buffered_pixel_rate %lld mipi_pixel_rate %lld\n",
		__func__,
		dfs->cnt,
		vc->pixel_mode,
		ctx->customized_pixel_rate,
		ctx->buffered_pixel_rate,
		ctx->mipi_pixel_rate);
	//Use SensorPixelrate
	if (ctx->customized_pixel_rate)
		pixel_rate = ctx->customized_pixel_rate;
	else if (ctx->buffered_pixel_rate)
		pixel_rate = ctx->buffered_pixel_rate;
	else if (ctx->mipi_pixel_rate)
		pixel_rate = ctx->mipi_pixel_rate;
	else {
		dev_info(ctx->dev, "failed to get pixel_rate\n");
		return -EINVAL;
	}

	pixelmode = vc->pixel_mode;

	for (i = 0; i < dfs->cnt; i++) {
		dfs_freq = dfs->freqs[i];
		dfs_freq = dfs_freq * (100 - SENINF_CLK_MARGIN_IN_PERCENT);
		do_div(dfs_freq, 100);
		if ((dfs_freq << pixelmode) >= pixel_rate)
			break;
	}
#endif

	if (i == dfs->cnt) {
		dev_info(ctx->dev, "mux is overrun. please adjust pixelmode\n");
		return -EINVAL;
	}

	ret = seninf_dfs_set(ctx, dfs->freqs[i]);
	if (ret) {
		dev_info(ctx->dev, "failed to set freq\n");
		return -EINVAL;
	}

	return 0;
}

static int debug_err_detect_initialize(struct seninf_ctx *ctx)
{
	struct seninf_core *core;
	struct seninf_ctx *ctx_;

	core = dev_get_drvdata(ctx->dev->parent);

	core->csi_irq_en_flag = 0;
	core->vsync_irq_en_flag = 0;
	core->vsync_irq_detect_csi_irq_error_flag = 0;
	core->err_detect_termination_flag = 1;

	list_for_each_entry(ctx_, &core->list, list) {
		ctx_->data_not_enough_flag = 0;
		ctx_->err_lane_resync_flag = 0;
		ctx_->crc_err_flag = 0;
		ctx_->ecc_err_double_flag = 0;
		ctx_->ecc_err_corrected_flag = 0;
		ctx_->fifo_overrun_flag = 0;
		ctx_->size_err_flag = 0;
		ctx_->data_not_enough_cnt = 0;
		ctx_->err_lane_resync_cnt = 0;
		ctx_->crc_err_cnt = 0;
		ctx_->ecc_err_double_cnt = 0;
		ctx_->ecc_err_corrected_cnt = 0;
		ctx_->fifo_overrun_cnt = 0;
		ctx_->size_err_cnt = 0;
#ifdef ERR_DETECT_TEST
		ctx_->test_cnt = 0;
#endif
		/* debug dump cur status initialize */
		ctx->debug_cur_sys_time_in_ns = 0;
		ctx->debug_cur_dphy_irq = 0;
		ctx->debug_cur_cphy_irq = 0;
		ctx->debug_cur_mac_irq = 0;
		ctx->debug_cur_temp = 0;
		ctx->debug_cur_seninf_irq = 0;
		ctx->debug_cur_mac_csi2_size_chk_ctrl0 = 0;
		ctx->debug_cur_mac_csi2_size_chk_ctrl1 = 0;
		ctx->debug_cur_mac_csi2_size_chk_ctrl2 = 0;
		ctx->debug_cur_mac_csi2_size_chk_ctrl3 = 0;
		ctx->debug_cur_mac_csi2_size_chk_ctrl4 = 0;
		ctx->debug_cur_mac_csi2_size_chk_rcv0 = 0;
		ctx->debug_cur_mac_csi2_size_chk_rcv1 = 0;
		ctx->debug_cur_mac_csi2_size_chk_rcv2 = 0;
		ctx->debug_cur_mac_csi2_size_chk_rcv3 = 0;
		ctx->debug_cur_mac_csi2_size_chk_rcv4 = 0;
	}

	return 0;
}

static int seninf_csi_s_stream(struct v4l2_subdev *sd, int enable)
{
#ifdef SENSOR_SECURE_MTEE_SUPPORT
	u32 ret_gz;
#endif
	int ret;
	struct seninf_ctx *ctx = sd_to_ctx(sd);
#if AOV_GET_PARAM
	struct seninf_core *core = ctx->core;
#endif

	if (ctx->csi_streaming == enable) {
		dev_info(ctx->dev,
			"[%s] is_csi_streaming is (%d) already, return\n",
			__func__, ctx->csi_streaming);
		return 0;
	}

	if (ctx->is_test_model)
		return 0; // skip

	if (!ctx->sensor_sd) {
		dev_info(ctx->dev, "[%s] no sensor\n", __func__);
		return -EFAULT;
	}

	dev_info(ctx->dev, "[%s] enable(%d)\n", __func__, enable);

	if (ctx->is_aov_real_sensor && !enable) {
		if (!core->pwr_refcnt_for_aov)
			dev_info(ctx->dev,
				"[%s] aov real sensor streaming off by aov MW on apmcu side\n",
				__func__);
		else {
			dev_info(ctx->dev,
				"[%s] streaming off by camsys, but aov real sensor still streaming on scp side\n",
				__func__);
			return 0;
		}
	}

	if (enable) {
		if (!(core->err_detect_init_flag))
			debug_err_detect_initialize(ctx);
		get_mbus_config(ctx, ctx->sensor_sd);

		get_pixel_rate(ctx, ctx->sensor_sd, &ctx->mipi_pixel_rate);

		ctx->buffered_pixel_rate = ctx->mipi_pixel_rate;
		get_buffered_pixel_rate(ctx, ctx->sensor_sd,
					ctx->sensor_pad_idx, &ctx->buffered_pixel_rate);

		get_customized_pixel_rate(ctx, ctx->sensor_sd, &ctx->customized_pixel_rate);
		ret = pm_runtime_get_sync(ctx->dev);
		if (ret < 0) {
			dev_info(ctx->dev, "%s pm_runtime_get_sync ret %d\n", __func__, ret);
			pm_runtime_put_noidle(ctx->dev);
			return ret;
		}

		update_isp_clk(ctx);
#if AOV_GET_PARAM
		if (!(core->aov_sensor_id < 0) &&
			!(core->current_sensor_id < 0) &&
			(core->current_sensor_id == core->aov_sensor_id))
			g_aov_param.isp_freq = ISP_CLK_LOW;
#endif
		ret = config_hw_csi(ctx);
		if (ret) {
			dev_info(ctx->dev, "config_seninf_hw ret %d\n", ret);
			return ret;
		}

#ifdef SENINF_VC_ROUTING
		//update_sensor_frame_desc(ctx);
#endif

	} else {
#ifdef SENSOR_SECURE_MTEE_SUPPORT
		if (ctx->is_secure == 1) {
			dev_info(ctx->dev, "sensor kernel ca_free");
			seninf_ca_free();

			dev_info(ctx->dev, "close seninf_ca");
			ret_gz = seninf_ca_close_session();
			ctx->is_secure = 0;
		}
#endif
		g_seninf_ops->_set_idle(ctx);
		mtk_cam_seninf_release_mux(ctx);
		mtk_cam_seninf_tsrec_n_reset(ctx->seninfIdx);
		seninf_dfs_set(ctx, 0);
		g_seninf_ops->_poweroff(ctx);
		ctx->dbg_last_dump_req = 0;
		pm_runtime_put_sync(ctx->dev);
	}

	ctx->csi_streaming = enable;
	return 0;
}

static int stream_sensor(struct seninf_ctx *ctx, bool enable)
{
	int ret;

	ret = v4l2_subdev_call(ctx->sensor_sd, video, s_stream, enable);
	if (ret) {
		dev_info(ctx->dev, "%s sensor stream-%s fail,ret(%d)\n",
			 __func__,
			 enable ? "on" : "off",
			 ret);
	} else {
#ifdef SENINF_UT_DUMP
		g_seninf_ops->_debug(ctx);
#endif
	}

	return ret;
}

static int seninf_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct seninf_ctx *ctx = sd_to_ctx(sd);
	struct seninf_core *core = ctx->core;
	int i;
	bool pad_inited = false;
#ifdef INIT_DESKEW_DEBUG
	int deskew_dump_idx;
#endif /*INIT_DESKEW_DEBUG*/

	/* get current sensor idx by get_sensor_idx */
	core->current_sensor_id = get_sensor_idx(ctx);
	if (core->current_sensor_id < 0) {
		dev_info(ctx->dev,
			"[%s] get_sensor_idx[%d] fail\n",
			__func__, core->current_sensor_id);
		return core->current_sensor_id;
	}

	if (core->aov_abnormal_init_flag) {
		ctx->is_aov_real_sensor = 1;
		/* switch aov pm ops: force power off sensor */
		aov_switch_pm_ops(ctx, AOV_ABNORMAL_FORCE_SENSOR_PWR_ON);
		core->aov_abnormal_init_flag = 0;
	}

	seninf_csi_s_stream(sd, enable);

	/* avoid stream on when pad2cam is not set, it will cont stream on after set camtg */
	if (enable && !ctx->is_aov_real_sensor) {
		for (i = 0; i < PAD_MAXCNT; i++) {
			if (ctx->pad2cam[i][0] != 0xff) {
				pad_inited = true;
				break;
			}
		}
		if (!pad_inited) {
			dev_info(ctx->dev,
				 "[%s] pad_inited(%d)\n", __func__, pad_inited);
			// stream on sensor while csi streamed
			stream_sensor(ctx, enable);
			return 0;
		}
	}

	if (ctx->streaming == enable) {
		dev_info(ctx->dev,
			"[%s] is_ctx_streaming(%d)\n",
			__func__, ctx->streaming);
		if (!enable) {
			// ensure forget cammux setting
			mtk_cam_seninf_forget_camtg_setting(ctx);
		}
		return 0;
	}

	dev_info(ctx->dev, "[%s] enable(%d)\n", __func__, enable);

	if (ctx->is_test_model)
		return set_test_model(ctx, enable);

	if (ctx->is_aov_real_sensor && !enable) {
		if (!core->pwr_refcnt_for_aov)
			dev_info(ctx->dev,
				"[%s] aov real sensor streaming off by aov MW on apmcu side\n",
				__func__);
		else {
			dev_info(ctx->dev,
				"[%s] streaming off by camsys, but aov real sensor still streaming on scp side\n",
				__func__);
			return 0;
		}
	}

	if (enable && !ctx->streaming) {
		if (unlikely(*(ctx->core->seninf_vsync_debug_flag))) {
			ctx->core->vsync_irq_en_flag = 1;
			g_seninf_ops->_set_all_cam_mux_vsync_irq(ctx, 1);
			dev_info(ctx->dev, "vsync irq enabled\n");
		}
		mtk_cam_seninf_s_stream_mux(ctx);
		// notify_fsync_listen_target(ctx);
	}

#ifdef INIT_DESKEW_DEBUG
	dev_info(ctx->dev, "[%s]dump after deskew config before stream on\n", __func__);
	g_seninf_ops->_debug_init_deskew_begin_end_apply_code(ctx);
#endif /*INIT_DESKEW_DEBUG*/

	// stream on sensor after mux set
	stream_sensor(ctx, enable);

#ifdef INIT_DESKEW_DEBUG
	//read initdeskew irq
	for(deskew_dump_idx = 0; deskew_dump_idx < 100; deskew_dump_idx++ ) {
		g_seninf_ops->_debug_init_deskew_irq(ctx);
		mdelay(1);
	}
	dev_info(ctx->dev, "[%s]dump after deskew config after stream on\n", __func__);
	g_seninf_ops->_debug_init_deskew_begin_end_apply_code(ctx);

#endif /*INIT_DESKEW_DEBUG*/
	ctx->streaming = enable;
	notify_fsync_listen_target_with_kthread(ctx, 2);

	if (core->aov_abnormal_deinit_flag) {
		ctx->is_aov_real_sensor = 0;
		if (!core->pwr_refcnt_for_aov &&
			core->aov_abnormal_deinit_usr_fd_kill_flag) {
			dev_info(ctx->dev,
				"[%s] set aov real sensor off\n", __func__);
			/* array size of aov_ctx[] is
			 * AOV_SENINF_NUM: most number of sensors support
			 */
			if (g_aov_param.sensor_idx < AOV_SENINF_NUM) {
				aov_ctx[g_aov_param.sensor_idx] = NULL;
				memset(&g_aov_param, 0,
					sizeof(struct mtk_seninf_aov_param));
			}
			core->aov_abnormal_deinit_usr_fd_kill_flag = 0;
		}
		/* switch aov pm ops: force power off sensor */
		aov_switch_pm_ops(ctx, AOV_ABNORMAL_FORCE_SENSOR_PWR_OFF);
		core->aov_abnormal_deinit_flag = 0;
	}

	if (enable && core->err_detect_init_flag) {
		dev_info(ctx->dev, "[%s] _enable_stream_err_detect enable(%d)\n", __func__, enable);
		g_seninf_ops->_enable_stream_err_detect(ctx);
	}

	/* reset all sentest flag */
	ctx->allow_adjust_isp_en = false;
	ctx->single_raw_streaming_en = false;

	return 0;
}

static int s_update_isp_clk_en(struct seninf_ctx *ctx, void *arg)
{
	int *en = arg;

	if (en == NULL) {
		dev_info(ctx->dev, "%s: en is NULL\n", __func__);
		return -EINVAL;
	}

	ctx->allow_adjust_isp_en = *en;

	dev_info(ctx->dev, "en: %d, allow_adjust_isp_en  is %d\n",
				*en, ctx->allow_adjust_isp_en);

	return 0;
}

static int s_single_raw_setreaming_en(struct seninf_ctx *ctx, void *arg)
{
	int *en = arg;

	if (en == NULL) {
		dev_info(ctx->dev, "%s: en is NULL\n", __func__);
		return -EINVAL;
	}

	ctx->single_raw_streaming_en = *en;

	dev_info(ctx->dev, "en: %d, single_raw_streaming_en  is %d\n",
				*en, ctx->single_raw_streaming_en);

	return 0;
}

long mtk_cam_seninf_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = -ENOIOCTLCMD;
	struct seninf_ctx *ctx = sd_to_ctx(sd);

	/* dispatch ioctl request */
	switch (cmd) {
	case VIDIOC_MTK_G_SENINF_DEBUG_RESULT:
		ret = g_seninf_ops->_get_debug_reg_result(ctx, arg);
		break;
	case VIDIOC_MTK_G_TSREC_TIMESTAMP:
		ret = g_seninf_ops->_get_tsrec_timestamp(ctx, arg);
		break;
	case VIDIOC_MTK_S_UPDATE_ISP_EN:
		ret = s_update_isp_clk_en(ctx, arg);
		break;
	case VIDIOC_MTK_S_TEST_STREAM_RAW0_EN:
		ret = s_single_raw_setreaming_en(ctx, arg);
		break;
	default:
		dev_info(ctx->dev, "ioctl cmd(%d) is invalid\n", cmd);
		break;
	}

	return ret;
}

static const struct v4l2_subdev_pad_ops seninf_subdev_pad_ops = {
	.link_validate = mtk_cam_seninf_link_validate,
#if (KERNEL_VERSION(6, 7, 0) >= LINUX_VERSION_CODE)
	.init_cfg = mtk_cam_seninf_init_cfg,
#endif
	.set_fmt = mtk_cam_seninf_set_fmt,
	.get_fmt = mtk_cam_seninf_get_fmt,
};

static const struct v4l2_subdev_video_ops seninf_subdev_video_ops = {
	.s_stream = seninf_s_stream,
};

static const struct v4l2_subdev_core_ops seninf_subdev_core_ops = {
	.subscribe_event	= seninf_subscribe_event,
	.unsubscribe_event	= v4l2_event_subdev_unsubscribe,
	.ioctl = mtk_cam_seninf_ioctl,
};

static const struct v4l2_subdev_ops seninf_subdev_ops = {
	.core	= &seninf_subdev_core_ops,
	.video	= &seninf_subdev_video_ops,
	.pad	= &seninf_subdev_pad_ops,
};

static int seninf_link_setup(struct media_entity *entity,
			     const struct media_pad *local,
				 const struct media_pad *remote, u32 flags)
{
	struct v4l2_subdev *sd;
	struct seninf_ctx *ctx;

	sd = media_entity_to_v4l2_subdev(entity);
	if (sd != NULL) {
		ctx = v4l2_get_subdevdata(sd);

		if (local->flags & MEDIA_PAD_FL_SOURCE) {
			if (flags & MEDIA_LNK_FL_ENABLED) {
				if (!mtk_cam_seninf_get_vc_by_pad(ctx, local->index)) {
					dev_info(ctx->dev,
						"%s enable link w/o vc_info pad idex %d\n",
						__func__, local->index);
				}
			}
		} else {
			/* NOTE: update vcinfo once the link becomes enabled */
			if (flags & MEDIA_LNK_FL_ENABLED) {
				ctx->sensor_sd =
					media_entity_to_v4l2_subdev(remote->entity);
				ctx->sensor_pad_idx = remote->index;
				mtk_cam_seninf_get_vcinfo(ctx);
			}
		}
	}

	return 0;
}

static const struct media_entity_operations seninf_media_ops = {
	.link_setup = seninf_link_setup,
	.link_validate = v4l2_subdev_link_validate,
};

static int seninf_notifier_bound(struct v4l2_async_notifier *notifier,
				 struct v4l2_subdev *sd,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
					 struct v4l2_async_subdev *asd)
#else
					 struct v4l2_async_connection *asd)
#endif
{
	struct seninf_ctx *ctx = notifier_to_ctx(notifier);
	int ret;

	dev_info(ctx->dev, "%s bounded\n", sd->entity.name);

	mutex_lock(&ctx->subdev.v4l2_dev->mdev->graph_mutex);
	ret = media_create_pad_link(&sd->entity, 0,
				    &ctx->subdev.entity, 0,
				    MEDIA_LNK_FL_DYNAMIC);
	if (ret) {
		dev_info(ctx->dev,
			"failed to create link for %s\n",
			sd->entity.name);
		mutex_unlock(&ctx->subdev.v4l2_dev->mdev->graph_mutex);
		return ret;
	}

	ret = v4l2_device_register_subdev_nodes(ctx->subdev.v4l2_dev);
	if (ret) {
		dev_info(ctx->dev, "failed to create subdev nodes\n");
		mutex_unlock(&ctx->subdev.v4l2_dev->mdev->graph_mutex);
		return ret;
	}
	mutex_unlock(&ctx->subdev.v4l2_dev->mdev->graph_mutex);
	dev_info(ctx->dev, "%s bounded exit\n", sd->entity.name);

	return 0;
}

static void seninf_notifier_unbind(struct v4l2_async_notifier *notifier,
				   struct v4l2_subdev *sd,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
					   struct v4l2_async_subdev *asd)
#else
					   struct v4l2_async_connection *asd)
#endif
{
	struct seninf_ctx *ctx = notifier_to_ctx(notifier);

	dev_info(ctx->dev, "%s is unbounded\n", sd->entity.name);
}

static const struct v4l2_async_notifier_operations seninf_async_ops = {
	.bound = seninf_notifier_bound,
	.unbind = seninf_notifier_unbind,
};

static int seninf_real_sensor_for_aov_param(struct seninf_ctx *ctx, u32 enable)
{
#ifndef SENSING_MODE_READY
	struct seninf_core *core = ctx->core;

	switch (enable) {
	case 0:
		if (ctx->streaming)
			return -EBUSY;
		ctx->is_aov_real_sensor = 0;
		if (!core->pwr_refcnt_for_aov) {
			dev_info(ctx->dev, "[%s] set aov real sensor off\n", __func__);
			/* because array size of aov_ctx[] is
			 * AOV_SENINF_NUM: most number of sensors support
			 */
			if (g_aov_param.sensor_idx < AOV_SENINF_NUM) {
				aov_ctx[g_aov_param.sensor_idx] = NULL;
				memset(&g_aov_param, 0, sizeof(struct mtk_seninf_aov_param));
			}
		}
		break;
	case 1:// RAW only
		if (ctx->streaming)
			return -EBUSY;
		ctx->is_aov_real_sensor = enable;
		if (!core->pwr_refcnt_for_aov) {
			dev_info(ctx->dev, "[%s] set aov real sensor on\n", __func__);
			/* get aov sensor idx by get_sensor_idx */
			core->aov_sensor_id = get_sensor_idx(ctx);
			/* array size of aov_ctx[] is
			 * AOV_SENINF_NUM: most number of sensors support
			 */
			if (core->aov_sensor_id >= 0 &&
				core->aov_sensor_id < AOV_SENINF_NUM) {
				g_aov_param.sensor_idx = core->aov_sensor_id;
				aov_ctx[g_aov_param.sensor_idx] = ctx;
				g_aov_param.is_test_model = 0;
			} else {
				dev_info(ctx->dev,
					"[%s] get_sensor_idx[%d] fail\n",
					__func__, core->aov_sensor_id);
				return core->aov_sensor_id;
			}
		}
		break;
	default:
		break;
	}
#endif
	return 0;
}

/* NOTE: update vcinfo once test_model switches */
static int seninf_test_pattern_for_aov_param(struct seninf_ctx *ctx, u32 pattern)
{
#ifndef SENSING_MODE_READY
	switch (pattern) {
	case 0:
		if (ctx->streaming)
			return -EBUSY;
		ctx->is_aov_test_model = 0;
		dev_info(ctx->dev, "set aov test pattern off\n");
		break;
	case 1:// RAW only
		if (ctx->streaming)
			return -EBUSY;
		ctx->is_aov_test_model = pattern;
		dev_info(ctx->dev, "set aov test pattern on\n");
		break;
	default:
		break;
	}
#endif
	return 0;
}

static int seninf_test_pattern(struct seninf_ctx *ctx, u32 pattern)
{
	switch (pattern) {
	case 0:
		if (ctx->streaming)
			return -EBUSY;
		ctx->is_test_model = 0;
		mtk_cam_seninf_get_vcinfo(ctx);
		dev_info(ctx->dev, "test pattern off\n");
		break;
	case 1:// RAW only
	case 2:// Stagger: 3 expo
	case 3:// 1 RAW + 1 PD
	case 4:// 3 RAW + 2 PD
	case 5:// 1 RAW + 1 W channel
		if (ctx->streaming)
			return -EBUSY;
		ctx->is_test_model = pattern;
		mtk_cam_seninf_get_vcinfo_test(ctx);
		dev_info(ctx->dev, "test pattern on\n");
		break;
	default:
		break;
	}

	return 0;
}

#ifdef SENINF_DEBUG
static int seninf_test_streamon(struct seninf_ctx *ctx, u32 en)
{
#ifdef SECURE_UT
	ctx->is_secure = 1;
	ctx->SecInfo_addr = 0x53;
#endif
	if (en) {
		ctx->is_test_streamon = 1;
		mtk_cam_seninf_alloc_cammux(ctx);
		seninf_s_stream(&ctx->subdev, 1);
	} else {
		seninf_s_stream(&ctx->subdev, 0);
		mtk_cam_seninf_release_cam_mux(ctx);
		ctx->is_test_streamon = 0;
	}

	return 0;
}
#endif

static int mtk_cam_seninf_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct seninf_ctx *ctx = ctrl_hdl_to_ctx(ctrl->handler);
	struct mtk_seninf_s_stream *s_stream_ctrl = ctrl->p_new.p;
	int ret = 0;
	struct seninf_core *core = ctx->core;

	switch (ctrl->id) {
	case V4L2_CID_TEST_PATTERN:
		ret = seninf_test_pattern(ctx, ctrl->val);
		break;
	case V4L2_CID_MTK_SENINF_S_STREAM:
		/* get current sensor idx by get_sensor_idx */
		core->current_sensor_id = get_sensor_idx(ctx);
		if (core->current_sensor_id < 0) {
			dev_info(ctx->dev,
				"[%s] get_sensor_idx[%d] fail\n",
				__func__, core->current_sensor_id);
			return core->current_sensor_id;
		}
		switch (s_stream_ctrl->stream_mode) {
		case AOV_TEST_MODEL:
			if (s_stream_ctrl->enable) {
				if (ctx->streaming) {
					dev_info(ctx->dev,
						"[%s] sensor still streaming on!\n", __func__);
					return -EBUSY;
				}
				ctx->is_test_model = 1;
				ctx->is_aov_test_model = 1;
				dev_info(ctx->dev, "set aov test pattern on\n");
				ret = seninf_s_stream(&ctx->subdev, s_stream_ctrl->enable);
			} else {
				ret = seninf_s_stream(&ctx->subdev, s_stream_ctrl->enable);
				if (ctx->streaming) {
					dev_info(ctx->dev,
						"[%s] sensor still streaming on!\n", __func__);
					return -EBUSY;
				}
				ctx->is_test_model = 0;
				ctx->is_aov_test_model = 0;
				dev_info(ctx->dev, "set aov test pattern off\n");
			}
			break;
		case AOV_REAL_SENSOR:
			if (s_stream_ctrl->enable) {
				if (ctx->streaming) {
					dev_info(ctx->dev,
						"[%s] sensor still streaming on!\n", __func__);
					return -EBUSY;
				}
				ctx->is_aov_real_sensor = 1;
				if (!core->pwr_refcnt_for_aov) {
					dev_info(ctx->dev,
						"[%s] set aov real sensor on\n", __func__);
					/* get aov sensor idx by get_sensor_idx */
					core->aov_sensor_id = get_sensor_idx(ctx);
					/* array size of aov_ctx[] is
					 * AOV_SENINF_NUM: most number of sensors support
					 */
					if (core->aov_sensor_id >= 0 &&
						core->aov_sensor_id < AOV_SENINF_NUM) {
						g_aov_param.sensor_idx = core->aov_sensor_id;
						aov_ctx[g_aov_param.sensor_idx] = ctx;
						g_aov_param.is_test_model = 0;
					} else {
						dev_info(ctx->dev,
							"[%s] get_sensor_idx[%d] fail\n",
							__func__, core->aov_sensor_id);
						return core->aov_sensor_id;
					}
				}
				if (core->current_sensor_id == core->aov_sensor_id)
					ret = seninf_s_stream(&ctx->subdev, s_stream_ctrl->enable);
				else
					dev_info(ctx->dev,
						"[%s] aov user input wrong sensor id!\n", __func__);
			} else {
				if (core->current_sensor_id == core->aov_sensor_id)
					ret = seninf_s_stream(&ctx->subdev, s_stream_ctrl->enable);
				else
					dev_info(ctx->dev,
						"[%s] aov user input wrong sensor id!\n", __func__);
				if (ctx->streaming) {
					dev_info(ctx->dev,
						"[%s] sensor still streaming on!\n", __func__);
					return -EBUSY;
				}
				ctx->is_aov_real_sensor = 0;
				if (!core->pwr_refcnt_for_aov) {
					dev_info(ctx->dev,
						"[%s] set aov real sensor off\n", __func__);
					/* array size of aov_ctx[] is
					 * AOV_SENINF_NUM: most number of sensors support
					 */
					if (g_aov_param.sensor_idx < AOV_SENINF_NUM) {
						aov_ctx[g_aov_param.sensor_idx] = NULL;
						memset(&g_aov_param, 0,
							sizeof(struct mtk_seninf_aov_param));
					}
				}
			}
			break;
		case NORMAL_CAMERA:
		default:
			ret = seninf_s_stream(&ctx->subdev, s_stream_ctrl->enable);
			break;
		}
		break;
#ifdef SENINF_DEBUG
	case V4L2_CID_MTK_TEST_STREAMON:
		ret = seninf_test_streamon(ctx, ctrl->val);
		break;
#endif
	case V4L2_CID_TEST_PATTERN_FOR_AOV_PARAM:
		ret = seninf_test_pattern_for_aov_param(ctx, ctrl->val);
		break;

	case V4L2_CID_REAL_SENSOR_FOR_AOV_PARAM:
		ret = seninf_real_sensor_for_aov_param(ctx, ctrl->val);
		break;
	default:
		ret = 0;
		dev_info(ctx->dev, "%s Unhandled id:0x%x, val:0x%x\n",
			 __func__, ctrl->id, ctrl->val);
		break;
	}

	return ret;
}

static int mtk_cam_seninf_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct seninf_ctx *ctx = ctrl_hdl_to_ctx(ctrl->handler);
	int ret;

	switch (ctrl->id) {
	case V4L2_CID_GET_CSI2_IRQ_STATUS:
		ret = mtk_cam_seninf_get_csi_irq_status(&ctx->subdev, ctrl);
		break;
	default:
		ret = 0;
		dev_info(ctx->dev, "%s Unhandled id:0x%x\n",
			 __func__, ctrl->id);
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops seninf_ctrl_ops = {
	.s_ctrl = mtk_cam_seninf_set_ctrl,
	.g_volatile_ctrl = mtk_cam_seninf_g_volatile_ctrl,
};

static int seninf_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct seninf_ctx *ctx = sd_to_ctx(sd);
	struct seninf_core *core;

	core = dev_get_drvdata(ctx->dev->parent);

	mutex_lock(&ctx->mutex);
	ctx->open_refcnt++;

	if (ctx->open_refcnt == 1) {
		dev_info(ctx->dev, "%s open_refcnt %d\n", __func__, ctx->open_refcnt);
		ctx->pid = find_get_pid(current->pid);
	}

	mutex_unlock(&ctx->mutex);

	return 0;
}

static int seninf_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct seninf_ctx *ctx = sd_to_ctx(sd);
	unsigned int i;

	mutex_lock(&ctx->mutex);
	ctx->open_refcnt--;
	ctx->is_aov_real_sensor = 0;

	if (!ctx->open_refcnt) {
		dev_info(ctx->dev, "%s open_refcnt %d\n", __func__, ctx->open_refcnt);

		/* clear aov_ctx */
		for (i = 0; i < AOV_SENINF_NUM; i++) {
			if (aov_ctx[i] == ctx) {
				aov_ctx[i] = NULL;
				dev_info(ctx->dev, "%s clear aov_ctx[%u]\n", __func__, i);
			}
		}
#ifdef SENINF_DEBUG
		if (ctx->is_test_streamon)
			seninf_test_streamon(ctx, 0);
		else if (ctx->streaming)
#else
		if (ctx->streaming)
#endif
			seninf_s_stream(&ctx->subdev, 0);
		else if (ctx->csi_streaming)
			seninf_csi_s_stream(&ctx->subdev, 0);

		if (ctx->pid) {
			put_pid(ctx->pid);
			ctx->pid = NULL;
		}
	}

	mutex_unlock(&ctx->mutex);

	return 0;
}

static const struct v4l2_subdev_internal_ops seninf_internal_ops = {
	.open = seninf_open,
	.close = seninf_close,
#if (KERNEL_VERSION(6, 7, 0) < LINUX_VERSION_CODE)
	.init_state = mtk_cam_seninf_init_cfg,
#endif
};

static const char * const seninf_test_pattern_menu[] = {
	"Disabled",
	"generate_test_pattern",
	"generate_test_pattern_stagger",
	"generate_test_pattern_pd",
	"generate_test_pattern_5_src_pad",
	"generate_test_pattern_raw_and_w",
};

#ifdef SENINF_DEBUG
static const struct v4l2_ctrl_config cfg_test_streamon = {
	.ops = &seninf_ctrl_ops,
	.id = V4L2_CID_MTK_TEST_STREAMON,
	.name = "test_streamon",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.max = 1,
	.step = 1,
};
#endif

static const struct v4l2_ctrl_config cfg_s_stream = {
	.ops = &seninf_ctrl_ops,
	.id = V4L2_CID_MTK_SENINF_S_STREAM,
	.name = "set_stream",
	.type = V4L2_CTRL_TYPE_U32,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.max = 0xffffffff,
	.step = 1,
	.dims = {sizeof_u32(struct mtk_seninf_s_stream)},
};

static const struct v4l2_ctrl_config cfg_s_test_model_for_aov_param = {
	.ops = &seninf_ctrl_ops,
	.id = V4L2_CID_TEST_PATTERN_FOR_AOV_PARAM,
	.name = "set_test_model_for_aov_param",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.max = 1,
	.step = 1,
};

static const struct v4l2_ctrl_config cfg_s_real_sensor_for_aov_param = {
	.ops = &seninf_ctrl_ops,
	.id = V4L2_CID_REAL_SENSOR_FOR_AOV_PARAM,
	.name = "set_real_sensor_for_aov_param",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.max = 1,
	.step = 1,
};

static const struct v4l2_ctrl_config cfg_g_csi2_irq_status = {
	.ops = &seninf_ctrl_ops,
	.id = V4L2_CID_GET_CSI2_IRQ_STATUS,
	.name = "get_csi2_irq_status",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_READ_ONLY|V4L2_CTRL_FLAG_VOLATILE,
	.max = 0x7fffffff,
	.step = 1,
};

static int seninf_initialize_controls(struct seninf_ctx *ctx)
{
	struct v4l2_ctrl_handler *handler;
	struct v4l2_ctrl *test_pattern;
	int ret;

	handler = &ctx->ctrl_handler;
	ret = v4l2_ctrl_handler_init(handler, 2);
	if (ret)
		return ret;
	test_pattern =
	v4l2_ctrl_new_std_menu_items(handler, &seninf_ctrl_ops,
				     V4L2_CID_TEST_PATTERN,
		ARRAY_SIZE(seninf_test_pattern_menu) - 1,
		0, 0, seninf_test_pattern_menu);

#ifdef SENINF_DEBUG
	v4l2_ctrl_new_custom(handler, &cfg_test_streamon, NULL);
#endif
	v4l2_ctrl_new_custom(handler, &cfg_s_stream, NULL);

	v4l2_ctrl_new_custom(handler, &cfg_s_test_model_for_aov_param, NULL);
	v4l2_ctrl_new_custom(handler, &cfg_s_real_sensor_for_aov_param, NULL);
	v4l2_ctrl_new_custom(handler, &cfg_g_csi2_irq_status, NULL);

	if (handler->error) {
		ret = handler->error;
		dev_info(ctx->dev,
			"Failed to init controls(%d)\n", ret);
		goto err_free_handler;
	}

	ctx->subdev.ctrl_handler = handler;
	return 0;

err_free_handler:
	v4l2_ctrl_handler_free(handler);

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static int seninf_parse_fwnode(struct device *dev, struct v4l2_async_notifier *notifier)
{
	struct fwnode_handle *fwnode;
	int ret = 0;

	fwnode_graph_for_each_endpoint(dev_fwnode(dev), fwnode) {
		struct v4l2_async_connection *s_asc;
		struct fwnode_handle *dev_fwnode;
		bool is_available;

		dev_fwnode = fwnode_graph_get_port_parent(fwnode);
		if (!dev_fwnode)
			continue;

		is_available = fwnode_device_is_available(dev_fwnode);
		fwnode_handle_put(dev_fwnode);
		if (!is_available)
			continue;

		s_asc = v4l2_async_nf_add_fwnode_remote(notifier, fwnode, struct v4l2_async_connection);
		if (IS_ERR(s_asc)) {
			ret = PTR_ERR(s_asc);
			break;
		}
	}

	if (fwnode)
		fwnode_handle_put(fwnode);

	return ret;
}
#endif

static int register_subdev(struct seninf_ctx *ctx, struct v4l2_device *v4l2_dev)
{
	int i, j, ret;
	struct v4l2_subdev *sd = &ctx->subdev;
	struct device *dev = ctx->dev;
	struct media_pad *pads = ctx->pads;
	struct v4l2_async_notifier *notifier = &ctx->notifier;

	v4l2_subdev_init(sd, &seninf_subdev_ops);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS;
	sd->dev = dev;

	if (strlen(dev->of_node->name) > 16) {
		ret = snprintf(sd->name, sizeof(sd->name), "%s-%s",
			dev_driver_string(dev), &dev->of_node->name[16]);
		if (ret < 0)
			dev_info(dev, "failed to snprintf\n");
	} else {
		ret = snprintf(sd->name, sizeof(sd->name), "%s-%s",
			dev_driver_string(dev),
			csi_port_names[(unsigned int)ctx->port]);
		if (ret < 0)
			dev_info(dev, "failed to snprintf\n");
	}

	v4l2_set_subdevdata(sd, ctx);

	sd->entity.function = MEDIA_ENT_F_VID_IF_BRIDGE;
	sd->entity.ops = &seninf_media_ops;
	sd->internal_ops = &seninf_internal_ops;

	pads[PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	for (i = PAD_SRC_RAW0; i < PAD_MAXCNT; i++)
		pads[i].flags = MEDIA_PAD_FL_SOURCE;

	for (i = 0; i < PAD_MAXCNT; i++)
		for (j = 0; j < MAX_DEST_NUM; j++)
			ctx->pad2cam[i][j] = 0xff;

	ret = media_entity_pads_init(&sd->entity, PAD_MAXCNT, pads);
	if (ret < 0) {
		dev_info(dev, "failed to init pads\n");
		return ret;
	}

	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (ret < 0) {
		dev_info(dev, "failed to register subdev\n");
		return ret;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	v4l2_async_nf_init(notifier);
	ret = v4l2_async_nf_parse_fwnode_endpoints
		(dev, notifier, sizeof(struct v4l2_async_subdev), NULL);
	if (ret < 0)
		dev_info(dev, "no endpoint\n");

	notifier->ops = &seninf_async_ops;
	ret = v4l2_async_nf_register(v4l2_dev, notifier);
#else
	v4l2_async_nf_init(notifier, v4l2_dev);
	ret = seninf_parse_fwnode(dev, notifier);
	if (ret < 0)
		dev_info(dev, "no endpoint\n");

	notifier->ops = &seninf_async_ops;
	ret = v4l2_async_nf_register(notifier);
#endif
	if (ret < 0) {
		dev_info(dev, "failed to register notifier\n");
		goto err_unregister_subdev;
	}

	return 0;

err_unregister_subdev:
	v4l2_device_unregister_subdev(sd);
	v4l2_async_nf_cleanup(notifier);

	return ret;
}

static void unregister_subdev(struct seninf_ctx *ctx)
{
	struct v4l2_subdev *sd = &ctx->subdev;

	v4l2_async_nf_unregister(&ctx->notifier);
	v4l2_async_nf_cleanup(&ctx->notifier);
	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
}

static int seninf_comp_bind(struct device *dev, struct device *master,
			    void *data)
{
	struct mtk_cam_device *cam_dev = data;
	struct v4l2_device *v4l2_dev = &cam_dev->v4l2_dev;
	struct seninf_ctx *ctx = dev_get_drvdata(dev);

	return register_subdev(ctx, v4l2_dev);
}

static void seninf_comp_unbind(struct device *dev, struct device *master,
			       void *data)
{
	struct seninf_ctx *ctx = dev_get_drvdata(dev);

	unregister_subdev(ctx);
}

static const struct component_ops seninf_comp_ops = {
	.bind = seninf_comp_bind,
	.unbind = seninf_comp_unbind,
};

static int seninf_probe(struct platform_device *pdev)
{
	int ret, port;
	struct seninf_ctx *ctx;
	struct device *dev = &pdev->dev;
	struct seninf_core *core;
	int i;
#if CSI_EFUSE_VERIFY_GORDAN_TABLE_EN == 1
	int csi_verify_ret = 0;
	int csi_i;
	int csi_temp = 0;
#endif  /*CSI_EFUSE_VERIFY_GORDAN_TABLE_EN*/

	if (!dev->parent)
		return -EPROBE_DEFER;

	core = dev_get_drvdata(dev->parent);
	if (!core)
		return -EPROBE_DEFER;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	dev_set_drvdata(dev, ctx);
	ctx->dev = dev;
	ctx->core = core;
	list_add(&ctx->list, &core->list);
	INIT_LIST_HEAD(&ctx->list_mux);
	INIT_LIST_HEAD(&ctx->list_cam_mux);
	memset(ctx->mux_by, 0, sizeof(ctx->mux_by));
	ctx->dbg_chmux_param = NULL;

	ctx->open_refcnt = 0;
	mutex_init(&ctx->mutex);

	ret = get_csi_port(dev, &port);
	if (ret) {
		dev_info(dev, "get_csi_port ret %d\n", ret);
		return ret;
	}

	ret = g_seninf_ops->_init_iomem(ctx, core->reg_if, core->reg_ana);
	if (ret) {
		dev_info(dev, "g_seninf_ops->_init_iomem failed ret %d\n", ret);
		return ret;
	}
	g_seninf_ops->_init_port(ctx, port);
	init_fmt(ctx);

	/* default platform properties */
	//ctx->seninf_dphy_settle_delay_dt = 0;

	/* read platform properties from device tree */
	//of_property_read_u32(dev->of_node, "seninf_dphy-settle-delay-dt",
	//	&ctx->seninf_dphy_settle_delay_dt);
	//dev_info(dev, "seninf dphy settle delay dt = %u\n",
	//	 ctx->seninf_dphy_settle_delay_dt);

	ctx->cphy_settle_delay_dt = ctx->core->cphy_settle_delay_dt;
	ctx->dphy_settle_delay_dt = ctx->core->dphy_settle_delay_dt;
	ctx->settle_delay_ck = ctx->core->settle_delay_ck;
	ctx->hs_trail_parameter = ctx->core->hs_trail_parameter;

	of_property_read_u32(dev->of_node, "cphy-settle-delay-dt",
		&ctx->cphy_settle_delay_dt);
	of_property_read_u32(dev->of_node, "dphy-settle-delay-dt",
		&ctx->dphy_settle_delay_dt);
	of_property_read_u32(dev->of_node, "settle-delay-ck",
		&ctx->settle_delay_ck);
	of_property_read_u32(dev->of_node, "hs-trail-parameter",
		&ctx->hs_trail_parameter);

	dev_info(dev,
		"seninf d_settlte/d_settle_ck/d_trail/c_settle= %u/%u/%u/%u\n",
		ctx->seninf_dphy_settle_delay_dt,
		ctx->settle_delay_ck,
		ctx->hs_trail_parameter,
		ctx->cphy_settle_delay_dt);

#ifdef CSI_EFUSE_SET
	ret = dev_read_csi_efuse(ctx);
	if (ret < 0)
		dev_info(dev, "Failed to read efuse data\n");
#if CSI_EFUSE_VERIFY_EN == 1
#if CSI_EFUSE_VERIFY_GORDAN_TABLE_EN == 1
	csi_temp = ctx->m_csi_efuse;
	for (csi_i = 0; csi_i <= 31; csi_i++) {
		ctx->m_csi_efuse = gordan_csi_efuse_table[csi_i][ctx->port];
		csi_verify_ret = csi_efuse_value_verify(ctx);
		if (csi_verify_ret < 0)
			dev_info(dev, "Gordan csi efuse verify fail\n");
	}
	ctx->m_csi_efuse = csi_temp;
#endif  /*CSI_EFUSE_VERIFY_GORDAN_TABLE_EN*/
	if (csi_efuse_value_verify(ctx) < 0) {
		dev_info(dev, "Failed to verify efuse data\n");
		aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
		"seninf", "Failed to verify efuse data");
	}
#endif  /*CSI_EFUSE_VERIFY_EN*/
#endif  /*CSI_EFUSE_SET*/

	ret = seninf_initialize_controls(ctx);
	if (ret) {
		dev_info(dev, "Failed to initialize controls\n");
		return ret;
	}

	ret = component_add(dev, &seninf_comp_ops);
	if (ret < 0) {
		dev_info(dev, "component_add failed\n");
		goto err_free_handler;
	}


	pm_runtime_enable(dev);
	device_enable_async_suspend(dev);

	memset(&g_aov_param, 0, sizeof(struct mtk_seninf_aov_param));
	/* array size of aov_ctx[] is
	 * AOV_SENINF_NUM: most number of sensors support
	 */
	for (i = 0; i < AOV_SENINF_NUM; i++)
		aov_ctx[i] = NULL;

	dev_info(dev, "%s: port=%d\n", __func__, ctx->port);

	return 0;

err_free_handler:
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);

	return ret;
}

static int enable_phya_clk(struct seninf_ctx *ctx)
{
	int ret = 0;
	struct seninf_core *core = ctx->core;

	/* set the parent of clk as parent_clk */
	if (core->pwr_refcnt_for_aov &&
		!(core->aov_sensor_id < 0) &&
		!(core->current_sensor_id < 0) &&
		(core->current_sensor_id != core->aov_sensor_id))
		seninf_logi(ctx, "aov is using phya osc source clk now\n");
	else {
		ret = g_seninf_ops->_set_phya_clock_src(ctx, 1);
		if (ret < 0) {
			seninf_logi(ctx, "fail to set phya osc source clk,ret(%d)\n", ret);
			return ret;
		}
	}

	return 0;
}

static int disable_phya_clk(struct seninf_ctx *ctx)
{
	int ret = 0;

	ret = g_seninf_ops->_set_phya_clock_src(ctx, 0);
	if (ret < 0) {
		seninf_logi(ctx, "fail to set phya pll source clk,ret(%d)\n", ret);
		return ret;
	}

	return 0;
}

static int mtk_cam_get_vcore_csi_clk_src(struct seninf_ctx *ctx,
	enum CDPHY_DVFS_STEP_ENUM index,
	unsigned int *clk_src_index)
{
	struct seninf_core *core = ctx->core;
	int ret = 0;

	/* vcore step index <-> csi clk mapping */
	switch (index) {
	case CDPHY_DVFS_STEP_0:
		/* ex. DX-3 csi clk: 312M */
		if (!core->clk[CLK_TOP_AP_STEP0]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_AP_STEP0] = NULL\n");
			return -EINVAL;
		} else
			*clk_src_index = CLK_TOP_AP_STEP0;
		break;
	case CDPHY_DVFS_STEP_1:
		/* ex. DX-3 csi clk: 356M */
		if (!core->clk[CLK_TOP_AP_STEP1]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_AP_STEP1] = NULL\n");
			return -EINVAL;
		} else
			*clk_src_index = CLK_TOP_AP_STEP1;
		break;
	case CDPHY_DVFS_STEP_2:
		/* ex. DX-3 csi clk: 416M */
		if (!core->clk[CLK_TOP_AP_STEP2]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_AP_STEP2] = NULL\n");
			return -EINVAL;
		} else
			*clk_src_index = CLK_TOP_AP_STEP2;
		break;
	case CDPHY_DVFS_STEP_3:
		/* ex. DX-3 csi clk: 499M */
		if (!core->clk[CLK_TOP_AP_STEP3]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_AP_STEP3] = NULL\n");
			return -EINVAL;
		} else
			*clk_src_index = CLK_TOP_AP_STEP3;
		break;
	case CDPHY_DVFS_STEP_4:
		/* ex. DX-3 csi clk: none */
		if (!core->clk[CLK_TOP_AP_STEP4]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_AP_STEP4] = NULL\n");
			return -EINVAL;
		} else
			*clk_src_index = CLK_TOP_AP_STEP4;
		break;
	case CDPHY_DVFS_STEP_5:
		/* ex. DX-3 csi clk: none */
		if (!core->clk[CLK_TOP_AP_STEP5]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_AP_STEP5] = NULL\n");
			return -EINVAL;
		} else
			*clk_src_index = CLK_TOP_AP_STEP5;
		break;
	default:
		/* csi clk: unknown */
		seninf_logi(ctx, "please check index:%u\n", index);
		return -EINVAL;
	}

	return ret;
}

static int set_csi_clk(struct seninf_ctx *ctx, enum CDPHY_DVFS_STEP_ENUM index)
{
	struct seninf_core *core = ctx->core;
	int ret = 0;
	struct clk *clk = NULL, *clk_src = NULL;
	unsigned int clk_index = 0, clk_src_index = 0;

	switch (ctx->seninfIdx) {
	case SENINF_1:
	case SENINF_2:
		if (!core->clk[CLK_TOP_SENINF]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_SENINF] = NULL\n");
			return -EINVAL;
		} else {
			clk_index = CLK_TOP_SENINF;
			clk = core->clk[CLK_TOP_SENINF];
		}
		break;
	case SENINF_3:
	case SENINF_4:
		if (!core->clk[CLK_TOP_SENINF1]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_SENINF1] = NULL\n");
			return -EINVAL;
		} else {
			clk_index = CLK_TOP_SENINF1;
			clk = core->clk[CLK_TOP_SENINF1];
		}
		break;
	case SENINF_5:
	case SENINF_6:
		if (!core->clk[CLK_TOP_SENINF2]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_SENINF2] = NULL\n");
			return -EINVAL;
		} else {
			clk_index = CLK_TOP_SENINF2;
			clk = core->clk[CLK_TOP_SENINF2];
		}
		break;
	case SENINF_7:
	case SENINF_8:
		if (!core->clk[CLK_TOP_SENINF3]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_SENINF3] = NULL\n");
			return -EINVAL;
		} else {
			clk_index = CLK_TOP_SENINF3;
			clk = core->clk[CLK_TOP_SENINF3];
		}
		break;
	case SENINF_9:
	case SENINF_10:
		if (!core->clk[CLK_TOP_SENINF4]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_SENINF4] = NULL\n");
			return -EINVAL;
		} else {
			clk_index = CLK_TOP_SENINF4;
			clk = core->clk[CLK_TOP_SENINF4];
		}
		break;
	case SENINF_11:
	case SENINF_12:
		if (!core->clk[CLK_TOP_SENINF5]) {
			seninf_logi(ctx, "core->clk[CLK_TOP_SENINF5] = NULL\n");
			return -EINVAL;
		} else {
			clk_index = CLK_TOP_SENINF5;
			clk = core->clk[CLK_TOP_SENINF5];
		}
		break;
	default:
		seninf_logi(ctx, "invalid seninfIdx %d\n", ctx->seninfIdx);
		return -EINVAL;
	}
	/* set csi clk from dts according to vcore voltage from dts */
	ret = mtk_cam_get_vcore_csi_clk_src(ctx, index, &clk_src_index);
	if (ret < 0) {
		seninf_logi(ctx, "mtk_cam_get_vcore_csi_clk_src(fail),ret(%d),index(%u)", ret, index);
		return -EINVAL;
	} else {
		seninf_logd(ctx,
			"mtk_cam_get_vcore_csi_clk_src(success),clk[%u]:%s,clk_src[%u]:%s,ret(%d)\n",
			clk_index, clk_names[clk_index],
			clk_src_index, clk_names[clk_src_index], ret);
		clk_src = core->clk[clk_src_index];
	}
	/* enable seninf csi clk */
	if (IS_ERR(clk)) {
		seninf_logi(ctx, "clk == NULL");
		return -EINVAL;
	}
	ret = clk_prepare_enable(clk);
	if (ret < 0) {
		seninf_logi(ctx,
			"clk_prepare_enable clk[%u]:%s(fail),ret(%d)\n",
			clk_index, clk_names[clk_index], ret);
		return ret;
	} else
		seninf_logd(ctx,
			"clk_prepare_enable clk[%u]:%s(success),ret(%d)\n",
			clk_index, clk_names[clk_index], ret);
	/* set seninf csi clk source */
	if (!core->aov_ut_debug_for_get_csi_param) {
		if (IS_ERR(clk_src)) {
			seninf_logi(ctx, "clk_src == NULL");
			return -EINVAL;
		}
		ret = clk_set_parent(clk, clk_src);
		if (ret) {
			seninf_logi(ctx,
				"clk_set_parent(clk[%u]:%s,clk_src[%u]:%s)(fail),ret(%d)\n",
				clk_index, clk_names[clk_index],
				clk_src_index, clk_names[clk_src_index], ret);
			return -EINVAL;
		} else {
			seninf_logd(ctx,
				"clk_set_parent(clk[%u]:%s,clk_src[%u]:%s)(success),ret(%d),aov_ut_debug_for_get_csi_param(%u)\n",
				clk_index, clk_names[clk_index],
				clk_src_index, clk_names[clk_src_index],
				ret, core->aov_ut_debug_for_get_csi_param);
			ctx->vcore_step_index = index;
			ctx->clk_index = clk_index;
			ctx->clk_src_index = clk_src_index;
		}
	} else {
		if (IS_ERR(core->clk[CLK_TOP_AOV_STEP0])) {
			seninf_logi(ctx, "core->clk[CLK_TOP_AOV_STEP0] = NULL\n");
			return -EINVAL;
		}
		ret = clk_set_parent(clk, core->clk[CLK_TOP_AOV_STEP0]);
		if (ret) {
			seninf_logi(ctx,
				"clk_set_parent(clk[%u]:%s,clk_src[%u]:%s)(fail),ret(%d),aov_ut_debug_for_get_csi_param(%u)\n",
				clk_index, clk_names[clk_index],
				CLK_TOP_AOV_STEP0, clk_names[CLK_TOP_AOV_STEP0],
				ret, core->aov_ut_debug_for_get_csi_param);
			return -EINVAL;
		} else
			seninf_logd(ctx,
				"clk_set_parent(clk[%u]:%s,clk_src[%u]:%s)(success),ret(%d),aov_ut_debug_for_get_csi_param(%u)\n",
				clk_index, clk_names[clk_index],
				CLK_TOP_AOV_STEP0, clk_names[CLK_TOP_AOV_STEP0],
				ret, core->aov_ut_debug_for_get_csi_param);
			ctx->vcore_step_index = index;
			ctx->clk_index = clk_index;
			ctx->clk_src_index = CLK_TOP_AOV_STEP0;
	}

	return 0;
}

static int data_rate2vcore_voltage(struct seninf_ctx *ctx,
	struct seninf_struct_pair_u64 *cdphy_dvfs_data_rate,
	u64 data_rate, enum CDPHY_DVFS_STEP_ENUM *index)
{
	struct seninf_core *core = ctx->core;
	enum CDPHY_DVFS_STEP_ENUM i;
	int vcore_voltage = 0;

	for (i = CDPHY_DVFS_STEP_0; i < CDPHY_DVFS_STEP_MAX_NUM; i++) {
		if (cdphy_dvfs_data_rate[i].first == 0 ||
			cdphy_dvfs_data_rate[i].second == 0)
			break;
		seninf_logd(ctx,
			"cdphy_dvfs_data_rate[i:%u].first/second:%llu/%llu",
			i, cdphy_dvfs_data_rate[i].first, cdphy_dvfs_data_rate[i].second);
	}

	if (ctx->is_4d1c) {
		for (i = CDPHY_DVFS_STEP_0; i < CDPHY_DVFS_STEP_MAX_NUM; i++) {
			if (data_rate < cdphy_dvfs_data_rate[i].first) {
				vcore_voltage =
					core->cdphy_dvfs_step[i].cdphy_voltage.first;
				*index = i;
				break;
			}
			if (cdphy_dvfs_data_rate[i].first == 0 && i > CDPHY_DVFS_STEP_0) {
				vcore_voltage =
					core->cdphy_dvfs_step[i-1].cdphy_voltage.first;
				*index = i-1;
				break;
			}
		}
	} else {
		for (i = CDPHY_DVFS_STEP_0; i < CDPHY_DVFS_STEP_MAX_NUM; i++) {
			if (data_rate < cdphy_dvfs_data_rate[i].second) {
				vcore_voltage =
					core->cdphy_dvfs_step[i].cdphy_voltage.second;
				*index = i;
				break;
			}
			if (cdphy_dvfs_data_rate[i].second == 0 && i > CDPHY_DVFS_STEP_0) {
				vcore_voltage =
					core->cdphy_dvfs_step[i-1].cdphy_voltage.second;
				*index = i-1;
				break;
			}
		}
	}
	return vcore_voltage;
}

static int set_vcore_power(struct seninf_ctx *ctx, u64 data_rate)
{
	struct seninf_core *core = ctx->core;
	int ret = 0;
	enum CDPHY_DVFS_STEP_ENUM i, j, index;
	struct seninf_struct_pair_u64 cdphy_dvfs_data_rate[CDPHY_DVFS_STEP_MAX_NUM] = {0};
	int vcore_voltage = 0;

	seninf_logd(ctx,
		"line:%d,data rate(%llu),is_cphy(%u),is_4d1c(%d)\n",
		__LINE__, data_rate, ctx->is_cphy, ctx->is_4d1c);
	if (data_rate <= 0) {
		seninf_logi(ctx,
			"line:%d,please check data rate(%llu),is_cphy(%u),is_4d1c(%d)\n",
			__LINE__, data_rate, ctx->is_cphy, ctx->is_4d1c);
		return -EINVAL;
	}
	/* loop compare data rate -> if match condition, then break return vcore voltage */
	if (!ctx->is_cphy) {	// Dphy
		/* dphy vcore range */
		for (i = CDPHY_DVFS_STEP_0; i < CDPHY_DVFS_STEP_MAX_NUM; i++) {
			cdphy_dvfs_data_rate[i].first =
				(u64)core->cdphy_dvfs_step[i].dphy_data_rate.first * CSI_CLK_FREQ_MULTIPLIER;
			cdphy_dvfs_data_rate[i].second =
				(u64)core->cdphy_dvfs_step[i].dphy_data_rate.second * CSI_CLK_FREQ_MULTIPLIER;
			if (cdphy_dvfs_data_rate[i].first == 0 ||
				cdphy_dvfs_data_rate[i].second == 0)
				break;
		}
	} else {	// Cphy
		/* cphy vcore range */
		for (j = CDPHY_DVFS_STEP_0; j < CDPHY_DVFS_STEP_MAX_NUM; j++) {
			cdphy_dvfs_data_rate[j].first =
				(u64)core->cdphy_dvfs_step[j].cphy_data_rate.first * CSI_CLK_FREQ_MULTIPLIER;
			cdphy_dvfs_data_rate[j].second =
				(u64)core->cdphy_dvfs_step[j].cphy_data_rate.second * CSI_CLK_FREQ_MULTIPLIER;
			if (cdphy_dvfs_data_rate[j].first == 0 ||
				cdphy_dvfs_data_rate[j].second == 0)
				break;
		}
	}
	vcore_voltage = data_rate2vcore_voltage(
		ctx, cdphy_dvfs_data_rate, data_rate, &index);
	if (!vcore_voltage) {
		seninf_logi(ctx,
			"can't find appropriate vcore_voltage(%d) with data rate(%llu)\n",
			vcore_voltage, data_rate);
		return -EINVAL;
	}
	seninf_logd(ctx,
		"line:%d,index(%u),vcore_voltage(%d)\n",
		__LINE__, index, vcore_voltage);
	/* regulator api to set vcore voltage. unit: uV */
	if (!IS_ERR(core->dvfsrc_vcore_power)) {
		if (regulator_get_voltage(core->dvfsrc_vcore_power) < vcore_voltage) {
			ret = regulator_set_voltage(core->dvfsrc_vcore_power,
				vcore_voltage, INT_MAX);
			if (ret) {
				seninf_logi(ctx,
					"regulator_set_voltage,val(%d),ret(%d)(fail) when data rate(%llu),is_cphy/4d1c(%u/%u)\n",
					vcore_voltage, ret, data_rate, ctx->is_cphy, ctx->is_4d1c);
				return ret;
			} else
				seninf_logd(ctx,
					"regulator_set_voltage,val(%d),ret(%d)(success) when data rate(%llu),is_cphy/4d1c(%u/%u)\n",
					vcore_voltage, ret, data_rate, ctx->is_cphy, ctx->is_4d1c);
		}
	} else {
		seninf_logi(ctx, "core->dvfsrc_vcore_power == NULL\n");
		return -EINVAL;
	}
	/* set csi clock after setting vcore voltage */
	ret = set_csi_clk(ctx, index);
	if (ret < 0) {
		seninf_logi(ctx,
			"set_csi_clk,ret(%d)(fail) when vcore_power(%d),data rate(%llu),is_cphy/4d1c(%u/%u)\n",
			ret, vcore_voltage, data_rate, ctx->is_cphy, ctx->is_4d1c);
		return ret;
	} else
		seninf_logd(ctx,
			"set_csi_clk,ret(%d)(success) when vcore_power(%d),data rate(%llu),is_cphy/4d1c(%u/%u)\n",
			ret, vcore_voltage, data_rate, ctx->is_cphy, ctx->is_4d1c);

	seninf_logi(ctx,
		"ret(%d)(success) in vcore_power/data rate/csi_clk[%u]/src[%u](%d/%llu/%s/%s),is_cphy/4d1c(%u/%u)\n",
		ret, ctx->clk_index, ctx->clk_src_index,
		vcore_voltage, data_rate,
		clk_names[ctx->clk_index], clk_names[ctx->clk_src_index],
		ctx->is_cphy, ctx->is_4d1c);

	return 0;
}

static int runtime_suspend(struct device *dev)
{
	struct seninf_ctx *ctx = dev_get_drvdata(dev);
	struct seninf_core *core = ctx->core;
	int i = 0;
	unsigned long flags;

	mutex_lock(&core->mutex);

	core->refcnt--;
	if (core->refcnt >= 0) {
		if (core->refcnt == 0)
			seninf_logi(ctx,
				"last user(%d),cnt(%d)\n",
				core->current_sensor_id, core->refcnt);
		else
			seninf_logi(ctx,
				"multi user(%d),cnt(%d)\n",
				core->current_sensor_id, core->refcnt);

		spin_lock_irqsave(&core->spinlock_irq, flags);
		ctx->power_status_flag = 0;
		spin_unlock_irqrestore(&core->spinlock_irq, flags);

		/* disable camtg_sel as phya clk */
		disable_phya_clk(ctx);
		/* disable seninf csi clk */
		switch (ctx->seninfIdx) {
		case SENINF_1:
		case SENINF_2:
			if (core->clk[CLK_TOP_SENINF])
				clk_disable_unprepare(core->clk[CLK_TOP_SENINF]);
			break;
		case SENINF_3:
		case SENINF_4:
			if (core->clk[CLK_TOP_SENINF1])
				clk_disable_unprepare(core->clk[CLK_TOP_SENINF1]);
			break;
		case SENINF_5:
		case SENINF_6:
			if (core->clk[CLK_TOP_SENINF2])
				clk_disable_unprepare(core->clk[CLK_TOP_SENINF2]);
			break;
		case SENINF_7:
		case SENINF_8:
			if (core->clk[CLK_TOP_SENINF3])
				clk_disable_unprepare(core->clk[CLK_TOP_SENINF3]);
			break;
		case SENINF_9:
		case SENINF_10:
			if (core->clk[CLK_TOP_SENINF4])
				clk_disable_unprepare(core->clk[CLK_TOP_SENINF4]);
			break;
		case SENINF_11:
		case SENINF_12:
			if (core->clk[CLK_TOP_SENINF5])
				clk_disable_unprepare(core->clk[CLK_TOP_SENINF5]);
			break;
		default:
			seninf_logi(ctx, "invalid seninfIdx(%d)\n", ctx->seninfIdx);
			mutex_unlock(&core->mutex);
			return -EINVAL;
		}

		if (core->refcnt == 0) {
			/* disable tsrec timer clk */
			mtk_cam_seninf_tsrec_timer_enable(0);

			/* one of the source clk of TSREC */
			if (core->clk[CLK_TOP_CAMTM]) {
				clk_disable_unprepare(core->clk[CLK_TOP_CAMTM]);
				seninf_logi(ctx, "clk_disable_unprepare CLK_TOP_CAMTM\n");
			}

			/* disable cg (cam, seninf, camtg) */
			for (i = CLK_TOP_SENINF - 1; i >= 0; --i) {
				if (core->clk[i]) {
					clk_disable_unprepare(core->clk[i]);
					seninf_logd(ctx,
						"clk_disable_unprepare clk[%d]:%s\n",
						i, clk_names[i]);
				}
			}

			/* power-domains disable */
			seninf_core_pm_runtime_put(core);
		}
	} else
		seninf_logi(ctx, "times error! cnt(%d)\n", core->refcnt);

	mutex_unlock(&core->mutex);

	return 0;
}

static int runtime_resume(struct device *dev)
{
	struct seninf_ctx *ctx = dev_get_drvdata(dev);
	struct seninf_core *core = ctx->core;
	unsigned int i = 0;
	int ret = 0;
	struct seninf_vc *vc = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW0);
	struct seninf_vc *vc1 = mtk_cam_seninf_get_vc_by_pad(ctx, PAD_SRC_RAW_EXT0);
	int bit_per_pixel = 10;
	u64 data_rate = 0;
	unsigned long flags;


	mutex_lock(&core->mutex);

	if (vc)
		bit_per_pixel = vc->bit_depth;
	else if (vc1)
		bit_per_pixel = vc1->bit_depth;

	core->refcnt++;
	if (core->refcnt > 0) {
		if (core->refcnt == 1) {
			dev_info(dev,
				"[%s] 1st user(%d),cnt(%d)\n",
				__func__, core->current_sensor_id, core->refcnt);
			/* power-domains enable */
			ret = seninf_core_pm_runtime_get_sync(core);
			if (ret < 0) {
				seninf_logi(ctx, "seninf_core_pm_runtime_get_sync(fail),ret(%d)\n", ret);
				mutex_unlock(&core->mutex);
				return ret;
			}
			seninf_logd(ctx, "seninf_core_pm_runtime_get_sync(success),ret(%d)\n", ret);
			/*
			 * enable seninf cg
			 * including cam, seninf, camtg
			 */
			for (i = 0; i < CLK_TOP_SENINF; ++i) {
				if (core->clk[i]) {
					ret = clk_prepare_enable(core->clk[i]);
					if (ret < 0) {
						seninf_logi(ctx,
							"clk_prepare_enable clk[%u]:%s(fail),ret(%d)\n",
							i, clk_names[i], ret);
						mutex_unlock(&core->mutex);
						return ret;
					}
					seninf_logd(ctx,
						"clk_prepare_enable clk[%u]:%s(success),ret(%d)\n",
						i, clk_names[i], ret);
				}
			}
			/* one of the source clk of TSREC */
			if (core->clk[CLK_TOP_CAMTM]) {
				ret = clk_prepare_enable(core->clk[CLK_TOP_CAMTM]);
				if (ret < 0) {
					seninf_logi(ctx,
						"clk_prepare_enable clk[CLK_TOP_CAMTM:%u]:%s(fail),ret(%d)\n",
						CLK_TOP_CAMTM, clk_names[CLK_TOP_CAMTM], ret);
					mutex_unlock(&core->mutex);
					return ret;
				}
				seninf_logd(ctx,
					"clk_prepare_enable clk[CLK_TOP_CAMTM:%u]:%s(success),ret(%d)\n",
					CLK_TOP_CAMTM, clk_names[CLK_TOP_CAMTM], ret);
				/* enable tsrec timer clk */
				mtk_cam_seninf_tsrec_timer_enable(1);
			}
		} else
			seninf_logi(ctx,
				"multi user(%d),cnt(%d)\n",
				core->current_sensor_id, core->refcnt);
		/*
		 * set vcore according to data rate
		 * set csi clk according to vcore range
		 */
		data_rate = ctx->mipi_pixel_rate * bit_per_pixel;
		if (data_rate > 0) {
			seninf_logd(ctx,
				"data rate(%llu),mipi pixel rate(%lld),bit_per_pixel(%d),num_data_lanes(%d)\n",
				data_rate, ctx->mipi_pixel_rate, bit_per_pixel, ctx->num_data_lanes);
			if (!ctx->is_cphy) {	// Dphy
				do_div(data_rate, ctx->num_data_lanes);
			} else {	// Cphy
				data_rate *= 7;
				do_div(data_rate, ctx->num_data_lanes*16);
			}
		} else {
			seninf_logi(ctx,
				"please check data rate(%llu),mipi pixel rate(%lld),bit_per_pixel(%d),num_data_lanes(%d)\n",
				data_rate, ctx->mipi_pixel_rate, bit_per_pixel, ctx->num_data_lanes);
			mutex_unlock(&core->mutex);
			return -EINVAL;
		}
		ret = set_vcore_power(ctx, data_rate);
		if (ret) {
			seninf_logi(ctx, "set_vcore_power(fail),ret(%d)\n", ret);
			mutex_unlock(&core->mutex);
			return -EINVAL;
		} else {
			if (!IS_ERR(core->dvfsrc_vcore_power))
				seninf_logd(ctx,
					"set_vcore_power(%d)(success),ret(%d)\n",
					regulator_get_voltage(core->dvfsrc_vcore_power), ret);
			else {
				seninf_logi(ctx, "core->dvfsrc_vcore_power == NULL\n");
				mutex_unlock(&core->mutex);
				return -EINVAL;
			}
		}
		/* enable camtg_sel as phya clk */
		ret = enable_phya_clk(ctx);
		if (ret < 0) {
			seninf_logi(ctx, "enable_phya_clk(fail),ret(%d)\n", ret);
			mutex_unlock(&core->mutex);
			return ret;
		}

		spin_lock_irqsave(&core->spinlock_irq, flags);
		ctx->power_status_flag = 1;
		spin_unlock_irqrestore(&core->spinlock_irq, flags);

		if (core->refcnt == 1) {
			if (core->pwr_refcnt_for_aov &&
				!(core->aov_sensor_id < 0) &&
				!(core->current_sensor_id < 0) &&
				(core->current_sensor_id != core->aov_sensor_id))
				seninf_logi(ctx, "aov sensor streaming on scp now, won't disable mux/cammux\n");
			else {
				seninf_logi(ctx, "common sensor streaming, disable mux/cammux for initialization\n");
				g_seninf_ops->_disable_all_mux(ctx);
				g_seninf_ops->_disable_all_cammux(ctx);
			}
		}
	} else
		seninf_logi(ctx, "times error! cnt(%d)\n", core->refcnt);

	mutex_unlock(&core->mutex);

	return ret;
}

static const struct dev_pm_ops pm_ops = {
	SET_RUNTIME_PM_OPS(runtime_suspend, runtime_resume, NULL)
};

static int seninf_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct seninf_ctx *ctx = dev_get_drvdata(dev);

	if (ctx->streaming) {
		g_seninf_ops->_set_idle(ctx);
		mtk_cam_seninf_release_mux(ctx);
		mtk_cam_seninf_tsrec_n_reset(ctx->seninfIdx);
	}

	pm_runtime_disable(ctx->dev);

	component_del(dev, &seninf_comp_ops);

	v4l2_ctrl_handler_free(&ctx->ctrl_handler);

	mutex_destroy(&ctx->mutex);

	return 0;
}

static const struct of_device_id seninf_of_match[] = {
	{.compatible = "mediatek,seninf"},
	{},
};
MODULE_DEVICE_TABLE(of, seninf_of_match);

static int seninf_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int seninf_resume(struct platform_device *pdev)
{
	return 0;
}

struct platform_driver seninf_pdrv = {
	.probe	= seninf_probe,
	.remove	= seninf_remove,
	.suspend = seninf_suspend,
	.resume = seninf_resume,
	.driver	= {
		.name = "seninf",
		.of_match_table = seninf_of_match,
		.pm  = &pm_ops,
	},
};

int mtk_cam_seninf_calc_pixelrate(struct device *dev, s64 width, s64 height,
				  s64 hblank, s64 vblank,
				  int fps_n, int fps_d,
				  s64 sensor_pixel_rate)
{
	int ret;
	s64 p_pixel_rate = sensor_pixel_rate;

	ret = calc_buffered_pixel_rate(dev, width, height, hblank, vblank,
				       fps_n, fps_d, &p_pixel_rate);
	if (ret)
		return sensor_pixel_rate;

	return p_pixel_rate;
}

/* to be replaced by mtk_cam_seninf_calc_pixelrate() */
int mtk_cam_seninf_get_pixelrate(struct v4l2_subdev *sd, s64 *p_pixel_rate)
{
	int ret;
	s64 pixel_rate = -1;
	struct seninf_ctx *ctx = sd_to_ctx(sd);

	if (!ctx->sensor_sd) {
		dev_info(ctx->dev, "no sensor\n");
		return -EFAULT;
	}

	ret = get_buffered_pixel_rate(ctx,
				      ctx->sensor_sd, ctx->sensor_pad_idx,
				      &pixel_rate);
	if (ret)
		get_pixel_rate(ctx, ctx->sensor_sd, &pixel_rate);

	if (pixel_rate <= 0) {
		dev_info(ctx->dev, "failed to get pixel_rate\n");
		return -EINVAL;
	}

	*p_pixel_rate = pixel_rate;

	return 0;
}

#define SOF_TIMEOUT_RATIO 110
int mtk_cam_seninf_check_timeout(struct v4l2_subdev *sd, u64 time_after_sof)
{
	struct seninf_ctx *ctx = sd_to_ctx(sd);
	u64 frame_time = 400000;//400ms
	int val = 0;
	int ret = 0;
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler, V4L2_CID_MTK_SOF_TIMEOUT_VALUE);
	if (!ctrl) {
		dev_info(ctx->dev, "no timeout value in subdev %s\n", sd->name);
		return -EINVAL;
	}

	val = v4l2_ctrl_g_ctrl(ctrl);

	if (val > 0)
		frame_time = val;
	time_after_sof /= 1000;// covert into us

	if ((time_after_sof) > ((frame_time * SOF_TIMEOUT_RATIO) / 100))
		ret = -1;

	dev_info(ctx->dev, "%s time_after_sof %llu frame_time %llu in us ret %d ratio %d val %d\n",
		__func__,
		time_after_sof,
		frame_time,
		ret,
		SOF_TIMEOUT_RATIO,
		val);
	return ret;
}

u64 mtk_cam_seninf_get_frame_time(struct v4l2_subdev *sd, u32 seq_id)
{
	u64 tmp = 33000;
	struct seninf_ctx *ctx = sd_to_ctx(sd);
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct v4l2_ctrl *ctrl;
	int val = 0;

	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler, V4L2_CID_MTK_SOF_TIMEOUT_VALUE);
	if (ctrl) {
		val = v4l2_ctrl_g_ctrl(ctrl);
		if (val > 0)
			tmp = val;
	}
	return tmp * 1000;
}

int mtk_cam_seninf_dump(struct v4l2_subdev *sd, u32 seq_id, bool force_check)
{
	int ret = 0;
	struct seninf_ctx *ctx = sd_to_ctx(sd);
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	struct v4l2_ctrl *ctrl;
	int val = 0;
	int reset_by_user = 0;
	bool in_reset = 0;

	if (!force_check && ctx->dbg_last_dump_req != 0 &&
		ctx->dbg_last_dump_req == seq_id) {
		dev_info(ctx->dev, "%s skip duplicate dump for req %u\n", __func__, seq_id);
		return 0;
	}

	ctx->dbg_last_dump_req = seq_id;
	ctx->dbg_timeout = 0; //in us

	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler, V4L2_CID_MTK_SOF_TIMEOUT_VALUE);
	if (ctrl) {
		val = v4l2_ctrl_g_ctrl(ctrl);
		if (val > 0)
			ctx->dbg_timeout = val;
	}
	ret = pm_runtime_get_sync(ctx->dev);
	if (ret < 0) {
		dev_info(ctx->dev, "%s pm_runtime_get_sync ret %d\n", __func__, ret);
		pm_runtime_put_noidle(ctx->dev);
		return ret;
	}

	/* query if sensor in reset */
	sensor_sd->ops->core->command(sensor_sd,
			V4L2_CMD_SENSOR_IN_RESET, &in_reset);

	if (ctx->streaming) {
		if (!in_reset) {
			ret = g_seninf_ops->_debug(sd_to_ctx(sd));
#if ESD_RESET_SUPPORT
			if (ret != 0) {
				reset_by_user = is_reset_by_user(sd_to_ctx(sd));
				if (!reset_by_user){
					reset_sensor(sd_to_ctx(sd));
					ctx->esd_status_flag = 1;
				}
			}
#endif
		} else
			dev_info(ctx->dev, "%s skip dump, sensor is in resetting\n", __func__);
	} else
		dev_info(ctx->dev, "%s should not dump during stream off\n", __func__);

	pm_runtime_put_sync(ctx->dev);

	dev_info(ctx->dev, "%s ret(%d), req(%u), force(%d) reset_by_user(%d)\n",
		 __func__, ret, seq_id, force_check, reset_by_user);

	return (ret && reset_by_user);
}

int mtk_cam_seninf_get_csi_irq_status(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct seninf_ctx *ctx = sd_to_ctx(sd);

	ctrl->val  = (g_seninf_ops->_get_csi_irq_status(sd_to_ctx(sd)) & 0x7fff)
							| (ctx->esd_status_flag << 15);
	ctx->esd_status_flag = 0;
	dev_info(ctx->dev,"SENINF%d_CSI2_IRQ_STATUS(0x%x)\n", ctx->seninfIdx, ctrl->val);

	return 0;
}

int mtk_cam_seninf_dump_current_status(struct v4l2_subdev *sd)
{
	int ret = 0;
	struct seninf_ctx *ctx = sd_to_ctx(sd);
	struct v4l2_subdev *sensor_sd = ctx->sensor_sd;
	int reset_by_user = 0;
	bool in_reset = 0;

	ret = pm_runtime_get_sync(ctx->dev);
	if (ret < 0) {
		dev_info(ctx->dev, "%s pm_runtime_get_sync ret %d\n", __func__, ret);
		pm_runtime_put_noidle(ctx->dev);
		return ret;
	}

	/* query if sensor in reset */
	sensor_sd->ops->core->command(sensor_sd,
			V4L2_CMD_SENSOR_IN_RESET, &in_reset);

	if (ctx->streaming) {
		if (!in_reset) {
			ret = g_seninf_ops->_debug_current_status(sd_to_ctx(sd));
		} else
			dev_info(ctx->dev, "%s skip dump, sensor is in resetting\n", __func__);
	} else
		dev_info(ctx->dev, "%s should not dump during stream off\n", __func__);

	pm_runtime_put_sync(ctx->dev);

	dev_info(ctx->dev, "%s ret(%d),reset_by_user(%d)\n",
		 __func__, ret, reset_by_user);

	return (ret && reset_by_user);
}

void mtk_cam_seninf_set_secure(struct v4l2_subdev *sd, int enable, u64 SecInfo_addr)
{
	struct seninf_ctx *ctx = sd_to_ctx(sd);

	ctx->SecInfo_addr = SecInfo_addr;
	dev_info(ctx->dev, "[%s]: %llx, enable: %d\n", __func__, SecInfo_addr, enable);
	ctx->is_secure = enable ? 1 : 0;
}

int mtk_cam_seninf_aov_runtime_suspend(unsigned int sensor_id)
{
	struct seninf_ctx *ctx = NULL;
	unsigned int real_sensor_id = 0;
	struct seninf_core *core = NULL;
	int ret = 0;
	int i = 0;

	pr_info("[%s] sensor_id(%d)\n", __func__, sensor_id);

	if (g_aov_param.is_test_model) {
		real_sensor_id = 5;
	} else {
		if (sensor_id == g_aov_param.sensor_idx) {
			real_sensor_id = g_aov_param.sensor_idx;
			pr_info("[%s] input sensor id(%u)(success)\n",
				__func__, real_sensor_id);
		} else {
			real_sensor_id = sensor_id;
			pr_info("input sensor id(%u)(fail)\n", real_sensor_id);
			seninf_aee_print(
				"[AEE] [%s] input sensor id(%u)(fail)",
				__func__, real_sensor_id);
			return -ENODEV;
		}
	}
	/* debug use
	 * if (g_aov_param.sensor_idx)
	 *	pr_info("g_aov_param.sensor_idx %d\n",
	 *		g_aov_param.sensor_idx);
	 */
	if (aov_ctx[real_sensor_id] != NULL) {
		pr_info("[%s] sensor idx(%u)\n", __func__, real_sensor_id);
		ctx = aov_ctx[real_sensor_id];
	} else {
		pr_info("[%s] Can't find ctx from input sensor id!\n", __func__);
		return -ENODEV;
	}

	core = ctx->core;
	mutex_lock(&core->mutex);

	core->pwr_refcnt_for_aov++;
	if (core->pwr_refcnt_for_aov < 0) {
		dev_info(ctx->dev,
			"[%s] please check aov_deinit times?(%d)\n",
			__func__,
			core->pwr_refcnt_for_aov);
		mutex_unlock(&core->mutex);
		return -ENODEV;
	}
	dev_info(ctx->dev,
		"[%s] pwr_refcnt_for_aov(%d)\n",
		__func__,
		core->pwr_refcnt_for_aov);
#ifdef AOV_SUSPEND_RESUME_USE_PM_CLK
	core->refcnt--;
	if (core->refcnt >= 0) {
		if (core->refcnt == 0)
			dev_info(ctx->dev,
				"[%s] last user(%d),cnt(%d)\n",
				__func__,
				core->aov_sensor_id, core->refcnt);
		else
			dev_info(ctx->dev,
				"[%s] multi user(%d),cnt(%d)\n",
				__func__,
				core->aov_sensor_id, core->refcnt);

		/* AP side to SCP */
		if (core->aov_csi_clk_switch_flag == CSI_CLK_130) {
			/* set the parent of clk as parent_clk */
			if (core->clk[ctx->clk_index] && core->clk[CLK_TOP_AOV_STEP0]) {
				ret = clk_set_parent(
					core->clk[ctx->clk_index],
					core->clk[CLK_TOP_AOV_STEP0]);
				if (ret < 0) {
					dev_info(ctx->dev,
						"[%s] clk[%u]:%s set_parent clk[CLK_TOP_AOV_STEP0:%u]:%s(fail),ret(%d)\n",
						__func__,
						ctx->clk_index, clk_names[ctx->clk_index],
						CLK_TOP_AOV_STEP0, clk_names[CLK_TOP_AOV_STEP0],
						ret);
					mutex_unlock(&core->mutex);
					return ret;
				}
				dev_info(ctx->dev,
					"[%s] clk[%u]:%s set_parent clk[CLK_TOP_AOV_STEP0:%u]:%s(correct),ret(%d)\n",
					__func__,
					ctx->clk_index, clk_names[ctx->clk_index],
					CLK_TOP_AOV_STEP0, clk_names[CLK_TOP_AOV_STEP0],
					ret);
			} else {
				dev_info(ctx->dev,
					"[%s] Please check clk get whether NULL?\n",
					__func__);
				mutex_unlock(&core->mutex);
				return -EINVAL;
			}
		}
		/* disable seninf csi clk which connects aov sensor */
		if (core->clk[ctx->clk_index]) {
			clk_disable_unprepare(core->clk[ctx->clk_index]);
			dev_info(ctx->dev,
				"[%s] clk_disable_unprepare clk[%u]:%s\n",
				__func__,
				ctx->clk_index, clk_names[ctx->clk_index]);
		}

		if (core->refcnt == 0) {
			/* disable tsrec timer clk */
			mtk_cam_seninf_tsrec_timer_enable(0);

			/* one of the source clk of TSREC */
			if (core->clk[CLK_TOP_CAMTM]) {
				clk_disable_unprepare(core->clk[CLK_TOP_CAMTM]);
				dev_info(ctx->dev,
					"[%s] clk_disable_unprepare clk[CLK_TOP_CAMTM:%u]:%s\n",
					__func__, CLK_TOP_CAMTM, clk_names[CLK_TOP_CAMTM]);
			}

			/* disable cg (cam, seninf, camtg) */
			for (i = CLK_TOP_SENINF - 1; i >= 0; --i) {
				if (core->clk[i]) {
					clk_disable_unprepare(core->clk[i]);
					dev_info(ctx->dev,
						"[%s] clk_disable_unprepare clk[%d]:%s\n",
						__func__, i, clk_names[i]);
				}
			}

			seninf_core_pm_runtime_put(core);
		}
	}
#endif
	mutex_unlock(&core->mutex);

	return 0;
}
EXPORT_SYMBOL(mtk_cam_seninf_aov_runtime_suspend);

int mtk_cam_seninf_aov_runtime_resume(unsigned int sensor_id,
	enum AOV_DEINIT_TYPE aov_seninf_deinit_type)
{
	struct seninf_ctx *ctx = NULL;
	unsigned int real_sensor_id = 0;
	struct seninf_core *core = NULL;
#ifdef AOV_SUSPEND_RESUME_USE_PM_CLK
	int ret = 0;
#endif
	int i = 0;

	pr_info("[%s] sensor_id(%d),aov_seninf_deinit_type(%u)\n",
		__func__, sensor_id, aov_seninf_deinit_type);

	if (g_aov_param.is_test_model) {
		real_sensor_id = 5;
	} else {
		if (sensor_id == g_aov_param.sensor_idx) {
			real_sensor_id = g_aov_param.sensor_idx;
			pr_info("input sensor id(%u)(success)\n", real_sensor_id);
		} else {
			real_sensor_id = sensor_id;
			pr_info("input sensor id(%u)(fail)\n", real_sensor_id);
			seninf_aee_print(
				"[AEE] [%s] input sensor id(%u)(fail)",
				__func__, real_sensor_id);
			return -ENODEV;
		}
	}
	/* debug use
	 * if (g_aov_param.sensor_idx)
	 *	pr_info("g_aov_param.sensor_idx %d\n",
	 *		g_aov_param.sensor_idx);
	 */
	if (aov_ctx[real_sensor_id] != NULL) {
		pr_info("[%s] sensor idx(%u)\n", __func__, real_sensor_id);
		ctx = aov_ctx[real_sensor_id];
#ifdef SENSING_MODE_READY
		if (!g_aov_param.is_test_model) {
			/* switch i2c bus scl from scp to apmcu */
			aov_switch_i2c_bus_scl_aux(ctx, SCL13);
			/* switch i2c bus sda from scp to apmcu */
			aov_switch_i2c_bus_sda_aux(ctx, SDA13);
			/* restore aov pm ops: pm_stay_awake */
			aov_switch_pm_ops(ctx, AOV_PM_STAY_AWAKE);
		}
#endif
	} else {
		pr_info("[%s] Can't find ctx from input sensor id!\n", __func__);
		return -ENODEV;
	}

	core = ctx->core;
	mutex_lock(&core->mutex);

	core->pwr_refcnt_for_aov--;
	if (core->pwr_refcnt_for_aov < 0) {
		dev_info(ctx->dev,
			"[%s] please check aov_deinit times?(%d)\n",
			__func__,
			core->pwr_refcnt_for_aov);
		mutex_unlock(&core->mutex);
		return -ENODEV;
	}
	dev_info(ctx->dev,
		"[%s] pwr_refcnt_for_aov(%d)\n",
		__func__,
		core->pwr_refcnt_for_aov);
#ifdef AOV_SUSPEND_RESUME_USE_PM_CLK
	core->refcnt++;
	if (core->refcnt >= 0) {
		if (core->refcnt == 1) {
			dev_info(ctx->dev,
				"[%s] 1st user(%d),cnt(%d)\n",
				__func__,
				core->aov_sensor_id, core->refcnt);
			/* power-domains enable */
			seninf_core_pm_runtime_get_sync(core);
			dev_info(ctx->dev, "[%s] phya clock source switch to scp side\n", __func__);
			/*
			 * enable seninf cg
			 * including cam, seninf, camtg
			 */
			for (i = 0; i < CLK_TOP_SENINF; ++i) {
				if (core->clk[i]) {
					ret = clk_prepare_enable(core->clk[i]);
					if (ret < 0) {
						dev_info(ctx->dev,
							"[%s] clk_prepare_enable clk[%u]:%s(fail),ret(%d)\n",
							__func__, i, clk_names[i], ret);
						mutex_unlock(&core->mutex);
						return ret;
					}
					dev_info(ctx->dev,
						"[%s] clk_prepare_enable clk[%u]:%s(success),ret(%d)\n",
						__func__, i, clk_names[i], ret);
				}
			}
			/* one of the source clk of TSREC */
			if (core->clk[CLK_TOP_CAMTM]) {
				ret = clk_prepare_enable(core->clk[CLK_TOP_CAMTM]);
				if (ret < 0) {
					dev_info(ctx->dev,
						"[%s] clk_prepare_enable clk[CLK_TOP_CAMTM:%u]:%s(fail),ret(%d)\n",
						__func__, CLK_TOP_CAMTM, clk_names[CLK_TOP_CAMTM], ret);
					mutex_unlock(&core->mutex);
					return ret;
				}
				dev_info(ctx->dev,
					"[%s] clk_prepare_enable clk[CLK_TOP_CAMTM:%u]:%s(success),ret(%d)\n",
					__func__, CLK_TOP_CAMTM, clk_names[CLK_TOP_CAMTM], ret);
				/* enable tsrec timer clk */
				mtk_cam_seninf_tsrec_timer_enable(1);
			}
		} else
			dev_info(ctx->dev,
				"[%s] multi user(%d),cnt(%d)\n",
				__func__,
				core->aov_sensor_id, core->refcnt);

		/* enable seninf csi clk which connects aov sensor */
		if (core->clk[ctx->clk_index]) {
			/* must enable mux clk before clk_set_parent */
			ret = clk_prepare_enable(core->clk[ctx->clk_index]);
			if (ret < 0) {
				dev_info(ctx->dev,
					"[%s] prepare_enable clk[%u]:%s(fail),ret(%d)\n",
					__func__,
					ctx->clk_index, clk_names[ctx->clk_index],
					ret);
				mutex_unlock(&core->mutex);
				return ret;
			}
			dev_info(ctx->dev,
				"[%s] prepare_enable clk[%u]:%s(correct),ret(%d)\n",
				__func__,
				ctx->clk_index, clk_names[ctx->clk_index],
				ret);
		} else {
			dev_info(ctx->dev,
				"[%s] Please check clk get whether NULL?\n",
				__func__);
			mutex_unlock(&core->mutex);
			return -EINVAL;
		}
		/* SCP side to AP */
		if (core->aov_csi_clk_switch_flag == CSI_CLK_130) {
			/* set the parent of clk as parent_clk */
			if (core->clk[ctx->clk_index] && core->clk[ctx->clk_src_index]) {
				ret = clk_set_parent(
					core->clk[ctx->clk_index],
					core->clk[ctx->clk_src_index]);
				if (ret < 0) {
					dev_info(ctx->dev,
						"[%s] clk[%u]:%s set_parent clk_src[%u]:%s(fail),ret(%d)\n",
						__func__,
						ctx->clk_index, clk_names[ctx->clk_index],
						ctx->clk_src_index,
						clk_names[ctx->clk_src_index],
						ret);
					mutex_unlock(&core->mutex);
					return ret;
				}
				dev_info(ctx->dev,
					"[%s] clk[%u]:%s set_parent clk_src[%u]:%s(correct),ret(%d)\n",
					__func__,
					ctx->clk_index, clk_names[ctx->clk_index],
					ctx->clk_src_index, clk_names[ctx->clk_src_index],
					ret);
			} else {
				dev_info(ctx->dev,
					"[%s] Please check clk get whether NULL?\n",
					__func__);
				mutex_unlock(&core->mutex);
				return -EINVAL;
			}
		}
		/* set register to switch phya clock source */
		ret = enable_phya_clk(ctx);
		if (ret < 0) {
			dev_info(ctx->dev,
				"[%s] fail to set phya osc source clk\n", __func__);
			mutex_unlock(&core->mutex);
			return ret;
		}
		dev_info(ctx->dev, "[%s] phya clock source switch to scp side\n", __func__);
	}
#endif
	mutex_unlock(&core->mutex);

	switch (aov_seninf_deinit_type) {
	case DEINIT_ABNORMAL_SCP_STOP:
		dev_info(ctx->dev,
			"[%s] deinit type is abnormal(%u)!\n",
			__func__, aov_seninf_deinit_type);
		core->aov_abnormal_deinit_flag = 1;
		core->aov_abnormal_deinit_usr_fd_kill_flag = 0;
		/* seninf/sensor streaming off */
		seninf_s_stream(&ctx->subdev, 0);
		break;
	case DEINIT_ABNORMAL_USR_FD_KILL:
		dev_info(ctx->dev,
			"[%s] deinit type is abnormal(%u)!\n",
			__func__, aov_seninf_deinit_type);
		core->aov_abnormal_deinit_flag = 1;
		core->aov_abnormal_deinit_usr_fd_kill_flag = 1;
		/* seninf/sensor streaming off */
		seninf_s_stream(&ctx->subdev, 0);
		break;
	case DEINIT_NORMAL:
	default:
		dev_info(ctx->dev,
			"[%s] deinit type is normal(%u)!\n",
			__func__, aov_seninf_deinit_type);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_cam_seninf_aov_runtime_resume);

int mtk_cam_seninf_aov_reset_sensor(unsigned int sensor_id)
{
	struct seninf_ctx *ctx = NULL;
	unsigned int real_sensor_id = 0;
	struct seninf_core *core = NULL;
	int reset_by_user = 0;

	pr_info("[%s] sensor_id(%d)\n", __func__, sensor_id);

	if (g_aov_param.is_test_model) {
		real_sensor_id = 5;
	} else {
		if (sensor_id == g_aov_param.sensor_idx) {
			real_sensor_id = g_aov_param.sensor_idx;
			pr_info("input sensor id(%u)(success)\n", real_sensor_id);
		} else {
			real_sensor_id = sensor_id;
			pr_info("input sensor id(%u)(fail)\n", real_sensor_id);
			seninf_aee_print(
				"[AEE] [%s] input sensor id(%u)(fail)",
				__func__, real_sensor_id);
			return -ENODEV;
		}
	}
	/* debug use
	 * if (g_aov_param.sensor_idx)
	 *	pr_info("g_aov_param.sensor_idx %d\n",
	 *		g_aov_param.sensor_idx);
	 */
	if (aov_ctx[real_sensor_id] != NULL) {
		pr_info("[%s] sensor idx(%u)\n", __func__, real_sensor_id);
		ctx = aov_ctx[real_sensor_id];
	} else {
		pr_info("[%s] Can't find ctx from input sensor id!\n", __func__);
		return -ENODEV;
	}

	core = ctx->core;
	mutex_lock(&core->mutex);

	reset_by_user = is_reset_by_user(ctx);
	if (!reset_by_user && !g_aov_param.is_test_model) {
		/* switch i2c bus scl from scp to apmcu */
		aov_switch_i2c_bus_scl_aux(ctx, SCL13);
		/* switch i2c bus sda from scp to apmcu */
		aov_switch_i2c_bus_sda_aux(ctx, SDA13);
		/* restore aov pm ops: pm_stay_awake */
		aov_switch_pm_ops(ctx, AOV_PM_STAY_AWAKE);
		reset_sensor(ctx);
		/* switch i2c bus scl from apmcu to scp */
		aov_switch_i2c_bus_scl_aux(ctx, SCL3);
		/* switch i2c bus sda from apmcu to scp */
		aov_switch_i2c_bus_sda_aux(ctx, SDA3);
		/* switch aov pm ops: pm_relax */
		aov_switch_pm_ops(ctx, AOV_PM_RELAX);
	}

	mutex_unlock(&core->mutex);

	return 0;
}
EXPORT_SYMBOL(mtk_cam_seninf_aov_reset_sensor);

int mtk_cam_seninf_aov_sensor_set_mclk(unsigned int sensor_id, bool enable)
{
	struct seninf_ctx *ctx = NULL;
	unsigned int real_sensor_id = 0;
	struct v4l2_subdev *sensor_sd = NULL;
	struct v4l2_ctrl *ctrl = NULL;

	pr_info("[%s] sensor_id(%d)\n", __func__, sensor_id);

	if (g_aov_param.is_test_model) {
		real_sensor_id = 5;
	} else {
		if (sensor_id == g_aov_param.sensor_idx) {
			real_sensor_id = g_aov_param.sensor_idx;
			pr_info("input sensor id(%u)(success)\n", real_sensor_id);
		} else {
			real_sensor_id = sensor_id;
			pr_info("input sensor id(%u)(fail)\n", real_sensor_id);
			seninf_aee_print(
				"[AEE] [%s] input sensor id(%u)(fail)",
				__func__, real_sensor_id);
			return -ENODEV;
		}
	}
	/* debug use
	 * if (g_aov_param.sensor_idx)
	 *	pr_info("g_aov_param.sensor_idx %d\n",
	 *		g_aov_param.sensor_idx);
	 */
	if (aov_ctx[real_sensor_id] != NULL) {
		pr_info("[%s] sensor idx(%u)\n", __func__, real_sensor_id);
		ctx = aov_ctx[real_sensor_id];
	} else {
		pr_info("[%s] Can't find ctx from input sensor id!\n", __func__);
		return -ENODEV;
	}

	sensor_sd = ctx->sensor_sd;
	ctrl = v4l2_ctrl_find(sensor_sd->ctrl_handler,
			V4L2_CID_MTK_SENSOR_SET_AOV_MCLK);
	if (!ctrl) {
		dev_info(ctx->dev, "V4L2_CID_MTK_SENSOR_SET_AOV_MCLK %s\n",
			sensor_sd->name);
		return -EINVAL;
	}

	v4l2_ctrl_s_ctrl(ctrl, enable);

	return 0;
}
EXPORT_SYMBOL(mtk_cam_seninf_aov_sensor_set_mclk);
