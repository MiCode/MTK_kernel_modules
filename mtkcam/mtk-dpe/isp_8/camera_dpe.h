/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2015 MediaTek Inc.
 */



#ifndef _MT_DPE_H
#define _MT_DPE_H

#include <linux/ioctl.h>

#include <linux/types.h>


#if IS_ENABLED(CONFIG_COMPAT)
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

/*
 *   enforce kernel log enable
 */
#define KERNEL_LOG /* enable debug log flag if defined */

#define _SUPPORT_MAX_DPE_FRAME_REQUEST_ 12 // 6
#define _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_ 4


#define SIG_ERESTARTSYS 512 /* ERESTARTSYS */
/*
 *
 */
#define DPE_DEV_MAJOR_NUMBER    302

#define DPE_MAGIC               'd'

#define DPE_REG_RANGE           (0x1000)

/* #define DPE_BASE_HW             0x3A770000 */

/*This macro is for setting irq status represnted
 * by a local variable,DPEInfo.IrqInfo.Status[DPE_IRQ_TYPE_INT_DPE_ST]
 */
#define DPE_INT_ST              (1UL<<31)

// MEDV buffer width size should be 64B align
// All other buffer width size should be 128B align
// Assume max width = 640 should meet above requirements
#define DPE_MAX_FRAME_SIZE 307200 //640x480
#define WB_ASFRM_SIZE DPE_MAX_FRAME_SIZE
#define WB_ASFRMExt_SIZE DPE_MAX_FRAME_SIZE
#define WB_WMFHF_SIZE DPE_MAX_FRAME_SIZE

#define WB_TOTAL_SIZE \
	(WB_ASFRM_SIZE+WB_ASFRMExt_SIZE+WB_WMFHF_SIZE)

// ----------------- DPE_DVS_ME  Grouping Definitions -------------------
struct DVS_ME_CFG {
	unsigned int DVS_ME_00;
	unsigned int DVS_ME_01;
	unsigned int DVS_ME_02;
	unsigned int DVS_ME_03;
	unsigned int DVS_ME_04;
	unsigned int DVS_ME_05;
	unsigned int DVS_ME_06;
	unsigned int DVS_ME_07;
	unsigned int DVS_ME_08;
	unsigned int DVS_ME_09;
	unsigned int DVS_ME_10;
	unsigned int DVS_ME_11;
	unsigned int DVS_ME_12;
	unsigned int DVS_ME_13;
	unsigned int DVS_ME_14;
	unsigned int DVS_ME_15;
	unsigned int DVS_ME_16;
	unsigned int DVS_ME_17;
	unsigned int DVS_ME_18;
	unsigned int DVS_ME_19;
	unsigned int DVS_ME_20;
	unsigned int DVS_ME_21;
	unsigned int DVS_ME_22;
	unsigned int DVS_ME_23;
	unsigned int DVS_ME_24;
	unsigned int DVS_ME_25;
	unsigned int DVS_ME_26;
	unsigned int DVS_ME_27;
	unsigned int DVS_ME_28;
	unsigned int DVS_ME_29;
	unsigned int DVS_ME_30;
	unsigned int DVS_ME_31;
	unsigned int DVS_ME_32;
	unsigned int DVS_ME_33;
	unsigned int DVS_ME_34;
	unsigned int DVS_ME_35;
	unsigned int DVS_ME_36;
	unsigned int DVS_ME_37;
	unsigned int DVS_ME_38;
	unsigned int DVS_ME_39;
	unsigned int DVS_DEBUG;
	unsigned int DVS_ME_RESERVED;
	unsigned int DVS_ME_ATPG;
	unsigned int DVS_ME_40;
	unsigned int DVS_ME_41;
	unsigned int DVS_ME_42;
	unsigned int DVS_ME_43;
	unsigned int DVS_ME_44;
	unsigned int DVS_ME_45;
	unsigned int DVS_ME_46;
	unsigned int DVS_ME_47;
	unsigned int DVS_ME_48;
	unsigned int DVS_ME_49;
	unsigned int DVS_ME_50;
	unsigned int DVS_ME_51;
	unsigned int DVS_ME_52;
	unsigned int DVS_ME_53;
	unsigned int DVS_ME_54;
	unsigned int DVS_ME_55;
	unsigned int DVS_ME_56;
	unsigned int DVS_ME_57;
	unsigned int DVS_ME_58;
	unsigned int DVS_ME_59;
};

struct DVS_ME_Kernel {
	unsigned int DVS_ME_00;
	unsigned int DVS_ME_01;
	unsigned int DVS_ME_02;
	unsigned int DVS_ME_03;
	unsigned int DVS_ME_04;
	unsigned int DVS_ME_05;
	unsigned int DVS_ME_06;
	unsigned int DVS_ME_07;
	unsigned int DVS_ME_08;
	unsigned int DVS_ME_09;
	unsigned int DVS_ME_10;
	unsigned int DVS_ME_11;
	unsigned int DVS_ME_12;
	unsigned int DVS_ME_13;
	unsigned int DVS_ME_14;
	unsigned int DVS_ME_15;
	unsigned int DVS_ME_16;
	unsigned int DVS_ME_17;
	unsigned int DVS_ME_18;
	unsigned int DVS_ME_19;
	unsigned int DVS_ME_20;
	unsigned int DVS_ME_21;
	unsigned int DVS_ME_22;
	unsigned int DVS_ME_23;
	unsigned int DVS_ME_24;
	unsigned int DVS_ME_25;
	unsigned int DVS_ME_26;
	unsigned int DVS_ME_27;
	unsigned int DVS_ME_28;
	unsigned int DVS_ME_29;
	unsigned int DVS_ME_30;
	unsigned int DVS_ME_31;
	unsigned int DVS_ME_32;
	unsigned int DVS_ME_33;
	unsigned int DVS_ME_34;
	unsigned int DVS_ME_35;
	unsigned int DVS_ME_36;
	unsigned int DVS_ME_37;
	unsigned int DVS_ME_38;
	unsigned int DVS_ME_39;
	unsigned int DVS_DEBUG;
	unsigned int DVS_ME_RESERVED;
	unsigned int DVS_ME_ATPG;
	unsigned int DVS_ME_40;
	unsigned int DVS_ME_41;
	unsigned int DVS_ME_42;
	unsigned int DVS_ME_43;
	unsigned int DVS_ME_44;
	unsigned int DVS_ME_45;
	unsigned int DVS_ME_46;
	unsigned int DVS_ME_47;
	unsigned int DVS_ME_48;
	unsigned int DVS_ME_49;
	unsigned int DVS_ME_50;
	unsigned int DVS_ME_51;
	unsigned int DVS_ME_52;
	unsigned int DVS_ME_53;
	unsigned int DVS_ME_54;
	unsigned int DVS_ME_55;
	unsigned int DVS_ME_56;
	unsigned int DVS_ME_57;
	unsigned int DVS_ME_58;
	unsigned int DVS_ME_59;
};



// ----------------- DPE_DVS_OCC  Grouping Definitions -------------------
struct DVS_OCC_CFG {
	unsigned int                DVS_OCC_PQ_0;
	unsigned int                DVS_OCC_PQ_1;
	unsigned int                DVS_OCC_PQ_2;
	unsigned int                DVS_OCC_PQ_3;
	unsigned int                DVS_OCC_PQ_4;
	unsigned int                DVS_OCC_PQ_5;
	unsigned int                DVS_OCC_PQ_10;
	unsigned int                DVS_OCC_PQ_11;
	unsigned int                DVS_OCC_PQ_12;
	unsigned int                DVS_OCC_ATPG;
	unsigned int                DVS_OCC_HIST0;
	unsigned int                DVS_OCC_HIST1;
	unsigned int                DVS_OCC_HIST2;
	unsigned int                DVS_OCC_HIST3;
	unsigned int                DVS_OCC_HIST4;
	unsigned int                DVS_OCC_HIST5;
	unsigned int                DVS_OCC_HIST6;
	unsigned int                DVS_OCC_HIST7;
	unsigned int                DVS_OCC_HIST8;
	unsigned int                DVS_OCC_HIST9;
	unsigned int                DVS_OCC_HIST10;
	unsigned int                DVS_OCC_HIST11;
	unsigned int                DVS_OCC_HIST12;
	unsigned int                DVS_OCC_HIST13;
	unsigned int                DVS_OCC_HIST14;
	unsigned int                DVS_OCC_HIST15;
	unsigned int                DVS_OCC_HIST16;
	unsigned int                DVS_OCC_HIST17;
	unsigned int                DVS_OCC_HIST18;
	unsigned int                DVS_OCC_LDV0;
};
struct DVS_OCC_Kernel {
	unsigned int                DVS_OCC_PQ_0;
	unsigned int                DVS_OCC_PQ_1;
	unsigned int                DVS_OCC_PQ_2;
	unsigned int                DVS_OCC_PQ_3;
	unsigned int                DVS_OCC_PQ_4;
	unsigned int                DVS_OCC_PQ_5;
	unsigned int                DVS_OCC_PQ_10;
	unsigned int                DVS_OCC_PQ_11;
	unsigned int                DVS_OCC_PQ_12;
	unsigned int                DVS_OCC_ATPG;
	unsigned int                DVS_OCC_HIST0;
	unsigned int                DVS_OCC_HIST1;
	unsigned int                DVS_OCC_HIST2;
	unsigned int                DVS_OCC_HIST3;
	unsigned int                DVS_OCC_HIST4;
	unsigned int                DVS_OCC_HIST5;
	unsigned int                DVS_OCC_HIST6;
	unsigned int                DVS_OCC_HIST7;
	unsigned int                DVS_OCC_HIST8;
	unsigned int                DVS_OCC_HIST9;
	unsigned int                DVS_OCC_HIST10;
	unsigned int                DVS_OCC_HIST11;
	unsigned int                DVS_OCC_HIST12;
	unsigned int                DVS_OCC_HIST13;
	unsigned int                DVS_OCC_HIST14;
	unsigned int                DVS_OCC_HIST15;
	unsigned int                DVS_OCC_HIST16;
	unsigned int                DVS_OCC_HIST17;
	unsigned int                DVS_OCC_HIST18;
	unsigned int                DVS_OCC_LDV0;
};
// ----------------- DPE_DVP_CTRL  Grouping Definitions -------------------
struct DVP_CORE_CFG {
	unsigned int                 DVP_CORE_00;         //3A770900
	unsigned int                 DVP_CORE_01;         //3A770904
	unsigned int                 DVP_CORE_02;         //3A770908
	unsigned int                 DVP_CORE_03;         //3A77090C
	unsigned int                 DVP_CORE_04;         //3A770910
	unsigned int                 DVP_CORE_05;         //3A770914
	unsigned int                 DVP_CORE_06;         //3A770918
	unsigned int                 DVP_CORE_07;         //3A77091C
	unsigned int                 DVP_CORE_08;         //3A770920
	unsigned int                 DVP_CORE_09;         //3A770924
	unsigned int                 DVP_CORE_10;         //3A770928
	unsigned int                 DVP_CORE_11;         //3A77092C
	unsigned int                 DVP_CORE_12;         //3A770930
	unsigned int                 DVP_CORE_13;         //3A770934
	unsigned int                 DVP_CORE_14;         //3A770938
	unsigned int                 DVP_CORE_15;         //3A77093C
	unsigned int                 DVP_CORE_16;         //3A770940
	unsigned int                 DVP_CORE_17;         //3A770944
	unsigned int                 DVP_CORE_18;         //3A770948
	unsigned int                 DVP_CORE_19;         //3A77094C
};

