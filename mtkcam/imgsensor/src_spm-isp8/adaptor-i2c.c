// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 MediaTek Inc.

#include <linux/i2c.h>
#include <linux/slab.h>
#include "mtk-i3c-i2c-wrap.h"

#include "adaptor-i2c.h"

#define MAX_BUF_SIZE 255
#define MAX_MSG_NUM_U8 (MAX_BUF_SIZE / 3)
#define MAX_MSG_NUM_U16 (MAX_BUF_SIZE / 4)
#define MAX_VAL_NUM_U8 (MAX_BUF_SIZE - 2)
#define MAX_VAL_NUM_U16 ((MAX_BUF_SIZE - 2) >> 1)

struct cache_wr_regs_u8 {
	u8 buf[MAX_BUF_SIZE];
	struct i2c_msg msg[MAX_MSG_NUM_U8];
};

struct cache_wr_regs_u16 {
	u8 buf[MAX_BUF_SIZE];
	struct i2c_msg msg[MAX_MSG_NUM_U16];
};

struct cache_wr_regs_u8_ixc {
	u8 buf[MAX_BUF_SIZE];
	struct i3c_i2c_xfer msg[MAX_MSG_NUM_U8];
};

struct cache_wr_regs_u16_ixc {
	u8 buf[MAX_BUF_SIZE];
	struct i3c_i2c_xfer msg[MAX_MSG_NUM_U16];
};

struct device *adaptor_ixc_get_dev (struct i3c_i2c_device *client)
{
	struct device *dev = NULL;

	if (client == NULL)
		return NULL;
	if (client->protocol == I3C_PROTOCOL)
		dev = &client->i3c_dev->dev;
	else if (client->protocol == I2C_PROTOCOL)
		dev = &client->i2c_dev->dev;

	return dev;
}

void *adaptor_ixc_get_clientdata(struct i3c_i2c_device *client)
{
	struct device *dev = adaptor_ixc_get_dev(client);

	if (dev == NULL)
		return NULL;
	return dev_get_drvdata(dev);
}

int adaptor_ixc_do_daa (struct i3c_i2c_device *client)
{
	int ret = 0;

	if (client == NULL)
		return -ENODEV;

	ret = mtk_i3c_i2c_device_do_daa(client);
	// if do daa error, retry it
	if (ret)
		ret = mtk_i3c_i2c_device_do_daa(client);

	return ret;
}

int adaptor_i2c_rd_u8(struct i2c_client *i2c_client,
		u16 addr, u16 reg, u8 *val)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msg[2];

	if (i2c_client == NULL)
		return -ENODEV;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg[0].addr = addr;
	msg[0].flags = i2c_client->flags;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].addr = addr;
	msg[1].flags = i2c_client->flags | I2C_M_RD;
	msg[1].buf = buf;
	msg[1].len = 1;

	ret = i2c_transfer(i2c_client->adapter, msg, 2);
	if (ret < 0) {
		dev_info(&i2c_client->dev, "i2c transfer failed (%d)\n", ret);
		return ret;
	}

	*val = buf[0];

	return 0;
}

int adaptor_i2c_rd_u16(struct i2c_client *i2c_client,
		u16 addr, u16 reg, u16 *val)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msg[2];

	if (i2c_client == NULL)
		return -ENODEV;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg[0].addr = addr;
	msg[0].flags = i2c_client->flags;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].addr  = addr;
	msg[1].flags = i2c_client->flags | I2C_M_RD;
	msg[1].buf = buf;
	msg[1].len = 2;

	ret = i2c_transfer(i2c_client->adapter, msg, 2);
	if (ret < 0) {
		dev_info(&i2c_client->dev, "i2c transfer failed (%d)\n", ret);
		return ret;
	}

	*val = ((u16)buf[0] << 8) | buf[1];

	return 0;
}

