// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include "cam_cal.h"
#include "cam_cal_define.h"
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif
#include "eeprom_utils.h"

/* Include platform define if necessary */
#ifdef EEPROM_PLATFORM_DEFINE
#include "eeprom_platform_def.h"
#endif

/************************************************************
 * I2C read function (Common)
 ************************************************************/

/* add for linux-4.4 */
#ifndef I2C_WR_FLAG
#define I2C_WR_FLAG		(0x1000)
#define I2C_MASK_FLAG	(0x00ff)
#endif

#define EEPROM_I2C_MSG_SIZE_READ 2

#ifndef EEPROM_I2C_READ_MSG_LENGTH_MAX
#define EEPROM_I2C_READ_MSG_LENGTH_MAX 1024
#endif
#ifndef EEPROM_I2C_WRITE_MSG_LENGTH_MAX
#define EEPROM_I2C_WRITE_MSG_LENGTH_MAX 32
#endif
#ifndef EEPROM_WRITE_EN
#define EEPROM_WRITE_EN 0
#endif

static int Read_I2C_CAM_CAL(struct i2c_client *client,
			    u16 a_u2Addr,
			    u32 ui4_length,
			    u8 *a_puBuff)
{
	int i4RetValue = 0;
	char puReadCmd[2] = { (char)(a_u2Addr >> 8), (char)(a_u2Addr & 0xFF) };
	struct i2c_msg msg[EEPROM_I2C_MSG_SIZE_READ];

	if (ui4_length > EEPROM_I2C_READ_MSG_LENGTH_MAX) {
		must_log("exceed one transition %d bytes limitation\n",
			 EEPROM_I2C_READ_MSG_LENGTH_MAX);
		return -1;
	}

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = 2;
	msg[0].buf = puReadCmd;

	msg[1].addr = client->addr;
	msg[1].flags = client->flags & I2C_M_TEN;
	msg[1].flags |= I2C_M_RD;
	msg[1].len = ui4_length;
	msg[1].buf = a_puBuff;

	i4RetValue = i2c_transfer(client->adapter, msg,
				EEPROM_I2C_MSG_SIZE_READ);

	if (i4RetValue != EEPROM_I2C_MSG_SIZE_READ) {
		must_log("I2C read data failed!!\n");
		return -1;
	}

	return 0;
}

static int iReadData_CAM_CAL(struct i2c_client *client,
			     unsigned int ui4_offset,
			     unsigned int ui4_length,
			     unsigned char *pinputdata)
{
	int i4ResidueSize;
	u32 u4CurrentOffset, u4Size;
	u8 *pBuff;

	i4ResidueSize = (int)ui4_length;
	u4CurrentOffset = ui4_offset;
	pBuff = pinputdata;
	do {
		u4Size = (i4ResidueSize >= EEPROM_I2C_READ_MSG_LENGTH_MAX)
			? EEPROM_I2C_READ_MSG_LENGTH_MAX : i4ResidueSize;

		if (Read_I2C_CAM_CAL(client, (u16) u4CurrentOffset,
				     u4Size, pBuff) != 0) {
			must_log("I2C iReadData failed!!\n");
			return -1;
		}

		i4ResidueSize -= u4Size;
		u4CurrentOffset += u4Size;
		pBuff += u4Size;
	} while (i4ResidueSize > 0);

	return 0;
}

#if EEPROM_WRITE_EN

static int Write_I2C_CAM_CAL(struct i2c_client *client,
			     u16 a_u2Addr,
			     u32 ui4_length,
			     u8 *a_puBuff)
{
	int i4RetValue = 0;
	char puCmd[2 + EEPROM_I2C_WRITE_MSG_LENGTH_MAX];
	struct i2c_msg msg;

	if (ui4_length > EEPROM_I2C_WRITE_MSG_LENGTH_MAX) {
		must_log("exceed one transition %d bytes limitation\n",
			 EEPROM_I2C_WRITE_MSG_LENGTH_MAX);
		return -1;
	}

	puCmd[0] = (char)(a_u2Addr >> 8);
	puCmd[1] = (char)(a_u2Addr & 0xFF);
	memcpy(puCmd + 2, a_puBuff, ui4_length);

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = 2 + ui4_length;
	msg.buf = puCmd;

	i4RetValue = i2c_transfer(client->adapter, &msg, 1);

	if (i4RetValue != 1) {
		must_log("I2C write data failed!!\n");
		return -1;
	}

	/* Wait for write complete */
	mdelay(5);

	return 0;
}

