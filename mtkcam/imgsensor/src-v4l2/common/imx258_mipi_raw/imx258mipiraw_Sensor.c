// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 IMX258mipiraw_Sensor.c
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

#include "imx258mipiraw_Sensor.h"
#include "imx258_pdafotp.h"
#include "imx258_ana_gain_table.h"

#include "adaptor-subdrv.h"
#include "adaptor-i2c.h"

#define read_cmos_sensor(...) subdrv_i2c_rd_u8(__VA_ARGS__)
#define write_cmos_sensor(...) subdrv_i2c_wr_u8(__VA_ARGS__)

/**********************Modify Following Strings for Debug**********************/
#define PFX "IMX258_camera_sensor"
#define LOG_1 LOG_INF("IMX258,MIPI 4LANE\n")
/***********************   Modify end    **************************************/
#define LOG_INF(format, args...) \
	pr_debug(PFX "[%s] " format, __func__, ##args)




static struct imgsensor_info_struct imgsensor_info = {

	/* IMX258MIPI_SENSOR_ID, sensor_id = 0x2680*/
	.sensor_id = IMX258_SENSOR_ID,

	.checksum_value = 0x38ebe79e, /* checksum value for Camera Auto Test */

	.pre = {
		.pclk = 259200000,	/* record different mode's pclk */
		.linelength = 5352,	/* record different mode's linelength */
		.framelength = 1614,  /* record different mode's framelength */
		.startx = 0, /* record different mode's startx of grabwindow */
		.starty = 0, /* record different mode's starty of grabwindow */

		/* record different mode's width of grabwindow */
		.grabwindow_width = 2100,

		/* record different mode's height of grabwindow */
		.grabwindow_height = 1560,

		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
		 * by different scenario
		 */
		.mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		/*       following for GetDefaultFramerateByScenario()  */
		.max_framerate = 300,
		},
	.cap = {		/*normal capture */
		.pclk = 518400000,
		.linelength = 5352,
		.framelength = 3228,	/* 1332, */
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4208,
		.grabwindow_height = 3120,
		.mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		.max_framerate = 300,	/* 300, */
		},
	.cap1 = {		/*PIP capture */
		 .pclk = 259200000,
		 .linelength = 5352,
		 .framelength = 3228,
		 .startx = 0,
		 .starty = 0,
		 .grabwindow_width = 4208,
		 .grabwindow_height = 3120,
		 .mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		 .max_framerate = 150,
		 },
	.normal_video = {
			 .pclk = 518400000,
			 .linelength = 5352,
			 .framelength = 3228,
			 .startx = 0,
			 .starty = 0,
			 .grabwindow_width = 4208,
			 .grabwindow_height = 3120,
			 .mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
			 .max_framerate = 300,	/* modify */
			 },
	.hs_video = {		/*slow motion */
		     .pclk = 480000000,	/* 518400000, */
		     .linelength = 5352,
		     .framelength = 746,	/* 806, */
		     .startx = 0,
		     .starty = 0,
		     .grabwindow_width = 1048,	/* 1400, */
		     .grabwindow_height = 480,	/* 752, */
		     .mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		     .max_framerate = 1200,	/* modify */

		     },
	.slim_video = {		/*VT Call */
		       .pclk = 259200000,	/* 158400000, */
		       .linelength = 5352,
		       .framelength = 1614,	/* 986,      //1236 */
		       .startx = 0,
		       .starty = 0,
		       .grabwindow_width = 2100,	/* 1400, */
		       .grabwindow_height = 1560,	/* 752, */
		       .mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		       .max_framerate = 300,
		       },
	.custom1 = {		/*PIP capture */
		    .pclk = 405600000,
		    .linelength = 5352,
		    .framelength = 3156,
		    .startx = 0,
		    .starty = 0,
		    .grabwindow_width = 4208,
		    .grabwindow_height = 3120,
		    .mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		    .max_framerate = 240,
		    },
	.custom2 = {		/*PIP capture  24fps*/
		    .pclk = 259200000,
		    .linelength = 5352,
		    .framelength = 2018,
		    .startx = 0,
		    .starty = 0,
		    .grabwindow_width = 2100,
		    .grabwindow_height = 1560,
		    .mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		    .max_framerate = 240,
		    },
	.margin = 4,		/* sensor framelength & shutter margin */
	.min_shutter = 1,	/* 1,          //min shutter */

	/* max framelength by sensor register's limitation */
	.max_frame_length = 0x7fff,
	.ae_shut_delay_frame = 0,

	/* shutter delay frame for AE cycle,
	 * 2 frame with ispGain_delay-shut_delay=2-0=2
	 */
	.ae_sensor_gain_delay_frame = 0,

	/* sensor gain delay frame for AE cycle,
	 * 2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	 */
	.ae_ispGain_delay_frame = 2,	/* isp gain delay frame for AE cycle */

	/* The delay frame of setting frame length	*/
	.frame_time_delay_frame = 3,

	.ihdr_support = 1,	/* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	/* 1,le first ; 0, se first */
	.sensor_mode_num = 7,	/* support sensor mode num */

	.cap_delay_frame = 2,	/* enter capture delay frame num */
	.pre_delay_frame = 2,	/* enter preview delay frame num */
	.video_delay_frame = 2,	/* enter video delay frame num */
	.hs_video_delay_frame = 2, /* enter high speed video  delay frame num */
	.slim_video_delay_frame = 2,	/* enter slim video delay frame num */
	.custom1_delay_frame = 2,	/* add new mode */
	.custom2_delay_frame = 2,
	.isp_driving_current = ISP_DRIVING_4MA,	/* mclk driving current */

	/* sensor_interface_type */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,

	/* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_sensor_type = MIPI_OPHY_NCSI2,

	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_MANUAL,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	/* sensor output first pixel color */
	.mclk = 24,	/* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_4_LANE,	/* mipi lane num */

/* record sensor support all write id addr, only supprt 4must end with 0xff */
	.i2c_addr_table = {0x34, 0x20, 0xff},

};



/* #define RAW_TYPE_OVERRIDE     //it's PDO function define */


#define IMX258_RAW_TYPE	(0x80)	/*enable test mode */

#define IMX258_HDR_TYPE (0x00)
#define IMX258_BINNING_TYPE (0x10)
#define IMX258_BW_TYPE		(0x20)
#define IMX258_NOPDAF_TYPE	(0x30)
#define IMX258_HDD_TYPE (0x40)	/* IMX258-0AUH5 */
static kal_uint16 imx258_type;
static kal_uint16 test_Pmode;
/*
 *-IMX258 0AQH5-C (BME-HDR version ,PDAF?¢Gsupport binning mode)
 *-IMX258 0APH5-C (Binning version ,HDR?¢Gsupport PDAF)
 *-IMX258 0AMH5-C (B/W version)
 *-IMX258 0ATH5-C (Non-PDAF version ,|3HDR)
 *-IMX258 0AUH5-C (Horizontal Double Density version)
 */
/* Sensor output window information */
/*according toIMX258 datasheet p53 image cropping*/
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] = {
	{4208, 3120, 4, 0, 4200, 3120, 2100, 1560,
	 0000, 0000, 2100, 1560, 0, 0, 2100, 1560},	/* Preview */
	{4208, 3120, 0, 0, 4208, 3120, 4208, 3120,
	 0000, 0000, 4208, 3120, 0, 0, 4208, 3120},	/*capture */
	{4208, 3120, 0, 0, 4208, 3120, 4208, 3120,
	 0000, 0000, 4208, 3120, 0, 0, 4208, 3120},	/*video */
	{4208, 3120, 8, 592, 4192, 1920, 1052, 480,
	 0000, 0000, 1048, 480, 0, 0, 1048, 480},	/*hight speed video */
	{4208, 3120, 4, 0, 4200, 3120, 2100, 1560,
	 0000, 0000, 2100, 1560, 0, 0, 2100, 1560},	/*slim video */
	{4208, 3120, 0, 0, 4208, 3120, 4208, 3120,
	 0000, 0000, 4208, 3120, 0, 0, 4208, 3120},	/*custom1 */
	{4208, 3120, 4, 0, 4200, 3120, 2100, 1560,
	 0000, 0000, 2100, 1560, 0, 0, 2100, 1560}	/*custom2 */
};

/*VC1 None , VC2 for PDAF(DT=0X36), unit : 8bit*/
static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3] = {
	/* Preview mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x0834, 0x0618, 0x00, 0x35, 0x0280, 0x0001,
	 0x00, 0x2f, 0x0000, 0x0000, 0x03, 0x00, 0x0000, 0x0000},
	/* Capture mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x1070, 0x0C30, 0x00, 0x35, 0x0280, 0x0001,
	 0x00, 0x2f, 0x00A0, 0x0C00, 0x03, 0x00, 0x0000, 0x0000},
	/* Video mode setting */
	{0x02, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x1070, 0x0C30, 0x01, 0x00, 0x0000, 0x0000,
	 0x00, 0x2f, 0x00A0, 0x0C00, 0x03, 0x00, 0x0000, 0x0000}
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 24,
	.i4OffsetY = 24,
	.i4PitchX = 32,
	.i4PitchY = 32,
	.i4PairNum = 8,
	.i4SubBlkW = 8,
	.i4SubBlkH = 16,
	.i4PosL = {{26, 29}, {34, 29}, {42, 29}, {50, 29},
		   {30, 45}, {38, 45}, {46, 45}, {54, 45} },
	.i4PosR = {{26, 33}, {34, 33}, {42, 33}, {50, 33},
		   {30, 49}, {38, 49}, {46, 49}, {54, 49} },

	/* 0:IMAGE_NORMAL,1:IMAGE_H_MIRROR,2:IMAGE_V_MIRROR,3:IMAGE_HV_MIRROR */
	.iMirrorFlip = 0,
};

/* Binning Type VC information*/
static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO_Binning[3] = {
	/* Preview mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x0834, 0x0618, 0x00, 0x35, 0x0280, 0x0001,
	 0x00, 0x2f, 0x00A0, 0x0602, 0x03, 0x00, 0x0000, 0x0000},
	/* Capture mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x1070, 0x0C30, 0x00, 0x35, 0x0280, 0x0001,
	 0x00, 0x2f, 0x00A0, 0x0C00, 0x03, 0x00, 0x0000, 0x0000},
	/* Video mode setting */
	{0x02, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x1070, 0x0C30, 0x01, 0x00, 0x0000, 0x0000,
	 0x00, 0x2f, 0x00A0, 0x0C00, 0x03, 0x00, 0x0000, 0x0000}
};

/*HDR mode PD position information*/
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_Binning = {
	.i4OffsetX = 24,
	.i4OffsetY = 24,
	.i4PitchX = 32,
	.i4PitchY = 32,
	.i4PairNum = 4,
	.i4SubBlkW = 16,
	.i4SubBlkH = 16,
	.i4PosL = {{30, 31}, {46, 31}, {37, 52}, {53, 52} },
	.i4PosR = {{29, 36}, {45, 36}, {38, 47}, {54, 47} },
};


#define IMX258MIPI_MaxGainIndex (154)
kal_uint16 IMX258MIPI_sensorGainMapping[IMX258MIPI_MaxGainIndex][2] = {
	{64, 0},
	{65, 8},
	{66, 16},
	{67, 25},
	{68, 30},
	{69, 37},
	{70, 45},
	{71, 51},
	{72, 57},
	{73, 63},
	{74, 67},
	{75, 75},
	{76, 81},
	{77, 85},
	{78, 92},
	{79, 96},
	{80, 103},
	{81, 107},
	{82, 112},
	{83, 118},
	{84, 122},
	{86, 133},
	{88, 140},
	{89, 144},
	{90, 148},
	{93, 159},
	{96, 171},
	{97, 175},
	{99, 182},
	{101, 188},
	{102, 192},
	{104, 197},
	{106, 202},
	{107, 206},
	{109, 211},
	{112, 220},
	{113, 222},
	{115, 228},
	{118, 235},
	{120, 239},
	{125, 250},
	{126, 252},
	{128, 256},
	{129, 258},
	{130, 260},
	{132, 264},
	{133, 266},
	{135, 269},
	{136, 271},
	{138, 274},
	{139, 276},
	{141, 279},
	{142, 282},
	{144, 285},
	{145, 286},
	{147, 290},
	{149, 292},
	{150, 294},
	{155, 300},
	{157, 303},
	{158, 305},
	{161, 309},
	{163, 311},
	{170, 319},
	{172, 322},
	{174, 324},
	{176, 326},
	{179, 329},
	{181, 331},
	{185, 335},
	{189, 339},
	{193, 342},
	{195, 344},
	{196, 345},
	{200, 348},
	{202, 350},
	{205, 352},
	{207, 354},
	{210, 356},
	{211, 357},
	{214, 359},
	{217, 361},
	{218, 362},
	{221, 364},
	{224, 366},
	{231, 370},
	{237, 374},
	{246, 379},
	{250, 381},
	{252, 382},
	{256, 384},
	{260, 386},
	{262, 387},
	{273, 392},
	{275, 393},
	{280, 395},
	{290, 399},
	{306, 405},
	{312, 407},
	{321, 410},
	{331, 413},
	{345, 417},
	{352, 419},
	{360, 421},
	{364, 422},
	{372, 424},
	{386, 427},
	{400, 430},
	{410, 432},
	{420, 434},
	{431, 436},
	{437, 437},
	{449, 439},
	{455, 440},
	{461, 441},
	{468, 442},
	{475, 443},
	{482, 444},
	{489, 445},
	{496, 446},
	{504, 447},
	{512, 448},
	{520, 449},
	{529, 450},
	{537, 451},
	{546, 452},
	{555, 453},
	{565, 454},
	{575, 455},
	{585, 456},
	{596, 457},
	{607, 458},
	{618, 459},
	{630, 460},
	{642, 461},
	{655, 462},
	{669, 463},
	{683, 464},
	{697, 465},
	{713, 466},
	{728, 467},
	{745, 468},
	{762, 469},
	{780, 470},
	{799, 471},
	{819, 472},
	{840, 473},
	{862, 474},
	{886, 475},
	{910, 476},
	{936, 477},
	{964, 478},
	{993, 479},
	{1024, 480}
};

static kal_uint8 IMX258MIPI_SPC_Data[126];
static kal_uint8 SPC_data_done;
static void load_imx258_SPC_Data(struct subdrv_ctx *ctx)
{
	if (SPC_data_done == false) {
		if (!read_imx258_eeprom_SPC(ctx, 0x0F73, IMX258MIPI_SPC_Data, 126)) {
			LOG_INF("imx258 load spc fail\n");
			return;
		}
		SPC_data_done = true;
	}
}

static void write_imx258_SPC_Data(struct subdrv_ctx *ctx)
{
	kal_uint16 i;

	if (SPC_data_done == false) {
		if (!read_imx258_eeprom_SPC(ctx, 0x0F73, IMX258MIPI_SPC_Data, 126)) {
			LOG_INF("imx258 load spc fail\n");
			return;
		}
		SPC_data_done = true;
	}

	for (i = 0; i < 63; i++) {
		write_cmos_sensor(ctx, 0xD04C + i, IMX258MIPI_SPC_Data[i]);
		/* LOG_INF("SPC_Data[%d] = %d\n", i, */
		/*	IMX258MIPI_SPC_Data[i]); */
	}
	for (i = 0; i < 63; i++) {
		write_cmos_sensor(ctx, 0xD08C + i, IMX258MIPI_SPC_Data[i + 63]);
		/* LOG_INF("SPC_Data[%d] = %d\n", i+63, */
		/*	IMX258MIPI_SPC_Data[i+63]); */
	}
}

static void set_dummy(struct subdrv_ctx *ctx)
{
	/*
	 *LOG_INF("dummyline = %d, dummypixels = %d\n",
	 *ctx->dummy_line, ctx->dummy_pixel);
	 */

	write_cmos_sensor(ctx, 0x0104, 0x01);

	write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
	write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
	write_cmos_sensor(ctx, 0x0342, ctx->line_length >> 8);
	write_cmos_sensor(ctx, 0x0343, ctx->line_length & 0xFF);

	write_cmos_sensor(ctx, 0x0104, 0x00);
}

static kal_uint32 return_sensor_id(struct subdrv_ctx *ctx)
{
/* kal_uint32 tmp = 0; */
	int retry = 10;

	if (write_cmos_sensor(ctx, 0x0A02, 0x0F) == 0) {
		write_cmos_sensor(ctx, 0x0A00, 0x01);
		while (retry--) {
			if (read_cmos_sensor(ctx, 0x0A01) == 0x01) {
				imx258_type = read_cmos_sensor(ctx, 0x0A2E);

				if (imx258_type == IMX258_HDD_TYPE) {
					if (test_Pmode)
						imx258_type = IMX258_RAW_TYPE;
					else
						imx258_type = IMX258_HDD_TYPE;
				}

				LOG_INF(
				    "imx258 type = 0x%x(0x00=HDR,0x10=binning,0x40=HDD,0x80=RAW)",
				    imx258_type);
				return (kal_uint16) (
					(read_cmos_sensor(ctx, 0x0A26) << 4) |
					(read_cmos_sensor(ctx, 0x0A27) >> 4));
			}
		}
	}

	return 0x00;

}

