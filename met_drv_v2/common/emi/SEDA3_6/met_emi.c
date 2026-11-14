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
#include <linux/dma-mapping.h>
#include <linux/string.h>

#define MET_USER_EVENT_SUPPORT
#include "met_drv.h"
#include "trace.h"

#include "mtk_typedefs.h"
#include "core_plf_init.h"
#include "mtk_emi_bm.h"
#include "interface.h"
#include "met_dramc.h"

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT)
#include "sspm/ondiemet_sspm.h"
#elif defined(TINYSYS_SSPM_SUPPORT)
#include "tinysys_sspm.h"
#include "tinysys_mgr.h" /* for ondiemet_module */
#endif
#endif

#define MAX_HEADER_LEN (1024 * 6)
static char header_str[MAX_HEADER_LEN];

static unsigned int output_header_len;
static unsigned int output_str_len;

/* #define FILE_NODE_DBG */

/*======================================================================*/
/*	Global variable definitions					*/
/*======================================================================*/
/*ondiemet emi sampling interval in us */
int emi_tsct_enable = 1;
int emi_mdct_enable = 1;
int emi_TP_busfiltr_enable;


/* Dynamic MonitorCounter selection !!!EXPERIMENT!!! */
static int msel_enable;
static unsigned int msel_group1 = BM_MASTER_ALL;
static unsigned int msel_group2 = BM_MASTER_ALL;
static unsigned int msel_group3 = BM_MASTER_ALL;


/* Global variables */
static struct kobject *kobj_emi;
static int rwtype = BM_BOTH_READ_WRITE;

/* BW Limiter */
/*#define CNT_COUNTDOWN	(1000-1)*/		/* 1000 * 1ms = 1sec */
#define CNT_COUNTDOWN   (0)                     /* 1ms */
/* static int countdown; */
static int bw_limiter_enable = BM_BW_LIMITER_ENABLE;

/* TTYPE counter */
static int ttype1_16_en = BM_TTYPE1_16_DISABLE;
static int ttype17_21_en = BM_TTYPE17_21_DISABLE;

static int dramc_pdir_enable;
static int dram_chann_num = 1;

enum SSPM_Mode {
	CUSTOMER_MODE = 0x0,
	UNDEFINE_MODE = 0x1,
	INTERNAL_MODE = 0X2780
};



/*======================================================================*/
/*	KOBJ Declarations						*/
/*======================================================================*/


DECLARE_KOBJ_ATTR_INT(emi_TP_busfiltr_enable, emi_TP_busfiltr_enable);


DECLARE_KOBJ_ATTR_INT(msel_enable, msel_enable);
DECLARE_KOBJ_ATTR_HEX_CHECK(msel_group1, msel_group1, msel_group1 > 0 && msel_group1 <= BM_MASTER_ALL);
DECLARE_KOBJ_ATTR_HEX_CHECK(msel_group2, msel_group2, msel_group2 > 0 && msel_group2 <= BM_MASTER_ALL);
DECLARE_KOBJ_ATTR_HEX_CHECK(msel_group3, msel_group3, msel_group3 > 0 && msel_group3 <= BM_MASTER_ALL);



/* KOBJ: rwtype */
DECLARE_KOBJ_ATTR_INT_CHECK(rwtype, rwtype, rwtype >= 0 && rwtype <= BM_WRITE_ONLY);

/*
static unsigned int get_emi_clock_rate(unsigned int dram_data_rate_MHz)
{
	unsigned int DRAM_TYPE;

	if (mtk_dramc_get_ddr_type_symbol) {
		DRAM_TYPE = mtk_dramc_get_ddr_type_symbol();

		if ((DRAM_TYPE == 5) || (DRAM_TYPE == 6) || (DRAM_TYPE == 7))
			return dram_data_rate_MHz / DRAM_EMI_BASECLOCK_RATE_LP4 / DRAM_DATARATE;
		else
			return dram_data_rate_MHz / DRAM_EMI_BASECLOCK_RATE_LP3 / DRAM_DATARATE;
	} else {
		METERROR("[%s][%d]mtk_dramc_get_ddr_type_symbol = NULL , use the TYPE_LPDDR4 setting\n", __func__, __LINE__);
		return dram_data_rate_MHz / DRAM_EMI_BASECLOCK_RATE_LP4 / DRAM_DATARATE;
	}
}
*/

/* KOBJ: ttype1_16_en */
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype1_16_en,
	KOBJ_ITEM_LIST(
		{ BM_TTYPE1_16_ENABLE,   "ENABLE" },
		{ BM_TTYPE1_16_DISABLE,  "DISABLE" }
		)
	);
DECLARE_KOBJ_ATTR_STR_LIST(ttype1_16_en, ttype1_16_en, ttype1_16_en);

/* KOBJ: ttype17_21_en */
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype17_21_en,
	KOBJ_ITEM_LIST(
		{ BM_TTYPE17_21_ENABLE,  "ENABLE" },
		{ BM_TTYPE17_21_DISABLE, "DISABLE" }
		)
	);
DECLARE_KOBJ_ATTR_STR_LIST(ttype17_21_en, ttype17_21_en, ttype17_21_en);

/* KOBJ: bw_limiter_enable */
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	bw_limiter_enable,
	KOBJ_ITEM_LIST(
		{ BM_BW_LIMITER_ENABLE,  "ENABLE" },
		{ BM_BW_LIMITER_DISABLE, "DISABLE" }
		)
	);

DECLARE_KOBJ_ATTR_STR_LIST(bw_limiter_enable, bw_limiter_enable, bw_limiter_enable);

/* KOBJ: ttype_master */
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	ttype_master,
	KOBJ_ITEM_LIST(
		{ BM_MASTER_M0,  "M0" },
		{ BM_MASTER_M1,  "M1" },
		{ BM_MASTER_M2,  "M2" },
		{ BM_MASTER_M3,  "M3" },
		{ BM_MASTER_M4,  "M4" },
		{ BM_MASTER_M5,  "M5" },
		{ BM_MASTER_M6,  "M6" },
		{ BM_MASTER_M7,  "M7" }
		)
	);


/* KOBJ: ttypeX_nbeat, ttypeX_nbyte, ttypeX_burst */
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


DECLARE_KOBJ_ATTR_INT(dramc_pdir_enable, dramc_pdir_enable);

/*enable high priority filter*/
static int high_priority_filter;
DECLARE_KOBJ_ATTR_HEX(high_priority_filter, high_priority_filter);



/**/
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


static unsigned int get_sspm_support_feature(void)
{
	unsigned int rdata=0;
#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
	int ret, i;
	unsigned int ipi_buf[4];

	for (i = 0; i < 4; i++)
		ipi_buf[i] = 0;

	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | (MID_EMI << MID_BIT_SHIFT) | MET_REQ_AP2MD ;
		ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	}
#endif
#endif
	return rdata;
}

/* SEDA 3.5 ext */
static unsigned int msel_group_ext_val[WSCT_AMOUNT];
static unsigned int wsct_rw_val[WSCT_AMOUNT];

char* const delim_comma = ",";
char* const delim_coclon = ":";


char msel_group_ext[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_msel_group_ext(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		msel_group_ext_val[i] = BM_MASTER_ALL;
	}

	/*WSCT 4~5 default is ultra, pre-ultra total*/
	msel_group_ext[0] = '\0';
}

static ssize_t msel_group_ext_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	/*parse wsct_id:group,
	1. split data  by ","
	2. split subdata by ":"
	3. check the value is OK

	don't clear the setting, do this by echo 1 > clear_setting
	*/

	char *token, *cur= msel_group_ext;
	char *_id = NULL, *_master_group = NULL;
	int id_int = 0;

	_clear_msel_group_ext();

	snprintf(msel_group_ext, FILE_NODE_DATA_LEN, "%s", buf);
	msel_group_ext[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:0xff , (ID,master_group)*/

		_id = strsep(&token, delim_coclon); // ID
		_master_group = strsep(&token, delim_coclon);

		PR_BOOTMSG("_id[%s] _master_group[%s]\n",_id,_master_group);

		if (_id == NULL || _master_group == NULL) {
			PR_BOOTMSG("err: _id[%s] _master_group[%s], para can't be NULL\n",_id,_master_group);
			_clear_msel_group_ext();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_msel_group_ext();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			if (kstrtouint(_master_group, 0, &msel_group_ext_val[id_int]) != 0) {
				PR_BOOTMSG("master_group[%s] trans to hex err\n",_master_group);
				_clear_msel_group_ext();
				return -EINVAL;
			}
		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_msel_group_ext();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",msel_group_ext);
	/*PR_BOOTMSG("msel_group_ext_store size para n[%d]\n",n);*/
	int i;
	PR_BOOTMSG("save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=%X\n",i,msel_group_ext_val[i]);
	}
#endif
	return n;
}

