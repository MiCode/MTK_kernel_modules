// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define_v4l2.h"
#include "kd_imgsensor_errcode.h"

#include "adaptor-subdrv.h"
#include "adaptor-i2c.h"

#include "max96712isxmipiyuv_Sensor.h"

#define PFX "max96712isx_mipi_yuv_camera_sensor"

#define VC                          0

#define SUPERFRAME_WIDTH            5120
#define SUPERFRAME_HEIGHT           720

#define FRAME_WIDTH                 1280
#define FRAME_HEIGHT                720

#define MAX96712ISX_ID_REG          0x00U
#define MAX96712ISX_SENSOR_ID       0x54

#define read_cmos_sensor(...) subdrv_i2c_rd_u8(__VA_ARGS__)
#define write_cmos_sensor(...) subdrv_i2c_wr_u8(__VA_ARGS__)

static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt);
/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
};


static int i2c_write(struct subdrv_ctx *ctx, u16 addr, u16 reg, u8 val)
{
	int ret;
	u8 buf[3];
	struct i2c_msg msg;
	struct i2c_client *client = ctx->i2c_client;

	buf[0] = reg & 0xff;
	buf[1] = val;

	msg.addr = addr;
	msg.flags = client->flags;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0)
		dev_info(&client->dev, "sub 96701 i2c transfer failed (%d)\n", ret);

	return ret;
}

static int write_cmos_max96701_sensor(struct subdrv_ctx *ctx, u32 reg, kal_uint32 para)
{
	u16 reg_addr = reg & 0x00FF;
	u8 val = para & 0xFF;
	kal_uint8 retry = 3;

	do {
		if (i2c_write(ctx, 0x40, reg_addr, val) >= 0)
			break;
			DRV_LOG(ctx, "sub fail [write]reg addr = 0x%x reg value = 0x%x", reg_addr, para);
			mdelay(1);
			retry--;

		} while (retry > 0);
	return 0;
}

static DEFINE_SPINLOCK(imgsensor_drv_lock);
static DEFINE_MUTEX(gsMutex_imgsensor_drv);