static void set_max_framerate(struct subdrv_ctx *ctx, UINT16 framerate, kal_bool min_framelength_en)
{
	/* kal_int16 dummy_line; */
	kal_uint32 frame_length = ctx->frame_length;
	/* unsigned long flags; */

	LOG_INF("framerate = %d, min framelength should enable %d\n", framerate,
		min_framelength_en);

	frame_length = ctx->pclk / framerate * 10 / ctx->line_length;
	ctx->frame_length =
	    (frame_length > ctx->min_frame_length)
	    ? frame_length : ctx->min_frame_length;

	ctx->dummy_line =
		ctx->frame_length - ctx->min_frame_length;

	/* dummy_line = frame_length - ctx->min_frame_length; */
	/* if (dummy_line < 0) */
	/* ctx->dummy_line = 0; */
	/* else */
	/* ctx->dummy_line = dummy_line; */
	/* ctx->frame_length = frame_length + ctx->dummy_line; */
	if (ctx->frame_length > imgsensor_info.max_frame_length) {
		ctx->frame_length = imgsensor_info.max_frame_length;

		ctx->dummy_line =
			ctx->frame_length - ctx->min_frame_length;
	}
	if (min_framelength_en)
		ctx->min_frame_length = ctx->frame_length;
	set_dummy(ctx);
}				/*      set_max_framerate  */