struct DVP_CORE_Kernel {
	unsigned int                 DVP_CORE_00;         //3A770900
	unsigned int                 DVP_CORE_01;         //3A770904
	unsigned int                 DVP_CORE_02;         //3A770908
	unsigned int                 DVP_CORE_03;         //3A77090C
	unsigned int                 DVP_CORE_04;         //3A770910
	unsigned int                 DVP_CORE_05;         //3A770914
	unsigned int                 DVP_CORE_06;         //3A770918
	unsigned int                 DVP_CORE_07;         //3A77091C
	unsigned int                 DVP_CORE_08;         //3A770920
	unsigned int                 DVP_CORE_09;         //3A770924
	unsigned int                 DVP_CORE_10;         //3A770928
	unsigned int                 DVP_CORE_11;         //3A77092C
	unsigned int                 DVP_CORE_12;         //3A770930
	unsigned int                 DVP_CORE_13;         //3A770934
	unsigned int                 DVP_CORE_14;         //3A770938
	unsigned int                 DVP_CORE_15;         //3A77093C
	unsigned int                 DVP_CORE_16;         //3A770940
	unsigned int                 DVP_CORE_17;         //3A770944
	unsigned int                 DVP_CORE_18;         //3A770948
	unsigned int                 DVP_CORE_19;         //3A77094C
};

struct DVGF_CORE_CFG {
	unsigned int  DVGF_CTRL_00; //DVGF
	unsigned int  DVGF_CTRL_01;
	unsigned int  DVGF_CTRL_02;
	unsigned int  DVGF_CTRL_03;
	unsigned int  DVGF_CTRL_05;
	unsigned int  DVGF_CTRL_07;
	unsigned int  DVGF_IRQ_00;
	unsigned int  DVGF_IRQ_01;
	unsigned int  DVGF_DRAM_PITCH;
	unsigned int  DVGF_DRAM_SEC_0;
	unsigned int  DVGF_DRAM_SEC_1;
	unsigned int DVGF_DRAM_AXSLC;
	unsigned int DVGF_CTRL_STATUS_32b_00;
	unsigned int DVGF_CTRL_STATUS_32b_01;
	unsigned int DVGF_CTRL_STATUS_32b_02;
	unsigned int DVGF_CTRL_STATUS_32b_03;
	unsigned int DVGF_CTRL_STATUS_32b_04;
	unsigned int  DVGF_CTRL_RESERVED;
	unsigned int  DVGF_CTRL_ATPG;
	unsigned int  DVGF_CORE_00;
	unsigned int  DVGF_CORE_01;
	unsigned int  DVGF_CORE_02;
	unsigned int  DVGF_CORE_03;
	unsigned int  DVGF_CORE_05;
	unsigned int  DVGF_CORE_06;
	unsigned int  DVGF_CORE_07;
	unsigned int  DVGF_CORE_08;
	unsigned int  DVGF_CORE_09;
	unsigned int  DVGF_CORE_10;
	unsigned int  DVGF_CORE_11;
	unsigned int  DVGF_CORE_12;
	unsigned int  DVGF_CORE_13;
	unsigned int  DVGF_CORE_14;
	unsigned int  DVGF_CORE_15;
	unsigned int  DVGF_CORE_16;
	unsigned int  DVGF_CORE_17;
	unsigned int  DVGF_CORE_18;
};
struct DVGF_CORE_Kernel {
	unsigned int  DVGF_CORE_00;
	unsigned int  DVGF_CORE_01;
	unsigned int  DVGF_CORE_02;
	unsigned int  DVGF_CORE_03;
	unsigned int  DVGF_CORE_05;
	unsigned int  DVGF_CORE_06;
	unsigned int  DVGF_CORE_07;
	unsigned int  DVGF_CORE_08;
	unsigned int  DVGF_CORE_09;
	unsigned int  DVGF_CORE_10;
	unsigned int  DVGF_CORE_11;
	unsigned int  DVGF_CORE_12;
	unsigned int  DVGF_CORE_13;
	unsigned int  DVGF_CORE_14;
	unsigned int  DVGF_CORE_15;
	unsigned int  DVGF_CORE_16;
	unsigned int  DVGF_CORE_17;
	unsigned int  DVGF_CORE_18;
};
// -----------------------------------------------------

struct DPE_REG_STRUCT {
	unsigned int module;
	unsigned int Addr;	/* register's addr */
	unsigned int Val;	/* register's value */
};

struct DPE_REG_IO_STRUCT {
	struct DPE_REG_STRUCT *pData;	/* pointer to DPE_REG_STRUCT */
	unsigned int Count;	/* count */
};

/*
 *   interrupt clear type
 */
enum DPE_IRQ_CLEAR_ENUM {
	DPE_IRQ_CLEAR_NONE,	/* non-clear wait, clear after wait */
	DPE_IRQ_CLEAR_WAIT,	/* clear wait, clear before and after wait */
	DPE_IRQ_WAIT_CLEAR,
	/* wait the signal and clear it, avoid hw executime is too s hort. */
	DPE_IRQ_CLEAR_STATUS,	/* clear specific status only */
	DPE_IRQ_CLEAR_ALL	/* clear all status */
};


/*
 *   module's interrupt , each module should have its own isr.
 *   note:
 *	mapping to isr table,ISR_TABLE when using no device tree
 */
enum DPE_IRQ_TYPE_ENUM {
	DPE_IRQ_TYPE_INT_DVP_ST,	/* DVP */
	DPE_IRQ_TYPE_INT_DVS_ST,	/* DVS */
	DPE_IRQ_TYPE_INT_DVGF_ST,	/* DVS */
	DPE_IRQ_TYPE_AMOUNT
};

struct DPE_WAIT_IRQ_STRUCT {
	enum DPE_IRQ_CLEAR_ENUM Clear;
	enum DPE_IRQ_TYPE_ENUM Type;
	unsigned int Status;	/*IRQ Status */
	unsigned int Timeout;
	int UserKey;		/* user key for doing interrupt operation */
	int ProcessID;		/* user ProcessID (will filled in kernel) */
	unsigned int bDumpReg;	/* check dump register or not */
};

struct DPE_CLEAR_IRQ_STRUCT {
	enum DPE_IRQ_TYPE_ENUM Type;
	int UserKey;		/* user key for doing interrupt operation */
	unsigned int Status;	/* Input */
};

enum dpe_token_type {
	token_none,
	token_set,
	token_wait
};

struct dpe_token_info {
	enum dpe_token_type d_token;
	unsigned int token_id;
};