static int max96712_sensor_init(struct subdrv_ctx *ctx)
{

	DRV_LOG(ctx, "--MAX96712-ISX019-sensor_init start\n");
		write_cmos_sensor(ctx, 0x0017, 0x14);
		write_cmos_sensor(ctx, 0x0019, 0x94);
		write_cmos_sensor(ctx, 0x0013, 0x75);
		msleep(50);
		write_cmos_sensor(ctx, 0x040B, 0x00);
		write_cmos_sensor(ctx, 0x0006, 0x00);
		/*Turn on HIM on MAX96712*/
		write_cmos_sensor(ctx, 0x0B06, 0xEF);
		write_cmos_sensor(ctx, 0x0C06, 0xEF);
		write_cmos_sensor(ctx, 0x0D06, 0xEF);
		write_cmos_sensor(ctx, 0x0E06, 0xEF);
		/*disable HS/VS processing*/
		write_cmos_sensor(ctx, 0x0B0F, 0x01);
		write_cmos_sensor(ctx, 0x0C0F, 0x01);
		write_cmos_sensor(ctx, 0x0D0F, 0x01);
		write_cmos_sensor(ctx, 0x0E0F, 0x01);
		write_cmos_sensor(ctx, 0x0B07, 0x84);
		write_cmos_sensor(ctx, 0x0C07, 0x84);
		write_cmos_sensor(ctx, 0x0D07, 0x84);
		write_cmos_sensor(ctx, 0x0E07, 0x84);
		/*YUV MUX mode */
		write_cmos_sensor(ctx, 0x041A, 0xF0);
		/* enable 4 pipe*/
		/*write_cmos_sensor(ctx, 0x00F4,0x0F);*/
		write_cmos_sensor(ctx, 0x040B, 0x40);
		write_cmos_sensor(ctx, 0x040C, 0x00);
		write_cmos_sensor(ctx, 0x040D, 0x00);
		write_cmos_sensor(ctx, 0x040E, 0x5E);
		write_cmos_sensor(ctx, 0x040F, 0x7E);
		write_cmos_sensor(ctx, 0x0410, 0x7A);
		write_cmos_sensor(ctx, 0x0411, 0x48);
		write_cmos_sensor(ctx, 0x0412, 0x20);
		write_cmos_sensor(ctx, 0x08A0, 0x04);
		write_cmos_sensor(ctx, 0x08A3, 0xE4);
		write_cmos_sensor(ctx, 0x08A4, 0xE4);
		write_cmos_sensor(ctx, 0x090A, 0xC0);
		write_cmos_sensor(ctx, 0x094A, 0xC0);
		write_cmos_sensor(ctx, 0x098A, 0xC0);
		write_cmos_sensor(ctx, 0x09CA, 0xC0);
		write_cmos_sensor(ctx, 0x08A2, 0xF0);
		write_cmos_sensor(ctx, 0x1C00, 0xF4);
		write_cmos_sensor(ctx, 0x1D00, 0xF4);
		write_cmos_sensor(ctx, 0x1E00, 0xF4);
		write_cmos_sensor(ctx, 0x1F00, 0xF4);
		write_cmos_sensor(ctx, 0x0415, 0xEF);
		write_cmos_sensor(ctx, 0x0418, 0xEF);
		write_cmos_sensor(ctx, 0x1C00, 0xF5);
		write_cmos_sensor(ctx, 0x1D00, 0xF5);
		write_cmos_sensor(ctx, 0x1E00, 0xF5);
		write_cmos_sensor(ctx, 0x1F00, 0xF5);
		write_cmos_sensor(ctx, 0x090B, 0x07);
		write_cmos_sensor(ctx, 0x092D, 0x15);
		write_cmos_sensor(ctx, 0x090D, 0x1E);
		write_cmos_sensor(ctx, 0x090E, 0x1E);
		write_cmos_sensor(ctx, 0x090F, 0x00);
		write_cmos_sensor(ctx, 0x0910, 0x00);
		write_cmos_sensor(ctx, 0x0911, 0x01);
		write_cmos_sensor(ctx, 0x0912, 0x01);
		write_cmos_sensor(ctx, 0x094B, 0x07);
		write_cmos_sensor(ctx, 0x096D, 0x15);
		write_cmos_sensor(ctx, 0x094D, 0x1E);
		write_cmos_sensor(ctx, 0x094E, 0x5E);
		write_cmos_sensor(ctx, 0x094F, 0x00);
		write_cmos_sensor(ctx, 0x0950, 0x40);
		write_cmos_sensor(ctx, 0x0951, 0x01);
		write_cmos_sensor(ctx, 0x0952, 0x41);
		write_cmos_sensor(ctx, 0x098B, 0x07);
		write_cmos_sensor(ctx, 0x09AD, 0x15);
		write_cmos_sensor(ctx, 0x098D, 0x1E);
		write_cmos_sensor(ctx, 0x098E, 0x9E);
		write_cmos_sensor(ctx, 0x098F, 0x00);
		write_cmos_sensor(ctx, 0x0990, 0x80);
		write_cmos_sensor(ctx, 0x0991, 0x01);
		write_cmos_sensor(ctx, 0x0992, 0x81);
		write_cmos_sensor(ctx, 0x09CB, 0x07);
		write_cmos_sensor(ctx, 0x09ED, 0x15);
		write_cmos_sensor(ctx, 0x09CD, 0x1E);
		write_cmos_sensor(ctx, 0x09CE, 0xDE);
		write_cmos_sensor(ctx, 0x09CF, 0x00);
		write_cmos_sensor(ctx, 0x09D0, 0xC0);
		write_cmos_sensor(ctx, 0x09D1, 0x01);
		write_cmos_sensor(ctx, 0x09D2, 0xC1);
		write_cmos_sensor(ctx, 0x0006, 0x0F);
		mdelay(10);
		write_cmos_sensor(ctx, 0x0010, 0x11);
		write_cmos_sensor(ctx, 0x0011, 0x11);
		write_cmos_sensor(ctx, 0x0018, 0x0F);
		msleep(100);
		write_cmos_max96701_sensor(ctx, 0x04, 0x43);
		mdelay(5);
		write_cmos_max96701_sensor(ctx, 0x07, 0x84);
		write_cmos_max96701_sensor(ctx, 0x47, 0x32);
		write_cmos_max96701_sensor(ctx, 0x43, 0x25);
		write_cmos_max96701_sensor(ctx, 0x67, 0xc4);
		write_cmos_max96701_sensor(ctx, 0x4d, 0xC0);
		write_cmos_max96701_sensor(ctx, 0x0e, 0x02);
		write_cmos_max96701_sensor(ctx, 0x0f, 0xbf);
		write_cmos_max96701_sensor(ctx, 0x02, 0x1f);
		/*crossbar*/
		write_cmos_max96701_sensor(ctx, 0x20, 0x07);
		write_cmos_max96701_sensor(ctx, 0x21, 0x06);
		write_cmos_max96701_sensor(ctx, 0x22, 0x05);
		write_cmos_max96701_sensor(ctx, 0x23, 0x04);
		write_cmos_max96701_sensor(ctx, 0x24, 0x03);
		write_cmos_max96701_sensor(ctx, 0x25, 0x02);
		write_cmos_max96701_sensor(ctx, 0x26, 0x01);
		write_cmos_max96701_sensor(ctx, 0x27, 0x00);
		write_cmos_max96701_sensor(ctx, 0x30, 0x17);
		write_cmos_max96701_sensor(ctx, 0x31, 0x16);
		write_cmos_max96701_sensor(ctx, 0x32, 0x15);
		write_cmos_max96701_sensor(ctx, 0x33, 0x14);
		write_cmos_max96701_sensor(ctx, 0x34, 0x13);
		write_cmos_max96701_sensor(ctx, 0x35, 0x12);
		write_cmos_max96701_sensor(ctx, 0x36, 0x11);
		write_cmos_max96701_sensor(ctx, 0x37, 0x10);
		/*crossbar*/
		write_cmos_sensor(ctx, 0x0006, 0x0F);
		mdelay(20);
		write_cmos_max96701_sensor(ctx, 0x04, 0x83);
		mdelay(20);
		//write_cmos_sensor(ctx, 0x040B, 0x42);
		write_cmos_sensor(ctx, 0x0001, 0xCC);
		/*write_cmos_sensor(ctx, 0x08A0, 0x84);*/
		write_cmos_sensor(ctx, 0x00FA, 0x50);

	DRV_LOG(ctx, "--MAX96712 ISX019-- sensor init end\n");
	return 0;
}