static void set_shutter(struct subdrv_ctx *ctx, kal_uint16 shutter)
{	kal_uint16 realtime_fps = 0;

	ctx->shutter = shutter;

	/* write_shutter(shutter); */
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK
	 * to get exposure larger than frame exposure
	 */
	/* AE doesn't update sensor gain at capture mode,
	 * thus extra exposure lines must be updated here.
	 */

	/* OV Recommend Solution */
	/* if shutter bigger than frame_length,
	 * should extend frame length first
	 */
	if (shutter > ctx->min_frame_length - imgsensor_info.margin)
		ctx->frame_length = shutter + imgsensor_info.margin;
	else
		ctx->frame_length = ctx->min_frame_length;
	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;

	shutter =
(shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;

	shutter =
	(shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
	? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (ctx->autoflicker_en) {
		realtime_fps = ctx->pclk
			/ ctx->line_length * 10 / ctx->frame_length;

		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(ctx, 146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(ctx, 0x0104, 0x01);
			write_cmos_sensor(ctx, 0x0340,
				ctx->frame_length >> 8);
			write_cmos_sensor(ctx, 0x0341,
				ctx->frame_length & 0xFF);
			write_cmos_sensor(ctx, 0x0104, 0x00);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(ctx, 0x0104, 0x01);
		write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
		write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
		write_cmos_sensor(ctx, 0x0104, 0x00);
	}

	/* Update Shutter */
	write_cmos_sensor(ctx, 0x0104, 0x01);
	write_cmos_sensor(ctx, 0x0350, 0x01);	/* Enable auto extend */
	write_cmos_sensor(ctx, 0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor(ctx, 0x0203, shutter & 0xFF);
	write_cmos_sensor(ctx, 0x0104, 0x00);
	LOG_INF("Exit! shutter =%d, framelength =%d, auto_extend=%d\n", shutter,
		ctx->frame_length, read_cmos_sensor(ctx, 0x0350));
}				/*    set_shutter */

static void set_frame_length(struct subdrv_ctx *ctx, kal_uint16 frame_length)
{
	if (frame_length > 1)
		ctx->frame_length = frame_length;

	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;
	if (ctx->min_frame_length > ctx->frame_length)
		ctx->frame_length = ctx->min_frame_length;

	/* Extend frame length */
	write_cmos_sensor(ctx, 0x0104, 0x01);
	write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
	write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
	write_cmos_sensor(ctx, 0x0104, 0x00);

	LOG_INF("Framelength: set=%d/input=%d/min=%d, auto_extend=%d\n",
		ctx->frame_length, frame_length, ctx->min_frame_length,
		read_cmos_sensor(ctx, 0x0350));
}

static void set_shutter_frame_length(struct subdrv_ctx *ctx,
				kal_uint16 shutter, kal_uint16 frame_length)
{	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	ctx->shutter = shutter;
	/* LOG_INF("shutter =%d, frame_time =%d\n", shutter, frame_time); */

	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger
	 * than frame exposure
	 */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure
	 * lines must be updated here.
	 */

	/* OV Recommend Solution */
	/* if shutter bigger than frame_length,
	 * should extend frame length first
	 */
	/*Change frame time */
	if (frame_length > 1)
		dummy_line = frame_length - ctx->frame_length;
	ctx->frame_length = ctx->frame_length + dummy_line;

	/*  */
	if (shutter > ctx->frame_length - imgsensor_info.margin)
		ctx->frame_length = shutter + imgsensor_info.margin;

	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;

	shutter =
(shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;

	shutter =
	(shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
	? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (ctx->autoflicker_en) {
		realtime_fps = ctx->pclk
			/ ctx->line_length * 10 / ctx->frame_length;

		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(ctx, 146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(ctx, 0x0104, 0x01);
			write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
		    write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
			write_cmos_sensor(ctx, 0x0104, 0x00);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(ctx, 0x0104, 0x01);
		write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
		write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
		write_cmos_sensor(ctx, 0x0104, 0x00);
	}

	/* Update Shutter */
	write_cmos_sensor(ctx, 0x0104, 0x01);
	write_cmos_sensor(ctx, 0x0350, 0x00);	/* Disable auto extend */
	write_cmos_sensor(ctx, 0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor(ctx, 0x0203, shutter & 0xFF);
	write_cmos_sensor(ctx, 0x0104, 0x00);

	LOG_INF(
	    "Exit! shutter =%d, framelength =%d/%d, dummy_line=%d, auto_extend=%d\n",
	    shutter, ctx->frame_length,
	    frame_length, dummy_line,
	    read_cmos_sensor(ctx, 0x0350));
}	/* set_shutter_frame_length */


static kal_uint16 gain2reg(struct subdrv_ctx *ctx, const kal_uint32 gain)
{
	kal_uint8 i;

	for (i = 0; i < IMX258MIPI_MaxGainIndex; i++) {
		if (gain <= IMX258MIPI_sensorGainMapping[i][0]*16)
			break;
	}
	if (gain != IMX258MIPI_sensorGainMapping[i][0]*16)
		LOG_INF("Gain mapping don't correctly:%d %d\n", gain,
			IMX258MIPI_sensorGainMapping[i][0]);
	return IMX258MIPI_sensorGainMapping[i][1];
}

/*************************************************************************
 * FUNCTION
 *	set_gain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *	iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint16 set_gain(struct subdrv_ctx *ctx, kal_uint32 gain)
{
	kal_uint16 reg_gain;

	/* 0x350A[0:1], 0x350B[0:7] AGC real gain */
	/* [0:3] = N meams N /16 X        */
	/* [4:9] = M meams M X             */
	/* Total gain = M + N /16 X   */

	/*  */
	if (gain < BASEGAIN || gain > 16 * BASEGAIN) {
		LOG_INF("Error gain setting");

		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 16 * BASEGAIN)
			gain = 16 * BASEGAIN;
	}

	reg_gain = gain2reg(ctx, gain);
	ctx->gain = reg_gain;
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(ctx, 0x0104, 0x01);
	write_cmos_sensor(ctx, 0x0204, (reg_gain >> 8) & 0xFF);
	write_cmos_sensor(ctx, 0x0205, reg_gain & 0xFF);
	write_cmos_sensor(ctx, 0x0104, 0x00);

	return gain;
}				/*      set_gain  */

static void ihdr_write_shutter_gain(struct subdrv_ctx *ctx,
				kal_uint16 le, kal_uint16 se, kal_uint32 gain)
{

	kal_uint16 realtime_fps = 0;
	kal_uint16 reg_gain;

	if (le > ctx->min_frame_length - imgsensor_info.margin)
		ctx->frame_length = le + imgsensor_info.margin;
	else
		ctx->frame_length = ctx->min_frame_length;
	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;
	if (le < imgsensor_info.min_shutter)
		le = imgsensor_info.min_shutter;
	if (ctx->autoflicker_en) {
		realtime_fps = ctx->pclk
			/ ctx->line_length * 10 / ctx->frame_length;

		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(ctx, 146, 0);
		else {
			write_cmos_sensor(ctx, 0x0104, 0x01);
			write_cmos_sensor(ctx, 0x0340,
				ctx->frame_length >> 8);
			write_cmos_sensor(ctx, 0x0341,
				ctx->frame_length & 0xFF);
			write_cmos_sensor(ctx, 0x0104, 0x00);
		}
	} else {
		write_cmos_sensor(ctx, 0x0104, 0x01);
		write_cmos_sensor(ctx, 0x0340, ctx->frame_length >> 8);
		write_cmos_sensor(ctx, 0x0341, ctx->frame_length & 0xFF);
		write_cmos_sensor(ctx, 0x0104, 0x00);
	}
	write_cmos_sensor(ctx, 0x0104, 0x01);
	write_cmos_sensor(ctx, 0x0350, 0x01);	/* Enable auto extend */
	/* Long exposure */
	write_cmos_sensor(ctx, 0x0202, (le >> 8) & 0xFF);
	write_cmos_sensor(ctx, 0x0203, le & 0xFF);
	/* Short exposure */
	write_cmos_sensor(ctx, 0x0224, (se >> 8) & 0xFF);
	write_cmos_sensor(ctx, 0x0225, se & 0xFF);
	reg_gain = gain2reg(ctx, gain);
	ctx->gain = reg_gain;
	/* Global analog Gain for Long expo */
	write_cmos_sensor(ctx, 0x0204, (reg_gain >> 8) & 0xFF);
	write_cmos_sensor(ctx, 0x0205, reg_gain & 0xFF);
	/* Global analog Gain for Short expo */
	/* write_cmos_sensor(ctx, 0x0216, (reg_gain>>8)& 0xFF); */
	/* write_cmos_sensor(ctx, 0x0217, reg_gain & 0xFF); */
	write_cmos_sensor(ctx, 0x0104, 0x00);
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x, auto_extend=%d\n", le, se, gain,
		read_cmos_sensor(ctx, 0x0350));
}

static void set_mirror_flip(struct subdrv_ctx *ctx, kal_uint8 image_mirror)
{
	kal_uint8 iTemp;

	LOG_INF("image_mirror = %d\n", image_mirror);
	iTemp = read_cmos_sensor(ctx, 0x0101);
	iTemp &= ~0x03;		/* Clear the mirror and flip bits. */

	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor(ctx, 0x0101, iTemp | 0x00);
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor(ctx, 0x0101, iTemp | 0x01);
		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor(ctx, 0x0101, iTemp | 0x02);
		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor(ctx, 0x0101, iTemp | 0x03);
		break;
	default:
		LOG_INF("Error image_mirror setting\n");
	}

}

/*************************************************************************
 * FUNCTION
 *	night_mode
 *
 * DESCRIPTION
 *	This function night mode of sensor.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void night_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
/*No Need to implement this function*/
}				/*      night_mode      */

	/*      preview_setting  */
static void imx258_ImageQuality_Setting(struct subdrv_ctx *ctx)
{
	if (imx258_type == IMX258_HDR_TYPE || imx258_type == IMX258_RAW_TYPE) {
		write_cmos_sensor(ctx, 0x94C7, 0xFF);
		write_cmos_sensor(ctx, 0x94C8, 0xFF);
		write_cmos_sensor(ctx, 0x94C9, 0xFF);
		write_cmos_sensor(ctx, 0x95C7, 0xFF);
		write_cmos_sensor(ctx, 0x95C8, 0xFF);
		write_cmos_sensor(ctx, 0x95C9, 0xFF);
		write_cmos_sensor(ctx, 0x94C4, 0x3F);
		write_cmos_sensor(ctx, 0x94C5, 0x3F);
		write_cmos_sensor(ctx, 0x94C6, 0x3F);
		write_cmos_sensor(ctx, 0x95C4, 0x3F);
		write_cmos_sensor(ctx, 0x95C5, 0x3F);
		write_cmos_sensor(ctx, 0x95C6, 0x3F);
		write_cmos_sensor(ctx, 0x94C1, 0x02);
		write_cmos_sensor(ctx, 0x94C2, 0x02);
		write_cmos_sensor(ctx, 0x94C3, 0x02);
		write_cmos_sensor(ctx, 0x95C1, 0x02);
		write_cmos_sensor(ctx, 0x95C2, 0x02);
		write_cmos_sensor(ctx, 0x95C3, 0x02);
		write_cmos_sensor(ctx, 0x94BE, 0x0C);
		write_cmos_sensor(ctx, 0x94BF, 0x0C);
		write_cmos_sensor(ctx, 0x94C0, 0x0C);
		write_cmos_sensor(ctx, 0x95BE, 0x0C);
		write_cmos_sensor(ctx, 0x95BF, 0x0C);
		write_cmos_sensor(ctx, 0x95C0, 0x0C);
		write_cmos_sensor(ctx, 0x94D0, 0x74);
		write_cmos_sensor(ctx, 0x94D1, 0x74);
		write_cmos_sensor(ctx, 0x94D2, 0x74);
		write_cmos_sensor(ctx, 0x95D0, 0x74);
		write_cmos_sensor(ctx, 0x95D1, 0x74);
		write_cmos_sensor(ctx, 0x95D2, 0x74);
		write_cmos_sensor(ctx, 0x94CD, 0x2E);
		write_cmos_sensor(ctx, 0x94CE, 0x2E);
		write_cmos_sensor(ctx, 0x94CF, 0x2E);
		write_cmos_sensor(ctx, 0x95CD, 0x2E);
		write_cmos_sensor(ctx, 0x95CE, 0x2E);
		write_cmos_sensor(ctx, 0x95CF, 0x2E);
		write_cmos_sensor(ctx, 0x94CA, 0x4C);
		write_cmos_sensor(ctx, 0x94CB, 0x4C);
		write_cmos_sensor(ctx, 0x94CC, 0x4C);
		write_cmos_sensor(ctx, 0x95CA, 0x4C);
		write_cmos_sensor(ctx, 0x95CB, 0x4C);
		write_cmos_sensor(ctx, 0x95CC, 0x4C);
		write_cmos_sensor(ctx, 0x900E, 0x32);
		write_cmos_sensor(ctx, 0x94E2, 0xFF);
		write_cmos_sensor(ctx, 0x94E3, 0xFF);
		write_cmos_sensor(ctx, 0x94E4, 0xFF);
		write_cmos_sensor(ctx, 0x95E2, 0xFF);
		write_cmos_sensor(ctx, 0x95E3, 0xFF);
		write_cmos_sensor(ctx, 0x95E4, 0xFF);
		write_cmos_sensor(ctx, 0x94DF, 0x6E);
		write_cmos_sensor(ctx, 0x94E0, 0x6E);
		write_cmos_sensor(ctx, 0x94E1, 0x6E);
		write_cmos_sensor(ctx, 0x95DF, 0x6E);
		write_cmos_sensor(ctx, 0x95E0, 0x6E);
		write_cmos_sensor(ctx, 0x95E1, 0x6E);
		write_cmos_sensor(ctx, 0x7FCC, 0x01);
		write_cmos_sensor(ctx, 0x7B78, 0x00);
	} else if (imx258_type == IMX258_BINNING_TYPE) {
		write_cmos_sensor(ctx, 0x94C7, 0xFF);
		write_cmos_sensor(ctx, 0x94C8, 0xFF);
		write_cmos_sensor(ctx, 0x94C9, 0xFF);
		write_cmos_sensor(ctx, 0x95C7, 0xFF);
		write_cmos_sensor(ctx, 0x95C8, 0xFF);
		write_cmos_sensor(ctx, 0x95C9, 0xFF);
		write_cmos_sensor(ctx, 0x94C4, 0x3F);
		write_cmos_sensor(ctx, 0x94C5, 0x3F);
		write_cmos_sensor(ctx, 0x94C6, 0x3F);
		write_cmos_sensor(ctx, 0x95C4, 0x3F);

		write_cmos_sensor(ctx, 0x95C5, 0x3F);
		write_cmos_sensor(ctx, 0x95C6, 0x3F);
		write_cmos_sensor(ctx, 0x94C1, 0x02);
		write_cmos_sensor(ctx, 0x94C2, 0x02);
		write_cmos_sensor(ctx, 0x94C3, 0x02);
		write_cmos_sensor(ctx, 0x95C1, 0x02);
		write_cmos_sensor(ctx, 0x95C2, 0x02);
		write_cmos_sensor(ctx, 0x95C3, 0x02);
		write_cmos_sensor(ctx, 0x94BE, 0x0C);
		write_cmos_sensor(ctx, 0x94BF, 0x0C);
		write_cmos_sensor(ctx, 0x94C0, 0x0C);
		write_cmos_sensor(ctx, 0x95BE, 0x0C);
		write_cmos_sensor(ctx, 0x95BF, 0x0C);
		write_cmos_sensor(ctx, 0x95C0, 0x0C);
		write_cmos_sensor(ctx, 0x94D0, 0x74);
		write_cmos_sensor(ctx, 0x94D1, 0x74);
		write_cmos_sensor(ctx, 0x94D2, 0x74);
		write_cmos_sensor(ctx, 0x95D0, 0x74);
		write_cmos_sensor(ctx, 0x95D1, 0x74);
		write_cmos_sensor(ctx, 0x95D2, 0x74);
		write_cmos_sensor(ctx, 0x94CD, 0x16);
		write_cmos_sensor(ctx, 0x94CE, 0x16);
		write_cmos_sensor(ctx, 0x94CF, 0x16);
		write_cmos_sensor(ctx, 0x95CD, 0x16);
		write_cmos_sensor(ctx, 0x95CE, 0x16);
		write_cmos_sensor(ctx, 0x95CF, 0x16);
		write_cmos_sensor(ctx, 0x94CA, 0x28);
		write_cmos_sensor(ctx, 0x94CB, 0x28);
		write_cmos_sensor(ctx, 0x94CC, 0x28);
		write_cmos_sensor(ctx, 0x95CA, 0x28);
		write_cmos_sensor(ctx, 0x95CB, 0x28);
		write_cmos_sensor(ctx, 0x95CC, 0x28);
		write_cmos_sensor(ctx, 0x900E, 0x32);
		write_cmos_sensor(ctx, 0x94E2, 0xFF);
		write_cmos_sensor(ctx, 0x94E3, 0xFF);
		write_cmos_sensor(ctx, 0x94E4, 0xFF);
		write_cmos_sensor(ctx, 0x95E2, 0xFF);
		write_cmos_sensor(ctx, 0x95E3, 0xFF);
		write_cmos_sensor(ctx, 0x95E4, 0xFF);
		write_cmos_sensor(ctx, 0x94DF, 0x6E);
		write_cmos_sensor(ctx, 0x94E0, 0x6E);
		write_cmos_sensor(ctx, 0x94E1, 0x6E);
		write_cmos_sensor(ctx, 0x95DF, 0x6E);
		write_cmos_sensor(ctx, 0x95E0, 0x6E);
		write_cmos_sensor(ctx, 0x95E1, 0x6E);
		write_cmos_sensor(ctx, 0x7FCC, 0x01);
		write_cmos_sensor(ctx, 0x7B78, 0x00);
		write_cmos_sensor(ctx, 0x9401, 0x35);
		write_cmos_sensor(ctx, 0x9403, 0x23);
		write_cmos_sensor(ctx, 0x9405, 0x23);
		write_cmos_sensor(ctx, 0x9406, 0x00);
		write_cmos_sensor(ctx, 0x9407, 0x31);
		write_cmos_sensor(ctx, 0x9408, 0x00);
		write_cmos_sensor(ctx, 0x9409, 0x1B);
		write_cmos_sensor(ctx, 0x940A, 0x00);
		write_cmos_sensor(ctx, 0x940B, 0x15);
		write_cmos_sensor(ctx, 0x940D, 0x3F);
		write_cmos_sensor(ctx, 0x940F, 0x3F);
		write_cmos_sensor(ctx, 0x9411, 0x3F);
		write_cmos_sensor(ctx, 0x9413, 0x64);
		write_cmos_sensor(ctx, 0x9415, 0x64);
		write_cmos_sensor(ctx, 0x9417, 0x64);
		write_cmos_sensor(ctx, 0x941D, 0x34);
		write_cmos_sensor(ctx, 0x941F, 0x01);
		write_cmos_sensor(ctx, 0x9421, 0x01);
		write_cmos_sensor(ctx, 0x9423, 0x01);
		write_cmos_sensor(ctx, 0x9425, 0x23);
		write_cmos_sensor(ctx, 0x9427, 0x23);
		write_cmos_sensor(ctx, 0x9429, 0x23);
		write_cmos_sensor(ctx, 0x942B, 0x2F);
		write_cmos_sensor(ctx, 0x942D, 0x1A);
		write_cmos_sensor(ctx, 0x942F, 0x14);
		write_cmos_sensor(ctx, 0x9431, 0x3F);
		write_cmos_sensor(ctx, 0x9433, 0x3F);
		write_cmos_sensor(ctx, 0x9435, 0x3F);
		write_cmos_sensor(ctx, 0x9437, 0x6B);
		write_cmos_sensor(ctx, 0x9439, 0x7C);
		write_cmos_sensor(ctx, 0x943B, 0x81);
		write_cmos_sensor(ctx, 0x9443, 0x0F);
		write_cmos_sensor(ctx, 0x9445, 0x0F);
		write_cmos_sensor(ctx, 0x9447, 0x0F);
		write_cmos_sensor(ctx, 0x9449, 0x0F);
		write_cmos_sensor(ctx, 0x944B, 0x0F);
		write_cmos_sensor(ctx, 0x944D, 0x0F);
		write_cmos_sensor(ctx, 0x944F, 0x1E);
		write_cmos_sensor(ctx, 0x9451, 0x0F);
		write_cmos_sensor(ctx, 0x9453, 0x0B);
		write_cmos_sensor(ctx, 0x9455, 0x28);
		write_cmos_sensor(ctx, 0x9457, 0x13);
		write_cmos_sensor(ctx, 0x9459, 0x0C);
		write_cmos_sensor(ctx, 0x945D, 0x00);
		write_cmos_sensor(ctx, 0x945E, 0x00);
		write_cmos_sensor(ctx, 0x945F, 0x00);
		write_cmos_sensor(ctx, 0x946D, 0x00);
		write_cmos_sensor(ctx, 0x946F, 0x10);
		write_cmos_sensor(ctx, 0x9471, 0x10);
		write_cmos_sensor(ctx, 0x9473, 0x40);
		write_cmos_sensor(ctx, 0x9475, 0x2E);
		write_cmos_sensor(ctx, 0x9477, 0x10);
		write_cmos_sensor(ctx, 0x9478, 0x0A);
		write_cmos_sensor(ctx, 0x947B, 0xE0);
		write_cmos_sensor(ctx, 0x947C, 0xE0);
		write_cmos_sensor(ctx, 0x947D, 0xE0);
		write_cmos_sensor(ctx, 0x947E, 0xE0);
		write_cmos_sensor(ctx, 0x947F, 0xE0);
		write_cmos_sensor(ctx, 0x9480, 0xE0);
		write_cmos_sensor(ctx, 0x9483, 0x14);
		write_cmos_sensor(ctx, 0x9485, 0x14);
		write_cmos_sensor(ctx, 0x9487, 0x14);
		write_cmos_sensor(ctx, 0x9501, 0x35);
		write_cmos_sensor(ctx, 0x9503, 0x14);
		write_cmos_sensor(ctx, 0x9505, 0x14);
		write_cmos_sensor(ctx, 0x9507, 0x31);
		write_cmos_sensor(ctx, 0x9509, 0x1B);
		write_cmos_sensor(ctx, 0x950B, 0x15);
		write_cmos_sensor(ctx, 0x950D, 0x1E);
		write_cmos_sensor(ctx, 0x950F, 0x1E);
		write_cmos_sensor(ctx, 0x9511, 0x1E);
		write_cmos_sensor(ctx, 0x9513, 0x64);
		write_cmos_sensor(ctx, 0x9515, 0x64);
		write_cmos_sensor(ctx, 0x9517, 0x64);
		write_cmos_sensor(ctx, 0x951D, 0x34);
		write_cmos_sensor(ctx, 0x951F, 0x01);
		write_cmos_sensor(ctx, 0x9521, 0x01);
		write_cmos_sensor(ctx, 0x9523, 0x01);
		write_cmos_sensor(ctx, 0x9525, 0x14);
		write_cmos_sensor(ctx, 0x9527, 0x14);
		write_cmos_sensor(ctx, 0x9529, 0x14);
		write_cmos_sensor(ctx, 0x952B, 0x2F);
		write_cmos_sensor(ctx, 0x952D, 0x1A);
		write_cmos_sensor(ctx, 0x952F, 0x14);
		write_cmos_sensor(ctx, 0x9531, 0x1E);
		write_cmos_sensor(ctx, 0x9533, 0x1E);
		write_cmos_sensor(ctx, 0x9535, 0x1E);
		write_cmos_sensor(ctx, 0x9537, 0x6B);
		write_cmos_sensor(ctx, 0x9539, 0x7C);
		write_cmos_sensor(ctx, 0x953B, 0x81);
		write_cmos_sensor(ctx, 0x9543, 0x0F);
		write_cmos_sensor(ctx, 0x9545, 0x0F);
		write_cmos_sensor(ctx, 0x9547, 0x0F);
		write_cmos_sensor(ctx, 0x9549, 0x0F);
		write_cmos_sensor(ctx, 0x954B, 0x0F);
		write_cmos_sensor(ctx, 0x954D, 0x0F);
		write_cmos_sensor(ctx, 0x954F, 0x15);
		write_cmos_sensor(ctx, 0x9551, 0x0B);
		write_cmos_sensor(ctx, 0x9553, 0x08);
		write_cmos_sensor(ctx, 0x9555, 0x1C);
		write_cmos_sensor(ctx, 0x9557, 0x0D);
		write_cmos_sensor(ctx, 0x9559, 0x08);
		write_cmos_sensor(ctx, 0x955D, 0x00);
		write_cmos_sensor(ctx, 0x955E, 0x00);
		write_cmos_sensor(ctx, 0x955F, 0x00);
		write_cmos_sensor(ctx, 0x956D, 0x00);
		write_cmos_sensor(ctx, 0x956F, 0x10);
		write_cmos_sensor(ctx, 0x9571, 0x10);
		write_cmos_sensor(ctx, 0x9573, 0x40);
		write_cmos_sensor(ctx, 0x9575, 0x2E);
		write_cmos_sensor(ctx, 0x9577, 0x10);
		write_cmos_sensor(ctx, 0x9578, 0x0A);
		write_cmos_sensor(ctx, 0x957B, 0xE0);
		write_cmos_sensor(ctx, 0x957C, 0xE0);
		write_cmos_sensor(ctx, 0x957D, 0xE0);
		write_cmos_sensor(ctx, 0x957E, 0xE0);
		write_cmos_sensor(ctx, 0x957F, 0xE0);
		write_cmos_sensor(ctx, 0x9580, 0xE0);
		write_cmos_sensor(ctx, 0x9583, 0x14);
		write_cmos_sensor(ctx, 0x9585, 0x14);
		write_cmos_sensor(ctx, 0x9587, 0x14);
		write_cmos_sensor(ctx, 0x7F78, 0x00);
		write_cmos_sensor(ctx, 0x7F89, 0x00);
		write_cmos_sensor(ctx, 0x7F93, 0x00);
		write_cmos_sensor(ctx, 0x924B, 0x1B);
		write_cmos_sensor(ctx, 0x924C, 0x0A);
		write_cmos_sensor(ctx, 0x9304, 0x04);
		write_cmos_sensor(ctx, 0x9315, 0x04);
		write_cmos_sensor(ctx, 0x9250, 0x50);
		write_cmos_sensor(ctx, 0x9251, 0x3C);
		write_cmos_sensor(ctx, 0x9252, 0x14);
		write_cmos_sensor(ctx, 0x7B5F, 0x01);
		write_cmos_sensor(ctx, 0x94DC, 0x20);
		write_cmos_sensor(ctx, 0x94DD, 0x20);
		write_cmos_sensor(ctx, 0x94DE, 0x20);
		write_cmos_sensor(ctx, 0x95DC, 0x20);
		write_cmos_sensor(ctx, 0x95DD, 0x20);
		write_cmos_sensor(ctx, 0x95DE, 0x20);
		write_cmos_sensor(ctx, 0x7FB0, 0x01);
		write_cmos_sensor(ctx, 0x9010, 0x48);
		write_cmos_sensor(ctx, 0x9419, 0x50);
		write_cmos_sensor(ctx, 0x941B, 0x50);
		write_cmos_sensor(ctx, 0x9519, 0x50);
		write_cmos_sensor(ctx, 0x951B, 0x50);


		write_cmos_sensor(ctx, 0xD000, 0x00);
		write_cmos_sensor(ctx, 0xD001, 0x18);
		write_cmos_sensor(ctx, 0xD002, 0x00);
		write_cmos_sensor(ctx, 0xD003, 0x18);
		write_cmos_sensor(ctx, 0xD004, 0x10);
		write_cmos_sensor(ctx, 0xD005, 0x57);
		write_cmos_sensor(ctx, 0xD006, 0x0C);
		write_cmos_sensor(ctx, 0xD007, 0x17);
		write_cmos_sensor(ctx, 0xD00A, 0x01);
		write_cmos_sensor(ctx, 0xD00B, 0x02);
		write_cmos_sensor(ctx, 0xD00D, 0x06);
		write_cmos_sensor(ctx, 0xD00E, 0x05);
		write_cmos_sensor(ctx, 0xD00F, 0x01);
		write_cmos_sensor(ctx, 0xD011, 0x06);
		write_cmos_sensor(ctx, 0xD012, 0x07);
		write_cmos_sensor(ctx, 0xD013, 0x01);
		write_cmos_sensor(ctx, 0xD015, 0x05);
		write_cmos_sensor(ctx, 0xD016, 0x0C);
		write_cmos_sensor(ctx, 0xD017, 0x02);
		write_cmos_sensor(ctx, 0xD019, 0x05);
		write_cmos_sensor(ctx, 0xD01A, 0x0E);
		write_cmos_sensor(ctx, 0xD01B, 0x02);
		write_cmos_sensor(ctx, 0xD01D, 0x0E);
		write_cmos_sensor(ctx, 0xD01E, 0x15);
		write_cmos_sensor(ctx, 0xD01F, 0x02);
		write_cmos_sensor(ctx, 0xD021, 0x0E);
		write_cmos_sensor(ctx, 0xD022, 0x17);
		write_cmos_sensor(ctx, 0xD023, 0x02);
		write_cmos_sensor(ctx, 0xD025, 0x0D);
		write_cmos_sensor(ctx, 0xD026, 0x1C);
		write_cmos_sensor(ctx, 0xD027, 0x01);
		write_cmos_sensor(ctx, 0xD029, 0x0D);
		write_cmos_sensor(ctx, 0xD02A, 0x1E);
		write_cmos_sensor(ctx, 0xD02B, 0x01);
	}
}


static void sensor_init(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	/* init setting */
	/* imx258 */
	write_cmos_sensor(ctx, 0x0136, 0x18);
	write_cmos_sensor(ctx, 0x0137, 0x00);

	write_cmos_sensor(ctx, 0x3051, 0x00);
	write_cmos_sensor(ctx, 0x6B11, 0xCF);
	write_cmos_sensor(ctx, 0x7FF0, 0x08);
	write_cmos_sensor(ctx, 0x7FF1, 0x0F);
	write_cmos_sensor(ctx, 0x7FF2, 0x08);
	write_cmos_sensor(ctx, 0x7FF3, 0x1B);
	write_cmos_sensor(ctx, 0x7FF4, 0x23);
	write_cmos_sensor(ctx, 0x7FF5, 0x60);
	write_cmos_sensor(ctx, 0x7FF6, 0x00);
	write_cmos_sensor(ctx, 0x7FF7, 0x01);
	write_cmos_sensor(ctx, 0x7FF8, 0x00);
	write_cmos_sensor(ctx, 0x7FF9, 0x78);
	write_cmos_sensor(ctx, 0x7FFA, 0x01);
	write_cmos_sensor(ctx, 0x7FFB, 0x00);
	write_cmos_sensor(ctx, 0x7FFC, 0x00);
	write_cmos_sensor(ctx, 0x7FFD, 0x00);
	write_cmos_sensor(ctx, 0x7FFE, 0x00);
	write_cmos_sensor(ctx, 0x7FFF, 0x03);
	write_cmos_sensor(ctx, 0x7F76, 0x03);
	write_cmos_sensor(ctx, 0x7F77, 0xFE);
	write_cmos_sensor(ctx, 0x7FA8, 0x03);
	write_cmos_sensor(ctx, 0x7FA9, 0xFE);
	write_cmos_sensor(ctx, 0x7B24, 0x81);
	write_cmos_sensor(ctx, 0x7B25, 0x01);
	write_cmos_sensor(ctx, 0x6564, 0x07);
	write_cmos_sensor(ctx, 0x6B0D, 0x41);
	write_cmos_sensor(ctx, 0x653D, 0x04);
	write_cmos_sensor(ctx, 0x6B05, 0x8C);
	write_cmos_sensor(ctx, 0x6B06, 0xF9);
	write_cmos_sensor(ctx, 0x6B08, 0x65);
	write_cmos_sensor(ctx, 0x6B09, 0xFC);
	write_cmos_sensor(ctx, 0x6B0A, 0xCF);
	write_cmos_sensor(ctx, 0x6B0B, 0xD2);
	write_cmos_sensor(ctx, 0x6700, 0x0E);
	write_cmos_sensor(ctx, 0x6707, 0x0E);
	write_cmos_sensor(ctx, 0x9104, 0x00);
	write_cmos_sensor(ctx, 0x7421, 0x1C);
	write_cmos_sensor(ctx, 0x7423, 0xD7);
	write_cmos_sensor(ctx, 0x5F04, 0x00);
	write_cmos_sensor(ctx, 0x5F05, 0xED);

	imx258_ImageQuality_Setting(ctx);
	/*Need Mirror/Flip */
	set_mirror_flip(ctx, 0);

	write_imx258_SPC_Data(ctx);
	write_cmos_sensor(ctx, 0x7BC8, 0x01);
	write_cmos_sensor(ctx, 0x7BC9, 0x01);
	write_cmos_sensor(ctx, 0x0B05, 0x01);	/* BPC */
	write_cmos_sensor(ctx, 0x0B06, 0x00);	/* turn off dynamic bpc */
	write_cmos_sensor(ctx, 0x0100, 0x00);
}				/*      sensor_init  */

static void preview_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("preview E\n");

	write_cmos_sensor(ctx, 0x0112, 0x0A);
	write_cmos_sensor(ctx, 0x0113, 0x0A);
	write_cmos_sensor(ctx, 0x0114, 0x03);

	write_cmos_sensor(ctx, 0x0301, 0x05);
	write_cmos_sensor(ctx, 0x0303, 0x02);
	write_cmos_sensor(ctx, 0x0305, 0x04);
	write_cmos_sensor(ctx, 0x0306, 0x00);
	write_cmos_sensor(ctx, 0x0307, 0x6C);
	write_cmos_sensor(ctx, 0x0309, 0x0A);
	write_cmos_sensor(ctx, 0x030B, 0x01);
	write_cmos_sensor(ctx, 0x030D, 0x02);
	write_cmos_sensor(ctx, 0x030E, 0x00);
	write_cmos_sensor(ctx, 0x030F, 0xD8);
	write_cmos_sensor(ctx, 0x0310, 0x00);
	write_cmos_sensor(ctx, 0x0820, 0x0A);
	write_cmos_sensor(ctx, 0x0821, 0x20);
	write_cmos_sensor(ctx, 0x0822, 0x00);
	write_cmos_sensor(ctx, 0x0823, 0x00);

	write_cmos_sensor(ctx, 0x0342, 0x14);
	write_cmos_sensor(ctx, 0x0343, 0xE8);

	write_cmos_sensor(ctx, 0x0340, 0x06);
	write_cmos_sensor(ctx, 0x0341, 0x4E);

	write_cmos_sensor(ctx, 0x0344, 0x00);
	write_cmos_sensor(ctx, 0x0345, 0x00);
	write_cmos_sensor(ctx, 0x0346, 0x00);
	write_cmos_sensor(ctx, 0x0347, 0x00);
	write_cmos_sensor(ctx, 0x0348, 0x10);
	write_cmos_sensor(ctx, 0x0349, 0x6F);
	write_cmos_sensor(ctx, 0x034A, 0x0C);
	write_cmos_sensor(ctx, 0x034B, 0x2F);

	write_cmos_sensor(ctx, 0x0381, 0x01);
	write_cmos_sensor(ctx, 0x0383, 0x01);
	write_cmos_sensor(ctx, 0x0385, 0x01);
	write_cmos_sensor(ctx, 0x0387, 0x01);
	write_cmos_sensor(ctx, 0x0900, 0x01);
	write_cmos_sensor(ctx, 0x0901, 0x12);
	write_cmos_sensor(ctx, 0x0902, 0x02);

	write_cmos_sensor(ctx, 0x0401, 0x01);
	write_cmos_sensor(ctx, 0x0404, 0x00);
	write_cmos_sensor(ctx, 0x0405, 0x20);
	write_cmos_sensor(ctx, 0x0408, 0x00);
	write_cmos_sensor(ctx, 0x0409, 0x00);
	write_cmos_sensor(ctx, 0x040A, 0x00);
	write_cmos_sensor(ctx, 0x040B, 0x00);
	write_cmos_sensor(ctx, 0x040C, 0x10);
	write_cmos_sensor(ctx, 0x040D, 0x70);
	write_cmos_sensor(ctx, 0x040E, 0x06);
	write_cmos_sensor(ctx, 0x040F, 0x18);
	write_cmos_sensor(ctx, 0x3038, 0x00);
	write_cmos_sensor(ctx, 0x303A, 0x00);
	write_cmos_sensor(ctx, 0x303B, 0x10);
	write_cmos_sensor(ctx, 0x300D, 0x00);

	write_cmos_sensor(ctx, 0x034C, 0x08);
	write_cmos_sensor(ctx, 0x034D, 0x34);
	write_cmos_sensor(ctx, 0x034E, 0x06);
	write_cmos_sensor(ctx, 0x034F, 0x18);

	write_cmos_sensor(ctx, 0x0202, 0x06);
	write_cmos_sensor(ctx, 0x0203, 0x44);

	write_cmos_sensor(ctx, 0x0204, 0x00);
	write_cmos_sensor(ctx, 0x0205, 0x00);
	write_cmos_sensor(ctx, 0x020E, 0x01);
	write_cmos_sensor(ctx, 0x020F, 0x00);

	if (imx258_type == IMX258_HDR_TYPE || imx258_type == IMX258_RAW_TYPE) {
		write_cmos_sensor(ctx, 0x94DC, 0x20);
		write_cmos_sensor(ctx, 0x94DD, 0x20);
		write_cmos_sensor(ctx, 0x94DE, 0x20);
		write_cmos_sensor(ctx, 0x95DC, 0x20);
		write_cmos_sensor(ctx, 0x95DD, 0x20);
		write_cmos_sensor(ctx, 0x95DE, 0x20);
		write_cmos_sensor(ctx, 0x7FB0, 0x00);
		write_cmos_sensor(ctx, 0x9010, 0x3E);
	} else if (imx258_type == IMX258_BINNING_TYPE) {
		write_cmos_sensor(ctx, 0x7BCD, 0x01);
	}

	write_cmos_sensor(ctx, 0x3030, 0x00);
	LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));

	if (test_Pmode) {
		write_cmos_sensor(ctx, 0x3030, 0x00);

		/* default 0, need not to set */
		write_cmos_sensor(ctx, 0x0B00, 0x00);

		write_cmos_sensor(ctx, 0x3051, 0x00);	/* need not to set */
		write_cmos_sensor(ctx, 0x3052, 0x00);
		write_cmos_sensor(ctx, 0x7BCA, 0x01);
		write_cmos_sensor(ctx, 0x7BCB, 0x00);

		/* set in sensor_init(ctx), need not to set */
		write_cmos_sensor(ctx, 0x7BC8, 0x01);

		write_cmos_sensor(ctx, 0x7BC9, 0x00);
	}
	write_cmos_sensor(ctx, 0x3032, 0x00);
	LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
	write_cmos_sensor(ctx, 0x0220, 0x00);
	write_cmos_sensor(ctx, 0x0350, 0x01);	/* Enable auto extend */


}