struct DPE_Kernel_Config {
	struct DVS_ME_Kernel TuningBuf_ME;
	struct DVS_OCC_Kernel TuningKernel_OCC;
	struct DVP_CORE_Kernel TuningKernel_DVP;
	struct DVGF_CORE_Kernel TuningKernel_DVGF;
	unsigned int	DVS_CTRL00; //DVS
	unsigned int	DVS_CTRL01;
	unsigned int	DVS_CTRL02;
	unsigned int	DVS_CTRL03;
	unsigned int	DVS_CTRL06;
	unsigned int	DVS_CTRL07;
	unsigned int	DVS_CTRL08;
	unsigned int	DVS_CTRL_STATUS3;
	unsigned int	DVS_IRQ_00;
	unsigned int	DVS_IRQ_01;
	unsigned int	DVS_CTRL_STATUS0;
	unsigned int	DVS_CTRL_STATUS2;
	unsigned int	DVS_IRQ_STATUS;
	unsigned int	DVS_FRM_STATUS0;
	unsigned int	DVS_FRM_STATUS1;
	unsigned int	DVS_FRM_STATUS2;
	unsigned int	DVS_FRM_STATUS3;
	unsigned int	DVS_FRM_STATUS4;
	unsigned int	DVS_EXT_STATUS0;
	unsigned int	DVS_EXT_STATUS1;
	unsigned int	DVS_CUR_STATUS;
	unsigned int	DVS_SRC_CTRL;
	unsigned int	DVS_CRC_CTRL;
	unsigned int	DVS_CRC_IN;
	unsigned int	DVS_DRAM_STA0;
	unsigned int	DVS_DRAM_STA1;
	unsigned int	DVS_DRAM_ULT;
	unsigned int	DVS_DRAM_PITCH;
	unsigned int	DVS_DRAM_PITCH1;
	unsigned int	DVS_SRC_00;
	unsigned int	DVS_SRC_01;
	unsigned int	DVS_SRC_02;
	unsigned int	DVS_SRC_03;
	unsigned int	DVS_SRC_04;
	dma_addr_t	DVS_SRC_05_L_FRM0;
	dma_addr_t	DVS_SRC_06_L_FRM1;
	dma_addr_t	DVS_SRC_07_L_FRM2;
	dma_addr_t	DVS_SRC_08_L_FRM3;
	dma_addr_t	DVS_SRC_09_R_FRM0;
	dma_addr_t	DVS_SRC_10_R_FRM1;
	dma_addr_t	DVS_SRC_11_R_FRM2;
	dma_addr_t	DVS_SRC_12_R_FRM3;
	dma_addr_t	DVS_SRC_13_Hist0;
	dma_addr_t	DVS_SRC_14_Hist1;
	dma_addr_t	DVS_SRC_15_Hist2;
	dma_addr_t	DVS_SRC_16_Hist3;
	dma_addr_t	DVS_SRC_17_OCCDV_EXT0;
	dma_addr_t	DVS_SRC_18_OCCDV_EXT1;
	dma_addr_t	DVS_SRC_19_OCCDV_EXT2;
	dma_addr_t	DVS_SRC_20_OCCDV_EXT3;
	dma_addr_t	DVS_SRC_21_P4_L_DV_ADR;
	dma_addr_t	DVS_SRC_22_OCCDV0;
	dma_addr_t	DVS_SRC_23_OCCDV1;
	dma_addr_t	DVS_SRC_24_OCCDV2;
	dma_addr_t	DVS_SRC_25_OCCDV3;
	dma_addr_t	DVS_SRC_26_P4_R_DV_ADR;
	unsigned int	DVS_SRC_27;
	unsigned int	DVS_SRC_28;
	unsigned int	DVS_SRC_29;
	unsigned int	DVS_SRC_30;
	unsigned int	DVS_SRC_31;
	unsigned int	DVS_SRC_32;
	unsigned int	DVS_SRC_33;
	unsigned int	DVS_SRC_34;
	unsigned int	DVS_SRC_35;
	unsigned int	DVS_SRC_36;
	unsigned int	DVS_SRC_37;
	unsigned int	DVS_SRC_38;
	unsigned int	DVS_SRC_39;
	dma_addr_t	DVS_SRC_40_LDV_HIST0;
	dma_addr_t	DVS_SRC_41_LDV_HIST1;
	dma_addr_t	DVS_SRC_42_LDV_HIST2;
	dma_addr_t	DVS_SRC_43_LDV_HIST3;
	unsigned int	DVS_SRC_44;
	unsigned int	DVS_CRC_OUT_0;
	unsigned int	DVS_CRC_OUT_1;
	unsigned int	DVS_CRC_OUT_2;
	unsigned int	DVS_CRC_OUT_3;
	unsigned int  DVS_DRAM_SEC_0;
	unsigned int  DVS_DRAM_SEC_1;
	unsigned int  DVS_DRAM_SEC_2;
	unsigned int  DVS_DRAM_SEC_3;
	unsigned int  DVS_DRAM_AXSLC_0;
	unsigned int  DVS_DRAM_AXSLC_1;
	unsigned int  DVS_DEQ_FORCE;
	unsigned int	DVS_CTRL_RESERVED;
	unsigned int	DVS_CTRL_ATPG;
	unsigned int	DVP_CTRL00;  //DVP
	unsigned int	DVP_CTRL01;
	unsigned int	DVP_CTRL02;
	unsigned int	DVP_CTRL03;
	unsigned int	DVP_CTRL04;
	unsigned int	DVP_CTRL05;
	unsigned int	DVP_CTRL07;
	unsigned int	DVP_IRQ_00;
	unsigned int  DVP_IRQ_01;
	unsigned int	DVP_CTRL_STATUS0;
	unsigned int	DVP_CTRL_STATUS1;
	unsigned int	DVP_IRQ_STATUS;
	unsigned int	DVP_FRM_STATUS0;
	unsigned int	DVP_FRM_STATUS1;
	unsigned int	DVP_FRM_STATUS2;
	unsigned int	DVP_FRM_STATUS3;
	unsigned int	DVP_CUR_STATUS;
	unsigned int	DVP_SRC_00;
	unsigned int	DVP_SRC_01;
	unsigned int	DVP_SRC_02;
	unsigned int	DVP_SRC_03;
	unsigned int	DVP_SRC_04;
	dma_addr_t	DVP_SRC_05_Y_FRM0;
	dma_addr_t	DVP_SRC_06_Y_FRM1;
	dma_addr_t	DVP_SRC_07_Y_FRM2;
	dma_addr_t	DVP_SRC_08_Y_FRM3;
	dma_addr_t	DVP_SRC_09_C_FRM0;
	dma_addr_t	DVP_SRC_10_C_FRM1;
	dma_addr_t	DVP_SRC_11_C_FRM2;
	dma_addr_t	DVP_SRC_12_C_FRM3;
	dma_addr_t	DVP_SRC_13_OCCDV0;
	dma_addr_t	DVP_SRC_14_OCCDV1;
	dma_addr_t	DVP_SRC_15_OCCDV2;
	dma_addr_t	DVP_SRC_16_OCCDV3;
	dma_addr_t	DVP_SRC_17_CRM;
	dma_addr_t	DVP_SRC_18_ASF_RMDV;
	dma_addr_t  DVP_SRC_19_ASF_RDDV;
	dma_addr_t  DVP_SRC_20_ASF_DV0;
	dma_addr_t  DVP_SRC_21_ASF_DV1;
	dma_addr_t  DVP_SRC_22_ASF_DV2;
	dma_addr_t  DVP_SRC_23_ASF_DV3;
	dma_addr_t  DVP_SRC_24_WMF_RDDV;
	dma_addr_t  DVP_SRC_25_WMF_HFDV;
	dma_addr_t  DVP_SRC_26_WMF_DV0;
	dma_addr_t  DVP_SRC_27_WMF_DV1;
	dma_addr_t  DVP_SRC_28_WMF_DV2;
	dma_addr_t  DVP_SRC_29_WMF_DV3;
	unsigned int  DVP_SRC_CTRL;
	unsigned int DVP_CTRL_RESERVED;
	unsigned int DVP_CTRL_ATPG;
	unsigned int DVP_CRC_OUT_0;
	unsigned int DVP_CRC_OUT_1;
	unsigned int DVP_CRC_OUT_2;
	unsigned int DVP_CRC_CTRL;
	unsigned int DVP_CRC_OUT;
	unsigned int DVP_CRC_IN;
	unsigned int DVP_DRAM_STA;
	unsigned int DVP_DRAM_ULT;
	unsigned int DVP_DRAM_PITCH;
	unsigned int DVP_DRAM_SEC_0;
	unsigned int DVP_DRAM_SEC_1;
	unsigned int DVP_DRAM_AXSLC;
	unsigned int DVP_CORE_CRC_IN;
	dma_addr_t DVP_EXT_SRC_13_OCCDV0;
	dma_addr_t DVP_EXT_SRC_14_OCCDV1;
	dma_addr_t DVP_EXT_SRC_15_OCCDV2;
	dma_addr_t DVP_EXT_SRC_16_OCCDV3;
	dma_addr_t DVP_EXT_SRC_18_ASF_RMDV;
	dma_addr_t DVP_EXT_SRC_19_ASF_RDDV;
	dma_addr_t DVP_EXT_SRC_20_ASF_DV0;
	dma_addr_t DVP_EXT_SRC_21_ASF_DV1;
	dma_addr_t DVP_EXT_SRC_22_ASF_DV2;
	dma_addr_t DVP_EXT_SRC_23_ASF_DV3;
	dma_addr_t DVP_EXT_SRC_24_WMF_RD_DV_ADR;
	unsigned int  DVGF_CTRL_00; //DVGF
	unsigned int  DVGF_CTRL_01;
	unsigned int  DVGF_CTRL_02;
	unsigned int  DVGF_CTRL_03;
	unsigned int  DVGF_CTRL_05;
	unsigned int  DVGF_CTRL_07;
	unsigned int  DVGF_IRQ_00;
	unsigned int  DVGF_IRQ_01;
	unsigned int  DVGF_CTRL_STATUS0;
	unsigned int  DVGF_CTRL_STATUS1;
	unsigned int  DVGF_IRQ_STATUS;
	unsigned int  DVGF_FRM_STATUS;
	unsigned int  DVGF_CUR_STATUS;
	unsigned int  DVGF_CRC_CTRL;
	unsigned int  DVGF_CRC_OUT;
	unsigned int  DVGF_CRC_IN;
	unsigned int  DVGF_CRC_OUT_0;
	unsigned int  DVGF_CRC_OUT_1;
	unsigned int  DVGF_CRC_OUT_2;
	unsigned int  DVGF_CORE_CRC_IN;
	unsigned int  DVGF_DRAM_STA;
	unsigned int  DVGF_DRAM_PITCH;
	unsigned int  DVGF_DRAM_SEC_0;
	unsigned int  DVGF_DRAM_SEC_1;
	unsigned int  DVGF_DRAM_AXSLC;
	unsigned int  DVGF_CTRL_STATUS_32b_00;
	unsigned int  DVGF_CTRL_STATUS_32b_01;
	unsigned int  DVGF_CTRL_STATUS_32b_02;
	unsigned int  DVGF_CTRL_STATUS_32b_03;
	unsigned int  DVGF_CTRL_STATUS_32b_04;
	unsigned int  DVGF_CTRL_RESERVED;
	unsigned int  DVGF_CTRL_ATPG;
	dma_addr_t    DVGF_SRC_00;
	dma_addr_t    DVGF_SRC_01;
	dma_addr_t    DVGF_SRC_02;
	dma_addr_t    DVGF_SRC_04;
	dma_addr_t    DVGF_SRC_05;
	dma_addr_t    DVGF_SRC_06;
	dma_addr_t    DVGF_SRC_07;
	dma_addr_t    DVGF_SRC_08;
	dma_addr_t    DVGF_SRC_09;
	dma_addr_t    DVGF_SRC_10;
	dma_addr_t    DVGF_SRC_11;
	dma_addr_t    DVGF_SRC_12;
	dma_addr_t    DVGF_SRC_13;
	dma_addr_t    DVGF_SRC_14;
	dma_addr_t    DVGF_SRC_15;
	dma_addr_t    DVGF_SRC_16;
	dma_addr_t    DVGF_SRC_17;
	dma_addr_t    DVGF_SRC_18;
	dma_addr_t    DVGF_SRC_19;
	dma_addr_t    DVGF_SRC_20;
	dma_addr_t    DVGF_SRC_21;
	dma_addr_t    DVGF_SRC_22;
	dma_addr_t    DVGF_SRC_23;
	dma_addr_t    DVGF_SRC_24;
	unsigned int DPE_MODE;
};