static int set_streaming_control(void *arg, bool enable)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	DRV_LOG(ctx, "streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable) {
		/*turn on mipi*/
		write_cmos_sensor(ctx, 0x040B, 0x42);
		mdelay(5);
	} else {
		/*turn off mipi*/
		write_cmos_sensor(ctx, 0x040B, 0x0);
		mdelay(5);
	}

	return ERROR_NONE;
}

static int return_sensor_id(struct subdrv_ctx *ctx)
{
		if(read_cmos_sensor(ctx, MAX96712ISX_ID_REG) == 0x52)
			return MAX96712ISX_SENSOR_ID;
		else
			return 0xff;
}

static int Chenck_Sensor_Signal(struct subdrv_ctx *ctx)
{
	int linka, linkb, linkc, linkd;

	//link lock check , linka:0xda
	linka = read_cmos_sensor(ctx, 0xBCB)&0x1;
	linkb = read_cmos_sensor(ctx, 0xCCB)&0x1;
	linkc = read_cmos_sensor(ctx, 0xDCB)&0x1;
	linkd = read_cmos_sensor(ctx, 0xECB)&0x1;
	DRV_LOG(ctx, "Sensor link lock Status:linka: 0x%x linkb: 0x%x linkc: 0x%x linkd: 0x%x\n",
		linka, linkb, linkc, linkd);
	if ((linka == 0x0) || (linkb == 0x0) || (linkc == 0x0) || (linkd == 0x0))
		return 1;
	else
		return 0;
}