static void capture_setting(struct subdrv_ctx *ctx, kal_uint16 curretfps, kal_uint8 pdaf_mode)
{
	LOG_INF("capture E\n");
	LOG_INF("E! currefps:%d\n", curretfps);
	if (curretfps == 150) {
		LOG_INF("PIP15fps capture E\n");

		write_cmos_sensor(ctx, 0x0112, 0x0A);
		write_cmos_sensor(ctx, 0x0113, 0x0A);
		write_cmos_sensor(ctx, 0x0114, 0x03);

		write_cmos_sensor(ctx, 0x0301, 0x05);
		write_cmos_sensor(ctx, 0x0303, 0x02);
		write_cmos_sensor(ctx, 0x0305, 0x04);
		write_cmos_sensor(ctx, 0x0306, 0x00);
		write_cmos_sensor(ctx, 0x0307, 0x6C);
		write_cmos_sensor(ctx, 0x0309, 0x0A);
		write_cmos_sensor(ctx, 0x030B, 0x01);
		write_cmos_sensor(ctx, 0x030D, 0x02);
		write_cmos_sensor(ctx, 0x030E, 0x00);
		write_cmos_sensor(ctx, 0x030F, 0xD8);
		write_cmos_sensor(ctx, 0x0310, 0x00);
		write_cmos_sensor(ctx, 0x0820, 0x0A);
		write_cmos_sensor(ctx, 0x0821, 0x20);
		write_cmos_sensor(ctx, 0x0822, 0x00);
		write_cmos_sensor(ctx, 0x0823, 0x00);

		write_cmos_sensor(ctx, 0x0342, 0x14);
		write_cmos_sensor(ctx, 0x0343, 0xE8);

		write_cmos_sensor(ctx, 0x0340, 0x0C);
		write_cmos_sensor(ctx, 0x0341, 0x9C);

		write_cmos_sensor(ctx, 0x0344, 0x00);
		write_cmos_sensor(ctx, 0x0345, 0x00);
		write_cmos_sensor(ctx, 0x0346, 0x00);
		write_cmos_sensor(ctx, 0x0347, 0x00);
		write_cmos_sensor(ctx, 0x0348, 0x10);
		write_cmos_sensor(ctx, 0x0349, 0x6F);
		write_cmos_sensor(ctx, 0x034A, 0x0C);
		write_cmos_sensor(ctx, 0x034B, 0x2F);

		write_cmos_sensor(ctx, 0x0381, 0x01);
		write_cmos_sensor(ctx, 0x0383, 0x01);
		write_cmos_sensor(ctx, 0x0385, 0x01);
		write_cmos_sensor(ctx, 0x0387, 0x01);
		write_cmos_sensor(ctx, 0x0900, 0x00);
		write_cmos_sensor(ctx, 0x0901, 0x11);
		if (imx258_type != IMX258_HDD_TYPE)
			write_cmos_sensor(ctx, 0x0902, 0x02);

		write_cmos_sensor(ctx, 0x0401, 0x00);
		write_cmos_sensor(ctx, 0x0404, 0x00);
		write_cmos_sensor(ctx, 0x0405, 0x10);
		write_cmos_sensor(ctx, 0x0408, 0x00);
		write_cmos_sensor(ctx, 0x0409, 0x00);
		write_cmos_sensor(ctx, 0x040A, 0x00);
		write_cmos_sensor(ctx, 0x040B, 0x00);
		write_cmos_sensor(ctx, 0x040C, 0x10);
		write_cmos_sensor(ctx, 0x040D, 0x70);
		write_cmos_sensor(ctx, 0x040E, 0x0C);
		write_cmos_sensor(ctx, 0x040F, 0x30);
		write_cmos_sensor(ctx, 0x3038, 0x00);
		write_cmos_sensor(ctx, 0x303A, 0x00);
		write_cmos_sensor(ctx, 0x303B, 0x10);
		write_cmos_sensor(ctx, 0x300D, 0x00);

		write_cmos_sensor(ctx, 0x034C, 0x10);
		write_cmos_sensor(ctx, 0x034D, 0x70);
		write_cmos_sensor(ctx, 0x034E, 0x0C);
		write_cmos_sensor(ctx, 0x034F, 0x30);

		write_cmos_sensor(ctx, 0x0202, 0x0C);
		write_cmos_sensor(ctx, 0x0203, 0x92);

		write_cmos_sensor(ctx, 0x0204, 0x00);
		write_cmos_sensor(ctx, 0x0205, 0x00);
		write_cmos_sensor(ctx, 0x020E, 0x01);
		write_cmos_sensor(ctx, 0x020F, 0x00);

		write_cmos_sensor(ctx, 0x7BCD, 0x00);
	if (imx258_type == IMX258_HDR_TYPE || imx258_type == IMX258_RAW_TYPE) {
		write_cmos_sensor(ctx, 0x94DC, 0x20);
		write_cmos_sensor(ctx, 0x94DD, 0x20);
		write_cmos_sensor(ctx, 0x94DE, 0x20);
		write_cmos_sensor(ctx, 0x95DC, 0x20);
		write_cmos_sensor(ctx, 0x95DD, 0x20);
		write_cmos_sensor(ctx, 0x95DE, 0x20);
		write_cmos_sensor(ctx, 0x7FB0, 0x00);
		write_cmos_sensor(ctx, 0x9010, 0x3E);
	} else if (imx258_type == IMX258_BINNING_TYPE) {
		write_cmos_sensor(ctx, 0x7BCD, 0x00);
	} else if (imx258_type == IMX258_HDD_TYPE) {
		write_cmos_sensor(ctx, 0x94DC, 0x20);
		write_cmos_sensor(ctx, 0x94DD, 0x20);
		write_cmos_sensor(ctx, 0x94DE, 0x20);
		write_cmos_sensor(ctx, 0x95DC, 0x20);
		write_cmos_sensor(ctx, 0x95DD, 0x20);
		write_cmos_sensor(ctx, 0x95DE, 0x20);
		write_cmos_sensor(ctx, 0x7FB0, 0x00);
		write_cmos_sensor(ctx, 0x9010, 0x3E);
		write_cmos_sensor(ctx, 0x9419, 0x50);
		write_cmos_sensor(ctx, 0x941B, 0x50);
		write_cmos_sensor(ctx, 0x9519, 0x50);
		write_cmos_sensor(ctx, 0x951B, 0x50);
		write_cmos_sensor(ctx, 0x7FB4, 0x02);
		write_cmos_sensor(ctx, 0x94BE, 0x3F);
		write_cmos_sensor(ctx, 0x94BF, 0x3F);
		write_cmos_sensor(ctx, 0x94C0, 0x3F);
		write_cmos_sensor(ctx, 0x94C4, 0x3F);
		write_cmos_sensor(ctx, 0x94C5, 0x3F);
		write_cmos_sensor(ctx, 0x94C6, 0x3F);
		write_cmos_sensor(ctx, 0x94CA, 0x40);
		write_cmos_sensor(ctx, 0x94CB, 0x40);
		write_cmos_sensor(ctx, 0x94CC, 0x40);
		write_cmos_sensor(ctx, 0x94CD, 0x30);
		write_cmos_sensor(ctx, 0x94CE, 0x30);
		write_cmos_sensor(ctx, 0x94CF, 0x30);
	}

		if (pdaf_mode == 1) {
			LOG_INF("read 0x3030\n");
			write_cmos_sensor(ctx, 0x3030, 0x01);  /* pdaf enable */
			LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
			write_cmos_sensor(ctx, 0x3032, 0x01);  /* 0:raw10, 1:BYTE2 */
			LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
		} else {
			write_cmos_sensor(ctx, 0x3030, 0x00);
			LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
			write_cmos_sensor(ctx, 0x3032, 0x00);
			LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
		}
		if (test_Pmode) {
			write_cmos_sensor(ctx, 0x3030, 0x00);
			write_cmos_sensor(ctx, 0x3052, 0x00);
			write_cmos_sensor(ctx, 0x7BCA, 0x01);
			write_cmos_sensor(ctx, 0x7BCB, 0x00);
			write_cmos_sensor(ctx, 0x7BC9, 0x00);
		}
		write_cmos_sensor(ctx, 0x0220, 0x00);
		write_cmos_sensor(ctx, 0x0350, 0x01);	/* Enable auto extend */

	} else if (curretfps == 240) {
		LOG_INF("PIP24fps capture E\n");

		write_cmos_sensor(ctx, 0x0112, 0x0A);
		write_cmos_sensor(ctx, 0x0113, 0x0A);
		write_cmos_sensor(ctx, 0x0114, 0x03);

		write_cmos_sensor(ctx, 0x0301, 0x05);
		write_cmos_sensor(ctx, 0x0303, 0x02);
		write_cmos_sensor(ctx, 0x0305, 0x04);
		write_cmos_sensor(ctx, 0x0306, 0x00);
		write_cmos_sensor(ctx, 0x0307, 0xAD);
		write_cmos_sensor(ctx, 0x0309, 0x0A);
		write_cmos_sensor(ctx, 0x030B, 0x01);
		write_cmos_sensor(ctx, 0x030D, 0x02);
		write_cmos_sensor(ctx, 0x030E, 0x00);
		write_cmos_sensor(ctx, 0x030F, 0xD8);
		write_cmos_sensor(ctx, 0x0310, 0x00);
		write_cmos_sensor(ctx, 0x0820, 0x10);
		write_cmos_sensor(ctx, 0x0821, 0x38);
		write_cmos_sensor(ctx, 0x0822, 0x00);
		write_cmos_sensor(ctx, 0x0823, 0x00);

		write_cmos_sensor(ctx, 0x0342, 0x14);
		write_cmos_sensor(ctx, 0x0343, 0xE8);

		write_cmos_sensor(ctx, 0x0340, 0x0C);
		write_cmos_sensor(ctx, 0x0341, 0x9C);

		write_cmos_sensor(ctx, 0x0344, 0x00);
		write_cmos_sensor(ctx, 0x0345, 0x00);
		write_cmos_sensor(ctx, 0x0346, 0x00);
		write_cmos_sensor(ctx, 0x0347, 0x00);
		write_cmos_sensor(ctx, 0x0348, 0x10);
		write_cmos_sensor(ctx, 0x0349, 0x6F);
		write_cmos_sensor(ctx, 0x034A, 0x0C);
		write_cmos_sensor(ctx, 0x034B, 0x2F);

		write_cmos_sensor(ctx, 0x0381, 0x01);
		write_cmos_sensor(ctx, 0x0383, 0x01);
		write_cmos_sensor(ctx, 0x0385, 0x01);
		write_cmos_sensor(ctx, 0x0387, 0x01);
		write_cmos_sensor(ctx, 0x0900, 0x00);
		write_cmos_sensor(ctx, 0x0901, 0x11);
		if (imx258_type != IMX258_HDD_TYPE)
			write_cmos_sensor(ctx, 0x0902, 0x02);


		write_cmos_sensor(ctx, 0x0401, 0x00);
		write_cmos_sensor(ctx, 0x0404, 0x00);
		write_cmos_sensor(ctx, 0x0405, 0x10);
		write_cmos_sensor(ctx, 0x0408, 0x00);
		write_cmos_sensor(ctx, 0x0409, 0x00);
		write_cmos_sensor(ctx, 0x040A, 0x00);
		write_cmos_sensor(ctx, 0x040B, 0x00);
		write_cmos_sensor(ctx, 0x040C, 0x10);
		write_cmos_sensor(ctx, 0x040D, 0x70);
		write_cmos_sensor(ctx, 0x040E, 0x0C);
		write_cmos_sensor(ctx, 0x040F, 0x30);
		write_cmos_sensor(ctx, 0x3038, 0x00);
		write_cmos_sensor(ctx, 0x303A, 0x00);
		write_cmos_sensor(ctx, 0x303B, 0x10);
		write_cmos_sensor(ctx, 0x300D, 0x00);

		write_cmos_sensor(ctx, 0x034C, 0x10);
		write_cmos_sensor(ctx, 0x034D, 0x70);
		write_cmos_sensor(ctx, 0x034E, 0x0C);
		write_cmos_sensor(ctx, 0x034F, 0x30);

		write_cmos_sensor(ctx, 0x0202, 0x0C);
		write_cmos_sensor(ctx, 0x0203, 0x92);

		write_cmos_sensor(ctx, 0x0204, 0x00);
		write_cmos_sensor(ctx, 0x0205, 0x00);
		write_cmos_sensor(ctx, 0x020E, 0x01);
		write_cmos_sensor(ctx, 0x020F, 0x00);

		write_cmos_sensor(ctx, 0x7BCD, 0x00);
	if (imx258_type == IMX258_HDR_TYPE || imx258_type == IMX258_RAW_TYPE) {
		write_cmos_sensor(ctx, 0x94DC, 0x20);
		write_cmos_sensor(ctx, 0x94DD, 0x20);
		write_cmos_sensor(ctx, 0x94DE, 0x20);
		write_cmos_sensor(ctx, 0x95DC, 0x20);
		write_cmos_sensor(ctx, 0x95DD, 0x20);
		write_cmos_sensor(ctx, 0x95DE, 0x20);
		write_cmos_sensor(ctx, 0x7FB0, 0x00);
		write_cmos_sensor(ctx, 0x9010, 0x3E);
	} else if (imx258_type == IMX258_BINNING_TYPE) {
		write_cmos_sensor(ctx, 0x7BCD, 0x00);
	} else if (imx258_type == IMX258_HDD_TYPE) {
		write_cmos_sensor(ctx, 0x94DC, 0x20);
		write_cmos_sensor(ctx, 0x94DD, 0x20);
		write_cmos_sensor(ctx, 0x94DE, 0x20);
		write_cmos_sensor(ctx, 0x95DC, 0x20);
		write_cmos_sensor(ctx, 0x95DD, 0x20);
		write_cmos_sensor(ctx, 0x95DE, 0x20);
		write_cmos_sensor(ctx, 0x7FB0, 0x00);
		write_cmos_sensor(ctx, 0x9010, 0x3E);
		write_cmos_sensor(ctx, 0x9419, 0x50);
		write_cmos_sensor(ctx, 0x941B, 0x50);
		write_cmos_sensor(ctx, 0x9519, 0x50);
		write_cmos_sensor(ctx, 0x951B, 0x50);
		write_cmos_sensor(ctx, 0x7FB4, 0x02);
		write_cmos_sensor(ctx, 0x94BE, 0x3F);
		write_cmos_sensor(ctx, 0x94BF, 0x3F);
		write_cmos_sensor(ctx, 0x94C0, 0x3F);
		write_cmos_sensor(ctx, 0x94C4, 0x3F);
		write_cmos_sensor(ctx, 0x94C5, 0x3F);
		write_cmos_sensor(ctx, 0x94C6, 0x3F);
		write_cmos_sensor(ctx, 0x94CA, 0x40);
		write_cmos_sensor(ctx, 0x94CB, 0x40);
		write_cmos_sensor(ctx, 0x94CC, 0x40);
		write_cmos_sensor(ctx, 0x94CD, 0x30);
		write_cmos_sensor(ctx, 0x94CE, 0x30);
		write_cmos_sensor(ctx, 0x94CF, 0x30);
	}

		if (pdaf_mode == 1) {
			LOG_INF("read 0x3030\n");
			write_cmos_sensor(ctx, 0x3030, 0x01);  /* pdaf enable */
			LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
			write_cmos_sensor(ctx, 0x3032, 0x01);  /* 0:raw10, 1:BYTE2 */
			LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
		} else {
			write_cmos_sensor(ctx, 0x3030, 0x00);
			LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
			write_cmos_sensor(ctx, 0x3032, 0x00);
			LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
		}
		if (test_Pmode) {
			write_cmos_sensor(ctx, 0x3030, 0x00);
			write_cmos_sensor(ctx, 0x3052, 0x00);
			write_cmos_sensor(ctx, 0x7BCA, 0x01);
			write_cmos_sensor(ctx, 0x7BCB, 0x00);
			write_cmos_sensor(ctx, 0x7BC9, 0x00);
		}

		write_cmos_sensor(ctx, 0x0220, 0x00);
		write_cmos_sensor(ctx, 0x0350, 0x01);	/* Enable auto extend */


	} else {


		write_cmos_sensor(ctx, 0x0112, 0x0A);
		write_cmos_sensor(ctx, 0x0113, 0x0A);
		write_cmos_sensor(ctx, 0x0114, 0x03);

		write_cmos_sensor(ctx, 0x0301, 0x05);
		write_cmos_sensor(ctx, 0x0303, 0x02);
		write_cmos_sensor(ctx, 0x0305, 0x04);
		write_cmos_sensor(ctx, 0x0306, 0x00);
		write_cmos_sensor(ctx, 0x0307, 0xD8);
		write_cmos_sensor(ctx, 0x0309, 0x0A);
		write_cmos_sensor(ctx, 0x030B, 0x01);
		write_cmos_sensor(ctx, 0x030D, 0x02);
		write_cmos_sensor(ctx, 0x030E, 0x00);
		write_cmos_sensor(ctx, 0x030F, 0xD8);
		write_cmos_sensor(ctx, 0x0310, 0x00);
		write_cmos_sensor(ctx, 0x0820, 0x14);
		write_cmos_sensor(ctx, 0x0821, 0x40);
		write_cmos_sensor(ctx, 0x0822, 0x00);
		write_cmos_sensor(ctx, 0x0823, 0x00);

		write_cmos_sensor(ctx, 0x0342, 0x14);
		write_cmos_sensor(ctx, 0x0343, 0xE8);

		write_cmos_sensor(ctx, 0x0340, 0x0C);
		write_cmos_sensor(ctx, 0x0341, 0x9C);

		write_cmos_sensor(ctx, 0x0344, 0x00);
		write_cmos_sensor(ctx, 0x0345, 0x00);
		write_cmos_sensor(ctx, 0x0346, 0x00);
		write_cmos_sensor(ctx, 0x0347, 0x00);
		write_cmos_sensor(ctx, 0x0348, 0x10);
		write_cmos_sensor(ctx, 0x0349, 0x6F);
		write_cmos_sensor(ctx, 0x034A, 0x0C);
		write_cmos_sensor(ctx, 0x034B, 0x2F);

		write_cmos_sensor(ctx, 0x0381, 0x01);
		write_cmos_sensor(ctx, 0x0383, 0x01);
		write_cmos_sensor(ctx, 0x0385, 0x01);
		write_cmos_sensor(ctx, 0x0387, 0x01);
		write_cmos_sensor(ctx, 0x0900, 0x00);
		write_cmos_sensor(ctx, 0x0901, 0x11);
		if (imx258_type != IMX258_HDD_TYPE)
			write_cmos_sensor(ctx, 0x0902, 0x02);


		write_cmos_sensor(ctx, 0x0401, 0x00);
		write_cmos_sensor(ctx, 0x0404, 0x00);
		write_cmos_sensor(ctx, 0x0405, 0x10);
		write_cmos_sensor(ctx, 0x0408, 0x00);
		write_cmos_sensor(ctx, 0x0409, 0x00);
		write_cmos_sensor(ctx, 0x040A, 0x00);
		write_cmos_sensor(ctx, 0x040B, 0x00);
		write_cmos_sensor(ctx, 0x040C, 0x10);
		write_cmos_sensor(ctx, 0x040D, 0x70);
		write_cmos_sensor(ctx, 0x040E, 0x0C);
		write_cmos_sensor(ctx, 0x040F, 0x30);
		write_cmos_sensor(ctx, 0x3038, 0x00);
		write_cmos_sensor(ctx, 0x303A, 0x00);
		write_cmos_sensor(ctx, 0x303B, 0x10);
		write_cmos_sensor(ctx, 0x300D, 0x00);

		write_cmos_sensor(ctx, 0x034C, 0x10);
		write_cmos_sensor(ctx, 0x034D, 0x70);
		write_cmos_sensor(ctx, 0x034E, 0x0C);
		write_cmos_sensor(ctx, 0x034F, 0x30);

		write_cmos_sensor(ctx, 0x0202, 0x0C);
		write_cmos_sensor(ctx, 0x0203, 0x92);

		write_cmos_sensor(ctx, 0x0204, 0x00);
		write_cmos_sensor(ctx, 0x0205, 0x00);
		write_cmos_sensor(ctx, 0x020E, 0x01);
		write_cmos_sensor(ctx, 0x020F, 0x00);

		write_cmos_sensor(ctx, 0x7BCD, 0x00);
		if (imx258_type == IMX258_HDR_TYPE ||
			imx258_type == IMX258_RAW_TYPE) {
			write_cmos_sensor(ctx, 0x94DC, 0x20);
			write_cmos_sensor(ctx, 0x94DD, 0x20);
			write_cmos_sensor(ctx, 0x94DE, 0x20);
			write_cmos_sensor(ctx, 0x95DC, 0x20);
			write_cmos_sensor(ctx, 0x95DD, 0x20);
			write_cmos_sensor(ctx, 0x95DE, 0x20);
			write_cmos_sensor(ctx, 0x7FB0, 0x00);
			write_cmos_sensor(ctx, 0x9010, 0x3E);
		} else if (imx258_type == IMX258_BINNING_TYPE) {
			write_cmos_sensor(ctx, 0x7BCD, 0x00);
		} else if (imx258_type == IMX258_HDD_TYPE) {

			write_cmos_sensor(ctx, 0x94DC, 0x20);
			write_cmos_sensor(ctx, 0x94DD, 0x20);
			write_cmos_sensor(ctx, 0x94DE, 0x20);
			write_cmos_sensor(ctx, 0x95DC, 0x20);
			write_cmos_sensor(ctx, 0x95DD, 0x20);
			write_cmos_sensor(ctx, 0x95DE, 0x20);
			write_cmos_sensor(ctx, 0x7FB0, 0x00);
			write_cmos_sensor(ctx, 0x9010, 0x3E);
			write_cmos_sensor(ctx, 0x9419, 0x50);
			write_cmos_sensor(ctx, 0x941B, 0x50);
			write_cmos_sensor(ctx, 0x9519, 0x50);
			write_cmos_sensor(ctx, 0x951B, 0x50);
			write_cmos_sensor(ctx, 0x7FB4, 0x02);
			write_cmos_sensor(ctx, 0x94BE, 0x3F);
			write_cmos_sensor(ctx, 0x94BF, 0x3F);
			write_cmos_sensor(ctx, 0x94C0, 0x3F);
			write_cmos_sensor(ctx, 0x94C4, 0x3F);
			write_cmos_sensor(ctx, 0x94C5, 0x3F);
			write_cmos_sensor(ctx, 0x94C6, 0x3F);
			write_cmos_sensor(ctx, 0x94CA, 0x40);
			write_cmos_sensor(ctx, 0x94CB, 0x40);
			write_cmos_sensor(ctx, 0x94CC, 0x40);
			write_cmos_sensor(ctx, 0x94CD, 0x30);
			write_cmos_sensor(ctx, 0x94CE, 0x30);
			write_cmos_sensor(ctx, 0x94CF, 0x30);
		}

		if (pdaf_mode == 1) {
			LOG_INF("read 0x3030\n");
			write_cmos_sensor(ctx, 0x3030, 0x01);  /* pdaf enable */
			LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
			write_cmos_sensor(ctx, 0x3032, 0x01);  /* 0:raw10, 1:BYTE2 */
			LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
		} else {
			write_cmos_sensor(ctx, 0x3030, 0x00);
			LOG_INF("0x3030=%d", read_cmos_sensor(ctx, 0x3030));
			write_cmos_sensor(ctx, 0x3032, 0x00);
			LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
		}
		if (test_Pmode) {
			write_cmos_sensor(ctx, 0x3030, 0x00);
			write_cmos_sensor(ctx, 0x3052, 0x00);
			write_cmos_sensor(ctx, 0x7BCA, 0x01);
			write_cmos_sensor(ctx, 0x7BCB, 0x00);
			write_cmos_sensor(ctx, 0x7BC9, 0x00);
		}

		write_cmos_sensor(ctx, 0x0220, 0x00);
		write_cmos_sensor(ctx, 0x0350, 0x01);  /* Enable auto extend */

	}
}

