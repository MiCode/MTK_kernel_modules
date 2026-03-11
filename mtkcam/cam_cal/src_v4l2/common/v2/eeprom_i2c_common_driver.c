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
#include "eeprom_i2c_common_driver.h"
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/pid.h>
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
#define OTP_CRASH_SUPPORT 1
static int Read_I2C_CAM_CAL(struct i2c_client *client,
			    u16 a_u2Addr,
			    u32 ui4_length,
			    u8 *a_puBuff)
{
	int i4RetValue = 0;
	char puReadCmd[2] = { (char)(a_u2Addr >> 8), (char)(a_u2Addr & 0xFF) };
	struct i2c_msg msg[EEPROM_I2C_MSG_SIZE_READ];

	if (client == NULL) {
		return -1;
	}

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

	if (client == NULL) {
		return -1;
	}

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

static bool Read_I2C_U16_CAM_CAL(struct i2c_client *client,
				u16 a_u2Addr,
				u32 ui4_length,
				u8 *a_puBuff)
{
	int i4RetValue = 0;
	u8 puReadCmd[2] = {(u8)(a_u2Addr >> 8), (u8)(a_u2Addr & 0xFF)};
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
	msg[0].len = 2;
	msg[0].buf = puReadCmd;

	msg[1].addr = client->addr;
	msg[1].flags = 1;
	msg[1].len = ui4_length;
	msg[1].buf = a_puBuff;

	i4RetValue = i2c_transfer(client->adapter, msg, EEPROM_I2C_MSG_SIZE_READ);

	if (i4RetValue < 0) {
		must_log("I2C read data failed %d!! addr=0x%x, reg=0x%x\n", i4RetValue, client->addr, a_u2Addr);
		return false;
	}

	return true;
}

static int Write_I2C_U16_CAM_CAL(struct i2c_client *client,
				u16 a_u2Addr,
				u8 val)
{
	int i4RetValue = 0;
	struct i2c_msg msg;

	if (client == NULL) {
		return -EINVAL;
	}

	u8 puCmd[3] = {
		(u8)(a_u2Addr >> 8),
		(u8)(a_u2Addr & 0xFF),
		val
	};

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.len = 3;
	msg.buf = puCmd;

	i4RetValue = i2c_transfer(client->adapter, &msg, 1);

	if (i4RetValue != 1) {
		must_log("I2C write data failed!! addr=0x%x, reg=0x%x\n", client->addr, a_u2Addr);
		return -EIO;
	}
	return 0;
}

struct OV08F_OTP_DATA
{
#define MAX_BLOCK_NUM 64
	u8 block_num;
	u8 block_reg_addr[MAX_BLOCK_NUM];
	u8 block_start_addr[MAX_BLOCK_NUM];
	u8 block_size[MAX_BLOCK_NUM];
};

struct SC821CS_OTP_DATA {
	u8 block_num;
	u8 block_bank_num[9];
	u16 block_offset_in_window[9];
	u16 block_size[9];
};

static bool SC821CS_eeprom_addr_to_otp_addr(u16 addr_in, u16 size, struct SC821CS_OTP_DATA *otp_data) {
	const u16 OTP_BASE = 0x8000;
	const u16 OTP_BANK_SIZE = 0x400;
	const u8 MAX_BANK_NUM = 9;
	u16 offset = 0;
	u16 remaining = size;

	if (addr_in < OTP_BASE || (addr_in + size) > (OTP_BASE + OTP_BANK_SIZE * MAX_BANK_NUM)) {
		must_log("Invalid address range: 0x%x-0x%x (OTP: 0x%x-0x%x)",
				addr_in, addr_in + size - 1, OTP_BASE, OTP_BASE + OTP_BANK_SIZE * MAX_BANK_NUM - 1);
		return false;
	}

	otp_data->block_num = 0;

	while (remaining > 0) {
		if (otp_data->block_num >= ARRAY_SIZE(otp_data->block_bank_num)) {
			must_log("Block_num exceeds array size: %d", otp_data->block_num);
			return false;
		}

		u16 current_addr = addr_in + offset;
		u16 bank_num = ((current_addr - OTP_BASE) / OTP_BANK_SIZE) + 1;
		u16 offset_in_window = (current_addr - OTP_BASE) % OTP_BANK_SIZE;

		u16 bank_remain = OTP_BANK_SIZE - offset_in_window;
		u16 current_size = min(remaining, bank_remain);

		if (bank_num > MAX_BANK_NUM) {
			must_log("bank_num %d exceeds hardware limit", bank_num);
			return false;
		}

		otp_data->block_bank_num[otp_data->block_num] = bank_num;
		otp_data->block_offset_in_window[otp_data->block_num] = offset_in_window;
		otp_data->block_size[otp_data->block_num] = current_size;

		otp_data->block_num++;
		remaining -= current_size;
		offset += current_size;
	}

	return true;
}
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
			ov08f_otp_data->block_num += 1;
			u8 index = ov08f_otp_data->block_num - 1;
			ov08f_otp_data->block_reg_addr[index] = ov08f_otp_data->block_reg_addr[0] + index;
			ov08f_otp_data->block_start_addr[index] = 0x00;
			ov08f_otp_data->block_size[index] = (size_in - ov08f_otp_data->block_size[0])%0x80;
		}
		return true;
	}
}

