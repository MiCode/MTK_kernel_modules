/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __TINYSYS_COMMON_H__
#define __TINYSYS_COMMON_H__

/*****************************************************************************
 * headers
 *****************************************************************************/


/*****************************************************************************
 * define declaration
 *****************************************************************************/
/* MET IPI command definition: mbox 0 */
/* main func ID: bit[31-24]; sub func ID: bit[23-18]; argu 0: bit[17-0] */
#define MET_MAIN_ID_MASK        0xff000000 /* bit 31 - 24 */
#define MET_SUB_ID_MASK         0x00fc0000 /* bit 23 - 18 */
#define MET_ARGU0_MASK          0x0003ffff /* bit 17 - 0 */
#define FUNC_BIT_SHIFT          18
#define MID_BIT_SHIFT           9
#define MET_MAIN_ID             0x06000000
/* handle argument and attribute */
#define PROCESS_ARGU            0x01
#define PROCESS_ATTR            0x02
#define MODULE_ID_MASK          0x3fe00 /* bit 9 - 17 */
#define ARGUMENT_MASK           0x01ff  /* bit 0 - 8 */

/* the following command is used for AP to MD32 */
/* argu 0: start: 0x01; stop: 0x02; extract: 0x03 */
#define MET_OP_START            0x00000001
#define MET_OP_STOP             0x00000002
#define MET_OP_EXTRACT          0x00000003
#define MET_OP_FLUSH            0x00000004
#define MET_OP                  (1 << FUNC_BIT_SHIFT)
#define MET_SR                  (2 << FUNC_BIT_SHIFT) /* sample rate */
#define MET_MODULE              (3 << FUNC_BIT_SHIFT) /* module enable/disable */
#define MET_ARGU                (4 << FUNC_BIT_SHIFT) /* argument passing */
#define MET_ATTR                (5 << FUNC_BIT_SHIFT) /* attribute passing */
/* system memory information for on-die-met log data */
#define MET_BUFFER_INFO         (6 << FUNC_BIT_SHIFT)
#define MET_TIMESTAMP           (7 << FUNC_BIT_SHIFT) /* timestamp info */
#define MET_GPT                 (8 << FUNC_BIT_SHIFT) /* GPT counter reading */
#define MET_REQ_AP2MD           (9 << FUNC_BIT_SHIFT) /* user defined command */
#define MET_RESP_AP2MD          (10 << FUNC_BIT_SHIFT) /* may no need */
/* mode: bit 15 - 0: */
/*  Bit 0: MD32 SRAM mode; Bit 1: System DRAM mode */
/*  value: 0: output to next level of storage; 1: loop in its own storage */
#define MET_DATA_MODE           (11 << FUNC_BIT_SHIFT) /* log output mode */
/* start/stop read data into MD32 SRAM buffer; both DMA and met_printf() */
#define MET_DATA_OP             (12 << FUNC_BIT_SHIFT) /* data read operation */
/* the following command is used for MD32 to AP */
#define MET_DUMP_BUFFER         (13 << FUNC_BIT_SHIFT)
#define MET_REQ_MD2AP           (14 << FUNC_BIT_SHIFT) /* user defined command */
#define MET_CLOSE_FILE          (15 << FUNC_BIT_SHIFT) /* Inform to close the SD file */
#define MET_RESP_MD2AP          (16 << FUNC_BIT_SHIFT)
#define MET_RUN_MODE            (17 << FUNC_BIT_SHIFT)
#define MET_SSPM_ACK            (18 << FUNC_BIT_SHIFT)
#define MET_AP_ACK              (19 << FUNC_BIT_SHIFT)

#define ID_PMQOS                (1 << MID_PMQOS)
#define ID_SMI                  (1 << MID_SMI)
#define ID_EMI                  (1 << MID_EMI)
#define ID_THERMAL_CPU          (1 << MID_THERMAL_CPU)
#define ID_WALL_TIME            (1 << MID_WALL_TIME)
#define ID_CPU_DVFS             (1 << MID_CPU_DVFS)
#define ID_GPU_DVFS             (1 << MID_GPU_DVFS)
#define ID_VCORE_DVFS           (1 << MID_VCORE_DVFS)
#define ID_PTPOD                (1 << MID_PTPOD)
#define ID_SPM                  (1 << MID_SPM)
#define ID_PROFILE              (1 << MID_PROFILE)
#define ID_COMMON               (1 << MID_COMMON)
#define ID_CPU_INFO_MAPPING     (1 << MID_CPU_INFO_MAPPING)
#define ID_SMI                  (1 << MID_SMI)
#define ID_PMU                  (1 << MID_PMU)
#define ID_CPU_PMUE             (1 << MID_CPU_PMUE)


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/
/* Note: the module ID and its bit pattern should be fixed as below */
/* DMA based module first */
enum {
	MID_PMQOS = 0,
	MID_VCORE_DVFS = 1,
	MID_EMI = 2,
	MID_THERMAL_CPU = 3,
	MID_WALL_TIME = 4,
	MID_CPU_DVFS = 5,
	MID_GPU_DVFS = 6,
	MID_PTPOD = 7,
	MID_SPM = 8,
	MID_PROFILE = 9,
	MID_MET_TAG = 10,
	MID_TS = 11,
	MID_TS_ISR = 12,
	MID_TS_LAST = 13,
	MID_TS_ISR_LAST = 14,
	MID_SRAM_INFO = 15,
	MID_MET_STOP = 16,
	MID_IOP_MON = 17,
	MID_CPU_INFO_MAPPING = 18,
	MID_SMI = 19,
	MID_PMU = 20,
	MID_MCUPM_CPU_PMU = 21,
	MID_MCUPM_SENSOR_NETWORK = 22,
	MID_MCUPM_CPU_THERMAL = 23,
	MID_MCUPM_PTPOD = 24,
	MID_MCUPM_CPU_DVFS = 25,
	MID_MCUPM_SYSTEM_PI = 26,
	MID_CPU_PMUE = 0x1D,
	MID_SYS_PI = 0x1E,
	MID_COMMON = 0x1F
};


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/


#endif /* __TINYSYS_COMMON_H__ */