static void normal_video_setting(struct subdrv_ctx *ctx, kal_uint16 currefps, kal_uint8 pdaf_mode)
{
	LOG_INF("normal video E\n");
	LOG_INF("E! currefps:%d\n", currefps);


	write_cmos_sensor(ctx, 0x0112, 0x0A);
	write_cmos_sensor(ctx, 0x0113, 0x0A);
	write_cmos_sensor(ctx, 0x0114, 0x03);

	write_cmos_sensor(ctx, 0x0301, 0x05);
	write_cmos_sensor(ctx, 0x0303, 0x02);
	write_cmos_sensor(ctx, 0x0305, 0x04);
	write_cmos_sensor(ctx, 0x0306, 0x00);
	write_cmos_sensor(ctx, 0x0307, 0xD8);
	write_cmos_sensor(ctx, 0x0309, 0x0A);
	write_cmos_sensor(ctx, 0x030B, 0x01);
	write_cmos_sensor(ctx, 0x030D, 0x02);
	write_cmos_sensor(ctx, 0x030E, 0x00);
	write_cmos_sensor(ctx, 0x030F, 0xD8);
	write_cmos_sensor(ctx, 0x0310, 0x00);
	write_cmos_sensor(ctx, 0x0820, 0x14);
	write_cmos_sensor(ctx, 0x0821, 0x40);
	write_cmos_sensor(ctx, 0x0822, 0x00);
	write_cmos_sensor(ctx, 0x0823, 0x00);

	write_cmos_sensor(ctx, 0x0342, 0x14);
	write_cmos_sensor(ctx, 0x0343, 0xE8);

	write_cmos_sensor(ctx, 0x0340, 0x0C);
	if (imx258_type == IMX258_HDR_TYPE || imx258_type == IMX258_RAW_TYPE)
		write_cmos_sensor(ctx, 0x0341, 0x9C);	/*  */
	else if (imx258_type == IMX258_BINNING_TYPE)
		write_cmos_sensor(ctx, 0x0341, 0x98);

	write_cmos_sensor(ctx, 0x0344, 0x00);
	write_cmos_sensor(ctx, 0x0345, 0x00);
	write_cmos_sensor(ctx, 0x0346, 0x00);
	write_cmos_sensor(ctx, 0x0347, 0x00);
	write_cmos_sensor(ctx, 0x0348, 0x10);
	write_cmos_sensor(ctx, 0x0349, 0x6F);
	write_cmos_sensor(ctx, 0x034A, 0x0C);
	write_cmos_sensor(ctx, 0x034B, 0x2F);

	write_cmos_sensor(ctx, 0x0381, 0x01);
	write_cmos_sensor(ctx, 0x0383, 0x01);
	write_cmos_sensor(ctx, 0x0385, 0x01);
	write_cmos_sensor(ctx, 0x0387, 0x01);
	write_cmos_sensor(ctx, 0x0900, 0x00);
	write_cmos_sensor(ctx, 0x0901, 0x11);
	write_cmos_sensor(ctx, 0x0902, 0x00);

	write_cmos_sensor(ctx, 0x0401, 0x00);
	write_cmos_sensor(ctx, 0x0404, 0x00);
	write_cmos_sensor(ctx, 0x0405, 0x10);
	write_cmos_sensor(ctx, 0x0408, 0x00);
	write_cmos_sensor(ctx, 0x0409, 0x00);
	write_cmos_sensor(ctx, 0x040A, 0x00);
	write_cmos_sensor(ctx, 0x040B, 0x00);
	write_cmos_sensor(ctx, 0x040C, 0x10);
	write_cmos_sensor(ctx, 0x040D, 0x70);
	write_cmos_sensor(ctx, 0x040E, 0x0C);
	write_cmos_sensor(ctx, 0x040F, 0x30);
	write_cmos_sensor(ctx, 0x3038, 0x00);
	write_cmos_sensor(ctx, 0x303A, 0x00);
	write_cmos_sensor(ctx, 0x303B, 0x10);
	write_cmos_sensor(ctx, 0x300D, 0x00);

	write_cmos_sensor(ctx, 0x034C, 0x10);
	write_cmos_sensor(ctx, 0x034D, 0x70);
	write_cmos_sensor(ctx, 0x034E, 0x0C);
	write_cmos_sensor(ctx, 0x034F, 0x30);

	write_cmos_sensor(ctx, 0x0202, 0x0C);
	if (imx258_type == IMX258_HDR_TYPE || imx258_type == IMX258_RAW_TYPE)
		write_cmos_sensor(ctx, 0x0203, 0x92);	/*  */
	else if (imx258_type == IMX258_BINNING_TYPE)
		write_cmos_sensor(ctx, 0x0203, 0x8E);

	write_cmos_sensor(ctx, 0x0204, 0x00);
	write_cmos_sensor(ctx, 0x0205, 0x00);
	write_cmos_sensor(ctx, 0x020E, 0x01);
	write_cmos_sensor(ctx, 0x020F, 0x00);

if (ctx->hdr_mode == 1) {
	if (imx258_type == IMX258_HDR_TYPE || imx258_type == IMX258_RAW_TYPE) {
		write_cmos_sensor(ctx, 0x94DC, 0x53);
		write_cmos_sensor(ctx, 0x94DD, 0x53);
		write_cmos_sensor(ctx, 0x94DE, 0x53);
		write_cmos_sensor(ctx, 0x95DC, 0x53);
		write_cmos_sensor(ctx, 0x95DD, 0x53);
		write_cmos_sensor(ctx, 0x95DE, 0x53);
		write_cmos_sensor(ctx, 0x7FB0, 0x01);
		write_cmos_sensor(ctx, 0x9010, 0x52);
	} else if (imx258_type == IMX258_BINNING_TYPE) {
		write_cmos_sensor(ctx, 0x7BCD, 0x00);
	}

		write_cmos_sensor(ctx, 0x3030, 0x00);
		LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
		write_cmos_sensor(ctx, 0x3032, 0x00);
		LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
		write_cmos_sensor(ctx, 0x0220, 0x21);	/* 0x03 */
		/* write_cmos_sensor(ctx, 0x0222,0x08); */
} else {
	if (imx258_type == IMX258_HDR_TYPE || imx258_type == IMX258_RAW_TYPE) {
		write_cmos_sensor(ctx, 0x94DC, 0x20);
		write_cmos_sensor(ctx, 0x94DD, 0x20);
		write_cmos_sensor(ctx, 0x94DE, 0x20);
		write_cmos_sensor(ctx, 0x95DC, 0x20);
		write_cmos_sensor(ctx, 0x95DD, 0x20);
		write_cmos_sensor(ctx, 0x95DE, 0x20);
		write_cmos_sensor(ctx, 0x7FB0, 0x00);
		write_cmos_sensor(ctx, 0x9010, 0x3E);
	} else if (imx258_type == IMX258_BINNING_TYPE) {
		write_cmos_sensor(ctx, 0x7BCD, 0x00);
	}

		if (pdaf_mode == 1) {
			write_cmos_sensor(ctx, 0x3030, 0x01);
			LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
			write_cmos_sensor(ctx, 0x3032, 0x01);
			LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
			write_cmos_sensor(ctx, 0x0220, 0x00);
		} else {
			write_cmos_sensor(ctx, 0x3030, 0x00);
			LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
			write_cmos_sensor(ctx, 0x3032, 0x00);
			LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
			write_cmos_sensor(ctx, 0x0220, 0x00);
		}
		if (test_Pmode) {
			write_cmos_sensor(ctx, 0x3030, 0x00);
			write_cmos_sensor(ctx, 0x3052, 0x00);
			write_cmos_sensor(ctx, 0x7BCA, 0x01);
			write_cmos_sensor(ctx, 0x7BCB, 0x00);
			write_cmos_sensor(ctx, 0x7BC9, 0x00);
		}
	}
	write_cmos_sensor(ctx, 0x0350, 0x01);	/* Enable auto extend */
	LOG_INF("ctx->hdr_mode in video mode:%d\n", ctx->hdr_mode);




}

