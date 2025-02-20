// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 MediaTek Inc.

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 ox08b40mipiraw_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include "ox08b40mipiraw_Sensor.h"

#define read_cmos_sensor(...) subdrv_i2c_rd_u8(__VA_ARGS__)
#define write_cmos_sensor(...) subdrv_i2c_wr_u8(__VA_ARGS__)

#define read_cmos_sensor_with_i2cid(subctx, i2c_id ,reg) \
({ \
	u8 __val = 0xff; \
	adaptor_i2c_rd_u8(subctx->i2c_client, \
		i2c_id >> 1, reg, &__val); \
	__val; \
})
#define write_cmos_sensor_with_i2cid(subctx, i2c_id ,reg, val) \
	adaptor_i2c_wr_u8(subctx->i2c_client, \
		i2c_id >> 1, reg, val)
#define ox08b40_table_write_cmos_sensor(subctx, i2c_id ,list, len) \
	adaptor_i2c_wr_regs_u8(subctx->i2c_client, \
		i2c_id >> 1, list, len)


/************************Modify Following Strings for Debug********************/
#define PFX "ox08b40_mipi_raw_camera_sensor"
/************************   Modify end    *************************************/
#define LOG_INF(format, args...)	pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_DBG(format, args...)	pr_debug(PFX "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...)	pr_info(PFX "[%s] " format, __func__, ##args)

/************************   SENSOR related    *********************************/
#define FRAME_WIDTH							3840
#define FRAME_HEIGHT						2160

#define MAX96712_ID_REG						0x00U
#define OX08B40_I2C_ADDR                     0x6C
#define MAX96717_I2C_ADDR                   0x80
#define MAX96712_I2C_ADDR                   0x52

//static void set_sensor_cali(void *arg);
//static int get_sensor_temperature(void *arg);
//static void set_group_hold(void *arg, u8 en);
static void ox08b40_set_dummy(struct subdrv_ctx *ctx);
static int ox08b40_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static u16 get_gain2reg(u32 gain);
//static int ox08b40_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int ox08b40_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len);
static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id);

/* STRUCT */

static struct subdrv_feature_control feature_control_list[] = {
	{SENSOR_FEATURE_SET_TEST_PATTERN, ox08b40_set_test_pattern},
	//{SENSOR_FEATURE_SEAMLESS_SWITCH, ox08b40_seamless_switch},
	{SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO, ox08b40_set_max_framerate_by_scenario},
};

static int write_cmos_max96712_sensor(struct subdrv_ctx *ctx, u32 reg, kal_uint32 para)
{
	u16 reg_addr = reg & 0xFFFF;
	u8 val = para & 0xFF;
	kal_uint8 retry = 3;

	do {
		//if (i2c_write(ctx, 0x40, reg_addr, val) >= 0)
		if (write_cmos_sensor_with_i2cid(ctx, MAX96712_I2C_ADDR ,reg_addr, val) >= 0)
			break;
			DRV_LOG(ctx,"fail [write]reg addr = 0x%x reg value = 0x%x", reg_addr, para);
			retry--;
			mdelay(1);
		} while (retry > 0);
	return 0;
}

static u8 read_cmos_max96712_sensor(struct subdrv_ctx *ctx, u32 reg)
{
	u16 reg_addr = reg & 0xFFFF;
	u8 val = 0;

	val = read_cmos_sensor_with_i2cid(ctx, MAX96712_I2C_ADDR ,reg_addr);
	LOG_INF("[read]reg addr = 0x%x reg value = 0x%x", reg_addr, val);

	return val;
}

static int write_cmos_max96717_sensor(struct subdrv_ctx *ctx, u32 reg, kal_uint32 para)
{
	u16 reg_addr = reg & 0xFFFF;
	u8 val = para & 0xFF;
	kal_uint8 retry = 3;

	do {
		//if (i2c_write(ctx, 0x40, reg_addr, val) >= 0)
		if (write_cmos_sensor_with_i2cid(ctx, MAX96717_I2C_ADDR ,reg_addr, val) >= 0)
			break;
			DRV_LOG(ctx,"fail [write]reg addr = 0x%x reg value = 0x%x", reg_addr, para);
			retry--;
			mdelay(1);
		} while (retry > 0);
	return 0;
}
/*
 *static u8 read_cmos_max96717_sensor(struct subdrv_ctx *ctx, u32 reg)
 *{
 *	u16 reg_addr = reg & 0xFFFF;
 *	u8 val = 0;
 *
 *	val = read_cmos_sensor_with_i2cid(ctx, MAX96717_I2C_ADDR ,reg_addr);
 *	LOG_INF("[read]reg addr = 0x%x reg value = 0x%x", reg_addr, val);
 *
 *	return val;
 *}
 *
 *static int write_cmos_ox08b40_sensor(struct subdrv_ctx *ctx, u32 reg, kal_uint32 para)
 *{
 *	u16 reg_addr = reg & 0xFFFF;
 *	u8 val = para & 0xFF;
 *	kal_uint8 retry = 3;
 *
 *	do {
 *		//if (i2c_write(ctx, 0x40, reg_addr, val) >= 0)
 *		if (write_cmos_sensor_with_i2cid(ctx, OX08B40_I2C_ADDR ,reg_addr, val) >= 0)
 *			break;
 *			DRV_LOG(ctx, "fail [write]reg addr = 0x%x reg value = 0x%x", reg_addr, para);
 *			retry--;
 *			mdelay(1);
 *		} while (retry > 0);
 *	return 0;
 *}
 *
 *static u8 read_cmos_ox08b40_sensor(struct subdrv_ctx *ctx, u32 reg)
 *{
 *	u16 reg_addr = reg & 0xFFFF;
 *	u8 val = 0;
 *
 *	val = read_cmos_sensor_with_i2cid(ctx, OX08B40_I2C_ADDR ,reg_addr);
 *	LOG_INF("[read]reg addr = 0x%x reg value = 0x%x", reg_addr, val);
 *
 *	return val;
 *}
 */