int adaptor_i2c_rd_p8(struct i2c_client *i2c_client,
		u16 addr, u16 reg, u8 *p_vals, u32 n_vals)
{
	int ret, cnt, total, recv, reg_b;
	u8 buf[2];
	struct i2c_msg msg[2];
	u8 *pbuf;

	if (i2c_client == NULL)
		return -ENODEV;

	recv = 0;
	total = n_vals;
	pbuf = p_vals;
	reg_b = reg;

	msg[0].addr = addr;
	msg[0].flags = i2c_client->flags;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].addr = addr;
	msg[1].flags = i2c_client->flags | I2C_M_RD;

	while (recv < total) {

		cnt = total - recv;
		if (cnt > MAX_VAL_NUM_U8)
			cnt = MAX_VAL_NUM_U8;

		buf[0] = reg_b >> 8;
		buf[1] = reg_b & 0xff;

		msg[1].buf = pbuf;
		msg[1].len = cnt;

		ret = i2c_transfer(i2c_client->adapter, msg, 2);
		if (ret < 0) {
			dev_info(&i2c_client->dev,
				"i2c transfer failed (%d)\n", ret);
			return -EIO;
		}

		pbuf += cnt;
		recv += cnt;
		reg_b += cnt;
	}

	return ret;
}

int adaptor_i2c_wr_u8(struct i2c_client *i2c_client,
		u16 addr, u16 reg, u8 val)
{
	int ret;
	u8 buf[3];
	struct i2c_msg msg;

	if (i2c_client == NULL)
		return -ENODEV;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = val;

	msg.addr = addr;
	msg.flags = i2c_client->flags;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i2c_transfer(i2c_client->adapter, &msg, 1);
	if (ret < 0)
		dev_info(&i2c_client->dev, "i2c transfer failed (%d)\n", ret);

	return ret;
}

int adaptor_i2c_wr_u16(struct i2c_client *i2c_client,
		u16 addr, u16 reg, u16 val)
{
	int ret;
	u8 buf[4];
	struct i2c_msg msg;

	if (i2c_client == NULL)
		return -ENODEV;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = val >> 8;
	buf[3] = val & 0xff;

	msg.addr = addr;
	msg.flags = i2c_client->flags;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i2c_transfer(i2c_client->adapter, &msg, 1);
	if (ret < 0)
		dev_info(&i2c_client->dev, "i2c transfer failed (%d)\n", ret);

	return ret;
}