static void hs_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("hs_video E\n");


	write_cmos_sensor(ctx, 0x0112, 0x0A);
	write_cmos_sensor(ctx, 0x0113, 0x0A);
	write_cmos_sensor(ctx, 0x0114, 0x03);

	write_cmos_sensor(ctx, 0x0301, 0x05);
	write_cmos_sensor(ctx, 0x0303, 0x02);
	write_cmos_sensor(ctx, 0x0305, 0x04);
	write_cmos_sensor(ctx, 0x0306, 0x00);
	write_cmos_sensor(ctx, 0x0307, 0xC8);
	write_cmos_sensor(ctx, 0x0309, 0x0A);
	write_cmos_sensor(ctx, 0x030B, 0x01);
	write_cmos_sensor(ctx, 0x030D, 0x02);
	write_cmos_sensor(ctx, 0x030E, 0x00);
	write_cmos_sensor(ctx, 0x030F, 0xD8);
	write_cmos_sensor(ctx, 0x0310, 0x00);
	write_cmos_sensor(ctx, 0x0820, 0x12);
	write_cmos_sensor(ctx, 0x0821, 0xC0);
	write_cmos_sensor(ctx, 0x0822, 0x00);
	write_cmos_sensor(ctx, 0x0823, 0x00);

	write_cmos_sensor(ctx, 0x0342, 0x14);
	write_cmos_sensor(ctx, 0x0343, 0xE8);

	write_cmos_sensor(ctx, 0x0340, 0x02);
	write_cmos_sensor(ctx, 0x0341, 0xEA);

	write_cmos_sensor(ctx, 0x0344, 0x00);
	write_cmos_sensor(ctx, 0x0345, 0x00);
	write_cmos_sensor(ctx, 0x0346, 0x02);
	write_cmos_sensor(ctx, 0x0347, 0x50);
	write_cmos_sensor(ctx, 0x0348, 0x10);
	write_cmos_sensor(ctx, 0x0349, 0x6F);
	write_cmos_sensor(ctx, 0x034A, 0x09);
	write_cmos_sensor(ctx, 0x034B, 0xCF);

	write_cmos_sensor(ctx, 0x0381, 0x01);
	write_cmos_sensor(ctx, 0x0383, 0x01);
	write_cmos_sensor(ctx, 0x0385, 0x03);
	write_cmos_sensor(ctx, 0x0387, 0x03);
	write_cmos_sensor(ctx, 0x0900, 0x01);
	write_cmos_sensor(ctx, 0x0901, 0x12);
	write_cmos_sensor(ctx, 0x0902, 0x02);

	write_cmos_sensor(ctx, 0x0401, 0x01);
	write_cmos_sensor(ctx, 0x0404, 0x00);
	write_cmos_sensor(ctx, 0x0405, 0x40);
	write_cmos_sensor(ctx, 0x0408, 0x00);
	write_cmos_sensor(ctx, 0x0409, 0x00);
	write_cmos_sensor(ctx, 0x040A, 0x00);
	write_cmos_sensor(ctx, 0x040B, 0x00);
	write_cmos_sensor(ctx, 0x040C, 0x10);
	write_cmos_sensor(ctx, 0x040D, 0x70);
	write_cmos_sensor(ctx, 0x040E, 0x02);
	write_cmos_sensor(ctx, 0x040F, 0x80);
	write_cmos_sensor(ctx, 0x3038, 0x00);
	write_cmos_sensor(ctx, 0x303A, 0x00);
	write_cmos_sensor(ctx, 0x303B, 0x10);
	write_cmos_sensor(ctx, 0x300D, 0x01);

	write_cmos_sensor(ctx, 0x034C, 0x04);
	write_cmos_sensor(ctx, 0x034D, 0x18);
	write_cmos_sensor(ctx, 0x034E, 0x01);
	write_cmos_sensor(ctx, 0x034F, 0xE0);

	write_cmos_sensor(ctx, 0x0202, 0x02);
	write_cmos_sensor(ctx, 0x0203, 0xE0);

	write_cmos_sensor(ctx, 0x0204, 0x00);
	write_cmos_sensor(ctx, 0x0205, 0x00);
	write_cmos_sensor(ctx, 0x020E, 0x01);
	write_cmos_sensor(ctx, 0x020F, 0x00);

	if (imx258_type == IMX258_HDR_TYPE || imx258_type == IMX258_RAW_TYPE) {
		write_cmos_sensor(ctx, 0x94DC, 0x20);
		write_cmos_sensor(ctx, 0x94DD, 0x20);
		write_cmos_sensor(ctx, 0x94DE, 0x20);
		write_cmos_sensor(ctx, 0x95DC, 0x20);
		write_cmos_sensor(ctx, 0x95DD, 0x20);
		write_cmos_sensor(ctx, 0x95DE, 0x20);
		write_cmos_sensor(ctx, 0x7FB0, 0x00);
		write_cmos_sensor(ctx, 0x9010, 0x3E);
	} else if (imx258_type == IMX258_BINNING_TYPE) {
		write_cmos_sensor(ctx, 0x7BCD, 0x01);
	}

	write_cmos_sensor(ctx, 0x3030, 0x00);
	LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
	write_cmos_sensor(ctx, 0x3032, 0x00);
	LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
	write_cmos_sensor(ctx, 0x0220, 0x00);

	write_cmos_sensor(ctx, 0x0350, 0x01);	/* Enable auto extend */


}

static void slim_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("slim video E\n");

	write_cmos_sensor(ctx, 0x0112, 0x0A);
	write_cmos_sensor(ctx, 0x0113, 0x0A);
	write_cmos_sensor(ctx, 0x0114, 0x03);

	write_cmos_sensor(ctx, 0x0301, 0x05);
	write_cmos_sensor(ctx, 0x0303, 0x02);
	write_cmos_sensor(ctx, 0x0305, 0x04);
	write_cmos_sensor(ctx, 0x0306, 0x00);
	write_cmos_sensor(ctx, 0x0307, 0x6C);
	write_cmos_sensor(ctx, 0x0309, 0x0A);
	write_cmos_sensor(ctx, 0x030B, 0x01);
	write_cmos_sensor(ctx, 0x030D, 0x02);
	write_cmos_sensor(ctx, 0x030E, 0x00);
	write_cmos_sensor(ctx, 0x030F, 0xD8);
	write_cmos_sensor(ctx, 0x0310, 0x00);
	write_cmos_sensor(ctx, 0x0820, 0x0A);
	write_cmos_sensor(ctx, 0x0821, 0x20);
	write_cmos_sensor(ctx, 0x0822, 0x00);
	write_cmos_sensor(ctx, 0x0823, 0x00);

	write_cmos_sensor(ctx, 0x0342, 0x14);
	write_cmos_sensor(ctx, 0x0343, 0xE8);

	write_cmos_sensor(ctx, 0x0340, 0x06);
	write_cmos_sensor(ctx, 0x0341, 0x4E);

	write_cmos_sensor(ctx, 0x0344, 0x00);
	write_cmos_sensor(ctx, 0x0345, 0x00);
	write_cmos_sensor(ctx, 0x0346, 0x00);
	write_cmos_sensor(ctx, 0x0347, 0x00);
	write_cmos_sensor(ctx, 0x0348, 0x10);
	write_cmos_sensor(ctx, 0x0349, 0x6F);
	write_cmos_sensor(ctx, 0x034A, 0x0C);
	write_cmos_sensor(ctx, 0x034B, 0x2F);

	write_cmos_sensor(ctx, 0x0381, 0x01);
	write_cmos_sensor(ctx, 0x0383, 0x01);
	write_cmos_sensor(ctx, 0x0385, 0x01);
	write_cmos_sensor(ctx, 0x0387, 0x01);
	write_cmos_sensor(ctx, 0x0900, 0x01);
	write_cmos_sensor(ctx, 0x0901, 0x12);
	write_cmos_sensor(ctx, 0x0902, 0x02);

	write_cmos_sensor(ctx, 0x0401, 0x01);
	write_cmos_sensor(ctx, 0x0404, 0x00);
	write_cmos_sensor(ctx, 0x0405, 0x20);
	write_cmos_sensor(ctx, 0x0408, 0x00);
	write_cmos_sensor(ctx, 0x0409, 0x00);
	write_cmos_sensor(ctx, 0x040A, 0x00);
	write_cmos_sensor(ctx, 0x040B, 0x00);
	write_cmos_sensor(ctx, 0x040C, 0x10);
	write_cmos_sensor(ctx, 0x040D, 0x70);
	write_cmos_sensor(ctx, 0x040E, 0x06);
	write_cmos_sensor(ctx, 0x040F, 0x18);
	write_cmos_sensor(ctx, 0x3038, 0x00);
	write_cmos_sensor(ctx, 0x303A, 0x00);
	write_cmos_sensor(ctx, 0x303B, 0x10);
	write_cmos_sensor(ctx, 0x300D, 0x00);

	write_cmos_sensor(ctx, 0x034C, 0x08);
	write_cmos_sensor(ctx, 0x034D, 0x34);
	write_cmos_sensor(ctx, 0x034E, 0x06);
	write_cmos_sensor(ctx, 0x034F, 0x18);

	write_cmos_sensor(ctx, 0x0202, 0x06);
	write_cmos_sensor(ctx, 0x0203, 0x44);

	write_cmos_sensor(ctx, 0x0204, 0x00);
	write_cmos_sensor(ctx, 0x0205, 0x00);
	write_cmos_sensor(ctx, 0x020E, 0x01);
	write_cmos_sensor(ctx, 0x020F, 0x00);

	if (imx258_type == IMX258_HDR_TYPE || imx258_type == IMX258_RAW_TYPE) {
		write_cmos_sensor(ctx, 0x94DC, 0x20);
		write_cmos_sensor(ctx, 0x94DD, 0x20);
		write_cmos_sensor(ctx, 0x94DE, 0x20);
		write_cmos_sensor(ctx, 0x95DC, 0x20);
		write_cmos_sensor(ctx, 0x95DD, 0x20);
		write_cmos_sensor(ctx, 0x95DE, 0x20);
		write_cmos_sensor(ctx, 0x7FB0, 0x00);
		write_cmos_sensor(ctx, 0x9010, 0x3E);
	} else if (imx258_type == IMX258_BINNING_TYPE) {
		write_cmos_sensor(ctx, 0x7BCD, 0x01);
	}

	write_cmos_sensor(ctx, 0x3030, 0x00);
	LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
	write_cmos_sensor(ctx, 0x3032, 0x00);
	LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
	write_cmos_sensor(ctx, 0x0220, 0x00);

	write_cmos_sensor(ctx, 0x0350, 0x01);	/* Enable auto extend */

}