static ssize_t msel_group_ext_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", msel_group_ext);
}


char wsct_rw[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_wsct_rw(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		wsct_rw_val[i] = BM_WSCT_RW_RWBOTH;
	}
	wsct_rw[0] = '\0';
}

static ssize_t wsct_rw_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	char *token, *cur= wsct_rw;
	char *_id = NULL, *_rw_type = NULL;
	int id_int = 0;

	_clear_wsct_rw();

	snprintf(wsct_rw, FILE_NODE_DATA_LEN, "%s", buf);
	wsct_rw[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_rw_type = strsep(&token, delim_coclon);

		if (_id == NULL || _rw_type == NULL) {
			PR_BOOTMSG("err: _id[%s] _rw_type[%s], para can't be NULL\n",_id, _rw_type);
			_clear_wsct_rw();
			return -EINVAL;
		}

		PR_BOOTMSG("_id[%s] _rw_type[%s]\n",_id, _rw_type);
		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_wsct_rw();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			if ( 0 == strncmp("NONE",_rw_type,4))
				wsct_rw_val[id_int] = BM_WSCT_RW_DISABLE;
			else if (0 == strncmp("R",_rw_type,4))
				wsct_rw_val[id_int] = BM_WSCT_RW_READONLY;
			else if (0 == strncmp("W",_rw_type,4))
				wsct_rw_val[id_int] = BM_WSCT_RW_WRITEONLY;
			else if (0 == strncmp("RW",_rw_type,4))
				wsct_rw_val[id_int] = BM_WSCT_RW_RWBOTH;
			else {
				PR_BOOTMSG("_id[%s] has err rwtype[%s]\n", _id, _rw_type);
				_clear_wsct_rw();
				return -EINVAL;
			}

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_wsct_rw();
			return -EINVAL;
		}
	}

#ifdef FILE_NODE_DBG
	PR_BOOTMSG("wsct_rw_store input data [%s]\n",wsct_rw);
	int i;
	PR_BOOTMSG("rwtype save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=%d\n",i,wsct_rw_val[i]);
	}
#endif
	return n;
}

static ssize_t wsct_rw_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", wsct_rw);
}


static unsigned int WSCT_HPRI_DIS[WSCT_AMOUNT];
static unsigned int WSCT_HPRI_SEL[WSCT_AMOUNT];
char wsct_high_priority_enable[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_wsct_high_priority_enable(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		WSCT_HPRI_DIS[i] = 1;
		WSCT_HPRI_SEL[i] = 0xF;
	}

	WSCT_HPRI_DIS[4] = 0;
	WSCT_HPRI_SEL[4] = 0x8;  /* ultra */

	WSCT_HPRI_DIS[5] = 0;
	WSCT_HPRI_SEL[5] = 0x4; /* pre_ultra */


	wsct_high_priority_enable[0] = '\0';
}

static ssize_t wsct_high_priority_enable_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	char *token, *cur= wsct_high_priority_enable;
	char *_id = NULL, *_enable = NULL,  *_level = NULL;
	int  id_int = 0, level_int = 0;

	_clear_wsct_high_priority_enable();

	snprintf(wsct_high_priority_enable, FILE_NODE_DATA_LEN, "%s", buf);
	wsct_high_priority_enable[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_enable = strsep(&token, delim_coclon);
		_level = strsep(&token, delim_coclon);

		PR_BOOTMSG("_id[%s] _enable[%s] _level[%s]\n",_id, _enable, _level);

		if (_id == NULL || _enable == NULL || _level == NULL ) {
			PR_BOOTMSG("err : _id[%s] _enable[%s] _level[%s], para can't be NULL\n",_id, _enable, _level);
			_clear_wsct_high_priority_enable();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_wsct_high_priority_enable();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			if ( 0 == strncmp("disable", _enable, 7)) {

				WSCT_HPRI_DIS[id_int] = 1;
				WSCT_HPRI_SEL[id_int] = 0xf;
			} else if ( 0 == strncmp("enable", _enable, 6)) {

				WSCT_HPRI_DIS[id_int] = 0;
				if (kstrtouint(_level, 0, &level_int) != 0) {
					PR_BOOTMSG("_id[%s] trans ultraLevel[%s] to hex err\n",_id, _level);
					_clear_wsct_high_priority_enable();
					return -EINVAL;
				}
				WSCT_HPRI_SEL[id_int] = level_int & 0xF;
			} else {
				PR_BOOTMSG("_id[%s] has err enable[%s] (enable/disable)\n", _id, _enable);
				_clear_wsct_high_priority_enable();
				return -EINVAL;
			}

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_wsct_high_priority_enable();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",wsct_high_priority_enable);
	int i;
	PR_BOOTMSG("wsct_high_priority_enable save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=(%X,%X)\n", i, WSCT_HPRI_DIS[i], WSCT_HPRI_SEL[i]);
	}
#endif
	return n;
}

static ssize_t wsct_high_priority_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", wsct_high_priority_enable);
}


static unsigned int wsct_busid_val[WSCT_AMOUNT];
static unsigned int wsct_idMask_val[WSCT_AMOUNT];

char wsct_busid[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_wsct_busid(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		wsct_busid_val[i] = 0xfffff;
		wsct_idMask_val[i] = 0x1FFF;
	}
	wsct_busid[0] = '\0';
}

static ssize_t wsct_busid_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	char *token, *cur= wsct_busid;

	char *_id = NULL, *_busid = NULL, *_idMask = NULL;
	int id_int = 0, busid_int = 0, idMask_int = 0;

	_clear_wsct_busid();

	snprintf(wsct_busid, FILE_NODE_DATA_LEN, "%s", buf);
	wsct_busid[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_busid = strsep(&token, delim_coclon);
		_idMask = strsep(&token, delim_coclon);

		PR_BOOTMSG("_id[%s] _busid[%s] _idMask[%s]\n",_id, _busid, _idMask);

		if (_id == NULL || _busid == NULL || _idMask == NULL) {
			PR_BOOTMSG("err: _id[%s] _busid[%s] _idMask[%s] ,parameter can't be NULL\n",_id, _busid, _idMask);
			_clear_wsct_busid();
			return -EINVAL;
		}


		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_wsct_busid();
			return -EINVAL;
		}
		if (kstrtouint(_busid, 0, &busid_int) != 0) {
			PR_BOOTMSG("_busid[%s] trans to hex err\n",_busid);
			_clear_wsct_busid();
			return -EINVAL;
		}
		if (kstrtouint(_idMask, 0, &idMask_int) != 0) {
			PR_BOOTMSG("_idMask[%s] trans to hex err\n",_idMask);
			_clear_wsct_busid();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			wsct_busid_val[id_int] = busid_int;
			wsct_idMask_val[id_int] = idMask_int;

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_wsct_busid();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",wsct_busid);
	int i;
	PR_BOOTMSG("wsct_busid save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d](busid,idMask)=(%X,%X)\n", i, wsct_busid_val[i], wsct_idMask_val[i]);
	}
#endif
	return n;
}

static ssize_t wsct_busid_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", wsct_busid);
}


static unsigned int  wsct_chn_rank_sel_val[WSCT_AMOUNT];
char wsct_chn_rank_sel[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_wsct_chn_rank_sel(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		wsct_chn_rank_sel_val[i] = 0xF;
	}
	wsct_chn_rank_sel[0] = '\0';
}