static int iWriteData_CAM_CAL(struct i2c_client *client,
			     unsigned int ui4_offset,
			     unsigned int ui4_length,
			     unsigned char *pinputdata)
{
	int i4ResidueSize;
	u32 u4CurrentOffset, u4Size;
	u8 *pBuff;

	i4ResidueSize = (int)ui4_length;
	u4CurrentOffset = ui4_offset;
	pBuff = pinputdata;
	do {
		u4Size = (i4ResidueSize >= EEPROM_I2C_WRITE_MSG_LENGTH_MAX)
			? EEPROM_I2C_WRITE_MSG_LENGTH_MAX : i4ResidueSize;

		if (Write_I2C_CAM_CAL(client, (u16) u4CurrentOffset,
				      u4Size, pBuff) != 0) {
			must_log("I2C iWriteData failed!!\n");
			return -1;
		}

		i4ResidueSize -= u4Size;
		u4CurrentOffset += u4Size;
		pBuff += u4Size;
	} while (i4ResidueSize > 0);

	return 0;
}
#endif

unsigned int Common_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	unsigned int ret = 0;
	struct timespec64 t;

	EEPROM_PROFILE_INIT(&t);

	if (iReadData_CAM_CAL(client, addr, size, data) == 0)
		ret = size;

	EEPROM_PROFILE(&t, "common_read_time");

	return ret;
}

unsigned int Common_write_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	unsigned int ret = 0;
#if EEPROM_WRITE_EN
	struct timespec64 t;

	EEPROM_PROFILE_INIT(&t);

	if (iWriteData_CAM_CAL(client, addr, size, data) == 0)
		ret = size;

	EEPROM_PROFILE(&t, "common_write_time");
#else
	must_log("Write operation disabled\n");
#endif

	return ret;
}

#ifdef __XIAOMI_CAMERA__
static bool Read_I2C_U8_CAM_CAL(struct i2c_client *client,
			    u8 a_u2Addr,
			    u32 ui4_length,
			    u8 *a_puBuff)
{
	int i4RetValue = 0;
	char puReadCmd[1] = {(char)(a_u2Addr)};
	struct i2c_msg msg[EEPROM_I2C_MSG_SIZE_READ];

	if (client == NULL) {
		return false;
	}

	if (ui4_length > EEPROM_I2C_READ_MSG_LENGTH_MAX) {
		must_log("exceed one transition %d bytes limitation\n",
			 EEPROM_I2C_READ_MSG_LENGTH_MAX);
		return false;
	}

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = puReadCmd;

	msg[1].addr = client->addr;
	msg[1].flags = 1;
	msg[1].len = ui4_length;
	msg[1].buf = a_puBuff;

	i4RetValue = i2c_transfer(client->adapter, msg,
				EEPROM_I2C_MSG_SIZE_READ);

	if (i4RetValue < 0) {
		must_log("I2C read data failed %d!!\n", i4RetValue);
		return false;
	}

	return true;
}

static int Write_I2C_U8_CAM_CAL(struct i2c_client *client,
			     u8 a_u2Addr,
			     u8 val)
{
	int i4RetValue = 0;
	char puCmd[2];
	struct i2c_msg msg;

	if (client == NULL) {
		return false;
	}

	puCmd[0] = a_u2Addr;
	puCmd[1] = val;

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.len = 2;
	msg.buf = puCmd;

	i4RetValue = i2c_transfer(client->adapter, &msg, 1);

	if (i4RetValue != 1) {
		must_log("I2C write data failed!!\n");
		return false;
	}
	return true;
}

struct OV08F_OTP_DATA
{
#define MAX_BLOCK_NUM 64
	u8 block_num;
	u8 block_reg_addr[MAX_BLOCK_NUM];
	u8 block_start_addr[MAX_BLOCK_NUM];
	u8 block_size[MAX_BLOCK_NUM];
};

static u8 ov08f_otp_all_data[3793];

static bool OV08F_eeprom_addr_to_otp_addr(unsigned int addr_in, unsigned int size_in, struct OV08F_OTP_DATA* ov08f_otp_data){
#define MAX_BLOCK_SIZE 0x80
	if(addr_in + size_in > 0x2000 || ov08f_otp_data == NULL){
		must_log("param error: 0x%x/%d/%p\n", addr_in, size_in, ov08f_otp_data);
		return false;
	}
	ov08f_otp_data->block_reg_addr[0]   = (int)(addr_in / MAX_BLOCK_SIZE);
	ov08f_otp_data->block_start_addr[0] = addr_in % MAX_BLOCK_SIZE;
	if(ov08f_otp_data->block_start_addr[0] + size_in <= MAX_BLOCK_SIZE){
		ov08f_otp_data->block_size[0] = size_in;
		ov08f_otp_data->block_num = 1;
		return true;
	} else {
		ov08f_otp_data->block_size[0] = 0x80 - ov08f_otp_data->block_start_addr[0];
		ov08f_otp_data->block_num = (int)((size_in - ov08f_otp_data->block_size[0])/0x80) + 1;
		if(ov08f_otp_data->block_num > 2){
			for(int i = 1; i < ov08f_otp_data->block_num; i++){
				ov08f_otp_data->block_reg_addr[i] = ov08f_otp_data->block_reg_addr[0] + i;
				ov08f_otp_data->block_start_addr[i] = 0x00;
				ov08f_otp_data->block_size[i] = 0x80;
			}
		}
		if((size_in - ov08f_otp_data->block_size[0])%0x80 != 0){
			u8 index = 0;
			ov08f_otp_data->block_num += 1;
			index = ov08f_otp_data->block_num - 1;
			ov08f_otp_data->block_reg_addr[index] = ov08f_otp_data->block_reg_addr[0] + index;
			ov08f_otp_data->block_start_addr[index] = 0x00;
			ov08f_otp_data->block_size[index] = (size_in - ov08f_otp_data->block_size[0])%0x80;
		}
		return true;
	}
}