static DEFINE_SPINLOCK(imgsensor_drv_lock);

static int max96712_sensor_init(struct subdrv_ctx *ctx)
{
	int linka, linkb, linkc, linkd;
	// int init_sz;
	// int i = 0;
	DRV_LOG(ctx, "--MAX96712-- sensor init begin\n");
	write_cmos_max96712_sensor(ctx, 0x0017, 0x14);//luna
	write_cmos_max96712_sensor(ctx, 0x0019, 0x94);//luna
	write_cmos_max96712_sensor(ctx, 0x0013, 0x75);//luna
	mdelay(50);//luna
	write_cmos_max96712_sensor(ctx, 0x040B, 0x00);
	write_cmos_max96712_sensor(ctx, 0x0006, 0x00);//luna
	write_cmos_max96712_sensor(ctx, 0x1445, 0x00);
	write_cmos_max96712_sensor(ctx, 0x1545, 0x00);
	write_cmos_max96712_sensor(ctx, 0x1645, 0x00);
	write_cmos_max96712_sensor(ctx, 0x1745, 0x00);
	write_cmos_max96712_sensor(ctx, 0x14D1, 0x03);
	write_cmos_max96712_sensor(ctx, 0x15D1, 0x03);
	write_cmos_max96712_sensor(ctx, 0x16D1, 0x03);
	write_cmos_max96712_sensor(ctx, 0x17D1, 0x03);
	write_cmos_max96712_sensor(ctx, 0x0006, 0xF1);//luna 0xff gmsl2_a linka
	write_cmos_max96712_sensor(ctx, 0x0010, 0x22);//phya/b 6gbps
	write_cmos_max96712_sensor(ctx, 0x0011, 0x22);//phyc/d 6gbps
	write_cmos_max96712_sensor(ctx, 0x0018, 0x0F);//ONE SHOT REST
	msleep(100);

	write_cmos_max96712_sensor(ctx, 0x00F0, 0x62);//config pipez phya/b
	write_cmos_max96712_sensor(ctx, 0x00F1, 0xEA);//config pipez phyc/d
	write_cmos_max96712_sensor(ctx, 0x00F4, 0x0F);//enable pipe 1-4

	//Enable alt mem map10 --bandwidth efficiency Optimization
	write_cmos_max96712_sensor(ctx, 0x0973, 0x04);

	// Video Pipe to MIPI Controller Mapping
	// RAW12, video pipe 0, map FS/FE
	write_cmos_max96712_sensor(ctx, 0x090B, 0x07);//luna 07
	write_cmos_max96712_sensor(ctx, 0x092D, 0x15);//luna 15
	write_cmos_max96712_sensor(ctx, 0x090D, 0x2B);//pipe 0 src data type raw10
	write_cmos_max96712_sensor(ctx, 0x090E, 0x2B);//pipe 0 des data type raw10
	write_cmos_max96712_sensor(ctx, 0x090F, 0x00);
	write_cmos_max96712_sensor(ctx, 0x0910, 0x00);
	write_cmos_max96712_sensor(ctx, 0x0911, 0x01);
	write_cmos_max96712_sensor(ctx, 0x0912, 0x01);

	write_cmos_max96712_sensor(ctx, 0x040B, 0x50);//luna pipe0 raw10
	write_cmos_max96712_sensor(ctx, 0x040E, 0x2B);//luna
	// Set Des,0x in 2x4 mode
	write_cmos_max96712_sensor(ctx, 0x08A0, 0x04);//2x4 mode two ports
	write_cmos_max96712_sensor(ctx, 0x08A3, 0xE4);
	write_cmos_max96712_sensor(ctx, 0x08A4, 0xE4);//new add

	write_cmos_max96712_sensor(ctx, 0x090A, 0xC0);//set 4 lanes
	write_cmos_max96712_sensor(ctx, 0x094A, 0xC0);
	write_cmos_max96712_sensor(ctx, 0x098A, 0xC0);
	write_cmos_max96712_sensor(ctx, 0x09CA, 0xC0);

	write_cmos_max96712_sensor(ctx, 0x08A2, 0xF0);//turn on mipi phys luna F4

	write_cmos_max96712_sensor(ctx, 0x1C00, 0xF4);
	write_cmos_max96712_sensor(ctx, 0x1D00, 0xF4);
	write_cmos_max96712_sensor(ctx, 0x1E00, 0xF4);
	write_cmos_max96712_sensor(ctx, 0x1F00, 0xF4);

	write_cmos_max96712_sensor(ctx, 0x0415, 0x2E);//luna 0x2E 1.4G; 0xEF 1.5G
	write_cmos_max96712_sensor(ctx, 0x0418, 0x2E);//luna 0x2E 1.4G; 0xEF 1.5G
	write_cmos_max96712_sensor(ctx, 0x1C00, 0xF5);
	write_cmos_max96712_sensor(ctx, 0x1D00, 0xF5);
	write_cmos_max96712_sensor(ctx, 0x1E00, 0xF5);
	write_cmos_max96712_sensor(ctx, 0x1F00, 0xF5);
	//msleep(200);

	LOG_INF("1\n");

	//link lock check
	linka = read_cmos_max96712_sensor(ctx, 0x1A);
	linkb = read_cmos_max96712_sensor(ctx, 0x0A);
	linkc = read_cmos_max96712_sensor(ctx, 0x0B);
	linkd = read_cmos_max96712_sensor(ctx, 0x0C);
	LOG_INF("Sensor link lock Status:linka: 0x%x linkb: 0x%x linkc: 0x%x linkd: 0x%x\n",
		linka, linkb, linkc, linkd);
		//link lock check
	linka = read_cmos_max96712_sensor(ctx, 0x26);
	linkb = read_cmos_max96712_sensor(ctx, 0x28);
	linkc = read_cmos_max96712_sensor(ctx, 0x2A);
	linkd = read_cmos_max96712_sensor(ctx, 0x2C);
	LOG_INF("96712 Status:0x26: 0x%x 0x28: 0x%x 0x2A: 0x%x 0x2C: 0x%x\n",
		linka, linkb, linkc, linkd);
	linka = read_cmos_max96712_sensor(ctx, 0x2E);
	linkb = read_cmos_max96712_sensor(ctx, 0x45);
	linkc = read_cmos_max96712_sensor(ctx, 0x47);
	linkd = read_cmos_max96712_sensor(ctx, 0x4A);
	LOG_INF("96712 Status:0x2E: 0x%x 0x45: 0x%x 0x47: 0x%x 0x4A: 0x%x\n",
		linka, linkb, linkc, linkd);

	//write_cmos_max96717_sensor(ctx, 0x030D, 0x01);//pipe only VC0
	write_cmos_max96717_sensor(ctx, 0x0383, 0x00);//
	write_cmos_max96717_sensor(ctx, 0x0318, 0x6B);//set pipe Z datatype=0x2B,raw10 //

	//Enable BPP10 double mode for pipeZ --bandwidth efficiency Optimization
	write_cmos_max96717_sensor(ctx, 0x0313, 0x04);
	write_cmos_max96717_sensor(ctx, 0x031E, 0x34);//20 bpp override on pipeZ

	write_cmos_max96717_sensor(ctx, 0x0570, 0x0C); //
	write_cmos_max96717_sensor(ctx, 0x0006, 0xb0); //
	write_cmos_max96717_sensor(ctx, 0x02d4, 0xa0);
	LOG_INF("2d3 01\n");
	write_cmos_max96717_sensor(ctx, 0x02d3, 0x10);
	msleep(255);
	LOG_INF("2d3 00\n");
	write_cmos_max96717_sensor(ctx, 0x02d3, 0x00);
	msleep(255);
	LOG_INF("2d3 01\n");
	write_cmos_max96717_sensor(ctx, 0x02d3, 0x10);
	mdelay(50);

	write_cmos_max96712_sensor(ctx, 0x0001, 0xCC);//disable pass through i2c2
	write_cmos_max96712_sensor(ctx, 0x00FA, 0x70);//MFP4=VS,MFP13=HS,VFP14=DE
	LOG_INF("2\n");

	write_cmos_sensor(ctx, 0x0103, 0x01);//write ox08b40
	write_cmos_sensor(ctx, 0x0107, 0x01);
	mdelay(10);

	//ox08b40_table_write_cmos_sensor(ctx, addr_data_pair_preview_ox08b402q,
	//	sizeof(addr_data_pair_preview_ox08b402q) / sizeof(kal_uint16));

	//init_sz = sizeof(addr_data_pair_preview_ox08b402q) / sizeof(kal_uint16);
	//LOG_INF("3, init_sz=%d\n",init_sz);
	//for(i = 0; i < init_sz;  i += 2)
	//	write_cmos_sensor(ctx, addr_data_pair_preview_ox08b402q[i],  addr_data_pair_preview_ox08b402q[i+1]);
	DRV_LOG(ctx, "--MAX96712-- sensor init end\n");
	return 0;
}

