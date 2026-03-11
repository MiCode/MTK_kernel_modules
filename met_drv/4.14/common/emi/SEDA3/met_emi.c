// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/string.h>

#ifdef CONFIG_ARM
//#include <asm/dma-mapping.h> /* arm_coherent_dma_ops */
#endif /* CONFIG_ARM */

#include <linux/dma-mapping.h>

#define MET_USER_EVENT_SUPPORT
#include "met_drv.h"
#include "trace.h"

#include "mtk_typedefs.h"
#include "core_plf_init.h"
/* #include "plf_trace.h" */
#include "mtk_emi_bm.h"
#include "interface.h"
#include "met_dramc.h"

#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT)
#include "sspm/ondiemet_sspm.h"
#elif defined(TINYSYS_SSPM_SUPPORT)
#include "tinysys_sspm.h"
#include "tinysys_mgr.h" /* for ondiemet_module */
#endif
#endif


/*======================================================================*/
/*	Global variable definitions					*/
/*======================================================================*/
int emi_TP_busfiltr_enable;
int emi_tsct_enable = 1;
int emi_mdct_enable = 1;

static int bw_limiter_enable = BM_BW_LIMITER_ENABLE;

static int msel_enable;
static unsigned int msel_group1 = _GP_1_Default;
static unsigned int msel_group2 = _GP_2_Default;
static unsigned int msel_group3 = _GP_3_Default;

/* Global variables */
static struct kobject *kobj_emi;

static int ttype1_16_en = BM_TTYPE1_16_DISABLE;
static int ttype17_21_en = BM_TTYPE17_21_DISABLE;

static int dramc_pdir_enable = 1;
static int dram_chann_num = 1;

static int times;
static ssize_t test_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf,
				size_t n)
{
	int i;
	unsigned int    *src_addr_v = NULL;
	dma_addr_t src_addr_p;
#ifdef CONFIG_ARM
	struct dma_map_ops *ops = (struct dma_map_ops *)symbol_get(arm_dma_ops);
#endif /* CONFIG_ARM */

	if ((n == 0) || (buf == NULL))
		return -EINVAL;
	if (kstrtoint(buf, 10, &times) != 0)
		return -EINVAL;
	if (times < 0)
		return -EINVAL;

	if (times > 5000)
		return -EINVAL;

#ifdef CONFIG_ARM
	if (ops && ops->alloc) {
		(met_device.this_device)->coherent_dma_mask = DMA_BIT_MASK(32);
		src_addr_v = ops->alloc(met_device.this_device,
						PAGE_SIZE,
						&src_addr_p,
						GFP_KERNEL,
						0);
	}
#endif /* CONFIG_ARM */

#ifdef CONFIG_ARM64
	/* dma_alloc */
	src_addr_v = dma_alloc_coherent(met_device.this_device,
					PAGE_SIZE,
					&src_addr_p,
					GFP_KERNEL);
#endif /* CONFIG_ARM64 */

	if (src_addr_v == NULL) {
#ifdef CONFIG_MET_MODULE
		met_tag_oneshot_real(0, "test dma alloc fail", PAGE_SIZE);
#else
		met_tag_oneshot(0, "test dma alloc fail", PAGE_SIZE);
#endif
		return -ENOMEM;
	}

	preempt_disable();
#ifdef CONFIG_MET_MODULE
	met_tag_start_real(0, "TEST_EMI");
#else
	met_tag_start(0, "TEST_EMI");
#endif
	for (i = 0; i < times; i++) {
		memset(src_addr_v, 2*i, PAGE_SIZE);
#ifdef CONFIG_MET_MODULE
		met_tag_oneshot_real(0, "TEST_EMI", PAGE_SIZE);
#else
		met_tag_oneshot(0, "TEST_EMI", PAGE_SIZE);
#endif
	}
#ifdef CONFIG_MET_MODULE
	met_tag_end_real(0, "TEST_EMI");
#else
	met_tag_end(0, "TEST_EMI");
#endif

	my_preempt_enable();

#ifdef CONFIG_ARM
	/* dma_free */
	if (ops && ops->free) {
		ops->free(met_device.this_device,
				  PAGE_SIZE,
				  src_addr_v,
				  src_addr_p,
			0);
	}
#endif /* CONFIG_ARM */

#ifdef CONFIG_ARM64
	/* dma_free */
	if (src_addr_v != NULL)
		dma_free_coherent(met_device.this_device,
				  PAGE_SIZE,
				  src_addr_v,
				  src_addr_p);
#endif /* CONFIG_ARM64 */

	return n;
}

/*======================================================================*/
/*	KOBJ Declarations						*/
/*======================================================================*/
DECLARE_KOBJ_ATTR_INT(emi_TP_busfiltr_enable, emi_TP_busfiltr_enable);
DECLARE_KOBJ_ATTR_INT(msel_enable, msel_enable);
DECLARE_KOBJ_ATTR_HEX_CHECK(msel_group1, msel_group1, msel_group1 > 0 && msel_group1 <= _ALL);
DECLARE_KOBJ_ATTR_HEX_CHECK(msel_group2, msel_group2, msel_group2 > 0 && msel_group2 <= _ALL);
DECLARE_KOBJ_ATTR_HEX_CHECK(msel_group3, msel_group3, msel_group3 > 0 && msel_group3 <= _ALL);


DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype1_16_en,
	KOBJ_ITEM_LIST(
		{ BM_TTYPE1_16_ENABLE,   "ENABLE" },
		{ BM_TTYPE1_16_DISABLE,  "DISABLE" }
		)
	);
DECLARE_KOBJ_ATTR_STR_LIST(ttype1_16_en, ttype1_16_en, ttype1_16_en);


DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype17_21_en,
	KOBJ_ITEM_LIST(
		{ BM_TTYPE17_21_ENABLE,  "ENABLE" },
		{ BM_TTYPE17_21_DISABLE, "DISABLE" }
		)
	);
DECLARE_KOBJ_ATTR_STR_LIST(ttype17_21_en, ttype17_21_en, ttype17_21_en);

DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype_master,
	KOBJ_ITEM_LIST(
		{ _M0,  "M0" },
		{ _M1,  "M1" },
		{ _M2,  "M2" },
		{ _M3,  "M3" },
		{ _M4,  "M4" },
		{ _M5,  "M5" },
		{ _M6,  "M6" },
		{ _M7,  "M7" }
		)
	);


DECLARE_KOBJ_ATTR_INT_LIST_ITEM(
	ttype_nbeat,
	KOBJ_ITEM_LIST(
		{ BM_TRANS_TYPE_1BEAT,   1 },
		{ BM_TRANS_TYPE_2BEAT,   2 },
		{ BM_TRANS_TYPE_3BEAT,   3 },
		{ BM_TRANS_TYPE_4BEAT,   4 },
		{ BM_TRANS_TYPE_5BEAT,   5 },
		{ BM_TRANS_TYPE_6BEAT,   6 },
		{ BM_TRANS_TYPE_7BEAT,   7 },
		{ BM_TRANS_TYPE_8BEAT,   8 },
		{ BM_TRANS_TYPE_9BEAT,   9 },
		{ BM_TRANS_TYPE_10BEAT,  10 },
		{ BM_TRANS_TYPE_11BEAT,  11 },
		{ BM_TRANS_TYPE_12BEAT,  12 },
		{ BM_TRANS_TYPE_13BEAT,  13 },
		{ BM_TRANS_TYPE_14BEAT,  14 },
		{ BM_TRANS_TYPE_15BEAT,  15 },
		{ BM_TRANS_TYPE_16BEAT,  16 }
		)
	);
DECLARE_KOBJ_ATTR_INT_LIST_ITEM(
	ttype_nbyte,
	KOBJ_ITEM_LIST(
		{ BM_TRANS_TYPE_1Byte,   1 },
		{ BM_TRANS_TYPE_2Byte,   2 },
		{ BM_TRANS_TYPE_4Byte,   4 },
		{ BM_TRANS_TYPE_8Byte,   8 },
		{ BM_TRANS_TYPE_16Byte,  16 },
		{ BM_TRANS_TYPE_32Byte,  32 }
		)
	);
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype_burst,
	KOBJ_ITEM_LIST(
		{ BM_TRANS_TYPE_BURST_INCR,      "INCR" },
		{ BM_TRANS_TYPE_BURST_WRAP,      "WRAP" }
		)
	);

DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype_rw,
	KOBJ_ITEM_LIST(
		{ BM_TRANS_RW_DEFAULT,   "DEFAULT" },
		{ BM_TRANS_RW_READONLY,  "R" },
		{ BM_TRANS_RW_WRITEONLY, "W" },
		{ BM_TRANS_RW_RWBOTH,    "BOTH" }
		)
	);


DECLARE_KOBJ_ATTR_SHOW_INT(test, times);

DECLARE_KOBJ_ATTR(test);


static int high_priority_filter;
DECLARE_KOBJ_ATTR_HEX(high_priority_filter, high_priority_filter);


static int ttype_master_val[21];
static int ttype_busid_val[21];
static int ttype_nbeat_val[21];
static int ttype_nbyte_val[21];
static int ttype_burst_val[21];
static int ttype_rw_val[21];