int LAPIS_OV08F_get_all_OTP(struct i2c_client *client){

	static int isInit = 0;
	u8 flag[2];
	u32 offset = 0;
	struct OV08F_OTP_DATA ov08f_otp_data = {0};
	u32 ret = 0;

	if(isInit){
		pr_debug("LAPIS_OV08F_get_all_OTP: already init");
		return 0;
	}
	//read otp init

	Write_I2C_U8_CAM_CAL(client, 0xfd, 0x00);
	Write_I2C_U8_CAM_CAL(client, 0x1d, 0x00);
	Write_I2C_U8_CAM_CAL(client, 0x1c, 0x19);
	Write_I2C_U8_CAM_CAL(client, 0x20, 0x0f);
	Write_I2C_U8_CAM_CAL(client, 0xe7, 0x03);
	Write_I2C_U8_CAM_CAL(client, 0xe7, 0x00);
	mdelay(3);

	Write_I2C_U8_CAM_CAL(client, 0xfd, 0x03);
	Write_I2C_U8_CAM_CAL(client, 0xa1, 0x46);
	Write_I2C_U8_CAM_CAL(client, 0xa6, 0x44);

	Write_I2C_U8_CAM_CAL(client, 0xfd, 0x03);
	Write_I2C_U8_CAM_CAL(client, 0x9f, 0x20);
	Write_I2C_U8_CAM_CAL(client, 0x9d, 0x10);

	Write_I2C_U8_CAM_CAL(client, 0xfd, 0x03);
	Write_I2C_U8_CAM_CAL(client, 0xa9, 0x3F); //block63
	Write_I2C_U8_CAM_CAL(client, 0xfd, 0x09);

	Read_I2C_U8_CAM_CAL(client, 0x23, 2, flag); //Flag位 0x1FA3 0x1FA4

	if(flag[0] == 0x55 && flag[1] == 0x00){
		offset = 0x0200;    //group 1
	} else if(flag[0] == 0xff && flag[1] == 0x55){
		offset = 0x10D2; //group 2 
	} else{
		must_log("ov08f read group flag error: Group1 flag 0x%x, Group2 flag 0x%x\n", flag[0], flag[1]);
		return -1;
	}

	if(OV08F_eeprom_addr_to_otp_addr(offset, 3793, &ov08f_otp_data)){
		for(int i = 0; i < ov08f_otp_data.block_num; i++){
			Write_I2C_U8_CAM_CAL(client, 0xfd, 0x03);
			Write_I2C_U8_CAM_CAL(client, 0xa9, ov08f_otp_data.block_reg_addr[i]);
			Write_I2C_U8_CAM_CAL(client, 0xfd, 0x09);
			if(Read_I2C_U8_CAM_CAL(client, ov08f_otp_data.block_start_addr[i], ov08f_otp_data.block_size[i], &ov08f_otp_all_data[ret])){
				ret += ov08f_otp_data.block_size[i];
			} else {
				must_log("ov08f read sensor otp error\n");
				return -1;
			}
		}
	};
	// for(i = 0; i < 3793; i++){
	// 	pr_info("zry ov08f_otp_all_data[%x] = %x", i, ov08f_otp_all_data[i]&0xff);
	// }
	isInit = 1;
	return ret;
}

unsigned int LAPIS_OV08F_OTP_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	unsigned int ret = 0;
	struct timespec64 t;

	must_log("LAPIS_OV08F_OTP_read_region : client 0x%x, addr 0x%x, size %d", client->addr, addr, size);
	EEPROM_PROFILE_INIT(&t);

	ret = LAPIS_OV08F_get_all_OTP(client);
	if(ret < 0){
		must_log("LAPIS_OV08F_OTP_read_region : LAPIS_OV08F_get_all_OTP error");
		return 0;
	}

	memcpy(data, &ov08f_otp_all_data[addr], sizeof(u8) * size);
	ret = size;

	EEPROM_PROFILE(&t, "OV08F_OTP_read_time");

	return ret;
}
#endif