int adaptor_i2c_wr_p8(struct i2c_client *i2c_client,
		u16 addr, u16 reg, u8 *p_vals, u32 n_vals)
{
	u8 *buf, *pbuf, *pdata;
	struct i2c_msg msg;
	int ret, sent, total, cnt;

	if (i2c_client == NULL)
		return -ENODEV;

	buf = kmalloc(MAX_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	sent = 0;
	total = n_vals;
	pdata = p_vals;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg.addr = addr;
	msg.flags = i2c_client->flags;
	msg.buf = buf;

	while (sent < total) {

		cnt = total - sent;
		if (cnt > MAX_VAL_NUM_U8)
			cnt = MAX_VAL_NUM_U8;

		pbuf = buf + 2;
		memcpy(pbuf, pdata, cnt);

		msg.len = 2 + cnt;

		ret = i2c_transfer(i2c_client->adapter, &msg, 1);
		if (ret < 0) {
			dev_info(&i2c_client->dev,
				"i2c transfer failed (%d)\n", ret);
			kfree(buf);
			return -EIO;
		}

		sent += cnt;
		pdata += cnt;
	}

	kfree(buf);

	return 0;
}

int adaptor_i2c_wr_p16(struct i2c_client *i2c_client,
		u16 addr, u16 reg, u16 *p_vals, u32 n_vals)
{
	u8 *buf, *pbuf;
	u16 *pdata;
	struct i2c_msg msg;
	int i, ret, sent, total, cnt;

	if (i2c_client == NULL)
		return -ENODEV;

	buf = kmalloc(MAX_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	sent = 0;
	total = n_vals;
	pdata = p_vals;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg.addr = addr;
	msg.flags = i2c_client->flags;
	msg.buf = buf;

	while (sent < total) {

		cnt = total - sent;
		if (cnt > MAX_VAL_NUM_U16)
			cnt = MAX_VAL_NUM_U16;

		pbuf = buf + 2;

		for (i = 0; i < cnt; i++) {
			pbuf[0] = pdata[0] >> 8;
			pbuf[1] = pdata[0] & 0xff;
			pdata++;
			pbuf += 2;
		}

		msg.len = 2 + (cnt << 1);

		ret = i2c_transfer(i2c_client->adapter, &msg, 1);
		if (ret < 0) {
			dev_info(&i2c_client->dev,
				"i2c transfer failed (%d)\n", ret);
			kfree(buf);
			return -EIO;
		}

		sent += cnt;
	}

	kfree(buf);

	return 0;
}

int adaptor_i2c_wr_seq_p8(struct i2c_client *i2c_client,
		u16 addr, u16 reg, u8 *p_vals, u32 n_vals)
{
	u8 *buf, *pbuf, *pdata;
	struct i2c_msg msg;
	int ret, sent, total, cnt;

	if (i2c_client == NULL)
		return -ENODEV;

	buf = kmalloc(MAX_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	sent = 0;
	total = n_vals;
	pdata = p_vals;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg.addr = addr;
	msg.flags = i2c_client->flags;
	msg.buf = buf;

	while (sent < total) {

		cnt = total - sent;
		if (cnt > MAX_VAL_NUM_U8)
			cnt = MAX_VAL_NUM_U8;

		buf[0] = reg >> 8;
		buf[1] = reg & 0xff;

		pbuf = buf + 2;
		memcpy(pbuf, pdata, cnt);

		msg.len = 2 + cnt;

		ret = i2c_transfer(i2c_client->adapter, &msg, 1);
		if (ret < 0) {
			dev_info(&i2c_client->dev,
				"i2c transfer failed (%d)\n", ret);
			kfree(buf);
			return -EIO;
		}

		sent += cnt;
		pdata += cnt;
		reg += cnt;
	}

	kfree(buf);

	return 0;
}

int adaptor_i2c_wr_regs_u8(struct i2c_client *i2c_client,
		u16 addr, u16 *list, u32 len)
{
	struct cache_wr_regs_u8 *pmem;
	struct i2c_msg *pmsg;
	u8 *pbuf;
	u16 *plist;
	int i, ret, sent, total, cnt;

	if (i2c_client == NULL)
		return -ENODEV;

	pmem = kmalloc(sizeof(*pmem), GFP_KERNEL);
	if (!pmem)
		return -ENOMEM;

	/* each msg contains 3 bytes: addr(u16) + val(u8) */
	sent = 0;
	total = len >> 1;
	plist = list;

	while (sent < total) {

		cnt = total - sent;
		if (cnt > ARRAY_SIZE(pmem->msg))
			cnt = ARRAY_SIZE(pmem->msg);

		pbuf = pmem->buf;
		pmsg = pmem->msg;

		for (i = 0; i < cnt; i++) {

			pbuf[0] = plist[0] >> 8;
			pbuf[1] = plist[0] & 0xff;
			pbuf[2] = plist[1] & 0xff;

			pmsg->addr = addr;
			pmsg->flags = i2c_client->flags;
			pmsg->len = 3;
			pmsg->buf = pbuf;

			plist += 2;
			pbuf += 3;
			pmsg++;
		}

		ret = i2c_transfer(i2c_client->adapter, pmem->msg, cnt);
		if (ret != cnt) {
			dev_info(&i2c_client->dev,
				"i2c transfer failed (%d)\n", ret);
			kfree(pmem);
			return -EIO;
		}

		sent += cnt;
	}

	kfree(pmem);

	return 0;
}

int adaptor_i2c_wr_regs_u16(struct i2c_client *i2c_client,
		u16 addr, u16 *list, u32 len)
{
	struct cache_wr_regs_u16 *pmem;
	struct i2c_msg *pmsg;
	u8 *pbuf;
	u16 *plist;
	int i, ret, sent, total, cnt;

	if (i2c_client == NULL)
		return -ENODEV;

	pmem = kmalloc(sizeof(*pmem), GFP_KERNEL);
	if (!pmem)
		return -ENOMEM;

	/* each msg contains 4 bytes: addr(u16) + val(u16) */
	sent = 0;
	total = len >> 1;
	plist = list;

	while (sent < total) {

		cnt = total - sent;
		if (cnt > ARRAY_SIZE(pmem->msg))
			cnt = ARRAY_SIZE(pmem->msg);

		pbuf = pmem->buf;
		pmsg = pmem->msg;

		for (i = 0; i < cnt; i++) {

			pbuf[0] = plist[0] >> 8;
			pbuf[1] = plist[0] & 0xff;
			pbuf[2] = plist[1] >> 8;
			pbuf[3] = plist[1] & 0xff;

			pmsg->addr = addr;
			pmsg->flags = i2c_client->flags;
			pmsg->len = 4;
			pmsg->buf = pbuf;

			plist += 2;
			pbuf += 4;
			pmsg++;
		}

		ret = i2c_transfer(i2c_client->adapter, pmem->msg, cnt);

		if (ret != cnt) {
			dev_info(&i2c_client->dev,
				"i2c transfer failed (%d)\n", ret);
			kfree(pmem);
			return -EIO;
		}

		sent += cnt;
	}

	kfree(pmem);

	return 0;
}

int adaptor_ixc_rd_u8(struct i3c_i2c_device *client,
		u16 addr, u16 reg, u8 *val)
{
	int ret;
	u8 buf[2];
	struct i3c_i2c_xfer msg[2];
	struct device *dev = adaptor_ixc_get_dev(client);

	if (client == NULL)
		return -ENODEV;

	if (client->protocol == I2C_PROTOCOL) {
		msg[0].addr = addr;
		msg[1].addr = addr;
	}

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg[0].flags = 0;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].flags = 1;
	msg[1].buf = buf;
	msg[1].len = 1;

	ret = i3c_i2c_transfer(client, msg, 2);
	if (ret < 0) {
		dev_info(dev, "[%s]ixc transfer failed (%d)\n", __func__,ret);
		return ret;
	}

	*val = buf[0];

	return 0;
}

int adaptor_ixc_rd_u16(struct i3c_i2c_device *client,
		u16 addr, u16 reg, u16 *val)
{
	int ret;
	u8 buf[2];
	struct i3c_i2c_xfer msg[2];
	struct device *dev = adaptor_ixc_get_dev(client);

	if (client == NULL)
		return -ENODEV;

	if (client->protocol == I2C_PROTOCOL) {
		msg[0].addr = addr;
		msg[1].addr = addr;
	}

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg[0].flags = 0;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].flags = 1;
	msg[1].buf = buf;
	msg[1].len = 2;

	ret = i3c_i2c_transfer(client, msg, 2);
	if (ret < 0) {
		dev_info(dev, "[%s]ixc transfer failed (%d)\n", __func__,ret);
		return ret;
	}

	*val = ((u16)buf[0] << 8) | buf[1];

	return 0;
}

int adaptor_ixc_rd_p8(struct i3c_i2c_device *client,
		u16 addr, u16 reg, u8 *p_vals, u32 n_vals)
{
	int ret, cnt, total, recv, reg_b;
	u8 buf[2];
	struct i3c_i2c_xfer msg[2];
	struct device *dev = adaptor_ixc_get_dev(client);
	u8 *pbuf;

	if (client == NULL)
		return -ENODEV;

	if (client->protocol == I2C_PROTOCOL) {
		msg[0].addr = addr;
		msg[1].addr = addr;
	}

	recv = 0;
	total = n_vals;
	pbuf = p_vals;
	reg_b = reg;

	msg[0].flags = 0;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].flags = 1;

	while (recv < total) {

		cnt = total - recv;
		if (cnt > MAX_VAL_NUM_U8)
			cnt = MAX_VAL_NUM_U8;

		buf[0] = reg_b >> 8;
		buf[1] = reg_b & 0xff;

		msg[1].buf = pbuf;
		msg[1].len = cnt;

		ret = i3c_i2c_transfer(client, msg, 2);
		if (ret < 0) {
			dev_info(dev, "[%s]ixc transfer failed (%d)\n", __func__,ret);
			return -EIO;
		}

		pbuf += cnt;
		recv += cnt;
		reg_b += cnt;
	}

	return ret;
}