unsigned int DALI_OV08F_OTP_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	unsigned int ret = 0;
	struct timespec64 t;

	EEPROM_PROFILE_INIT(&t);

	//read otp init
	u32 offset = 0;
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
	Write_I2C_U8_CAM_CAL(client, 0xa9, 0x04);
	Write_I2C_U8_CAM_CAL(client, 0xfd, 0x09);

	u8 flag1[2], flag2[2];
	Read_I2C_U8_CAM_CAL(client, 0x00, 2, flag1);
	Read_I2C_U8_CAM_CAL(client, 0x02, 2, flag2);

	if(flag1[0] == 0xff && flag1[1] == 0xff && flag2[0] == 0 && flag2[1] == 0){
		offset = 0;    //group 0
	} else if(flag1[0] == 0xff && flag1[1] == 0xff && flag2[0] == 0xff && flag2[1] == 0xff){
		offset = 3838; //group 1
	} else{
		must_log("ov08f read group flag error: flag1 %d/%d, flag2 %d/%d\n", flag1[0], flag1[1], flag2[0], flag2[1]);
		return ret;
	}
	must_log("addr 0x%x, offset %d, size %d, flag: %d/%d/%d/%d", addr, offset, size, flag1[0], flag1[1], flag2[0], flag2[1]);

	struct OV08F_OTP_DATA ov08f_otp_data;
	unsigned int eeprom_addr;
	if(addr == 0 && size == 0x2000){ /*for dump*/
		eeprom_addr = 0;
	} else {
		eeprom_addr = addr + offset;
	}
	if(OV08F_eeprom_addr_to_otp_addr(eeprom_addr, size, &ov08f_otp_data)){
		for(int i = 0; i < ov08f_otp_data.block_num; i++){
			// must_log("ov08f otp data: %d/%d, 0x%x/0x%x/%d \n",
			// 	ov08f_otp_data.block_num, i,
			// 	ov08f_otp_data.block_reg_addr[i], ov08f_otp_data.block_start_addr[i], ov08f_otp_data.block_size[i]);
			Write_I2C_U8_CAM_CAL(client, 0xfd, 0x03);
			Write_I2C_U8_CAM_CAL(client, 0xa9, ov08f_otp_data.block_reg_addr[i]);
			Write_I2C_U8_CAM_CAL(client, 0xfd, 0x09);
			if(Read_I2C_U8_CAM_CAL(client, ov08f_otp_data.block_start_addr[i], ov08f_otp_data.block_size[i], &data[ret])){
				ret += ov08f_otp_data.block_size[i];
			} else {
				must_log("ov08f read sensor otp error\n");
				return ret;
			}
		}
	};

	EEPROM_PROFILE(&t, "OV08F_OTP_read_time");
	return ret;
}