static ssize_t wsct_chn_rank_sel_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	char *token, *cur= wsct_chn_rank_sel;
	char *_id = NULL, *_chn_rank = NULL;
	int id_int = 0, chn_rank_int = 0;

	_clear_wsct_chn_rank_sel();

	snprintf(wsct_chn_rank_sel, FILE_NODE_DATA_LEN, "%s", buf);
	wsct_chn_rank_sel[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:f , 5:C (ID,chn_rnk_sel)*/

		_id = strsep(&token, delim_coclon); // ID
		_chn_rank = strsep(&token, delim_coclon);

		PR_BOOTMSG("_id[%s] _chn_rank[%s]\n",_id, _chn_rank);

		if (_id == NULL || _chn_rank == NULL) {
			PR_BOOTMSG("err : _id[%s] _chn_rank[%s], para can't be NULL\n",_id, _chn_rank);
			_clear_wsct_chn_rank_sel();
			return -EINVAL;
		}


		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_wsct_chn_rank_sel();
			return -EINVAL;
		}
		if (kstrtouint(_chn_rank, 0, &chn_rank_int) != 0) {
			PR_BOOTMSG("_chn_rank[%s] trans to hex err\n",_id);
			_clear_wsct_chn_rank_sel();
			return -EINVAL;
		}

		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			wsct_chn_rank_sel_val[id_int] = chn_rank_int;

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_wsct_chn_rank_sel();
			return -EINVAL;
		}
	}

#ifdef FILE_NODE_DBG
	PR_BOOTMSG("wsct_chn_rank_sel input data [%s]\n",wsct_chn_rank_sel);
	int i;
	PR_BOOTMSG("wsct_chn_rank_sel_val save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=%X\n",i,wsct_chn_rank_sel_val[i]);
	}
#endif
	return n;
}

static ssize_t wsct_chn_rank_sel_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", wsct_chn_rank_sel);
}

static unsigned int  wsct_byte_low_bnd_val[WSCT_AMOUNT];
static unsigned int  wsct_byte_up_bnd_val[WSCT_AMOUNT];
static unsigned int  wsct_byte_bnd_dis[WSCT_AMOUNT];
char wsct_burst_range[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_wsct_burst_range(void) {
	int i;

	for (i=0;i<WSCT_AMOUNT;i++) {
		wsct_byte_low_bnd_val[i] = 0x0;
		wsct_byte_up_bnd_val[i] = 0x1FF;
		wsct_byte_bnd_dis[i] = 1;
	}
	wsct_burst_range[0] = '\0';
}

static ssize_t wsct_burst_range_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	char *token, *cur= wsct_burst_range;
	char *_id = NULL, *_low_bnd = NULL, *_up_bnd = NULL;
	int id_int = 0, low_bnd_int = 0, up_bnd_int = 0;

	_clear_wsct_burst_range();

	snprintf(wsct_burst_range, FILE_NODE_DATA_LEN, "%s", buf);
	wsct_burst_range[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:f , 5:C (ID,chn_rnk_sel)*/

		_id = strsep(&token, delim_coclon); // ID
		_low_bnd = strsep(&token, delim_coclon);
		_up_bnd = strsep(&token, delim_coclon);

		PR_BOOTMSG("_id[%s] _low_bnd[%s] _up_bnd[%s]\n",_id, _low_bnd, _up_bnd);

		if (_id == NULL || _low_bnd == NULL || _up_bnd == NULL) {
			PR_BOOTMSG("err : _id[%s] _low_bnd[%s] _up_bnd[%s], para can't be NULL\n",_id, _low_bnd, _up_bnd);
			_clear_wsct_burst_range();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_wsct_burst_range();
			return -EINVAL;
		}
		if (kstrtouint(_low_bnd, 0, &low_bnd_int) != 0) {
			PR_BOOTMSG("_low_bnd[%s] trans to hex err\n",_id);
			_clear_wsct_burst_range();
			return -EINVAL;
		}
		if (kstrtouint(_up_bnd, 0, &up_bnd_int) != 0) {
			PR_BOOTMSG("_up_bnd[%s] trans to hex err\n",_id);
			_clear_wsct_burst_range();
			return -EINVAL;
		}

		if ( id_int >= 0 && id_int < WSCT_AMOUNT) {
			wsct_byte_low_bnd_val[id_int] = low_bnd_int;
			wsct_byte_up_bnd_val[id_int] = up_bnd_int;
			wsct_byte_bnd_dis[id_int] = 0;
		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, WSCT_AMOUNT-1);
			_clear_wsct_burst_range();
			return -EINVAL;
		}
	}

#ifdef FILE_NODE_DBG
	PR_BOOTMSG("wsct_burst_range_store input data [%s]\n",wsct_burst_range);
	int i;
	PR_BOOTMSG("wsct_burst_range save data\n");
	for (i=0;i<WSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d](low_bnd,up_bnd)=(%X,%X)\n",i,wsct_byte_low_bnd_val[i],wsct_byte_up_bnd_val[i]);
	}
#endif
	return n;
}

static ssize_t wsct_burst_range_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", wsct_burst_range);
}