int adaptor_ixc_wr_u8(struct i3c_i2c_device *client,
		u16 addr, u16 reg, u8 val)
{
	int ret;
	u8 buf[3];
	struct i3c_i2c_xfer msg;
	struct device *dev = adaptor_ixc_get_dev(client);

	if (client == NULL)
		return -ENODEV;

	if (client->protocol == I2C_PROTOCOL)
		msg.addr = addr;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = val;

	msg.flags = 0;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i3c_i2c_transfer(client, &msg, 1);
	if (ret < 0)
		dev_info(dev, "[%s]ixc transfer failed (%d)\n", __func__,ret);

	return ret;
}

int adaptor_ixc_wr_u16(struct i3c_i2c_device *client,
		u16 addr, u16 reg, u16 val)
{
	int ret;
	u8 buf[4];
	struct i3c_i2c_xfer msg;
	struct device *dev = adaptor_ixc_get_dev(client);

	if (client == NULL)
		return -ENODEV;

	if (client->protocol == I2C_PROTOCOL)
		msg.addr = addr;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = val >> 8;
	buf[3] = val & 0xff;

	msg.flags = 0;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i3c_i2c_transfer(client, &msg, 1);
	if (ret < 0)
		dev_info(dev, "[%s]ixc transfer failed (%d)\n", __func__,ret);

	return ret;
}