static const struct subdrv_ctx defctx = {
	.ana_gain_def = 0x100,
	.ana_gain_max = 1024,
	.ana_gain_min = 64,
	.ana_gain_step = 1,
	.exposure_def = 0x3D0,
	.exposure_max = 0xffff - 61,
	.exposure_min = 6,
	.exposure_step = 1,
	.frame_time_delay_frame = 3,
	.margin = 61,
	.max_frame_length = 0xffff,
	//.hdr_cap = HDR_CAP_3HDR | HDR_CAP_ATR,

	.mirror = IMAGE_NORMAL, /* mirrorflip information */
	/* IMGSENSOR_MODE enum value,record current sensor mode,such as:
	 *  INIT, Preview, Capture, Video,High Speed Video, Slim Video
	 */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0, /* current shutter */
	.gain = 0x100, /* current gain */
	.dummy_pixel = 0, /* current dummypixel */
	.dummy_line = 0, /* current dummyline */
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW,
	.ihdr_mode = 0, /* sensor need support LE, SE with HDR feature */
	.hdr_mode = 0, /* HDR mODE : 0: disable HDR, 1:IHDR, 2:HDR, 9:ZHDR */
	.i2c_write_id = 0x52,
};

static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x1E, //yuv422
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
			.user_data_desc = VC_BRIDGE_RAW_0,
		},
	},
	{
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x1E, //yuv422
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
			.user_data_desc = VC_BRIDGE_RAW_1,
		},
	},
	{
		.bus.csi2 = {
			.channel = 2,
			.data_type = 0x1E, //yuv422
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
			.user_data_desc = VC_BRIDGE_RAW_2,
		},
	},
	{
		.bus.csi2 = {
			.channel = 3,
			.data_type = 0x1E, //yuv422
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
			.user_data_desc = VC_BRIDGE_RAW_3,
		},
	},
};