unsigned int TURNER_OV08F_OTP_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	unsigned int ret = 0;
	struct timespec64 t;

	must_log("TURNER_OV08F_OTP_read_region : client 0x%x, addr 0x%x, size %d", client->addr, addr, size);
	EEPROM_PROFILE_INIT(&t);

	//read otp init
	u32 offset = 0;
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

	u8 flag[2];
	Read_I2C_U8_CAM_CAL(client, 0x23, 2, flag); //Flag位 0x1FA3 0x1FA4

	if(flag[0] == 0x55 && flag[1] == 0x00){
		offset = 0;    //group 1
	} else if(flag[0] == 0xff && flag[1] == 0x55){
		offset = 3793; //group 2
	} else{
		must_log("ov08f read group flag error: Group1 flag 0x%x, Group2 flag 0x%x\n", flag[0], flag[1]);
		return ret;
	}
	must_log("addr 0x%x, offset %d, size %d, flag: %d/%d", addr, offset, size, flag[0], flag[1]);

	struct OV08F_OTP_DATA ov08f_otp_data;
	unsigned int eeprom_addr;
	if(addr == 0 && size == 0x2000){ /*for dump*/
		eeprom_addr = 0;
	} else {
		eeprom_addr = addr + offset;
	}
	if(OV08F_eeprom_addr_to_otp_addr(eeprom_addr, size, &ov08f_otp_data)){
		for(int i = 0; i < ov08f_otp_data.block_num; i++){
			// must_log("ov08f otp data: %d/%d, 0x%x/0x%x/%d \n",
			// 	ov08f_otp_data.block_num, i,
			// 	ov08f_otp_data.block_reg_addr[i], ov08f_otp_data.block_start_addr[i], ov08f_otp_data.block_size[i]);
			Write_I2C_U8_CAM_CAL(client, 0xfd, 0x03);
			Write_I2C_U8_CAM_CAL(client, 0xa9, ov08f_otp_data.block_reg_addr[i]);
			Write_I2C_U8_CAM_CAL(client, 0xfd, 0x09);
			if(Read_I2C_U8_CAM_CAL(client, ov08f_otp_data.block_start_addr[i], ov08f_otp_data.block_size[i], &data[ret])){
				ret += ov08f_otp_data.block_size[i];
			} else {
				must_log("ov08f read sensor otp error\n");
				return ret;
			}
		}
	};

	EEPROM_PROFILE(&t, "OV08F_OTP_read_time");

	return ret;
}
#endif

static int sc821cs_set_threshold(struct i2c_client *client, u8 threshold) //set thereshold
{
	int threshold_reg1[3] = { 0x78, 0x78, 0x78 };
	int threshold_reg2[3] = { 0x02, 0x02, 0x02 };
	int threshold_reg3[3] = { 0x42, 0x71, 0x03 };

	if (threshold < 3 && threshold >= 0) {
		Write_I2C_U16_CAM_CAL(client, 0x36b0, threshold_reg1[threshold]);
		Write_I2C_U16_CAM_CAL(client, 0x36b1, threshold_reg2[threshold]);
		Write_I2C_U16_CAM_CAL(client, 0x36b2, threshold_reg3[threshold]);
		must_log("sc821cs_otp set_threshold %d\n", threshold);
	} else {
		must_log("sc821cs_otp set invalid threshold %d\n", threshold);
	}
	return 0;
}
static int sc821cs_set_page_and_load_data(struct i2c_client *client, int page_num) //set page
{
	int delay = 0;
	u8 reg_page_val;
	if (page_num < 1 || page_num > 9) {
        must_log("Invalid page number: %d\n", page_num);
        return -EINVAL;
	}
	reg_page_val = (u8)(2 * page_num - 1);
	Write_I2C_U16_CAM_CAL(client, 0x4408, 0x00);
	Write_I2C_U16_CAM_CAL(client, 0x4409, 0x00);
	Write_I2C_U16_CAM_CAL(client, 0x448a, 0x03);
	Write_I2C_U16_CAM_CAL(client, 0x448b, 0xff);

	Write_I2C_U16_CAM_CAL(client, 0x4401, 0x15); // address set finished
	Write_I2C_U16_CAM_CAL(client, 0x4412, reg_page_val); // set page
	if (page_num == 1) {
		Write_I2C_U16_CAM_CAL(client, 0x4407, 0x11); // set page
	}else {
		Write_I2C_U16_CAM_CAL(client, 0x4407, 0x00); // set page
	}
	Write_I2C_U16_CAM_CAL(client, 0x4402, 0x01);
	Write_I2C_U16_CAM_CAL(client, 0x4403, 0x03);
	Write_I2C_U16_CAM_CAL(client, 0x4404, 0x09);
	Write_I2C_U16_CAM_CAL(client, 0x4405, 0x0b);
	Write_I2C_U16_CAM_CAL(client, 0x440c, 0x0e);
	Write_I2C_U16_CAM_CAL(client, 0x440d, 0x0e);
	Write_I2C_U16_CAM_CAL(client, 0x440e, 0x0b);
	Write_I2C_U16_CAM_CAL(client, 0x0100, 0x00);
	Write_I2C_U16_CAM_CAL(client, 0x4400, 0x11); // manual load begin
	mdelay(15);
	unsigned char load_flag = 0xFF;
	Read_I2C_U16_CAM_CAL(client, 0x4420, 1, &load_flag);
	while ((load_flag & 0x01) == 0x01) {
		delay++;
		must_log("sc821cs_otp set_page waitting, OTP is still busy for loading %d times\n", delay);
		mdelay(10);
		if (delay == 5) {
			must_log("sc821cs_otp set_page fail, load timeout!!!\n");
			return false;
		}
	}
	must_log("sc821cs_otp set_page success\n");
	return true;
}
static bool CheckSum(unsigned char* data, const struct SubModuleCheckSum *subModule, int checkSumPos)
{
	int sum = 0;
	bool result = false;
		for (int s = 0; s < 3; s++) {
			struct Segment seg = subModule->segments[s];
	        if (seg.start >= seg.end) {
	            continue;
	        }
	        for (int i = seg.start; i <= seg.end; i++) {
				sum += data[i];
			}
		}

	unsigned int checkSumValue = data[checkSumPos];
	if (checkSumValue == (sum % 255 + 1)){
		result = true;
	}
	return result;
}