static unsigned int tsct_busid_enable_val[TSCT_AMOUNT];
char tsct_busid_enable[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_tsct_busid_enable(void) {
	int i;

	for (i=0;i<TSCT_AMOUNT;i++) {
		tsct_busid_enable_val[i] = 0;
	}
	tsct_busid_enable[0] = '\0';
}

static ssize_t tsct_busid_enable_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	char *token, *cur= tsct_busid_enable;
	char *_id = NULL, *_enable = NULL;
	int  id_int = 0;

	_clear_tsct_busid_enable();

	snprintf(tsct_busid_enable, FILE_NODE_DATA_LEN, "%s", buf);
	tsct_busid_enable[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_enable = strsep(&token, delim_coclon);


		PR_BOOTMSG("_id[%s] _enable[%s]\n",_id, _enable);

		if (_id == NULL || _enable == NULL) {
			PR_BOOTMSG("err : _id[%s] _enable[%s], para can't be NULL\n",_id, _enable);
			_clear_tsct_busid_enable();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_tsct_busid_enable();
			return -EINVAL;
		}


		if ( id_int >= 0 && id_int < TSCT_AMOUNT) {
			if ( 0 == strncmp("disable", _enable, 7)) {
				tsct_busid_enable_val[id_int] = 0;
			} else if ( 0 == strncmp("enable", _enable, 6)) {
				tsct_busid_enable_val[id_int] = 1;
			} else {
				PR_BOOTMSG("_id[%s] has err enable[%s] (enable/disable)\n", _id, _enable);
				_clear_tsct_busid_enable();
				return -EINVAL;
			}

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 0~%d\n",id_int, TSCT_AMOUNT-1);
			_clear_tsct_busid_enable();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("tsct_busid_enable input data [%s]\n",tsct_busid_enable);
	int i;
	PR_BOOTMSG("wsct_high_priority_enable save data\n");
	for (i=0;i<TSCT_AMOUNT;i++) {
		PR_BOOTMSG("id[%d]=(%d)\n", i, tsct_busid_enable_val[i]);
	}
#endif
	return n;
}

static ssize_t tsct_busid_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", tsct_busid_enable);
}


/* use the origin para high_priority_filter to save the en/dis setting */
static unsigned int TTYPE_HPRI_SEL[BM_COUNTER_MAX];
char ttype_high_priority_ext[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_ttype_high_priority_ext(void) {
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		TTYPE_HPRI_SEL[i] = 0xf;
	}

	high_priority_filter = 0x0;
	ttype_high_priority_ext[0] = '\0';
}

static ssize_t ttype_high_priority_ext_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	char *token, *cur= ttype_high_priority_ext;
	char *_id = NULL, *_enable = NULL,  *_level = NULL;
	int  id_int = 0, level_int = 0;

	_clear_ttype_high_priority_ext();

	snprintf(ttype_high_priority_ext, FILE_NODE_DATA_LEN, "%s", buf);
	ttype_high_priority_ext[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_enable = strsep(&token, delim_coclon);
		_level = strsep(&token, delim_coclon);

		PR_BOOTMSG("_id[%s] _enable[%s] _level[%s]\n",_id, _enable, _level);

		if (_id == NULL || _enable == NULL || _level == NULL ) {
			PR_BOOTMSG("err : _id[%s] _enable[%s] _level[%s], para can't be NULL\n",_id, _enable, _level);
			_clear_ttype_high_priority_ext();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_ttype_high_priority_ext();
			return -EINVAL;
		}

		id_int = id_int - 1;
		if ( id_int >= 0 && id_int < BM_COUNTER_MAX) {
			if ( 0 == strncmp("disable", _enable, 7)) {

				high_priority_filter = ( high_priority_filter & ~(1<<id_int) ) | ( 0<<id_int );
				TTYPE_HPRI_SEL[id_int] = 0xf;
			} else if ( 0 == strncmp("enable", _enable, 6)) {

				high_priority_filter = ( high_priority_filter & ~(1<<id_int) ) | ( 1<<id_int );
				if (kstrtouint(_level, 0, &level_int) != 0) {
					PR_BOOTMSG("_id[%s] trans ultraLevel[%s] to hex err\n",_id, _level);
					_clear_ttype_high_priority_ext();
					return -EINVAL;
				}
				TTYPE_HPRI_SEL[id_int] = level_int & 0xF;
			} else {
				PR_BOOTMSG("ttype_high_priority_ext: _id[%s] has err enable[%s] (enable/disable)\n", _id, _enable);
				_clear_ttype_high_priority_ext();
				return -EINVAL;
			}

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 1~%d\n",id_int+1, BM_COUNTER_MAX);
			_clear_ttype_high_priority_ext();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("input data [%s]\n",ttype_high_priority_ext);

	int i;
	PR_BOOTMSG("wsct_high_priority_enable save data\n");
	for (i=0;i<BM_COUNTER_MAX;i++) {
		PR_BOOTMSG("id[%d]=(%X,%X)\n", i+1, high_priority_filter>>i & 0x1, TTYPE_HPRI_SEL[i]);
	}
#endif
	return n;
}

static ssize_t ttype_high_priority_ext_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", ttype_high_priority_ext);
}


static unsigned int ttype_idMask_val[BM_COUNTER_MAX];
char ttype_busid_ext[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_ttype_busid_ext(void) {
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		ttype_busid_val[i] = 0xfffff;
		ttype_idMask_val[i] = 0x1FFF;
	}
	ttype_busid_ext[0] = '\0';
}

/*id: 1~21*/
static ssize_t ttype_busid_ext_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	char *token, *cur= ttype_busid_ext;

	char *_id = NULL, *_busid = NULL, *_idMask = NULL;
	int id_int = 0, busid_int = 0, idMask_int = 0;

	_clear_ttype_busid_ext();

	snprintf(ttype_busid_ext, FILE_NODE_DATA_LEN, "%s", buf);
	ttype_busid_ext[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:R , 5:W (ID,RW)*/

		_id = strsep(&token, delim_coclon); // ID
		_busid = strsep(&token, delim_coclon);
		_idMask = strsep(&token, delim_coclon);

		PR_BOOTMSG("_id[%s] _busid[%s] _idMask[%s]\n",_id, _busid, _idMask);

		if (_id == NULL || _busid == NULL || _idMask == NULL) {
			PR_BOOTMSG("err: ttype_busid_ext _id[%s] _busid[%s] _idMask[%s] ,parameter can't be NULL\n",_id, _busid, _idMask);
			_clear_ttype_busid_ext();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_ttype_busid_ext();
			return -EINVAL;
		}
		if (kstrtouint(_busid, 0, &busid_int) != 0) {
			PR_BOOTMSG("_busid[%s] trans to hex err\n",_busid);
			_clear_ttype_busid_ext();
			return -EINVAL;
		}
		if (kstrtouint(_idMask, 0, &idMask_int) != 0) {
			PR_BOOTMSG("_idMask[%s] trans to hex err\n",_idMask);
			_clear_ttype_busid_ext();
			return -EINVAL;
		}

		id_int = id_int - 1;
		if ( id_int >= 0 && id_int < BM_COUNTER_MAX) {
			ttype_busid_val[id_int] = busid_int;
			ttype_idMask_val[id_int] = idMask_int;

		} else {
			PR_BOOTMSG("ttype_busid_ext id[%d] exceed the range, it must be 1~%d\n",id_int+1, BM_COUNTER_MAX);
			_clear_ttype_busid_ext();
			return -EINVAL;
		}
	}
#ifdef FILE_NODE_DBG
	PR_BOOTMSG("ttype_busid_ext input data [%s]\n",ttype_busid_ext);

	int i;
	PR_BOOTMSG("ttype_busid_ext save data\n");
	for (i=0;i<BM_COUNTER_MAX;i++) {
		PR_BOOTMSG("id[%d](busid,idMask)=(%X,%X)\n", i+1, ttype_busid_val[i], ttype_idMask_val[i]);
	}
#endif
	return n;
}

static ssize_t ttype_busid_ext_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", ttype_busid_ext);
}


static unsigned int  ttype_chn_rank_sel_val[BM_COUNTER_MAX];
char ttype_chn_rank_sel[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_ttype_chn_rank_sel(void) {
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		ttype_chn_rank_sel_val[i] = 0xF;
	}
	ttype_chn_rank_sel[0] = '\0';
}

static ssize_t ttype_chn_rank_sel_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	char *token, *cur= ttype_chn_rank_sel;
	char *_id = NULL, *_chn_rank = NULL;
	int id_int = 0, chn_rank_int = 0;

	_clear_ttype_chn_rank_sel();

	snprintf(ttype_chn_rank_sel, FILE_NODE_DATA_LEN, "%s", buf);
	ttype_chn_rank_sel[n-1]='\0';

	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:f , 5:C (ID,chn_rnk_sel)*/

		_id = strsep(&token, delim_coclon); // ID
		_chn_rank = strsep(&token, delim_coclon);

		PR_BOOTMSG("_id[%s] _chn_rank[%s]\n",_id, _chn_rank);

		if (_id == NULL || _chn_rank == NULL) {
			PR_BOOTMSG("err (ttype_chn_rank_sel): _id[%s] _chn_rank[%s], para can't be NULL\n",_id, _chn_rank);
			_clear_ttype_chn_rank_sel();
			return -EINVAL;
		}


		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_ttype_chn_rank_sel();
			return -EINVAL;
		}
		if (kstrtouint(_chn_rank, 0, &chn_rank_int) != 0) {
			PR_BOOTMSG("_chn_rank[%s] trans to hex err\n",_id);
			_clear_ttype_chn_rank_sel();
			return -EINVAL;
		}

		id_int = id_int -1;

		if ( id_int >= 0 && id_int < BM_COUNTER_MAX) {
			ttype_chn_rank_sel[id_int] = chn_rank_int;

		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 1~%d\n",id_int+1, BM_COUNTER_MAX);
			_clear_ttype_chn_rank_sel();
			return -EINVAL;
		}
	}

#ifdef FILE_NODE_DBG
	PR_BOOTMSG("ttype_chn_rank_sel input data [%s]\n",ttype_chn_rank_sel);

	int i;
	PR_BOOTMSG("wsct_chn_rank_sel_val save data\n");
	for (i=0;i<BM_COUNTER_MAX;i++) {
		PR_BOOTMSG("id[%d]=%X\n",i+1,ttype_chn_rank_sel[i]);
	}
#endif
	return n;
}

static ssize_t ttype_chn_rank_sel_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", ttype_chn_rank_sel);
}


static unsigned int  ttype_byte_low_bnd_val[BM_COUNTER_MAX];
static unsigned int  ttype_byte_up_bnd_val[BM_COUNTER_MAX];
static unsigned int  ttype_byte_bnd_dis[BM_COUNTER_MAX];
char ttype_burst_range[FILE_NODE_DATA_LEN] = {'\0'};

static void _clear_ttype_burst_range(void) {
	int i;

	for (i=0;i<BM_COUNTER_MAX;i++) {
		ttype_byte_low_bnd_val[i] = 0x0;
		ttype_byte_up_bnd_val[i] = 0x1FF;
		ttype_byte_bnd_dis[i] = 1;
	}
	ttype_burst_range[0] = '\0';
}