static int set_streaming_control(void *arg, bool enable)
{
	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;

	DRV_LOG(ctx, "streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable) {
		/*turn on mipi*/
		write_cmos_sensor(ctx, 0x0100, 0x01); //disable sleep mode of ox08b40
		write_cmos_max96712_sensor(ctx, 0x08A0, 0x84);
		write_cmos_max96712_sensor(ctx, 0x040B, 0x52);
	} else {
		/*turn off mipi*/
		write_cmos_sensor(ctx, 0x0100, 0x00); //enable sleep mode of ox08b40
		write_cmos_max96712_sensor(ctx, 0x08A0, 0x04);
		write_cmos_max96712_sensor(ctx, 0x040B, 0x00);
	}
	msleep(20);
	return ERROR_NONE;
}
/*
 *static int return_sensor_id(struct subdrv_ctx *ctx)
 *{
 *	return read_cmos_max96712_sensor(ctx, MAX96712_ID_REG);
 *}
 */
static int Check_Sensor_Signal(struct subdrv_ctx *ctx)
{
	int linka, linkb, linkc, linkd;

    //link lock check , linka:0xda
	linka = read_cmos_max96712_sensor(ctx, 0x1A);
	linkb = read_cmos_max96712_sensor(ctx, 0x0A);
	linkc = read_cmos_max96712_sensor(ctx, 0x0B);
	linkd = read_cmos_max96712_sensor(ctx, 0x0C);
	DRV_LOG(ctx, "Sensor link lock Status:linka: 0x%x linkb: 0x%x linkc: 0x%x linkd: 0x%x\n",
		linka, linkb, linkc, linkd);
	if (((linka&0x08) == 0x8) || ((linkb&0x08) == 0x8) || ((linkc&0x08) == 0x8) || ((linkd&0x08) == 0x8))
		return 0;
	else
		return 1;

}

static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = { //maybe used for hdr mode
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = FRAME_WIDTH,
			.vsize = FRAME_HEIGHT,
			.fs_seq = MTK_FRAME_DESC_FS_SEQ_FIRST,
		},
	},
};

static struct subdrv_mode_struct mode_struct[] = {
	{

