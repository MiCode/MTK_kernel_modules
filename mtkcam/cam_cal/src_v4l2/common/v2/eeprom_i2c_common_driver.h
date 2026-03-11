/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __EEPROM_I2C_COMMON_DRIVER_H
#define __EEPROM_I2C_COMMON_DRIVER_H
#include <linux/i2c.h>

#define EEPROM_SUB_MODULE_MAX 16
struct Segment {
	int start;
	int end;
};
struct SubModuleCheckSum
{
	const char *subModuleName;
	struct Segment segments[3];
	int checkSumAddr;
};

struct EepromCheckSum
{
	const char* productName;
	unsigned int vendorId;
	unsigned int roleId;
	struct SubModuleCheckSum subModule[EEPROM_SUB_MODULE_MAX];
};

static const struct EepromCheckSum CheckSumList[] = {
	{
		"klee", 0x10, 21, {
			{"BASIC", {{0x0001, 0x0023}, {0, 0}, {0, 0}}, 0x0024},
			{"AWB", {{0x081C, 0x082E}, {0, 0}, {0, 0}}, 0x082F},
			{"LSC1", {{0x0833, 0x095F}, {0x09CC, 0x0D5F}, {0x0DCC, 0x105A}}, 0x105B},
			{"LSC2", {{0x0833, 0x08DD}, {0x094A, 0x0CDD}, {0x0D4A, 0x105A}}, 0x105B},
			{"SN", {{0x106F, 0x107C}, {0, 0}, {0, 0}},0x107D},
			/* subModule end */
			{NULL, {{0, 0}, {0, 0}, {0, 0}},0},
		}
	}
};
static int CheckSumNum = sizeof(CheckSumList) / sizeof(CheckSumList[0]);

static const struct EepromCheckSum DashCheckSumList[] = {
	{
		"dash", 0x10, 21, {
			{"BASIC", {{0x0001, 0x0023}, {0, 0}, {0, 0}}, 0x0024},
			{"AWB", {{0x081C, 0x082E}, {0, 0}, {0, 0}}, 0x082F},
			{"LSC1", {{0x0833, 0x095F}, {0x09CC, 0x0D5F}, {0x0DCC, 0x105A}}, 0x105B},
			{"LSC2", {{0x0833, 0x08DD}, {0x094A, 0x0CDD}, {0x0D4A, 0x105A}}, 0x105B},
			{"SN", {{0x106F, 0x107C}, {0, 0}, {0, 0}},0x107D},
			/* subModule end */
			{NULL, {{0, 0}, {0, 0}, {0, 0}},0},
		}
	}
};
static int DashCheckSumNum = sizeof(DashCheckSumList) / sizeof(DashCheckSumList[0]);

unsigned int Common_read_region(struct i2c_client *client,
				unsigned int addr,
				unsigned char *data,
				unsigned int size);

unsigned int Common_write_region(struct i2c_client *client,
				 unsigned int addr,
				 unsigned char *data,
				 unsigned int size);

#ifdef __XIAOMI_CAMERA__
unsigned int DALI_OV08F_OTP_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size);

unsigned int TURNER_OV08F_OTP_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size);

unsigned int KLEE_SC821CS_OTP_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size);

unsigned int DASH_SC821CS_OTP_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size);

int getoffset(struct i2c_client *client);
int getDashSc821csOffset(struct i2c_client *client);
#endif

#endif				/* __CAM_CAL_LIST_H */
