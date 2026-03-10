// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/ctype.h>

#ifdef USE_KERNEL_SYNC_WRITE_H
#include <mt-plat/sync_write.h>
#else
#include "sync_write.h"
#endif

#ifdef USE_KERNEL_MTK_IO_H
#include <mt-plat/mtk_io.h>
#else
#include "mtk_io.h"
#endif

#include "met_drv.h"
#include "trace.h"

#include "mtk_typedefs.h"
#include "sspm_mtk_smi.h"
#include "sspm_met_smi.h"
#include "sspm_met_smi_name.h"
#include "core_plf_trace.h"
#include "core_plf_init.h"
#include "interface.h"
#include "sspm/ondiemet_sspm.h"

#define MET_SMI_DEBUG		1
#define MET_SMI_BUF_SIZE	128
#define MET_SMI_DEBUGBUF_SIZE	512
#define NPORT_IN_PM		4

// SMI Encode -- Master
#ifdef SMI_MASTER_8BIT
// bit15~bit16
#define MET_SMI_BIT_REQ_LARB	15
// bit13~bit14
#define MET_SMI_BIT_REQ_COMM	13
// bit12:Parallel Mode */
#define MET_SMI_BIT_PM		12
// bit9~bit8:Destination */
#define MET_SMI_BIT_DST		10
/* bit5~bit4:Request Type */
#define MET_SMI_BIT_REQ		8
/* bit3~bit0:Master */
#define MET_SMI_BIT_MASTER	0
#else
// bit15~bit16
#define MET_SMI_BIT_REQ_LARB	15
// bit13~bit14
#define MET_SMI_BIT_REQ_COMM	13
// bit12:Parallel Mode */
#define MET_SMI_BIT_PM		12
// bit9~bit8:Destination */
#define MET_SMI_BIT_DST		8
/* bit5~bit4:Request Type */
#define MET_SMI_BIT_REQ		4
/* bit3~bit0:Master */
#define MET_SMI_BIT_MASTER	0
#endif

// SMI Encode -- Metadata
/* bit6~bit5:RW */
#define MET_SMI_BIT_RW		5
/* bit4~bit0:Port */
#define MET_SMI_BIT_PORT	0

/*
*declare smi: larb0-larbn:
*real define table in met_smi_name.h
*/
/*MET_SMI_DESC_DEFINE();*/
/**/

/*======================================================================*/
/*	Global variable definitions					*/
/*======================================================================*/
#define MAX_CONFIG_ARRAY_SIZE 20
struct metdevice met_sspm_smi;
struct met_smi_conf smi_conf_array[MAX_CONFIG_ARRAY_SIZE];
int smi_array_index;

//static unsigned int smi_met_larb_number = SMI_LARB_NUMBER;
static int count = SMI_LARB_NUMBER + SMI_COMM_NUMBER;
static struct kobject *kobj_smi;

/* Request type */
static unsigned int larb_req_type = SMI_REQ_ALL;
static unsigned int comm_req_type = SMI_REQ_ALL;

/* Parallel mode */
static unsigned int parallel_mode;

/* Read/Write type in parallel mode */
static int comm_pm_rw_type[SMI_COMM_NUMBER][NPORT_IN_PM];
/* Error message */
static char err_msg[MET_SMI_BUF_SIZE];
static char debug_msg[MET_SMI_DEBUGBUF_SIZE];

/*======================================================================*/
/*	KOBJ Declarations						*/
/*======================================================================*/
/* KOBJ: larb_req_type */
DECLARE_KOBJ_ATTR_INT(larb_req_type, larb_req_type);

/* KOBJ : comm_req_type */
DECLARE_KOBJ_ATTR_INT(comm_req_type, comm_req_type);

/* KOBJ : enable_parallel_mode */
DECLARE_KOBJ_ATTR_INT(enable_parallel_mode, parallel_mode);