		.frame_desc = frame_desc_prev,
		.num_entries = ARRAY_SIZE(frame_desc_prev),
		.mode_setting_table = addr_data_pair_preview_ox08b402q,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview_ox08b402q),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 323896320,
		.linelength = 4752,
		.framelength = 2272,
		.max_framerate = 300,
		.mipi_pixel_rate = 432000000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = FRAME_WIDTH,
			.full_h = FRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = FRAME_WIDTH,
			.h0_size = FRAME_HEIGHT,
			.scale_w = FRAME_WIDTH,
			.scale_h = FRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = FRAME_WIDTH,
			.h1_size = FRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = FRAME_WIDTH,
			.h2_tg_size = FRAME_HEIGHT,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,// TRUE,
		.imgsensor_pd_info = PARAM_UNDEFINED,// &imgsensor_pd_info,
		.ae_binning_ratio = 1,
		.fine_integ_line = 0,
		.delay_frame = 3,// luna need to be confirm
		.csi_param = {
			// .legacy_phy = 0,
			// .not_fixed_trail_settle = 1,
			// .not_fixed_dphy_settle = 1,
			// .dphy_data_settle = 0x13,
			// .dphy_clk_settle = 0x13,
			// .dphy_trail = 0x35,
			// .dphy_csi2_resync_dmy_cycle = 0xF,
		},
	},
	{
		.frame_desc = frame_desc_cap,
		.num_entries = ARRAY_SIZE(frame_desc_cap),
		.mode_setting_table = addr_data_pair_preview_ox08b402q,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview_ox08b402q),
		.seamless_switch_group = PARAM_UNDEFINED,// 1,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,// addr_data_pair_capture_ox08b402q,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,// ARRAY_SIZE(addr_data_pair_capture_ox08b402q),
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 323896320,
		.linelength = 4752,
		.framelength = 2272,
		.max_framerate = 300,
		.mipi_pixel_rate = 432000000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = FRAME_WIDTH,
			.full_h = FRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = FRAME_WIDTH,
			.h0_size = FRAME_HEIGHT,
			.scale_w = FRAME_WIDTH,
			.scale_h = FRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = FRAME_WIDTH,
			.h1_size = FRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = FRAME_WIDTH,
			.h2_tg_size = FRAME_HEIGHT,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,// TRUE,
		.imgsensor_pd_info = PARAM_UNDEFINED,// &imgsensor_pd_info,
		.ae_binning_ratio = 1,
		.fine_integ_line = 0,
		.delay_frame = 3,// luna need to be confirm
		.csi_param = {
			// .legacy_phy = 0,
			// .not_fixed_trail_settle = 1,
			// .not_fixed_dphy_settle = 1,
			// .dphy_data_settle = 0x13,
			// .dphy_clk_settle = 0x13,
			// .dphy_trail = 0x35,
			// .dphy_csi2_resync_dmy_cycle = 0xF,
		},
	},
	{
		.frame_desc = frame_desc_vid,
		.num_entries = ARRAY_SIZE(frame_desc_vid),
		.mode_setting_table = addr_data_pair_preview_ox08b402q,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview_ox08b402q),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 323896320,
		.linelength = 4752,
		.framelength = 2272,
		.max_framerate = 300,
		.mipi_pixel_rate = 432000000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = FRAME_WIDTH,
			.full_h = FRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = FRAME_WIDTH,
			.h0_size = FRAME_HEIGHT,
			.scale_w = FRAME_WIDTH,
			.scale_h = FRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = FRAME_WIDTH,
			.h1_size = FRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = FRAME_WIDTH,
			.h2_tg_size = FRAME_HEIGHT,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,// TRUE,
		.imgsensor_pd_info = PARAM_UNDEFINED,// &imgsensor_pd_info,
		.ae_binning_ratio = 1,
		.fine_integ_line = 0,
		.delay_frame = 3,// luna need to be confirm
		.csi_param = {
			// .legacy_phy = 0,
			// .not_fixed_trail_settle = 1,
			// .not_fixed_dphy_settle = 1,
			// .dphy_data_settle = 0x13,
			// .dphy_clk_settle = 0x13,
			// .dphy_trail = 0x35,
			// .dphy_csi2_resync_dmy_cycle = 0xF,
		},
	},
	{
		.frame_desc = frame_desc_hs_vid,
		.num_entries = ARRAY_SIZE(frame_desc_hs_vid),
		.mode_setting_table = addr_data_pair_preview_ox08b402q,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview_ox08b402q),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 323896320,
		.linelength = 4752,
		.framelength = 2272,
		.max_framerate = 300,
		.mipi_pixel_rate = 432000000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = FRAME_WIDTH,
			.full_h = FRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = FRAME_WIDTH,
			.h0_size = FRAME_HEIGHT,
			.scale_w = FRAME_WIDTH,
			.scale_h = FRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = FRAME_WIDTH,
			.h1_size = FRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = FRAME_WIDTH,
			.h2_tg_size = FRAME_HEIGHT,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,// TRUE,
		.imgsensor_pd_info = PARAM_UNDEFINED,// &imgsensor_pd_info,
		.ae_binning_ratio = 1,
		.fine_integ_line = 0,
		.delay_frame = 3,// luna need to be confirm
		.csi_param = {
			// .legacy_phy = 0,
			// .not_fixed_trail_settle = 1,
			// .not_fixed_dphy_settle = 1,
			// .dphy_data_settle = 0x13,
			// .dphy_clk_settle = 0x13,
			// .dphy_trail = 0x35,
			// .dphy_csi2_resync_dmy_cycle = 0xF,
		},
	},
	{
		.frame_desc = frame_desc_slim_vid,
		.num_entries = ARRAY_SIZE(frame_desc_slim_vid),
		.mode_setting_table = addr_data_pair_preview_ox08b402q,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview_ox08b402q),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 323896320,
		.linelength = 4752,
		.framelength = 2272,
		.max_framerate = 300,
		.mipi_pixel_rate = 432000000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = FRAME_WIDTH,
			.full_h = FRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = FRAME_WIDTH,
			.h0_size = FRAME_HEIGHT,
			.scale_w = FRAME_WIDTH,
			.scale_h = FRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = FRAME_WIDTH,
			.h1_size = FRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = FRAME_WIDTH,
			.h2_tg_size = FRAME_HEIGHT,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,// TRUE,
		.imgsensor_pd_info = PARAM_UNDEFINED,// &imgsensor_pd_info,
		.ae_binning_ratio = 1,
		.fine_integ_line = 0,
		.delay_frame = 3,// luna need to be confirm
		.csi_param = {
			// .legacy_phy = 0,
			// .not_fixed_trail_settle = 1,
			// .not_fixed_dphy_settle = 1,
			// .dphy_data_settle = 0x13,
			// .dphy_clk_settle = 0x13,
			// .dphy_trail = 0x35,
			// .dphy_csi2_resync_dmy_cycle = 0xF,
		},
	},
	{
		.frame_desc = frame_desc_cus1,
		.num_entries = ARRAY_SIZE(frame_desc_cus1),
		.mode_setting_table = addr_data_pair_preview_ox08b402q,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview_ox08b402q),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 323896320,
		.linelength = 4752,
		.framelength = 2272,
		.max_framerate = 300,
		.mipi_pixel_rate = 432000000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = FRAME_WIDTH,
			.full_h = FRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = FRAME_WIDTH,
			.h0_size = FRAME_HEIGHT,
			.scale_w = FRAME_WIDTH,
			.scale_h = FRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = FRAME_WIDTH,
			.h1_size = FRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = FRAME_WIDTH,
			.h2_tg_size = FRAME_HEIGHT,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,// TRUE,
		.imgsensor_pd_info = PARAM_UNDEFINED,// &imgsensor_pd_info,
		.ae_binning_ratio = 1,
		.fine_integ_line = 0,
		.delay_frame = 3,// luna need to be confirm
		.csi_param = {
			// .legacy_phy = 0,
			// .not_fixed_trail_settle = 1,
			// .not_fixed_dphy_settle = 1,
			// .dphy_data_settle = 0x13,
			// .dphy_clk_settle = 0x13,
			// .dphy_trail = 0x35,
			// .dphy_csi2_resync_dmy_cycle = 0xF,
		},
	},
	{
		.frame_desc = frame_desc_cus2,
		.num_entries = ARRAY_SIZE(frame_desc_cus2),
		.mode_setting_table = addr_data_pair_preview_ox08b402q,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview_ox08b402q),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 323896320,
		.linelength = 4752,
		.framelength = 2272,
		.max_framerate = 300,
		.mipi_pixel_rate = 432000000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = FRAME_WIDTH,
			.full_h = FRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = FRAME_WIDTH,
			.h0_size = FRAME_HEIGHT,
			.scale_w = FRAME_WIDTH,
			.scale_h = FRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = FRAME_WIDTH,
			.h1_size = FRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = FRAME_WIDTH,
			.h2_tg_size = FRAME_HEIGHT,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,// TRUE,
		.imgsensor_pd_info = PARAM_UNDEFINED,// &imgsensor_pd_info,
		.ae_binning_ratio = 1,
		.fine_integ_line = 0,
		.delay_frame = 3,// luna need to be confirm
		.csi_param = {
			// .legacy_phy = 0,
			// .not_fixed_trail_settle = 1,
			// .not_fixed_dphy_settle = 1,
			// .dphy_data_settle = 0x13,
			// .dphy_clk_settle = 0x13,
			// .dphy_trail = 0x35,
			// .dphy_csi2_resync_dmy_cycle = 0xF,
		},
	},
	{
		.frame_desc = frame_desc_cus3,
		.num_entries = ARRAY_SIZE(frame_desc_cus3),
		.mode_setting_table = addr_data_pair_preview_ox08b402q,
		.mode_setting_len = ARRAY_SIZE(addr_data_pair_preview_ox08b402q),
		.seamless_switch_group = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_table = PARAM_UNDEFINED,
		.seamless_switch_mode_setting_len = PARAM_UNDEFINED,
		.hdr_mode = HDR_NONE,
		.raw_cnt = 1,
		.exp_cnt = 1,
		.pclk = 323896320,
		.linelength = 4752,
		.framelength = 2272,
		.max_framerate = 300,
		.mipi_pixel_rate = 432000000,
		.readout_length = 0,
		.read_margin = 0,
		.imgsensor_winsize_info = {
			.full_w = FRAME_WIDTH,
			.full_h = FRAME_HEIGHT,
			.x0_offset = 0,
			.y0_offset = 0,
			.w0_size = FRAME_WIDTH,
			.h0_size = FRAME_HEIGHT,
			.scale_w = FRAME_WIDTH,
			.scale_h = FRAME_HEIGHT,
			.x1_offset = 0,
			.y1_offset = 0,
			.w1_size = FRAME_WIDTH,
			.h1_size = FRAME_HEIGHT,
			.x2_tg_offset = 0,
			.y2_tg_offset = 0,
			.w2_tg_size = FRAME_WIDTH,
			.h2_tg_size = FRAME_HEIGHT,
		},
		.aov_mode = 0,
		.s_dummy_support = 1,
		.ae_ctrl_support = 1,
		.pdaf_cap = FALSE,// TRUE,
		.imgsensor_pd_info = PARAM_UNDEFINED,// &imgsensor_pd_info,
		.ae_binning_ratio = 1,
		.fine_integ_line = 0,
		.delay_frame = 3,// luna need to be confirm
		.csi_param = {
			// .legacy_phy = 0,
			// .not_fixed_trail_settle = 1,
			// .not_fixed_dphy_settle = 1,
			// .dphy_data_settle = 0x13,
			// .dphy_clk_settle = 0x13,
			// .dphy_trail = 0x35,
			// .dphy_csi2_resync_dmy_cycle = 0xF,
		},
	},
};
static struct subdrv_static_ctx static_ctx = {
	.sensor_id = OX08B40_SENSOR_ID,
	.reg_addr_sensor_id = {0x300A, 0x300B, 0x300C},
	.i2c_addr_table = {0x6C, 0xFF},
	.i2c_burst_write_support = TRUE,
	.i2c_transfer_data_type = I2C_DT_ADDR_16_DATA_8,
	//.eeprom_info = eeprom_info,
	//.eeprom_num = ARRAY_SIZE(eeprom_info),
	.resolution = {FRAME_WIDTH, FRAME_HEIGHT},
	.mirror = IMAGE_NORMAL,