static ssize_t ttype_burst_range_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf,
		size_t n)
{
	char *token, *cur= ttype_burst_range;
	char *_id = NULL, *_low_bnd = NULL, *_up_bnd = NULL;
	int id_int = 0, low_bnd_int = 0, up_bnd_int = 0;

	_clear_ttype_burst_range();

	snprintf(ttype_burst_range, FILE_NODE_DATA_LEN, "%s", buf);
	ttype_burst_range[n-1]='\0';


	while (cur != NULL) {
		token = strsep(&cur, delim_comma);
		PR_BOOTMSG("token: %s\n",token);
		/*token EX: 4:f , 5:C (ID,chn_rnk_sel)*/

		_id = strsep(&token, delim_coclon); // ID
		_low_bnd = strsep(&token, delim_coclon);
		_up_bnd = strsep(&token, delim_coclon);

		PR_BOOTMSG("_id[%s] _low_bnd[%s] _up_bnd[%s]\n",_id, _low_bnd, _up_bnd);

		if (_id == NULL || _low_bnd == NULL || _up_bnd == NULL) {
			PR_BOOTMSG("err (ttype_burst_range): _id[%s] _low_bnd[%s] _up_bnd[%s], para can't be NULL\n",
				        _id, _low_bnd, _up_bnd);
			_clear_ttype_burst_range();
			return -EINVAL;
		}

		if (kstrtouint(_id, 0, &id_int) != 0) {
			PR_BOOTMSG("_id[%s] trans to hex err\n",_id);
			_clear_ttype_burst_range();
			return -EINVAL;
		}
		if (kstrtouint(_low_bnd, 0, &low_bnd_int) != 0) {
			PR_BOOTMSG("_low_bnd[%s] trans to hex err\n",_id);
			_clear_ttype_burst_range();
			return -EINVAL;
		}
		if (kstrtouint(_up_bnd, 0, &up_bnd_int) != 0) {
			PR_BOOTMSG("_up_bnd[%s] trans to hex err\n",_id);
			_clear_ttype_burst_range();
			return -EINVAL;
		}

		id_int = id_int - 1;
		if ( id_int >= 0 && id_int < BM_COUNTER_MAX) {
			ttype_byte_low_bnd_val[id_int] = low_bnd_int;
			ttype_byte_up_bnd_val[id_int] = up_bnd_int;
			ttype_byte_bnd_dis[id_int] = 0;
		} else {
			PR_BOOTMSG("id[%d] exceed the range, it must be 1~%d\n",id_int, BM_COUNTER_MAX);
			_clear_ttype_burst_range();
			return -EINVAL;
		}
	}

#ifdef FILE_NODE_DBG
	PR_BOOTMSG("ttype_burst_range_store input data [%s]\n",ttype_burst_range);

	int i;
	PR_BOOTMSG("ttype_burst_range save data\n");
	for (i=0;i<BM_COUNTER_MAX;i++) {
		PR_BOOTMSG("id[%d](low_bnd,up_bnd)=(%X,%X)\n",i+1,ttype_byte_low_bnd_val[i],ttype_byte_up_bnd_val[i]);
	}
#endif
	return n;
}

static ssize_t ttype_burst_range_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", ttype_burst_range);
}

static int reserve_wsct_setting;
DECLARE_KOBJ_ATTR_INT(reserve_wsct_setting, reserve_wsct_setting);

static void _clear_setting(void) {
	/*clear all file node para here*/

	PR_BOOTMSG("clear EMI file node setting\n");

	_clear_msel_group_ext();
	_clear_wsct_rw();
	_clear_wsct_high_priority_enable();
	_clear_wsct_busid();
	_clear_wsct_chn_rank_sel();
	_clear_wsct_burst_range();

	_clear_tsct_busid_enable();
	_clear_ttype_high_priority_ext();
	_clear_ttype_busid_ext();
	_clear_ttype_chn_rank_sel();
	_clear_ttype_burst_range();
	reserve_wsct_setting = 0;



	emi_TP_busfiltr_enable = 0;

	high_priority_filter = 0x0;
	rwtype = BM_BOTH_READ_WRITE;
	dramc_pdir_enable = 1;


	msel_enable = 0;
	msel_group1 = BM_MASTER_ALL;
	msel_group2 = BM_MASTER_ALL;
	msel_group3 = BM_MASTER_ALL;


	bw_limiter_enable = BM_BW_LIMITER_ENABLE;
	ttype1_16_en = BM_TTYPE1_16_DISABLE;
	ttype17_21_en = BM_TTYPE17_21_DISABLE;

}

static ssize_t clear_setting_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf,
			size_t n)
{
	int value;

	if ((n == 0) || (buf == NULL))
		return -EINVAL;

	if (kstrtoint(buf, 0, &value) != 0)
		return -EINVAL;

	if (value == 1)
		_clear_setting();

	return n;
}

static struct kobj_attribute clear_setting_attr = __ATTR_WO(clear_setting); // OK
static struct kobj_attribute msel_group_ext_attr = __ATTR(msel_group_ext, 0664, msel_group_ext_show, msel_group_ext_store); //OK
static struct kobj_attribute wsct_rw_attr = __ATTR(wsct_rw, 0664, wsct_rw_show, wsct_rw_store);
static struct kobj_attribute wsct_high_priority_enable_attr = __ATTR(wsct_high_priority_enable, 0664, wsct_high_priority_enable_show, wsct_high_priority_enable_store);
static struct kobj_attribute wsct_busid_attr = __ATTR(wsct_busid, 0664, wsct_busid_show, wsct_busid_store);
static struct kobj_attribute wsct_chn_rank_sel_attr = __ATTR(wsct_chn_rank_sel, 0664, wsct_chn_rank_sel_show, wsct_chn_rank_sel_store);
static struct kobj_attribute wsct_burst_range_attr = __ATTR(wsct_burst_range, 0664, wsct_burst_range_show, wsct_burst_range_store);
static struct kobj_attribute tsct_busid_enable_attr = __ATTR(tsct_busid_enable, 0664, tsct_busid_enable_show, tsct_busid_enable_store);
static struct kobj_attribute ttype_high_priority_ext_attr = __ATTR(ttype_high_priority_ext, 0664, ttype_high_priority_ext_show, ttype_high_priority_ext_store);
static struct kobj_attribute ttype_busid_ext_attr = __ATTR(ttype_busid_ext, 0664, ttype_busid_ext_show, ttype_busid_ext_store);
static struct kobj_attribute ttype_chn_rank_sel_attr = __ATTR(ttype_chn_rank_sel, 0664, ttype_chn_rank_sel_show, ttype_chn_rank_sel_store);
static struct kobj_attribute ttype_burst_range_attr = __ATTR(ttype_burst_range, 0664, ttype_burst_range_show, ttype_burst_range_store);






/**/
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
		KOBJ_ATTR_ITEM(rwtype); \
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
		KOBJ_ATTR_ITEM(bw_limiter_enable); \
		KOBJ_ATTR_ITEM(dramc_pdir_enable); \
		KOBJ_ATTR_ITEM(clear_setting);\
		KOBJ_ATTR_ITEM(msel_group_ext);\
		KOBJ_ATTR_ITEM(wsct_rw);\
		KOBJ_ATTR_ITEM(wsct_high_priority_enable);\
		KOBJ_ATTR_ITEM(wsct_busid);\
		KOBJ_ATTR_ITEM(wsct_chn_rank_sel);\
		KOBJ_ATTR_ITEM(wsct_burst_range);\
		KOBJ_ATTR_ITEM(tsct_busid_enable);\
		KOBJ_ATTR_ITEM(ttype_high_priority_ext);\
		KOBJ_ATTR_ITEM(ttype_busid_ext);\
		KOBJ_ATTR_ITEM(ttype_chn_rank_sel);\
		KOBJ_ATTR_ITEM(ttype_burst_range);\
		KOBJ_ATTR_ITEM(reserve_wsct_setting);\
	} while (0)