/* KOBJ : pm_rwtypeX */
DECLARE_KOBJ_ATTR_STR_LIST_ITEM(
	pm_rwtype,
	KOBJ_ITEM_LIST(
		{SMI_READ_ONLY,		"READ"},
		{SMI_WRITE_ONLY,	"WRITE"}
	)
);
DECLARE_KOBJ_ATTR_STR_LIST(pm_rwtype1, comm_pm_rw_type[0][0], pm_rwtype);
DECLARE_KOBJ_ATTR_STR_LIST(pm_rwtype2, comm_pm_rw_type[0][1], pm_rwtype);
DECLARE_KOBJ_ATTR_STR_LIST(pm_rwtype3, comm_pm_rw_type[0][2], pm_rwtype);
DECLARE_KOBJ_ATTR_STR_LIST(pm_rwtype4, comm_pm_rw_type[0][3], pm_rwtype);

/* KOBJ : count */
DECLARE_KOBJ_ATTR_RO_INT(count, count);

/* KOBJ : err_msg */
DECLARE_KOBJ_ATTR_RO_STR(err_msg, err_msg);

#define KOBJ_ATTR_LIST \
do { \
	KOBJ_ATTR_ITEM(larb_req_type); \
	KOBJ_ATTR_ITEM(comm_req_type); \
	KOBJ_ATTR_ITEM(enable_parallel_mode); \
	KOBJ_ATTR_ITEM(pm_rwtype1); \
	KOBJ_ATTR_ITEM(pm_rwtype2); \
	KOBJ_ATTR_ITEM(pm_rwtype3); \
	KOBJ_ATTR_ITEM(pm_rwtype4); \
	KOBJ_ATTR_ITEM(count); \
	KOBJ_ATTR_ITEM(err_msg); \
} while (0)

/*======================================================================*/
/*	SMI Operations							*/
/*======================================================================*/
static void met_smi_debug(char *debug_log)
{
	MET_TRACE("%s", debug_log);
}

static int do_smi(void)
{
	return met_sspm_smi.mode;
}

static void smi_init_value(void)
{
	int i;

	smi_array_index = 0;
	for (i = 0; i < MAX_CONFIG_ARRAY_SIZE; i++) {
		smi_conf_array[i].master = 0;
		smi_conf_array[i].port[0] = -1;
		smi_conf_array[i].port[1] = -1;
		smi_conf_array[i].port[2] = -1;
		smi_conf_array[i].port[3] = -1;
		smi_conf_array[i].rwtype[0] = SMI_RW_ALL;
		smi_conf_array[i].rwtype[1] = SMI_RW_ALL;
		smi_conf_array[i].rwtype[2] = SMI_RW_ALL;
		smi_conf_array[i].rwtype[3] = SMI_RW_ALL;
		smi_conf_array[i].desttype = SMI_DEST_EMI;
		smi_conf_array[i].reqtype = SMI_REQ_ALL;
	}
}

static int smi_init(void)
{
	int i;

	if (smi_array_index < MAX_CONFIG_ARRAY_SIZE) {
		for (i = 0; i < (smi_array_index + 1); i++) {
			snprintf(debug_msg, MET_SMI_DEBUGBUF_SIZE,
				"===SMI config: parallel_mode = %d, master = %d, port0 = %d, port1 = %d, port2 = %d, port3 = %d, rwtype0 = %d, rwtype1 = %d, rwtype2 = %d, rwtype3 = %d, desttype = %d, reqtype(larb) = %d, reqtype(comm) = %d\n",
				parallel_mode,
				smi_conf_array[i].master,
				smi_conf_array[i].port[0],
				smi_conf_array[i].port[1],
				smi_conf_array[i].port[2],
				smi_conf_array[i].port[3],
				smi_conf_array[i].rwtype[0],
				smi_conf_array[i].rwtype[1],
				smi_conf_array[i].rwtype[2],
				smi_conf_array[i].rwtype[3],
				smi_conf_array[i].desttype,
				smi_conf_array[i].reqtype >> MET_SMI_BIT_REQ_LARB,
				smi_conf_array[i].reqtype >> MET_SMI_BIT_REQ_COMM);
			met_smi_debug(debug_msg);
		}
	} else {
		met_smi_debug("smi_init() FAIL : smi_array_index overflow\n");
		return -1;
	}

	return 0;
}