	.mclk = 24,
	.isp_driving_current = ISP_DRIVING_8MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.ob_pedestal = 0x40,

	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_min = BASEGAIN * 1,
	.ana_gain_max = BASEGAIN * 15.5, //luna
	.ana_gain_type = 1, //luna
	.ana_gain_step = 4, //luna
	.ana_gain_table = ox08b40_ana_gain_table,
	.ana_gain_table_size = sizeof(ox08b40_ana_gain_table),
	.tuning_iso_base = 100,
	.exposure_def = 0x3D0,
	.exposure_min = 8,
	.exposure_max = 0xFFFFFF - 22,
	.exposure_step = 2,
	.exposure_margin = 22,

	.frame_length_max = 0xFFFFFF,
	.ae_effective_frame = 2,
	.frame_time_delay_frame = 2,
	.start_exposure_offset = 500000,

	.pdaf_type = PARAM_UNDEFINED,// PDAF_SUPPORT_CAMSV,
	.hdr_type = HDR_SUPPORT_NA,
	.seamless_switch_support = FALSE,
	.temperature_support = FALSE, //
	//.g_temp = get_sensor_temperature,
	.g_gain2reg = get_gain2reg,
	//.s_gph = set_group_hold,
	//.s_cali = set_sensor_cali,
	.s_streaming_control = set_streaming_control,