struct DPE_Kernel_Dump {
	unsigned int	DVS_CTRL00; //DVS
	unsigned int	DVS_CTRL01;
	unsigned int	DVS_CTRL02;
	unsigned int	DVS_CTRL03;
	unsigned int	DVS_CTRL06;
	unsigned int	DVS_CTRL07;
	unsigned int	DVS_CTRL08;
	unsigned int	DVS_CTRL_STATUS3;
	unsigned int	DVS_IRQ_00;
	unsigned int	DVS_IRQ_01;
	unsigned int	DVS_CTRL_STATUS0;
	unsigned int	DVS_CTRL_STATUS2;
	unsigned int	DVS_IRQ_STATUS;
	unsigned int	DVS_FRM_STATUS0;
	unsigned int	DVS_FRM_STATUS1;
	unsigned int	DVS_FRM_STATUS2;
	unsigned int	DVS_FRM_STATUS3;
	unsigned int	DVS_FRM_STATUS4;
	unsigned int	DVS_EXT_STATUS0;
	unsigned int	DVS_EXT_STATUS1;
	unsigned int	DVS_CUR_STATUS;
	unsigned int	DVS_SRC_CTRL;
	unsigned int	DVS_CRC_CTRL;
	unsigned int	DVS_CRC_IN;
	unsigned int	DVS_DRAM_STA0;
	unsigned int	DVS_DRAM_STA1;
	unsigned int	DVS_DRAM_ULT;
	unsigned int	DVS_DRAM_PITCH;
	unsigned int	DVS_DRAM_PITCH1;
	unsigned int	DVS_SRC_00;
	unsigned int	DVS_SRC_01;
	unsigned int	DVS_SRC_02;
	unsigned int	DVS_SRC_03;
	unsigned int	DVS_SRC_04;
	unsigned int	DVS_SRC_05_L_FRM0;
	unsigned int	DVS_SRC_06_L_FRM1;
	unsigned int	DVS_SRC_07_L_FRM2;
	unsigned int	DVS_SRC_08_L_FRM3;
	unsigned int	DVS_SRC_09_R_FRM0;
	unsigned int	DVS_SRC_10_R_FRM1;
	unsigned int	DVS_SRC_11_R_FRM2;
	unsigned int	DVS_SRC_12_R_FRM3;
	unsigned int	DVS_SRC_13_Hist0;
	unsigned int	DVS_SRC_14_Hist1;
	unsigned int	DVS_SRC_15_Hist2;
	unsigned int	DVS_SRC_16_Hist3;
	unsigned int	DVS_SRC_17_OCCDV_EXT0;
	unsigned int	DVS_SRC_18_OCCDV_EXT1;
	unsigned int	DVS_SRC_19_OCCDV_EXT2;
	unsigned int	DVS_SRC_20_OCCDV_EXT3;
	unsigned int	DVS_SRC_21_P4_L_DV_ADR;
	unsigned int	DVS_SRC_22_OCCDV0;
	unsigned int	DVS_SRC_23_OCCDV1;
	unsigned int	DVS_SRC_24_OCCDV2;
	unsigned int	DVS_SRC_25_OCCDV3;
	unsigned int	DVS_SRC_26_P4_R_DV_ADR;
	unsigned int	DVS_SRC_27;
	unsigned int	DVS_SRC_28;
	unsigned int	DVS_SRC_29;
	unsigned int	DVS_SRC_30;
	unsigned int	DVS_SRC_31;
	unsigned int	DVS_SRC_32;
	unsigned int	DVS_SRC_33;
	unsigned int	DVS_SRC_34;
	unsigned int	DVS_SRC_35;
	unsigned int	DVS_SRC_36;
	unsigned int	DVS_SRC_37;
	unsigned int	DVS_SRC_38;
	unsigned int	DVS_SRC_39;
	unsigned int	DVS_SRC_40_LDV_HIST0;
	unsigned int	DVS_SRC_41_LDV_HIST1;
	unsigned int	DVS_SRC_42_LDV_HIST2;
	unsigned int	DVS_SRC_43_LDV_HIST3;
	unsigned int	DVS_SRC_44;
	unsigned int	DVS_CRC_OUT_0;
	unsigned int	DVS_CRC_OUT_1;
	unsigned int	DVS_CRC_OUT_2;
	unsigned int	DVS_CRC_OUT_3;
	unsigned int	DVS_DRAM_SEC_0;
	unsigned int	DVS_DRAM_SEC_1;
	unsigned int	DVS_DRAM_SEC_2;
	unsigned int	DVS_DRAM_SEC_3;
	unsigned int	DVS_DRAM_AXSLC_0;
	unsigned int	DVS_DRAM_AXSLC_1;
	unsigned int	DVS_DEQ_FORCE;
	unsigned int	DVS_CTRL_RESERVED;
	unsigned int	DVS_CTRL_ATPG;
	unsigned int	DVS_ME_00; //DVS_ME
	unsigned int	DVS_ME_01;
	unsigned int	DVS_ME_02;
	unsigned int	DVS_ME_03;
	unsigned int	DVS_ME_04;
	unsigned int	DVS_ME_05;
	unsigned int	DVS_ME_06;
	unsigned int	DVS_ME_07;
	unsigned int	DVS_ME_08;
	unsigned int	DVS_ME_09;
	unsigned int	DVS_ME_10;
	unsigned int	DVS_ME_11;
	unsigned int	DVS_ME_12;
	unsigned int	DVS_ME_13;
	unsigned int	DVS_ME_14;
	unsigned int	DVS_ME_15;
	unsigned int	DVS_ME_16;
	unsigned int	DVS_ME_17;
	unsigned int	DVS_ME_18;
	unsigned int	DVS_ME_19;
	unsigned int	DVS_ME_20;
	unsigned int	DVS_ME_21;
	unsigned int	DVS_ME_22;
	unsigned int	DVS_ME_23;
	unsigned int	DVS_ME_24;
	unsigned int	DVS_ME_25;
	unsigned int	DVS_ME_26;
	unsigned int	DVS_ME_27;
	unsigned int	DVS_ME_28;
	unsigned int	DVS_ME_29;
	unsigned int	DVS_ME_30;
	unsigned int	DVS_ME_31;
	unsigned int	DVS_ME_32;
	unsigned int	DVS_ME_33;
	unsigned int	DVS_ME_34;
	unsigned int	DVS_ME_35;
	unsigned int	DVS_ME_36;
	unsigned int	DVS_ME_37;
	unsigned int	DVS_ME_38;
	unsigned int	DVS_ME_39;
	unsigned int	DVS_DEBUG;
	unsigned int	DVS_ME_RESERVED;
	unsigned int	DVS_ME_ATPG;
	unsigned int	DVS_ME_40;
	unsigned int	DVS_ME_41;
	unsigned int	DVS_ME_42;
	unsigned int	DVS_ME_43;
	unsigned int	DVS_ME_44;
	unsigned int	DVS_ME_45;
	unsigned int	DVS_ME_46;
	unsigned int	DVS_ME_47;
	unsigned int	DVS_ME_48;
	unsigned int	DVS_ME_49;
	unsigned int	DVS_ME_50;
	unsigned int	DVS_ME_51;
	unsigned int	DVS_ME_52;
	unsigned int	DVS_ME_53;
	unsigned int	DVS_ME_54;
	unsigned int	DVS_ME_55;
	unsigned int	DVS_ME_56;
	unsigned int	DVS_ME_57;
	unsigned int	DVS_ME_58;
	unsigned int	DVS_ME_59;
	unsigned int	DVS_OCC_PQ_0; //DVS_OCC
	unsigned int	DVS_OCC_PQ_1;
	unsigned int	DVS_OCC_PQ_2;
	unsigned int	DVS_OCC_PQ_3;
	unsigned int	DVS_OCC_PQ_4;
	unsigned int	DVS_OCC_PQ_5;
	unsigned int	DVS_OCC_PQ_10;
	unsigned int	DVS_OCC_PQ_11;
	unsigned int	DVS_OCC_PQ_12;
	unsigned int	DVS_OCC_ATPG;
	unsigned int	DVS_OCC_HIST0;
	unsigned int	DVS_OCC_HIST1;
	unsigned int	DVS_OCC_HIST2;
	unsigned int	DVS_OCC_HIST3;
	unsigned int	DVS_OCC_HIST4;
	unsigned int	DVS_OCC_HIST5;
	unsigned int	DVS_OCC_HIST6;
	unsigned int	DVS_OCC_HIST7;
	unsigned int	DVS_OCC_HIST8;
	unsigned int	DVS_OCC_HIST9;
	unsigned int	DVS_OCC_HIST10;
	unsigned int	DVS_OCC_HIST11;
	unsigned int	DVS_OCC_HIST12;
	unsigned int	DVS_OCC_HIST13;
	unsigned int	DVS_OCC_HIST14;
	unsigned int	DVS_OCC_HIST15;
	unsigned int	DVS_OCC_HIST16;
	unsigned int	DVS_OCC_HIST17;
	unsigned int	DVS_OCC_HIST18;
	unsigned int	DVS_OCC_LDV0;
	unsigned int	DVP_CTRL00;  //DVP
	unsigned int	DVP_CTRL01;
	unsigned int	DVP_CTRL02;
	unsigned int	DVP_CTRL03;
	unsigned int	DVP_CTRL04;
	unsigned int	DVP_CTRL05;
	unsigned int	DVP_CTRL07;
	unsigned int	DVP_IRQ_00;
	unsigned int	DVP_IRQ_01;
	unsigned int	DVP_CTRL_STATUS0;
	unsigned int	DVP_CTRL_STATUS1;
	unsigned int	DVP_IRQ_STATUS;
	unsigned int	DVP_FRM_STATUS0;
	unsigned int	DVP_FRM_STATUS1;
	unsigned int	DVP_FRM_STATUS2;
	unsigned int	DVP_FRM_STATUS3;
	unsigned int	DVP_CUR_STATUS;
	unsigned int	DVP_SRC_00;
	unsigned int	DVP_SRC_01;
	unsigned int	DVP_SRC_02;
	unsigned int	DVP_SRC_03;
	unsigned int	DVP_SRC_04;
	unsigned int	DVP_SRC_05_Y_FRM0;
	unsigned int	DVP_SRC_06_Y_FRM1;
	unsigned int	DVP_SRC_07_Y_FRM2;
	unsigned int	DVP_SRC_08_Y_FRM3;
	unsigned int	DVP_SRC_09_C_FRM0;
	unsigned int	DVP_SRC_10_C_FRM1;
	unsigned int	DVP_SRC_11_C_FRM2;
	unsigned int	DVP_SRC_12_C_FRM3;
	unsigned int	DVP_SRC_13_OCCDV0;
	unsigned int	DVP_SRC_14_OCCDV1;
	unsigned int	DVP_SRC_15_OCCDV2;
	unsigned int	DVP_SRC_16_OCCDV3;
	unsigned int	DVP_SRC_17_CRM;
	unsigned int	DVP_SRC_18_ASF_RMDV;
	unsigned int	DVP_SRC_19_ASF_RDDV;
	unsigned int	DVP_SRC_20_ASF_DV0;
	unsigned int	DVP_SRC_21_ASF_DV1;
	unsigned int	DVP_SRC_22_ASF_DV2;
	unsigned int	DVP_SRC_23_ASF_DV3;
	unsigned int	DVP_SRC_24_WMF_RDDV;
	unsigned int	DVP_SRC_25_WMF_HFDV;
	unsigned int	DVP_SRC_26_WMF_DV0;
	unsigned int	DVP_SRC_27_WMF_DV1;
	unsigned int	DVP_SRC_28_WMF_DV2;
	unsigned int	DVP_SRC_29_WMF_DV3;
	unsigned int	DVP_CORE_00;
	unsigned int	DVP_CORE_01;
	unsigned int	DVP_CORE_02;
	unsigned int	DVP_CORE_03;
	unsigned int	DVP_CORE_04;
	unsigned int	DVP_CORE_05;
	unsigned int	DVP_CORE_06;
	unsigned int	DVP_CORE_07;
	unsigned int	DVP_CORE_08;
	unsigned int	DVP_CORE_09;
	unsigned int	DVP_CORE_10;
	unsigned int	DVP_CORE_11;
	unsigned int	DVP_CORE_12;
	unsigned int	DVP_CORE_13;
	unsigned int	DVP_CORE_14;
	unsigned int	DVP_CORE_15;
	unsigned int	DVP_CORE_16;
	unsigned int	DVP_CORE_17;
	unsigned int	DVP_CORE_18;
	unsigned int	DVP_CORE_19;
	unsigned int	DVP_SRC_CTRL;
	unsigned int	DVP_CTRL_RESERVED;
	unsigned int	DVP_CTRL_ATPG;
	unsigned int	DVP_CRC_OUT_0;
	unsigned int	DVP_CRC_OUT_1;
	unsigned int	DVP_CRC_OUT_2;
	unsigned int	DVP_CRC_CTRL;
	unsigned int	DVP_CRC_OUT;
	unsigned int	DVP_CRC_IN;
	unsigned int	DVP_DRAM_STA;
	unsigned int	DVP_DRAM_ULT;
	unsigned int	DVP_DRAM_PITCH;
	unsigned int	DVP_DRAM_SEC_0;
	unsigned int	DVP_DRAM_SEC_1;
	unsigned int	DVP_DRAM_AXSLC;
	unsigned int	DVP_CORE_CRC_IN;
	unsigned int	DVP_EXT_SRC_13_OCCDV0;
	unsigned int	DVP_EXT_SRC_14_OCCDV1;
	unsigned int	DVP_EXT_SRC_15_OCCDV2;
	unsigned int	DVP_EXT_SRC_16_OCCDV3;
	unsigned int	DVP_EXT_SRC_18_ASF_RMDV;
	unsigned int	DVP_EXT_SRC_19_ASF_RDDV;
	unsigned int	DVP_EXT_SRC_20_ASF_DV0;
	unsigned int	DVP_EXT_SRC_21_ASF_DV1;
	unsigned int	DVP_EXT_SRC_22_ASF_DV2;
	unsigned int	DVP_EXT_SRC_23_ASF_DV3;
	unsigned int	DVP_EXT_SRC_24_WMF_RDDV;
	unsigned int	DVGF_CTRL_00; //DVGF
	unsigned int	DVGF_CTRL_01;
	unsigned int	DVGF_CTRL_02;
	unsigned int	DVGF_CTRL_03;
	unsigned int	DVGF_CTRL_05;
	unsigned int	DVGF_CTRL_07;
	unsigned int	DVGF_IRQ_00;
	unsigned int	DVGF_IRQ_01;
	unsigned int	DVGF_CTRL_STATUS0;
	unsigned int	DVGF_CTRL_STATUS1;
	unsigned int	DVGF_IRQ_STATUS;
	unsigned int	DVGF_FRM_STATUS;
	unsigned int	DVGF_CUR_STATUS;
	unsigned int	DVGF_CRC_CTRL;
	unsigned int	DVGF_CRC_OUT;
	unsigned int	DVGF_CRC_IN;
	unsigned int	DVGF_CRC_OUT_0;
	unsigned int	DVGF_CRC_OUT_1;
	unsigned int	DVGF_CRC_OUT_2;
	unsigned int	DVGF_CORE_CRC_IN;
	unsigned int	DVGF_DRAM_STA;
	unsigned int	DVGF_DRAM_PITCH;
	unsigned int	DVGF_DRAM_SEC_0;
	unsigned int	DVGF_DRAM_SEC_1;
	unsigned int	DVGF_DRAM_AXSLC;
	unsigned int	DVGF_CTRL_STATUS_32b_00;
	unsigned int	DVGF_CTRL_STATUS_32b_01;
	unsigned int	DVGF_CTRL_STATUS_32b_02;
	unsigned int	DVGF_CTRL_STATUS_32b_03;
	unsigned int	DVGF_CTRL_STATUS_32b_04;
	unsigned int	DVGF_CTRL_RESERVED;
	unsigned int	DVGF_CTRL_ATPG;
	unsigned int	DVGF_SRC_00;
	unsigned int	DVGF_SRC_01;
	unsigned int	DVGF_SRC_02;
	unsigned int	DVGF_SRC_04;
	unsigned int	DVGF_SRC_05;
	unsigned int	DVGF_SRC_06;
	unsigned int	DVGF_SRC_07;
	unsigned int	DVGF_SRC_08;
	unsigned int	DVGF_SRC_09;
	unsigned int	DVGF_SRC_10;
	unsigned int	DVGF_SRC_11;
	unsigned int	DVGF_SRC_12;
	unsigned int	DVGF_SRC_13;
	unsigned int	DVGF_SRC_14;
	unsigned int	DVGF_SRC_15;
	unsigned int	DVGF_SRC_16;
	unsigned int	DVGF_SRC_17;
	unsigned int	DVGF_SRC_18;
	unsigned int	DVGF_SRC_19;
	unsigned int	DVGF_SRC_20;
	unsigned int	DVGF_SRC_21;
	unsigned int	DVGF_SRC_22;
	unsigned int	DVGF_SRC_23;
	unsigned int	DVGF_SRC_24;
	unsigned int	DVGF_CORE_00;
	unsigned int	DVGF_CORE_01;
	unsigned int	DVGF_CORE_02;
	unsigned int	DVGF_CORE_03;
	unsigned int	DVGF_CORE_05;
	unsigned int	DVGF_CORE_06;
	unsigned int	DVGF_CORE_07;
	unsigned int	DVGF_CORE_08;
	unsigned int	DVGF_CORE_09;
	unsigned int	DVGF_CORE_10;
	unsigned int	DVGF_CORE_11;
	unsigned int	DVGF_CORE_12;
	unsigned int	DVGF_CORE_13;
	unsigned int	DVGF_CORE_14;
	unsigned int	DVGF_CORE_15;
	unsigned int	DVGF_CORE_16;
	unsigned int	DVGF_CORE_17;
	unsigned int	DVGF_CORE_18;
	unsigned int	DPE_MODE;
};