static struct subdrv_mode_struct mode_struct[] = {
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = PARAM_UNDEFINED,
		.mode_setting_len = PARAM_UNDEFINED,
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 205100000,
		.linelength = 5860,
		.framelength = 1400,
		.max_framerate = 300,
		.mipi_pixel_rate = 200000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_UYVY,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = SUPERFRAME_WIDTH,
			.full_h = SUPERFRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = SUPERFRAME_WIDTH,
			.h0_size = SUPERFRAME_HEIGHT,
			.scale_w = SUPERFRAME_WIDTH,
			.scale_h = SUPERFRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = SUPERFRAME_WIDTH,
			.h1_size = SUPERFRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = SUPERFRAME_WIDTH,
			.h2_tg_size = SUPERFRAME_HEIGHT,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_trail = 0x18,
			.dphy_data_settle = 0x27,
			.cphy_settle = 0x13,
			.clk_lane_no_initial_flow = 0,
		},
		.dpc_enabled = true, /* reg 0x0b06 */
	}, /* preview */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = PARAM_UNDEFINED,
		.mode_setting_len = PARAM_UNDEFINED,
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 205100000,
		.linelength = 5860,
		.framelength = 1400,
		.max_framerate = 300,
		.mipi_pixel_rate = 200000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_UYVY,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = SUPERFRAME_WIDTH,
			.full_h = SUPERFRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = SUPERFRAME_WIDTH,
			.h0_size = SUPERFRAME_HEIGHT,
			.scale_w = SUPERFRAME_WIDTH,
			.scale_h = SUPERFRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = SUPERFRAME_WIDTH,
			.h1_size = SUPERFRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = SUPERFRAME_WIDTH,
			.h2_tg_size = SUPERFRAME_HEIGHT,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_trail = 0x18,
			.dphy_data_settle = 0x27,
			.cphy_settle = 0x13,
			.clk_lane_no_initial_flow = 0,
		},
		.dpc_enabled = true, /* reg 0x0b06 */
	}, /* capture */
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = PARAM_UNDEFINED,
		.mode_setting_len = PARAM_UNDEFINED,
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 205100000,
		.linelength = 5860,
		.framelength = 1400,
		.max_framerate = 300,
		.mipi_pixel_rate = 200000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_UYVY,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = SUPERFRAME_WIDTH,
			.full_h = SUPERFRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = SUPERFRAME_WIDTH,
			.h0_size = SUPERFRAME_HEIGHT,
			.scale_w = SUPERFRAME_WIDTH,
			.scale_h = SUPERFRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = SUPERFRAME_WIDTH,
			.h1_size = SUPERFRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = SUPERFRAME_WIDTH,
			.h2_tg_size = SUPERFRAME_HEIGHT,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_trail = 0x18,
			.dphy_data_settle = 0x27,
			.cphy_settle = 0x13,
			.clk_lane_no_initial_flow = 0,
		},
		.dpc_enabled = true, /* reg 0x0b06 */
	}, /*video*/
	{
		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = PARAM_UNDEFINED,
		.mode_setting_len = PARAM_UNDEFINED,
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 205100000,
		.linelength = 5860,
		.framelength = 1400,
		.max_framerate = 300,
		.mipi_pixel_rate = 200000000,
		.readout_length = 0,
		.read_margin = 10,
		.framelength_step = 2,
		.coarse_integ_step = 2,
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_UYVY,
		.multi_exposure_shutter_range[IMGSENSOR_EXPOSURE_LE].min = 8,
		.imgsensor_winsize_info = {
			.full_w = SUPERFRAME_WIDTH,
			.full_h = SUPERFRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = SUPERFRAME_WIDTH,
			.h0_size = SUPERFRAME_HEIGHT,
			.scale_w = SUPERFRAME_WIDTH,
			.scale_h = SUPERFRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = SUPERFRAME_WIDTH,
			.h1_size = SUPERFRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = SUPERFRAME_WIDTH,
			.h2_tg_size = SUPERFRAME_HEIGHT,
		},
		.pdaf_cap = TRUE,
		.imgsensor_pd_info = NULL,
		.ae_binning_ratio = 1465,
		.fine_integ_line = 826,
		.delay_frame = 3,
		.csi_param = {
			.legacy_phy = 0,
			.not_fixed_trail_settle = 1,
			.not_fixed_dphy_settle = 1,
			.dphy_trail = 0x18,
			.dphy_data_settle = 0x27,
			.cphy_settle = 0x13,
			.clk_lane_no_initial_flow = 0,
		},
		.dpc_enabled = true, /* reg 0x0b06 */
	}, /* hs video*/
};