	.reg_addr_stream = 0x0100,
	.reg_addr_mirror_flip = PARAM_UNDEFINED,
	.reg_addr_exposure = {{0x3501, 0x3502},},
	.long_exposure_support = FALSE,
	.reg_addr_exposure_lshift = PARAM_UNDEFINED,
	.reg_addr_ana_gain = {{0x3508, 0x3509},},
	.reg_addr_frame_length = {0x380E, 0x380F},
	.reg_addr_temp_en = 0x4D07,
	.reg_addr_temp_read = PARAM_UNDEFINED,// {0x4D2A, 0x4D2B},
	.reg_addr_auto_extend = PARAM_UNDEFINED,
	.reg_addr_frame_count = PARAM_UNDEFINED,// {0x483E, 0x483F},

	.init_setting_table = PARAM_UNDEFINED,// addr_data_pair_init_ox08b402q,
	.init_setting_len = 0,// ARRAY_SIZE(addr_data_pair_init_ox08b402q),
	.mode = mode_struct,
	.sensor_mode_num = ARRAY_SIZE(mode_struct),
	.list = feature_control_list,
	.list_len = ARRAY_SIZE(feature_control_list),
	.chk_s_off_sta = 1,
	.chk_s_off_end = 0,

	.checksum_value = 0x388C7147,
	.aov_sensor_support = TRUE,
	.init_in_open = TRUE,
	.streaming_ctrl_imp = TRUE,
};