struct DPE_meta_dvs {
	unsigned int	DVS_CTRL00;
	unsigned int	DVS_CTRL01;
	unsigned int	DVS_CTRL02;
	unsigned int	DVS_CTRL03;
	unsigned int	DVS_CTRL06;
	unsigned int	DVS_CTRL07;
	unsigned int	DVS_CTRL08;
	unsigned int	DVS_CTRL_STATUS3;
	unsigned int	DVS_IRQ_00;
	unsigned int	DVS_IRQ_01;
	unsigned int	DVS_CTRL_STATUS0;
	unsigned int	DVS_CTRL_STATUS2;
	unsigned int	DVS_IRQ_STATUS;
	unsigned int	DVS_FRM_STATUS0;
	unsigned int	DVS_FRM_STATUS1;
	unsigned int	DVS_FRM_STATUS2;
	unsigned int	DVS_FRM_STATUS3;
	unsigned int	DVS_FRM_STATUS4;
	unsigned int	DVS_EXT_STATUS0;
	unsigned int	DVS_EXT_STATUS1;
	unsigned int	DVS_CUR_STATUS;
	unsigned int	DVS_SRC_CTRL;
	unsigned int	DVS_CRC_CTRL;
	unsigned int	DVS_CRC_IN;
	unsigned int	DVS_DRAM_STA0;
	unsigned int	DVS_DRAM_STA1;
	unsigned int	DVS_DRAM_ULT;
	unsigned int	DVS_DRAM_PITCH;
	unsigned int	DVS_DRAM_PITCH1;
	unsigned int	DVS_SRC_00;
	unsigned int	DVS_SRC_01;
	unsigned int	DVS_SRC_02;
	unsigned int	DVS_SRC_03;
	unsigned int	DVS_SRC_04;
	unsigned int	DVS_SRC_05_L_FRM0;
	unsigned int	DVS_SRC_06_L_FRM1;
	unsigned int	DVS_SRC_07_L_FRM2;
	unsigned int	DVS_SRC_08_L_FRM3;
	unsigned int	DVS_SRC_09_R_FRM0;
	unsigned int	DVS_SRC_10_R_FRM1;
	unsigned int	DVS_SRC_11_R_FRM2;
	unsigned int	DVS_SRC_12_R_FRM3;
	unsigned int	DVS_SRC_13_Hist0;
	unsigned int	DVS_SRC_14_Hist1;
	unsigned int	DVS_SRC_15_Hist2;
	unsigned int	DVS_SRC_16_Hist3;
	unsigned int	DVS_SRC_17_OCCDV_EXT0;
	unsigned int	DVS_SRC_18_OCCDV_EXT1;
	unsigned int	DVS_SRC_19_OCCDV_EXT2;
	unsigned int	DVS_SRC_20_OCCDV_EXT3;
	unsigned int	DVS_SRC_21_P4_L_DV_ADR;
	unsigned int	DVS_SRC_22_OCCDV0;
	unsigned int	DVS_SRC_23_OCCDV1;
	unsigned int	DVS_SRC_24_OCCDV2;
	unsigned int	DVS_SRC_25_OCCDV3;
	unsigned int	DVS_SRC_26_P4_R_DV_ADR;
	unsigned int	DVS_SRC_27;
	unsigned int	DVS_SRC_28;
	unsigned int	DVS_SRC_29;
	unsigned int	DVS_SRC_30;
	unsigned int	DVS_SRC_31;
	unsigned int	DVS_SRC_32;
	unsigned int	DVS_SRC_33;
	unsigned int	DVS_SRC_34;
	unsigned int	DVS_SRC_35;
	unsigned int	DVS_SRC_36;
	unsigned int	DVS_SRC_37;
	unsigned int	DVS_SRC_38;
	unsigned int	DVS_SRC_39;
	unsigned int	DVS_SRC_40_LDV_HIST0;
	unsigned int	DVS_SRC_41_LDV_HIST1;
	unsigned int	DVS_SRC_42_LDV_HIST2;
	unsigned int	DVS_SRC_43_LDV_HIST3;
	unsigned int	DVS_SRC_44;
	unsigned int	DVS_CRC_OUT_0;
	unsigned int	DVS_CRC_OUT_1;
	unsigned int	DVS_CRC_OUT_2;
	unsigned int	DVS_CRC_OUT_3;
	unsigned int	DVS_DRAM_SEC_0;
	unsigned int	DVS_DRAM_SEC_1;
	unsigned int	DVS_DRAM_SEC_2;
	unsigned int	DVS_DRAM_SEC_3;
	unsigned int	DVS_DRAM_AXSLC_0;
	unsigned int	DVS_DRAM_AXSLC_1;
	unsigned int	DVS_DEQ_FORCE;
	unsigned int	DVS_CTRL_RESERVED;
	unsigned int	DVS_CTRL_ATPG;
	unsigned int	DVS_ME_00; //DVS_ME
	unsigned int	DVS_ME_01;
	unsigned int	DVS_ME_02;
	unsigned int	DVS_ME_03;
	unsigned int	DVS_ME_04;
	unsigned int	DVS_ME_05;
	unsigned int	DVS_ME_06;
	unsigned int	DVS_ME_07;
	unsigned int	DVS_ME_08;
	unsigned int	DVS_ME_09;
	unsigned int	DVS_ME_10;
	unsigned int	DVS_ME_11;
	unsigned int	DVS_ME_12;
	unsigned int	DVS_ME_13;
	unsigned int	DVS_ME_14;
	unsigned int	DVS_ME_15;
	unsigned int	DVS_ME_16;
	unsigned int	DVS_ME_17;
	unsigned int	DVS_ME_18;
	unsigned int	DVS_ME_19;
	unsigned int	DVS_ME_20;
	unsigned int	DVS_ME_21;
	unsigned int	DVS_ME_22;
	unsigned int	DVS_ME_23;
	unsigned int	DVS_ME_24;
	unsigned int	DVS_ME_25;
	unsigned int	DVS_ME_26;
	unsigned int	DVS_ME_27;
	unsigned int	DVS_ME_28;
	unsigned int	DVS_ME_29;
	unsigned int	DVS_ME_30;
	unsigned int	DVS_ME_31;
	unsigned int	DVS_ME_32;
	unsigned int	DVS_ME_33;
	unsigned int	DVS_ME_34;
	unsigned int	DVS_ME_35;
	unsigned int	DVS_ME_36;
	unsigned int	DVS_ME_37;
	unsigned int	DVS_ME_38;
	unsigned int	DVS_ME_39;
	unsigned int	DVS_DEBUG;
	unsigned int	DVS_ME_RESERVED;
	unsigned int	DVS_ME_ATPG;
	unsigned int	DVS_ME_40;
	unsigned int	DVS_ME_41;
	unsigned int	DVS_ME_42;
	unsigned int	DVS_ME_43;
	unsigned int	DVS_ME_44;
	unsigned int	DVS_ME_45;
	unsigned int	DVS_ME_46;
	unsigned int	DVS_ME_47;
	unsigned int	DVS_ME_48;
	unsigned int	DVS_ME_49;
	unsigned int	DVS_ME_50;
	unsigned int	DVS_ME_51;
	unsigned int	DVS_ME_52;
	unsigned int	DVS_ME_53;
	unsigned int	DVS_ME_54;
	unsigned int	DVS_ME_55;
	unsigned int	DVS_ME_56;
	unsigned int	DVS_ME_57;
	unsigned int	DVS_ME_58;
	unsigned int	DVS_ME_59;
	unsigned int	DVS_OCC_PQ_0; //DVS_OCC
	unsigned int	DVS_OCC_PQ_1;
	unsigned int	DVS_OCC_PQ_2;
	unsigned int	DVS_OCC_PQ_3;
	unsigned int	DVS_OCC_PQ_4;
	unsigned int	DVS_OCC_PQ_5;
	unsigned int	DVS_OCC_PQ_10;
	unsigned int	DVS_OCC_PQ_11;
	unsigned int	DVS_OCC_PQ_12;
	unsigned int	DVS_OCC_ATPG;
	unsigned int	DVS_OCC_HIST0;
	unsigned int	DVS_OCC_HIST1;
	unsigned int	DVS_OCC_HIST2;
	unsigned int	DVS_OCC_HIST3;
	unsigned int	DVS_OCC_HIST4;
	unsigned int	DVS_OCC_HIST5;
	unsigned int	DVS_OCC_HIST6;
	unsigned int	DVS_OCC_HIST7;
	unsigned int	DVS_OCC_HIST8;
	unsigned int	DVS_OCC_HIST9;
	unsigned int	DVS_OCC_HIST10;
	unsigned int	DVS_OCC_HIST11;
	unsigned int	DVS_OCC_HIST12;
	unsigned int	DVS_OCC_HIST13;
	unsigned int	DVS_OCC_HIST14;
	unsigned int	DVS_OCC_HIST15;
	unsigned int	DVS_OCC_HIST16;
	unsigned int	DVS_OCC_HIST17;
	unsigned int	DVS_OCC_HIST18;
	unsigned int	DVS_OCC_LDV0;
};

