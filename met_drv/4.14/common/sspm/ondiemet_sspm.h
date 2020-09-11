/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __ONDIEMET_SSPM_H
#define __ONDIEMET_SSPM_H

#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) && defined(ONDIEMET_SUPPORT)
#include "ondiemet.h"

#ifndef SSPM_VERSION_V2
#include "sspm_ipi.h"
#else
#include <sspm_ipi_id.h>
#endif /* SSPM_VERSION_V2 */
#include <linux/dma-mapping.h>

/* we may use IPI_ID_PLATFORM for mt6759 to reduce SRAM */
#ifndef SSPM_VERSION_V2
#ifndef IPI_ID_MET
/* #define IPI_ID_MET IPI_ID_TST1 */
#define IPI_ID_MET IPI_ID_PLATFORM
#endif
#endif /* SSPM_VERSION_V2 */
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
#define MET_OP            (1 << FUNC_BIT_SHIFT)
/* argu 0: start: 0x01; stop: 0x02; extract: 0x03 */
#define MET_OP_START        0x00000001
#define MET_OP_STOP         0x00000002
#define MET_OP_EXTRACT      0x00000003
#define MET_OP_FLUSH        0x00000004
#define MET_SR            (2 << FUNC_BIT_SHIFT) /* sample rate */
#define MET_MODULE        (3 << FUNC_BIT_SHIFT) /* module enable/disable */
#define MET_ARGU          (4 << FUNC_BIT_SHIFT) /* argument passing */
#define MET_ATTR          (5 << FUNC_BIT_SHIFT) /* attribute passing */
/* system memory information for on-die-met log data */
#define MET_BUFFER_INFO   (6 << FUNC_BIT_SHIFT)
#define MET_TIMESTAMP     (7 << FUNC_BIT_SHIFT) /* timestamp info */
#define MET_GPT           (8 << FUNC_BIT_SHIFT) /* GPT counter reading */
#define MET_REQ_AP2MD     (9 << FUNC_BIT_SHIFT) /* user defined command */
#define MET_RESP_AP2MD    (10 << FUNC_BIT_SHIFT) /* may no need */
/* mode: bit 15 - 0: */
/*  Bit 0: MD32 SRAM mode; Bit 1: System DRAM mode */
/*  value: 0: output to next level of storage; 1: loop in its own storage */
#define MET_DATA_MODE     (11 << FUNC_BIT_SHIFT) /* log output mode */
/* start/stop read data into MD32 SRAM buffer; both DMA and met_printf() */
#define MET_DATA_OP       (12 << FUNC_BIT_SHIFT) /* data read operation */
/* the following command is used for MD32 to AP */
#define MET_DUMP_BUFFER   (13 << FUNC_BIT_SHIFT)
#define MET_REQ_MD2AP     (14 << FUNC_BIT_SHIFT) /* user defined command */
#define MET_CLOSE_FILE    (15 << FUNC_BIT_SHIFT) /* Inform to close the SD file */
#define MET_RESP_MD2AP    (16 << FUNC_BIT_SHIFT)
#define MET_RUN_MODE      (17 << FUNC_BIT_SHIFT)

/* Note: the module ID and its bit pattern should be fixed as below */
/* DMA based module first */
enum {
	MID_PMQOS = 0,
	MID_VCORE_DVFS,
	MID_EMI,
	MID_THERMAL_CPU,
	MID_WALL_TIME,
	MID_CPU_DVFS,
	MID_GPU_DVFS,
	MID_PTPOD,
	MID_SPM,
	MID_PROFILE,
	MID_MET_TAG,
	MID_TS,
	MID_TS_ISR,
	MID_TS_LAST,
	MID_TS_ISR_LAST,
	MID_SRAM_INFO,
	MID_MET_STOP,
	MID_IOP_MON,
	MID_CPU_INFO_MAPPING,
	MID_SMI,
	MID_PMU,

	MID_COMMON = 0x1F
};

#define ID_PMQOS       (1 << MID_PMQOS)
#define ID_SMI         (1 << MID_SMI)
#define ID_EMI         (1 << MID_EMI)
#define ID_THERMAL_CPU (1 << MID_THERMAL_CPU)
#define ID_WALL_TIME   (1 << MID_WALL_TIME)
#define ID_CPU_DVFS    (1 << MID_CPU_DVFS)
#define ID_GPU_DVFS    (1 << MID_GPU_DVFS)
#define ID_VCORE_DVFS  (1 << MID_VCORE_DVFS)
#define ID_PTPOD       (1 << MID_PTPOD)
#define ID_SPM         (1 << MID_SPM)
#define ID_PROFILE     (1 << MID_PROFILE)
#define ID_COMMON      (1 << MID_COMMON)
#define ID_CPU_INFO_MAPPING      (1 << MID_CPU_INFO_MAPPING)
#define ID_SMI      (1 << MID_SMI)
#define ID_PMU      (1 << MID_PMU)

extern void ondiemet_extract(void);
extern void ondiemet_stop(void);
extern void ondiemet_start(void);

extern void start_sspm_ipi_recv_thread(void);
extern void stop_sspm_ipi_recv_thread(void);
extern int met_ipi_to_sspm_command(void *buffer, int slot,
						   unsigned int *retbuf, int retslot);
extern int met_ipi_to_sspm_command_async(void *buffer, int slot,
						   unsigned int *retbuf, int retslot);


extern unsigned int ondiemet_ipi_buf[];

/* extern phys_addr_t ondiemet_sspm_log_phy_addr, ondiemet_sspm_log_virt_addr; */
extern dma_addr_t ondiemet_sspm_log_phy_addr;

extern void *ondiemet_sspm_log_virt_addr;
extern uint32_t ondiemet_sspm_log_size;

extern int ondiemet_attr_init(struct device *dev);
extern int ondiemet_attr_uninit(struct device *dev);
extern int met_sspm_log_discard;

#define SSPM_LOG_FILE 0
#define SSPM_LOG_SRAM 1
#define SSPM_LOG_DRAM 2
extern int sspm_log_mode;
#define SSPM_RUN_NORMAL 0
#define SSPM_RUN_CONTINUOUS 1
extern int sspm_run_mode;

/* extern void sspm_get_buffer_info(void); */
extern int sspm_buf_available;
extern int sspm_buffer_dumping;

void sspm_flush(void);

#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT && ONDIEMET_SUPPORT */
#endif /* __ONDIEMET_SSPM_H */