static kal_uint32 return_ox08b40_sensor_id(struct subdrv_ctx *ctx)
{
	u8 i = 0;
	u8 retry;
	u32 sensor_id = 0;

	ctx->i2c_write_id = static_ctx.i2c_addr_table[0];
	retry = 2;

	while (static_ctx.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		ctx->i2c_write_id = static_ctx.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = ((read_cmos_sensor(ctx, 0x300a) << 16) |
				(read_cmos_sensor(ctx, 0x300b) << 8) | read_cmos_sensor(ctx, 0x300c));
			LOG_INF("read ox08b40 sensor_id:0x%x, exp:0x%x",sensor_id, OX08B40_SENSOR_ID);
			if (sensor_id == (static_ctx.sensor_id)) {
				LOG_DBG("i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, sensor_id);
				return sensor_id;
			}
			LOG_ERR("Read id fail, id: 0x%x, i2c id: 0x%x\n",
				sensor_id, ctx->i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	return 0;
}

/*************************************************************************
 * FUNCTION
 *    get_imgsensor_id
 *
 * DESCRIPTION
 *    This function get the sensor ID
 *
 * PARAMETERS
 *    *sensorID : return the sensor ID
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int get_imgsensor_id(struct subdrv_ctx *ctx, UINT32 *sensor_id)
{
	int linka = 0;
	int retry = 2;

	max96712_sensor_init(ctx);

	do {
		if (Check_Sensor_Signal(ctx)) {
			LOG_INF("Sensor Not Connect");
			mdelay(5);
			retry--;
		} else
			break;
	} while (retry > 0);

	if (retry == 0) {
		LOG_INF("Sensor Not Connect!");
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	//try to read ox08b40 sensor id, i2c id:0x60
	retry = 2;
	do {
		*sensor_id = return_ox08b40_sensor_id(ctx);
		if (*sensor_id == static_ctx.sensor_id) {
			LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
				OX08B40_I2C_ADDR, *sensor_id);
			return ERROR_NONE;
		}
		LOG_INF("ox08b40 Read id fail, id: 0x%x, i2c id: 0x%x\n",
			*sensor_id, OX08B40_I2C_ADDR);
		retry--;
	} while (retry > 0);

	LOG_INF("failed,Sensor Status:linkb0x5009: 0x%x retry=%d\n", linka, retry);
	return ERROR_SENSOR_CONNECT_FAIL;
}

/*************************************************************************
 * FUNCTION
 *    open
 *
 * DESCRIPTION
 *    This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *    None
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int open(struct subdrv_ctx *ctx)
{
	u8 retry = 2;
	int linka = 0;
	u32 sensor_id = 0;
	u32 scenario_id = 0;

	/* initail sequence write in  */
	max96712_sensor_init(ctx);

	do {
		if (Check_Sensor_Signal(ctx)) {
			LOG_INF("Sensor Not Connect");
			mdelay(5);
			retry--;
		} else
			break;
	} while (retry > 0);

	if (retry == 0) {
		LOG_INF("Sensor Not Connect!");
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	//try to read ox08b40 sensor id, i2c id:0x60
	retry = 2;
	do {
		sensor_id = return_ox08b40_sensor_id(ctx);
		if (sensor_id == static_ctx.sensor_id) {
			LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
				OX08B40_I2C_ADDR, sensor_id);
			break;
		}
		LOG_INF("ox08b40 Read id fail, id: 0x%x, i2c id: 0x%x\n",
			sensor_id, OX08B40_I2C_ADDR);
		retry--;
	} while (retry > 0);

	if (retry == 0) {
		LOG_INF("failed,Sensor Status:linkb0x5009: 0x%x retry=%d\n", linka, retry);
		return ERROR_SENSOR_CONNECT_FAIL;
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
} /* open */

static struct subdrv_ops ops = {
	.get_id = get_imgsensor_id,//
	.init_ctx = init_ctx,
	.open = open,//
	.get_info = common_get_info,
	.get_resolution = common_get_resolution,
	.control = common_control,
	.feature_control = common_feature_control,
	.close = common_close,
	.get_frame_desc = common_get_frame_desc,
	.get_temp = common_get_temp,
	.get_csi_param = common_get_csi_param,
	.update_sof_cnt = common_update_sof_cnt,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_PDN, 0, 20000},
	{HW_ID_PDN, 1800000, 20000},
};

const struct subdrv_entry ox08b40_mipi_raw_entry = {
	.name = "ox08b40_mipi_raw",
	.id = OX08B40_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

/* STRUCT */

/*static void set_sensor_cali(void *arg)
 *{
 *	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
 *
 *	u16 idx = 0;
 *	u8 support = FALSE;
 *	u8 *pbuf = NULL;
 *	u16 size = 0;
 *	u16 addr = 0;
 *	struct eeprom_info_struct *info = ctx->s_ctx.eeprom_info;
 *
 *	if (!probe_eeprom(ctx))
 *		return;
 *
 *	idx = ctx->eeprom_index;
 *
 *	// PDC data
 *	support = info[idx].pdc_support;
 *	if (support) {
 *		pbuf = info[idx].preload_pdc_table;
 *		if (pbuf != NULL) {
 *			size = 8;
 *			addr = 0x5C0E;
 *			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
 *			pbuf += size;
 *			size = 720;
 *			addr = 0x5900;
 *			subdrv_i2c_wr_seq_p8(ctx, addr, pbuf, size);
 *			DRV_LOG(ctx, "set PDC calibration data done.");
 *		}
 *	}
 *}
 */

/*
 *static int get_sensor_temperature(void *arg)
 *{
 *	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
 *	int temperature = 0;
 *
 *	// TEMP_SEN_CTL
 *	subdrv_i2c_wr_u8(ctx, ctx->s_ctx.reg_addr_temp_en, 0x01);
 *	temperature = subdrv_i2c_rd_u8(ctx, ctx->s_ctx.reg_addr_temp_read);
 *	temperature = (temperature > 0xC0) ? (temperature - 0x100) : temperature;
 *
 *	DRV_LOG(ctx, "temperature: %d degrees\n", temperature);
 *	return temperature;
 *}
 */

/*
 *static void set_group_hold(void *arg, u8 en)
 *{
 *	struct subdrv_ctx *ctx = (struct subdrv_ctx *)arg;
 *
 *	if (en) {
 *		set_i2c_buffer(ctx, 0x3208, 0x00);
 *	} else {
 *		set_i2c_buffer(ctx, 0x3208, 0x10);
 *		set_i2c_buffer(ctx, 0x3208, 0xA0);
 *	}
 *}
 */

static void ox08b40_set_dummy(struct subdrv_ctx *ctx)
{
	// bool gph = !ctx->is_seamless && (ctx->s_ctx.s_gph != NULL);

	// if (gph)
	// ctx->s_ctx.s_gph((void *)ctx, 1);
	write_frame_length(ctx, ctx->frame_length);
	// if (gph)
	// ctx->s_ctx.s_gph((void *)ctx, 0);

	commit_i2c_buffer(ctx);
}

static int ox08b40_set_max_framerate_by_scenario(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u64 *feature_data = (u64 *)para;
	enum SENSOR_SCENARIO_ID_ENUM scenario_id = (enum SENSOR_SCENARIO_ID_ENUM)*feature_data;
	u32 framerate = *(feature_data + 1);
	u32 frame_length;

	if (scenario_id >= ctx->s_ctx.sensor_mode_num) {
		DRV_LOG(ctx, "invalid sid:%u, mode_num:%u\n",
			scenario_id, ctx->s_ctx.sensor_mode_num);
		scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
	}

	if (framerate == 0) {
		DRV_LOG(ctx, "framerate should not be 0\n");
		return ERROR_NONE;
	}

	if (ctx->s_ctx.mode[scenario_id].linelength == 0) {
		DRV_LOG(ctx, "linelength should not be 0\n");
		return ERROR_NONE;
	}

	if (ctx->line_length == 0) {
		DRV_LOG(ctx, "ctx->line_length should not be 0\n");
		return ERROR_NONE;
	}

	if (ctx->frame_length == 0) {
		DRV_LOG(ctx, "ctx->frame_length should not be 0\n");
		return ERROR_NONE;
	}

	frame_length = ctx->s_ctx.mode[scenario_id].pclk / framerate * 10
		/ ctx->s_ctx.mode[scenario_id].linelength;
	ctx->frame_length =
		max(frame_length, ctx->s_ctx.mode[scenario_id].framelength);
	ctx->frame_length = min(ctx->frame_length, ctx->s_ctx.frame_length_max);
	ctx->current_fps = ctx->pclk / ctx->frame_length * 10 / ctx->line_length;
	ctx->min_frame_length = ctx->frame_length;
	DRV_LOG(ctx, "max_fps(input/output):%u/%u(sid:%u), min_fl_en:1\n",
		framerate, ctx->current_fps, scenario_id);
	if (ctx->frame_length > (ctx->exposure[0] + ctx->s_ctx.exposure_margin))
		ox08b40_set_dummy(ctx);
	return ERROR_NONE;
}

static u16 get_gain2reg(u32 gain)
{
	return gain * 256 / BASEGAIN;
}
/*
 *static int ox08b40_seamless_switch(struct subdrv_ctx *ctx, u8 *para, u32 *len)
 *{
 *	enum SENSOR_SCENARIO_ID_ENUM scenario_id;
 *	u32 *ae_ctrl = NULL;
 *	u64 *feature_data = (u64 *)para;
 *
 *	if (feature_data == NULL) {
 *		DRV_LOGE(ctx, "input scenario is null!");
 *		return ERROR_NONE;
 *	}
 *	scenario_id = *feature_data;
 *	if ((feature_data + 1) != NULL)
 *		ae_ctrl = (u32 *)((uintptr_t)(*(feature_data + 1)));
 *	else
 *		DRV_LOGE(ctx, "no ae_ctrl input");
 *
 *	check_current_scenario_id_bound(ctx);
 *	DRV_LOG(ctx, "E: set seamless switch %u %u\n", ctx->current_scenario_id, scenario_id);
 *	if (!ctx->extend_frame_length_en)
 *		DRV_LOGE(ctx, "please extend_frame_length before seamless_switch!\n");
 *	ctx->extend_frame_length_en = FALSE;
 *
 *	if (scenario_id >= ctx->s_ctx.sensor_mode_num) {
 *		DRV_LOGE(ctx, "invalid sid:%u, mode_num:%u\n",
 *			scenario_id, ctx->s_ctx.sensor_mode_num);
 *		return ERROR_NONE;
 *	}
 *	if (ctx->s_ctx.mode[scenario_id].seamless_switch_group == 0 ||
 *		ctx->s_ctx.mode[scenario_id].seamless_switch_group !=
 *			ctx->s_ctx.mode[ctx->current_scenario_id].seamless_switch_group) {
 *		DRV_LOGE(ctx, "seamless_switch not supported\n");
 *		return ERROR_NONE;
 *	}
 *	if (ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table == NULL) {
 *		DRV_LOGE(ctx, "Please implement seamless_switch setting\n");
 *		return ERROR_NONE;
 *	}
 *
 *	ctx->is_seamless = TRUE;
 *	update_mode_info(ctx, scenario_id);
 *
 *	i2c_table_write(ctx, addr_data_pair_seamless_switch_step1_ox08b402q,
 *		ARRAY_SIZE(addr_data_pair_seamless_switch_step1_ox08b402q));
 *	i2c_table_write(ctx,
 *		ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_table,
 *		ctx->s_ctx.mode[scenario_id].seamless_switch_mode_setting_len);
 *	if (ae_ctrl) {
 *		set_shutter(ctx, ae_ctrl[0]);
 *		set_gain(ctx, ae_ctrl[5]);
 *	}
 *	i2c_table_write(ctx, addr_data_pair_seamless_switch_step2_ox08b402q,
 *		ARRAY_SIZE(addr_data_pair_seamless_switch_step2_ox08b402q));
 *	if (ae_ctrl) {
 *		set_shutter(ctx, ae_ctrl[10]);
 *		set_gain(ctx, ae_ctrl[15]);
 *	}
 *	i2c_table_write(ctx, addr_data_pair_seamless_switch_step3_ox08b402q,
 *		ARRAY_SIZE(addr_data_pair_seamless_switch_step3_ox08b402q));
 *
 *	ctx->is_seamless = FALSE;
 *	DRV_LOG(ctx, "X: set seamless switch done\n");
 *	return ERROR_NONE;
 *}
 */

static int ox08b40_set_test_pattern(struct subdrv_ctx *ctx, u8 *para, u32 *len)
{
	u32 mode = *((u32 *)para);

	if (mode != ctx->test_pattern)
		DRV_LOG(ctx, "mode(%u->%u)\n", ctx->test_pattern, mode);
	/* 1:Solid Color 2:Color Bar 5:Black */
	switch (mode) {
	case 2:
//		subdrv_i2c_wr_u8(ctx, 0x5000, 0x81);
//		subdrv_i2c_wr_u8(ctx, 0x5001, 0x00);
//		subdrv_i2c_wr_u8(ctx, 0x5002, 0x92);
//		subdrv_i2c_wr_u8(ctx, 0x5081, 0x01);
		break;
	case 5:
//		subdrv_i2c_wr_u8(ctx, 0x3019, 0xF0);
//		subdrv_i2c_wr_u8(ctx, 0x4308, 0x01);
		break;
	default:
		break;
	}

	if (mode != ctx->test_pattern)
		switch (ctx->test_pattern) {
		case 2:
//			subdrv_i2c_wr_u8(ctx, 0x5000, 0xCB);
//			subdrv_i2c_wr_u8(ctx, 0x5001, 0x43);
//			subdrv_i2c_wr_u8(ctx, 0x5002, 0x9E);
//			subdrv_i2c_wr_u8(ctx, 0x5081, 0x00);
			break;
		case 5:
//			subdrv_i2c_wr_u8(ctx, 0x3019, 0xD2);
//			subdrv_i2c_wr_u8(ctx, 0x4308, 0x00);
			break;
		default:
			break;
		}

	ctx->test_pattern = mode;
	return ERROR_NONE;
}

static int init_ctx(struct subdrv_ctx *ctx,	struct i2c_client *i2c_client, u8 i2c_write_id)
{
	memcpy(&(ctx->s_ctx), &static_ctx, sizeof(struct subdrv_static_ctx));
	subdrv_ctx_init(ctx);
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}