static int met_smi_create(struct kobject *parent)
{
#define	KOBJ_ATTR_ITEM(attr_name) \
do { \
	ret = sysfs_create_file(kobj_smi, &attr_name##_attr.attr); \
	if (ret != 0) { \
		pr_debug("Failed to create " #attr_name " in sysfs\n"); \
		return ret; \
	} \
} while (0)

	int	j;
	int	ret = 0;

	pr_debug(" met_smi_create\n  met_smi_create\n  met_smi_create\n  met_smi_create\n  met_smi_create\n  met_smi_create\n  met_smi_create\n  met_smi_create\n");

	/* Init. */

	smi_init_value();

	larb_req_type = SMI_REQ_ALL;
	comm_req_type = SMI_REQ_ALL;
	parallel_mode = 0;

	for (j = 0; j < NPORT_IN_PM; j++)
		comm_pm_rw_type[0][j] = SMI_READ_ONLY;

	kobj_smi = parent;

	KOBJ_ATTR_LIST;

	return ret;

#undef	KOBJ_ATTR_ITEM
}

void met_smi_delete(void)
{
#define	KOBJ_ATTR_ITEM(attr_name) \
	sysfs_remove_file(kobj_smi, &attr_name##_attr.attr)


	if (kobj_smi != NULL) {
		KOBJ_ATTR_LIST;
		kobj_smi = NULL;
	}

#undef	KOBJ_ATTR_ITEM
}

static void met_smi_start(void)
{
	if (do_smi()) {
		if (smi_init() != 0) {
			met_sspm_smi.mode = 0;
			smi_init_value();
			return;
		}
	}
}

static void met_smi_stop(void)
{
	int j;

	if (do_smi()) {

		/* Reset */
		smi_init_value();

		larb_req_type = SMI_REQ_ALL;
		comm_req_type = SMI_REQ_ALL;
		parallel_mode = 0;

		for (j = 0; j < NPORT_IN_PM; j++)
			comm_pm_rw_type[0][j] = SMI_READ_ONLY;


		met_sspm_smi.mode = 0;
	}
	return ;
}

static char help[] =
"  --smi=master:port:rw:dest:bus         monitor specified SMI banwidth\n"
"  --smi=master:p1[:p2][:p3][:p4]        monitor parallel mode\n";

static int smi_print_help(char *buf, int len)
{
	return snprintf(buf, PAGE_SIZE, "%s", help);
}

static int get_num(const char *__restrict__ dc, int *pValue)
{
	int	value = 0, digit;
	int	i = 0;

	while (((*dc) >= '0') && ((*dc) <= '9')) {
		digit = (int)(*dc - '0');
		value = value * 10 + digit;
		dc++;
		i++;
	}

	if (i == 0)
		return 0;
	*pValue = value;

	return i;
}

/*
 * There are serveal cases as follows:
 *
 * 1. "met-cmd --start --smi=master:port:rwtype:desttype:bustype" => Can assign multiple master
 * 2. "met-cmd --start --smi=master:port[:port1][:port2][:port3]" ==> parael mode
 *
 */
static int smi_process_argument(const char *__restrict__ arg, int len)
{
	int	args[5];
	int	i, array_index;
	int	idx;
	unsigned int smi_conf_index = 0;
	struct met_smi_conf smi_conf;

	uint32_t ipi_buf[4];
	uint32_t ret;
	uint32_t rdata;
	uint16_t sspm_master = 0;
	uint32_t sspm_meta = 0;

	if (len < 3)
		return -1;

	/*reset local config structure*/
	memset(err_msg, 0, MET_SMI_BUF_SIZE);
	for (i = 0; i < 4; i++) {
		smi_conf.port[i] = -1;
		smi_conf.rwtype[i] = SMI_RW_ALL;
	}
	smi_conf.master = 0;
	smi_conf.reqtype = SMI_REQ_ALL;
	smi_conf.desttype = SMI_DEST_EMI;

	if (met_sspm_smi.mode != 0 && met_sspm_smi.mode != 1)
		return -1;

	/*
	 * parse arguments
	 * arg[0] = master
	 * arg[1] = port or port1
	 * arg[2] = rwtype or port2
	 * arg[3] = desttype or port3
	 * arg[4] = bustype or port4
	 */
	for (i = 0; i < ARRAY_SIZE(args); i++)
		args[i] = -1;
	idx = 0;
	for (i = 0; i < ARRAY_SIZE(args); i++) {
		ret = get_num(&(arg[idx]), &(args[i]));
		if (ret == 0)
			break;
		idx += ret;
		if (arg[idx] != ':')
			break;
		idx++;
	}

	pr_debug("===SMI process argu: args[0](%d), args[1](%d), args[2](%d), args[3](%d), args[4](%d)\n",
		args[0],
		args[1],
		args[2],
		args[3],
		args[4]);

	/*fill local config structure*/
	smi_conf.master = args[0];
	smi_conf.reqtype = (larb_req_type << MET_SMI_BIT_REQ_LARB) | (comm_req_type << MET_SMI_BIT_REQ_COMM);
	if (parallel_mode == 0) {			/*legacy mode*/
		smi_conf.rwtype[0] = args[2];
		smi_conf.desttype = args[3];
		smi_conf.port[0] = args[1];
	} else {							/*parallel mode*/
		smi_conf.desttype = SMI_DEST_EMI;
		for (i = 0; i < 4; i++) {
			if (args[i+1] < 0)
				break;
			smi_conf.port[i] = args[i+1];
			smi_conf.rwtype[i] = comm_pm_rw_type[0][i];
		}
	}

/*debug log to ftrace*/
#ifdef MET_SMI_DEBUG
	snprintf(debug_msg, MET_SMI_DEBUGBUF_SIZE,
		"(argu)===SMI process argu Master[%d]: parallel_mode = %d, master = %d, port0 = %d, port1 = %d, port2 = %d, port3 = %d, rwtype0 = %d, rwtype1 = %d, rwtype2 = %d, rwtype3 = %d, desttype = %d, reqtype(larb) = %d, reqtype(comm) = %d\n",
		args[0],
		parallel_mode,
		smi_conf.master,
		smi_conf.port[0],
		smi_conf.port[1],
		smi_conf.port[2],
		smi_conf.port[3],
		smi_conf.rwtype[0],
		smi_conf.rwtype[1],
		smi_conf.rwtype[2],
		smi_conf.rwtype[3],
		smi_conf.desttype,
		(smi_conf.reqtype >> MET_SMI_BIT_REQ_LARB) & 0x3,
		(smi_conf.reqtype >> MET_SMI_BIT_REQ_COMM) & 0x3);
		met_smi_debug(debug_msg);
#endif

	/*find the empty conf_array*/
	for (i = 0; i < MAX_CONFIG_ARRAY_SIZE; i++) {
		if ((smi_conf_array[i].master == smi_conf.master) && (smi_conf_array[i].port[0] != -1))
			break;
	}
	if (i >= MAX_CONFIG_ARRAY_SIZE) {
		if (smi_conf_array[0].port[0] == -1) {
			smi_array_index = 0;
			array_index = 0;
		} else {
			smi_array_index = smi_array_index + 1;
			array_index = smi_array_index;
		}
	} else {
		array_index = i;
	}

	if ((smi_array_index >= MAX_CONFIG_ARRAY_SIZE) || (array_index >= MAX_CONFIG_ARRAY_SIZE)) {
		snprintf(err_msg, MET_SMI_BUF_SIZE,
			"===Setting Master[%d]: check smi_array_index=%d, array_index=%d overflow (> %d)\n",
			args[0], smi_array_index, array_index, MAX_CONFIG_ARRAY_SIZE);
		return -1;
	}

	smi_conf_array[array_index].master = smi_conf.master;


	if (parallel_mode == 0) {	/* Legacy mode */
		smi_conf_array[array_index].port[0] = smi_conf.port[0];
	} else {					/* Parallel mode */
		for (i = 0; i < NPORT_IN_PM; i++) {
			if (smi_conf_array[array_index].port[i] == -1)
				smi_conf_array[array_index].port[i] = smi_conf.port[smi_conf_index++];
		}
	}
	smi_conf_array[array_index].rwtype[0] = smi_conf.rwtype[0];
	smi_conf_array[array_index].rwtype[1] = smi_conf.rwtype[1];
	smi_conf_array[array_index].rwtype[2] = smi_conf.rwtype[2];
	smi_conf_array[array_index].rwtype[3] = smi_conf.rwtype[3];
	smi_conf_array[array_index].desttype = smi_conf.desttype;
	smi_conf_array[array_index].reqtype = smi_conf.reqtype;

	pr_debug("===SMI process argu Master[%d]: parallel_mode = %d, master = %d, port0 = %d, port1 = %d, port2 = %d, port3 = %d, rwtype0 = %d, rwtype1 = %d, rwtype2 = %d, rwtype3 = %d, desttype = %d, reqtype(larb) = %d, reqtype(comm) = %d\n",
		args[0],
		parallel_mode,
		smi_conf.master,
		smi_conf.port[0],
		smi_conf.port[1],
		smi_conf.port[2],
		smi_conf.port[3],
		smi_conf.rwtype[0],
		smi_conf.rwtype[1],
		smi_conf.rwtype[2],
		smi_conf.rwtype[3],
		smi_conf.desttype,
		(smi_conf.reqtype >> MET_SMI_BIT_REQ_LARB) & 0x3,
		(smi_conf.reqtype >> MET_SMI_BIT_REQ_COMM) & 0x3);

	// Encode SMI config: Master (request format from SMI driver)
	sspm_master = sspm_master | (smi_conf_array[array_index].master << MET_SMI_BIT_MASTER);
	sspm_master = sspm_master | 0 << MET_SMI_BIT_REQ; //reqtype value will be update in sspm side
	sspm_master = sspm_master | (smi_conf_array[array_index].desttype << MET_SMI_BIT_DST);
	sspm_master = sspm_master | (parallel_mode << MET_SMI_BIT_PM);
	// Extrace info for larb and comm reqestType since of unable to recognize master belong to larb or comm.
	// BIT13~BIT14: comm reqtype
	// BIT15~BIT16: larb reqtype
	sspm_master = sspm_master | smi_conf_array[array_index].reqtype;


	// Encode SMI config: Meta (request format from SMI driver)
	// Encode 4 Metadata into 1 unsigned int
	// BIT0~BIT4: port
	// BIT5~BIT6: rwtype
	if(parallel_mode == 0){
		sspm_meta = sspm_meta | (smi_conf_array[array_index].port[0] << MET_SMI_BIT_PORT);
		sspm_meta = sspm_meta | (smi_conf_array[array_index].rwtype[0] << MET_SMI_BIT_RW);
	}
	else{
		for(i = 0; i < 4; i++){
			if(smi_conf_array[array_index].port[i] == 0xFFFFFFFF){
				smi_conf_array[array_index].port[i] = USHRT_MAX;
			}
			sspm_meta = sspm_meta | (smi_conf_array[array_index].port[i] << (MET_SMI_BIT_PORT+8*i));
			sspm_meta = sspm_meta | (smi_conf_array[array_index].rwtype[i] << (MET_SMI_BIT_RW+8*i));
		}
	}

	// Transfer to SSPM side
	if (sspm_buf_available == 1)
	{
		ipi_buf[0] = MET_MAIN_ID | MET_ARGU | MID_SMI << MID_BIT_SHIFT | 1;
		ipi_buf[1] = sspm_master;
		ipi_buf[2] = sspm_meta;
		ipi_buf[3] = 0;
		ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);

		/* Set mode */
		met_sspm_smi.mode = 1;
		ondiemet_module[ONDIEMET_SSPM] |= ID_SMI;
	}

	return 0;
}

struct metdevice met_sspm_smi = {
	.name						= "smi",
	.owner						= THIS_MODULE,
	.type						= MET_TYPE_BUS,
	.create_subfs				= met_smi_create,
	.delete_subfs				= met_smi_delete,
	.cpu_related				= 0,
	.ondiemet_mode 				= 1,
	.ondiemet_start				= met_smi_start,
	.ondiemet_stop				= met_smi_stop,
	.ondiemet_process_argument 	= smi_process_argument,
	.ondiemet_print_help		= smi_print_help,
};

