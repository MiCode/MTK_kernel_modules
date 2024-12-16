/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __TEST_DRV_SDIO_OPS_H__
#define __TEST_DRV_SDIO_OPS_H__

#include <linux/mmc/core.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>


int mmc_send_io_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr);
int mmc_io_rw_direct(struct mmc_card *card, int write, unsigned fn,
	unsigned addr, u8 in, u8 *out);
int mmc_io_rw_extended(struct mmc_card *card, int write, unsigned fn,
	unsigned addr, int incr_addr, u8 *buf, unsigned blocks, unsigned blksz);

int sdio_disable_wide(struct mmc_card *card);
int sdio_enable_4bit_bus(struct mmc_card *card);



#include <linux/delay.h>
#include <linux/sched.h>
#define MMC_CMD_RETRIES        3

static inline void mmc_delay(unsigned int ms)
{
	if (ms < 1000 / HZ) {
		cond_resched();
		mdelay(ms);
	} else {
		msleep(ms);
	}
}

#endif