static void custom1_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("custom1 E\n");


	/*data rate = 1014 Mbps/lane */
	write_cmos_sensor(ctx, 0x0112, 0x0A);
	write_cmos_sensor(ctx, 0x0113, 0x0A);
	write_cmos_sensor(ctx, 0x0114, 0x03);
	write_cmos_sensor(ctx, 0x0301, 0x05);
	write_cmos_sensor(ctx, 0x0303, 0x02);
	write_cmos_sensor(ctx, 0x0305, 0x04);
	write_cmos_sensor(ctx, 0x0306, 0x00);
	write_cmos_sensor(ctx, 0x0307, 0xA9);
	write_cmos_sensor(ctx, 0x0309, 0x0A);
	write_cmos_sensor(ctx, 0x030B, 0x01);
	write_cmos_sensor(ctx, 0x030D, 0x02);
	write_cmos_sensor(ctx, 0x030E, 0x00);
	write_cmos_sensor(ctx, 0x030F, 0xD8);
	write_cmos_sensor(ctx, 0x0310, 0x00);
	write_cmos_sensor(ctx, 0x0820, 0x0F);
	write_cmos_sensor(ctx, 0x0821, 0xD8);
	write_cmos_sensor(ctx, 0x0822, 0x00);
	write_cmos_sensor(ctx, 0x0823, 0x00);
	write_cmos_sensor(ctx, 0x4648, 0x7F);
	write_cmos_sensor(ctx, 0x7420, 0x00);
	write_cmos_sensor(ctx, 0x7421, 0x1C);
	write_cmos_sensor(ctx, 0x7422, 0x00);
	write_cmos_sensor(ctx, 0x7423, 0xD7);
	write_cmos_sensor(ctx, 0x9104, 0x00);
	write_cmos_sensor(ctx, 0x0342, 0x14);
	write_cmos_sensor(ctx, 0x0343, 0xE8);
	write_cmos_sensor(ctx, 0x0340, 0x0C);
	write_cmos_sensor(ctx, 0x0341, 0x54);
	write_cmos_sensor(ctx, 0x0344, 0x00);
	write_cmos_sensor(ctx, 0x0345, 0x00);
	write_cmos_sensor(ctx, 0x0346, 0x00);
	write_cmos_sensor(ctx, 0x0347, 0x00);
	write_cmos_sensor(ctx, 0x0348, 0x10);
	write_cmos_sensor(ctx, 0x0349, 0x6F);
	write_cmos_sensor(ctx, 0x034A, 0x0C);
	write_cmos_sensor(ctx, 0x034B, 0x2F);

	write_cmos_sensor(ctx, 0x0381, 0x01);
	write_cmos_sensor(ctx, 0x0383, 0x01);
	write_cmos_sensor(ctx, 0x0385, 0x01);
	write_cmos_sensor(ctx, 0x0387, 0x01);
	write_cmos_sensor(ctx, 0x0900, 0x00);
	write_cmos_sensor(ctx, 0x0901, 0x11);

	write_cmos_sensor(ctx, 0x0401, 0x00);
	write_cmos_sensor(ctx, 0x0404, 0x00);
	write_cmos_sensor(ctx, 0x0405, 0x10);
	write_cmos_sensor(ctx, 0x0408, 0x00);
	write_cmos_sensor(ctx, 0x0409, 0x00);
	write_cmos_sensor(ctx, 0x040A, 0x00);
	write_cmos_sensor(ctx, 0x040B, 0x00);
	write_cmos_sensor(ctx, 0x040C, 0x10);
	write_cmos_sensor(ctx, 0x040D, 0x70);
	write_cmos_sensor(ctx, 0x040E, 0x0C);
	write_cmos_sensor(ctx, 0x040F, 0x30);
	write_cmos_sensor(ctx, 0x3038, 0x00);
	write_cmos_sensor(ctx, 0x303A, 0x00);
	write_cmos_sensor(ctx, 0x303B, 0x10);
	write_cmos_sensor(ctx, 0x300D, 0x00);

	write_cmos_sensor(ctx, 0x034C, 0x10);
	write_cmos_sensor(ctx, 0x034D, 0x70);
	write_cmos_sensor(ctx, 0x034E, 0x0C);
	write_cmos_sensor(ctx, 0x034F, 0x30);
	write_cmos_sensor(ctx, 0x0202, 0x0C);
	write_cmos_sensor(ctx, 0x0203, 0x4A);
	write_cmos_sensor(ctx, 0x0204, 0x00);
	write_cmos_sensor(ctx, 0x0205, 0x00);
	write_cmos_sensor(ctx, 0x020E, 0x01);
	write_cmos_sensor(ctx, 0x020F, 0x00);
	write_cmos_sensor(ctx, 0x0210, 0x01);
	write_cmos_sensor(ctx, 0x0211, 0x00);
	write_cmos_sensor(ctx, 0x0212, 0x01);
	write_cmos_sensor(ctx, 0x0213, 0x00);
	write_cmos_sensor(ctx, 0x0214, 0x01);
	write_cmos_sensor(ctx, 0x0215, 0x00);

	write_cmos_sensor(ctx, 0x7BCD, 0x00);
	write_cmos_sensor(ctx, 0x94DC, 0x20);
	write_cmos_sensor(ctx, 0x94DD, 0x20);
	write_cmos_sensor(ctx, 0x94DE, 0x20);
	write_cmos_sensor(ctx, 0x95DC, 0x20);
	write_cmos_sensor(ctx, 0x95DD, 0x20);
	write_cmos_sensor(ctx, 0x95DE, 0x20);
	write_cmos_sensor(ctx, 0x7FB0, 0x00);
	write_cmos_sensor(ctx, 0x9010, 0x3E);
	write_cmos_sensor(ctx, 0x9419, 0x50);
	write_cmos_sensor(ctx, 0x941B, 0x50);
	write_cmos_sensor(ctx, 0x9519, 0x50);
	write_cmos_sensor(ctx, 0x951B, 0x50);
	write_cmos_sensor(ctx, 0x7FB4, 0x00);
	write_cmos_sensor(ctx, 0x94BE, 0x0C);
	write_cmos_sensor(ctx, 0x94BF, 0x0C);
	write_cmos_sensor(ctx, 0x94C0, 0x0C);
	write_cmos_sensor(ctx, 0x94C4, 0x3F);
	write_cmos_sensor(ctx, 0x94C5, 0x3F);
	write_cmos_sensor(ctx, 0x94C6, 0x3F);
	write_cmos_sensor(ctx, 0x94CA, 0x4C);
	write_cmos_sensor(ctx, 0x94CB, 0x4C);
	write_cmos_sensor(ctx, 0x94CC, 0x4C);
	write_cmos_sensor(ctx, 0x94CD, 0x2E);
	write_cmos_sensor(ctx, 0x94CE, 0x2E);
	write_cmos_sensor(ctx, 0x94CF, 0x2E);
	write_cmos_sensor(ctx, 0x3030, 0x01);
	write_cmos_sensor(ctx, 0x3032, 0x01);
	write_cmos_sensor(ctx, 0x0220, 0x00);

}

static void custom2_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("custom2 E\n");
	/* binning mode,  2100 X 1560  24fps */


	write_cmos_sensor(ctx, 0x0112, 0x0A);
	write_cmos_sensor(ctx, 0x0113, 0x0A);
	write_cmos_sensor(ctx, 0x0114, 0x03);

	write_cmos_sensor(ctx, 0x0301, 0x05);
	write_cmos_sensor(ctx, 0x0303, 0x02);
	write_cmos_sensor(ctx, 0x0305, 0x04);
	write_cmos_sensor(ctx, 0x0306, 0x00);
	write_cmos_sensor(ctx, 0x0307, 0x6C);
	write_cmos_sensor(ctx, 0x0309, 0x0A);
	write_cmos_sensor(ctx, 0x030B, 0x01);
	write_cmos_sensor(ctx, 0x030D, 0x02);
	write_cmos_sensor(ctx, 0x030E, 0x00);
	write_cmos_sensor(ctx, 0x030F, 0xD8);
	write_cmos_sensor(ctx, 0x0310, 0x00);
	write_cmos_sensor(ctx, 0x0820, 0x0A);
	write_cmos_sensor(ctx, 0x0821, 0x20);
	write_cmos_sensor(ctx, 0x0822, 0x00);
	write_cmos_sensor(ctx, 0x0823, 0x00);

	write_cmos_sensor(ctx, 0x0342, 0x14);
	write_cmos_sensor(ctx, 0x0343, 0xE8);

	write_cmos_sensor(ctx, 0x0340, 0x07);
	write_cmos_sensor(ctx, 0x0341, 0xE2);

	write_cmos_sensor(ctx, 0x0344, 0x00);
	write_cmos_sensor(ctx, 0x0345, 0x00);
	write_cmos_sensor(ctx, 0x0346, 0x00);
	write_cmos_sensor(ctx, 0x0347, 0x00);
	write_cmos_sensor(ctx, 0x0348, 0x10);
	write_cmos_sensor(ctx, 0x0349, 0x6F);
	write_cmos_sensor(ctx, 0x034A, 0x0C);
	write_cmos_sensor(ctx, 0x034B, 0x2F);

	write_cmos_sensor(ctx, 0x0381, 0x01);
	write_cmos_sensor(ctx, 0x0383, 0x01);
	write_cmos_sensor(ctx, 0x0385, 0x01);
	write_cmos_sensor(ctx, 0x0387, 0x01);
	write_cmos_sensor(ctx, 0x0900, 0x01);
	write_cmos_sensor(ctx, 0x0901, 0x12);
	write_cmos_sensor(ctx, 0x0902, 0x02);

	write_cmos_sensor(ctx, 0x0401, 0x01);
	write_cmos_sensor(ctx, 0x0404, 0x00);
	write_cmos_sensor(ctx, 0x0405, 0x20);
	write_cmos_sensor(ctx, 0x0408, 0x00);
	write_cmos_sensor(ctx, 0x0409, 0x00);
	write_cmos_sensor(ctx, 0x040A, 0x00);
	write_cmos_sensor(ctx, 0x040B, 0x00);
	write_cmos_sensor(ctx, 0x040C, 0x10);
	write_cmos_sensor(ctx, 0x040D, 0x70);
	write_cmos_sensor(ctx, 0x040E, 0x06);
	write_cmos_sensor(ctx, 0x040F, 0x18);
	write_cmos_sensor(ctx, 0x3038, 0x00);
	write_cmos_sensor(ctx, 0x303A, 0x00);
	write_cmos_sensor(ctx, 0x303B, 0x10);
	write_cmos_sensor(ctx, 0x300D, 0x00);

	write_cmos_sensor(ctx, 0x034C, 0x08);
	write_cmos_sensor(ctx, 0x034D, 0x34);
	write_cmos_sensor(ctx, 0x034E, 0x06);
	write_cmos_sensor(ctx, 0x034F, 0x18);

	write_cmos_sensor(ctx, 0x0202, 0x06);
	write_cmos_sensor(ctx, 0x0203, 0x4E);

	write_cmos_sensor(ctx, 0x0204, 0x00);
	write_cmos_sensor(ctx, 0x0205, 0x00);
	write_cmos_sensor(ctx, 0x020E, 0x01);
	write_cmos_sensor(ctx, 0x020F, 0x00);

	if (imx258_type == IMX258_HDR_TYPE) {
		write_cmos_sensor(ctx, 0x7BCD, 0x00);
		write_cmos_sensor(ctx, 0x94DC, 0x20);
		write_cmos_sensor(ctx, 0x94DD, 0x20);
		write_cmos_sensor(ctx, 0x94DE, 0x20);
		write_cmos_sensor(ctx, 0x95DC, 0x20);
		write_cmos_sensor(ctx, 0x95DD, 0x20);
		write_cmos_sensor(ctx, 0x95DE, 0x20);
		write_cmos_sensor(ctx, 0x7FB0, 0x00);
		write_cmos_sensor(ctx, 0x9010, 0x3E);
		write_cmos_sensor(ctx, 0x9419, 0x50);
		write_cmos_sensor(ctx, 0x941B, 0x50);
		write_cmos_sensor(ctx, 0x9519, 0x50);
		write_cmos_sensor(ctx, 0x951B, 0x50);
	} else if (imx258_type == IMX258_BINNING_TYPE) {
		write_cmos_sensor(ctx, 0x7BCD, 0x01);
	}

	write_cmos_sensor(ctx, 0x3030, 0x00);
	LOG_INF("0x3030=%d\n", read_cmos_sensor(ctx, 0x3030));
	write_cmos_sensor(ctx, 0x3032, 0x00);
	LOG_INF("0x3032=%d\n", read_cmos_sensor(ctx, 0x3032));
	write_cmos_sensor(ctx, 0x0220, 0x00);

}

/*************************************************************************
 * FUNCTION
 *	get_imgsensor_id
 *
 * DESCRIPTION
 *	This function get the sensor ID
 *
 * PARAMETERS
 *	*sensorID : return the sensor ID
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int get_imgsensor_id(struct subdrv_ctx *ctx, UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry_total = 1;
	kal_uint8 retry_cnt = retry_total;
	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
			*sensor_id = return_sensor_id(ctx);
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, *sensor_id);
				load_imx258_SPC_Data(ctx);
				return ERROR_NONE;
			}

		      pr_debug("Read sensor id fail, write id: 0x%x, id:0x%x\n",
				ctx->i2c_write_id, *sensor_id);

			retry_cnt--;
		} while (retry_cnt > 0);
		i++;
		retry_cnt = retry_total;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {

	/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF */
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}


/*************************************************************************
 * FUNCTION
 *	open
 *
 * DESCRIPTION
 *	This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int open(struct subdrv_ctx *ctx)
{
	/* const kal_uint8 i2c_addr[] = {
	 * IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2};
	 */
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0;

	LOG_1;
	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
			sensor_id = return_sensor_id(ctx);
			if (sensor_id == imgsensor_info.sensor_id) {
				pr_debug("i2c write id: 0x%x, sensor id:0x%x\n",
					ctx->i2c_write_id, sensor_id);
				break;
			}

		      pr_debug("Read sensor id fail, write id: 0x%x, id:0x%x\n",
				ctx->i2c_write_id, sensor_id);

			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;

	/* initail sequence write in  */
	sensor_init(ctx);


	ctx->autoflicker_en = KAL_FALSE;
	ctx->sensor_mode = IMGSENSOR_MODE_INIT;
	ctx->shutter = 0x3D0;
	ctx->gain = BASEGAIN * 4;
	ctx->pclk = imgsensor_info.pre.pclk;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->dummy_pixel = 0;
	ctx->dummy_line = 0;
	ctx->hdr_mode = 0;
	ctx->test_pattern = KAL_FALSE;
	ctx->current_fps = imgsensor_info.pre.max_framerate;

	return ERROR_NONE;
}				/*      open  */



/*************************************************************************
 * FUNCTION
 *	close
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int close(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");

	/*No Need to implement this function */

	return ERROR_NONE;
}				/*      close  */

static kal_uint32 custom1(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	if (ctx->current_fps == imgsensor_info.custom1.max_framerate) {
		ctx->pclk = imgsensor_info.custom1.pclk;
		ctx->line_length = imgsensor_info.custom1.linelength;
		ctx->frame_length = imgsensor_info.custom1.framelength;
		ctx->min_frame_length = imgsensor_info.custom1.framelength;
		ctx->autoflicker_en = KAL_FALSE;
	}
	/*PIP24fps_capture_setting(ctx->current_fps,ctx->pdaf_mode);*/
	custom1_setting(ctx);
	return ERROR_NONE;
}

static kal_uint32 custom2(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM2;
	if (ctx->current_fps == imgsensor_info.custom2.max_framerate) {
		ctx->pclk = imgsensor_info.custom2.pclk;
		ctx->line_length = imgsensor_info.custom2.linelength;
		ctx->frame_length = imgsensor_info.custom2.framelength;
		ctx->min_frame_length = imgsensor_info.custom2.framelength;
		ctx->autoflicker_en = KAL_FALSE;
	}
	custom2_setting(ctx);
	return ERROR_NONE;
}

/*************************************************************************
 * FUNCTION
 * preview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *	*image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 preview(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	ctx->sensor_mode = IMGSENSOR_MODE_PREVIEW;
	ctx->pclk = imgsensor_info.pre.pclk;
	/* ctx->video_mode = KAL_FALSE; */
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	preview_setting(ctx);
	return ERROR_NONE;
}				/*      preview   */

/*************************************************************************
 * FUNCTION
 *	capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 capture(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (ctx->current_fps == imgsensor_info.cap1.max_framerate) {
		ctx->pclk = imgsensor_info.cap1.pclk;
		ctx->line_length = imgsensor_info.cap1.linelength;
		ctx->frame_length = imgsensor_info.cap1.framelength;
		ctx->min_frame_length = imgsensor_info.cap1.framelength;
		ctx->autoflicker_en = KAL_FALSE;
	} else {
		if (ctx->current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF(
			    "current_fps %d fps is not support, so use cap's setting: %d fps!\n",
			    ctx->current_fps,
			    imgsensor_info.cap.max_framerate / 10);
		ctx->pclk = imgsensor_info.cap.pclk;
		ctx->line_length = imgsensor_info.cap.linelength;
		ctx->frame_length = imgsensor_info.cap.framelength;
		ctx->min_frame_length = imgsensor_info.cap.framelength;
		ctx->autoflicker_en = KAL_FALSE;
	}
	capture_setting(ctx, ctx->current_fps, ctx->pdaf_mode);
	mdelay(100);
	return ERROR_NONE;
}				/* capture(ctx) */

static kal_uint32 normal_video(struct subdrv_ctx *ctx,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	ctx->sensor_mode = IMGSENSOR_MODE_VIDEO;
	ctx->pclk = imgsensor_info.normal_video.pclk;
	ctx->line_length = imgsensor_info.normal_video.linelength;
	ctx->frame_length = imgsensor_info.normal_video.framelength;
	ctx->min_frame_length = imgsensor_info.normal_video.framelength;
	/* ctx->current_fps = 300; */
	ctx->autoflicker_en = KAL_FALSE;
	LOG_INF("ihdr enable :%d\n", ctx->hdr_mode);
	normal_video_setting(ctx, ctx->current_fps, ctx->pdaf_mode);
	return ERROR_NONE;
}				/*      normal_video   */

static kal_uint32 hs_video(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	ctx->sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	ctx->pclk = imgsensor_info.hs_video.pclk;
	/* ctx->video_mode = KAL_TRUE; */
	ctx->line_length = imgsensor_info.hs_video.linelength;
	ctx->frame_length = imgsensor_info.hs_video.framelength;
	ctx->min_frame_length = imgsensor_info.hs_video.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	/* ctx->current_fps = 600; */
	ctx->autoflicker_en = KAL_FALSE;
	hs_video_setting(ctx);

	return ERROR_NONE;
}				/*      hs_video   */

static kal_uint32 slim_video(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	ctx->sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	ctx->pclk = imgsensor_info.slim_video.pclk;
	/* ctx->video_mode = KAL_TRUE; */
	ctx->line_length = imgsensor_info.slim_video.linelength;
	ctx->frame_length = imgsensor_info.slim_video.framelength;
	ctx->min_frame_length = imgsensor_info.slim_video.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	/* ctx->current_fps = 1200; */
	ctx->autoflicker_en = KAL_FALSE;
	slim_video_setting(ctx);

	return ERROR_NONE;
}				/*      slim_video       */



static int get_resolution(struct subdrv_ctx *ctx,
		MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	int i = 0;

	for (i = SENSOR_SCENARIO_ID_MIN; i < SENSOR_SCENARIO_ID_MAX; i++) {
		if (i < imgsensor_info.sensor_mode_num) {
			sensor_resolution->SensorWidth[i] = imgsensor_winsize_info[i].w2_tg_size;
			sensor_resolution->SensorHeight[i] = imgsensor_winsize_info[i].h2_tg_size;
		} else {
			sensor_resolution->SensorWidth[i] = 0;
			sensor_resolution->SensorHeight[i] = 0;
		}
	}



	return ERROR_NONE;
}				/*      get_resolution  */