struct DPE_meta_dvp {
	unsigned int	DVP_CTRL00;  //DVP
	unsigned int	DVP_CTRL01;
	unsigned int	DVP_CTRL02;
	unsigned int	DVP_CTRL03;
	unsigned int	DVP_CTRL04;
	unsigned int	DVP_CTRL05;
	unsigned int	DVP_CTRL07;
	unsigned int	DVP_IRQ_00;
	unsigned int	DVP_IRQ_01;
	unsigned int	DVP_CTRL_STATUS0;
	unsigned int	DVP_CTRL_STATUS1;
	unsigned int	DVP_IRQ_STATUS;
	unsigned int	DVP_FRM_STATUS0;
	unsigned int	DVP_FRM_STATUS1;
	unsigned int	DVP_FRM_STATUS2;
	unsigned int	DVP_FRM_STATUS3;
	unsigned int	DVP_CUR_STATUS;
	unsigned int	DVP_SRC_00;
	unsigned int	DVP_SRC_01;
	unsigned int	DVP_SRC_02;
	unsigned int	DVP_SRC_03;
	unsigned int	DVP_SRC_04;
	unsigned int	DVP_SRC_05_Y_FRM0;
	unsigned int	DVP_SRC_06_Y_FRM1;
	unsigned int	DVP_SRC_07_Y_FRM2;
	unsigned int	DVP_SRC_08_Y_FRM3;
	unsigned int	DVP_SRC_09_C_FRM0;
	unsigned int	DVP_SRC_10_C_FRM1;
	unsigned int	DVP_SRC_11_C_FRM2;
	unsigned int	DVP_SRC_12_C_FRM3;
	unsigned int	DVP_SRC_13_OCCDV0;
	unsigned int	DVP_SRC_14_OCCDV1;
	unsigned int	DVP_SRC_15_OCCDV2;
	unsigned int	DVP_SRC_16_OCCDV3;
	unsigned int	DVP_SRC_17_CRM;
	unsigned int	DVP_SRC_18_ASF_RMDV;
	unsigned int	DVP_SRC_19_ASF_RDDV;
	unsigned int	DVP_SRC_20_ASF_DV0;
	unsigned int	DVP_SRC_21_ASF_DV1;
	unsigned int	DVP_SRC_22_ASF_DV2;
	unsigned int	DVP_SRC_23_ASF_DV3;
	unsigned int	DVP_SRC_24_WMF_RDDV;
	unsigned int	DVP_SRC_25_WMF_HFDV;
	unsigned int	DVP_SRC_26_WMF_DV0;
	unsigned int	DVP_SRC_27_WMF_DV1;
	unsigned int	DVP_SRC_28_WMF_DV2;
	unsigned int	DVP_SRC_29_WMF_DV3;
	unsigned int	DVP_CORE_00;
	unsigned int	DVP_CORE_01;
	unsigned int	DVP_CORE_02;
	unsigned int	DVP_CORE_03;
	unsigned int	DVP_CORE_04;
	unsigned int	DVP_CORE_05;
	unsigned int	DVP_CORE_06;
	unsigned int	DVP_CORE_07;
	unsigned int	DVP_CORE_08;
	unsigned int	DVP_CORE_09;
	unsigned int	DVP_CORE_10;
	unsigned int	DVP_CORE_11;
	unsigned int	DVP_CORE_12;
	unsigned int	DVP_CORE_13;
	unsigned int	DVP_CORE_14;
	unsigned int	DVP_CORE_15;
	unsigned int	DVP_CORE_16;
	unsigned int	DVP_CORE_17;
	unsigned int	DVP_CORE_18;
	unsigned int	DVP_CORE_19;
	unsigned int	DVP_SRC_CTRL;
	unsigned int	DVP_CTRL_RESERVED;
	unsigned int	DVP_CTRL_ATPG;
	unsigned int	DVP_CRC_OUT_0;
	unsigned int	DVP_CRC_OUT_1;
	unsigned int	DVP_CRC_OUT_2;
	unsigned int	DVP_CRC_CTRL;
	unsigned int	DVP_CRC_OUT;
	unsigned int	DVP_CRC_IN;
	unsigned int	DVP_DRAM_STA;
	unsigned int	DVP_DRAM_ULT;
	unsigned int	DVP_DRAM_PITCH;
	unsigned int	DVP_DRAM_SEC_0;
	unsigned int	DVP_DRAM_SEC_1;
	unsigned int	DVP_DRAM_AXSLC;
	unsigned int	DVP_CORE_CRC_IN;
	unsigned int	DVP_EXT_SRC_13_OCCDV0;
	unsigned int	DVP_EXT_SRC_14_OCCDV1;
	unsigned int	DVP_EXT_SRC_15_OCCDV2;
	unsigned int	DVP_EXT_SRC_16_OCCDV3;
	unsigned int	DVP_EXT_SRC_18_ASF_RMDV;
	unsigned int	DVP_EXT_SRC_19_ASF_RDDV;
	unsigned int	DVP_EXT_SRC_20_ASF_DV0;
	unsigned int	DVP_EXT_SRC_21_ASF_DV1;
	unsigned int	DVP_EXT_SRC_22_ASF_DV2;
	unsigned int	DVP_EXT_SRC_23_ASF_DV3;
	unsigned int	DVP_EXT_SRC_24_WMF_RD_DV_ADR;
};