static bool CheckSumResult(unsigned char* data, int offset)
{
	bool result = false;
	for (int i = 0; i < CheckSumNum; i++) {
		for (int j = 0; j < EEPROM_SUB_MODULE_MAX; j++) {
			if (CheckSumList[i].subModule[j].subModuleName == NULL) {
				break;
			}
			if (strcmp(CheckSumList[i].subModule[j].subModuleName, "LSC1") == 0 && offset == 4226) {
				continue;
			}
			if (strcmp(CheckSumList[i].subModule[j].subModuleName, "LSC2") == 0 && offset != 4226) {
				continue;
			}
			result = CheckSum(data, &CheckSumList[i].subModule[j],
						CheckSumList[i].subModule[j].checkSumAddr);
			if (!result) {
				must_log("roleId(%d), submodule(%s) checksum fail !", CheckSumList[i].roleId, CheckSumList[i].subModule[j].subModuleName);
				return result;
			} else {
				must_log("roleId(%d), submodule(%s) checksum pass !", CheckSumList[i].roleId, CheckSumList[i].subModule[j].subModuleName);
			}
		}
	}
	return result;
}
int getoffset(struct i2c_client *client) {
	unsigned char flag_group2;
	int offset = 0;

	sc821cs_set_page_and_load_data(client, 5);

	if (!Read_I2C_U16_CAM_CAL(client, 0x931D, 1, &flag_group2)) {
		must_log("Failed to read flag_group2 from 0x931D\n");
		return 0;
	}

	if (flag_group2 == 0x01) {
		offset = 4226;
	} else {
		offset = 0;
	}

	return offset;
}
unsigned int KLEE_SC821CS_OTP_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	unsigned int ret = 0;
	struct timespec64 t;
	int threshold = 0;
	EEPROM_PROFILE_INIT(&t);
	//read otp init
	struct SC821CS_OTP_DATA otp_data;
	otp_data.block_num = 0;
	bool checksum_passed = false;
	u32 offset = 0;
	for (threshold = 0; threshold < ((size == 4224) ? 3 : 1); threshold++) {
		sc821cs_set_threshold(client, threshold);
		ret = 0;
		offset = getoffset(client);
		must_log("cam_cal addr_in: 0x%x size: /%d  offset %d\n", addr, size, offset);
		if(SC821CS_eeprom_addr_to_otp_addr(addr+offset, size, &otp_data)){
			for(int i = 0; i < otp_data.block_num; i++){
				must_log("cam_cal sc821cs otp data: %d/%d, 0x%08x 0x%x %d/\n",
				otp_data.block_num, i,otp_data.block_offset_in_window[i],otp_data.block_bank_num[i],otp_data.block_size[i]);
				int page_num = otp_data.block_bank_num[i];
				sc821cs_set_page_and_load_data(client, page_num);
				u16 read_addr_in_window = 0x8000 + otp_data.block_offset_in_window[i];  // 0x8000 + offset_in_window
				if(!Read_I2C_U16_CAM_CAL(client, read_addr_in_window, otp_data.block_size[i], &data[ret])){
					must_log("sc821cs read sensor otp error\n");
					return ret;
				}
				ret += otp_data.block_size[i];
			}
		}
		if(size == 4224){
			checksum_passed = CheckSumResult(data, offset);
			if(checksum_passed == true){
				must_log("Checksum passed, proceed with data.");
				break;
			}else{
				memset(data, 0, size);
				ret = 0;
				must_log("Checksum failed, retrying with next threshold...\n");
			}
		}else{
			break;
		}
	}

	if (size == 4224 && !checksum_passed) {
		#if OTP_CRASH_SUPPORT
			struct pid *pid_struct = get_task_pid(current, PIDTYPE_PID);
			kill_pid(pid_struct, SIGSEGV, 1);
			put_pid(pid_struct);
		#endif
		must_log("All thresholds tried, checksum failed\n");
		return ret;
	}
	EEPROM_PROFILE(&t, "SC821CS_OTP_read_time");
	return ret;
}