#define DECLARE_KOBJ_TTYPE_MASTER(nr) \
	DECLARE_KOBJ_ATTR_STR_LIST(ttype ## nr ## _master, ttype_master_val[nr - 1], ttype_master)

#define DECLARE_KOBJ_TTYPE_NBEAT(nr) \
	DECLARE_KOBJ_ATTR_INT_LIST(ttype ## nr ## _nbeat, ttype_nbeat_val[nr - 1], ttype_nbeat)

#define DECLARE_KOBJ_TTYPE_NBYTE(nr) \
	DECLARE_KOBJ_ATTR_INT_LIST(ttype ## nr ## _nbyte, ttype_nbyte_val[nr - 1], ttype_nbyte)

#define DECLARE_KOBJ_TTYPE_BURST(nr) \
	DECLARE_KOBJ_ATTR_STR_LIST(ttype ## nr ## _burst, ttype_burst_val[nr - 1], ttype_burst)

#define DECLARE_KOBJ_TTYPE_RW(nr) \
	DECLARE_KOBJ_ATTR_STR_LIST(ttype ## nr ## _rw, ttype_rw_val[nr - 1], ttype_rw)

#define DECLARE_KOBJ_TTYPE_BUSID_VAL(nr) \
	DECLARE_KOBJ_ATTR_HEX(ttype ## nr ## _busid, ttype_busid_val[nr - 1])

DECLARE_KOBJ_TTYPE_MASTER(1);
DECLARE_KOBJ_TTYPE_NBEAT(1);
DECLARE_KOBJ_TTYPE_NBYTE(1);
DECLARE_KOBJ_TTYPE_BURST(1);
DECLARE_KOBJ_TTYPE_RW(1);
DECLARE_KOBJ_TTYPE_BUSID_VAL(1);

DECLARE_KOBJ_TTYPE_MASTER(2);
DECLARE_KOBJ_TTYPE_NBEAT(2);
DECLARE_KOBJ_TTYPE_NBYTE(2);
DECLARE_KOBJ_TTYPE_BURST(2);
DECLARE_KOBJ_TTYPE_RW(2);
DECLARE_KOBJ_TTYPE_BUSID_VAL(2);

DECLARE_KOBJ_TTYPE_MASTER(3);
DECLARE_KOBJ_TTYPE_NBEAT(3);
DECLARE_KOBJ_TTYPE_NBYTE(3);
DECLARE_KOBJ_TTYPE_BURST(3);
DECLARE_KOBJ_TTYPE_RW(3);
DECLARE_KOBJ_TTYPE_BUSID_VAL(3);

DECLARE_KOBJ_TTYPE_MASTER(4);
DECLARE_KOBJ_TTYPE_NBEAT(4);
DECLARE_KOBJ_TTYPE_NBYTE(4);
DECLARE_KOBJ_TTYPE_BURST(4);
DECLARE_KOBJ_TTYPE_RW(4);
DECLARE_KOBJ_TTYPE_BUSID_VAL(4);

DECLARE_KOBJ_TTYPE_MASTER(5);
DECLARE_KOBJ_TTYPE_NBEAT(5);
DECLARE_KOBJ_TTYPE_NBYTE(5);
DECLARE_KOBJ_TTYPE_BURST(5);
DECLARE_KOBJ_TTYPE_RW(5);
DECLARE_KOBJ_TTYPE_BUSID_VAL(5);

DECLARE_KOBJ_TTYPE_MASTER(6);
DECLARE_KOBJ_TTYPE_NBEAT(6);
DECLARE_KOBJ_TTYPE_NBYTE(6);
DECLARE_KOBJ_TTYPE_BURST(6);
DECLARE_KOBJ_TTYPE_RW(6);
DECLARE_KOBJ_TTYPE_BUSID_VAL(6);

DECLARE_KOBJ_TTYPE_MASTER(7);
DECLARE_KOBJ_TTYPE_NBEAT(7);
DECLARE_KOBJ_TTYPE_NBYTE(7);
DECLARE_KOBJ_TTYPE_BURST(7);
DECLARE_KOBJ_TTYPE_RW(7);
DECLARE_KOBJ_TTYPE_BUSID_VAL(7);

DECLARE_KOBJ_TTYPE_MASTER(8);
DECLARE_KOBJ_TTYPE_NBEAT(8);
DECLARE_KOBJ_TTYPE_NBYTE(8);
DECLARE_KOBJ_TTYPE_BURST(8);
DECLARE_KOBJ_TTYPE_RW(8);
DECLARE_KOBJ_TTYPE_BUSID_VAL(8);

DECLARE_KOBJ_TTYPE_MASTER(9);
DECLARE_KOBJ_TTYPE_NBEAT(9);
DECLARE_KOBJ_TTYPE_NBYTE(9);
DECLARE_KOBJ_TTYPE_BURST(9);
DECLARE_KOBJ_TTYPE_RW(9);
DECLARE_KOBJ_TTYPE_BUSID_VAL(9);

DECLARE_KOBJ_TTYPE_MASTER(10);
DECLARE_KOBJ_TTYPE_NBEAT(10);
DECLARE_KOBJ_TTYPE_NBYTE(10);
DECLARE_KOBJ_TTYPE_BURST(10);
DECLARE_KOBJ_TTYPE_RW(10);
DECLARE_KOBJ_TTYPE_BUSID_VAL(10);

DECLARE_KOBJ_TTYPE_MASTER(11);
DECLARE_KOBJ_TTYPE_NBEAT(11);
DECLARE_KOBJ_TTYPE_NBYTE(11);
DECLARE_KOBJ_TTYPE_BURST(11);
DECLARE_KOBJ_TTYPE_RW(11);
DECLARE_KOBJ_TTYPE_BUSID_VAL(11);

DECLARE_KOBJ_TTYPE_MASTER(12);
DECLARE_KOBJ_TTYPE_NBEAT(12);
DECLARE_KOBJ_TTYPE_NBYTE(12);
DECLARE_KOBJ_TTYPE_BURST(12);
DECLARE_KOBJ_TTYPE_RW(12);
DECLARE_KOBJ_TTYPE_BUSID_VAL(12);

DECLARE_KOBJ_TTYPE_MASTER(13);
DECLARE_KOBJ_TTYPE_NBEAT(13);
DECLARE_KOBJ_TTYPE_NBYTE(13);
DECLARE_KOBJ_TTYPE_BURST(13);
DECLARE_KOBJ_TTYPE_RW(13);
DECLARE_KOBJ_TTYPE_BUSID_VAL(13);

DECLARE_KOBJ_TTYPE_MASTER(14);
DECLARE_KOBJ_TTYPE_NBEAT(14);
DECLARE_KOBJ_TTYPE_NBYTE(14);
DECLARE_KOBJ_TTYPE_BURST(14);
DECLARE_KOBJ_TTYPE_RW(14);
DECLARE_KOBJ_TTYPE_BUSID_VAL(14);

DECLARE_KOBJ_TTYPE_MASTER(15);
DECLARE_KOBJ_TTYPE_NBEAT(15);
DECLARE_KOBJ_TTYPE_NBYTE(15);
DECLARE_KOBJ_TTYPE_BURST(15);
DECLARE_KOBJ_TTYPE_RW(15);
DECLARE_KOBJ_TTYPE_BUSID_VAL(15);

DECLARE_KOBJ_TTYPE_MASTER(16);
DECLARE_KOBJ_TTYPE_NBEAT(16);
DECLARE_KOBJ_TTYPE_NBYTE(16);
DECLARE_KOBJ_TTYPE_BURST(16);
DECLARE_KOBJ_TTYPE_RW(16);
DECLARE_KOBJ_TTYPE_BUSID_VAL(16);

DECLARE_KOBJ_TTYPE_MASTER(17);
DECLARE_KOBJ_TTYPE_NBEAT(17);
DECLARE_KOBJ_TTYPE_NBYTE(17);
DECLARE_KOBJ_TTYPE_BURST(17);
DECLARE_KOBJ_TTYPE_RW(17);
DECLARE_KOBJ_TTYPE_BUSID_VAL(17);

DECLARE_KOBJ_TTYPE_MASTER(18);
DECLARE_KOBJ_TTYPE_NBEAT(18);
DECLARE_KOBJ_TTYPE_NBYTE(18);
DECLARE_KOBJ_TTYPE_BURST(18);
DECLARE_KOBJ_TTYPE_RW(18);
DECLARE_KOBJ_TTYPE_BUSID_VAL(18);

DECLARE_KOBJ_TTYPE_MASTER(19);
DECLARE_KOBJ_TTYPE_NBEAT(19);
DECLARE_KOBJ_TTYPE_NBYTE(19);
DECLARE_KOBJ_TTYPE_BURST(19);
DECLARE_KOBJ_TTYPE_RW(19);
DECLARE_KOBJ_TTYPE_BUSID_VAL(19);

DECLARE_KOBJ_TTYPE_MASTER(20);
DECLARE_KOBJ_TTYPE_NBEAT(20);
DECLARE_KOBJ_TTYPE_NBYTE(20);
DECLARE_KOBJ_TTYPE_BURST(20);
DECLARE_KOBJ_TTYPE_RW(20);
DECLARE_KOBJ_TTYPE_BUSID_VAL(20);

DECLARE_KOBJ_TTYPE_MASTER(21);
DECLARE_KOBJ_TTYPE_NBEAT(21);
DECLARE_KOBJ_TTYPE_NBYTE(21);
DECLARE_KOBJ_TTYPE_BURST(21);
DECLARE_KOBJ_TTYPE_RW(21);
DECLARE_KOBJ_TTYPE_BUSID_VAL(21);

#define KOBJ_ATTR_ITEM_SERIAL_FNODE(nr) \
	do { \
		KOBJ_ATTR_ITEM(ttype ## nr ## _master); \
		KOBJ_ATTR_ITEM(ttype ## nr ## _nbeat); \
		KOBJ_ATTR_ITEM(ttype ## nr ## _nbyte); \
		KOBJ_ATTR_ITEM(ttype ## nr ## _burst); \
		KOBJ_ATTR_ITEM(ttype ## nr ## _busid); \
		KOBJ_ATTR_ITEM(ttype ## nr ## _rw); \
	} while (0)

#define KOBJ_ATTR_LIST \
	do { \
		KOBJ_ATTR_ITEM(high_priority_filter); \
		KOBJ_ATTR_ITEM(emi_TP_busfiltr_enable); \
		KOBJ_ATTR_ITEM(msel_enable); \
		KOBJ_ATTR_ITEM(msel_group1); \
		KOBJ_ATTR_ITEM(msel_group2); \
		KOBJ_ATTR_ITEM(msel_group3); \
		KOBJ_ATTR_ITEM(ttype17_21_en); \
		KOBJ_ATTR_ITEM(ttype1_16_en); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(1); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(2); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(3); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(4); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(5); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(6); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(7); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(8); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(9); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(10); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(11); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(12); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(13); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(14); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(15); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(16); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(17); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(18); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(19); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(20); \
		KOBJ_ATTR_ITEM_SERIAL_FNODE(21); \
		KOBJ_ATTR_ITEM(test); \
	} while (0)

/*======================================================================*/
/*	EMI Operations							*/
/*======================================================================*/
static void emi_init(void)
{
	unsigned int bmrw0_val = 0, bmrw1_val = 0, i, enable;
	unsigned int msel_group_val[4];

	MET_BM_SaveCfg();

	/* get dram channel number */
	dram_chann_num = MET_EMI_GetDramChannNum();

	if ((ttype1_16_en != BM_TTYPE1_16_ENABLE) && (emi_TP_busfiltr_enable != 1)) {
		if (msel_enable) {
			msel_group_val[0] = _ALL;
			msel_group_val[1] = msel_group1;
			msel_group_val[2] = msel_group2;
			msel_group_val[3] = msel_group3;
		} else {
			msel_group_val[0] = _ALL;
			msel_group_val[1] = _ALL;
			msel_group_val[2] = _ALL;
			msel_group_val[3] = _ALL;
		}

		MET_BM_SetLatencyCounter(1);

		for (i = 1; i <= 4; i++) {
			MET_BM_SetMonitorCounter(i,
						 msel_group_val[i - 1] & _ALL,
						 BM_TRANS_TYPE_4BEAT |
						 BM_TRANS_TYPE_8Byte |
						 BM_TRANS_TYPE_BURST_WRAP);
			MET_BM_SetbusID(i, 0);
			MET_BM_SetbusID_En(i, 0);
		}
		for (i = 0; i < 4; i++)
			MET_BM_Set_WsctTsct_id_sel(i, 0);

	} else if ((ttype1_16_en != BM_TTYPE1_16_ENABLE) && (emi_TP_busfiltr_enable == 1)) {
		MET_BM_SetLatencyCounter(1);

		for (i = 1; i <= 4; i++) {
			MET_BM_SetMonitorCounter(i,
						 ttype_master_val[i - 1],
						 ttype_nbeat_val[i - 1] |
						 ttype_nbyte_val[i - 1] |
						 ttype_burst_val[i - 1]);
			MET_BM_SetbusID(i, ttype_busid_val[i - 1]);
			MET_BM_SetbusID_En(i, 0);
		}
		for (i = 0; i < 4; i++)
			MET_BM_Set_WsctTsct_id_sel(i, 1);

	} else if ((ttype1_16_en == BM_TTYPE1_16_ENABLE) && (emi_TP_busfiltr_enable != 1)) {
		MET_BM_SetLatencyCounter(0);

		for (i = 1; i <= 16; i++) {
			MET_BM_SetMonitorCounter(i,
						 ttype_master_val[i - 1],
						 ttype_nbeat_val[i - 1] |
						 ttype_nbyte_val[i - 1] |
						 ttype_burst_val[i - 1]);

			MET_BM_SetbusID(i, ttype_busid_val[i - 1]);

			MET_BM_SetbusID_En(i, (ttype_busid_val[i - 1] > 0xffff) ? 0 : 1);
		}
		for (i = 0; i < 4; i++)
			MET_BM_Set_WsctTsct_id_sel(i, 0);
	} else {	/* (ttype1_16_en == BM_TTYPE1_16_ENABLE)  &&  (emi_TP_busfiltr_enable == 1) */

		MET_BM_SetLatencyCounter(0);

		for (i = 1; i <= 16; i++) {
			MET_BM_SetMonitorCounter(i,
						 ttype_master_val[i - 1],
						 ttype_nbeat_val[i - 1] |
						 ttype_nbyte_val[i - 1] |
						 ttype_burst_val[i - 1]);

			MET_BM_SetbusID(i, ttype_busid_val[i - 1]);

			MET_BM_SetbusID_En(i, (ttype_busid_val[i - 1] > 0xffff) ? 0 : 1);
		}
		for (i = 0; i < 4; i++)
			MET_BM_Set_WsctTsct_id_sel(i, 1);
	}

	if (ttype17_21_en == BM_TTYPE17_21_ENABLE) {
		for (i = 17; i <= 21; i++) {
			MET_BM_SetMonitorCounter(i,
						 ttype_master_val[i - 1],
						 ttype_nbeat_val[i - 1] |
						 ttype_nbyte_val[i - 1] |
						 ttype_burst_val[i - 1]);
			MET_BM_SetbusID(i, ttype_busid_val[i - 1]);

			MET_BM_SetbusID_En(i, (ttype_busid_val[i - 1] > 0xffff) ? 0 : 1);
		}
	}

	bmrw0_val = (
		(ttype_rw_val[0] << 0) | (ttype_rw_val[1] << 2) |
		(ttype_rw_val[2] << 4) | (ttype_rw_val[3] << 6) |
		(ttype_rw_val[4] << 8) | (ttype_rw_val[5] << 10) |
		(ttype_rw_val[6] << 12) | (ttype_rw_val[7] << 14) |
		(ttype_rw_val[8] << 16) | (ttype_rw_val[9] << 18) |
		(ttype_rw_val[10] << 20) | (ttype_rw_val[11] << 22) |
		(ttype_rw_val[12] << 24) | (ttype_rw_val[13] << 26) |
		(ttype_rw_val[14] << 28) | (ttype_rw_val[15] << 30));

	bmrw1_val = (
		(ttype_rw_val[16] << 0) | (ttype_rw_val[17] << 2) |
		(ttype_rw_val[18] << 4) | (ttype_rw_val[19] << 6) |
		(ttype_rw_val[20] << 8));

	MET_BM_SetTtypeCounterRW(bmrw0_val, bmrw1_val);

	for (i = 0; i < BM_COUNTER_MAX; i++) {
		if ((high_priority_filter & (1 << i)) == 0)
			enable = 0;
		else
			enable = 1;

		MET_BM_SetUltraHighFilter(i + 1, enable);
	}

}

static inline int do_emi(void)
{
	return met_sspm_emi.mode;
}

/*======================================================================*/
/*	MET Device Operations						*/
/*======================================================================*/
static int emi_inited;
static int met_emi_create(struct kobject *parent)
{
	int ret = 0;
	int i;

	for (i = 0; i < 21; i++) {
		ttype_master_val[i] = _M0;
		ttype_nbeat_val[i] = BM_TRANS_TYPE_1BEAT;
		ttype_nbyte_val[i] = BM_TRANS_TYPE_8Byte;
		ttype_burst_val[i] = BM_TRANS_TYPE_BURST_INCR;
		ttype_busid_val[i] = 0xfffff;
		ttype_rw_val[i] =  BM_TRANS_RW_DEFAULT;
	}
	ret = MET_BM_Init();
	if (ret != 0) {
		pr_debug("MET_BM_Init failed!!!\n");
		ret = 0;
	} else
		emi_inited = 1;

	kobj_emi = parent;

#define	KOBJ_ATTR_ITEM(attr_name) \
do { \
	ret = sysfs_create_file(kobj_emi, &attr_name##_attr.attr); \
	if (ret != 0) { \
		pr_debug("Failed to create " #attr_name " in sysfs\n"); \
		return ret; \
	} \
} while (0)
	KOBJ_ATTR_LIST;
#undef	KOBJ_ATTR_ITEM

	return ret;
}

static void met_emi_delete(void)
{
#define	KOBJ_ATTR_ITEM(attr_name) \
	sysfs_remove_file(kobj_emi, &attr_name##_attr.attr)
	if (kobj_emi != NULL) {
		KOBJ_ATTR_LIST;
		kobj_emi = NULL;
	}
#undef	KOBJ_ATTR_ITEM

	if (emi_inited)
		MET_BM_DeInit();
}


static void met_emi_resume(void)
{
	if (!do_emi())
		return;

	emi_init();
}


#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
static const char help[] = "  --emi                                 monitor EMI banwidth\n";

#define TTYPE_NAME_STR_LEN  64
/* static char ttype_name[21][TTYPE_NAME_STR_LEN]; */

static int emi_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
}

static int emi_print_header(char *buf, int len)
{
	int ret = 0;
/*	int ret_m[21]; */
	int i = 0;
	int err;
	unsigned int dram_data_rate_MHz;
	unsigned int DRAM_TYPE;

	if ((ttype1_16_en != BM_TTYPE1_16_ENABLE) && (emi_TP_busfiltr_enable != 1)) {
		if (msel_enable) {
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"met-info [000] 0.0: met_emi_msel: %x,%x,%x\n",
				msel_group1 & _ALL,
				msel_group2 & _ALL,
				msel_group3 & _ALL);
		} else {
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"met-info [000] 0.0: met_emi_msel: %x,%x,%x\n",
				_ALL & _ALL,
				_ALL & _ALL,
				_ALL & _ALL);
		}
	} else {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"met-info [000] 0.0: met_emi_ttype_master: %x,%x,%x,%x\n",
				ttype_master_val[0], ttype_master_val[1], ttype_master_val[2], ttype_master_val[3]);

		if (emi_TP_busfiltr_enable == 1) {

			ret += snprintf(buf + ret, PAGE_SIZE - ret,
					"met-info [000] 0.0: met_emi_ttype_busid: %x,%x,%x,%x\n",
					ttype_busid_val[0], ttype_busid_val[1], ttype_busid_val[2], ttype_busid_val[3]);
		}
	}

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "met-info [000] 0.0: met_emi_rw_cfg: ");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "BOTH");

	for (i = 0; i < 21; i++) {
		if (ttype_rw_val[i] == BM_TRANS_RW_DEFAULT)
			ret += snprintf(buf + ret, PAGE_SIZE - ret, ",DEFAULT");
		else if (ttype_rw_val[i] == BM_TRANS_RW_READONLY)
			ret += snprintf(buf + ret, PAGE_SIZE - ret, ",R");
		else if (ttype_rw_val[i] == BM_TRANS_RW_WRITEONLY)
			ret += snprintf(buf + ret, PAGE_SIZE - ret, ",W");
		else
			ret += snprintf(buf + ret, PAGE_SIZE - ret, ",BOTH");
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"met-info [000] 0.0: met_emi_ultra_filter: %x\n", high_priority_filter);

	/* ttype header */
	if (ttype17_21_en == BM_TTYPE17_21_ENABLE) {
		int i = 0;
		int j = 0;

		/* ttype master list */
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "met-info [000] 0.0: met_emi_ttype_master_list: ");
		for (i = 0; i < 21; i++) {
			for (j = 0; j < ARRAY_SIZE(ttype_master_list_item); j++) {
				if (ttype_master_val[i] == ttype_master_list_item[j].key) {
					ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s,", ttype_master_list_item[j].val);
				}
			}
		}
		/* remove the last comma */
		err = snprintf(buf + ret -1, PAGE_SIZE - ret + 1, "\n");
		if (err < 0) return err;

		/* ttype busid list */
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "met-info [000] 0.0: met_emi_ttype_busid_list: ");
		for (i = 0; i < 21; i++)
			ret += snprintf(buf + ret, PAGE_SIZE - ret, "%x,", ttype_busid_val[i]);

		err = snprintf(buf + ret -1, PAGE_SIZE - ret + 1, "\n");
		if (err < 0) return err;

		/* ttype nbeat list */
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "met-info [000] 0.0: met_emi_ttype_nbeat_list: ");
		for (i = 0; i < 21; i++) {
			for (j = 0; j < ARRAY_SIZE(ttype_nbeat_list_item); j++) {
				if (ttype_nbeat_val[i] == ttype_nbeat_list_item[j].key) {
					ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d,", ttype_nbeat_list_item[j].val);
				}
			}
		}
		err = snprintf(buf + ret -1, PAGE_SIZE - ret + 1, "\n");
		if (err < 0) return err;

		/* ttype nbyte list */
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "met-info [000] 0.0: met_emi_ttype_nbyte_list: ");
		for (i = 0; i < 21; i++) {
			for (j = 0; j < ARRAY_SIZE(ttype_nbyte_list_item); j++) {
				if (ttype_nbyte_val[i] == ttype_nbyte_list_item[j].key) {
					ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d,", ttype_nbyte_list_item[j].val);
				}
			}
		}
		err = snprintf(buf + ret -1, PAGE_SIZE - ret + 1, "\n");
		if (err < 0) return err;

		/* ttype burst list */
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "met-info [000] 0.0: met_emi_ttype_burst_list: ");
		for (i = 0; i < 21; i++) {
			for (j = 0; j < ARRAY_SIZE(ttype_burst_list_item); j++) {
				if (ttype_burst_val[i] == ttype_burst_list_item[j].key) {
					ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s,", ttype_burst_list_item[j].val);
				}
			}
		}
		err = snprintf(buf + ret -1, PAGE_SIZE - ret + 1, "\n");
		if (err < 0) return err;

		/* ttype enable */
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "met-info [000] 0.0: met_emi_ttype_enable: %d,%d\n",ttype1_16_en, ttype17_21_en);
	}
	/*IP version*/
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"met-info [000] 0.0: DRAMC_VER: %d\n", DRAMC_VER);

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"met-info [000] 0.0: EMI_VER: %d.%d\n", EMI_VER_MAJOR, EMI_VER_MINOR);
#if 1 /* move to AP side print header */
//#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	dram_chann_num = MET_EMI_GetDramChannNum();
	/*	met_dram_chann_num_header
	 *	channel number
	 *	LP4: 2, LP3: 1
	 */

	/*
	 *	the ddr type define :
	 *	enum DDRTYPE {
	 *	TYPE_LPDDR3 = 1,
	 *	TYPE_LPDDR4,
	 *	TYPE_LPDDR4X,
	 *	TYPE_LPDDR2
	 *	};
	 */
	if (mtk_dramc_get_ddr_type_symbol)
		DRAM_TYPE = mtk_dramc_get_ddr_type_symbol();
	else if (get_ddr_type_symbol)
		DRAM_TYPE = get_ddr_type_symbol();

	if (mtk_dramc_get_ddr_type_symbol || get_ddr_type_symbol) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "met-info [000] 0.0: met_dram_type: %d\n", DRAM_TYPE);

		if ((DRAM_TYPE == 2) || (DRAM_TYPE == 3))
			ret += snprintf(buf + ret, PAGE_SIZE - ret, "met-info [000] 0.0: met_dram_chann_num_header: %d,%d,%d,%d\n",
					dram_chann_num, DRAM_EMI_BASECLOCK_RATE_LP4,
					DRAM_IO_BUS_WIDTH_LP4, DRAM_DATARATE);
		else
			ret += snprintf(buf + ret, PAGE_SIZE - ret, "met-info [000] 0.0: met_dram_chann_num_header: %d,%d,%d,%d\n",
					dram_chann_num, DRAM_EMI_BASECLOCK_RATE_LP3,
					DRAM_IO_BUS_WIDTH_LP3, DRAM_DATARATE);
	} else
		METERROR("[%s][%d]mtk_dramc_get_ddr_type_symbol = NULL , use the TYPE_LPDDR3 setting\n", __func__, __LINE__);


	/* met_emi_clockrate */
	if (mtk_dramc_get_data_rate_symbol)
		dram_data_rate_MHz = mtk_dramc_get_data_rate_symbol();
	else if (get_dram_data_rate_symbol)
		dram_data_rate_MHz = get_dram_data_rate_symbol();
	else {
		METERROR("mtk_dramc_get_data_rate_symbol = NULL\n");
		dram_data_rate_MHz = 0;
	}

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"met-info [000] 0.0: met_dram_clockrate: %d\n",
			dram_data_rate_MHz);

	/*dram bank num*/
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"met-info [000] 0.0: met_dram_rank_num_header: %u,%u\n", MET_EMI_GetDramRankNum(),
				MET_EMI_GetDramRankNum());