struct DPE_meta_dvgf {
	unsigned int	DVGF_CTRL_00; //DVGF
	unsigned int	DVGF_CTRL_01;
	unsigned int	DVGF_CTRL_02;
	unsigned int	DVGF_CTRL_03;
	unsigned int	DVGF_CTRL_05;
	unsigned int	DVGF_CTRL_07;
	unsigned int	DVGF_IRQ_00;
	unsigned int	DVGF_IRQ_01;
	unsigned int	DVGF_CTRL_STATUS0;
	unsigned int	DVGF_CTRL_STATUS1;
	unsigned int	DVGF_IRQ_STATUS;
	unsigned int	DVGF_FRM_STATUS;
	unsigned int	DVGF_CUR_STATUS;
	unsigned int	DVGF_CRC_CTRL;
	unsigned int	DVGF_CRC_OUT;
	unsigned int	DVGF_CRC_IN;
	unsigned int	DVGF_CRC_OUT_0;
	unsigned int	DVGF_CRC_OUT_1;
	unsigned int	DVGF_CRC_OUT_2;
	unsigned int	DVGF_CORE_CRC_IN;
	unsigned int	DVGF_DRAM_STA;
	unsigned int	DVGF_DRAM_PITCH;
	unsigned int	DVGF_DRAM_SEC_0;
	unsigned int	DVGF_DRAM_SEC_1;
	unsigned int	DVGF_DRAM_AXSLC;
	unsigned int	DVGF_CTRL_STATUS_32b_00;
	unsigned int	DVGF_CTRL_STATUS_32b_01;
	unsigned int	DVGF_CTRL_STATUS_32b_02;
	unsigned int	DVGF_CTRL_STATUS_32b_03;
	unsigned int	DVGF_CTRL_STATUS_32b_04;
	unsigned int	DVGF_CTRL_RESERVED;
	unsigned int	DVGF_CTRL_ATPG;
	unsigned int	DVGF_SRC_00;
	unsigned int	DVGF_SRC_01;
	unsigned int	DVGF_SRC_02;
	unsigned int	DVGF_SRC_04;
	unsigned int	DVGF_SRC_05;
	unsigned int	DVGF_SRC_06;
	unsigned int	DVGF_SRC_07;
	unsigned int	DVGF_SRC_08;
	unsigned int	DVGF_SRC_09;
	unsigned int	DVGF_SRC_10;
	unsigned int	DVGF_SRC_11;
	unsigned int	DVGF_SRC_12;
	unsigned int	DVGF_SRC_13;
	unsigned int	DVGF_SRC_14;
	unsigned int	DVGF_SRC_15;
	unsigned int	DVGF_SRC_16;
	unsigned int	DVGF_SRC_17;
	unsigned int	DVGF_SRC_18;
	unsigned int	DVGF_SRC_19;
	unsigned int	DVGF_SRC_20;
	unsigned int	DVGF_SRC_21;
	unsigned int	DVGF_SRC_22;
	unsigned int	DVGF_SRC_23;
	unsigned int	DVGF_SRC_24;
	unsigned int	DVGF_CORE_00;
	unsigned int	DVGF_CORE_01;
	unsigned int	DVGF_CORE_02;
	unsigned int	DVGF_CORE_03;
	unsigned int	DVGF_CORE_05;
	unsigned int	DVGF_CORE_06;
	unsigned int	DVGF_CORE_07;
	unsigned int	DVGF_CORE_08;
	unsigned int	DVGF_CORE_09;
	unsigned int	DVGF_CORE_10;
	unsigned int	DVGF_CORE_11;
	unsigned int	DVGF_CORE_12;
	unsigned int	DVGF_CORE_13;
	unsigned int	DVGF_CORE_14;
	unsigned int	DVGF_CORE_15;
	unsigned int	DVGF_CORE_16;
	unsigned int	DVGF_CORE_17;
	unsigned int	DVGF_CORE_18;
};

enum DPEMODE {
	MODE_DVS_DVP_BOTH = 0,
	MODE_DVS_ONLY,
	MODE_DVP_ONLY,
	MODE_DVGF_ONLY
};

enum DPE_MAINEYE_SEL {
	LEFT = 0,
	RIGHT = 1
};

struct DVS_SubModule_EN {
	bool sbf_en;
	bool occ_en;
	bool occ_Hist_en;
};

struct DVP_SubModule_EN {
	bool asf_crm_en;
	bool asf_rm_en;
	bool asf_rd_en;
	bool asf_hf_en;
	bool wmf_rd_en;
	bool wmf_hf_en;
	bool wmf_filt_en;
	unsigned int asf_hf_rounds;
	unsigned int asf_nb_rounds;
	unsigned int wmf_filt_rounds;
	bool asf_recursive_en;
	unsigned int asf_depth_mode;
};

struct DVGF_SubModule_EN {
	bool dpe_dvgf_en;
};

struct DPE_feedback {
	unsigned int reg1;
	unsigned int reg2;
};

struct DVS_Settings {
	enum DPE_MAINEYE_SEL mainEyeSel;
	struct DVS_ME_CFG TuningBuf_ME;
	struct DVS_OCC_CFG TuningBuf_OCC;
	struct DVS_SubModule_EN SubModule_EN;
	struct DPE_meta_dvs Tuning_meta_dvs;
	bool is_pd_mode;
	unsigned int pd_frame_num; // set by driver
	unsigned int pd_st_x; // set by driver
	unsigned int frm_width;
	unsigned int frm_height;
	unsigned int l_eng_start_x;
	unsigned int r_eng_start_x;
	unsigned int eng_start_y;
	unsigned int eng_width;
	unsigned int eng_height;
	unsigned int occ_width;
	unsigned int dram_out_pitch_en; //!ISP7 tile mode enable
	unsigned int dram_out_pitch;
	unsigned int occ_start_x;
	unsigned int dram_pxl_pitch;
	unsigned int input_offset;
	unsigned int output_offset;
	unsigned int out_adj_en;
	unsigned int out_adj_dv_en;
	unsigned int out_adj_width;
	unsigned int out_adj_height;
	unsigned int ext_frm_mode_en;
	unsigned int ext_frm_num;
	unsigned int ext1_eng_start_x_L;
	unsigned int ext1_eng_start_x_R;
	unsigned int ext1_eng_start_y;
	unsigned int ext1_occ_start_x;
	unsigned int ext1_occ_width;
	unsigned int ext1_out_adj_width;
	unsigned int ext1_frm_width;
	unsigned int ext1_eng_width;
	unsigned int ext2_eng_start_x_L;
	unsigned int ext2_eng_start_x_R;
	unsigned int ext2_eng_start_y;
	unsigned int ext2_occ_start_x;
	unsigned int ext2_occ_width;
	unsigned int ext2_out_adj_width;
	unsigned int ext2_frm_width;
	unsigned int ext2_eng_width;
};

struct DVP_Settings {
	enum DPE_MAINEYE_SEL mainEyeSel;
	bool Y_only;
	struct DVP_CORE_CFG TuningBuf_CORE;
	struct DVP_SubModule_EN SubModule_EN;
	struct DPE_meta_dvp Tuning_meta_dvp;
	//bool disp_guide_en;
	unsigned int frm_width;
	unsigned int frm_height;
	unsigned int eng_start_x;
	unsigned int eng_start_y;
	unsigned int eng_width;
	unsigned int eng_height;
	unsigned int occdv_width;
	unsigned int occdv_height;
};

struct DVGF_Settings {
	struct DVGF_SubModule_EN SubModule_EN;
	struct DVGF_CORE_CFG TuningBuf_CORE;
	struct DPE_meta_dvgf Tuning_meta_dvgf;
	unsigned int frm_width;
	unsigned int frm_height;
	unsigned int eng_start_x;
	unsigned int eng_start_y;
	unsigned int eng_width;
	unsigned int eng_height;
};

struct DPE_Config_map {
	unsigned int Dpe_InBuf_SrcImg_Y_L_fd;
	unsigned int Dpe_InBuf_SrcImg_Y_L_Pre_fd;
	unsigned int Dpe_InBuf_SrcImg_Y_L_Ofs;
	unsigned int Dpe_InBuf_SrcImg_Y_L_Pre_Ofs;
	unsigned int Dpe_InBuf_SrcImg_Y_R_fd;
	unsigned int Dpe_InBuf_SrcImg_Y_R_Pre_fd;
	unsigned int Dpe_InBuf_SrcImg_Y_R_Ofs;
	unsigned int Dpe_InBuf_SrcImg_Y_R_Pre_Ofs;
	unsigned int Dpe_InBuf_SrcImg_Y_fd;
	unsigned int Dpe_InBuf_SrcImg_Y_Ofs;
	unsigned int Dpe_InBuf_SrcImg_Y_Pre_fd;
	unsigned int Dpe_InBuf_SrcImg_Y_Pre_Ofs;
	unsigned int Dpe_InBuf_SrcImg_C_fd;
	unsigned int Dpe_InBuf_SrcImg_C_Ofs;
	unsigned int Dpe_InBuf_SrcImg_C_Pre_fd;
	unsigned int Dpe_InBuf_SrcImg_C_Pre_Ofs;
	unsigned int Dpe_OutBuf_CONF_fd;
	unsigned int Dpe_OutBuf_CONF_Ofs;
	unsigned int Dpe_OutBuf_OCC_fd;
	unsigned int Dpe_OutBuf_OCC_Ofs;
	unsigned int Dpe_OutBuf_OCC_Ext_fd;
	unsigned int Dpe_OutBuf_OCC_Ext_Ofs;
	unsigned int Dpe_Output_OCC_Hist_fd;
	unsigned int Dpe_Output_OCC_Hist_Ofs;
	unsigned int Dpe_InBuf_OCC_fd;
	unsigned int Dpe_InBuf_OCC_Ofs;
	unsigned int Dpe_InBuf_OCC_Ext_fd;
	unsigned int Dpe_InBuf_OCC_Ext_Ofs;
	unsigned int Dpe_OutBuf_CRM_fd;
	unsigned int Dpe_OutBuf_CRM_Ofs;
	unsigned int Dpe_OutBuf_ASF_RM_fd;
	unsigned int Dpe_OutBuf_ASF_RM_Ofs;
	unsigned int Dpe_OutBuf_ASF_RM_Ext_fd;
	unsigned int Dpe_OutBuf_ASF_RM_Ext_Ofs;
	unsigned int Dpe_OutBuf_ASF_RD_fd;
	unsigned int Dpe_OutBuf_ASF_RD_Ofs;
	unsigned int Dpe_OutBuf_ASF_RD_Ext_fd;
	unsigned int Dpe_OutBuf_ASF_RD_Ext_Ofs;
	unsigned int Dpe_OutBuf_ASF_HF_fd;
	unsigned int Dpe_OutBuf_ASF_HF_Ofs;
	unsigned int Dpe_OutBuf_ASF_HF_Ext_fd;
	unsigned int Dpe_OutBuf_ASF_HF_Ext_Ofs;
	unsigned int Dpe_OutBuf_WMF_RD_fd;
	unsigned int Dpe_OutBuf_WMF_RD_Ofs;
	unsigned int Dpe_OutBuf_WMF_FILT_fd;
	unsigned int Dpe_OutBuf_WMF_FILT_Ofs;
	unsigned int DVS_SRC_21_INTER_MEDV_fd;
	unsigned int DVS_SRC_21_INTER_MEDV_Ofs;
	unsigned int DVS_SRC_34_DCV_L_FRM0_fd;
	unsigned int DVS_SRC_34_DCV_L_FRM0_Ofs;
	unsigned int DVP_SRC_18_ASF_RMDV_fd;
	unsigned int DVP_SRC_18_ASF_RMDV_Ofs;
	unsigned int DVP_SRC_24_WMF_HFDV_fd;
	unsigned int DVP_SRC_24_WMF_HFDV_Ofs;
	unsigned int DVP_EXT_SRC_18_ASF_RMDV_fd;
	unsigned int DVP_EXT_SRC_18_ASF_RMDV_Ofs;
	unsigned int Dpe_InBuf_P4_L_DV_fd;
	unsigned int Dpe_InBuf_P4_L_DV_Ofs;
	unsigned int Dpe_InBuf_P4_R_DV_fd;
	unsigned int Dpe_InBuf_P4_R_DV_Ofs;
	unsigned int Dpe_OutBuf_WT_Fnl_fd;
	unsigned int Dpe_OutBuf_WT_Fnl_Ofs;
	unsigned int Dpe_OutBuf_RW_IIR_fd;
	unsigned int Dpe_OutBuf_RW_IIR_Ofs;
};