/*======================================================================*/
/*	EMI Operations							*/
/*======================================================================*/
static void emi_init(void)
{
	unsigned int bmrw0_val, bmrw1_val, i;
	/*unsigned int msel_group_val[4];*/

	/*save origianl EMI config*/
	MET_BM_SaveCfg();

	/* get dram channel number */
	dram_chann_num = MET_EMI_GetDramChannNum(0);

	/* Init. EMI bus monitor */
	MET_BM_SetReadWriteType(rwtype);

	/*handle the ori */

	if (ttype1_16_en != BM_TTYPE1_16_ENABLE) {
		MET_BM_SetLatencyCounter(1);    /*enable latency count*/
	}
	else {
		MET_BM_SetLatencyCounter(0);    /*disable latency count*/

		for (i = 1; i <= 16; i++) {
			MET_BM_SetMonitorCounter(i,
						 ttype_master_val[i - 1],
						 ttype_nbeat_val[i - 1] |
						 ttype_nbyte_val[i - 1] |
						 ttype_burst_val[i - 1]);
		}
	}

	if (ttype17_21_en == BM_TTYPE17_21_ENABLE) {
		for (i = 17; i <= 21; i++) {
			MET_BM_SetMonitorCounter(i,
						 ttype_master_val[i - 1],
						 ttype_nbeat_val[i - 1] |
						 ttype_nbyte_val[i - 1] |
						 ttype_burst_val[i - 1]);
		}
	}

	PR_BOOTMSG("[%s]reserve_wsct_setting=%d\n",__func__,reserve_wsct_setting);

	if (reserve_wsct_setting == 0) {
		/* wsct 0 : total-all*/
		msel_group_ext_val[0] = BM_MASTER_ALL;
		wsct_rw_val[0] = BM_WSCT_RW_RWBOTH;
		WSCT_HPRI_DIS[0] = 1;
		WSCT_HPRI_SEL[0] = 0xF;
		wsct_busid_val[0] = 0xFFFFF;
		wsct_idMask_val[0] = 0xFFFF;
		wsct_chn_rank_sel_val[0] = 0xF;
		wsct_byte_bnd_dis[0] = 1;

		/* wsct 4 : total-ultra*/
		msel_group_ext_val[4] = BM_MASTER_ALL;
		wsct_rw_val[4] = BM_WSCT_RW_RWBOTH;
		WSCT_HPRI_DIS[4] = 0;
		WSCT_HPRI_SEL[4] = 0x8;  /* ultra */
		wsct_busid_val[4] = 0xFFFFF;
		wsct_idMask_val[4] = 0xFFFF;
		wsct_chn_rank_sel_val[4] = 0xF;
		wsct_byte_bnd_dis[4] = 1;

		/* wsct 5 : total-pre_ultra*/
		msel_group_ext_val[5] = BM_MASTER_ALL;
		wsct_rw_val[5] = BM_WSCT_RW_RWBOTH;
		WSCT_HPRI_DIS[5] = 0;
		WSCT_HPRI_SEL[5] = 0x4; /* pre_ultra */
		wsct_busid_val[5] = 0xFFFFF;
		wsct_idMask_val[5] = 0xFFFF;
		wsct_chn_rank_sel_val[5] = 0xF;
		wsct_byte_bnd_dis[5] = 1;
	}

	if (msel_enable) {
		/* if ole file node set, use the value */
		if ( msel_group1 != BM_MASTER_ALL )
			msel_group_ext_val[1] = msel_group1;

		if ( msel_group2 != BM_MASTER_ALL )
			msel_group_ext_val[2] = msel_group2;

		if ( msel_group3 != BM_MASTER_ALL )
			msel_group_ext_val[3] = msel_group3;

	} else {
		for ( i=1; i<=3; i++) {
			msel_group_ext_val[i] = BM_MASTER_ALL;
		}
	}

	MET_BM_SetWSCT_master_rw(msel_group_ext_val, wsct_rw_val);
	MET_BM_SetWSCT_high_priority(WSCT_HPRI_DIS, WSCT_HPRI_SEL);
	MET_BM_SetWSCT_busid_idmask(wsct_busid_val, wsct_idMask_val);
	MET_BM_SetWSCT_chn_rank_sel(wsct_chn_rank_sel_val);
	MET_BM_SetWSCT_burst_range(wsct_byte_bnd_dis, wsct_byte_low_bnd_val, wsct_byte_up_bnd_val);
	MET_BM_SetTSCT_busid_enable(tsct_busid_enable_val);

	MET_BM_SetTtype_high_priority_sel(high_priority_filter, TTYPE_HPRI_SEL);
	MET_BM_SetTtype_busid_idmask(ttype_busid_val, ttype_idMask_val, ttype1_16_en, ttype17_21_en);
	MET_BM_SetTtype_chn_rank_sel(ttype_chn_rank_sel_val);
	MET_BM_SetTtype_burst_range(ttype_byte_bnd_dis, ttype_byte_low_bnd_val, ttype_byte_up_bnd_val);


	bmrw0_val = 0;
	for (i = 0; i < 16; i++)
		bmrw0_val |= (ttype_rw_val[i] << (i * 2));

	bmrw1_val = 0;
	for (i = 16; i < 21; i++)
		bmrw1_val |= (ttype_rw_val[i] << ((i-16) * 2));

	MET_BM_SetTtypeCounterRW(bmrw0_val, bmrw1_val);

}


static void emi_uninit(void)
{
	MET_BM_RestoreCfg();
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
		ttype_master_val[i] = BM_MASTER_M0;
		ttype_nbeat_val[i] = BM_TRANS_TYPE_1BEAT;
		ttype_nbyte_val[i] = BM_TRANS_TYPE_8Byte;
		ttype_burst_val[i] = BM_TRANS_TYPE_BURST_INCR;
		ttype_busid_val[i] = 0xfffff;   /*default disable ttype bus sel if busid > 0xff_ff */
		ttype_rw_val[i] =  BM_TRANS_RW_DEFAULT;
	}

	_clear_msel_group_ext();
	_clear_wsct_rw();
	_clear_wsct_high_priority_enable();
	_clear_wsct_busid();
	_clear_wsct_chn_rank_sel();
	_clear_wsct_burst_range();

	_clear_tsct_busid_enable();
	_clear_ttype_high_priority_ext();
	_clear_ttype_high_priority_ext();
	_clear_ttype_busid_ext();
	_clear_ttype_chn_rank_sel();
	_clear_ttype_burst_range();

	reserve_wsct_setting = 0;


	ret = MET_BM_Init();
	if (ret != 0) {
		pr_notice("MET_BM_Init failed!!!\n");
		ret = 0;        /* will retry later */
	} else {
		emi_inited = 1;
	}

	kobj_emi = parent;