static int get_info(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;

	/* not use */
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;

	/* inverse with datasheet */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4;	/* not use */
	sensor_info->SensorResetActiveHigh = FALSE;	/* not use */
	sensor_info->SensorResetDelayCount = 5;	/* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;

	sensor_info->SettleDelayMode =
		imgsensor_info.mipi_settle_delay_mode;

	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;


	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_PREVIEW] =
		imgsensor_info.pre_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_CAPTURE] =
		imgsensor_info.cap_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_VIDEO] =
		imgsensor_info.video_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO] =
		imgsensor_info.hs_video_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_SLIM_VIDEO] =
		imgsensor_info.slim_video_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_CUSTOM1] =
		imgsensor_info.custom1_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_CUSTOM2] =
		imgsensor_info.custom2_delay_frame;
	sensor_info->SensorMasterClockSwitch = 0;	/* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;

	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;

	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;

	sensor_info->FrameTimeDelayFrame =
		imgsensor_info.frame_time_delay_frame;

	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->TEMPERATURE_SUPPORT = 1;

	if (imx258_type == IMX258_HDR_TYPE)
		sensor_info->PDAF_Support = PDAF_SUPPORT_CAMSV;
	else if (imx258_type == IMX258_BINNING_TYPE)
		sensor_info->PDAF_Support = PDAF_SUPPORT_CAMSV_LEGACY;
	else if (imx258_type == IMX258_HDD_TYPE)
		sensor_info->PDAF_Support = PDAF_SUPPORT_CAMSV;
	else if (imx258_type == IMX258_RAW_TYPE)
		sensor_info->PDAF_Support = PDAF_SUPPORT_RAW;
	else
		sensor_info->PDAF_Support = PDAF_SUPPORT_NA;

	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3;	/* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2;	/* not use */
	sensor_info->SensorPixelClockCount = 3;	/* not use */
	sensor_info->SensorDataLatchCount = 2;	/* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;	/* 0 is default 1x */
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x */
	sensor_info->SensorPacketECCOrder = 1;

	return ERROR_NONE;
}				/*      get_info  */


static int control(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
			  MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("scenario_id = %d\n", scenario_id);
	ctx->current_scenario_id = scenario_id;
	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		preview(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		capture(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		normal_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		hs_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		slim_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		custom1(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
		custom2(ctx, image_window, sensor_config_data);
		break;
	default:
		LOG_INF("Error ScenarioId setting");
		preview(ctx, image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}				/* control(ctx) */



static kal_uint32 set_video_mode(struct subdrv_ctx *ctx, UINT16 framerate)
{
	LOG_DEBUG("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate */
	if (framerate == 0)
		/* Dynamic frame rate */
		return ERROR_NONE;
	if (framerate == 300 && ctx->autoflicker_en)
		ctx->current_fps = 296;
	else if (framerate == 150 && ctx->autoflicker_en)
		ctx->current_fps = 146;
	else
		ctx->current_fps = framerate;
	set_max_framerate(ctx, ctx->current_fps, 1);
	set_dummy(ctx);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(struct subdrv_ctx *ctx, INT32 enable, UINT16 framerate)
{
	LOG_DEBUG("enable = %d, framerate = %d\n", enable, framerate);
	ctx->autoflicker_en = enable;
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(struct subdrv_ctx *ctx,
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		frame_length = imgsensor_info.pre.pclk
			/ framerate * 10 / imgsensor_info.pre.linelength;


		ctx->dummy_line =
		  (frame_length > imgsensor_info.pre.framelength)
		? (frame_length - imgsensor_info.pre.framelength) : 0;

		ctx->frame_length =
			imgsensor_info.pre.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		if (framerate == 0)
			return ERROR_NONE;

		frame_length = imgsensor_info.normal_video.pclk
		    / framerate * 10 / imgsensor_info.normal_video.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.normal_video.framelength)
		? (frame_length - imgsensor_info.normal_video.framelength) : 0;

		ctx->frame_length =
		 imgsensor_info.normal_video.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
	if (ctx->current_fps == imgsensor_info.cap1.max_framerate) {
		frame_length = imgsensor_info.cap1.pclk
			/ framerate * 10 / imgsensor_info.cap1.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.cap1.framelength)
		? (frame_length - imgsensor_info.cap1.framelength) : 0;

		ctx->frame_length =
		    imgsensor_info.cap1.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
	} else {
		if (ctx->current_fps != imgsensor_info.cap.max_framerate)
			pr_debug(
			    "current_fps %d fps is not support, so use cap's setting: %d fps!\n",
			    framerate,
			    imgsensor_info.cap.max_framerate / 10);

		frame_length = imgsensor_info.cap.pclk /
			framerate * 10 / imgsensor_info.cap.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.cap.framelength)
		? (frame_length - imgsensor_info.cap.framelength) : 0;

		ctx->frame_length =
		    imgsensor_info.cap.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		}
		set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk
			/ framerate * 10 / imgsensor_info.hs_video.linelength;

		ctx->dummy_line =
		    (frame_length > imgsensor_info.hs_video.framelength)
		    ? (frame_length - imgsensor_info.hs_video.framelength) : 0;

		ctx->frame_length =
		    imgsensor_info.hs_video.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk
			/ framerate * 10 / imgsensor_info.slim_video.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.slim_video.framelength)
		? (frame_length - imgsensor_info.slim_video.framelength) : 0;

		ctx->frame_length =
		  imgsensor_info.slim_video.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk
		    / framerate * 10 / imgsensor_info.custom1.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.custom1.framelength)
		? (frame_length - imgsensor_info.custom1.framelength) : 0;

		ctx->frame_length =
		    imgsensor_info.custom1.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
		frame_length = imgsensor_info.custom2.pclk
		    / framerate * 10 / imgsensor_info.custom2.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.custom2.framelength)
		? (frame_length - imgsensor_info.custom2.framelength) : 0;

		ctx->frame_length =
		    imgsensor_info.custom2.framelength + ctx->dummy_line;

		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		break;
	default:		/* coding with  preview scenario by default */
		frame_length = imgsensor_info.pre.pclk
		    / framerate * 10 / imgsensor_info.pre.linelength;

		ctx->dummy_line =
		  (frame_length > imgsensor_info.pre.framelength)
		? (frame_length - imgsensor_info.pre.framelength) : 0;

		ctx->frame_length =
			imgsensor_info.pre.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		set_dummy(ctx);
		LOG_INF("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(struct subdrv_ctx *ctx,
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		*framerate = imgsensor_info.pre.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		*framerate = imgsensor_info.normal_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		*framerate = imgsensor_info.cap.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		*framerate = imgsensor_info.hs_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		*framerate = imgsensor_info.slim_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		*framerate = imgsensor_info.custom1.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
		*framerate = imgsensor_info.custom2.max_framerate;
		break;
	default:
		break;
	}

	return ERROR_NONE;
}


static kal_uint32 set_test_pattern_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable)
		write_cmos_sensor(ctx, 0x0601, 0x02);
	else
		write_cmos_sensor(ctx, 0x0601, 0x00);

	ctx->test_pattern = enable;
	return ERROR_NONE;
}

static kal_uint32 get_sensor_temperature(struct subdrv_ctx *ctx)
{
	UINT8 temperature;
	INT32 temperature_convert;

	/*TEMP_SEN_CTL */
	write_cmos_sensor(ctx, 0x0138, 0x01);
	temperature = read_cmos_sensor(ctx, 0x013a);

	if (temperature >= 0x0 && temperature <= 0x4F)
		temperature_convert = temperature;
	else if (temperature >= 0x50 && temperature <= 0x7F)
		temperature_convert = 80;
	else if (temperature >= 0x80 && temperature <= 0xEC)
		temperature_convert = -20;
	else
		temperature_convert = (INT8) temperature;

	LOG_INF("temp_c(%d), read_reg(%d)\n", temperature_convert, temperature);

	return temperature_convert;

}

static kal_uint32 streaming_control(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor(ctx, 0x0100, 0x01);
	else
		write_cmos_sensor(ctx, 0x0100, 0x00);
	mdelay(10);
	return ERROR_NONE;
}

static int feature_control(struct subdrv_ctx *ctx, MSDK_SENSOR_FEATURE_ENUM feature_id,
				  UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *)feature_para;

	/* unsigned long long *feature_return_para =
	 * (unsigned long long *) feature_para;
	 */

	struct SET_PD_BLOCK_INFO_T *PDAFinfo;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;

	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	/* LOG_INF("feature_id = %d\n", feature_id); */

	switch (feature_id) {
	case SENSOR_FEATURE_GET_ANA_GAIN_TABLE:
		if ((void *)(uintptr_t) (*(feature_data + 1)) == NULL) {
			*(feature_data + 0) =
				sizeof(imx258_ana_gain_table);
		} else {
			memcpy((void *)(uintptr_t) (*(feature_data + 1)),
			(void *)imx258_ana_gain_table,
			sizeof(imx258_ana_gain_table));
		}
		break;
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = ctx->line_length;
		*feature_return_para_16 = ctx->frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*feature_para = ctx->pclk;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(ctx, (UINT16) *feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		night_mode(ctx, (BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain(ctx, (UINT32) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;

	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor(ctx,
			sensor_reg_data->RegAddr, sensor_reg_data->RegData);
		break;

	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor(ctx, sensor_reg_data->RegAddr);
		break;

	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM
		 * or just return LENS_DRIVER_ID_DO_NOT_CARE
		 */
		/* if EEPROM does not exist in camera module. */
		*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len = 4;
		break;

	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(ctx, *feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		get_imgsensor_id(ctx, feature_return_para_32);
		break;

	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode(ctx,
			*feature_data_16, *(feature_data_16 + 1));
		break;

	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(ctx,
			(enum MSDK_SCENARIO_ID_ENUM) *feature_data,
					      *(feature_data + 1));
		break;

	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(ctx,
			(enum MSDK_SCENARIO_ID_ENUM) *feature_data,
			  (MUINT32 *) (uintptr_t) (*(feature_data + 1)));
		break;

	case SENSOR_FEATURE_GET_PDAF_DATA:
		pr_debug("SENSOR_FEATURE_GET_PDAF_DATA\n");
		read_imx258_pdaf(ctx, (kal_uint16) (*feature_data),
				 (char *)(uintptr_t) (*(feature_data + 1)),
				 (kal_uint32) (*(feature_data + 2)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode(ctx, (BOOL)*feature_data);
		break;

	/* for factory mode auto testing */
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;

	case SENSOR_FEATURE_SET_FRAMERATE:
		LOG_INF("current fps :%d\n", *feature_data_32);
		ctx->current_fps = (UINT16)*feature_data_32;
		break;
	case SENSOR_FEATURE_SET_HDR:
		/* HDR mODE : 0: disable HDR, 1:IHDR, 2:HDR, 9:ZHDR */
		LOG_INF("ihdr enable :%d\n", *feature_data_32);
		ctx->hdr_mode = (UINT8)*feature_data_32;
		LOG_INF("ihdr enable :%d\n", ctx->hdr_mode);
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
			(UINT32) *feature_data);
		wininfo =
	(struct SENSOR_WINSIZE_INFO_STRUCT *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data_32) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[1],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[2],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[3],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[4],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_CUSTOM1:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[5],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_CUSTOM2:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[6],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[0],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;

	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%llu\n",
			*feature_data);
		PDAFinfo =
	      (struct SET_PD_BLOCK_INFO_T *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			if (imx258_type != IMX258_BINNING_TYPE)
				memcpy(
				(void *)PDAFinfo,
				(void *)&imgsensor_pd_info,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			else
				memcpy(
				(void *)PDAFinfo,
				(void *)&imgsensor_pd_info_Binning,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			break;

		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			break;
		}
		break;

	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16) *feature_data,
			(UINT16) *(feature_data + 1),
			(UINT16) *(feature_data + 2));

		ihdr_write_shutter_gain(ctx, (UINT16) *feature_data,
					(UINT16) *(feature_data + 1),
					(UINT16) *(feature_data + 2));
		break;
	case SENSOR_FEATURE_GET_VC_INFO:
		LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n",
			(UINT16) *feature_data);
		pvcinfo = (struct SENSOR_VC_INFO_STRUCT *)
			(uintptr_t) (*(feature_data + 1));
		switch (*feature_data_32) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_CUSTOM1:
			if (imx258_type != IMX258_BINNING_TYPE)
				memcpy(
				(void *)pvcinfo,
				(void *)&SENSOR_VC_INFO[1],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			else
				memcpy(
				(void *)pvcinfo,
				(void *)&SENSOR_VC_INFO_Binning[1],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			if (imx258_type != IMX258_BINNING_TYPE)
				memcpy(
				(void *)pvcinfo,
				(void *)&SENSOR_VC_INFO[2],
				 sizeof(struct SENSOR_VC_INFO_STRUCT));
			else
				memcpy(
				(void *)pvcinfo,
				(void *)&SENSOR_VC_INFO_Binning[2],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		case SENSOR_SCENARIO_ID_CUSTOM2:
		default:
			if (imx258_type != IMX258_BINNING_TYPE)
				memcpy(
				(void *)pvcinfo,
				(void *)&SENSOR_VC_INFO[0],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			else
				memcpy(
				(void *)pvcinfo,
				(void *)&SENSOR_VC_INFO_Binning[0],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		}
		break;
		/*PDAF CMD */
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		LOG_INF(
		    "SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%llu\n",
		    *feature_data);
		/* PDAF capacity enable or not,
		 * 2p8 only full size support PDAF
		 */
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;
			break;

		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		case SENSOR_SCENARIO_ID_CUSTOM2:
		default:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_SET_PDAF:
		LOG_INF("PDAF mode :%d\n", *feature_data_16);
		ctx->pdaf_mode = *feature_data_16;
		break;

	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length(ctx,
			(UINT16) *feature_data, (UINT16) *(feature_data + 1));
		break;

	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_i32 = get_sensor_temperature(ctx);
		*feature_para_len = 4;
		break;

	case SENSOR_FEATURE_GET_PDAF_TYPE:
		switch (imx258_type) {
		case 0x00:
			sprintf(
			      feature_para, "IMX258 0AQH5-C (BME-HDR version)");
			break;

		case 0x10:
			sprintf(
			      feature_para, "IMX258 0APH5-C (Binning version)");
			break;

		case 0x20:
			sprintf(
				feature_para, "IMX258 0AMH5-C (B/W version)");
			break;

		case 0x30:
			sprintf(
			     feature_para, "IMX258 0ATH5-C (Non-PDAF version)");
			break;

		case 0x40:
			sprintf(feature_para,
			  "IMX258 0AUH5-C (Horizontal Double Density version)");
			break;
		case 0x80:
			sprintf(feature_para, "IMX258 enable PDO");
			break;
		default:
			sprintf(feature_para, "Other case %x", imx258_type);
		}

		LOG_INF("get PDAF type = %d\n", imx258_type);
		break;
	case SENSOR_FEATURE_SET_PDAF_TYPE:
		if (strstr(&(*feature_para), "pdo")) {
			test_Pmode = 1;
			imx258_type = IMX258_RAW_TYPE;
		} else {
			test_Pmode = 0;
			imx258_type = IMX258_HDD_TYPE;
		}
		LOG_INF("set Pinfo = %d\n", test_Pmode);
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(ctx, KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME\n");
		if (*feature_data != 0)
			set_shutter(ctx, *feature_data);
		streaming_control(ctx, KAL_TRUE);
		break;
	case SENSOR_FEATURE_SET_FRAMELENGTH:
		set_frame_length(ctx, (UINT16) (*feature_data));
		break;
	default:
		break;
	}

	return ERROR_NONE;
}				/*      feature_control(ctx)  */



static const struct subdrv_ctx defctx = {

	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_max = BASEGAIN * 16,
	.ana_gain_min = BASEGAIN,
	.ana_gain_step = 1,
	.exposure_def = 0x3d0,
	.exposure_max = 0x7fff - 4,
	.exposure_min = 1,
	.exposure_step = 1,
	.frame_time_delay_frame = 3,
	.margin = 4,
	.max_frame_length = 0x7fff,

	.mirror = IMAGE_NORMAL,	/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3d0,	/* current shutter */
	.gain = BASEGAIN * 4,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */

	/* full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,

	/* current scenario id */
	.current_scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW,

	.hdr_mode = 0,	/* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x34,	/* record current sensor's i2c write id */
};

static int init_ctx(struct subdrv_ctx *ctx,
		struct i2c_client *i2c_client, u8 i2c_write_id)
{
	memcpy(ctx, &defctx, sizeof(*ctx));
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}

static int get_temp(struct subdrv_ctx *ctx, int *temp)
{
	*temp = get_sensor_temperature(ctx) * 1000;
	return 0;
}

static struct subdrv_ops ops = {
	.get_id = get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = open,
	.get_info = get_info,
	.get_resolution = get_resolution,
	.control = control,
	.feature_control = feature_control,
	.close = close,
	.get_temp = get_temp,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_MCLK, 24, 0},
	{HW_ID_MCLK_DRIVING_CURRENT, 4, 0},
	{HW_ID_PDN, 0, 0},
	{HW_ID_RST, 0, 0},
	{HW_ID_DOVDD, 1800000, 0},
	{HW_ID_AVDD, 2800000, 0},
	{HW_ID_DVDD, 1200000, 0},
	{HW_ID_AFVDD, 2800000, 1},
	{HW_ID_PDN, 1, 0},
	{HW_ID_RST, 1, 5},
};

const struct subdrv_entry imx258_mipi_raw_entry = {
	.name = "imx258_mipi_raw",
	.id = IMX258_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