#if 0
	/* ms_emi header */
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"# ms_emi: TS0,TS1,GP0_WSCT,GP1_WSCT,GP2_WSCT,GP3_WSCT,");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"M0_LATENCY,M1_LATENCY,M2_LATENCY,M3_LATENCY,M4_LATENCY,M5_LATENCY,M6_LATENCY,M7_LATENCY,");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"M0_TRANS,M1_TRANS,M2_TRANS,M3_TRANS,M4_TRANS,M5_TRANS,M6_TRANS,M7_TRANS,");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"BACT,BSCT,BCNT,WACT,DCM_CTRL,TACT,");

	for (i = 0; i < dram_chann_num; i++) {
		if (i != 0)
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
					",");
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"PageHit_%d,PageMiss_%d,InterBank_%d,Idle_%d,", i, i, i, i);
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"mr4_%d,refresh_pop_%d,freerun_26m_%d,", i, i, i);
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"read_bytes_%d,write_bytes_%d", i, i);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
#endif

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"met-info [000] 0.0: met_emi_header: TS0,TS1,GP0_WSCT,GP1_WSCT,GP2_WSCT,GP3_WSCT,");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"M0_LATENCY,M1_LATENCY,M2_LATENCY,M3_LATENCY,M4_LATENCY,M5_LATENCY,M6_LATENCY,M7_LATENCY,");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"M0_TRANS,M1_TRANS,M2_TRANS,M3_TRANS,M4_TRANS,M5_TRANS,M6_TRANS,M7_TRANS,");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"BACT,BSCT,BCNT,WACT,DCM_CTRL,TACT,");

	for (i = 0; i < dram_chann_num; i++) {
		if (i != 0)
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
					",");
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"PageHit_%d,PageMiss_%d,InterBank_%d,Idle_%d,", i, i, i, i);
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"mr4_%d,refresh_pop_%d,freerun_26m_%d,", i, i, i);
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"read_bytes_%d,write_bytes_%d", i, i);
	}
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");

	/*TSCT header*/
	if (emi_tsct_enable == 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"met-info [000] 0.0: ms_emi_tsct_header: ms_emi_tsct,");
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"tsct1,tsct2,tsct3\n");
	}

	/*MDCT header*/
	if (emi_mdct_enable == 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"met-info [000] 0.0: ms_emi_mdct_header: ms_emi_mdct,");
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"RD_ULTRA,RD_MDMCU\n");
	}

	/* met_bw_limiter_header */
	if (bw_limiter_enable == BM_BW_LIMITER_ENABLE) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"met-info [000] 0.0: met_bw_limiter_header: CLK,");
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"ARBA,ARBB,ARBC,ARBD,ARBE,ARBF,ARBG,ARBH,BWCT0,BWCT1,BWCT2,BWCT3,BWCT4,BWST0,BWST1,BWCT0_2ND,BWCT1_2ND,BWST_2ND\n");
	}

	/* DRAM DVFS header */
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"met-info [000] 0.0: DRAM_DVFS_header: datarate(MHz)\n");

	/*PDIR met_dramc_header*/