#define KOBJ_ATTR_ITEM(attr_name) \
	do { \
		ret = sysfs_create_file(kobj_emi, &attr_name ## _attr.attr); \
		if (ret != 0) { \
			pr_notice("Failed to create " #attr_name " in sysfs\n"); \
			return ret; \
		} \
	} while (0)
	KOBJ_ATTR_LIST;
#undef  KOBJ_ATTR_ITEM

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


static const char help[] = "  --emi                                 monitor EMI banwidth\n";

static int emi_print_header(char *buf, int len)
{

	if( (strlen(header_str) - output_str_len) > PAGE_SIZE ){
		char output_buf[PAGE_SIZE/4];

		strncpy(output_buf, header_str+output_str_len, (PAGE_SIZE/4) -1);
		output_buf[(PAGE_SIZE/4) - 1] = '\0';

		len = snprintf(buf, PAGE_SIZE, "%s", output_buf);
		if (len < 0)
			PR_BOOTMSG("emi_print_header snprintf return err[%d]\n",len);

		output_str_len += len;
		met_sspm_emi.header_read_again = 1;

		PR_BOOTMSG("EMI header read again!\n");
	}
	else{
		len = snprintf(buf, PAGE_SIZE, "%s\n", header_str+output_str_len);
		if (len < 0)
			PR_BOOTMSG("emi_print_header snprintf return err[%d]\n",len);
		/* reset state */
		met_sspm_emi.header_read_again = 0;
		output_header_len = 0;
		output_str_len = 0;
	}


	return len;
}


#define TTYPE_NAME_STR_LEN  64
/* static char ttype_name[21][TTYPE_NAME_STR_LEN]; */

static int emi_create_header(char *buf, int buf_len)
{
	int ret = 0;
/*	int ret_m[21]; */
	int i = 0;

#if 1 /* move to AP side print header */
/*#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT*/
	unsigned int dram_data_rate_MHz;
	unsigned int DRAM_TYPE;
	unsigned int base_clock_rate;
#endif


	ret += snprintf(buf + ret, buf_len - ret,
		"met-info [000] 0.0: met_emi_wsct_amount: %d\n",WSCT_AMOUNT);

	/* master selection header */
	ret += snprintf(buf + ret, buf_len - ret,
		"met-info [000] 0.0: met_emi_msel: %x,%x,%x\n",
		msel_group_ext_val[1] & BM_MASTER_ALL,
		msel_group_ext_val[2] & BM_MASTER_ALL,
		msel_group_ext_val[3] & BM_MASTER_ALL);

	/*Ttype RW type header*/
	PR_BOOTMSG("rwtype=%d\n",rwtype);
	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_rw_cfg: ");
	if (rwtype == BM_READ_ONLY)
		ret += snprintf(buf + ret, buf_len - ret, "R");
	else if (rwtype == BM_WRITE_ONLY)
		ret += snprintf(buf + ret, buf_len - ret, "W");
	else
		ret += snprintf(buf + ret, buf_len - ret, "BOTH");

	for (i = 0; i < 21; i++) {
		if (ttype_rw_val[i] == BM_TRANS_RW_DEFAULT)
			ret += snprintf(buf + ret, buf_len - ret, ",DEFAULT");
		else if (ttype_rw_val[i] == BM_TRANS_RW_READONLY)
			ret += snprintf(buf + ret, buf_len - ret, ",R");
		else if (ttype_rw_val[i] == BM_TRANS_RW_WRITEONLY)
			ret += snprintf(buf + ret, buf_len - ret, ",W");
		else    /*BM_TRANS_RW_RWBOTH*/
			ret += snprintf(buf + ret, buf_len - ret, ",BOTH");
	}
	ret += snprintf(buf + ret, buf_len - ret, "\n");

	/*ultra header*/
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_emi_ultra_filter: %x\n", high_priority_filter);

	/* ttype header */
	if (ttype17_21_en == BM_TTYPE17_21_ENABLE) {
		int i = 0;
		int j = 0;

		/* ttype master list */
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_master_list: ");
		for (i = 0; i < 21; i++) {
			for (j = 0; j < ARRAY_SIZE(ttype_master_list_item); j++) {
				if (ttype_master_val[i] == ttype_master_list_item[j].key) {
					ret += snprintf(buf + ret, buf_len - ret, "%s,", ttype_master_list_item[j].val);
				}
			}
		}
		/* remove the last comma */
		snprintf(buf + ret -1, buf_len - ret + 1, "\n");

		/* ttype busid list */
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_busid_list: ");
		for (i = 0; i < 21; i++)
			ret += snprintf(buf + ret, buf_len - ret, "%x,", ttype_busid_val[i]);

		snprintf(buf + ret -1, buf_len - ret + 1, "\n");

		/* ttype nbeat list */
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_nbeat_list: ");
		for (i = 0; i < 21; i++) {
			for (j = 0; j < ARRAY_SIZE(ttype_nbeat_list_item); j++) {
				if (ttype_nbeat_val[i] == ttype_nbeat_list_item[j].key) {
					ret += snprintf(buf + ret, buf_len - ret, "%d,", ttype_nbeat_list_item[j].val);
				}
			}
		}
		snprintf(buf + ret -1, buf_len - ret + 1, "\n");

		/* ttype nbyte list */
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_nbyte_list: ");
		for (i = 0; i < 21; i++) {
			for (j = 0; j < ARRAY_SIZE(ttype_nbyte_list_item); j++) {
				if (ttype_nbyte_val[i] == ttype_nbyte_list_item[j].key) {
					ret += snprintf(buf + ret, buf_len - ret, "%d,", ttype_nbyte_list_item[j].val);
				}
			}
		}
		snprintf(buf + ret -1, buf_len - ret + 1, "\n");

		/* ttype burst list */
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_burst_list: ");
		for (i = 0; i < 21; i++) {
			for (j = 0; j < ARRAY_SIZE(ttype_burst_list_item); j++) {
				if (ttype_burst_val[i] == ttype_burst_list_item[j].key) {
					ret += snprintf(buf + ret, buf_len - ret, "%s,", ttype_burst_list_item[j].val);
				}
			}
		}
		snprintf(buf + ret -1, buf_len - ret + 1, "\n");

	}
	/* ttype enable */
	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_ttype_enable: %d,%d\n",ttype1_16_en, ttype17_21_en);


#if 1 /*SEDA 3.5*/

	ret += snprintf(buf + ret, buf_len - ret,
		"met-info [000] 0.0: met_emi_msel_ext: %x,%x,%x\n",
		msel_group_ext_val[0] & BM_MASTER_ALL,
		msel_group_ext_val[4] & BM_MASTER_ALL,
		msel_group_ext_val[5] & BM_MASTER_ALL);

	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_wsct_rw: ");

	for (i=0;i<WSCT_AMOUNT;i++) {
		if (wsct_rw_val[i] == BM_WSCT_RW_RWBOTH)
			ret += snprintf(buf + ret, buf_len - ret, "RW,");
		else if (wsct_rw_val[i] == BM_WSCT_RW_READONLY)
			ret += snprintf(buf + ret, buf_len - ret, "R,");
		else if (wsct_rw_val[i] == BM_WSCT_RW_WRITEONLY)
			ret += snprintf(buf + ret, buf_len - ret, "W,");
		else    /*disable*/
			ret += snprintf(buf + ret, buf_len - ret, "NONE,");
	}
	snprintf(buf + ret -1, buf_len - ret + 1, "\n");


	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_wsct_HPRI_DIS: ");
	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, buf_len - ret, "%d,",WSCT_HPRI_DIS[i]);
	}
	snprintf(buf + ret -1, buf_len - ret + 1, "\n");


	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_wsct_HPRI_SEL: ");
	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, buf_len - ret, "%x,",WSCT_HPRI_SEL[i]);
	}
	snprintf(buf + ret -1, buf_len - ret + 1, "\n");

	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_wsct_busid: ");
	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, buf_len - ret, "%x,",wsct_busid_val[i]);
	}
	snprintf(buf + ret -1, buf_len - ret + 1, "\n");


	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_emi_wsct_idMask: ");
	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, buf_len - ret, "%x,",wsct_idMask_val[i]);
	}
	snprintf(buf + ret -1, buf_len - ret + 1, "\n");

	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: wsct_chn_rank_sel: ");
	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, buf_len - ret, "%x,",wsct_chn_rank_sel_val[i]);
	}
	snprintf(buf + ret -1, buf_len - ret + 1, "\n");

	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: wsct_byte_bnd_dis: ");
	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, buf_len - ret, "%d,",wsct_byte_bnd_dis[i]);
	}
	snprintf(buf + ret -1, buf_len - ret + 1, "\n");


	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: wsct_byte_low_bnd: ");
	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, buf_len - ret, "%x,",wsct_byte_low_bnd_val[i]);
	}
	snprintf(buf + ret -1, buf_len - ret + 1, "\n");

	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: wsct_byte_up_bnd: ");
	for (i=0;i<WSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, buf_len - ret, "%x,",wsct_byte_up_bnd_val[i]);
	}
	snprintf(buf + ret -1, buf_len - ret + 1, "\n");

	ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: tsct_busid_enable: ");
	for (i=0;i<TSCT_AMOUNT;i++) {
		ret += snprintf(buf + ret, buf_len - ret, "%d,",tsct_busid_enable_val[i]);
	}
	snprintf(buf + ret -1, buf_len - ret + 1, "\n");

	/***************************** ttype ****************************************/
	if (ttype17_21_en == BM_TTYPE17_21_ENABLE) {

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: TTYPE_HPRI_SEL: ");
		for (i=0;i<BM_COUNTER_MAX;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",TTYPE_HPRI_SEL[i]);
		}
		snprintf(buf + ret -1, buf_len - ret + 1, "\n");

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ttype_idMask: ");
		for (i=0;i<BM_COUNTER_MAX;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",ttype_idMask_val[i]);
		}
		snprintf(buf + ret -1, buf_len - ret + 1, "\n");

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ttype_chn_rank_sel: ");
		for (i=0;i<BM_COUNTER_MAX;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",ttype_chn_rank_sel_val[i]);
		}
		snprintf(buf + ret -1, buf_len - ret + 1, "\n");

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ttype_byte_bnd_dis: ");
		for (i=0;i<BM_COUNTER_MAX;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%d,",ttype_byte_bnd_dis[i]);
		}
		snprintf(buf + ret -1, buf_len - ret + 1, "\n");

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ttype_byte_low_bnd_val: ");
		for (i=0;i<BM_COUNTER_MAX;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",ttype_byte_low_bnd_val[i]);
		}
		snprintf(buf + ret -1, buf_len - ret + 1, "\n");

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ttype_byte_up_bnd_val: ");
		for (i=0;i<BM_COUNTER_MAX;i++) {
			ret += snprintf(buf + ret, buf_len - ret, "%x,",ttype_byte_up_bnd_val[i]);
		}
		snprintf(buf + ret -1, buf_len - ret + 1, "\n");
	}
#endif

#ifdef EMI_NUM
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: EMI_NUM: %d\n", EMI_NUM);
#endif
	/*IP version*/
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: DRAMC_VER: %d\n", DRAMC_VER);

	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: EMI_VER: %d.%d\n", EMI_VER_MAJOR, EMI_VER_MINOR);