static struct subdrv_static_ctx static_ctx = {
	.sensor_id = MAX96712ISXMIPI_SENSOR_ID,
	.reg_addr_sensor_id = {0x0000},
	.i2c_addr_table = {0x52, 0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	.eeprom_info = 0,
	.eeprom_num = 0,
	.resolution = {SUPERFRAME_WIDTH, SUPERFRAME_HEIGHT},
	.mirror = IMAGE_HV_MIRROR,

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.ob_pedestal = 0x40,
	.cam_type = SENSOR_TYPE_NON_COMB_AVM,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_UYVY,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 64,
	.ana_gain_type = 0,
	.ana_gain_step = 1,
	.ana_gain_table = PARAM_UNDEFINED,
	.ana_gain_table_size = PARAM_UNDEFINED,
	.tuning_iso_base = 100,
	.exposure_def = 0x3D0,
	.exposure_min = 24,
	.exposure_max = 128 * (0xFFFC - 48),
	.exposure_step = 4,
	.exposure_margin = 48,
	.dig_gain_min = BASE_DGAIN * 1,
	.dig_gain_max = BASE_DGAIN * 16,
	.dig_gain_step = 4,

	.frame_length_max = 0xFFFC,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 3,
	.start_exposure_offset = 500000,

	.pdaf_type = PDAF_SUPPORT_NA,
	.hdr_type = HDR_SUPPORT_NA,
	.seamless_switch_support = FALSE,
	.seamless_switch_type = SEAMLESS_SWITCH_CUT_VB_INIT_SHUT,
	.seamless_switch_hw_re_init_time_ns = 2750000,
	.seamless_switch_prsh_hw_fixed_value = 32,
	.seamless_switch_prsh_length_lc = 0,
	.reg_addr_prsh_length_lines = {0x3039, 0x303a, 0x303b},
	.reg_addr_prsh_mode = 0x3036,

	.temperature_support = FALSE,

	.g_temp = NULL,
	.g_gain2reg = NULL,
	.s_gph = NULL,
	.s_cali = NULL,
	.s_streaming_control = set_streaming_control,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = 0x0101,
	.reg_addr_exposure = {
			{0x0202, 0x0203},
			{0x313A, 0x313B},
			{0x0224, 0x0225},
	},
	.long_exposure_support = TRUE,
	.reg_addr_exposure_lshift = 0x3128,
	.reg_addr_ana_gain = {
			{0x0204, 0x0205},
			{0x313C, 0x313D},
			{0x0216, 0x0217},
	},
	.reg_addr_dig_gain = {
			{0x020E, 0x020F},
			{0x313E, 0x313F},
			{0x0218, 0x0219},
	},
	.reg_addr_frame_length = {0x0340, 0x0341},
	.reg_addr_temp_en = 0x0138,
	.reg_addr_temp_read = 0x013A,
	.reg_addr_auto_extend = 0x0350,
	.reg_addr_frame_count = 0x0005,
	.reg_addr_fast_mode = 0x3010,

	.init_setting_table = PARAM_UNDEFINED,
	.init_setting_len = PARAM_UNDEFINED,
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 1,
	.chk_s_off_end = 0,

	.checksum_value = 0xc3dd012b,
	.aov_sensor_support = TRUE,
	.init_in_open = TRUE,
	.streaming_ctrl_imp = TRUE,

};

static int get_sensor_usage(struct subdrv_ctx *ctx, enum mtk_sensor_usage *usage)
{
	*usage = MTK_SENSOR_USAGE_NONCOMB;
	return 0;
}

static int get_imgsensor_id(struct subdrv_ctx *ctx, u32 *sensor_id)
{
	u8 i = 0;
	u8 retry;

	ctx->i2c_write_id = static_ctx.i2c_addr_table[0];
	retry = 2;

	while (static_ctx.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		ctx->i2c_write_id = static_ctx.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {

			*sensor_id = return_sensor_id(ctx);
			if (*sensor_id == (static_ctx.sensor_id)) {
				DRV_LOG(ctx, "i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}
			DRV_LOG(ctx, "Read id fail, id: 0x%x, i2c id: 0x%x\n",
				*sensor_id, ctx->i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != (static_ctx.sensor_id)) {
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}

static int init_ctx(struct subdrv_ctx *ctx,
		struct i2c_client *i2c_client, u8 i2c_write_id)
{
	memcpy(&(ctx->s_ctx), &static_ctx, sizeof(struct subdrv_static_ctx));
	subdrv_ctx_init(ctx);
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}

static int open(struct subdrv_ctx *ctx)
{
	u8 i = 0;
	u8 retry = 2;
	u16 sensor_id = 0;
	u32 scenario_id = 0;

	while (static_ctx.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		ctx->i2c_write_id = static_ctx.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id(ctx);
			if (sensor_id == MAX96712ISX_SENSOR_ID) {
				DRV_LOG(ctx, "i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, sensor_id);
				break;
			}
			DRV_LOG(ctx, "Read id fail, id: 0x%x, i2c id: 0x%x\n",
				sensor_id, ctx->i2c_write_id);
			retry--;
			msleep(50);
		} while (retry > 0);
		i++;
		if (sensor_id == MAX96712ISX_SENSOR_ID)
			break;
		retry = 2;
	}
	if (sensor_id != MAX96712ISX_SENSOR_ID)
		return -ERROR_SENSOR_CONNECT_FAIL;

	/* initail sequence write in */

	max96712_sensor_init(ctx);
	retry = 3;
	do {
		if (Chenck_Sensor_Signal(ctx)) {
			DRV_LOG(ctx, "Sensor Not Connect");
			mdelay(5);
			retry--;
		} else
			break;
	} while (retry > 0);

	if (retry == 0) {
		DRV_LOG(ctx, "OPEN FAIL!Sensor Not Connect");
		return -ERROR_SENSOR_CONNECT_FAIL;
	}

	/* move camera init to initctx(), void max9286 init too much time */

	spin_lock(&imgsensor_drv_lock);

	memset(ctx->exposure, 0, sizeof(ctx->exposure));
	memset(ctx->ana_gain, 0, sizeof(ctx->gain));
	ctx->exposure[0] = ctx->s_ctx.exposure_def;
	ctx->ana_gain[0] = ctx->s_ctx.ana_gain_def;
	ctx->current_scenario_id = scenario_id;
	ctx->pclk = mode_struct[scenario_id].pclk;
	ctx->line_length = mode_struct[scenario_id].linelength;
	ctx->frame_length = mode_struct[scenario_id].framelength;
	ctx->frame_length_rg = ctx->frame_length;
	ctx->current_fps = ctx->pclk / ctx->line_length * 10 / ctx->frame_length;
	ctx->readout_length = mode_struct[scenario_id].readout_length;
	ctx->read_margin = mode_struct[scenario_id].read_margin;
	ctx->min_frame_length = ctx->frame_length;
	ctx->autoflicker_en = FALSE;
	ctx->test_pattern = 0;
	ctx->ihdr_mode = 0;
	ctx->pdaf_mode = 0;
	ctx->hdr_mode = 0;
	ctx->extend_frame_length_en = 0;
	ctx->is_seamless = 0;
	ctx->fast_mode_on = 0;
	ctx->sof_cnt = 0;
	ctx->ref_sof_cnt = 0;
	ctx->is_streaming = 0;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

static int vsync_notify(struct subdrv_ctx *ctx,	unsigned int sof_cnt)
{
	DRV_LOG(ctx, "sof_cnt(%u) ctx->ref_sof_cnt(%u) ctx->fast_mode_on(%d)",
		sof_cnt, ctx->ref_sof_cnt, ctx->fast_mode_on);
	if (ctx->fast_mode_on && (sof_cnt > ctx->ref_sof_cnt)) {
		ctx->fast_mode_on = FALSE;
		ctx->ref_sof_cnt = 0;
		DRV_LOG(ctx, "seamless_switch disabled.");
		subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_prsh_mode, 0x00);
		set_i2c_buffer(ctx, 0x3010, 0x00);
		commit_i2c_buffer(ctx);
	}
	return 0;
}

static struct subdrv_ops ops = {
	.get_id = get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = open,
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = common_control,
	.feature_control = common_feature_control,
	.close = common_close,
	.get_frame_desc = common_get_frame_desc,
	.get_temp = common_get_temp,
	.get_csi_param = common_get_csi_param,
	.vsync_notify = vsync_notify,
	.update_sof_cnt = common_update_sof_cnt,
	.parse_ebd_line = common_parse_ebd_line,
	.get_sensor_usage = get_sensor_usage,
};


static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_PDN, 0, 20},
	{HW_ID_PDN, 1800000, 20},
};

const struct subdrv_entry max96712isx_mipi_yuv_entry = {
	.name = "max96712isx_mipi_yuv",
	.id = MAX96712ISXMIPI_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};