#if DRAMC_VER >= 2
	if (dramc_pdir_enable == 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"met-info [000] 0.0: met_dramc_header: ");
		for (i = 0; i < dram_chann_num; i++) {
			if (i != 0)
				ret += snprintf(buf + ret, PAGE_SIZE - ret,
						",");
			ret += snprintf(buf + ret, PAGE_SIZE - ret, "freerun_26m_%d,", i);
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
					"rk0_pre_sb_%d,rk0_pre_pd_%d,rk0_act_sb_%d,rk0_act_pd_%d,", i, i, i, i);
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
					"rk1_pre_sb_%d,rk1_pre_pd_%d,rk1_act_sb_%d,rk1_act_pd_%d,", i, i, i, i);
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
					"rk2_pre_sb_%d,rk2_pre_pd_%d,rk2_act_sb_%d,rk2_act_pd_%d", i, i, i, i);
		}
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	}
#endif

	/* DRS header */
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"met-info [000] 0.0: emi_drs_header: ch0_RANK1_GP(%%),ch0_RANK1_SF(%%),ch0_ALL_SF(%%),ch1_RANK1_GP(%%),ch1_RANK1_SF(%%),ch1_ALL_SF(%%)\n");
#endif
	return ret;
}

static int ondiemet_emi_print_header(char *buf, int len)
{
	return emi_print_header(buf, len);
}