#if 1 /* SEDA3.5 header print move to AP side */
/*#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT*/
	dram_chann_num = MET_EMI_GetDramChannNum(0);
	/*	met_dram_chann_num_header
	 *	channel number
	 *	LP4: 2, LP3: 1
	 */

	/*
	 *	the ddr type define :
	 *	enum DDRTYPE {
	 *	TYPE_DDR1 = 1,
	 *	TYPE_LPDDR2,
	 *	TYPE_LPDDR3,
	 *	TYPE_PCDDR3,
	 *	TYPE_LPDDR4,
	 *	TYPE_LPDDR4X,
	 *	TYPE_LPDDR4P
	 *	};
	 */
	if (!get_cur_ddr_ratio_symbol){
		PR_BOOTMSG("[%s][%d]get_cur_ddr_ratio_symbol = NULL , use the TYPE_LPDDR4 setting\n", __func__, __LINE__);
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ##_EMI_warning: get_cur_ddr_ratio_symbol = NULL , use the TYPE_LPDDR4 setting\n");
	}

	if (mtk_dramc_get_ddr_type_symbol) {
		DRAM_TYPE = mtk_dramc_get_ddr_type_symbol();

		base_clock_rate = MET_EMI_Get_BaseClock_Rate();

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_dram_type: %d\n", DRAM_TYPE);

		if ((DRAM_TYPE == 5) || (DRAM_TYPE == 6) || (DRAM_TYPE == 7))
			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_dram_chann_num_header: %d,%d,%d,%d\n",
					dram_chann_num, base_clock_rate,
					DRAM_IO_BUS_WIDTH_LP4, DRAM_DATARATE);
		else
			ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_dram_chann_num_header: %d,%d,%d,%d\n",
					dram_chann_num, base_clock_rate,
					DRAM_IO_BUS_WIDTH_LP3, DRAM_DATARATE);
	} else {
		METERROR("[%s][%d]mtk_dramc_get_ddr_type_symbol = NULL , use the TYPE_LPDDR4 setting\n", __func__, __LINE__);
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ##_EMI_warning: mtk_dramc_get_ddr_type_symbol = NULL , use the TYPE_LPDDR4 setting\n");

		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: met_dram_chann_num_header: %d,%d,%d,%d\n",
					dram_chann_num, DDR_RATIO_DEFAULT,
					DRAM_IO_BUS_WIDTH_LP4, DRAM_DATARATE);
	}



	/* met_emi_clockrate */
	if (mtk_dramc_get_data_rate_symbol) {
		dram_data_rate_MHz = mtk_dramc_get_data_rate_symbol();
	} else {
		METERROR("mtk_dramc_get_data_rate_symbol = NULL\n");
		ret += snprintf(buf + ret, buf_len - ret, "met-info [000] 0.0: ##_EMI_warning: mtk_dramc_get_data_rate_symbol = NULL\n");
		dram_data_rate_MHz = DRAM_FREQ_DEFAULT;
	}

	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_dram_clockrate: %d\n",
			dram_data_rate_MHz);

	/* 1 : by ondiemet, 0: by pure linux */
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: ##_emi_use_ondiemet: 1,%X\n",
			get_sspm_support_feature());

	/*dram bank num*/
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_dram_rank_num_header: %u,%u\n", MET_EMI_GetDramRankNum(0),
				MET_EMI_GetDramRankNum(0));

	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_emi_header: TS0,TS1,GP0_WSCT,GP1_WSCT,GP2_WSCT,GP3_WSCT,");
	ret += snprintf(buf + ret, buf_len - ret,
			"M0_LATENCY,M1_LATENCY,M2_LATENCY,M3_LATENCY,M4_LATENCY,M5_LATENCY,M6_LATENCY,M7_LATENCY,");
	ret += snprintf(buf + ret, buf_len - ret,
			"M0_TRANS,M1_TRANS,M2_TRANS,M3_TRANS,M4_TRANS,M5_TRANS,M6_TRANS,M7_TRANS,");
	ret += snprintf(buf + ret, buf_len - ret,
			"BACT,BSCT,BCNT,WACT,DCM_CTRL,TACT,");

	for (i = 0; i < dram_chann_num; i++) {
		if (i != 0)
			ret += snprintf(buf + ret, buf_len - ret,
					",");
		ret += snprintf(buf + ret, buf_len - ret,
				"PageHit_%d,PageMiss_%d,InterBank_%d,Idle_%d,", i, i, i, i);
		ret += snprintf(buf + ret, buf_len - ret,
				"mr4_%d,refresh_pop_%d,freerun_26m_%d,", i, i, i);
		ret += snprintf(buf + ret, buf_len - ret,
				"read_bytes_%d,write_bytes_%d", i, i);
	}
	ret += snprintf(buf + ret, buf_len - ret, "\n");

	/*TSCT header*/
	if (emi_tsct_enable == 1) {
		ret += snprintf(buf + ret, buf_len - ret,
				"met-info [000] 0.0: ms_emi_tsct_header: ms_emi_tsct,");
		ret += snprintf(buf + ret, buf_len - ret,
				"tsct1,tsct2,tsct3\n");
	}

	/*MDCT header*/
	if (emi_mdct_enable == 1) {
		ret += snprintf(buf + ret, buf_len - ret,
				"met-info [000] 0.0: ms_emi_mdct_header: ms_emi_mdct,");
		ret += snprintf(buf + ret, buf_len - ret,
				"RD_ULTRA,RD_MDMCU\n");
	}



	/* met_bw_rgtor_header */
	ret += snprintf(buf + ret, buf_len - ret,	"met-info [000] 0.0: met_bw_rgtor_unit_header: %x\n", MET_EMI_Get_CONH_2ND(0));

	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: met_bw_rglr_header: metadata\n");


	/* DRAM DVFS header */
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: DRAM_DVFS_header: datarate(MHz)\n");

	/*PDIR met_dramc_header*/
	if (dramc_pdir_enable == 1 && DRAMC_VER >= 2 ) {
		ret += snprintf(buf + ret, buf_len - ret,
				"met-info [000] 0.0: met_dramc_header: ");
		for (i = 0; i < dram_chann_num; i++) {
			if (i != 0)
				ret += snprintf(buf + ret, buf_len - ret,
						",");
			ret += snprintf(buf + ret, buf_len - ret, "freerun_26m_%d,", i);
			ret += snprintf(buf + ret, buf_len - ret,
					"rk0_pre_sb_%d,rk0_pre_pd_%d,rk0_act_sb_%d,rk0_act_pd_%d,", i, i, i, i);
			ret += snprintf(buf + ret, buf_len - ret,
					"rk1_pre_sb_%d,rk1_pre_pd_%d,rk1_act_sb_%d,rk1_act_pd_%d,", i, i, i, i);
			ret += snprintf(buf + ret, buf_len - ret,
					"rk2_pre_sb_%d,rk2_pre_pd_%d,rk2_act_sb_%d,rk2_act_pd_%d", i, i, i, i);
		}
		ret += snprintf(buf + ret, buf_len - ret, "\n");
	}

	/* DRS header */
	ret += snprintf(buf + ret, buf_len - ret,
			"met-info [000] 0.0: emi_drs_header: ch0_RANK1_GP(%%),ch0_RANK1_SF(%%),ch0_ALL_SF(%%),ch1_RANK1_GP(%%),ch1_RANK1_SF(%%),ch1_ALL_SF(%%)\n");
#endif

	met_sspm_emi.header_read_again = 0;
	output_header_len = 0;
	output_str_len = 0;

	return ret;
}


#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
static int emi_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, help);
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
		ipi_buf[2] = EMI_VER_MAJOR << 24 | EMI_VER_MINOR << 16 | DRAMC_VER << 8 | 0;
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

	emi_create_header(header_str, MAX_HEADER_LEN);
}

static void ondiemet_emi_stop(void)
{
	if (!emi_inited)
		return;

	if (do_emi())
		emi_uninit();
}
#endif
#endif


struct metdevice met_sspm_emi = {
	.name			= "emi",
	.owner			= THIS_MODULE,
	.type			= MET_TYPE_BUS,
	.create_subfs		= met_emi_create,
	.delete_subfs		= met_emi_delete,
	.resume			= met_emi_resume,
#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
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