struct DPE_Config_ISP8 {
	enum DPEMODE Dpe_engineSelect;
	unsigned int Dpe_RegDump;
	unsigned int Dpe_is16BitMode;
	//struct dpe_token_info DPE_Notify_Token_Info;
	unsigned int req_no;
	unsigned int frm_no;
	unsigned int req_fd;
	struct dpe_token_info DPE_Token_Info[3];
	struct DVS_Settings Dpe_DVSSettings;
	struct DVP_Settings Dpe_DVPSettings;
	struct DVGF_Settings Dpe_DVGFSettings;
	struct DPE_Config_map DPE_DMapSettings;
	struct DPE_Kernel_Dump DPE_Kernel_DpeConfig;
	dma_addr_t Dpe_InBuf_SrcImg_Y_L;
	dma_addr_t Dpe_InBuf_SrcImg_Y_R;
	dma_addr_t Dpe_InBuf_SrcImg_Y;
	dma_addr_t Dpe_InBuf_SrcImg_C;
	dma_addr_t Dpe_InBuf_SrcImg_Y_Pre;
	dma_addr_t Dpe_InBuf_SrcImg_C_Pre;
	dma_addr_t Dpe_OutBuf_CONF;
	dma_addr_t Dpe_OutBuf_OCC;
	dma_addr_t Dpe_OutBuf_OCC_Ext;
	dma_addr_t Dpe_OutBuf_OCC_Hist;
	dma_addr_t Dpe_InBuf_OCC;
	dma_addr_t Dpe_InBuf_OCC_Ext;
	dma_addr_t Dpe_OutBuf_CRM;
	dma_addr_t Dpe_OutBuf_WMF_RD;
	dma_addr_t Dpe_OutBuf_WMF_RD_Ext;
	dma_addr_t Dpe_OutBuf_ASF_RD;
	dma_addr_t Dpe_OutBuf_ASF_RD_Ext;
	dma_addr_t Dpe_OutBuf_ASF_HF;
	dma_addr_t Dpe_OutBuf_ASF_HF_Ext;
	dma_addr_t Dpe_OutBuf_WMF_HF;
	dma_addr_t Dpe_OutBuf_WMF_FILT;
	dma_addr_t DVP_SRC_18_ASF_RMDV;
	dma_addr_t DVP_SRC_24_WMF_HFDV;
	dma_addr_t DVP_EXT_SRC_18_ASF_RMDV;
	dma_addr_t Dpe_InBuf_SrcImg_Y_L_Pre;
	dma_addr_t Dpe_InBuf_SrcImg_Y_R_Pre;
	dma_addr_t Dpe_InBuf_P4_L_DV;
	dma_addr_t Dpe_InBuf_P4_R_DV;
	dma_addr_t Dpe_OutBuf_WT_Fnl;
	dma_addr_t Dpe_OutBuf_RW_IIR;
	struct DPE_feedback Dpe_feedback;
};

/*
 *
 */
enum DPE_CMD_ENUM {
	DPE_CMD_RESET,		/* Reset */
	DPE_CMD_DUMP_REG,	/* Dump DPE Register */
	DPE_CMD_DUMP_ISR_LOG,	/* Dump DPE ISR log */
	DPE_CMD_READ_REG,	/* Read register from driver */
	DPE_CMD_WRITE_REG,	/* Write register to driver */
	DPE_CMD_WAIT_IRQ,	/* Wait IRQ */
	DPE_CMD_CLEAR_IRQ,	/* Clear IRQ */
	DPE_CMD_ENQUE_NUM,	/* DPE Enque Number */
	DPE_CMD_ENQUE,		/* DPE Enque */
	DPE_CMD_ENQUE_REQ,	/* DPE Enque Request */
	DPE_CMD_DEQUE_NUM,	/* DPE Deque Number */
	DPE_CMD_DEQUE,		/* DPE Deque */
	DPE_CMD_DEQUE_REQ,	/* DPE Deque Request */
	DPE_CMD_TOTAL,
};

enum DPE_DVS_BUF {
	SrcImg_Y_L,
	SrcImg_Y_R,
	OutBuf_OCC,
	OutBuf_OCC_EXT,
	OutBuf_OCC_HIST,
	SrcImg_Y_L_PRE,
	SrcImg_Y_R_PRE,
	InBuf_P4_L_DV,
	InBuf_P4_R_DV,
	DVS_BUF_TOTAL,
};

enum DPE_DVP_BUF {
	SrcImg_Y,
	SrcImg_C,
	InBuf_OCC,
	OutBuf_CRM,
	OutBuf_WMF_RD,
	OutBuf_ASF_RD = OutBuf_WMF_RD,
	OutBuf_ASF_HF,
	OutBuf_WMF_FILT,
	InBuf_OCC_Ext,
	OutBuf_ASF_RD_Ext,
	OutBuf_ASF_HF_Ext,
	DVP_BUF_TOTAL,
};

enum DPE_DVGF_BUF {
	DVGF_SrcImg_Y,
	DVGF_SrcImg_C,
	SrcImg_Y_Pre,
	SrcImg_C_Pre,
	DVGF_OutBuf_OCC_Ext,
	OutBuf_WT_Fnl,
	OutBuf_RW_IIR,
	DVGF_OutBuf_WMF_FILT,
	DVGF_BUF_TOTAL,
};
/*  */

struct DPE_Request {
	unsigned int m_ReqNum;
	struct DPE_Config_ISP8 *m_pDpeConfig;
};

#if IS_ENABLED(CONFIG_COMPAT)
struct compat_DPE_REG_IO_STRUCT {
	compat_uptr_t pData;
	unsigned int Count;	/* count */
};

struct compat_DPE_Request {
	unsigned int m_ReqNum;
	compat_uptr_t m_pDpeConfig;
};

#endif


#define DPE_RESET        _IO(DPE_MAGIC, DPE_CMD_RESET)
#define DPE_DUMP_REG        _IO(DPE_MAGIC, DPE_CMD_DUMP_REG)
#define DPE_DUMP_ISR_LOG    _IO(DPE_MAGIC, DPE_CMD_DUMP_ISR_LOG)

#define DPE_READ_REGISTER						\
	_IOWR(DPE_MAGIC, DPE_CMD_READ_REG, struct DPE_REG_IO_STRUCT)
#define DPE_WRITE_REGISTER						\
	_IOWR(DPE_MAGIC, DPE_CMD_WRITE_REG, struct DPE_REG_IO_STRUCT)
#define DPE_WAIT_IRQ							\
	_IOW(DPE_MAGIC, DPE_CMD_WAIT_IRQ, struct DPE_WAIT_IRQ_STRUCT)
#define DPE_CLEAR_IRQ							\
	_IOW(DPE_MAGIC, DPE_CMD_CLEAR_IRQ, struct DPE_CLEAR_IRQ_STRUCT)

#define DPE_ENQNUE_NUM  _IOW(DPE_MAGIC, DPE_CMD_ENQUE_NUM,    int)
#define DPE_ENQUE      _IOWR(DPE_MAGIC, DPE_CMD_ENQUE,      struct DPE_Config_ISP8)
#define DPE_ENQUE_REQ  _IOWR(DPE_MAGIC, DPE_CMD_ENQUE_REQ,  struct DPE_Request)

#define DPE_DEQUE_NUM  _IOR(DPE_MAGIC, DPE_CMD_DEQUE_NUM,    int)
#define DPE_DEQUE      _IOWR(DPE_MAGIC, DPE_CMD_DEQUE,      struct DPE_Config_ISP8)
#define DPE_DEQUE_REQ  _IOWR(DPE_MAGIC, DPE_CMD_DEQUE_REQ,  struct DPE_Request)

//#if IS_ENABLED(CONFIG_COMPAT)
#define COMPAT_DPE_WRITE_REGISTER					\
	_IOWR(DPE_MAGIC, DPE_CMD_WRITE_REG, struct compat_DPE_REG_IO_STRUCT)
#define COMPAT_DPE_READ_REGISTER					\
	_IOWR(DPE_MAGIC, DPE_CMD_READ_REG, struct compat_DPE_REG_IO_STRUCT)

#define COMPAT_DPE_ENQUE_REQ						\
	_IOWR(DPE_MAGIC, DPE_CMD_ENQUE_REQ, struct compat_DPE_Request)
#define COMPAT_DPE_DEQUE_REQ						\
	_IOWR(DPE_MAGIC, DPE_CMD_DEQUE_REQ, struct compat_DPE_Request)
//#endif

/*  */
#endif