int adaptor_ixc_wr_p8(struct i3c_i2c_device *client,
		u16 addr, u16 reg, u8 *p_vals, u32 n_vals)
{
	u8 *buf, *pbuf, *pdata;
	struct i3c_i2c_xfer msg;
	struct device *dev = adaptor_ixc_get_dev(client);
	int ret, sent, total, cnt;

	if (client == NULL)
		return -ENODEV;

	if (client->protocol == I2C_PROTOCOL)
		msg.addr = addr;

	buf = kmalloc(MAX_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	sent = 0;
	total = n_vals;
	pdata = p_vals;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg.flags = 0;
	msg.buf = buf;

	while (sent < total) {

		cnt = total - sent;
		if (cnt > MAX_VAL_NUM_U8)
			cnt = MAX_VAL_NUM_U8;

		pbuf = buf + 2;
		memcpy(pbuf, pdata, cnt);

		msg.len = 2 + cnt;

		ret = i3c_i2c_transfer(client, &msg, 1);
		if (ret < 0) {
			dev_info(dev, "[%s]ixc transfer failed (%d)\n", __func__,ret);
			kfree(buf);
			return -EIO;
		}

		sent += cnt;
		pdata += cnt;
	}

	kfree(buf);

	return 0;
}

int adaptor_ixc_wr_p16(struct i3c_i2c_device *client,
		u16 addr, u16 reg, u16 *p_vals, u32 n_vals)
{
	u8 *buf, *pbuf;
	u16 *pdata;
	struct i3c_i2c_xfer msg;
	struct device *dev = adaptor_ixc_get_dev(client);
	int i, ret, sent, total, cnt;

	if (client == NULL)
		return -ENODEV;

	if (client->protocol == I2C_PROTOCOL)
		msg.addr = addr;

	buf = kmalloc(MAX_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	sent = 0;
	total = n_vals;
	pdata = p_vals;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg.flags = 0;
	msg.buf = buf;

	while (sent < total) {

		cnt = total - sent;
		if (cnt > MAX_VAL_NUM_U16)
			cnt = MAX_VAL_NUM_U16;

		pbuf = buf + 2;

		for (i = 0; i < cnt; i++) {
			pbuf[0] = pdata[0] >> 8;
			pbuf[1] = pdata[0] & 0xff;
			pdata++;
			pbuf += 2;
		}

		msg.len = 2 + (cnt << 1);

		ret = i3c_i2c_transfer(client, &msg, 1);
		if (ret < 0) {
			dev_info(dev, "[%s]ixc transfer failed (%d)\n", __func__,ret);
			kfree(buf);
			return -EIO;
		}

		sent += cnt;
	}

	kfree(buf);

	return 0;
}

int adaptor_ixc_wr_seq_p8(struct i3c_i2c_device *client,
		u16 addr, u16 reg, u8 *p_vals, u32 n_vals)
{
	u8 *buf, *pbuf, *pdata;
	struct i3c_i2c_xfer msg;
	struct device *dev = adaptor_ixc_get_dev(client);
	int ret, sent, total, cnt;

	if (client == NULL)
		return -ENODEV;

	if (client->protocol == I2C_PROTOCOL)
		msg.addr = addr;

	buf = kmalloc(MAX_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	sent = 0;
	total = n_vals;
	pdata = p_vals;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg.flags = 0;
	msg.buf = buf;

	while (sent < total) {

		cnt = total - sent;
		if (cnt > MAX_VAL_NUM_U8)
			cnt = MAX_VAL_NUM_U8;

		buf[0] = reg >> 8;
		buf[1] = reg & 0xff;

		pbuf = buf + 2;
		memcpy(pbuf, pdata, cnt);

		msg.len = 2 + cnt;

		ret = i3c_i2c_transfer(client, &msg, 1);
		if (ret < 0) {
			dev_info(dev, "[%s]ixc transfer failed (%d)\n", __func__,ret);
			kfree(buf);
			return -EIO;
		}

		sent += cnt;
		pdata += cnt;
		reg += cnt;
	}

	kfree(buf);

	return 0;
}

int adaptor_ixc_wr_regs_u8(struct i3c_i2c_device *client,
		u16 addr, u16 *list, u32 len)
{
	struct cache_wr_regs_u8_ixc *pmem;
	struct i3c_i2c_xfer *pmsg;
	struct device *dev = adaptor_ixc_get_dev(client);
	u8 *pbuf;
	u16 *plist;
	int i, ret, sent, total, cnt;

	if (client == NULL)
		return -ENODEV;

	pmem = kmalloc(sizeof(*pmem), GFP_KERNEL);
	if (!pmem)
		return -ENOMEM;

	/* each msg contains 3 bytes: addr(u16) + val(u8) */
	sent = 0;
	total = len >> 1;
	plist = list;

	while (sent < total) {

		cnt = total - sent;
		if (cnt > ARRAY_SIZE(pmem->msg))
			cnt = ARRAY_SIZE(pmem->msg);

		pbuf = pmem->buf;
		pmsg = pmem->msg;

		for (i = 0; i < cnt; i++) {

			pbuf[0] = plist[0] >> 8;
			pbuf[1] = plist[0] & 0xff;
			pbuf[2] = plist[1] & 0xff;

			if (client->protocol == I2C_PROTOCOL)
				pmsg->addr = addr;

			pmsg->flags = 0;
			pmsg->len = 3;
			pmsg->buf = pbuf;

			plist += 2;
			pbuf += 3;
			pmsg++;
		}

		ret = i3c_i2c_transfer(client, pmem->msg, cnt);
		if (ret < 0){
			dev_info(dev, "[%s]ixc transfer failed (%d)\n", __func__,ret);
			kfree(pmem);
			return -EIO;
		}

		sent += cnt;
	}

	kfree(pmem);

	return 0;
}

int adaptor_ixc_wr_regs_u16(struct i3c_i2c_device *client,
		u16 addr, u16 *list, u32 len)
{
	struct cache_wr_regs_u16_ixc *pmem;
	struct i3c_i2c_xfer *pmsg;
	struct device *dev = adaptor_ixc_get_dev(client);
	u8 *pbuf;
	u16 *plist;
	int i, ret, sent, total, cnt;

	if (client == NULL)
		return -ENODEV;

	pmem = kmalloc(sizeof(*pmem), GFP_KERNEL);
	if (!pmem)
		return -ENOMEM;

	/* each msg contains 4 bytes: addr(u16) + val(u16) */
	sent = 0;
	total = len >> 1;
	plist = list;

	while (sent < total) {

		cnt = total - sent;
		if (cnt > ARRAY_SIZE(pmem->msg))
			cnt = ARRAY_SIZE(pmem->msg);

		pbuf = pmem->buf;
		pmsg = pmem->msg;

		for (i = 0; i < cnt; i++) {

			pbuf[0] = plist[0] >> 8;
			pbuf[1] = plist[0] & 0xff;
			pbuf[2] = plist[1] >> 8;
			pbuf[3] = plist[1] & 0xff;

			if (client->protocol == I2C_PROTOCOL)
				pmsg->addr = addr;

			pmsg->flags = 0;
			pmsg->len = 4;
			pmsg->buf = pbuf;
			plist += 2;
			pbuf += 4;
			pmsg++;
		}

		ret = i3c_i2c_transfer(client, pmem->msg, cnt);

		if (ret < 0){
			dev_info(dev, "[%s]ixc transfer failed (%d)\n", __func__,ret);
			kfree(pmem);
			return -EIO;
		}

		sent += cnt;
	}

	kfree(pmem);

	return 0;
}