int getDashSc821csOffset(struct i2c_client *client) {
	unsigned char flag_group2;
	int offset = 0;

	sc821cs_set_page_and_load_data(client, 5);

	if (!Read_I2C_U16_CAM_CAL(client, 0x931D, 1, &flag_group2)) {
		must_log("Failed to read flag_group2 from 0x931D\n");
		return 0;
	}

	if (flag_group2 == 0x01) {
		offset = 4226;
	} else {
		offset = 0;
	}

	return offset;
}

static bool DashCheckSumResult(unsigned char* data, int offset)
{
	bool result = false;
	for (int i = 0; i < DashCheckSumNum; i++) {
		for (int j = 0; j < EEPROM_SUB_MODULE_MAX; j++) {
			if (DashCheckSumList[i].subModule[j].subModuleName == NULL) {
				break;
			}
			if (strcmp(DashCheckSumList[i].subModule[j].subModuleName, "LSC1") == 0 && offset == 4226) {
				continue;
			}
			if (strcmp(DashCheckSumList[i].subModule[j].subModuleName, "LSC2") == 0 && offset != 4226) {
				continue;
			}
			result = CheckSum(data, &DashCheckSumList[i].subModule[j],
				DashCheckSumList[i].subModule[j].checkSumAddr);
			if (!result) {
				must_log("roleId(%d), submodule(%s) checksum fail !", DashCheckSumList[i].roleId, DashCheckSumList[i].subModule[j].subModuleName);
				return result;
			} else {
				must_log("roleId(%d), submodule(%s) checksum pass !", DashCheckSumList[i].roleId, DashCheckSumList[i].subModule[j].subModuleName);
			}
		}
	}
	return result;
}

unsigned int DASH_SC821CS_OTP_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	unsigned int ret = 0;
	struct timespec64 t;
	int threshold = 0;
	EEPROM_PROFILE_INIT(&t);
	//read otp init
	struct SC821CS_OTP_DATA otp_data;
	otp_data.block_num = 0;
	bool checksum_passed = false;
	u32 offset = 0;
	for (threshold = 0; threshold < ((size == 4224) ? 3 : 1); threshold++) {
		sc821cs_set_threshold(client, threshold);
		ret = 0;
		offset = getDashSc821csOffset(client);
		must_log("cam_cal addr_in: 0x%x size: /%d  offset %d\n", addr, size, offset);
		if(SC821CS_eeprom_addr_to_otp_addr(addr+offset, size, &otp_data)){
			for(int i = 0; i < otp_data.block_num; i++){
				must_log("cam_cal sc821cs otp data: %d/%d, 0x%08x 0x%x %d/\n",
				otp_data.block_num, i,otp_data.block_offset_in_window[i],otp_data.block_bank_num[i],otp_data.block_size[i]);
				int page_num = otp_data.block_bank_num[i];
				sc821cs_set_page_and_load_data(client, page_num);
				u16 read_addr_in_window = 0x8000 + otp_data.block_offset_in_window[i];  // 0x8000 + offset_in_window
				if(!Read_I2C_U16_CAM_CAL(client, read_addr_in_window, otp_data.block_size[i], &data[ret])){
					must_log("sc821cs read sensor otp error\n");
					return ret;
				}
				ret += otp_data.block_size[i];
			}
		}
		if(size == 4224){
			checksum_passed = DashCheckSumResult(data, offset);
			if(checksum_passed == true){
				must_log("Checksum passed, proceed with data.");
				break;
			}else{
				memset(data, 0, size);
				ret = 0;
				must_log("Checksum failed, retrying with next threshold...\n");
			}
		}else{
			break;
		}
	}

	if (size == 4224 && !checksum_passed) {
		#if OTP_CRASH_SUPPORT
			struct pid *pid_struct = get_task_pid(current, PIDTYPE_PID);
			kill_pid(pid_struct, SIGSEGV, 1);
			put_pid(pid_struct);
		#endif
		must_log("All thresholds tried, checksum failed\n");
		return ret;
	}
	EEPROM_PROFILE(&t, "SC821CS_OTP_read_time");
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