static void MET_BM_IPI_REGISTER_CB(void)
{
	int ret, i;
	unsigned int rdata;
	unsigned int ipi_buf[4];

	for (i = 0; i < 4; i++)
		ipi_buf[i] = 0;

	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | (MID_EMI << MID_BIT_SHIFT) | MET_ARGU | SET_REGISTER_CB;
		ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	}
}

static void MET_BM_IPI_configs(void)
{
	int ret, i;
	unsigned int rdata;
	unsigned int ipi_buf[4];

	for (i = 0; i < 4; i++)
		ipi_buf[i] = 0;

	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | (MID_EMI << MID_BIT_SHIFT) | MET_ARGU | SET_EBM_CONFIGS1;
		ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	}
}

static void ondiemet_emi_start(void)
{
	MET_BM_IPI_REGISTER_CB();
	if (!emi_inited) {
		if (MET_BM_Init() != 0) {
			met_sspm_emi.mode = 0;
			pr_notice("MET_BM_Init failed!!!\n");
			return;
		}
		emi_inited = 1;
	}
	MET_BM_IPI_configs();

	if (do_emi())
		emi_init();

	ondiemet_module[ONDIEMET_SSPM] |= ID_EMI;
}

static void emi_uninit(void)
{
	MET_BM_RestoreCfg();
}

static void ondiemet_emi_stop(void)
{
	if (!emi_inited)
		return;

	if (do_emi())
		emi_uninit();
}
#endif /* end of #if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT) */
#endif /* end of #if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) */

struct metdevice met_sspm_emi = {
	.name			= "emi",
	.owner			= THIS_MODULE,
	.type			= MET_TYPE_BUS,
	.create_subfs		= met_emi_create,
	.delete_subfs		= met_emi_delete,
	.resume			= met_emi_resume,
#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
	.ondiemet_start		= ondiemet_emi_start,
	.ondiemet_stop		= ondiemet_emi_stop,
	.ondiemet_print_help	= emi_print_help,
	.ondiemet_print_header	= ondiemet_emi_print_header,
#endif
#endif
	.ondiemet_mode		= 1,
};
EXPORT_SYMBOL(met_sspm_emi);
