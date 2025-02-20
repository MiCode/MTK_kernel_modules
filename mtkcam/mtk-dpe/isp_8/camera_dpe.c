// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2015 MediaTek Inc.
 */
/**************************************************************
 * camera_DPE.c - Linux DPE Device Driver
 *
 * DESCRIPTION:
 *     This file provide the other drivers DPE relative functions
 *
 **************************************************************/

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/types.h>
/* #include <asm/io.h> */
/* #include <asm/tcm.h> */
#include <linux/proc_fs.h> /* proc file use */
/*  */
#include <linux/slab.h>
#include <linux/spinlock.h>
/* #include <linux/io.h> */
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/sched/clock.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/suspend.h>
#include <linux/rtc.h>
#include <linux/mutex.h>
// V4L2
#include <media/v4l2-device.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-ioctl.h>
#include <linux/completion.h>
#define KERNEL_DMA_BUFFER
#ifdef KERNEL_DMA_BUFFER
#include <media/videobuf2-memops.h>
#include <linux/dma-direction.h>
#include <linux/dma-buf.h>
#endif
/*#include <linux/xlog.h>		 For xlog_printk(). */
/*  */
/*#include <mach/hardware.h>*/
/* #include <mach/mt6593_pll.h> */
#include "camera_dpe.h"
#include "mtk_cam-bwr.h"
/*#include <mach/irqs.h>*/
/* #include <mach/mt_reg_base.h> */
/* #if IS_ENABLED(CONFIG_MTK_LEGACY) */
/* For clock mgr APIS. enable_clock()/disable_clock(). */
/* #include <mach/mt_clkmgr.h> */
/* #endif */
//!#include <mt-plat/sync_write.h> /* For mt65xx_reg_sync_writel(). */
/* For spm_enable_sodi()/spm_disable_sodi(). */
/* #include <mach/mt_spm_idle.h> */
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
//#include <cmdq_core.h>
//#include <cmdq_record.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
//#include <linux/soc/mediatek/mtk-cmdq.h>
#include <soc/mediatek/smi.h>
#include<soc/mediatek/mmdvfs_v3.h>
#include "mtk-interconnect.h"
#include "iommu_debug.h"
#include "mtk-smmu-v3.h"
#include "mtk_imgsys_frm_sync.h"
#include "mtk_imgsys_frm_sync_event.h"

//! for IOVA to PA
#include <linux/iommu.h>
#define CMdq_en
#define DPE_WR32_en
#define dpe_dump_read_en
#define SMI_CLK
#define CMDQ_COMMON
#define CHECK_SERVICE_IF_0 0
//#define DPE_debug_log_en 1
//#define m4u_en
//!#define smi_en
//!#define WAKEUP_INIT
#define ENQUE_FAIL -1
#define MAX_REQ 3
#ifdef m4u_en
#if IS_ENABLED(CONFIG_MTK_IOMMU_V2)
#include <mach/mt_iommu.h>
#include "mach/pseudo_m4u.h"
#else /* CONFIG_MTK_IOMMU_V2 */
//#include <m4u.h>
#endif /* CONFIG_MTK_IOMMU_V2 */
#endif

#ifdef smi_en
#include <smi_public.h>
#endif

#include "engine_request.h"
#ifdef KERNEL_DMA_BUFFER
#include "videobuf2-dma-contig.h"
#endif
/*#define DPE_PMQOS_EN*/
#if defined(DPE_PMQOS_EN) && defined(CONFIG_MTK_QOS_SUPPORT)
#include <linux/pm_qos.h>
#endif
/* Measure the kernel performance
 * #define __DPE_KERNEL_PERFORMANCE_MEASURE__
 */
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
#include <linux/met_drv.h>
#include <linux/mtk_ftrace.h>
#endif

//Another Performance Measure Usage
//#include <linux/ftrace_event.h>
//#include <linux/kallsyms.h>
//static unsigned long __read_mostly tracing_mark_write_addr;
//#define _kernel_trace_begin(name)                                              \
//	{                                                                      \
//		tracing_mark_write_addr =                                      \
//			kallsyms_lookup_name("tracing_mark_write");            \
//		event_trace_printk(tracing_mark_write_addr, "B|%d|%s\n",       \
//				   current->tgid, name);                       \
//	}
//#define _kernel_trace_end()                                                    \
//	{                                                                      \
//		event_trace_printk(tracing_mark_write_addr, "E\n");            \
//	}
// How to Use
//char strName[128];
// sprintf(strName, "TAG_K_WAKEUP (%d)",sof_count[_PASS1]);
// _kernel_trace_begin(strName);
// _kernel_trace_end();

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#if IS_ENABLED(CONFIG_COMPAT)
/* 64 bit */
#include <linux/compat.h>
#include <linux/fs.h>
#endif
/*  #include "smi_common.h" */
#include <linux/pm_wakeup.h>
/* CCF */
#if !IS_ENABLED(CONFIG_MTK_LEGACY) && IS_ENABLED(CONFIG_COMMON_CLK) /*CCF*/
#include <linux/clk.h>
struct DPE_CLK_STRUCT {
	// struct clk *CLK_CK2_DPE_SEL;
	struct clk *CLK_CAM_MAIN_CAM;
	struct clk *CLK_CAMSYS_IPE_LARB19_CAMERA_P2;
	struct clk *CLK_CAMSYS_IPE_DPE_CAMERA_P2;
	struct clk *CLK_CAMSYS_IPE_FUS_CAMERA_P2;
	struct clk *CLK_CAMSYS_IPE_DHZE_CAMERA_P2;
	struct clk *CLK_CAMSYS_IPE_GALS_CAMERA_P2;
};

struct DPE_CLK_STRUCT dpe_clk;
#endif

// mmqos relate
static const char * const mmqos_dpe_names_rdma[] = {
	"l19_dvs_rd_p4",
	"l19_dvp_rd_p6",
	"l19_dvgf_rd_p10"
};
#define DPE_MMQOS_RDMA_NUM ARRAY_SIZE(mmqos_dpe_names_rdma)
struct icc_path *icc_path_dpe_rdma[DPE_MMQOS_RDMA_NUM];

static const char * const mmqos_dpe_names_wdma[] = {
	"l19_dvs_wt_p5",
	"l19_dvp_wt_p7",
	"l19_dvgf_wt_p11"
};
#define DPE_MMQOS_WDMA_NUM ARRAY_SIZE(mmqos_dpe_names_wdma)
struct icc_path *icc_path_dpe_wdma[DPE_MMQOS_WDMA_NUM];

static unsigned int g_dvs_rdma_ttl_bw, g_dvs_wdma_ttl_bw;
static unsigned int g_dvp_rdma_ttl_bw, g_dvp_wdma_ttl_bw;
static unsigned int g_dvgf_rdma_ttl_bw, g_dvgf_wdma_ttl_bw;

struct mtk_bwr_device *dpe_bwr_device;

#ifndef MTRUE
#define MTRUE 1
#endif
#ifndef MFALSE
#define MFALSE 0
#endif
#define DPE_DEV_NAME "camera-dpe"
//#define EP_NO_CLKMGR
#define BYPASS_REG (0)
#define DUMMY_DPE (0)
#define UT_CASE
/*I can' test the situation in FPGA due to slow FPGA. */
#define MyTag "[DPE]"
#define IRQTag "KEEPER"
#define LOG_VRB(format, args...) pr_debug(MyTag format, ##args)
//#define DPE_DEBUG_USE
#ifdef DPE_DEBUG_USE
#define LOG_DBG(format, args...) pr_info(MyTag format, ##args)
#else
#define LOG_DBG(format, args...) pr_debug(MyTag format, ##args)
#endif
#define LOG_INF(format, args...) pr_info(MyTag format, ##args)
#define LOG_NOTICE(format, args...) pr_notice(MyTag format, ##args)
//#define LOG_WRN(format, args...) pr_info(MyTag format, ##args)
#define LOG_ERR(format, args...) pr_err(MyTag format, ##args)
#define LOG_AST(format, args...) pr_info(MyTag format, ##args)
/**************************************************************
 *
 **************************************************************/
/* #define DPE_WR32(addr, data) iowrite32(data, addr) */
//#define DPE_WR32(addr, data) mt_reg_sync_writel(data, addr)
//#define DPE_RD32(addr) ioread32(addr)
#define DPE_WR32(addr, data) writel(data, addr)
#define DPE_RD32(addr) readl(addr)
#define DPE_MASKWR(addr, data, mask) \
	DPE_WR32(addr, ((DPE_RD32(addr) & ~(mask)) | data))
/**************************************************************
 *
 **************************************************************/
/* dynamic log level */
#define DPE_DBG_DBGLOG (0x00000001)
#define DPE_DBG_INFLOG (0x00000002)
#define DPE_DBG_INT (0x00000004)
#define DPE_DBG_READ_REG (0x00000008)
#define DPE_DBG_WRITE_REG (0x00000010)
#define DPE_DBG_TASKLET (0x00000020)
/*
 *    CAM interrupt status
 */
/* normal siganl : happens to be the same bit as register bit*/
/*#define DPE_INT_ST           (1<<0)*/
/*
 *   IRQ signal mask
 */
#define INT_ST_MASK_DPE (DPE_INT_ST)
#define CMDQ_REG_MASK 0xffffffff
#define MASK_15 0xF
#define ALIGN16(x) (((x)+MASK_15)&(~(MASK_15)))
#define MAX_NUM_TILE 4
#define TILE_WITH_NUM 3
//#define IOVA_TO_PA
#define DPE_IRQ_ENABLE (0)
#define IRQ_LOG

spinlock_t REQ_LOCK;
unsigned int reqidx;
#if DPE_IRQ_ENABLE
/* static irqreturn_t DPE_Irq_CAM_A(signed int  Irq,void *DeviceId); */
static irqreturn_t ISP_Irq_DVP(signed int Irq, void *DeviceId);
static irqreturn_t ISP_Irq_DVS(signed int Irq, void *DeviceId);
static irqreturn_t ISP_Irq_DVGF(signed int Irq, void *DeviceId);
typedef irqreturn_t (*IRQ_CB)(signed int, void *);
struct ISR_TABLE {
	IRQ_CB isr_fp;
	unsigned int int_number;
	char device_name[16];
};
#if !IS_ENABLED(CONFIG_OF)
const struct ISR_TABLE DPE_IRQ_CB_TBL[DPE_IRQ_TYPE_AMOUNT] = {
	{ISP_Irq_DVP, DPE_IRQ_BIT_ID, "dvp"},
	{ISP_Irq_DVS, DPE_IRQ_BIT_ID, "dvs"},
	{ISP_Irq_DVGF, DPE_IRQ_BIT_ID, "dvgf"},
};
#else
/* int number is got from kernel api */
const struct ISR_TABLE DPE_IRQ_CB_TBL[DPE_IRQ_TYPE_AMOUNT] = {
#if DUMMY_DPE
	{ISP_Irq_DVP, 0, "dvp-dummy"},
	{ISP_Irq_DVS, 0, "dvs-dummy"},
	{ISP_Irq_DVGF, 0, "dvgf-dummy"},
#else
	{ISP_Irq_DVP, 0, "dvp"},
	{ISP_Irq_DVS, 0, "dvs"},
	{ISP_Irq_DVGF, 0, "dvgf"},
#endif
};
#endif
#endif
static void DVS_ScheduleWork(struct work_struct *data);
static void DVP_ScheduleWork(struct work_struct *data);
static void DVGF_ScheduleWork(struct work_struct *data);
/*
 */
/*  */

struct completion DPEinit_done;

typedef void (*tasklet_cb)(unsigned long);
struct Tasklet_table {
	tasklet_cb tkt_cb;
	struct tasklet_struct *pDPE_tkt;
};
struct tasklet_struct Dpetkt[DPE_IRQ_TYPE_AMOUNT];
static void ISP_TaskletFunc_DVP(unsigned long data);
static void ISP_TaskletFunc_DVS(unsigned long data);
static void ISP_TaskletFunc_DVGF(unsigned long data);
static struct Tasklet_table DPE_tasklet[DPE_IRQ_TYPE_AMOUNT] = {
	{ISP_TaskletFunc_DVP, &Dpetkt[DPE_IRQ_TYPE_INT_DVP_ST]},
	{ISP_TaskletFunc_DVS, &Dpetkt[DPE_IRQ_TYPE_INT_DVS_ST]},
	{ISP_TaskletFunc_DVGF, &Dpetkt[DPE_IRQ_TYPE_INT_DVGF_ST]},
};
static struct work_struct logWork;
static void logPrint(struct work_struct *data);
struct wakeup_source DPE_wake_lock;
static DEFINE_MUTEX(gDpeMutex);
static DEFINE_MUTEX(gDpeDequeMutex);
static DEFINE_MUTEX(MutexDPERef);
static DEFINE_MUTEX(gFDMutex);
static DEFINE_MUTEX(gDVSMutex);
#if IS_ENABLED(CONFIG_OF)
struct DPE_device {
	void __iomem *regs;
	struct device *dev;
	struct device *larb19;
	struct device *camisp_vcore;
	struct device *smmu_dev;
	struct clk_bulk_data *clks;
	unsigned int clk_num;
	int irq;
	struct platform_device *frm_sync_pdev;
	int dev_ver;
// V4L2
	struct v4l2_device v4l2_dev;
	struct mutex mutex;
	struct video_device vid_dpe_dev;
};
static struct DPE_device *DPE_devs;
static int nr_DPE_devs;
static unsigned int DPE_BASE_HW;
static const struct of_device_id DPE_of_ids[];
/* Get HW modules' base address from device nodes */
#define DPE_DEV_NODE_IDX 0
#define IPESYS_DEV_MODE_IDX 1
#define ISP_DPE_BASE (DPE_devs[DPE_DEV_NODE_IDX].regs)
#define ISP_IPESYS_BASE (DPE_devs[IPESYS_DEV_MODE_IDX].regs)
#else
#define ISP_DPE_BASE 0x3A770000
#define DPE_BASE_HW 0x3A770000
#endif

#ifdef SMI_CLK
struct platform_device *DPE_pdev;
#endif


struct device *gdev;
struct device *smmudev;

#ifdef KERNEL_DMA_BUFFER
struct dma_buf *dbuf;
struct vb2_dc_buf {
	struct device *dev;
	void *vaddr;
	unsigned long size;
	void *cookie;
	dma_addr_t dma_addr;
	unsigned long attrs;
	enum dma_data_direction dma_dir;
	struct sg_table *dma_sgt;
	struct frame_vector *vec;
	/* MMAP related */
	struct vb2_vmarea_handler handler;
	refcount_t refcount;
	struct sg_table *sgt_base;
	const char *name;
	/* DMABUF related */
	struct dma_buf_attachment *db_attach;
	struct dma_buf *dma_buf;
	struct sg_table *sgt;
};
struct vb2_dc_buf *kernel_dpebuf;
struct vb2_dc_buf *dpebuf;
dma_addr_t *g_dpewb_asfrm_Buffer_pa;
dma_addr_t *g_dpewb_asfrmext_Buffer_pa;
dma_addr_t *g_dpewb_wmfhf_Buffer_pa;
#endif
static unsigned int g_isshutdown;
static unsigned int g_u4EnableClockCount;
static unsigned int g_SuspendCnt;
/* maximum number for supporting user to do interrupt operation */
/* index 0 is for all the user that do not do register irq first */
#define IRQ_USER_NUM_MAX 32
bool g_DPE_PMState;
bool g_isDPELogEnable = MFALSE;
//for cmdq malibox
static struct cmdq_client *dpe_clt;
struct cmdq_base *dpe_clt_base;
u32 dvs_event_id;
u32 dvp_event_id;
u32 dvgf_event_id;

enum DPE_FRAME_STATUS_ENUM {
	DPE_FRAME_STATUS_EMPTY,    /* 0 */
	DPE_FRAME_STATUS_ENQUE,    /* 1 */
	DPE_FRAME_STATUS_RUNNING,  /* 2 */
	DPE_FRAME_STATUS_FINISHED, /* 3 */
	DPE_FRAME_STATUS_TOTAL
};
enum DPE_REQUEST_STATE_ENUM {
	DPE_REQUEST_STATE_EMPTY,    /* 0 */
	DPE_REQUEST_STATE_PENDING,  /* 1 */
	DPE_REQUEST_STATE_RUNNING,  /* 2 */
	DPE_REQUEST_STATE_FINISHED, /* 3 */
	DPE_REQUEST_STATE_TOTAL
};
struct DPE_REQUEST_STRUCT {
	enum DPE_REQUEST_STATE_ENUM State;
	pid_t processID; /* caller process ID */
	unsigned int callerID; /* caller thread ID */
	unsigned int enqueReqNum;   /* to judge it belongs to which frame package */
	unsigned int FrameWRIdx; /* Frame write Index */
	unsigned int RrameRDIdx; /* Frame read Index */
	enum DPE_FRAME_STATUS_ENUM
		DpeFrameStatus[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
	struct DPE_Config_ISP8 DpeFrameConfig[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
};
struct DPE_REQUEST_RING_STRUCT {
	unsigned int WriteIdx;     /* enque how many request  */
	unsigned int ReadIdx;      /* read which request index */
	unsigned int HWProcessIdx; /* HWWriteIdx */
	struct DPE_REQUEST_STRUCT
		DPEReq_Struct[_SUPPORT_MAX_DPE_REQUEST_RING_SIZE_];
};
struct DPE_CONFIG_STRUCT {
	struct DPE_Config_ISP8 DpeFrameConfig[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
};
static struct DPE_REQUEST_RING_STRUCT g_DPE_ReqRing;
static struct DPE_CONFIG_STRUCT g_DpeEnqueReq_Struct;
static struct DPE_CONFIG_STRUCT g_DpeDequeReq_Struct;
//static struct engine_requests dpe_reqs;
static struct engine_requests dpe_reqs_dvs;
static struct engine_requests dpe_reqs_dvp;
static struct engine_requests dpe_reqs_dvgf;
static struct DPE_Request kDpeReq;
#define PMD_ENTRIES_MAX 512
#define MMU_ION_BUF BIT(24)
union mmu_table {
	u64 *entries; /* Array of PTEs */
	/* Array of pages */
	struct page **pages;
	/* Array of VAs */
	uintptr_t *vas;
	/* Address of table */
	void *addr;
	/* Page for table */
	unsigned long page;
};
//static struct DPE_device *DPE_DVSreg;//!
struct tee_mmu {
	struct iommu_domain *domain;
	dma_addr_t dma_addr;
	/* ION case only */
	struct dma_buf *dma_buf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
};
//DVS
struct tee_mmu *SrcImg_Y_L_mmu;
struct tee_mmu *SrcImg_Y_R_mmu;
struct tee_mmu *OutBuf_OCC_mmu;
struct tee_mmu *OutBuf_OCC_Hist_mmu;
struct tee_mmu *OutBuf_OCC_Ext_mmu;
struct tee_mmu *SrcImg_Y_L_Pre_mmu;
struct tee_mmu *SrcImg_Y_R_Pre_mmu;
struct tee_mmu *InBuf_P4_L_mmu;
struct tee_mmu *InBuf_P4_R_mmu;
//DVP
struct tee_mmu *SrcImg_Y_mmu;
struct tee_mmu *SrcImg_C_mmu;
struct tee_mmu *InBuf_OCC_mmu;
struct tee_mmu *OutBuf_CRM_mmu;
struct tee_mmu *ASF_RD_mmu;
struct tee_mmu *ASF_HF_mmu;
struct tee_mmu *WMF_RD_mmu;
struct tee_mmu *WMF_FILT_mmu;
struct tee_mmu *InBuf_OCC_Ext_mmu;
struct tee_mmu *ASF_RD_Ext_mmu;
struct tee_mmu *ASF_HF_Ext_mmu;
//DVGF
struct tee_mmu *DVGF_SrcImg_Y_mmu;
struct tee_mmu *DVGF_SrcImg_C_mmu;
struct tee_mmu *SrcImg_Y_Pre_mmu;
struct tee_mmu *SrcImg_C_Pre_mmu;
struct tee_mmu *WT_Fnl_mmu;
struct tee_mmu *RW_IIR_mmu;
struct tee_mmu *DVGF_WMF_FILT_mmu;
struct tee_mmu *DVGF_OutBuf_OCC_Ext_mmu;

struct tee_mmu *RegDumpt_mmu;
unsigned int get_dvs_iova[DVS_BUF_TOTAL];
unsigned int get_dvp_iova[DVP_BUF_TOTAL];
unsigned int get_dvgf_iova[DVGF_BUF_TOTAL];
unsigned int DVS_only_en;
unsigned int DVS_Num;
unsigned int DVP_only_en;
unsigned int DVP_Num;
unsigned int DVGF_only_en;
unsigned int DVGF_Num;
//unsigned int Get_DVS_IRQ;
//unsigned int Get_DVP_IRQ;
//unsigned int Get_DVGF_IRQ;
//unsigned int No_SMMU;
//unsigned int DVGF_Frame_cnt;
#define CMASYS_CLK_Debug 1
#ifdef CMASYS_CLK_Debug
// add Rang to dts
// <0 0x3A000000 0 0x1000>,
// <0 0x3A7A0000 0 0x1000>,

static unsigned int DPE_debug_log_en;
module_param(DPE_debug_log_en, uint, 0644);
MODULE_PARM_DESC(DPE_debug_log_en, "dpe dumpreg enable");

struct CAM_device {
		void __iomem *regs;
	struct device *dev;
	struct device *larb19;
	struct device *camisp_vcore;
	struct device *smmu_dev;
	int irq;
// V4L2
	struct v4l2_device v4l2_dev;
	struct mutex mutex;
	struct video_device vid_dpe_dev;
};
struct IPE_device {
		void __iomem *regs;
	struct device *dev;
	struct device *larb19;
	struct device *camisp_vcore;
	struct device *smmu_dev;
	int irq;
// V4L2
	struct v4l2_device v4l2_dev;
	struct mutex mutex;
	struct video_device vid_dpe_dev;
};
static struct CAM_device *CAM_devs;
static struct IPE_device *IPE_devs;

//
#define CAMSYS_MAIN_BASE (CAM_devs[0].regs)
#define IPE_MAIN_BASE (IPE_devs[0].regs)
#endif

struct token_index_info {
	unsigned int token_index[3];
	unsigned int token_cnt;
};
//!
struct my_callback_data {
	struct work_struct cmdq_cb_work;
	struct cmdq_pkt *pkt;
	unsigned int dpe_mode;
	signed int err;
	struct token_index_info token_info;
};

/**************************************************************
 *
 **************************************************************/
struct DPE_USER_INFO_STRUCT {
	pid_t Pid;
	pid_t Tid;
};
enum DPE_PROCESS_ID_ENUM {
	DPE_PROCESS_ID_NONE,
	DPE_PROCESS_ID_DPE,
	DPE_PROCESS_ID_AMOUNT
};
/**************************************************************
 *
 **************************************************************/
struct DPE_IRQ_INFO_STRUCT {
	unsigned int Status[DPE_IRQ_TYPE_AMOUNT];
	signed int DpeIrqCnt[IRQ_USER_NUM_MAX];
	pid_t ProcessID[IRQ_USER_NUM_MAX];
	unsigned int Mask[DPE_IRQ_TYPE_AMOUNT];
};
struct DPE_INFO_STRUCT {
	//spinlock_t SpinLockDPERef;
	spinlock_t SpinLockDPE;
	spinlock_t SpinLockFD;//!
	spinlock_t SpinLockIrq[DPE_IRQ_TYPE_AMOUNT];
	wait_queue_head_t WaitQueueHead;
	struct work_struct ScheduleDpeWork;
	struct work_struct DVP_ScheduleDpeWork;
	struct work_struct DVGF_ScheduleDpeWork;
	struct workqueue_struct *wkqueue;
	struct workqueue_struct *cmdq_wq;
	unsigned int UserCount; /* User Count */
	unsigned int ServeCount; /*Serve Count */
	unsigned int DebugMask; /* Debug Mask */
	signed int IrqNum;
	struct DPE_IRQ_INFO_STRUCT IrqInfo;
	signed int WriteReqIdx;
	signed int ReadReqIdx;
	pid_t ProcessID[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
};
static struct DPE_INFO_STRUCT DPEInfo;
#if defined(DPE_PMQOS_EN) && defined(CONFIG_MTK_QOS_SUPPORT)
struct pm_qos_request dpe_pm_qos_request;
#endif
enum eLOG_TYPE {
	_LOG_DBG = 0,
	_LOG_INF = 1,
	_LOG_ERR = 2,
	_LOG_MAX = 3,
};
#define NORMAL_STR_LEN (512)
#define ERR_PAGE 2
#define DBG_PAGE 2
#define INF_PAGE 4
/* #define SV_LOG_STR_LEN NORMAL_STR_LEN */
#define LOG_PPNUM 2
static unsigned int m_CurrentPPB;
struct SV_LOG_STR {
	unsigned int _cnt[LOG_PPNUM][_LOG_MAX];
	/* char   _str[_LOG_MAX][SV_LOG_STR_LEN]; */
	char *_str[LOG_PPNUM][_LOG_MAX];
} *PSV_LOG_STR;
static void *pLog_kmalloc;
static struct SV_LOG_STR gSvLog[DPE_IRQ_TYPE_AMOUNT];

static int DPE_cmdq_buf_idx;

/*
 *   for irq used,keep log until IRQ_LOG_PRINTER being involked,
 *   limited:
 *   each log must shorter than 512 bytes
 *  total log length in each irq/logtype can't over 1024 bytes
 */
#ifdef IRQ_LOG
#define IRQ_LOG_KEEPER(irq, ppb, logT, fmt, ...) do {\
	char *ptr; \
	char *pDes;\
	int avaLen;\
	unsigned int *ptr2 = &gSvLog[irq]._cnt[ppb][logT];\
	unsigned int str_leng;\
	unsigned int logi;\
	int ret; \
	struct SV_LOG_STR *pSrc = &gSvLog[irq];\
	if (logT == _LOG_ERR) {\
		str_leng = NORMAL_STR_LEN*ERR_PAGE; \
	} else if (logT == _LOG_DBG) {\
		str_leng = NORMAL_STR_LEN*DBG_PAGE; \
	} else if (logT == _LOG_INF) {\
		str_leng = NORMAL_STR_LEN*INF_PAGE;\
	} else {\
		str_leng = 0;\
	} \
	ptr = pDes = (char *)\
		&(gSvLog[irq]._str[ppb][logT][gSvLog[irq]._cnt[ppb][logT]]);   \
	avaLen = str_leng - 1 - gSvLog[irq]._cnt[ppb][logT];\
	if (avaLen > 1) {\
		ret = snprintf((char *)(pDes), avaLen, fmt,\
			##__VA_ARGS__);   \
		if (ret < 0) { \
			LOG_ERR("snprintf fail(%d)\n", ret); \
		} \
		if ('\0' != gSvLog[irq]._str[ppb][logT][str_leng - 1]) {\
			LOG_ERR("log str over flow(%d)", irq);\
		} \
		while (*ptr++ != '\0') {        \
			(*ptr2)++;\
		}     \
	} else { \
		LOG_INF("(%d)(%d)log str avalible=0, print log\n", irq, logT);\
		ptr = pSrc->_str[ppb][logT];\
		if (pSrc->_cnt[ppb][logT] != 0) {\
			if (logT == _LOG_DBG) {\
				for (logi = 0; logi < DBG_PAGE; logi++) {\
					if (ptr[NORMAL_STR_LEN*(logi+1) - 1] !=\
									'\0') {\
						ptr[NORMAL_STR_LEN*(logi+1)\
								- 1] = '\0';\
						LOG_DBG("%s", &ptr[\
							NORMAL_STR_LEN*logi]);\
					} else {\
						LOG_DBG("%s",\
						&ptr[NORMAL_STR_LEN*logi]);\
						break;\
					} \
				} \
			} \
			else if (logT == _LOG_INF) {\
				for (logi = 0; logi < INF_PAGE; logi++) {\
					if (ptr[NORMAL_STR_LEN*(logi+1) - 1] !=\
									'\0') {\
						ptr[NORMAL_STR_LEN*(logi+1)\
								   - 1] = '\0';\
						LOG_INF("%s",		       \
						    &ptr[NORMAL_STR_LEN*logi]);\
					} else {\
						LOG_INF("%s",\
						&ptr[NORMAL_STR_LEN*logi]);\
						break;\
					} \
				} \
			} \
			else if (logT == _LOG_ERR) {\
				for (logi = 0; logi < ERR_PAGE; logi++) {\
					if (ptr[NORMAL_STR_LEN*(logi+1) - 1] !=\
									'\0') {\
						ptr[NORMAL_STR_LEN*(logi+1)\
								   - 1] = '\0';\
						LOG_INF("%s",\
						&ptr[NORMAL_STR_LEN*logi]);\
					} else {\
						LOG_INF("%s",\
						&ptr[NORMAL_STR_LEN*logi]);\
						break;\
					} \
				} \
			} \
			else {\
				LOG_INF("N.S.%d", logT);\
			} \
			ptr[0] = '\0';\
			pSrc->_cnt[ppb][logT] = 0;\
			avaLen = str_leng - 1;\
			ptr = pDes = (char *)\
			&(pSrc->_str[ppb][logT][pSrc->_cnt[ppb][logT]]);\
			ptr2 = &(pSrc->_cnt[ppb][logT]);\
		ret = snprintf((char *)(pDes), avaLen, fmt, ##__VA_ARGS__);  \
		if (ret < 0) { \
			LOG_ERR("snprintf fail(%d)\n", ret); \
		} \
			while (*ptr++ != '\0') {\
				(*ptr2)++;\
			} \
		} \
	} \
} while (0)
#else
#define IRQ_LOG_KEEPER(irq, ppb, logT, fmt, ...) \
	xlog_printk(ANDROID_LOG_DEBUG,\
"KEEPER", "[%s] " fmt, __func__, ##__VA_ARGS__)
#endif
#ifdef IRQ_LOG
#define IRQ_LOG_PRINTER(irq, ppb_in, logT_in) do {\
	struct SV_LOG_STR *pSrc = &gSvLog[irq];\
	char *ptr;\
	unsigned int i;\
	unsigned int ppb = 0;\
	unsigned int logT = 0;\
	if (ppb_in > 1) {\
		ppb = 1;\
	} else {\
		ppb = ppb_in;\
	} \
	if (logT_in > _LOG_ERR) {\
		logT = _LOG_ERR;\
	} else {\
		logT = logT_in;\
	} \
	ptr = pSrc->_str[ppb][logT];\
	if (pSrc->_cnt[ppb][logT] != 0) {\
		if (logT == _LOG_DBG) {\
			for (i = 0; i < DBG_PAGE; i++) {\
				if (ptr[NORMAL_STR_LEN*(i+1) - 1] != '\0') {\
					ptr[NORMAL_STR_LEN*(i+1) - 1] = '\0';\
					LOG_DBG("%s", &ptr[NORMAL_STR_LEN*i]);\
				} else {\
					LOG_DBG("%s", &ptr[NORMAL_STR_LEN*i]);\
					break;\
				} \
			} \
		} \
	else if (logT == _LOG_INF) {\
		for (i = 0; i < INF_PAGE; i++) {\
			if (ptr[NORMAL_STR_LEN*(i+1) - 1] != '\0') {\
				ptr[NORMAL_STR_LEN*(i+1) - 1] = '\0';\
				LOG_INF("%s", &ptr[NORMAL_STR_LEN*i]);\
			} else {\
				LOG_INF("%s", &ptr[NORMAL_STR_LEN*i]);\
				break;\
			} \
		} \
	} \
	else if (logT == _LOG_ERR) {\
		for (i = 0; i < ERR_PAGE; i++) {\
			if (ptr[NORMAL_STR_LEN*(i+1) - 1] != '\0') {\
				ptr[NORMAL_STR_LEN*(i+1) - 1] = '\0';\
				LOG_ERR("%s", &ptr[NORMAL_STR_LEN*i]);\
			} else {\
				LOG_ERR("%s", &ptr[NORMAL_STR_LEN*i]);\
				break;\
			} \
		} \
	} \
	else {\
		LOG_ERR("N.S.%d", logT);\
	} \
		ptr[0] = '\0';\
		pSrc->_cnt[ppb][logT] = 0;\
	} \
} while (0)
#else
#define IRQ_LOG_PRINTER(irq, ppb, logT)
#endif

#ifdef CMASYS_CLK_Debug
#define CAMSYS_REG_CG_CON               (CAMSYS_MAIN_BASE + 0x0)
#define IPE_REG_CG_CON              __(IPE_MAIN_BASE + 0x0)
#endif

#define IPESYS_REG_CG_CON               (ISP_IPESYS_BASE + 0x0)
#define IPESYS_REG_CG_SET               (ISP_IPESYS_BASE + 0x4)
#define IPESYS_REG_CG_CLR               (ISP_IPESYS_BASE + 0x8)
/* DPE unmapped base address macro for GCE to access */
#define DVS_CTRL00_HW                (DPE_BASE_HW)
#define DVS_CTRL01_HW                (DPE_BASE_HW + 0x004)
#define DVS_CTRL02_HW                (DPE_BASE_HW + 0x008)
#define DVS_CTRL03_HW                (DPE_BASE_HW + 0x00C)
#define DVS_CTRL06_HW                (DPE_BASE_HW + 0x018)
#define DVS_CTRL07_HW                (DPE_BASE_HW + 0x01C)
#define DVS_CTRL08_HW                (DPE_BASE_HW + 0x020)
#define DVS_CTRL_STATUS3_HW          (DPE_BASE_HW + 0x030)
#define DVS_IRQ_00_HW                (DPE_BASE_HW + 0x040)
#define DVS_IRQ_01_HW                (DPE_BASE_HW + 0x044)
#define DVS_CTRL_STATUS0_HW          (DPE_BASE_HW + 0x050)
#define DVS_CTRL_STATUS2_HW          (DPE_BASE_HW + 0x058)
#define DVS_IRQ_STATUS_HW            (DPE_BASE_HW + 0x05C)
#define DVS_FRM_STATUS0_HW           (DPE_BASE_HW + 0x060)
#define DVS_FRM_STATUS1_HW           (DPE_BASE_HW + 0x064)
#define DVS_FRM_STATUS2_HW           (DPE_BASE_HW + 0x068)
#define DVS_FRM_STATUS3_HW           (DPE_BASE_HW + 0x06C)
#define DVS_FRM_STATUS4_HW           (DPE_BASE_HW + 0x070)
#define DVS_EXT_STATUS0_HW           (DPE_BASE_HW + 0x074)
#define DVS_EXT_STATUS1_HW           (DPE_BASE_HW + 0x078)
#define DVS_CUR_STATUS_HW            (DPE_BASE_HW + 0x080)
#define DVS_SRC_CTRL_HW              (DPE_BASE_HW + 0x084)
#define DVS_CRC_CTRL_HW              (DPE_BASE_HW + 0x088)
#define DVS_CRC_IN_HW                (DPE_BASE_HW + 0x08C)
#define DVS_DRAM_STA0_HW             (DPE_BASE_HW + 0x090)
#define DVS_DRAM_STA1_HW             (DPE_BASE_HW + 0x094)
#define DVS_DRAM_ULT_HW              (DPE_BASE_HW + 0x098)
#define DVS_DRAM_PITCH_HW            (DPE_BASE_HW + 0x09C)
#define DVS_DRAM_PITCH1_HW           (DPE_BASE_HW + 0x0A0)
#define DVS_SRC_00_HW                (DPE_BASE_HW + 0x100)
#define DVS_SRC_01_HW                (DPE_BASE_HW + 0x104)
#define DVS_SRC_02_HW                (DPE_BASE_HW + 0x108)
#define DVS_SRC_03_HW                (DPE_BASE_HW + 0x10C)
#define DVS_SRC_04_HW                (DPE_BASE_HW + 0x110)
#define DVS_SRC_05_L_FRM0_HW         (DPE_BASE_HW + 0x114)
#define DVS_SRC_06_L_FRM1_HW         (DPE_BASE_HW + 0x118)
#define DVS_SRC_07_L_FRM2_HW         (DPE_BASE_HW + 0x11C)
#define DVS_SRC_08_L_FRM3_HW         (DPE_BASE_HW + 0x120)
#define DVS_SRC_09_R_FRM0_HW         (DPE_BASE_HW + 0x124)
#define DVS_SRC_10_R_FRM1_HW         (DPE_BASE_HW + 0x128)
#define DVS_SRC_11_R_FRM2_HW         (DPE_BASE_HW + 0x12C)
#define DVS_SRC_12_R_FRM3_HW         (DPE_BASE_HW + 0x130)
#define DVS_SRC_13_Hist0_HW          (DPE_BASE_HW + 0x134)
#define DVS_SRC_14_Hist1_HW          (DPE_BASE_HW + 0x138)
#define DVS_SRC_15_Hist2_HW          (DPE_BASE_HW + 0x13C)
#define DVS_SRC_16_Hist3_HW          (DPE_BASE_HW + 0x140)
#define DVS_SRC_17_OCCDV_EXT0_HW     (DPE_BASE_HW + 0x144)
#define DVS_SRC_18_OCCDV_EXT1_HW     (DPE_BASE_HW + 0x148)
#define DVS_SRC_19_OCCDV_EXT2_HW     (DPE_BASE_HW + 0x14C)
#define DVS_SRC_20_OCCDV_EXT3_HW     (DPE_BASE_HW + 0x150)
#define DVS_SRC_21_P4_L_DV_ADR_HW    (DPE_BASE_HW + 0x154)
#define DVS_SRC_22_OCCDV0_HW         (DPE_BASE_HW + 0x158)
#define DVS_SRC_23_OCCDV1_HW         (DPE_BASE_HW + 0x15C)
#define DVS_SRC_24_OCCDV2_HW         (DPE_BASE_HW + 0x160)
#define DVS_SRC_25_OCCDV3_HW         (DPE_BASE_HW + 0x164)
#define DVS_SRC_26_P4_R_DV_ADR_HW    (DPE_BASE_HW + 0x168)
#define DVS_SRC_27_HW                (DPE_BASE_HW + 0x16C)
#define DVS_SRC_28_HW                (DPE_BASE_HW + 0x170)
#define DVS_SRC_29_HW                (DPE_BASE_HW + 0x174)
#define DVS_SRC_30_HW                (DPE_BASE_HW + 0x178)
#define DVS_SRC_31_HW                (DPE_BASE_HW + 0x17C)
#define DVS_SRC_32_HW                (DPE_BASE_HW + 0x180)
#define DVS_SRC_33_HW                (DPE_BASE_HW + 0x184)
#define DVS_SRC_34_HW                (DPE_BASE_HW + 0x188)
#define DVS_SRC_35_HW                (DPE_BASE_HW + 0x18C)
#define DVS_SRC_36_HW                (DPE_BASE_HW + 0x190)
#define DVS_SRC_37_HW                (DPE_BASE_HW + 0x194)
#define DVS_SRC_38_HW                (DPE_BASE_HW + 0x198)
#define DVS_SRC_39_HW                (DPE_BASE_HW + 0x19C)
#define DVS_SRC_40_LDV_HIST0_HW      (DPE_BASE_HW + 0x1A0)
#define DVS_SRC_41_LDV_HIST1_HW      (DPE_BASE_HW + 0x1A4)
#define DVS_SRC_42_LDV_HIST2_HW      (DPE_BASE_HW + 0x1A8)
#define DVS_SRC_43_LDV_HIST3_HW      (DPE_BASE_HW + 0x1AC)
#define DVS_SRC_44_HW                (DPE_BASE_HW + 0x1B0)
#define DVS_CRC_OUT_0_HW             (DPE_BASE_HW + 0x1C0)
#define DVS_CRC_OUT_1_HW             (DPE_BASE_HW + 0x1C4)
#define DVS_CRC_OUT_2_HW             (DPE_BASE_HW + 0x1C8)
#define DVS_CRC_OUT_3_HW             (DPE_BASE_HW + 0x1CC)
#define DVS_DRAM_SEC_0_HW            (DPE_BASE_HW + 0x1D0)
#define DVS_DRAM_SEC_1_HW            (DPE_BASE_HW + 0x1D4)
#define DVS_DRAM_SEC_2_HW            (DPE_BASE_HW + 0x1D8)
#define DVS_DRAM_SEC_3_HW            (DPE_BASE_HW + 0x1DC)
#define DVS_DRAM_AXSLC_0_HW          (DPE_BASE_HW + 0x1E0)
#define DVS_DRAM_AXSLC_1_HW          (DPE_BASE_HW + 0x1E4)
#define DVS_DEQ_FORCE_HW             (DPE_BASE_HW + 0x200)
#define DVS_CTRL_RESERVED_HW         (DPE_BASE_HW + 0x2F8)
#define DVS_CTRL_ATPG_HW             (DPE_BASE_HW + 0x2FC)
#define DVS_ME_00_HW                 (DPE_BASE_HW + 0x300)
#define DVS_ME_01_HW                 (DPE_BASE_HW + 0x304)
#define DVS_ME_02_HW                 (DPE_BASE_HW + 0x308)
#define DVS_ME_03_HW                 (DPE_BASE_HW + 0x30C)
#define DVS_ME_04_HW                 (DPE_BASE_HW + 0x310)
#define DVS_ME_05_HW                 (DPE_BASE_HW + 0x314)
#define DVS_ME_06_HW                 (DPE_BASE_HW + 0x318)
#define DVS_ME_07_HW                 (DPE_BASE_HW + 0x31C)
#define DVS_ME_08_HW                 (DPE_BASE_HW + 0x320)
#define DVS_ME_09_HW                 (DPE_BASE_HW + 0x324)
#define DVS_ME_10_HW                 (DPE_BASE_HW + 0x328)
#define DVS_ME_11_HW                 (DPE_BASE_HW + 0x32C)
#define DVS_ME_12_HW                 (DPE_BASE_HW + 0x330)
#define DVS_ME_13_HW                 (DPE_BASE_HW + 0x334)
#define DVS_ME_14_HW                 (DPE_BASE_HW + 0x338)
#define DVS_ME_15_HW                 (DPE_BASE_HW + 0x33C)
#define DVS_ME_16_HW                 (DPE_BASE_HW + 0x340)
#define DVS_ME_17_HW                 (DPE_BASE_HW + 0x344)
#define DVS_ME_18_HW                 (DPE_BASE_HW + 0x348)
#define DVS_ME_19_HW                 (DPE_BASE_HW + 0x34C)
#define DVS_ME_20_HW                 (DPE_BASE_HW + 0x350)
#define DVS_ME_21_HW                 (DPE_BASE_HW + 0x354)
#define DVS_ME_22_HW                 (DPE_BASE_HW + 0x358)
#define DVS_ME_23_HW                 (DPE_BASE_HW + 0x35C)
#define DVS_ME_24_HW                 (DPE_BASE_HW + 0x360)
#define DVS_ME_25_HW                 (DPE_BASE_HW + 0x364)
#define DVS_ME_26_HW                 (DPE_BASE_HW + 0x368)
#define DVS_ME_27_HW                 (DPE_BASE_HW + 0x36C)
#define DVS_ME_28_HW                 (DPE_BASE_HW + 0x370)
#define DVS_ME_29_HW                 (DPE_BASE_HW + 0x374)
#define DVS_ME_30_HW                 (DPE_BASE_HW + 0x378)
#define DVS_ME_31_HW                 (DPE_BASE_HW + 0x37C)
#define DVS_ME_32_HW                 (DPE_BASE_HW + 0x380)
#define DVS_ME_33_HW                 (DPE_BASE_HW + 0x384)
#define DVS_ME_34_HW                 (DPE_BASE_HW + 0x388)
#define DVS_ME_35_HW                 (DPE_BASE_HW + 0x38C)
#define DVS_ME_36_HW                 (DPE_BASE_HW + 0x390)
#define DVS_ME_37_HW                 (DPE_BASE_HW + 0x394)
#define DVS_ME_38_HW                 (DPE_BASE_HW + 0x398)
#define DVS_ME_39_HW                 (DPE_BASE_HW + 0x39C)
#define DVS_DEBUG_HW                 (DPE_BASE_HW + 0x3F4)
#define DVS_ME_RESERVED_HW           (DPE_BASE_HW + 0x3F8)
#define DVS_ME_ATPG_HW               (DPE_BASE_HW + 0x3FC)
#define DVS_ME_40_HW                 (DPE_BASE_HW + 0x400)
#define DVS_ME_41_HW                 (DPE_BASE_HW + 0x404)
#define DVS_ME_42_HW                 (DPE_BASE_HW + 0x408)
#define DVS_ME_43_HW                 (DPE_BASE_HW + 0x40C)
#define DVS_ME_44_HW                 (DPE_BASE_HW + 0x410)
#define DVS_ME_45_HW                 (DPE_BASE_HW + 0x414)
#define DVS_ME_46_HW                 (DPE_BASE_HW + 0x418)
#define DVS_ME_47_HW                 (DPE_BASE_HW + 0x41C)
#define DVS_ME_48_HW                 (DPE_BASE_HW + 0x420)
#define DVS_ME_49_HW                 (DPE_BASE_HW + 0x424)
#define DVS_ME_50_HW                 (DPE_BASE_HW + 0x428)
#define DVS_ME_51_HW                 (DPE_BASE_HW + 0x42C)
#define DVS_ME_52_HW                 (DPE_BASE_HW + 0x430)
#define DVS_ME_53_HW                 (DPE_BASE_HW + 0x434)
#define DVS_ME_54_HW                 (DPE_BASE_HW + 0x438)
#define DVS_ME_55_HW                 (DPE_BASE_HW + 0x43C)
#define DVS_ME_56_HW                 (DPE_BASE_HW + 0x440)
#define DVS_ME_57_HW                 (DPE_BASE_HW + 0x444)
#define DVS_ME_58_HW                 (DPE_BASE_HW + 0x448)
#define DVS_ME_59_HW                 (DPE_BASE_HW + 0x44C)
#define DVS_OCC_PQ_0_HW              (DPE_BASE_HW + 0x600)
#define DVS_OCC_PQ_1_HW              (DPE_BASE_HW + 0x604)
#define DVS_OCC_PQ_2_HW              (DPE_BASE_HW + 0x608)
#define DVS_OCC_PQ_3_HW              (DPE_BASE_HW + 0x60C)
#define DVS_OCC_PQ_4_HW              (DPE_BASE_HW + 0x610)
#define DVS_OCC_PQ_5_HW              (DPE_BASE_HW + 0x614)
#define DVS_OCC_PQ_10_HW             (DPE_BASE_HW + 0x628)
#define DVS_OCC_PQ_11_HW             (DPE_BASE_HW + 0x62C)
#define DVS_OCC_PQ_12_HW             (DPE_BASE_HW + 0x630)
#define DVS_OCC_ATPG_HW              (DPE_BASE_HW + 0x634)
#define DVS_OCC_HIST0_HW             (DPE_BASE_HW + 0x640)
#define DVS_OCC_HIST1_HW             (DPE_BASE_HW + 0x644)
#define DVS_OCC_HIST2_HW             (DPE_BASE_HW + 0x648)
#define DVS_OCC_HIST3_HW             (DPE_BASE_HW + 0x64C)
#define DVS_OCC_HIST4_HW             (DPE_BASE_HW + 0x650)
#define DVS_OCC_HIST5_HW             (DPE_BASE_HW + 0x654)
#define DVS_OCC_HIST6_HW             (DPE_BASE_HW + 0x658)
#define DVS_OCC_HIST7_HW             (DPE_BASE_HW + 0x65C)
#define DVS_OCC_HIST8_HW             (DPE_BASE_HW + 0x660)
#define DVS_OCC_HIST9_HW             (DPE_BASE_HW + 0x664)
#define DVS_OCC_HIST10_HW            (DPE_BASE_HW + 0x668)
#define DVS_OCC_HIST11_HW            (DPE_BASE_HW + 0x66C)
#define DVS_OCC_HIST12_HW            (DPE_BASE_HW + 0x670)
#define DVS_OCC_HIST13_HW            (DPE_BASE_HW + 0x674)
#define DVS_OCC_HIST14_HW            (DPE_BASE_HW + 0x678)
#define DVS_OCC_HIST15_HW            (DPE_BASE_HW + 0x67C)
#define DVS_OCC_HIST16_HW            (DPE_BASE_HW + 0x680)
#define DVS_OCC_HIST17_HW            (DPE_BASE_HW + 0x684)
#define DVS_OCC_HIST18_HW            (DPE_BASE_HW + 0x688)
#define DVS_OCC_LDV0_HW              (DPE_BASE_HW + 0x68C)
#define DVP_CTRL00_HW                (DPE_BASE_HW + 0x800)
#define DVP_CTRL01_HW                (DPE_BASE_HW + 0x804)
#define DVP_CTRL02_HW                (DPE_BASE_HW + 0x808)
#define DVP_CTRL03_HW                (DPE_BASE_HW + 0x80C)
#define DVP_CTRL04_HW                (DPE_BASE_HW + 0x810)
#define DVP_CTRL05_HW                (DPE_BASE_HW + 0x814)
#define DVP_CTRL07_HW                (DPE_BASE_HW + 0x81C)
#define DVP_IRQ_00_HW                (DPE_BASE_HW + 0x840)
#define DVP_IRQ_01_HW                (DPE_BASE_HW + 0x844)
#define DVP_CTRL_STATUS0_HW          (DPE_BASE_HW + 0x850)
#define DVP_CTRL_STATUS1_HW          (DPE_BASE_HW + 0x854)
#define DVP_IRQ_STATUS_HW            (DPE_BASE_HW + 0x85C)
#define DVP_FRM_STATUS0_HW           (DPE_BASE_HW + 0x860)
#define DVP_FRM_STATUS1_HW           (DPE_BASE_HW + 0x864)
#define DVP_FRM_STATUS2_HW           (DPE_BASE_HW + 0x868)
#define DVP_FRM_STATUS3_HW           (DPE_BASE_HW + 0x86C)
#define DVP_CUR_STATUS_HW            (DPE_BASE_HW + 0x870)
#define DVP_SRC_00_HW                (DPE_BASE_HW + 0x880)
#define DVP_SRC_01_HW                (DPE_BASE_HW + 0x884)
#define DVP_SRC_02_HW                (DPE_BASE_HW + 0x888)
#define DVP_SRC_03_HW                (DPE_BASE_HW + 0x88C)
#define DVP_SRC_04_HW                (DPE_BASE_HW + 0x890)
#define DVP_SRC_05_Y_FRM0_HW         (DPE_BASE_HW + 0x894)
#define DVP_SRC_06_Y_FRM1_HW         (DPE_BASE_HW + 0x898)
#define DVP_SRC_07_Y_FRM2_HW         (DPE_BASE_HW + 0x89C)
#define DVP_SRC_08_Y_FRM3_HW         (DPE_BASE_HW + 0x8A0)
#define DVP_SRC_09_C_FRM0_HW         (DPE_BASE_HW + 0x8A4)
#define DVP_SRC_10_C_FRM1_HW         (DPE_BASE_HW + 0x8A8)
#define DVP_SRC_11_C_FRM2_HW         (DPE_BASE_HW + 0x8AC)
#define DVP_SRC_12_C_FRM3_HW         (DPE_BASE_HW + 0x8B0)
#define DVP_SRC_13_OCCDV0_HW         (DPE_BASE_HW + 0x8B4)
#define DVP_SRC_14_OCCDV1_HW         (DPE_BASE_HW + 0x8B8)
#define DVP_SRC_15_OCCDV2_HW         (DPE_BASE_HW + 0x8BC)
#define DVP_SRC_16_OCCDV3_HW         (DPE_BASE_HW + 0x8C0)
#define DVP_SRC_17_CRM_HW            (DPE_BASE_HW + 0x8C4)
#define DVP_SRC_18_ASF_RMDV_HW       (DPE_BASE_HW + 0x8C8)
#define DVP_SRC_19_ASF_RDDV_HW       (DPE_BASE_HW + 0x8CC)
#define DVP_SRC_20_ASF_DV0_HW        (DPE_BASE_HW + 0x8D0)
#define DVP_SRC_21_ASF_DV1_HW        (DPE_BASE_HW + 0x8D4)
#define DVP_SRC_22_ASF_DV2_HW        (DPE_BASE_HW + 0x8D8)
#define DVP_SRC_23_ASF_DV3_HW        (DPE_BASE_HW + 0x8DC)
#define DVP_SRC_24_WMF_RDDV_HW       (DPE_BASE_HW + 0x8E0)
#define DVP_SRC_25_WMF_HFDV_HW       (DPE_BASE_HW + 0x8E4)
#define DVP_SRC_26_WMF_DV0_HW        (DPE_BASE_HW + 0x8E8)
#define DVP_SRC_27_WMF_DV1_HW        (DPE_BASE_HW + 0x8EC)
#define DVP_SRC_28_WMF_DV2_HW        (DPE_BASE_HW + 0x8F0)
#define DVP_SRC_29_WMF_DV3_HW        (DPE_BASE_HW + 0x8F4)
#define DVP_CORE_00_HW               (DPE_BASE_HW + 0x900)
#define DVP_CORE_01_HW               (DPE_BASE_HW + 0x904)
#define DVP_CORE_02_HW               (DPE_BASE_HW + 0x908)
#define DVP_CORE_03_HW               (DPE_BASE_HW + 0x90C)
#define DVP_CORE_04_HW               (DPE_BASE_HW + 0x910)
#define DVP_CORE_05_HW               (DPE_BASE_HW + 0x914)
#define DVP_CORE_06_HW               (DPE_BASE_HW + 0x918)
#define DVP_CORE_07_HW               (DPE_BASE_HW + 0x91C)
#define DVP_CORE_08_HW               (DPE_BASE_HW + 0x920)
#define DVP_CORE_09_HW               (DPE_BASE_HW + 0x924)
#define DVP_CORE_10_HW               (DPE_BASE_HW + 0x928)
#define DVP_CORE_11_HW               (DPE_BASE_HW + 0x92C)
#define DVP_CORE_12_HW               (DPE_BASE_HW + 0x930)
#define DVP_CORE_13_HW               (DPE_BASE_HW + 0x934)
#define DVP_CORE_14_HW               (DPE_BASE_HW + 0x938)
#define DVP_CORE_15_HW               (DPE_BASE_HW + 0x93C)
#define DVP_CORE_16_HW               (DPE_BASE_HW + 0x940)
#define DVP_CORE_17_HW               (DPE_BASE_HW + 0x944)
#define DVP_CORE_18_HW               (DPE_BASE_HW + 0x948)
#define DVP_CORE_19_HW               (DPE_BASE_HW + 0x94C)
#define DVP_SRC_CTRL_HW              (DPE_BASE_HW + 0x9F4)
#define DVP_CTRL_RESERVED_HW         (DPE_BASE_HW + 0x9F8)
#define DVP_CTRL_ATPG_HW             (DPE_BASE_HW + 0x9FC)
#define DVP_CRC_OUT_0_HW             (DPE_BASE_HW + 0xA00)
#define DVP_CRC_OUT_1_HW             (DPE_BASE_HW + 0xA04)
#define DVP_CRC_OUT_2_HW             (DPE_BASE_HW + 0xA08)
#define DVP_CRC_CTRL_HW              (DPE_BASE_HW + 0xA60)
#define DVP_CRC_OUT_HW               (DPE_BASE_HW + 0xA64)
#define DVP_CRC_IN_HW                (DPE_BASE_HW + 0xA6C)
#define DVP_DRAM_STA_HW              (DPE_BASE_HW + 0xA70)
#define DVP_DRAM_ULT_HW              (DPE_BASE_HW + 0xA74)
#define DVP_DRAM_PITCH_HW            (DPE_BASE_HW + 0xA78)
#define DVP_DRAM_SEC_0_HW            (DPE_BASE_HW + 0xA7C)
#define DVP_DRAM_SEC_1_HW            (DPE_BASE_HW + 0xA80)
#define DVP_DRAM_AXSLC_HW            (DPE_BASE_HW + 0xA84)
#define DVP_CORE_CRC_IN_HW           (DPE_BASE_HW + 0xA8C)
#define DVP_EXT_SRC_13_OCCDV0_HW     (DPE_BASE_HW + 0xBB4)
#define DVP_EXT_SRC_14_OCCDV1_HW     (DPE_BASE_HW + 0xBB8)
#define DVP_EXT_SRC_15_OCCDV2_HW     (DPE_BASE_HW + 0xBBC)
#define DVP_EXT_SRC_16_OCCDV3_HW     (DPE_BASE_HW + 0xBC0)
#define DVP_EXT_SRC_18_ASF_RMDV_HW   (DPE_BASE_HW + 0xBC8)
#define DVP_EXT_SRC_19_ASF_RDDV_HW   (DPE_BASE_HW + 0xBCC)
#define DVP_EXT_SRC_20_ASF_DV0_HW    (DPE_BASE_HW + 0xBD0)
#define DVP_EXT_SRC_21_ASF_DV1_HW    (DPE_BASE_HW + 0xBD4)
#define DVP_EXT_SRC_22_ASF_DV2_HW    (DPE_BASE_HW + 0xBD8)
#define DVP_EXT_SRC_23_ASF_DV3_HW    (DPE_BASE_HW + 0xBDC)
#define DVP_EXT_SRC_24_WMF_RDDV_HW   (DPE_BASE_HW + 0xBE0)
#define DVGF_CTRL_00_HW              (DPE_BASE_HW + 0xC00) //DVGF
#define DVGF_CTRL_01_HW              (DPE_BASE_HW + 0xC04)
#define DVGF_CTRL_02_HW              (DPE_BASE_HW + 0xC08)
#define DVGF_CTRL_03_HW              (DPE_BASE_HW + 0xC0C)
#define DVGF_CTRL_05_HW              (DPE_BASE_HW + 0xC14)
#define DVGF_CTRL_07_HW              (DPE_BASE_HW + 0xC1C)
#define DVGF_IRQ_00_HW               (DPE_BASE_HW + 0xC20)
#define DVGF_IRQ_01_HW               (DPE_BASE_HW + 0xC24)
#define DVGF_CTRL_STATUS0_HW         (DPE_BASE_HW + 0xC30)
#define DVGF_CTRL_STATUS1_HW         (DPE_BASE_HW + 0xC34)
#define DVGF_IRQ_STATUS_HW           (DPE_BASE_HW + 0xC3C)
#define DVGF_FRM_STATUS_HW           (DPE_BASE_HW + 0xC40)
#define DVGF_CUR_STATUS_HW           (DPE_BASE_HW + 0xC44)
#define DVGF_CRC_CTRL_HW             (DPE_BASE_HW + 0xC50)
#define DVGF_CRC_OUT_HW              (DPE_BASE_HW + 0xC54)
#define DVGF_CRC_IN_HW               (DPE_BASE_HW + 0xC58)
#define DVGF_CRC_OUT_0_HW            (DPE_BASE_HW + 0xC5C)
#define DVGF_CRC_OUT_1_HW            (DPE_BASE_HW + 0xC60)
#define DVGF_CRC_OUT_2_HW            (DPE_BASE_HW + 0xC64)
#define DVGF_CORE_CRC_IN_HW          (DPE_BASE_HW + 0xC68)
#define DVGF_DRAM_STA_HW             (DPE_BASE_HW + 0xC70)
#define DVGF_DRAM_PITCH_HW           (DPE_BASE_HW + 0xC78)
#define DVGF_DRAM_SEC_0_HW           (DPE_BASE_HW + 0xC7C)
#define DVGF_DRAM_SEC_1_HW           (DPE_BASE_HW + 0xC80)
#define DVGF_DRAM_AXSLC_HW           (DPE_BASE_HW + 0xC84)
#define DVGF_CTRL_STATUS_32b_00_HW   (DPE_BASE_HW + 0xC90)
#define DVGF_CTRL_STATUS_32b_01_HW   (DPE_BASE_HW + 0xC94)
#define DVGF_CTRL_STATUS_32b_02_HW   (DPE_BASE_HW + 0xC98)
#define DVGF_CTRL_STATUS_32b_03_HW   (DPE_BASE_HW + 0xC9C)
#define DVGF_CTRL_STATUS_32b_04_HW   (DPE_BASE_HW + 0xCA0)
#define DVGF_CTRL_RESERVED_HW        (DPE_BASE_HW + 0xCF8)
#define DVGF_CTRL_ATPG_HW            (DPE_BASE_HW + 0xCFC)
#define DVGF_SRC_00_HW               (DPE_BASE_HW + 0xD30)
#define DVGF_SRC_01_HW               (DPE_BASE_HW + 0xD34)
#define DVGF_SRC_02_HW               (DPE_BASE_HW + 0xD38)
#define DVGF_SRC_04_HW               (DPE_BASE_HW + 0xD40)
#define DVGF_SRC_05_HW               (DPE_BASE_HW + 0xD44)
#define DVGF_SRC_06_HW               (DPE_BASE_HW + 0xD48)
#define DVGF_SRC_07_HW               (DPE_BASE_HW + 0xD4C)
#define DVGF_SRC_08_HW               (DPE_BASE_HW + 0xD50)
#define DVGF_SRC_09_HW               (DPE_BASE_HW + 0xD54)
#define DVGF_SRC_10_HW               (DPE_BASE_HW + 0xD58)
#define DVGF_SRC_11_HW               (DPE_BASE_HW + 0xD5C)
#define DVGF_SRC_12_HW               (DPE_BASE_HW + 0xD60)
#define DVGF_SRC_13_HW               (DPE_BASE_HW + 0xD64)
#define DVGF_SRC_14_HW               (DPE_BASE_HW + 0xD68)
#define DVGF_SRC_15_HW               (DPE_BASE_HW + 0xD6C)
#define DVGF_SRC_16_HW               (DPE_BASE_HW + 0xD70)
#define DVGF_SRC_17_HW               (DPE_BASE_HW + 0xD74)
#define DVGF_SRC_18_HW               (DPE_BASE_HW + 0xD78)
#define DVGF_SRC_19_HW               (DPE_BASE_HW + 0xD7C)
#define DVGF_SRC_20_HW               (DPE_BASE_HW + 0xD80)
#define DVGF_SRC_21_HW               (DPE_BASE_HW + 0xD84)
#define DVGF_SRC_22_HW               (DPE_BASE_HW + 0xD88)
#define DVGF_SRC_23_HW               (DPE_BASE_HW + 0xD8C)
#define DVGF_SRC_24_HW               (DPE_BASE_HW + 0xD90)
#define DVGF_CORE_00_HW              (DPE_BASE_HW + 0xE00)
#define DVGF_CORE_01_HW              (DPE_BASE_HW + 0xE04)
#define DVGF_CORE_02_HW              (DPE_BASE_HW + 0xE08)
#define DVGF_CORE_03_HW              (DPE_BASE_HW + 0xE0C)
#define DVGF_CORE_05_HW              (DPE_BASE_HW + 0xE14)
#define DVGF_CORE_06_HW              (DPE_BASE_HW + 0xE18)
#define DVGF_CORE_07_HW              (DPE_BASE_HW + 0xE1C)
#define DVGF_CORE_08_HW              (DPE_BASE_HW + 0xE20)
#define DVGF_CORE_09_HW              (DPE_BASE_HW + 0xE24)
#define DVGF_CORE_10_HW              (DPE_BASE_HW + 0xE28)
#define DVGF_CORE_11_HW              (DPE_BASE_HW + 0xE2C)
#define DVGF_CORE_12_HW              (DPE_BASE_HW + 0xE30)
#define DVGF_CORE_13_HW              (DPE_BASE_HW + 0xE34)
#define DVGF_CORE_14_HW              (DPE_BASE_HW + 0xE38)
#define DVGF_CORE_15_HW              (DPE_BASE_HW + 0xE3C)
#define DVGF_CORE_16_HW              (DPE_BASE_HW + 0xE40)
#define DVGF_CORE_17_HW              (DPE_BASE_HW + 0xE44)
#define DVGF_CORE_18_HW              (DPE_BASE_HW + 0xE48)
/*SW Access Registers : using mapped base address from DTS*/
#define DVS_CTRL00_REG                (ISP_DPE_BASE)
#define DVS_CTRL01_REG                (ISP_DPE_BASE + 0x004)
#define DVS_CTRL02_REG                (ISP_DPE_BASE + 0x008)
#define DVS_CTRL03_REG                (ISP_DPE_BASE + 0x00C)
#define DVS_CTRL06_REG                (ISP_DPE_BASE + 0x018)
#define DVS_CTRL07_REG                (ISP_DPE_BASE + 0x01C)
#define DVS_CTRL08_REG                (ISP_DPE_BASE + 0x020)
#define DVS_CTRL_STATUS3_REG          (ISP_DPE_BASE + 0x030)
#define DVS_IRQ_00_REG                (ISP_DPE_BASE + 0x040)
#define DVS_IRQ_01_REG                (ISP_DPE_BASE + 0x044)
#define DVS_CTRL_STATUS0_REG          (ISP_DPE_BASE + 0x050)
#define DVS_CTRL_STATUS2_REG          (ISP_DPE_BASE + 0x058)
#define DVS_IRQ_STATUS_REG            (ISP_DPE_BASE + 0x05C)
#define DVS_FRM_STATUS0_REG           (ISP_DPE_BASE + 0x060)
#define DVS_FRM_STATUS1_REG           (ISP_DPE_BASE + 0x064)
#define DVS_FRM_STATUS2_REG           (ISP_DPE_BASE + 0x068)
#define DVS_FRM_STATUS3_REG           (ISP_DPE_BASE + 0x06C)
#define DVS_FRM_STATUS4_REG           (ISP_DPE_BASE + 0x070)
#define DVS_EXT_STATUS0_REG           (ISP_DPE_BASE + 0x074)
#define DVS_EXT_STATUS1_REG           (ISP_DPE_BASE + 0x078)
#define DVS_CUR_STATUS_REG            (ISP_DPE_BASE + 0x080)
#define DVS_SRC_CTRL_REG              (ISP_DPE_BASE + 0x084)
#define DVS_CRC_CTRL_REG              (ISP_DPE_BASE + 0x088)
#define DVS_CRC_IN_REG                (ISP_DPE_BASE + 0x08C)
#define DVS_DRAM_STA0_REG             (ISP_DPE_BASE + 0x090)
#define DVS_DRAM_STA1_REG             (ISP_DPE_BASE + 0x094)
#define DVS_DRAM_ULT_REG              (ISP_DPE_BASE + 0x098)
#define DVS_DRAM_PITCH_REG            (ISP_DPE_BASE + 0x09C)
#define DVS_DRAM_PITCH1_REG           (ISP_DPE_BASE + 0x0A0)
#define DVS_SRC_00_REG                (ISP_DPE_BASE + 0x100)
#define DVS_SRC_01_REG                (ISP_DPE_BASE + 0x104)
#define DVS_SRC_02_REG                (ISP_DPE_BASE + 0x108)
#define DVS_SRC_03_REG                (ISP_DPE_BASE + 0x10C)
#define DVS_SRC_04_REG                (ISP_DPE_BASE + 0x110)
#define DVS_SRC_05_L_FRM0_REG         (ISP_DPE_BASE + 0x114)
#define DVS_SRC_06_L_FRM1_REG         (ISP_DPE_BASE + 0x118)
#define DVS_SRC_07_L_FRM2_REG         (ISP_DPE_BASE + 0x11C)
#define DVS_SRC_08_L_FRM3_REG         (ISP_DPE_BASE + 0x120)
#define DVS_SRC_09_R_FRM0_REG         (ISP_DPE_BASE + 0x124)
#define DVS_SRC_10_R_FRM1_REG         (ISP_DPE_BASE + 0x128)
#define DVS_SRC_11_R_FRM2_REG         (ISP_DPE_BASE + 0x12C)
#define DVS_SRC_12_R_FRM3_REG         (ISP_DPE_BASE + 0x130)
#define DVS_SRC_13_Hist0_REG          (ISP_DPE_BASE + 0x134)
#define DVS_SRC_14_Hist1_REG          (ISP_DPE_BASE + 0x138)
#define DVS_SRC_15_Hist2_REG          (ISP_DPE_BASE + 0x13C)
#define DVS_SRC_16_Hist3_REG          (ISP_DPE_BASE + 0x140)
#define DVS_SRC_17_OCCDV_EXT0_REG     (ISP_DPE_BASE + 0x144)
#define DVS_SRC_18_OCCDV_EXT1_REG     (ISP_DPE_BASE + 0x148)
#define DVS_SRC_19_OCCDV_EXT2_REG     (ISP_DPE_BASE + 0x14C)
#define DVS_SRC_20_OCCDV_EXT3_REG     (ISP_DPE_BASE + 0x150)
#define DVS_SRC_21_P4_L_DV_ADR_REG    (ISP_DPE_BASE + 0x154)
#define DVS_SRC_22_OCCDV0_REG         (ISP_DPE_BASE + 0x158)
#define DVS_SRC_23_OCCDV1_REG         (ISP_DPE_BASE + 0x15C)
#define DVS_SRC_24_OCCDV2_REG         (ISP_DPE_BASE + 0x160)
#define DVS_SRC_25_OCCDV3_REG         (ISP_DPE_BASE + 0x164)
#define DVS_SRC_26_P4_R_DV_ADR_REG    (ISP_DPE_BASE + 0x168)
#define DVS_SRC_27_REG                (ISP_DPE_BASE + 0x16C)
#define DVS_SRC_28_REG                (ISP_DPE_BASE + 0x170)
#define DVS_SRC_29_REG                (ISP_DPE_BASE + 0x174)
#define DVS_SRC_30_REG                (ISP_DPE_BASE + 0x178)
#define DVS_SRC_31_REG                (ISP_DPE_BASE + 0x17C)
#define DVS_SRC_32_REG                (ISP_DPE_BASE + 0x180)
#define DVS_SRC_33_REG                (ISP_DPE_BASE + 0x184)
#define DVS_SRC_34_REG                (ISP_DPE_BASE + 0x188)
#define DVS_SRC_35_REG                (ISP_DPE_BASE + 0x18C)
#define DVS_SRC_36_REG                (ISP_DPE_BASE + 0x190)
#define DVS_SRC_37_REG                (ISP_DPE_BASE + 0x194)
#define DVS_SRC_38_REG                (ISP_DPE_BASE + 0x198)
#define DVS_SRC_39_REG                (ISP_DPE_BASE + 0x19C)
#define DVS_SRC_40_LDV_HIST0_REG      (ISP_DPE_BASE + 0x1A0)
#define DVS_SRC_41_LDV_HIST1_REG      (ISP_DPE_BASE + 0x1A4)
#define DVS_SRC_42_LDV_HIST2_REG      (ISP_DPE_BASE + 0x1A8)
#define DVS_SRC_43_LDV_HIST3_REG      (ISP_DPE_BASE + 0x1AC)
#define DVS_SRC_44_REG                (ISP_DPE_BASE + 0x1B0)
#define DVS_CRC_OUT_0_REG             (ISP_DPE_BASE + 0x1C0)
#define DVS_CRC_OUT_1_REG             (ISP_DPE_BASE + 0x1C4)
#define DVS_CRC_OUT_2_REG             (ISP_DPE_BASE + 0x1C8)
#define DVS_CRC_OUT_3_REG             (ISP_DPE_BASE + 0x1CC)
#define DVS_DRAM_SEC_0_REG            (ISP_DPE_BASE + 0x1D0)
#define DVS_DRAM_SEC_1_REG            (ISP_DPE_BASE + 0x1D4)
#define DVS_DRAM_SEC_2_REG            (ISP_DPE_BASE + 0x1D8)
#define DVS_DRAM_SEC_3_REG            (ISP_DPE_BASE + 0x1DC)
#define DVS_DRAM_AXSLC_0_REG          (ISP_DPE_BASE + 0x1E0)
#define DVS_DRAM_AXSLC_1_REG          (ISP_DPE_BASE + 0x1E4)
#define DVS_DEQ_FORCE_REG             (ISP_DPE_BASE + 0x200)
#define DVS_CTRL_RESERVED_REG         (ISP_DPE_BASE + 0x2F8)
#define DVS_CTRL_ATPG_REG             (ISP_DPE_BASE + 0x2FC)
#define DVS_ME_00_REG                 (ISP_DPE_BASE + 0x300)
#define DVS_ME_01_REG                 (ISP_DPE_BASE + 0x304)
#define DVS_ME_02_REG                 (ISP_DPE_BASE + 0x308)
#define DVS_ME_03_REG                 (ISP_DPE_BASE + 0x30C)
#define DVS_ME_04_REG                 (ISP_DPE_BASE + 0x310)
#define DVS_ME_05_REG                 (ISP_DPE_BASE + 0x314)
#define DVS_ME_06_REG                 (ISP_DPE_BASE + 0x318)
#define DVS_ME_07_REG                 (ISP_DPE_BASE + 0x31C)
#define DVS_ME_08_REG                 (ISP_DPE_BASE + 0x320)
#define DVS_ME_09_REG                 (ISP_DPE_BASE + 0x324)
#define DVS_ME_10_REG                 (ISP_DPE_BASE + 0x328)
#define DVS_ME_11_REG                 (ISP_DPE_BASE + 0x32C)
#define DVS_ME_12_REG                 (ISP_DPE_BASE + 0x330)
#define DVS_ME_13_REG                 (ISP_DPE_BASE + 0x334)
#define DVS_ME_14_REG                 (ISP_DPE_BASE + 0x338)
#define DVS_ME_15_REG                 (ISP_DPE_BASE + 0x33C)
#define DVS_ME_16_REG                 (ISP_DPE_BASE + 0x340)
#define DVS_ME_17_REG                 (ISP_DPE_BASE + 0x344)
#define DVS_ME_18_REG                 (ISP_DPE_BASE + 0x348)
#define DVS_ME_19_REG                 (ISP_DPE_BASE + 0x34C)
#define DVS_ME_20_REG                 (ISP_DPE_BASE + 0x350)
#define DVS_ME_21_REG                 (ISP_DPE_BASE + 0x354)
#define DVS_ME_22_REG                 (ISP_DPE_BASE + 0x358)
#define DVS_ME_23_REG                 (ISP_DPE_BASE + 0x35C)
#define DVS_ME_24_REG                 (ISP_DPE_BASE + 0x360)
#define DVS_ME_25_REG                 (ISP_DPE_BASE + 0x364)
#define DVS_ME_26_REG                 (ISP_DPE_BASE + 0x368)
#define DVS_ME_27_REG                 (ISP_DPE_BASE + 0x36C)
#define DVS_ME_28_REG                 (ISP_DPE_BASE + 0x370)
#define DVS_ME_29_REG                 (ISP_DPE_BASE + 0x374)
#define DVS_ME_30_REG                 (ISP_DPE_BASE + 0x378)
#define DVS_ME_31_REG                 (ISP_DPE_BASE + 0x37C)
#define DVS_ME_32_REG                 (ISP_DPE_BASE + 0x380)
#define DVS_ME_33_REG                 (ISP_DPE_BASE + 0x384)
#define DVS_ME_34_REG                 (ISP_DPE_BASE + 0x388)
#define DVS_ME_35_REG                 (ISP_DPE_BASE + 0x38C)
#define DVS_ME_36_REG                 (ISP_DPE_BASE + 0x390)
#define DVS_ME_37_REG                 (ISP_DPE_BASE + 0x394)
#define DVS_ME_38_REG                 (ISP_DPE_BASE + 0x398)
#define DVS_ME_39_REG                 (ISP_DPE_BASE + 0x39C)
#define DVS_DEBUG_REG                 (ISP_DPE_BASE + 0x3F4)
#define DVS_ME_RESERVED_REG           (ISP_DPE_BASE + 0x3F8)
#define DVS_ME_ATPG_REG               (ISP_DPE_BASE + 0x3FC)
#define DVS_ME_40_REG                 (ISP_DPE_BASE + 0x400)
#define DVS_ME_41_REG                 (ISP_DPE_BASE + 0x404)
#define DVS_ME_42_REG                 (ISP_DPE_BASE + 0x408)
#define DVS_ME_43_REG                 (ISP_DPE_BASE + 0x40C)
#define DVS_ME_44_REG                 (ISP_DPE_BASE + 0x410)
#define DVS_ME_45_REG                 (ISP_DPE_BASE + 0x414)
#define DVS_ME_46_REG                 (ISP_DPE_BASE + 0x418)
#define DVS_ME_47_REG                 (ISP_DPE_BASE + 0x41C)
#define DVS_ME_48_REG                 (ISP_DPE_BASE + 0x420)
#define DVS_ME_49_REG                 (ISP_DPE_BASE + 0x424)
#define DVS_ME_50_REG                 (ISP_DPE_BASE + 0x428)
#define DVS_ME_51_REG                 (ISP_DPE_BASE + 0x42C)
#define DVS_ME_52_REG                 (ISP_DPE_BASE + 0x430)
#define DVS_ME_53_REG                 (ISP_DPE_BASE + 0x434)
#define DVS_ME_54_REG                 (ISP_DPE_BASE + 0x438)
#define DVS_ME_55_REG                 (ISP_DPE_BASE + 0x43C)
#define DVS_ME_56_REG                 (ISP_DPE_BASE + 0x440)
#define DVS_ME_57_REG                 (ISP_DPE_BASE + 0x444)
#define DVS_ME_58_REG                 (ISP_DPE_BASE + 0x448)
#define DVS_ME_59_REG                 (ISP_DPE_BASE + 0x44C)
#define DVS_OCC_PQ_0_REG              (ISP_DPE_BASE + 0x600)
#define DVS_OCC_PQ_1_REG              (ISP_DPE_BASE + 0x604)
#define DVS_OCC_PQ_2_REG              (ISP_DPE_BASE + 0x608)
#define DVS_OCC_PQ_3_REG              (ISP_DPE_BASE + 0x60C)
#define DVS_OCC_PQ_4_REG              (ISP_DPE_BASE + 0x610)
#define DVS_OCC_PQ_5_REG              (ISP_DPE_BASE + 0x614)
#define DVS_OCC_PQ_10_REG             (ISP_DPE_BASE + 0x628)
#define DVS_OCC_PQ_11_REG             (ISP_DPE_BASE + 0x62C)
#define DVS_OCC_PQ_12_REG             (ISP_DPE_BASE + 0x630)
#define DVS_OCC_ATPG_REG              (ISP_DPE_BASE + 0x634)
#define DVS_OCC_HIST0_REG             (ISP_DPE_BASE + 0x640)
#define DVS_OCC_HIST1_REG             (ISP_DPE_BASE + 0x644)
#define DVS_OCC_HIST2_REG             (ISP_DPE_BASE + 0x648)
#define DVS_OCC_HIST3_REG             (ISP_DPE_BASE + 0x64C)
#define DVS_OCC_HIST4_REG             (ISP_DPE_BASE + 0x650)
#define DVS_OCC_HIST5_REG             (ISP_DPE_BASE + 0x654)
#define DVS_OCC_HIST6_REG             (ISP_DPE_BASE + 0x658)
#define DVS_OCC_HIST7_REG             (ISP_DPE_BASE + 0x65C)
#define DVS_OCC_HIST8_REG             (ISP_DPE_BASE + 0x660)
#define DVS_OCC_HIST9_REG             (ISP_DPE_BASE + 0x664)
#define DVS_OCC_HIST10_REG            (ISP_DPE_BASE + 0x668)
#define DVS_OCC_HIST11_REG            (ISP_DPE_BASE + 0x66C)
#define DVS_OCC_HIST12_REG            (ISP_DPE_BASE + 0x670)
#define DVS_OCC_HIST13_REG            (ISP_DPE_BASE + 0x674)
#define DVS_OCC_HIST14_REG            (ISP_DPE_BASE + 0x678)
#define DVS_OCC_HIST15_REG            (ISP_DPE_BASE + 0x67C)
#define DVS_OCC_HIST16_REG            (ISP_DPE_BASE + 0x680)
#define DVS_OCC_HIST17_REG            (ISP_DPE_BASE + 0x684)
#define DVS_OCC_HIST18_REG            (ISP_DPE_BASE + 0x688)
#define DVS_OCC_LDV0_REG              (ISP_DPE_BASE + 0x68C)
#define DVP_CTRL00_REG                (ISP_DPE_BASE + 0x800)
#define DVP_CTRL01_REG                (ISP_DPE_BASE + 0x804)
#define DVP_CTRL02_REG                (ISP_DPE_BASE + 0x808)
#define DVP_CTRL03_REG                (ISP_DPE_BASE + 0x80C)
#define DVP_CTRL04_REG                (ISP_DPE_BASE + 0x810)
#define DVP_CTRL05_REG                (ISP_DPE_BASE + 0x814)
#define DVP_CTRL07_REG                (ISP_DPE_BASE + 0x81C)
#define DVP_IRQ_00_REG                (ISP_DPE_BASE + 0x840)
#define DVP_IRQ_01_REG                (ISP_DPE_BASE + 0x844)
#define DVP_CTRL_STATUS0_REG          (ISP_DPE_BASE + 0x850)
#define DVP_CTRL_STATUS1_REG          (ISP_DPE_BASE + 0x854)
#define DVP_IRQ_STATUS_REG            (ISP_DPE_BASE + 0x85C)
#define DVP_FRM_STATUS0_REG           (ISP_DPE_BASE + 0x860)
#define DVP_FRM_STATUS1_REG           (ISP_DPE_BASE + 0x864)
#define DVP_FRM_STATUS2_REG           (ISP_DPE_BASE + 0x868)
#define DVP_FRM_STATUS3_REG           (ISP_DPE_BASE + 0x86C)
#define DVP_CUR_STATUS_REG            (ISP_DPE_BASE + 0x870)
#define DVP_SRC_00_REG                (ISP_DPE_BASE + 0x880)
#define DVP_SRC_01_REG                (ISP_DPE_BASE + 0x884)
#define DVP_SRC_02_REG                (ISP_DPE_BASE + 0x888)
#define DVP_SRC_03_REG                (ISP_DPE_BASE + 0x88C)
#define DVP_SRC_04_REG                (ISP_DPE_BASE + 0x890)
#define DVP_SRC_05_Y_FRM0_REG         (ISP_DPE_BASE + 0x894)
#define DVP_SRC_06_Y_FRM1_REG         (ISP_DPE_BASE + 0x898)
#define DVP_SRC_07_Y_FRM2_REG         (ISP_DPE_BASE + 0x89C)
#define DVP_SRC_08_Y_FRM3_REG         (ISP_DPE_BASE + 0x8A0)
#define DVP_SRC_09_C_FRM0_REG         (ISP_DPE_BASE + 0x8A4)
#define DVP_SRC_10_C_FRM1_REG         (ISP_DPE_BASE + 0x8A8)
#define DVP_SRC_11_C_FRM2_REG         (ISP_DPE_BASE + 0x8AC)
#define DVP_SRC_12_C_FRM3_REG         (ISP_DPE_BASE + 0x8B0)
#define DVP_SRC_13_OCCDV0_REG         (ISP_DPE_BASE + 0x8B4)
#define DVP_SRC_14_OCCDV1_REG         (ISP_DPE_BASE + 0x8B8)
#define DVP_SRC_15_OCCDV2_REG         (ISP_DPE_BASE + 0x8BC)
#define DVP_SRC_16_OCCDV3_REG         (ISP_DPE_BASE + 0x8C0)
#define DVP_SRC_17_CRM_REG            (ISP_DPE_BASE + 0x8C4)
#define DVP_SRC_18_ASF_RMDV_REG       (ISP_DPE_BASE + 0x8C8)
#define DVP_SRC_19_ASF_RDDV_REG       (ISP_DPE_BASE + 0x8CC)
#define DVP_SRC_20_ASF_DV0_REG        (ISP_DPE_BASE + 0x8D0)
#define DVP_SRC_21_ASF_DV1_REG        (ISP_DPE_BASE + 0x8D4)
#define DVP_SRC_22_ASF_DV2_REG        (ISP_DPE_BASE + 0x8D8)
#define DVP_SRC_23_ASF_DV3_REG        (ISP_DPE_BASE + 0x8DC)
#define DVP_SRC_24_WMF_RDDV_REG       (ISP_DPE_BASE + 0x8E0)
#define DVP_SRC_25_WMF_HFDV_REG       (ISP_DPE_BASE + 0x8E4)
#define DVP_SRC_26_WMF_DV0_REG        (ISP_DPE_BASE + 0x8E8)
#define DVP_SRC_27_WMF_DV1_REG        (ISP_DPE_BASE + 0x8EC)
#define DVP_SRC_28_WMF_DV2_REG        (ISP_DPE_BASE + 0x8F0)
#define DVP_SRC_29_WMF_DV3_REG        (ISP_DPE_BASE + 0x8F4)
#define DVP_CORE_00_REG               (ISP_DPE_BASE + 0x900)
#define DVP_CORE_01_REG               (ISP_DPE_BASE + 0x904)
#define DVP_CORE_02_REG               (ISP_DPE_BASE + 0x908)
#define DVP_CORE_03_REG               (ISP_DPE_BASE + 0x90C)
#define DVP_CORE_04_REG               (ISP_DPE_BASE + 0x910)
#define DVP_CORE_05_REG               (ISP_DPE_BASE + 0x914)
#define DVP_CORE_06_REG               (ISP_DPE_BASE + 0x918)
#define DVP_CORE_07_REG               (ISP_DPE_BASE + 0x91C)
#define DVP_CORE_08_REG               (ISP_DPE_BASE + 0x920)
#define DVP_CORE_09_REG               (ISP_DPE_BASE + 0x924)
#define DVP_CORE_10_REG               (ISP_DPE_BASE + 0x928)
#define DVP_CORE_11_REG               (ISP_DPE_BASE + 0x92C)
#define DVP_CORE_12_REG               (ISP_DPE_BASE + 0x930)
#define DVP_CORE_13_REG               (ISP_DPE_BASE + 0x934)
#define DVP_CORE_14_REG               (ISP_DPE_BASE + 0x938)
#define DVP_CORE_15_REG               (ISP_DPE_BASE + 0x93C)
#define DVP_CORE_16_REG               (ISP_DPE_BASE + 0x940)
#define DVP_CORE_17_REG               (ISP_DPE_BASE + 0x944)
#define DVP_CORE_18_REG               (ISP_DPE_BASE + 0x948)
#define DVP_CORE_19_REG               (ISP_DPE_BASE + 0x94C)
#define DVP_SRC_CTRL_REG              (ISP_DPE_BASE + 0x9F4)
#define DVP_CTRL_RESERVED_REG         (ISP_DPE_BASE + 0x9F8)
#define DVP_CTRL_ATPG_REG             (ISP_DPE_BASE + 0x9FC)
#define DVP_CRC_OUT_0_REG             (ISP_DPE_BASE + 0xA00)
#define DVP_CRC_OUT_1_REG             (ISP_DPE_BASE + 0xA04)
#define DVP_CRC_OUT_2_REG             (ISP_DPE_BASE + 0xA08)
#define DVP_CRC_CTRL_REG              (ISP_DPE_BASE + 0xA60)
#define DVP_CRC_OUT_REG               (ISP_DPE_BASE + 0xA64)
#define DVP_CRC_IN_REG                (ISP_DPE_BASE + 0xA6C)
#define DVP_DRAM_STA_REG              (ISP_DPE_BASE + 0xA70)
#define DVP_DRAM_ULT_REG              (ISP_DPE_BASE + 0xA74)
#define DVP_DRAM_PITCH_REG            (ISP_DPE_BASE + 0xA78)
#define DVP_DRAM_SEC_0_REG            (ISP_DPE_BASE + 0xA7C)
#define DVP_DRAM_SEC_1_REG            (ISP_DPE_BASE + 0xA80)
#define DVP_DRAM_AXSLC_REG            (ISP_DPE_BASE + 0xA84)
#define DVP_CORE_CRC_IN_REG           (ISP_DPE_BASE + 0xA8C)
#define DVP_EXT_SRC_13_OCCDV0_REG     (ISP_DPE_BASE + 0xBB4)
#define DVP_EXT_SRC_14_OCCDV1_REG     (ISP_DPE_BASE + 0xBB8)
#define DVP_EXT_SRC_15_OCCDV2_REG     (ISP_DPE_BASE + 0xBBC)
#define DVP_EXT_SRC_16_OCCDV3_REG     (ISP_DPE_BASE + 0xBC0)
#define DVP_EXT_SRC_18_ASF_RMDV_REG   (ISP_DPE_BASE + 0xBC8)
#define DVP_EXT_SRC_19_ASF_RDDV_REG   (ISP_DPE_BASE + 0xBCC)
#define DVP_EXT_SRC_20_ASF_DV0_REG    (ISP_DPE_BASE + 0xBD0)
#define DVP_EXT_SRC_21_ASF_DV1_REG    (ISP_DPE_BASE + 0xBD4)
#define DVP_EXT_SRC_22_ASF_DV2_REG    (ISP_DPE_BASE + 0xBD8)
#define DVP_EXT_SRC_23_ASF_DV3_REG    (ISP_DPE_BASE + 0xBDC)
#define DVP_EXT_SRC_24_WMF_RDDV_REG   (ISP_DPE_BASE + 0xBE0)
#define DVGF_CTRL_00_REG              (ISP_DPE_BASE + 0xC00) //DVGF
#define DVGF_CTRL_01_REG              (ISP_DPE_BASE + 0xC04)
#define DVGF_CTRL_02_REG              (ISP_DPE_BASE + 0xC08)
#define DVGF_CTRL_03_REG              (ISP_DPE_BASE + 0xC0C)
#define DVGF_CTRL_05_REG              (ISP_DPE_BASE + 0xC14)
#define DVGF_CTRL_07_REG              (ISP_DPE_BASE + 0xC1C)
#define DVGF_IRQ_00_REG               (ISP_DPE_BASE + 0xC20)
#define DVGF_IRQ_01_REG               (ISP_DPE_BASE + 0xC24)
#define DVGF_CTRL_STATUS0_REG         (ISP_DPE_BASE + 0xC30)
#define DVGF_CTRL_STATUS1_REG         (ISP_DPE_BASE + 0xC34)
#define DVGF_IRQ_STATUS_REG           (ISP_DPE_BASE + 0xC3C)
#define DVGF_FRM_STATUS_REG           (ISP_DPE_BASE + 0xC40)
#define DVGF_CUR_STATUS_REG           (ISP_DPE_BASE + 0xC44)
#define DVGF_CRC_CTRL_REG             (ISP_DPE_BASE + 0xC50)
#define DVGF_CRC_OUT_REG              (ISP_DPE_BASE + 0xC54)
#define DVGF_CRC_IN_REG               (ISP_DPE_BASE + 0xC58)
#define DVGF_CRC_OUT_0_REG            (ISP_DPE_BASE + 0xC5C)
#define DVGF_CRC_OUT_1_REG            (ISP_DPE_BASE + 0xC60)
#define DVGF_CRC_OUT_2_REG            (ISP_DPE_BASE + 0xC64)
#define DVGF_CORE_CRC_IN_REG          (ISP_DPE_BASE + 0xC68)
#define DVGF_DRAM_STA_REG             (ISP_DPE_BASE + 0xC70)
#define DVGF_DRAM_PITCH_REG           (ISP_DPE_BASE + 0xC78)
#define DVGF_DRAM_SEC_0_REG           (ISP_DPE_BASE + 0xC7C)
#define DVGF_DRAM_SEC_1_REG           (ISP_DPE_BASE + 0xC80)
#define DVGF_DRAM_AXSLC_REG           (ISP_DPE_BASE + 0xC84)
#define DVGF_CTRL_STATUS_32b_00_REG   (ISP_DPE_BASE + 0xC90)
#define DVGF_CTRL_STATUS_32b_01_REG   (ISP_DPE_BASE + 0xC94)
#define DVGF_CTRL_STATUS_32b_02_REG   (ISP_DPE_BASE + 0xC98)
#define DVGF_CTRL_STATUS_32b_03_REG   (ISP_DPE_BASE + 0xC9C)
#define DVGF_CTRL_STATUS_32b_04_REG   (ISP_DPE_BASE + 0xCA0)
#define DVGF_CTRL_RESERVED_REG        (ISP_DPE_BASE + 0xCF8)
#define DVGF_CTRL_ATPG_REG            (ISP_DPE_BASE + 0xCFC)
#define DVGF_SRC_00_REG               (ISP_DPE_BASE + 0xD30)
#define DVGF_SRC_01_REG               (ISP_DPE_BASE + 0xD34)
#define DVGF_SRC_02_REG               (ISP_DPE_BASE + 0xD38)
#define DVGF_SRC_04_REG               (ISP_DPE_BASE + 0xD40)
#define DVGF_SRC_05_REG               (ISP_DPE_BASE + 0xD44)
#define DVGF_SRC_06_REG               (ISP_DPE_BASE + 0xD48)
#define DVGF_SRC_07_REG               (ISP_DPE_BASE + 0xD4C)
#define DVGF_SRC_08_REG               (ISP_DPE_BASE + 0xD50)
#define DVGF_SRC_09_REG               (ISP_DPE_BASE + 0xD54)
#define DVGF_SRC_10_REG               (ISP_DPE_BASE + 0xD58)
#define DVGF_SRC_11_REG               (ISP_DPE_BASE + 0xD5C)
#define DVGF_SRC_12_REG               (ISP_DPE_BASE + 0xD60)
#define DVGF_SRC_13_REG               (ISP_DPE_BASE + 0xD64)
#define DVGF_SRC_14_REG               (ISP_DPE_BASE + 0xD68)
#define DVGF_SRC_15_REG               (ISP_DPE_BASE + 0xD6C)
#define DVGF_SRC_16_REG               (ISP_DPE_BASE + 0xD70)
#define DVGF_SRC_17_REG               (ISP_DPE_BASE + 0xD74)
#define DVGF_SRC_18_REG               (ISP_DPE_BASE + 0xD78)
#define DVGF_SRC_19_REG               (ISP_DPE_BASE + 0xD7C)
#define DVGF_SRC_20_REG               (ISP_DPE_BASE + 0xD80)
#define DVGF_SRC_21_REG               (ISP_DPE_BASE + 0xD84)
#define DVGF_SRC_22_REG               (ISP_DPE_BASE + 0xD88)
#define DVGF_SRC_23_REG               (ISP_DPE_BASE + 0xD8C)
#define DVGF_SRC_24_REG               (ISP_DPE_BASE + 0xD90)
#define DVGF_CORE_00_REG              (ISP_DPE_BASE + 0xE00)
#define DVGF_CORE_01_REG              (ISP_DPE_BASE + 0xE04)
#define DVGF_CORE_02_REG              (ISP_DPE_BASE + 0xE08)
#define DVGF_CORE_03_REG              (ISP_DPE_BASE + 0xE0C)
#define DVGF_CORE_05_REG              (ISP_DPE_BASE + 0xE14)
#define DVGF_CORE_06_REG              (ISP_DPE_BASE + 0xE18)
#define DVGF_CORE_07_REG              (ISP_DPE_BASE + 0xE1C)
#define DVGF_CORE_08_REG              (ISP_DPE_BASE + 0xE20)
#define DVGF_CORE_09_REG              (ISP_DPE_BASE + 0xE24)
#define DVGF_CORE_10_REG              (ISP_DPE_BASE + 0xE28)
#define DVGF_CORE_11_REG              (ISP_DPE_BASE + 0xE2C)
#define DVGF_CORE_12_REG              (ISP_DPE_BASE + 0xE30)
#define DVGF_CORE_13_REG              (ISP_DPE_BASE + 0xE34)
#define DVGF_CORE_14_REG              (ISP_DPE_BASE + 0xE38)
#define DVGF_CORE_15_REG              (ISP_DPE_BASE + 0xE3C)
#define DVGF_CORE_16_REG              (ISP_DPE_BASE + 0xE40)
#define DVGF_CORE_17_REG              (ISP_DPE_BASE + 0xE44)
#define DVGF_CORE_18_REG              (ISP_DPE_BASE + 0xE48)
#define DPE_MAX_REG_CNT              (0xBE0 >> 2)

static struct clk_bulk_data isp8_dpe_clks[] = {
	// { .id = "CLK_CK2_DPE_SEL" },
	{ .id = "CLK_CAM_MAIN_CAM" },
	{ .id = "CLK_CAMSYS_IPE_LARB19" },
	{ .id = "CLK_CAMSYS_IPE_DPE" },
	{ .id = "CLK_CAMSYS_IPE_FUS" },
	{ .id = "CLK_CAMSYS_IPE_DHZE" },
	{ .id = "CLK_CAMSYS_IPE_GALS" },
};

/**************************************************************
 *
 **************************************************************/
static inline unsigned int DPE_MsToJiffies(unsigned int Ms)
{
	return ((Ms * HZ + 512) >> 10);
}
/**************************************************************
 *
 **************************************************************/
static inline unsigned int DPE_UsToJiffies(unsigned int Us)
{
	return (((Us / 1000) * HZ + 512) >> 10);
}
/**************************************************************
 *
 **************************************************************/
static inline unsigned int
DPE_GetIRQState(unsigned int type, unsigned int userNumber, unsigned int stus,
		enum DPE_PROCESS_ID_ENUM whichReq, int ProcessID)
{
	unsigned int ret = 0;
	unsigned int p;
	unsigned long flags;

	p = ProcessID % IRQ_USER_NUM_MAX;
	// LOG_INF("DPE_GetIRQState p= %d,stus  %d DPE_I.NT_ST= %lu\n", p, stus, DPE_INT_ST);
	spin_lock_irqsave(&(DPEInfo.SpinLockIrq[type]), flags);
	if (stus & DPE_INT_ST) {

		//LOG_INF("GetIRQState DpeIrqCnt = %d,ProcessID[p] = %d,ProcessID = %d\n",
		//DPEInfo.IrqInfo.DpeIrqCnt[p], DPEInfo.IrqInfo.ProcessID[p], ProcessID);

		ret = ((DPEInfo.IrqInfo.DpeIrqCnt[p] > 0) &&
		       (DPEInfo.IrqInfo.ProcessID[p] == ProcessID));

		//LOG_INF("GetIRQ ret = %d\n", ret);
		//LOG_INF("GetIRQState DVS_only_en = %d,DVP_only_en = %d,DVGF_only_en = %d\n",
		//DVS_only_en, DVP_only_en, DVGF_only_en);
		//LOG_INF("GetIRQState Get_DVS_IRQ = %d,Get_DVP_IRQ = %d,Get_DVGF_IRQ = %d\n",
		//Get_DVS_IRQ, Get_DVP_IRQ, Get_DVGF_IRQ);


	} else {
		LOG_ERR("EWIRQ,type:%d,u:%d,stat:%d,wReq:%d,PID:0x%x\n",
			type, userNumber, stus, p, ProcessID);
	}
	spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[type]), flags);
	/*  */
	return ret;
}
/**************************************************************
 *
 **************************************************************/
static inline unsigned int DPE_JiffiesToMs(unsigned int Jiffies)
{
	return ((Jiffies * 1000) / HZ);
}
#define RegDump(start, end)                                                \
{                                                                          \
unsigned int i;                                                            \
for (i = start; i <= end; i += 0x10) {                                     \
	LOG_DBG("[0x%08X %08X],[0x%08X %08X],[0x%08X %08X],[0x%08X %08X]", \
		(unsigned int)(ISP_DPE_BASE + i),                          \
		(unsigned int)DPE_RD32(ISP_DPE_BASE + i),                  \
		(unsigned int)(ISP_DPE_BASE + i + 0x4),                    \
		(unsigned int)DPE_RD32(ISP_DPE_BASE + i +                  \
				       0x4),                               \
		(unsigned int)(ISP_DPE_BASE + i + 0x8),                    \
		(unsigned int)DPE_RD32(ISP_DPE_BASE + i +                  \
				       0x8),                               \
		(unsigned int)(ISP_DPE_BASE + i + 0xc),                    \
		(unsigned int)DPE_RD32(ISP_DPE_BASE + i +                  \
				       0xc));                              \
}                                                                          \
}
static bool dpe_get_dma_buffer(struct tee_mmu *mmu, int fd)
{
	struct dma_buf *buf;
	int Get_SMMU = 0;

	// LOG_INF("get_dma_buffer_fd= %d\n", fd);
	if (fd <= 0)
		return false;
	buf = dma_buf_get(fd);
	// LOG_INF("dpe_get_dma_buffer buf= %d\n", buf);
	if (IS_ERR(buf))
		return false;
	// LOG_INF("buf_addr = %x\n", buf);
	mmu->dma_buf = buf;

	Get_SMMU = smmu_v3_enabled();
	// LOG_INF("Get_SMMU = %x\n", Get_SMMU);
	if (Get_SMMU == 1) {
		if (smmudev != NULL)
			mmu->attach = dma_buf_attach(mmu->dma_buf, smmudev);
	} else {
		if (gdev != NULL)
			mmu->attach = dma_buf_attach(mmu->dma_buf, gdev);
	}
	// LOG_INF("mmu->attach = %x\n", mmu->attach);

	if (IS_ERR(mmu->attach))
		goto err_attach;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	mmu->sgt = dma_buf_map_attachment_unlocked(mmu->attach,
	DMA_BIDIRECTIONAL);
#else
	mmu->sgt = dma_buf_map_attachment(mmu->attach,
	DMA_BIDIRECTIONAL);
#endif
	// LOG_INF("mmu->sgt = %x\n", mmu->sgt);

	if (IS_ERR(mmu->sgt))
		goto err_map;

	return true;
err_map:
		dma_buf_detach(mmu->dma_buf, mmu->attach);
		LOG_ERR("err_map!\n");
		return false;
err_attach:
	LOG_ERR("err_attach!\n");
	dma_buf_put(mmu->dma_buf);
	return false;
}
/**************************************************************
 *
 **************************************************************/
signed int dpe_enque_cb(struct frame *frames, void *req, unsigned int reqcnt)
{
	//unsigned int f, fcnt, t, ucnt;
	//unsigned int pd_frame_num = 0;
	unsigned int ucnt = 0;
	unsigned int DVP_16bitMode;
	unsigned int WMF_RD_EN;
	unsigned int WMF_F_EN;
	unsigned int WMF_FILT_Ofs;
	unsigned int DPE_P4_EN;
	unsigned int en_idx;

#ifdef IOVA_TO_PA
	uint64_t iova_temp = 0x200000000;
	uint64_t pgpa;
	struct iommu_domain *domain;
#endif
	unsigned int success = 0;

	/*TODO: define engine request struct */
	struct DPE_Request *_req;

	_req = (struct DPE_Request *) req;
	if (frames == NULL || _req == NULL)
		return -1;
#ifdef IOVA_TO_PA
	domain = iommu_get_domain_for_dev(gdev);
#endif

	ucnt = 0;
	if (DPE_debug_log_en == 1)
		LOG_INF("dpe enque start\n");

	mutex_lock(&gFDMutex);
	DVP_16bitMode = (_req->m_pDpeConfig[ucnt].Dpe_is16BitMode);
	WMF_RD_EN = (_req->m_pDpeConfig[ucnt].Dpe_DVPSettings.SubModule_EN.wmf_rd_en);
	WMF_F_EN = (_req->m_pDpeConfig[ucnt].Dpe_DVPSettings.SubModule_EN.wmf_filt_en);
	mutex_unlock(&gFDMutex);

	if ((_req->m_pDpeConfig[ucnt].Dpe_engineSelect == MODE_DVS_ONLY) ||
		(_req->m_pDpeConfig[ucnt].Dpe_engineSelect == MODE_DVS_DVP_BOTH)) {

		mutex_lock(&gFDMutex);
		DPE_P4_EN = (((_req->m_pDpeConfig[ucnt].Dpe_DVSSettings.TuningBuf_ME.DVS_ME_28) &
						0x400) >> 10);
		if (DVS_Num == 0) {
			SrcImg_Y_L_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!SrcImg_Y_L_mmu)) {
				LOG_ERR("SrcImg_Y_L_mmu alloc fail\n");
				return -1;
			}
			SrcImg_Y_R_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!SrcImg_Y_R_mmu)) {
				LOG_ERR("SrcImg_Y_R_mmu alloc fail\n");
				return -1;
			}
			OutBuf_OCC_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!OutBuf_OCC_mmu)) {
				LOG_ERR("OutBuf_OCC_mmu alloc fail\n");
				return -1;
			}
			OutBuf_OCC_Hist_mmu  = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!OutBuf_OCC_Hist_mmu)) {
				LOG_ERR("OutBuf_OCC_Hist_mmu alloc fail\n");
				return -1;
			}
			OutBuf_OCC_Ext_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!OutBuf_OCC_Ext_mmu)) {
				LOG_ERR("OutBuf_OCC_Ext_mmu alloc fail\n");
				return -1;
			}
			SrcImg_Y_L_Pre_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!SrcImg_Y_L_Pre_mmu)) {
				LOG_ERR("SrcImg_Y_L_Pre_mmu alloc fail\n");
				return -1;
			}
			SrcImg_Y_R_Pre_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!SrcImg_Y_R_Pre_mmu)) {
				LOG_ERR("SrcImg_Y_R_Pre_mmu alloc fail\n");
				return -1;
			}
			InBuf_P4_L_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!InBuf_P4_L_mmu)) {
				LOG_ERR("InBuf_P4_L_mmu alloc fail\n");
				return -1;
			}
			InBuf_P4_R_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!InBuf_P4_R_mmu)) {
				LOG_ERR("InBuf_P4_R_mmu alloc fail\n");
				return -1;
			}
		}
		// mutex_unlock(&gFDMutex);

		LOG_INF("YL:%d,YR:%d,OCC:%d,OCC_E:%d,OCC_H:%d,YL_Pre:%d,YR_Pre:%d,P4_L:%d,P4_R:%d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_L_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_R_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ext_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_Output_OCC_Hist_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_L_Pre_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_R_Pre_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_P4_L_DV_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_P4_R_DV_fd);

		// mutex_lock(&gFDMutex);
		//LOG_INF("dpe enque star DVS, P4 = %d\n", DPE_P4_EN);
		// DVS_only_en++;
		DVS_Num++;
		en_idx = reqcnt;
		if (DPE_debug_log_en == 1)
			LOG_INF("DVS_only_en = %d ,DVS_Num = %d\n", DVS_only_en, DVS_Num);


		if (DPE_debug_log_en == 1) {
			LOG_INF("SrcImg_Y_L fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_L_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_L_Ofs);
		}

		success = dpe_get_dma_buffer(&SrcImg_Y_L_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_L_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_L =
			(sg_dma_address(SrcImg_Y_L_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_L_Ofs));
			get_dvs_iova[SrcImg_Y_L] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_InBuf_SrcImg_Y_L iova = %llu, get_dvs_iova[SrcImg_Y_L] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_L, get_dvs_iova[SrcImg_Y_L]);
			}
			#ifdef IOVA_TO_PA
			iova_temp = _req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_L | iova_temp;
			pgpa = iommu_iova_to_phys(domain, iova_temp);
			LOG_INF("Dpe_InBuf_SrcImg_Y_L pgpa = %llu\n", pgpa);
			iova_temp = 0x200000000;
			#endif
		} else {
			LOG_ERR("get Dpe_InBuf_SrcImg_Y_L_fd fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		if (DPE_debug_log_en == 1) {
			LOG_INF("SrcImg_Y_R va = %llu\n",
			_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_R);
			LOG_INF("SrcImg_Y_R fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_R_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_R_Ofs);
		}

		success = dpe_get_dma_buffer(&SrcImg_Y_R_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_R_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_R =
			(sg_dma_address(SrcImg_Y_R_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_R_Ofs));
			get_dvs_iova[SrcImg_Y_R] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_InBuf_SrcImg_Y_R iova = %llu, get_dvs_iova[SrcImg_Y_R] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_R, get_dvs_iova[SrcImg_Y_R]);
			}
		} else {
			LOG_ERR("get Dpe_InBuf_SrcImg_Y_R fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		if (DPE_debug_log_en == 1) {
			LOG_INF("OCC fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ofs);
		}

		success = dpe_get_dma_buffer(&OutBuf_OCC_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_fd);

		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_OutBuf_OCC =
			(sg_dma_address(OutBuf_OCC_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ofs));
			get_dvs_iova[OutBuf_OCC] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_OCC = %llu, get_dvs_iova[OutBuf_OCC] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_OCC, get_dvs_iova[OutBuf_OCC]);
				LOG_INF("=========================================\n");
			}
			#ifdef IOVA_TO_PA
			iova_temp = _req->m_pDpeConfig[ucnt].Dpe_OutBuf_OCC | iova_temp;
			pgpa = iommu_iova_to_phys(domain, iova_temp);
			LOG_INF("Dpe_InBuf_SrcImg_Y_L pgpa = %llx\n", pgpa);
			iova_temp = 0x200000000;
			#endif
		} else {
			LOG_ERR("get Dpe_OutBuf_OCC fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		if (DPE_debug_log_en == 1) {
			LOG_INF("OCC_Ext fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ext_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ext_Ofs);
		}

		success = dpe_get_dma_buffer(&OutBuf_OCC_Ext_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ext_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_OutBuf_OCC_Ext =
			(sg_dma_address(OutBuf_OCC_Ext_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ext_Ofs));
			get_dvs_iova[OutBuf_OCC_EXT] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_OCC_Ext = %llu, get_dvs_iova[OutBuf_OCC_EXT] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_OCC_Ext, get_dvs_iova[OutBuf_OCC_EXT]);
				LOG_INF("=========================================\n");
			}
		} else {
			LOG_ERR("get Dpe_OutBuf_OCC_Ext fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		//OCC_Hist
		if (DPE_debug_log_en == 1) {
			LOG_INF("OCC_Hist fd= %x ,offset = %x\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_Output_OCC_Hist_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_Output_OCC_Hist_Ofs);
		}
		success = dpe_get_dma_buffer(&OutBuf_OCC_Hist_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_Output_OCC_Hist_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_OutBuf_OCC_Hist =
			(sg_dma_address(OutBuf_OCC_Hist_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_Output_OCC_Hist_Ofs));
			get_dvs_iova[OutBuf_OCC_HIST] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_OCC_Hist = %llu, get_dvs_iova[OutBuf_OCC_HIST] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_OCC_Hist, get_dvs_iova[OutBuf_OCC_HIST]);
				LOG_INF("=========================================\n");
			}
		} else {
			LOG_ERR("get Dpe_OutBuf_OCC_Hist fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		//!!!!!
		//---For p4
		//mutex_lock(&gFDMutex);
		if (DPE_P4_EN == 1) {
			if (DPE_debug_log_en == 1) {
				LOG_INF("SrcImg_Y_L_Pre fd= %x ,offset = %x\n",
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_L_Pre_fd,
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_L_Pre_Ofs);
			}
			success = dpe_get_dma_buffer(&SrcImg_Y_L_Pre_mmu[en_idx],
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_L_Pre_fd);
			if (success) {
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_L_Pre =
				sg_dma_address(SrcImg_Y_L_Pre_mmu[en_idx].sgt->sgl) +
				_req->m_pDpeConfig[0].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_L_Pre_Ofs;
				get_dvs_iova[SrcImg_Y_L_PRE] += 1;
				if (DPE_debug_log_en == 1) {
					LOG_INF("Dpe_InBuf_SrcImg_Y_L_Pre = %llu iova[SrcImg_Y_L_PRE]=%d\n",
					_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_L_Pre,
					get_dvs_iova[SrcImg_Y_L_PRE]);
					LOG_INF("====================================\n");
				}
			} else {
				LOG_ERR("get Dpe_InBuf_SrcImg_Y_L_Pre fail\n");
				mutex_unlock(&gFDMutex);
				return ENQUE_FAIL;
			}

			if (DPE_debug_log_en == 1) {
				LOG_INF("SrcImg_Y_R_Pre fd= %x ,offset = %x\n",
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_R_Pre_fd,
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_R_Pre_Ofs);
			}
			success = dpe_get_dma_buffer(&SrcImg_Y_R_Pre_mmu[en_idx],
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_R_Pre_fd);
			if (success) {
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_R_Pre =
				sg_dma_address(SrcImg_Y_R_Pre_mmu[en_idx].sgt->sgl) +
				_req->m_pDpeConfig[0].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_R_Pre_Ofs;
				get_dvs_iova[SrcImg_Y_R_PRE] += 1;
				if (DPE_debug_log_en == 1) {
					LOG_INF("Dpe_InBuf_SrcImg_Y_R_Pre = %llu iova[SrcImg_Y_R_PRE]= %d\n",
					_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_R_Pre,
					get_dvs_iova[SrcImg_Y_R_PRE]);
					LOG_INF("===========================\n");
				}
			}	else {
				LOG_ERR("get Dpe_InBuf_SrcImg_Y_R_Pre fail\n");
				mutex_unlock(&gFDMutex);
				return ENQUE_FAIL;
			}

			if (DPE_debug_log_en == 1) {
				LOG_INF("P4_L_DV fd= %x ,offset = %x\n",
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_P4_L_DV_fd,
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_P4_L_DV_Ofs);
			}

			success = dpe_get_dma_buffer(&InBuf_P4_L_mmu[en_idx],
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_P4_L_DV_fd);
			if (success) {
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_P4_L_DV =
				(sg_dma_address(InBuf_P4_L_mmu[en_idx].sgt->sgl) +
				(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_P4_L_DV_Ofs));
				get_dvs_iova[InBuf_P4_L_DV] += 1;
				if (DPE_debug_log_en == 1) {
					LOG_INF("Dpe_InBuf_P4_L_DV = %llu iova[InBuf_P4_L_DV]= %d\n",
					_req->m_pDpeConfig[ucnt].Dpe_InBuf_P4_L_DV,
					get_dvs_iova[InBuf_P4_L_DV]);
					LOG_INF("===========================\n");
				}
			} else {
				LOG_ERR("get Dpe_InBuf_P4_L_DV fail\n");
				mutex_unlock(&gFDMutex);
				return ENQUE_FAIL;
			}

			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_InBuf_P4_R_DV fd= %x ,offset = %x\n",
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_P4_R_DV_fd,
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_P4_R_DV_Ofs);
			}

			success = dpe_get_dma_buffer(&InBuf_P4_R_mmu[en_idx],
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_P4_R_DV_fd);
			if (success) {
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_P4_R_DV =
				(sg_dma_address(InBuf_P4_R_mmu[en_idx].sgt->sgl) +
				(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_P4_R_DV_Ofs));
				get_dvs_iova[InBuf_P4_R_DV] += 1;
				if (DPE_debug_log_en == 1) {
					LOG_INF("Dpe_InBuf_P4_R_DV = %llu iova[InBuf_P4_R_DV] = %d\n",
					_req->m_pDpeConfig[ucnt].Dpe_InBuf_P4_R_DV,
					get_dvs_iova[InBuf_P4_R_DV]);
					LOG_INF("===================\n");
				}
			} else {
				LOG_ERR("get Dpe_InBuf_P4_R_DV fail\n");
				mutex_unlock(&gFDMutex);
				return ENQUE_FAIL;
			}
		}
		mutex_unlock(&gFDMutex);

		if (DPE_debug_log_en == 1)
			LOG_INF("dvs_buffer end\n");

	}
	///////DVP
	if ((_req->m_pDpeConfig[ucnt].Dpe_engineSelect == MODE_DVP_ONLY) ||
		(_req->m_pDpeConfig[ucnt].Dpe_engineSelect == MODE_DVS_DVP_BOTH)) {

		mutex_lock(&gFDMutex);
		if (DVP_Num == 0) {
			SrcImg_Y_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!SrcImg_Y_mmu)) {
				LOG_ERR("SrcImg_Y_L_mmu alloc fail\n");
				return -1;
			}
			SrcImg_C_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!SrcImg_C_mmu)) {
				LOG_ERR("SrcImg_C_mmu alloc fail\n");
				return -1;
			}
			OutBuf_CRM_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!OutBuf_CRM_mmu)) {
				LOG_ERR("OutBuf_CRM_mmu alloc fail\n");
				return -1;
			}
			InBuf_OCC_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!InBuf_OCC_mmu)) {
				LOG_ERR("InBuf_OCC_mmu alloc fail\n");
				return -1;
			}
			ASF_RD_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!ASF_RD_mmu)) {
				LOG_ERR("ASF_RD_mmu alloc fail\n");
				return -1;
			}
			ASF_HF_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!ASF_HF_mmu)) {
				LOG_ERR("ASF_HF_mmu alloc fail\n");
				return -1;
			}
			WMF_RD_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!WMF_RD_mmu)) {
				LOG_ERR("WMF_RD_mmu alloc fail\n");
				return -1;
			}
			WMF_FILT_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!WMF_FILT_mmu)) {
				LOG_ERR("WMF_FILT_mmu alloc fail\n");
				return -1;
			}
			InBuf_OCC_Ext_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!InBuf_OCC_Ext_mmu)) {
				LOG_ERR("InBuf_OCC_Ext_mmu alloc fail\n");
				return -1;
			}
			ASF_RD_Ext_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!ASF_RD_Ext_mmu)) {
				LOG_ERR("ASF_RD_Ext_mmu alloc fail\n");
				return -1;
			}
			ASF_HF_Ext_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!ASF_HF_Ext_mmu)) {
				LOG_ERR("ASF_HF_Ext_mmu alloc fail\n");
				return -1;
			}
		}
		// mutex_unlock(&gFDMutex);
		LOG_INF("DVP:Y:%d,C:%d,CRM:%d,OCC:%d,OCC_E:%d,WT_RD:%d,ASF_RD:%d,ASF_HF:%d,WMF_FILT:%d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_CRM_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_OCC_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_OCC_Ext_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_RD_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_RD_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_HF_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_FILT_fd);

		// mutex_lock(&gFDMutex);
		// DVP_only_en++;
		DVP_Num++;
		en_idx = reqcnt;
		//mutex_unlock(&gFDMutex);
		// if (DPE_debug_log_en == 1)
		// LOG_INF("DVP_only_en = %d ,DVP_Num = %d\n", DVP_only_en, DVP_Num);

		if (DPE_debug_log_en == 1) {
			LOG_INF("SrcImg_Y fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_Ofs);
		}
		//mutex_lock(&gFDMutex);
		success = dpe_get_dma_buffer(&SrcImg_Y_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y =
			(sg_dma_address(SrcImg_Y_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_Ofs));
			get_dvp_iova[SrcImg_Y] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_InBuf_SrcImg_Y = %llu, get_dvp_iova[SrcImg_Y] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y, get_dvp_iova[SrcImg_Y]);
				LOG_INF("==========================================\n");
			}
		} else {
			LOG_ERR("get Dpe_InBuf_SrcImg_Y fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}
		//mutex_unlock(&gFDMutex);

		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_InBuf_SrcImg_C fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_Ofs);
		}

		//mutex_lock(&gFDMutex);
		success = dpe_get_dma_buffer(&SrcImg_C_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_C =
			(sg_dma_address(SrcImg_C_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_Ofs));
			get_dvp_iova[SrcImg_C] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("SrcImg_C = %llu, get_dvp_iova[SrcImg_C] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_C, get_dvp_iova[SrcImg_C]);
				LOG_INF("=======================================\n");
			}
		} else {
			LOG_ERR("get Dpe_InBuf_SrcImg_C fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		//mutex_unlock(&gFDMutex);
		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_InBuf_OCC fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_OCC_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_OCC_Ofs);
		}

		//mutex_lock(&gFDMutex);
		success = dpe_get_dma_buffer(&InBuf_OCC_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_OCC_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_InBuf_OCC =
			(sg_dma_address(InBuf_OCC_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_OCC_Ofs));
			get_dvp_iova[InBuf_OCC] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_InBuf_OCC = %llu, get_dvp_iova[InBuf_OCC] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_OCC, get_dvp_iova[InBuf_OCC]);
				LOG_INF("=======================================\n");
			}
		} else {
			LOG_ERR("get Dpe_InBuf_OCC fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}
		//mutex_unlock(&gFDMutex);
		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_OutBuf_CRM fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_CRM_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_CRM_Ofs);
		}

		//mutex_lock(&gFDMutex);
		success = dpe_get_dma_buffer(&OutBuf_CRM_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_CRM_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_OutBuf_CRM =
			(sg_dma_address(OutBuf_CRM_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_CRM_Ofs));
			get_dvp_iova[OutBuf_CRM] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_CRM = %llu, get_dvp_iova[OutBuf_CRM] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_CRM, get_dvp_iova[OutBuf_CRM]);
				LOG_INF("===========================================\n");
			}
		} else {
			LOG_ERR("get Dpe_OutBuf_CRM fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}
		//mutex_unlock(&gFDMutex);

		//WMF_RD
		if (WMF_RD_EN == 1) {
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_WMF_RD fd = %d offset = %d\n",
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_RD_fd,
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_RD_Ofs);
			}

			//mutex_lock(&gFDMutex);
			success = dpe_get_dma_buffer(&WMF_RD_mmu[en_idx],
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_RD_fd);
			if (success) {
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_WMF_RD =
				(sg_dma_address(WMF_RD_mmu[en_idx].sgt->sgl) +
				(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_RD_Ofs));
				get_dvp_iova[OutBuf_WMF_RD] += 1;
				if (DPE_debug_log_en == 1) {
					LOG_INF("Dpe_OutBuf_WMF_RD = %llu, get_dvp_iova[OutBuf_WMF_RD] = %d\n",
					_req->m_pDpeConfig[ucnt].Dpe_OutBuf_WMF_RD,
					get_dvp_iova[OutBuf_WMF_RD]);
					LOG_INF("==============================================\n");
				}
			} else {
				LOG_ERR("get Dpe_OutBuf_WMF_RD fail\n");
				mutex_unlock(&gFDMutex);
				return ENQUE_FAIL;
			}
				//mutex_unlock(&gFDMutex);
		} else if (WMF_RD_EN == 0) {
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_ASF_RD fd = %d offset = %d\n",
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_RD_fd,
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_RD_Ofs);
			}
			//mutex_lock(&gFDMutex);
			success = dpe_get_dma_buffer(&ASF_RD_mmu[en_idx],
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_RD_fd);
			if (success) {
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_ASF_RD =
				(sg_dma_address(ASF_RD_mmu[en_idx].sgt->sgl) +
				(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_RD_Ofs));
				get_dvp_iova[OutBuf_ASF_RD] += 1;
				if (DPE_debug_log_en == 1) {
					LOG_INF("Dpe_OutBuf_ASF_RD = %llu, get_dvp_iova[OutBuf_ASF_RD] = %d\n",
					_req->m_pDpeConfig[ucnt].Dpe_OutBuf_ASF_RD,
					get_dvp_iova[OutBuf_ASF_RD]);
					LOG_INF("==============================================\n");
				}
			} else {
				LOG_ERR("get Dpe_OutBuf_ASF_RD fail\n");
				mutex_unlock(&gFDMutex);
				return ENQUE_FAIL;
			}
			//mutex_unlock(&gFDMutex);
		}

		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_OutBuf_ASF_HF fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_HF_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_HF_Ofs);
		}
		//spin_lock(&(DPEInfo.SpinLockFD));
		//mutex_lock(&gFDMutex);
		success = dpe_get_dma_buffer(&ASF_HF_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_HF_fd);

		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_OutBuf_ASF_HF =
			(sg_dma_address(ASF_HF_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_HF_Ofs));
			get_dvp_iova[OutBuf_ASF_HF] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_ASF_HF = %llu, get_dvp_iova[OutBuf_ASF_HF] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_ASF_HF,
				get_dvp_iova[OutBuf_ASF_HF]);
				LOG_INF("=========================================\n");
			}
		} else {
			LOG_ERR("get Dpe_OutBuf_ASF_HF fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		if (WMF_F_EN) {
			//mutex_unlock(&gFDMutex);
			if (DVP_16bitMode == 1) {
				LOG_ERR("WMF_F only work in 8 bit mmode\n");
				return ENQUE_FAIL;
			}
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_WMF_FILT fd = %d offset = %d\n",
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_FILT_fd,
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_FILT_Ofs);
			}
			WMF_FILT_Ofs =
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_FILT_Ofs;
			//mutex_lock(&gFDMutex);
			success = dpe_get_dma_buffer(&WMF_FILT_mmu[en_idx],
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_FILT_fd);
			if (success) {
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_WMF_FILT =
				(sg_dma_address(WMF_FILT_mmu[en_idx].sgt->sgl) +
				WMF_FILT_Ofs);
				get_dvp_iova[OutBuf_WMF_FILT] += 1;
			} else {
				LOG_ERR("get Dpe_OutBuf_WMF_FILT fail\n");
				mutex_unlock(&gFDMutex);
				return ENQUE_FAIL;
			}
			//mutex_unlock(&gFDMutex);
		}

		if (DVP_16bitMode == 1) {
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_InBuf_OCC_Ext fd = %d offset = %d\n",
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_OCC_Ext_fd,
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_OCC_Ext_Ofs);
			}

			//mutex_lock(&gFDMutex);
			success = dpe_get_dma_buffer(&InBuf_OCC_Ext_mmu[en_idx],
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_OCC_Ext_fd);
			LOG_DBG("InBuf_OCC_Ext 1 = %d\n", success);
			if (success) {
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_OCC_Ext =
				(sg_dma_address(InBuf_OCC_Ext_mmu[en_idx].sgt->sgl) +
				(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_OCC_Ext_Ofs));
				LOG_DBG("InBuf_OCC_Ext 2 = %d\n", success);
				get_dvp_iova[InBuf_OCC_Ext] += 1;
			} else {
				LOG_ERR("get Dpe_InBuf_OCC_Ext fail\n");
				mutex_unlock(&gFDMutex);
				return ENQUE_FAIL;
			}

			if (WMF_RD_EN == 0) {
				LOG_INF("Dpe_OutBuf_ASF_RD_Ext fd = %d offset = %d\n",
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_RD_Ext_fd,
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_RD_Ext_Ofs);

				//mutex_lock(&gFDMutex);
				success = dpe_get_dma_buffer(&ASF_RD_Ext_mmu[en_idx],
				_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_RD_Ext_fd);
				LOG_INF("Dpe_OutBuf_ASF_RD_Ext 1 = %d\n", success);
				if (success) {
					_req->m_pDpeConfig[ucnt].Dpe_OutBuf_ASF_RD_Ext =
					(sg_dma_address(ASF_RD_Ext_mmu[en_idx].sgt->sgl) +
					(_req->m_pDpeConfig[
					ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_RD_Ext_Ofs));
					LOG_INF("Dpe_OutBuf_ASF_RD_Ext 2 = %d\n", success);
					get_dvp_iova[OutBuf_ASF_RD_Ext] += 1;
				} else {
					LOG_ERR("get Dpe_OutBuf_ASF_RD_Ext fail\n");
					mutex_unlock(&gFDMutex);
					return ENQUE_FAIL;
				}
			//mutex_unlock(&gFDMutex);
			}

			LOG_INF("Dpe_OutBuf_ASF_HF_Ext fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_HF_Ext_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_HF_Ext_Ofs);

			//mutex_lock(&gFDMutex);
			success = dpe_get_dma_buffer(&ASF_HF_Ext_mmu[en_idx],
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_HF_Ext_fd);
			LOG_INF("Dpe_OutBuf_ASF_HF_Ext_fd 1 = %d\n", success);
			if (success) {
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_ASF_HF_Ext =
				(sg_dma_address(ASF_HF_Ext_mmu[en_idx].sgt->sgl) +
				(_req->m_pDpeConfig[
				ucnt].DPE_DMapSettings.Dpe_OutBuf_ASF_HF_Ext_Ofs));

				LOG_INF("Dpe_OutBuf_ASF_HF_Ext = %llu\n",
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_ASF_HF_Ext);
				get_dvp_iova[OutBuf_ASF_HF_Ext] += 1;
			} else {
				LOG_ERR("get Dpe_OutBuf_ASF_HF_Ext fail\n");
				mutex_unlock(&gFDMutex);
				return ENQUE_FAIL;
			}
			//mutex_unlock(&gFDMutex);
		}
		mutex_unlock(&gFDMutex);
	}
	////////DVGF
	if (_req->m_pDpeConfig[ucnt].Dpe_engineSelect == MODE_DVGF_ONLY) {

		mutex_lock(&gFDMutex);
		if (DVGF_Num == 0) {
			DVGF_SrcImg_Y_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!DVGF_SrcImg_Y_mmu)) {
				LOG_ERR("DVGF_SrcImg_Y_mmu alloc fail\n");
				return -1;
			}
			DVGF_SrcImg_C_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!DVGF_SrcImg_C_mmu)) {
				LOG_ERR("DVGF_SrcImg_C_mmu alloc fail\n");
				return -1;
			}
			WT_Fnl_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!WT_Fnl_mmu)) {
				LOG_ERR("WT_Fnl_mmu alloc fail\n");
				return -1;
			}
			RW_IIR_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!RW_IIR_mmu)) {
				LOG_ERR("RW_IIR_mmu alloc fail\n");
				return -1;
			}
			DVGF_WMF_FILT_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!DVGF_WMF_FILT_mmu)) {
				LOG_ERR("DVGF_WMF_FILT_mmu alloc fail\n");
				return -1;
			}
			DVGF_OutBuf_OCC_Ext_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!DVGF_OutBuf_OCC_Ext_mmu)) {
				LOG_ERR("DVGF_OutBuf_OCC_Ext_mmu alloc fail\n");
				return -1;
			}
			SrcImg_Y_Pre_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!SrcImg_Y_Pre_mmu)) {
				LOG_ERR("SrcImg_Y_Pre_mmu alloc fail\n");
				return -1;
			}
			SrcImg_C_Pre_mmu = kzalloc(sizeof(struct tee_mmu) * 4, GFP_KERNEL);
			if ((!SrcImg_C_Pre_mmu)) {
				LOG_ERR("SrcImg_C_Pre_mmu alloc fail\n");
				return -1;
			}
		}
		LOG_INF("Yfd:%d,Cfd:%d,Y_P:%d,C_P:%d,OCC_E:%d,WT_Fnl:%d,IIR:%d,FILT:%d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_Pre_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_Pre_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ext_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WT_Fnl_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_RW_IIR_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_FILT_fd);

		// DVGF_only_en++;
		DVGF_Num++;
		en_idx = reqcnt;

		if (DPE_debug_log_en == 1) {
			LOG_INF("=============DVGF CONFIGs=================\n");
			LOG_INF("SrcImg_Y fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_Ofs);
		}

		success = dpe_get_dma_buffer(&DVGF_SrcImg_Y_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y =
			(sg_dma_address(DVGF_SrcImg_Y_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_Ofs));
			get_dvgf_iova[DVGF_SrcImg_Y] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_InBuf_SrcImg_Y = %llu, get_dvgf_iova[DVGF_SrcImg_Y] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y, get_dvgf_iova[DVGF_SrcImg_Y]);
				LOG_INF("==========================================\n");
			}
		} else {
			LOG_ERR("get Dpe_InBuf_SrcImg_Y fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}


		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_InBuf_SrcImg_C fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_Ofs);
		}

		success = dpe_get_dma_buffer(&DVGF_SrcImg_C_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_C =
			(sg_dma_address(DVGF_SrcImg_C_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_Ofs));
			get_dvgf_iova[DVGF_SrcImg_C] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("SrcImg_C = %llu, get_dvgf_iova[DVGF_SrcImg_C] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_C, get_dvgf_iova[DVGF_SrcImg_C]);
				LOG_INF("=======================================\n");
			}
		} else {
			LOG_ERR("get Dpe_InBuf_SrcImg_C fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_InBuf_SrcImg_Y_Pre fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_Pre_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_Pre_Ofs);
		}

		success = dpe_get_dma_buffer(&SrcImg_Y_Pre_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_Pre_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_Pre =
			sg_dma_address(SrcImg_Y_Pre_mmu[en_idx].sgt->sgl) +
			_req->m_pDpeConfig[0].DPE_DMapSettings.Dpe_InBuf_SrcImg_Y_Pre_Ofs;
			get_dvgf_iova[SrcImg_Y_Pre] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_InBuf_SrcImg_Y_Pre = %llu, get_dvgf_iova[SrcImg_Y_Pre]=%d\n",
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_Y_Pre,
				get_dvgf_iova[SrcImg_Y_Pre]);
				LOG_INF("====================================\n");
			}
		} else {
			LOG_ERR("get Dpe_InBuf_SrcImg_Y_Pre fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_InBuf_SrcImg_C_Pre fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_Pre_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_Pre_Ofs);
		}

		success = dpe_get_dma_buffer(&SrcImg_C_Pre_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_Pre_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_C_Pre =
			sg_dma_address(SrcImg_C_Pre_mmu[en_idx].sgt->sgl) +
			_req->m_pDpeConfig[0].DPE_DMapSettings.Dpe_InBuf_SrcImg_C_Pre_Ofs;
			get_dvgf_iova[SrcImg_C_Pre] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_InBuf_SrcImg_C_Pre = %llu, get_dvgf_iova[SrcImg_C_Pre]= %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_InBuf_SrcImg_C_Pre,
				get_dvgf_iova[SrcImg_C_Pre]);
				LOG_INF("===========================\n");
			}
		}	else {
			LOG_ERR("get Dpe_InBuf_SrcImg_C_Pre fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_OutBuf_OCC_Ext fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ext_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ext_Ofs);
		}

		//mutex_lock(&gFDMutex);
		success = dpe_get_dma_buffer(&DVGF_OutBuf_OCC_Ext_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ext_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_OutBuf_OCC_Ext =
			(sg_dma_address(DVGF_OutBuf_OCC_Ext_mmu[en_idx].sgt->sgl) +
			(_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_OCC_Ext_Ofs));
			get_dvgf_iova[DVGF_OutBuf_OCC_Ext] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_OCC_Ext = %llu, get_dvgf_iova[DVGF_OutBuf_OCC_Ext] = %d\n",
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_OCC_Ext, get_dvgf_iova[DVGF_OutBuf_OCC_Ext]);
				LOG_INF("=========================================\n");
			}
		} else {
			LOG_ERR("get Dpe_OutBuf_OCC_Ext fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}
		//mutex_unlock(&gFDMutex);

		//WMT_Fnl
		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_OutBuf_WT_Fnl fd= %x ,offset = %x\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WT_Fnl_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WT_Fnl_Ofs);
		}
		success = dpe_get_dma_buffer(&WT_Fnl_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WT_Fnl_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_OutBuf_WT_Fnl =
			sg_dma_address(WT_Fnl_mmu[en_idx].sgt->sgl) +
			_req->m_pDpeConfig[0].DPE_DMapSettings.Dpe_OutBuf_WT_Fnl_Ofs;
			get_dvgf_iova[OutBuf_WT_Fnl] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_WT_Fnl = %llu, get_dvgf_iova[OutBuf_WT_Fnl]=%d\n",
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_WT_Fnl,
				get_dvgf_iova[OutBuf_WT_Fnl]);
				LOG_INF("====================================\n");
			}
		} else {
			LOG_ERR("get Dpe_OutBuf_WMT_Fnl fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}
		//RW_IIR
		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_OutBuf_RW_IIR fd= %x ,offset = %x\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_RW_IIR_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_RW_IIR_Ofs);
		}
		success = dpe_get_dma_buffer(&RW_IIR_mmu[en_idx],
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_RW_IIR_fd);
		if (success) {
			_req->m_pDpeConfig[ucnt].Dpe_OutBuf_RW_IIR =
			sg_dma_address(RW_IIR_mmu[en_idx].sgt->sgl) +
			_req->m_pDpeConfig[0].DPE_DMapSettings.Dpe_OutBuf_RW_IIR_Ofs;
			get_dvgf_iova[OutBuf_RW_IIR] += 1;
			if (DPE_debug_log_en == 1) {
				LOG_INF("Dpe_OutBuf_RW_IIR = %llu, get_dvgf_iova[OutBuf_RW_IIR]=%d\n",
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_RW_IIR,
				get_dvgf_iova[OutBuf_RW_IIR]);
				LOG_INF("====================================\n");
			}
		} else {
			LOG_ERR("get Dpe_OutBuf_RW_IIR fail\n");
			mutex_unlock(&gFDMutex);
			return ENQUE_FAIL;
		}

		if (DPE_debug_log_en == 1) {
			LOG_INF("Dpe_OutBuf_WMF_FILT fd = %d offset = %d\n",
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_FILT_fd,
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_FILT_Ofs);
		}
		WMF_FILT_Ofs =
		_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_FILT_Ofs;
			//mutex_lock(&gFDMutex);
			success = dpe_get_dma_buffer(&DVGF_WMF_FILT_mmu[en_idx],
			_req->m_pDpeConfig[ucnt].DPE_DMapSettings.Dpe_OutBuf_WMF_FILT_fd);
			if (success) {
				_req->m_pDpeConfig[ucnt].Dpe_OutBuf_WMF_FILT =
				(sg_dma_address(DVGF_WMF_FILT_mmu[en_idx].sgt->sgl) +
				WMF_FILT_Ofs);
				get_dvgf_iova[DVGF_OutBuf_WMF_FILT] += 1;
			} else {
				LOG_ERR("get Dpe_OutBuf_WMF_FILT fail\n");
				mutex_unlock(&gFDMutex);
				return ENQUE_FAIL;
			}
		mutex_unlock(&gFDMutex);
	}

	memcpy(frames[0].data, &_req->m_pDpeConfig[ucnt],
					sizeof(struct DPE_Config_ISP8));

	if (DPE_debug_log_en == 1) {
		LOG_INF("[%s]1 frm_width:%d!\n", __func__,
		_req->m_pDpeConfig[ucnt].Dpe_DVSSettings.frm_width);
	}

	ucnt++;

	return 0;
}

void mmu_release(struct tee_mmu *mmu, int fd_cnt)
{
	//LOG_INF("mmu_release fd_cnt = %d\n", fd_cnt);
	if (mmu->dma_buf) {
		//LOG_INF("put mmu->dma_buf = %x\n", mmu->dma_buf);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
		dma_buf_unmap_attachment_unlocked(mmu->attach, mmu->sgt, DMA_BIDIRECTIONAL);
#else
		dma_buf_unmap_attachment(mmu->attach, mmu->sgt, DMA_BIDIRECTIONAL);
#endif
		dma_buf_detach(mmu->dma_buf, mmu->attach);
		dma_buf_put(mmu->dma_buf);
		//LOG_INF("put end\n");
	}
}

signed int dpe_deque_cb(struct frame *frames, void *req, unsigned int reqcnt)
{
	unsigned int f, fcnt, ucnt;
	unsigned int pd_frame_num;
	unsigned int DPE_P4_EN;
	struct DPE_Request *_req;
	struct DPE_Config_ISP8 *pDpeConfig;
	int dvs_cnt, dvp_cnt, dvp_put, dvs_put, dvgf_cnt;
	struct tee_mmu temp_dvs;
	struct tee_mmu temp_dvp;
	struct tee_mmu temp_dvgf;
	unsigned int de_idx;
	_req = (struct DPE_Request *) req;
	if (frames == NULL || _req == NULL)
		return -1;
	if (DPE_debug_log_en == 1)
		LOG_INF("dpe_deque start\n");

	/*TODO: m_ReqNum is FrmNum; FIFO only thus f starts from 0 */
	ucnt = 0;
	fcnt = _req->m_ReqNum;
	for (f = 0; f < fcnt; f++) {
		pDpeConfig = (struct DPE_Config_ISP8 *) frames[f].data;
		if (pDpeConfig->Dpe_DVSSettings.is_pd_mode) {
			pd_frame_num = pDpeConfig->Dpe_DVSSettings.pd_frame_num;
			memcpy(&_req->m_pDpeConfig[ucnt], frames[f].data,
						sizeof(struct DPE_Config_ISP8));
			f += (pd_frame_num-1);
		} else {
			memcpy(&_req->m_pDpeConfig[ucnt], frames[f].data,
						sizeof(struct DPE_Config_ISP8));
		}
		pDpeConfig = &_req->m_pDpeConfig[ucnt];
		ucnt++;
		//memcpy(&_req->m_pDpeConfig[f], frames[f].data,
		//sizeof(struct DPE_Config_ISP8));
		if (DPE_debug_log_en == 1) {
			LOG_INF("[%s]request dequeued frame(%d/%d).", __func__, f,
									fcnt);
		}
#ifdef dpe_dump_read_en
		//LOG_INF(
		//"[%s] request queued with  frame(%d)", __func__, f);
#endif
	}
	dvp_cnt = 0;
	dvs_cnt = 0;
	dvgf_cnt = 0;
	dvp_put = 0;
	dvs_put = 0;
	//spin_lock(&(DPEInfo.SpinLockFD));

	if (DPE_debug_log_en == 1) {
		/*LOG_INF("put fd DVS_only_en =%d, DVP_only_en =%d\n",*/
		/*DVS_only_en, DVP_only_en);*/
		/*LOG_INF("put fd DVS_Num =%d DVP_Num =%d\n", DVS_Num, DVP_Num);*/
		LOG_INF("[dpe_deque] Dpe_engineSelect %d\n",
		pDpeConfig->Dpe_engineSelect);
	}

	if ((pDpeConfig->Dpe_engineSelect == MODE_DVS_ONLY) ||
		(pDpeConfig->Dpe_engineSelect == MODE_DVS_DVP_BOTH)) {
		//LOG_INF("dpe_deque DVS put fd\n");
		mutex_lock(&gFDMutex);
		DPE_P4_EN = (((_req->m_pDpeConfig[0].Dpe_DVSSettings.TuningBuf_ME.DVS_ME_28) &
							0x400) >> 10);
		//LOG_INF("dpe_deque DPE_P4_EN = %d\n", DPE_P4_EN);
		// mutex_unlock(&gFDMutex);

		//mutex_lock(&gFDMutex);
		//LOG_INF("put fd DVS_only_en =%d, get_dvs_iova =%d\n",
		//DVS_only_en, get_dvs_iova[0]);
		//mutex_unlock(&gFDMutex);
		de_idx = reqcnt;
		//spin_unlock(&(DPEInfo.SpinLockFD));
		// mutex_lock(&gFDMutex);
		if (get_dvs_iova[SrcImg_Y_L] >= 1) {
			get_dvs_iova[SrcImg_Y_L]--;
			memcpy(&temp_dvs, &SrcImg_Y_L_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvs, SrcImg_Y_L);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque SrcImg_Y_L_mmu put fd\n");

			dvs_cnt++;
		}

		if (get_dvs_iova[SrcImg_Y_R] >= 1) {
			get_dvs_iova[SrcImg_Y_R]--;
			memcpy(&temp_dvs, &SrcImg_Y_R_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvs, SrcImg_Y_R);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque SrcImg_Y_R_mmu put fd\n");
			dvs_cnt++;
		}

		if (get_dvs_iova[OutBuf_OCC] >= 1) {
			get_dvs_iova[OutBuf_OCC]--;
			memcpy(&temp_dvs, &OutBuf_OCC_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvs, OutBuf_OCC);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque OutBuf_OCC_mmu put fd\n");
			dvs_cnt++;
		}

		if (get_dvs_iova[OutBuf_OCC_EXT] >= 1) {
			get_dvs_iova[OutBuf_OCC_EXT]--;
			memcpy(&temp_dvs, &OutBuf_OCC_Ext_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvs, OutBuf_OCC_EXT);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque OutBuf_OCC_Ext_mmu put fd\n");
			dvs_cnt++;
		}

		if (get_dvs_iova[OutBuf_OCC_HIST] >= 1) {
			get_dvs_iova[OutBuf_OCC_HIST]--;
			memcpy(&temp_dvs, &OutBuf_OCC_Hist_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvs, OutBuf_OCC_HIST);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque OutBuf_OCC_Hist_mmu put fd\n");
			dvs_cnt++;
		}

		if (DPE_P4_EN == 1) {
			if (DPE_debug_log_en == 1)
				LOG_INF("p4 Buf put fd\n");

			if (get_dvs_iova[SrcImg_Y_L_PRE] >= 1) {
				get_dvs_iova[SrcImg_Y_L_PRE]--;
				memcpy(&temp_dvs, &SrcImg_Y_L_Pre_mmu[de_idx], sizeof(struct tee_mmu));
				mmu_release(&temp_dvs, SrcImg_Y_L_PRE);
				if (DPE_debug_log_en == 1)
					LOG_INF("dpe_deque SrcImg_Y_L_Pre put fd\n");
				dvs_cnt++;
			}

			if (get_dvs_iova[SrcImg_Y_R_PRE] >= 1) {
				//mutex_lock(&gFDMutex);
				get_dvs_iova[SrcImg_Y_R_PRE]--;
				memcpy(&temp_dvs, &SrcImg_Y_R_Pre_mmu[de_idx], sizeof(struct tee_mmu));
				mmu_release(&temp_dvs, SrcImg_Y_R_PRE);
				dvs_cnt++;
			}

			if (get_dvs_iova[InBuf_P4_L_DV] >= 1) {
				get_dvs_iova[InBuf_P4_L_DV]--;
				memcpy(&temp_dvs, &InBuf_P4_L_mmu[de_idx], sizeof(struct tee_mmu));
				mmu_release(&temp_dvs, InBuf_P4_L_DV);
				dvs_cnt++;
			}

			if (get_dvs_iova[InBuf_P4_R_DV] >= 1) {
				get_dvs_iova[InBuf_P4_R_DV]--;
				memcpy(&temp_dvs, &InBuf_P4_R_mmu[de_idx], sizeof(struct tee_mmu));
				mmu_release(&temp_dvs, InBuf_P4_R_DV);
				dvs_cnt++;
			}
		}

		DVS_Num--;
		if (DVS_Num == 0) {
			kfree((struct tee_mmu *)SrcImg_Y_L_mmu);
			kfree((struct tee_mmu *)SrcImg_Y_R_mmu);
			kfree((struct tee_mmu *)OutBuf_OCC_mmu);
			kfree((struct tee_mmu *)OutBuf_OCC_Hist_mmu);
			kfree((struct tee_mmu *)OutBuf_OCC_Ext_mmu);
			kfree((struct tee_mmu *)SrcImg_Y_L_Pre_mmu);
			kfree((struct tee_mmu *)SrcImg_Y_R_Pre_mmu);
			kfree((struct tee_mmu *)InBuf_P4_L_mmu);
			kfree((struct tee_mmu *)InBuf_P4_R_mmu);
		}
		mutex_unlock(&gFDMutex);
	}

	if ((pDpeConfig->Dpe_engineSelect == MODE_DVP_ONLY) ||
		(pDpeConfig->Dpe_engineSelect == MODE_DVS_DVP_BOTH)) {
		//mutex_lock(&gFDMutex);
		if (DPE_debug_log_en == 1)
			LOG_INF("dpe_deque DVP put fd\n");

		mutex_lock(&gFDMutex);
		de_idx = reqcnt;
		if (get_dvp_iova[SrcImg_Y] >= 1) {
			get_dvp_iova[SrcImg_Y]--;
			memcpy(&temp_dvp, &SrcImg_Y_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvp, SrcImg_Y);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque SrcImg_Y_mmu put fd\n");

			dvp_cnt++;
		}
//		mutex_unlock(&gFDMutex);
		//mutex_lock(&gFDMutex);
		if (get_dvp_iova[SrcImg_C] >= 1) {
			get_dvp_iova[SrcImg_C]--;
			memcpy(&temp_dvp, &SrcImg_C_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvp, SrcImg_C);
			dvp_cnt++;
		}
//		mutex_unlock(&gFDMutex);
		//mutex_lock(&gFDMutex);
		if (get_dvp_iova[InBuf_OCC] >= 1) {
			get_dvp_iova[InBuf_OCC]--;
			memcpy(&temp_dvp, &InBuf_OCC_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvp, InBuf_OCC);
			dvp_cnt++;
		}
//		mutex_unlock(&gFDMutex);
		//mutex_lock(&gFDMutex);
		if (get_dvp_iova[OutBuf_CRM] >= 1) {
			get_dvp_iova[OutBuf_CRM]--;
			memcpy(&temp_dvp, &OutBuf_CRM_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvp, OutBuf_CRM);
			dvp_cnt++;
		}
//		mutex_unlock(&gFDMutex);
		//mutex_lock(&gFDMutex);
		if (get_dvp_iova[OutBuf_WMF_RD] >= 1) {
			get_dvp_iova[OutBuf_WMF_RD]--;
			memcpy(&temp_dvp, &WMF_RD_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvp, OutBuf_WMF_RD);
			dvp_cnt++;
		}
//		mutex_unlock(&gFDMutex);
		//mutex_lock(&gFDMutex);
		if (get_dvp_iova[OutBuf_ASF_HF] >= 1) {
			get_dvp_iova[OutBuf_ASF_HF]--;
			memcpy(&temp_dvp, &ASF_HF_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvp, OutBuf_ASF_HF);
			dvp_cnt++;
		}
//		mutex_unlock(&gFDMutex);
		//mutex_lock(&gFDMutex);
		if (get_dvp_iova[OutBuf_WMF_FILT] >= 1) {
			get_dvp_iova[OutBuf_WMF_FILT]--;
			memcpy(&temp_dvp, &WMF_FILT_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvp, OutBuf_WMF_FILT);
			dvp_cnt++;
		}
//		mutex_unlock(&gFDMutex);
		//mutex_lock(&gFDMutex);
		if (get_dvp_iova[InBuf_OCC_Ext] >= 1) {
			get_dvp_iova[InBuf_OCC_Ext]--;
			memcpy(&temp_dvp, &InBuf_OCC_Ext_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvp, InBuf_OCC_Ext);
			dvp_cnt++;
		}

		if (get_dvp_iova[OutBuf_ASF_RD_Ext] >= 1) {
			get_dvp_iova[OutBuf_ASF_RD_Ext]--;
			memcpy(&temp_dvp, &ASF_RD_Ext_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvp, OutBuf_ASF_RD_Ext);
			dvp_cnt++;
		}

		if (get_dvp_iova[OutBuf_ASF_HF_Ext] >= 1) {
			get_dvp_iova[OutBuf_ASF_HF_Ext]--;
			memcpy(&temp_dvp, &ASF_HF_Ext_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvp, OutBuf_ASF_HF_Ext);
			dvp_cnt++;
		}

//		mutex_unlock(&gFDMutex);
		//mutex_lock(&gFDMutex);
		DVP_Num--;
		if (DVP_Num == 0) {
			kfree((struct tee_mmu *)SrcImg_Y_mmu);
			kfree((struct tee_mmu *)SrcImg_C_mmu);
			kfree((struct tee_mmu *)InBuf_OCC_mmu);
			kfree((struct tee_mmu *)OutBuf_CRM_mmu);
			kfree((struct tee_mmu *)ASF_RD_mmu);
			kfree((struct tee_mmu *)ASF_HF_mmu);
			kfree((struct tee_mmu *)WMF_RD_mmu);
			kfree((struct tee_mmu *)WMF_FILT_mmu);
			kfree((struct tee_mmu *)InBuf_OCC_Ext_mmu);
			kfree((struct tee_mmu *)ASF_RD_Ext_mmu);
			kfree((struct tee_mmu *)ASF_HF_Ext_mmu);
		}
		mutex_unlock(&gFDMutex);
	}

	if (pDpeConfig->Dpe_engineSelect == MODE_DVGF_ONLY) {
		//mutex_lock(&gFDMutex);
		if (DPE_debug_log_en == 1)
			LOG_INF("dpe_deque DVGF put fd\n");

		mutex_lock(&gFDMutex);
		de_idx = reqcnt;
		if (get_dvgf_iova[DVGF_SrcImg_Y] >= 1) {
			get_dvgf_iova[DVGF_SrcImg_Y]--;
			memcpy(&temp_dvgf, &DVGF_SrcImg_Y_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvgf, DVGF_SrcImg_Y);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque SrcImg_Y_mmu put fd\n");

			dvgf_cnt++;
		}

		if (get_dvgf_iova[DVGF_SrcImg_C] >= 1) {
			get_dvgf_iova[DVGF_SrcImg_C]--;
			memcpy(&temp_dvgf, &DVGF_SrcImg_C_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvgf, DVGF_SrcImg_C);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque SrcImg_C_mmu put fd\n");

			dvgf_cnt++;
		}


		if (get_dvgf_iova[SrcImg_Y_Pre] >= 1) {
			get_dvgf_iova[SrcImg_Y_Pre]--;
			memcpy(&temp_dvgf, &SrcImg_Y_Pre_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvgf, SrcImg_Y_Pre);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque SrcImg_Y_Pre_mmu put fd\n");

			dvgf_cnt++;
		}

		if (get_dvgf_iova[SrcImg_C_Pre] >= 1) {
			get_dvgf_iova[SrcImg_C_Pre]--;
			memcpy(&temp_dvgf, &SrcImg_C_Pre_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvgf, SrcImg_C_Pre);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque SrcImg_C_Pre_mmu put fd\n");

			dvgf_cnt++;
		}

		if (get_dvgf_iova[DVGF_OutBuf_OCC_Ext] >= 1) {
			get_dvgf_iova[DVGF_OutBuf_OCC_Ext]--;
			memcpy(&temp_dvgf, &DVGF_OutBuf_OCC_Ext_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvgf, DVGF_OutBuf_OCC_Ext);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque DVGF_OutBuf_OCC_Ext_mmu put fd\n");

			dvgf_cnt++;
		}



		if (get_dvgf_iova[OutBuf_WT_Fnl] >= 1) {
			get_dvgf_iova[OutBuf_WT_Fnl]--;
			memcpy(&temp_dvgf, &WT_Fnl_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvgf, OutBuf_WT_Fnl);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque WT_Fnl_mmu put fd\n");

			dvgf_cnt++;
		}

		if (get_dvgf_iova[OutBuf_RW_IIR] >= 1) {
			get_dvgf_iova[OutBuf_RW_IIR]--;
			memcpy(&temp_dvgf, &RW_IIR_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvgf, OutBuf_RW_IIR);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque RW_IIR_mmu put fd\n");

			dvgf_cnt++;
		}

		if (get_dvgf_iova[DVGF_OutBuf_WMF_FILT] >= 1) {
			get_dvgf_iova[DVGF_OutBuf_WMF_FILT]--;
			memcpy(&temp_dvgf, &DVGF_WMF_FILT_mmu[de_idx], sizeof(struct tee_mmu));
			mmu_release(&temp_dvgf, DVGF_OutBuf_WMF_FILT);
			if (DPE_debug_log_en == 1)
				LOG_INF("dpe_deque DVGF_WMF_FILT_mmu put fd\n");

			dvgf_cnt++;
		}

		DVGF_Num--;
		if (DVGF_Num == 0) {
			kfree((struct tee_mmu *)SrcImg_Y_Pre_mmu);
			kfree((struct tee_mmu *)SrcImg_C_Pre_mmu);
			kfree((struct tee_mmu *)WT_Fnl_mmu);
			kfree((struct tee_mmu *)RW_IIR_mmu);
			kfree((struct tee_mmu *)DVGF_SrcImg_Y_mmu);
			kfree((struct tee_mmu *)DVGF_SrcImg_C_mmu);
			kfree((struct tee_mmu *)DVGF_OutBuf_OCC_Ext_mmu);
			kfree((struct tee_mmu *)DVGF_WMF_FILT_mmu);
		}
		mutex_unlock(&gFDMutex);
	}
	LOG_INF("put end put_dvs = %d put_dvp = %d put_dvgf = %d\n",
	dvs_cnt, dvp_cnt, dvgf_cnt);

	//!mutex_unlock(&gDpeMutex);
	_req->m_ReqNum = ucnt;
	return 0;
}

int DPE_Config_DVS(struct DPE_Config_ISP8 *pDpeConfig,
	struct DPE_Kernel_Config *pConfigToKernel)
{
	unsigned int frmWidth = pDpeConfig->Dpe_DVSSettings.frm_width;
	unsigned int frmHeight = pDpeConfig->Dpe_DVSSettings.frm_height;
	unsigned int L_engStartX = pDpeConfig->Dpe_DVSSettings.l_eng_start_x;
	unsigned int R_engStartX = pDpeConfig->Dpe_DVSSettings.r_eng_start_x;
	unsigned int engStartY = pDpeConfig->Dpe_DVSSettings.eng_start_y;
	unsigned int engWidth = pDpeConfig->Dpe_DVSSettings.eng_width;
	unsigned int engHeight = pDpeConfig->Dpe_DVSSettings.eng_height;
	unsigned int occWidth = pDpeConfig->Dpe_DVSSettings.occ_width;
	unsigned int occStartX = pDpeConfig->Dpe_DVSSettings.occ_start_x;
	unsigned int DVS_OUT_ADJ_En = pDpeConfig->Dpe_DVSSettings.out_adj_en;
	unsigned int DVS_OUT_ADJ_Dv_En = pDpeConfig->Dpe_DVSSettings.out_adj_dv_en;
	unsigned int DVS_OUT_ADJ_WIDTH = pDpeConfig->Dpe_DVSSettings.out_adj_width;
	unsigned int DVS_OUT_ADJ_HIGHT = pDpeConfig->Dpe_DVSSettings.out_adj_height;
	//ext1
	unsigned int ext1_L_engStartX = pDpeConfig->Dpe_DVSSettings.ext1_eng_start_x_L;
	unsigned int ext1_R_engStartX = pDpeConfig->Dpe_DVSSettings.ext1_eng_start_x_R;
	//unsigned int ext1_engStartY = pDpeConfig->Dpe_DVSSettings.ext1_eng_start_y;
	unsigned int ext1_occStartX = pDpeConfig->Dpe_DVSSettings.ext1_occ_start_x;
	unsigned int ext1_occWidth = pDpeConfig->Dpe_DVSSettings.ext1_occ_width;
	unsigned int ext1_adjWidth = pDpeConfig->Dpe_DVSSettings.ext1_out_adj_width;
	unsigned int ext1_frmWidth = pDpeConfig->Dpe_DVSSettings.ext1_frm_width;
	unsigned int ext1_engWidth = pDpeConfig->Dpe_DVSSettings.ext1_eng_width;
	//ext2
	unsigned int ext2_L_engStartX = pDpeConfig->Dpe_DVSSettings.ext2_eng_start_x_L;
	unsigned int ext2_R_engStartX = pDpeConfig->Dpe_DVSSettings.ext2_eng_start_x_R;
	//unsigned int ext2_engStartY = pDpeConfig->Dpe_DVSSettings.ext2_eng_start_y;
	unsigned int ext2_occStartX = pDpeConfig->Dpe_DVSSettings.ext2_occ_start_x;
	unsigned int ext2_occWidth = pDpeConfig->Dpe_DVSSettings.ext2_occ_width;
	unsigned int ext2_adjWidth = pDpeConfig->Dpe_DVSSettings.ext2_out_adj_width;
	unsigned int ext2_frmWidth = pDpeConfig->Dpe_DVSSettings.ext2_frm_width;
	unsigned int ext2_engWidth = pDpeConfig->Dpe_DVSSettings.ext2_eng_width;

	unsigned int frm_mode_en = pDpeConfig->Dpe_DVSSettings.ext_frm_mode_en;
	unsigned int frm_num = pDpeConfig->Dpe_DVSSettings.ext_frm_num;

	// pitch / 16
	unsigned int pitch = pDpeConfig->Dpe_DVSSettings.dram_pxl_pitch>>4;
	unsigned int full_tile_width = pDpeConfig->Dpe_DVSSettings.dram_out_pitch >> 4;
	unsigned int tile_pitch = ((ext1_engWidth >> 1) + (ext2_engWidth >> 1) + 15) >> 4;
	unsigned int DPE_P4_EN;

	mutex_lock(&gFDMutex);
	DPE_P4_EN = ((pDpeConfig->Dpe_DVSSettings.TuningBuf_ME.DVS_ME_28 & 0x400)>>10);
	mutex_unlock(&gFDMutex);

	LOG_INF(
	"DVS param: frm w/h(%d/%d), engStart X_L/X_R/Y(%d/%d/%d), eng w/h(%d/%d), occ w(%d), occ startX(%d), pitch(%d), main eye(%d), 16bit mode(%d), sbf/occ_en(%d/%d), Dpe_InBuf_SrcImg_Y_L: (%llu), Dpe_InBuf_SrcImg_Y_R(%llu)), Dpe_OutBuf_OCC(%llu)\n",
	frmWidth, frmHeight, L_engStartX, R_engStartX, engStartY,
	engWidth, engHeight, occWidth, occStartX, pitch,
	pDpeConfig->Dpe_DVSSettings.mainEyeSel, pDpeConfig->Dpe_is16BitMode,
	pDpeConfig->Dpe_DVSSettings.SubModule_EN.sbf_en,
	pDpeConfig->Dpe_DVSSettings.SubModule_EN.occ_en,
	pDpeConfig->Dpe_InBuf_SrcImg_Y_L, pDpeConfig->Dpe_InBuf_SrcImg_Y_R,
	pDpeConfig->Dpe_OutBuf_OCC);

	if (DPE_debug_log_en == 1) {
		LOG_INF(
		"Dpe_InBuf_SrcImg_Y_L: (%llu), Dpe_InBuf_SrcImg_Y_R(%llu), Dpe_OutBuf_OCC(%llu), P4 = %d,dram_out_pitch = %d, tile_pitch = %d\n",
		pDpeConfig->Dpe_InBuf_SrcImg_Y_L,
		pDpeConfig->Dpe_InBuf_SrcImg_Y_R,
		pDpeConfig->Dpe_OutBuf_OCC,
		DPE_P4_EN,
		pDpeConfig->Dpe_DVSSettings.dram_out_pitch,
		tile_pitch);
	}
	if ((frmWidth % 16 != 0)) {
		LOG_ERR("frame width is not 16 byte align w(%d)\n", frmWidth);
		return -1;
	}
	if ((frmHeight % 2 != 0)) {
		LOG_ERR("frame height is not 2 byte align h(%d)\n", frmHeight);
		return -1;
	}
	if ((occWidth % 16 != 0)) {
		LOG_ERR("occ width is not 16 byte align w(%d)\n", occWidth);
		return -1;
	}
	if (L_engStartX < R_engStartX) {
		LOG_ERR("L_engStartX(%d) < R_engStartX(%d)\n",
		L_engStartX, R_engStartX);
		return -1;
	}

	if (pDpeConfig->Dpe_engineSelect == MODE_DVS_DVP_BOTH) {
		pConfigToKernel->DVS_CTRL00 =
		((0xD & 0x1F) << 6) |
		((0x0 << 0x1) << 11) |
		((pDpeConfig->Dpe_DVSSettings.SubModule_EN.sbf_en & 0x1) << 12) |
		((pDpeConfig->Dpe_DVSSettings.SubModule_EN.occ_en & 0x1) << 13) |
		((0x0 & 0x3) << 16) | // c_dvgf_cur_frm
		((0x1 & 0x3) << 18) | // c_dvgf_prev_frm
		((0x0 & 0x3) << 20) | // c_dvp_cur_frm
		((0x0 & 0x3) << 22) | // c_dvs_cur_frm
		((0x1 & 0x3) << 24) | // c_dvs_prev_frm //!ISP7 Temporal Stable ?V P4 on WTA
		((0x0 & 0x1) << 26) | // c_dvgf_trig_en
		((0x3 & 0x3) << 27) | // c_dvp_trig_en, c_dvs_trig_en
		((0x2 & 0x3) << 29)	| // c_dpe_fw_trig, c_dpe_fw_trig_en
		((0x1 & 0x1) << 31); // c_dvs_en
	} else if (pDpeConfig->Dpe_engineSelect == MODE_DVS_ONLY) {
		pConfigToKernel->DVS_CTRL00 =
		((0xD & 0x1F) << 6) |
		((0x0 << 0x1) << 11) |
		((pDpeConfig->Dpe_DVSSettings.SubModule_EN.sbf_en & 0x1) << 12) |
		((pDpeConfig->Dpe_DVSSettings.SubModule_EN.occ_en & 0x1) << 13) |
		((0x0 & 0x3) << 16) | // c_dvgf_cur_frm
		((0x1 & 0x3) << 18) | // c_dvgf_prev_frm
		((0x0 & 0x3) << 20) | // c_dvp_cur_frm
		((0x0 & 0x3) << 22) | // c_dvs_cur_frm
		((0x1 & 0x3) << 24) | // c_dvs_prev_frm //!ISP7 Temporal Stable ?V P4 on WTA
		((0x0 & 0x1) << 26) | // c_dvgf_trig_en
		((0x2 & 0x3) << 27) | // c_dvp_trig_en, c_dvs_trig_en
		((0x0 & 0x1) << 29) | // c_dpe_fw_trig_en
		((0x1 & 0x1) << 30) | // c_dpe_fw_trig
		((0x1 & 0x1) << 31); // c_dvs_en
	}
	// pConfigToKernel->DVS_CTRL01 = ((0x1 & 0x1) << 8);
	pConfigToKernel->DVS_CTRL03 = 0x00100552;
	//!ISP7 Tile mode
	//if (pDpeConfig->Dpe_DVSSettings.is_pd_mode) {
	//	pConfigToKernel->DVS_DRAM_PITCH =
	//	((pitch) & 0x3FF) | ((pitch & 0x3FF) << 10) |
	//	((pDpeConfig->Dpe_DVSSettings.dram_out_pitch_en & 0x01) << 31) |
	//	((full_tile_width & 0x3FF) << 20); //!ISP7
	//	pConfigToKernel->DVS_SRC_00 =
	//	((pDpeConfig->Dpe_is16BitMode & 0x1) << 0) | //!c_dv16b_mode = 1 ;ISP7 only
	//	((pDpeConfig->Dpe_DVSSettings.mainEyeSel & 0x1) << 1) |
	//	((0x0 & 0x1) << 8) | // c_vmap_in_pxl
	//	((0x0 & 0x1) << 9); // c_vmap_all_vld
	//} else {
	//	pConfigToKernel->DVS_DRAM_PITCH =
	//	((pitch) & 0x3FF) | ((pitch & 0x3FF) << 10) |
	//	((pDpeConfig->Dpe_DVSSettings.dram_out_pitch_en & 0x01) << 31) |
	//	((full_tile_width & 0x3FF) << 20); //!ISP7

	//	pConfigToKernel->DVS_SRC_00 =
	//	((pDpeConfig->Dpe_is16BitMode & 0x1) << 0) | //!c_dv16b_mode = 1 ;ISP7 only
	//	((pDpeConfig->Dpe_DVSSettings.mainEyeSel & 0x1) << 1) |
	//	((0x0 & 0x1) << 8) | // c_vmap_in_pxl
	//	((0x0 & 0x1) << 9); // c_vmap_all_vld
	//}
	if (DPE_debug_log_en == 1) {
		LOG_INF("dram_out_pitch_en=%d , pitch=%d ,full_tile_width=%d\n",
		pDpeConfig->Dpe_DVSSettings.dram_out_pitch_en, pitch, full_tile_width);
	}

	pConfigToKernel->DVS_DRAM_PITCH =
	((pitch) & 0x3FF) | ((pitch & 0x3FF) << 10) |
	((pDpeConfig->Dpe_DVSSettings.dram_out_pitch_en & 0x01) << 31) |
	((full_tile_width & 0x3FF) << 20); //!ISP7


	if (pDpeConfig->Dpe_DVSSettings.ext_frm_mode_en == 0) {
		tile_pitch = (((engWidth >> 1) + 15) >> 4);
		if (DPE_debug_log_en == 1)
			LOG_INF("tile_pitch =%d\n", tile_pitch);
	}

	if (pDpeConfig->Dpe_DVSSettings.out_adj_width !=
		pDpeConfig->Dpe_DVSSettings.occ_width) {
		pConfigToKernel->DVS_DRAM_PITCH1 = 0;
	} else {
		pConfigToKernel->DVS_DRAM_PITCH1 =
		((tile_pitch) & 0x3FF) |
		((pDpeConfig->Dpe_DVSSettings.dram_out_pitch_en & 0x01) << 31) ;//0x80000014;
	}


	pConfigToKernel->DVS_SRC_00 = //0x00000502;
	((pDpeConfig->Dpe_is16BitMode & 0x1) << 0) | //!c_dv16b_mode = 1 ;ISP7 only
	((pDpeConfig->Dpe_DVSSettings.mainEyeSel & 0x1) << 1) |
	((0x1 & 0x1) << 8) |
	((pDpeConfig->Dpe_DVSSettings.SubModule_EN.occ_Hist_en & 0x1) << 10); // c_occ_histen


	pConfigToKernel->DVS_SRC_01 =
	((frmHeight & 0x7FF) << 0) | ((frmWidth & 0x7FF) << 12);

	pConfigToKernel->DVS_SRC_02 =
	((engHeight & 0x7FF) << 0) | ((engWidth & 0x7FF) << 12);

	pConfigToKernel->DVS_SRC_03 =
	((R_engStartX & 0x7FF) << 0) |
	((L_engStartX & 0x7FF) << 12) |
	((engStartY & 0xFF) << 24);

	pConfigToKernel->DVS_SRC_04 =
	((occWidth & 0x7FF) << 0) | ((occStartX & 0x7FF) << 12);

	if (pDpeConfig->Dpe_InBuf_SrcImg_Y_L != 0x0) {
		pConfigToKernel->DVS_SRC_05_L_FRM0 =
		pDpeConfig->Dpe_InBuf_SrcImg_Y_L;
	} else {
		LOG_ERR("No Left Src Image Y!\n");
		return -1;
	}
	//LOG_INF("DVS_SRC_05_L_FRM0 =(%llu)",
	//pConfigToKernel->DVS_SRC_05_L_FRM0);

	if (pDpeConfig->Dpe_InBuf_SrcImg_Y_R != 0x0) {
		pConfigToKernel->DVS_SRC_09_R_FRM0 =
		pDpeConfig->Dpe_InBuf_SrcImg_Y_R;
	} else {
		LOG_ERR("No Right Src Image Y!\n");
		return -1;
	}
	//LOG_INF("DVS_SRC_09_R_FRM0 =(%llu)",
	//pConfigToKernel->DVS_SRC_09_R_FRM0);

	mutex_lock(&gFDMutex);
	if (DPE_P4_EN == 1) {
		//LOG_INF("dpe_p4_enable == 1\n");
		if (pDpeConfig->Dpe_InBuf_SrcImg_Y_L_Pre != 0x0) {
			pConfigToKernel->DVS_SRC_06_L_FRM1 =
			pDpeConfig->Dpe_InBuf_SrcImg_Y_L_Pre;
		} else {
			LOG_ERR("No Pre Left Src Image Y!\n");
			return -1;
		}

		if (pDpeConfig->Dpe_InBuf_SrcImg_Y_R_Pre != 0x0) {
			pConfigToKernel->DVS_SRC_10_R_FRM1 =
			pDpeConfig->Dpe_InBuf_SrcImg_Y_R_Pre;
		} else {
			LOG_ERR("No Right Src Image Y!\n");
			return -1;
		}

		if (pDpeConfig->Dpe_InBuf_P4_L_DV != 0x0) {
			pConfigToKernel->DVS_SRC_21_P4_L_DV_ADR =
			pDpeConfig->Dpe_InBuf_P4_L_DV;
		} else {
			LOG_ERR("No DVS DVS_SRC_21_P4_L_DV Buffer!\n");
			return -1;
		}

		if (pDpeConfig->Dpe_InBuf_P4_R_DV != 0x0) {
			pConfigToKernel->DVS_SRC_26_P4_R_DV_ADR =
			pDpeConfig->Dpe_InBuf_P4_R_DV;
		} else {
			LOG_ERR("No DVS DVS_SRC_34_P4_R_DV Buffer!\n");
			return -1;
		}
	}
	mutex_unlock(&gFDMutex);

	//!ISP7 DVS NN Down-Sample
	if (DPE_debug_log_en == 1) {
		LOG_INF("ADJ_Dv_HIGHT =(%d), ADJ_Dv_WIDTH=(%d), ADJ_Dv_En=(%d)\n",
		DVS_OUT_ADJ_HIGHT, DVS_OUT_ADJ_WIDTH, DVS_OUT_ADJ_Dv_En);
	}
	pConfigToKernel->DVS_SRC_28 =
	((DVS_OUT_ADJ_En & 0x1) << 24) |
	((DVS_OUT_ADJ_Dv_En & 0x1) << 23) |
	((DVS_OUT_ADJ_WIDTH & 0x7FF) << 12) |
	((DVS_OUT_ADJ_HIGHT & 0x7FF) << 0);
	//LOG_INF("DVS_SRC_28 =(0x%llu)", pConfigToKernel->DVS_SRC_28);

	pConfigToKernel->DVS_CTRL_ATPG = 0x80000000;
	if (pDpeConfig->Dpe_OutBuf_OCC != 0x0) {
		pConfigToKernel->DVS_SRC_22_OCCDV0 =
		pDpeConfig->Dpe_OutBuf_OCC;
	} else {
		LOG_ERR("No DVS OCC Output Buffer!\n");
		return -1;
	}
	//LOG_INF("DVS_SRC_22_OCCDV0 =(%llu)\n",
	//pConfigToKernel->DVS_SRC_22_OCCDV0);

	if (pDpeConfig->Dpe_OutBuf_OCC_Ext != 0x0) {
		pConfigToKernel->DVS_SRC_17_OCCDV_EXT0 =
		pDpeConfig->Dpe_OutBuf_OCC_Ext;
	} else
		LOG_ERR("No DVS Ext Output Buffer!\n");

	//LOG_INF("DVS_SRC_17_OCCDV_EXT0 =(%llu)",
	//pConfigToKernel->DVS_SRC_17_OCCDV_EXT0);

	if (pDpeConfig->Dpe_OutBuf_OCC_Hist != 0x0) {
		pConfigToKernel->DVS_SRC_13_Hist0 =
		pDpeConfig->Dpe_OutBuf_OCC_Hist;
	} else
		LOG_ERR("No OCC HIST Output Buffer!\n");

	//Tile mode
	pConfigToKernel->DVS_SRC_27 =
	((frm_num & 0xf) << 27) |
	((frm_mode_en & 0x1) << 31);

	pConfigToKernel->DVS_SRC_29 =
	((ext2_R_engStartX & 0x07FF) << 0) |
	((ext2_L_engStartX & 0x07FF) << 12);

	pConfigToKernel->DVS_SRC_30 =
	((ext2_engWidth & 0x07FF) << 0) |
	((ext2_frmWidth & 0x07FF) << 12);

	pConfigToKernel->DVS_SRC_31 =
	((ext2_adjWidth & 0x07FF) << 0) |
	((ext2_occWidth & 0x07FF) << 12);

	pConfigToKernel->DVS_SRC_32 =
	((ext1_occStartX & 0x07FF) << 0) |
	((ext2_occStartX & 0x07FF) << 12);

	pConfigToKernel->DVS_SRC_33 =
	((ext1_R_engStartX & 0x07FF) << 0) |
	((ext1_L_engStartX & 0x07FF) << 12);

	pConfigToKernel->DVS_SRC_34 =
	((ext1_engWidth & 0x07FF) << 0) |
	((ext1_frmWidth & 0x07FF) << 12);

	pConfigToKernel->DVS_SRC_35 =
	((ext1_adjWidth & 0x07FF) << 0) |
	((ext1_occWidth & 0x07FF) << 12);


	pConfigToKernel->DVS_SRC_36 = pDpeConfig->Dpe_DVSSettings.Tuning_meta_dvs.DVS_SRC_36;
	pConfigToKernel->DVS_SRC_37 = pDpeConfig->Dpe_DVSSettings.Tuning_meta_dvs.DVS_SRC_37;
	pConfigToKernel->DVS_SRC_38 = pDpeConfig->Dpe_DVSSettings.Tuning_meta_dvs.DVS_SRC_38;
	pConfigToKernel->DVS_SRC_39 = pDpeConfig->Dpe_DVSSettings.Tuning_meta_dvs.DVS_SRC_39;
	pConfigToKernel->DVS_CTRL_ATPG = 0x80000000;
	//
	memcpy(&pConfigToKernel->TuningBuf_ME,
	&pDpeConfig->Dpe_DVSSettings.TuningBuf_ME,
	sizeof(pDpeConfig->Dpe_DVSSettings.TuningBuf_ME));


	memcpy(&pConfigToKernel->TuningKernel_OCC,
	&pDpeConfig->Dpe_DVSSettings.TuningBuf_OCC,
	sizeof(pDpeConfig->Dpe_DVSSettings.TuningBuf_OCC));
	return 0;
}

int DPE_Config_DVP(struct DPE_Config_ISP8 *pDpeConfig,
	struct DPE_Kernel_Config *pConfigToKernel)
{
	unsigned int frmWidth, frmHeight, engWidth, engHeight;
	unsigned int occWidth, occHeight;
	unsigned int engStartX, engStartY, occStartX, occStartY, pitch;
	unsigned int engStart_offset_Y, engStart_offset_C;
	unsigned int DVP_ASF_DV16b_EN = pDpeConfig->Dpe_is16BitMode;
	unsigned int DVP_ASF_DEPTH_MODE = pDpeConfig->Dpe_DVPSettings.SubModule_EN.asf_depth_mode;
	unsigned int DVP_WMF_RD_EN = pDpeConfig->Dpe_DVPSettings.SubModule_EN.wmf_rd_en;
	unsigned int DVP_WMF_HF_EN = pDpeConfig->Dpe_DVPSettings.SubModule_EN.wmf_hf_en;
	unsigned int DVP_WMF_F_EN = pDpeConfig->Dpe_DVPSettings.SubModule_EN.wmf_filt_en;

	engStartX = pDpeConfig->Dpe_DVPSettings.eng_start_x;
	engStartY = pDpeConfig->Dpe_DVPSettings.eng_start_y;
	frmWidth = pDpeConfig->Dpe_DVPSettings.frm_width;
	frmHeight = pDpeConfig->Dpe_DVPSettings.frm_height;
	engWidth = pDpeConfig->Dpe_DVPSettings.frm_width - engStartX;
	engHeight = frmHeight;
	occStartX = engStartX;
	occStartY = 0;
	occWidth = engWidth;
	occHeight = engHeight;

	//DPE_is16BitMode = pDpeConfig->Dpe_is16BitMode;
	// pitch = frame width / 16
	pitch = ALIGN16(pDpeConfig->Dpe_DVPSettings.frm_width) >> 4;
	engStart_offset_Y = engStartY * (pitch << 4);
	engStart_offset_C = engStart_offset_Y >> 1;
	LOG_INF(
	"DVP param: frm w/h(%d/%d), engstart X/Y(%d/%d), eng w/h(%d/%d), occ w/h(%d/%d), occstart X/Y(%d/%d), pitch(%d), eng start offset Y/C(0x%x/0x%x), Dpe_InBuf_SrcImg_Y: (%llu), Dpe_InBuf_SrcImg_C(%llu), Dpe_InBuf_OCC(%llu), Dpe_OutBuf_CRM(%llu), Dpe_OutBuf_WMF_RD(%llu), Dpe_OutBuf_ASF_HF(%llu), DVP_SRC_18_ASF_RMDV(%lu)\n",
	frmWidth, frmHeight, engStartX, engStartY, engWidth, engHeight,
	occWidth, occHeight, occStartX, occStartY, pitch,
	engStart_offset_Y, engStart_offset_C,
	pDpeConfig->Dpe_InBuf_SrcImg_Y, pDpeConfig->Dpe_InBuf_SrcImg_C,
	pDpeConfig->Dpe_InBuf_OCC, pDpeConfig->Dpe_OutBuf_CRM,
	pDpeConfig->Dpe_OutBuf_WMF_RD, pDpeConfig->Dpe_OutBuf_ASF_HF,
	((uintptr_t)g_dpewb_asfrm_Buffer_pa & 0xfffffffff));

	if ((occWidth % 16 != 0)) {
		LOG_ERR("occ width is not 16 byte align w (%d)\n", occWidth);
		return -1;
	}

	if (DVP_ASF_DV16b_EN &&
		pDpeConfig->Dpe_DVPSettings.SubModule_EN.wmf_hf_en) {
		LOG_ERR("WMF should not enable in 16 bit mode\n");
		return -1;
	}

	// If hf rounds is odd, nb_rounds can't use.
	if (pDpeConfig->Dpe_DVPSettings.SubModule_EN.asf_hf_rounds % 2)
		pDpeConfig->Dpe_DVPSettings.SubModule_EN.asf_nb_rounds = 0;

	// pConfigToKernel->DVP_CTRL01 = ((0x1 & 0x1) << 20);

	pConfigToKernel->DVP_CTRL04 =
	(pDpeConfig->Dpe_DVPSettings.SubModule_EN.asf_crm_en << 0) |
	(pDpeConfig->Dpe_DVPSettings.SubModule_EN.asf_rm_en << 1) |
	(pDpeConfig->Dpe_DVPSettings.SubModule_EN.asf_rd_en << 2) |
	(pDpeConfig->Dpe_DVPSettings.SubModule_EN.asf_hf_en << 3) |
	(pDpeConfig->Dpe_DVPSettings.SubModule_EN.wmf_rd_en << 4) |
	(pDpeConfig->Dpe_DVPSettings.SubModule_EN.wmf_hf_en << 5) |
	(pDpeConfig->Dpe_DVPSettings.SubModule_EN.wmf_filt_en << 6) |
	((pDpeConfig->Dpe_DVPSettings.SubModule_EN.asf_hf_rounds & (0xF)) << 8) |
	((pDpeConfig->Dpe_DVPSettings.SubModule_EN.asf_nb_rounds & (0x3)) << 12) |
	((pDpeConfig->Dpe_DVPSettings.SubModule_EN.wmf_filt_rounds & (0x3)) << 16) |
	(DVP_ASF_DV16b_EN << 18) | //!ISP7.0
	(pDpeConfig->Dpe_DVPSettings.SubModule_EN.asf_recursive_en << 19) |
	(DVP_ASF_DEPTH_MODE << 22);

	pConfigToKernel->DVP_DRAM_PITCH =
	((pitch & 0x7F) << 4) | ((pitch & 0x7F) << 20);

	pConfigToKernel->DVP_SRC_00 =
	((engHeight & 0x7FF) << 0) | ((engWidth & 0x7FF) << 16) |
	((pDpeConfig->Dpe_DVPSettings.Y_only & 0x1) << 29) |
	((pDpeConfig->Dpe_DVPSettings.mainEyeSel & 0x1) << 30);

	pConfigToKernel->DVP_SRC_01 =
	((frmHeight & 0x7FF) << 0) | ((frmWidth & 0x7FF) << 16);

	pConfigToKernel->DVP_SRC_02 =
	((engStartX & 0x7FF) << 0) |
	((engStartY & 0x7FF) << 16); //for Simulation

	pConfigToKernel->DVP_SRC_03 =
	((occHeight & 0x7FF) << 0) | ((occWidth & 0x7FF) << 16);

	pConfigToKernel->DVP_SRC_04 =
	((occStartX & 0x7FF) << 0) |
	((occStartY & 0x7FF) << 16); //for Simulation

	if (pDpeConfig->Dpe_DVPSettings.mainEyeSel == RIGHT) {
		if (pDpeConfig->Dpe_InBuf_SrcImg_Y != 0x0) {
			pConfigToKernel->DVP_SRC_05_Y_FRM0 =
			pDpeConfig->Dpe_InBuf_SrcImg_Y +
			(engStart_offset_Y);
		} else {
			LOG_ERR("No DVP Right Src Image Y!\n");
			return -1;
		}
	} else if (pDpeConfig->Dpe_DVPSettings.mainEyeSel == LEFT) {
		if (pDpeConfig->Dpe_InBuf_SrcImg_Y != 0x0) {
			pConfigToKernel->DVP_SRC_05_Y_FRM0 =
			pDpeConfig->Dpe_InBuf_SrcImg_Y +
			(engStart_offset_Y);
		} else {
			LOG_ERR("No DVP Left Src Image Y!\n");
			return -1;
		}
	}

	if (DPE_debug_log_en == 1) {
		LOG_INF("DVP_SRC_05_Y_FRM0 = %llu\n",
		pConfigToKernel->DVP_SRC_05_Y_FRM0);
	}

	if (pDpeConfig->Dpe_InBuf_SrcImg_C != 0x0) {
		pConfigToKernel->DVP_SRC_09_C_FRM0 =
		pDpeConfig->Dpe_InBuf_SrcImg_C +
		(engStart_offset_C);
	} else {
		LOG_ERR("No Src Image C!\n");
		return -1;
	}

	if (DPE_debug_log_en == 1) {
		LOG_INF("DVP_SRC_09_C_FRM0 = %llu\n",
		pConfigToKernel->DVP_SRC_09_C_FRM0);
	}

	if (pDpeConfig->Dpe_InBuf_OCC != 0x0) {
		pConfigToKernel->DVP_SRC_13_OCCDV0 =
		pDpeConfig->Dpe_InBuf_OCC;
	} else {
		LOG_ERR("No DVP OCC In!\n");
		return -1;
	}

	if (DPE_debug_log_en == 1) {
		LOG_INF("DVP_SRC_13_OCCDV0 = %llu\n",
		pConfigToKernel->DVP_SRC_13_OCCDV0);
	}

	if (pDpeConfig->Dpe_OutBuf_CRM != 0x0) {
		pConfigToKernel->DVP_SRC_17_CRM =
		pDpeConfig->Dpe_OutBuf_CRM;
	} else
		LOG_INF("No CRM Output Buffer!\n");

	if (DPE_debug_log_en == 1) {
		LOG_INF("DVP_SRC_17_CRM = %llu\n",
		pConfigToKernel->DVP_SRC_17_CRM);
	}

	#ifdef KERNEL_DMA_BUFFER
	//LOG_INF("get kernel asf buffer 1\n");
	pConfigToKernel->DVP_SRC_18_ASF_RMDV =
	((uintptr_t)g_dpewb_asfrm_Buffer_pa & 0xfffffffff);
	#else
	if (pDpeConfig->Dpe_OutBuf_ASF_RM != 0x0) {
		pConfigToKernel->DVP_SRC_18_ASF_RMDV =
		pDpeConfig->Dpe_OutBuf_ASF_RM;
	} else
		LOG_INF("No DVS DVP_SRC_18_ASF_RMDV Buffer!\n");
	#endif

	if (DPE_debug_log_en == 1) {
		LOG_INF("DVP_SRC_18_ASF_RMDV = %llu\n",
		pConfigToKernel->DVP_SRC_18_ASF_RMDV);
	}

	if (DVP_WMF_RD_EN == 1) {
		if (pDpeConfig->Dpe_OutBuf_WMF_RD != 0x0) {
			pConfigToKernel->DVP_SRC_24_WMF_RDDV =
			pDpeConfig->Dpe_OutBuf_WMF_RD;
		} else
			LOG_INF("No WMF_RD Output Buffer!\n");

		if (DPE_debug_log_en == 1) {
			LOG_INF("DVP_SRC_24_WMF_RDDV = %llu\n",
			pConfigToKernel->DVP_SRC_24_WMF_RDDV);
		}
	} else {
		if (pDpeConfig->Dpe_OutBuf_ASF_RD != 0x0) {
			pConfigToKernel->DVP_SRC_19_ASF_RDDV =
			pDpeConfig->Dpe_OutBuf_ASF_RD;
		} else
			LOG_INF("No ASF_RD Output Buffer!\n");

		if (DPE_debug_log_en == 1) {
			LOG_INF("DVP_SRC_19_ASF_RDDV = %llu\n",
			pConfigToKernel->DVP_SRC_19_ASF_RDDV);
		}
	}

	if (pDpeConfig->Dpe_OutBuf_ASF_HF != 0x0) {
		pConfigToKernel->DVP_SRC_20_ASF_DV0 =
		pDpeConfig->Dpe_OutBuf_ASF_HF;
	} else
		LOG_INF("No ASF Output Buffer!\n");

	if (DPE_debug_log_en == 1) {
		LOG_INF("DVP_SRC_20_ASF_HF = %llu\n",
		pConfigToKernel->DVP_SRC_20_ASF_DV0);
	}
	//LOG_INF("Dpe_is16BitMode = %d\n", pDpeConfig->Dpe_is16BitMode);

	if (DVP_WMF_HF_EN == 1) { //for WMF
		#ifdef KERNEL_DMA_BUFFER
		//LOG_INF("get kernel asf buffer2\n");
		if (DVP_WMF_F_EN == 0) {
			pConfigToKernel->DVP_SRC_26_WMF_DV0 =
			((dma_addr_t)g_dpewb_wmfhf_Buffer_pa & 0xfffffffff);
		} else {
			pConfigToKernel->DVP_SRC_25_WMF_HFDV =
			((dma_addr_t)g_dpewb_wmfhf_Buffer_pa & 0xfffffffff);
		}
		#else
		if (DVP_WMF_F_EN == 0) {
			if (pDpeConfig->DVP_SRC_25_WMF_HFDV != 0x0) {
				pConfigToKernel->DVP_SRC_26_WMF_DV0 =
				(dma_addr_t)pDpeConfig->DVP_SRC_25_WMF_HFDV;
			} else
				LOG_INF("No DVS DVP_SRC_25_WMF_HFDV Buffer!\n");
		} else {
			if (pDpeConfig->DVP_SRC_25_WMF_HFDV != 0x0) {
				pConfigToKernel->DVP_SRC_25_WMF_HFDV =
				(dma_addr_t)pDpeConfig->DVP_SRC_25_WMF_HFDV;
			} else
				LOG_INF("No DVS DVP_SRC_25_WMF_HFDV Buffer!\n");
		}
		#endif

		if (DPE_debug_log_en == 1) {
			LOG_INF("DVP_SRC_25_WMF_HFDV = %llu\n",
			pConfigToKernel->DVP_SRC_25_WMF_HFDV);
			LOG_INF("DVP_SRC_26_WMF_DV0 = %llu\n",
			pConfigToKernel->DVP_SRC_26_WMF_DV0);
		}
	}

	if (DVP_WMF_F_EN == 1) { //for WMF
		if (pDpeConfig->Dpe_OutBuf_WMF_FILT != 0x0) {
			pConfigToKernel->DVP_SRC_26_WMF_DV0 =
			(dma_addr_t)pDpeConfig->Dpe_OutBuf_WMF_FILT;
		} else
			LOG_INF("No WMF Output Buffer!\n");

		if (DPE_debug_log_en == 1) {
			LOG_INF("DVP_SRC_26_WMF_DV0 = %llu\n",
			pConfigToKernel->DVP_SRC_26_WMF_DV0);
		}
	}

	if (pDpeConfig->Dpe_is16BitMode == 1) {
		if (pDpeConfig->Dpe_InBuf_OCC_Ext != 0x0) {
			pConfigToKernel->DVP_EXT_SRC_13_OCCDV0 =
			(dma_addr_t)pDpeConfig->Dpe_InBuf_OCC_Ext;
		} else {
			LOG_ERR("No DVP Ext OCC Input Buffer!\n");
			return -1;
		}

		// LOG_INF("DVP_EXT_SRC_13_OCCDV0 = %llu\n",
		// pConfigToKernel->DVP_EXT_SRC_13_OCCDV0);

		#ifdef KERNEL_DMA_BUFFER
			LOG_DBG("get kernel asf ext buffer\n");
			pConfigToKernel->DVP_EXT_SRC_18_ASF_RMDV =
			((dma_addr_t)g_dpewb_asfrmext_Buffer_pa & 0xfffffffff);
		#else
			if (pDpeConfig->Dpe_OutBuf_ASF_RM_Ext != 0x0) {
				pConfigToKernel->DVP_EXT_SRC_18_ASF_RMDV =
				pDpeConfig->Dpe_OutBuf_ASF_RM_Ext;
			} else
				LOG_INF("No DVS DVP_EXT_SRC_18_ASF_RMDV Buffer!\n");
		#endif

		//LOG_INF("DVP_EXT_SRC_18_ASF_RMDV = %llu\n",
		//pConfigToKernel->DVP_EXT_SRC_18_ASF_RMDV);

		if (pDpeConfig->Dpe_OutBuf_ASF_RD_Ext != 0x0) {
			pConfigToKernel->DVP_EXT_SRC_19_ASF_RDDV =
			pDpeConfig->Dpe_OutBuf_ASF_RD_Ext;
			LOG_INF("set Dpe_OutBuf_ASF_RD_Ext!\n");
		} else
			LOG_INF("No ASF_RD_EXT Output Buffer!\n");

		//LOG_INF("DVP_EXT_SRC_19_ASF_RDDV = %llu\n",
		//pConfigToKernel->DVP_EXT_SRC_19_ASF_RDDV);

		if (pDpeConfig->Dpe_OutBuf_ASF_HF_Ext != 0x0) {
			pConfigToKernel->DVP_EXT_SRC_20_ASF_DV0 =
			(dma_addr_t)pDpeConfig->Dpe_OutBuf_ASF_HF_Ext;
			LOG_INF("set Dpe_OutBuf_ASF_HF_Ext!\n");
		} else
			LOG_INF("No DVP Ext ASF Output Buffer!\n");

		//LOG_INF("DVP_EXT_SRC_20_ASF_DV0 = %llu\n",
		//pConfigToKernel->DVP_EXT_SRC_20_ASF_DV0);
	}

	//LOG_INF("pDpeConfig->Dpe_RegDump =%d\n", pDpeConfig->Dpe_RegDump);

	pConfigToKernel->DVP_CTRL_ATPG = 0x80000000;
	memcpy(&pConfigToKernel->TuningKernel_DVP,
	&pDpeConfig->Dpe_DVPSettings.TuningBuf_CORE,
	sizeof(pDpeConfig->Dpe_DVPSettings.TuningBuf_CORE));
	return 0;
}

int DPE_Config_DVGF(struct DPE_Config_ISP8 *pDpeConfig,
	struct DPE_Kernel_Config *pConfigToKernel)
{
	unsigned int frmWidth = pDpeConfig->Dpe_DVGFSettings.frm_width;
	unsigned int frmHeight = pDpeConfig->Dpe_DVGFSettings.frm_height;
	unsigned int engStartX = pDpeConfig->Dpe_DVGFSettings.eng_start_x;
	unsigned int engStartY = pDpeConfig->Dpe_DVGFSettings.eng_start_y;
	unsigned int engWidth = pDpeConfig->Dpe_DVGFSettings.eng_width;
	unsigned int engHeight = pDpeConfig->Dpe_DVGFSettings.eng_height;
	unsigned int pitch;
	unsigned int engStart_offset_Y, engStart_offset_C;

	pitch = ALIGN16(pDpeConfig->Dpe_DVGFSettings.frm_width) >> 4;
	engStart_offset_Y = engStartY * (pitch << 4);
	engStart_offset_C = engStart_offset_Y >> 1;

	LOG_INF("DVGF frmW =%d frmH=%d engSX=%d engSY =%d engW=%d engH=%d pitch =%d\n",
	frmWidth, frmHeight, engStartX, engStartY, engWidth, engHeight, pitch);

	//LOG_ERR("DVGF pitch =%d engStartoft_Y=%d engStartoffst_C=%d\n",
	//pitch, engStart_offset_Y, engStart_offset_C);

	pConfigToKernel->DVGF_CTRL_00 =
	(pDpeConfig->Dpe_DVGFSettings.SubModule_EN.dpe_dvgf_en << 31);

	// pConfigToKernel->DVGF_CTRL_01 =  0x00110000;
	if (DPE_debug_log_en == 1) {
		LOG_ERR("DVGF DVGF_CTRL_01 =0x%x\n",
		pConfigToKernel->DVGF_CTRL_01);
	}

	pConfigToKernel->DVGF_CTRL_02 =  0x11010000;
	if (DPE_debug_log_en == 1) {
		LOG_ERR("DVGF DVGF_CTRL_02 =0x%x\n",
		pConfigToKernel->DVGF_CTRL_02);
	}
	pConfigToKernel->DVGF_CTRL_05 = 0x00000900;
	if (DPE_debug_log_en == 1) {
		LOG_ERR("DVGF DVGF_CTRL_05 =0x%x\n",
		pConfigToKernel->DVGF_CTRL_05);
	}

	pConfigToKernel->DVGF_CTRL_07 = 0x00000707;
	if (DPE_debug_log_en == 1) {
		LOG_ERR("DVGF DVGF_CTRL_07 =0x%x\n",
		pConfigToKernel->DVGF_CTRL_07);
	}

	pConfigToKernel->DVGF_IRQ_00 = 0x00000f00;
	if (DPE_debug_log_en == 1) {
		LOG_ERR("DVGF DVGF_IRQ_00 =0x%x\n",
		pConfigToKernel->DVGF_IRQ_00);
	}

	pConfigToKernel->DVGF_IRQ_01 = 0x00000f00;
	//pDpeConfig->Dpe_DVGFSettings.TuningBuf_CORE.DVGF_IRQ_01; //0x00000f00
	if (DPE_debug_log_en == 1) {
		LOG_ERR("DVGF DVGF_IRQ_01 =0x%x\n",
		pConfigToKernel->DVGF_IRQ_01);
	}

	pConfigToKernel->DVGF_DRAM_PITCH =
	pDpeConfig->Dpe_DVGFSettings.Tuning_meta_dvgf.DVGF_DRAM_PITCH;

	pConfigToKernel->DVGF_DRAM_SEC_0 =
	pDpeConfig->Dpe_DVGFSettings.Tuning_meta_dvgf.DVGF_DRAM_SEC_0;

	pConfigToKernel->DVGF_DRAM_SEC_1 =
	pDpeConfig->Dpe_DVGFSettings.Tuning_meta_dvgf.DVGF_DRAM_SEC_1;

	pConfigToKernel->DVGF_CTRL_RESERVED =
	pDpeConfig->Dpe_DVGFSettings.Tuning_meta_dvgf.DVGF_CTRL_RESERVED;

	pConfigToKernel->DVGF_CTRL_ATPG = 0x80000000;
	//pDpeConfig->Dpe_DVGFSettings.TuningBuf_CORE.DVGF_CTRL_ATPG;

	pConfigToKernel->DVGF_SRC_00 =
	((engHeight & 0x7FF) << 0) | ((engWidth & 0x7FF) << 16);

	pConfigToKernel->DVGF_SRC_01 =
	((frmHeight & 0x7FF) << 0) | ((frmWidth & 0x7FF) << 16);

	pConfigToKernel->DVGF_SRC_02 =
	((engStartX & 0x7FF) << 0) | ((engStartY & 0x7FF) << 16);

	pConfigToKernel->DVGF_DRAM_PITCH =
	((pitch & 0x7F) << 4) | ((pitch & 0x7F) << 20);


	//Src Y
	if (pDpeConfig->Dpe_InBuf_SrcImg_Y != 0x0) {
		pConfigToKernel->DVGF_SRC_04 =
		pDpeConfig->Dpe_InBuf_SrcImg_Y +
		(engStart_offset_Y);
		//pConfigToKernel->DVGF_SRC_06 =
		//pDpeConfig->Dpe_InBuf_SrcImg_Y +
		//(engStart_offset_Y);
		//pConfigToKernel->DVGF_SRC_07 =
		//pDpeConfig->Dpe_InBuf_SrcImg_Y +
		//(engStart_offset_Y);
	} else {
		LOG_ERR("No DVGF Src Image Y!\n");
		return -1;
	}
	//Src C
	if (pDpeConfig->Dpe_InBuf_SrcImg_C != 0x0) {
		pConfigToKernel->DVGF_SRC_08 =
		pDpeConfig->Dpe_InBuf_SrcImg_C +
		(engStart_offset_C);
		//pConfigToKernel->DVGF_SRC_10 =
		//pDpeConfig->Dpe_InBuf_SrcImg_C +
		//(engStart_offset_C);
		//pConfigToKernel->DVGF_SRC_11 =
		//pDpeConfig->Dpe_InBuf_SrcImg_C +
		//(engStart_offset_C);
	} else {
		LOG_ERR("No Src Image C!\n");
		return -1;
	}

	//SrcYpre
	if (pDpeConfig->Dpe_InBuf_SrcImg_Y_Pre != 0x0) {
		pConfigToKernel->DVGF_SRC_05 =
		pDpeConfig->Dpe_InBuf_SrcImg_Y_Pre;
	} else {
		LOG_ERR("No Pre Y Src Image Y!\n");
		return -1;
	}
	//Src C Pre
	if (pDpeConfig->Dpe_InBuf_SrcImg_C_Pre != 0x0) {
		pConfigToKernel->DVGF_SRC_09 =
		pDpeConfig->Dpe_InBuf_SrcImg_C_Pre;
	} else {
		LOG_ERR("No C Src Image Y!\n");
		return -1;
	}

	//WT
	if (pDpeConfig->Dpe_OutBuf_WT_Fnl != 0x0) {
		pConfigToKernel->DVGF_SRC_21 =
		pDpeConfig->Dpe_OutBuf_WT_Fnl;
		pConfigToKernel->DVGF_SRC_22 =
		pDpeConfig->Dpe_OutBuf_WT_Fnl;
		pConfigToKernel->DVGF_SRC_23 =
		pDpeConfig->Dpe_OutBuf_WT_Fnl;
		pConfigToKernel->DVGF_SRC_24 =
		pDpeConfig->Dpe_OutBuf_WT_Fnl;
	} else {
		LOG_ERR("No WT Fnl Image buffer !\n");
		return -1;
	}

	//IIR
	if (pDpeConfig->Dpe_OutBuf_RW_IIR != 0x0) {
		pConfigToKernel->DVGF_SRC_20 =
		pDpeConfig->Dpe_OutBuf_RW_IIR;
	} else
		LOG_INF("No IIR Image buffer !\n");

	//DVS out-->DVGF
	if (pDpeConfig->Dpe_OutBuf_OCC_Ext != 0x0) {
		pConfigToKernel->DVGF_SRC_12 =
		pDpeConfig->Dpe_OutBuf_OCC_Ext;
		pConfigToKernel->DVGF_SRC_13 =
		pDpeConfig->Dpe_OutBuf_OCC_Ext;
		pConfigToKernel->DVGF_SRC_14 =
		pDpeConfig->Dpe_OutBuf_OCC_Ext;
		pConfigToKernel->DVGF_SRC_15 =
		pDpeConfig->Dpe_OutBuf_OCC_Ext;
	} else {
		LOG_ERR("No dvs conf Image buffer !\n");
		return -1;
	}

	//DVP out-->DVGF
	if (pDpeConfig->Dpe_OutBuf_WMF_FILT != 0x0) {
		pConfigToKernel->DVGF_SRC_16 =
		pDpeConfig->Dpe_OutBuf_WMF_FILT;
		pConfigToKernel->DVGF_SRC_17 =
		pDpeConfig->Dpe_OutBuf_WMF_FILT;
		pConfigToKernel->DVGF_SRC_18 =
		pDpeConfig->Dpe_OutBuf_WMF_FILT;
		pConfigToKernel->DVGF_SRC_19 =
		pDpeConfig->Dpe_OutBuf_WMF_FILT;
	} else
		LOG_INF("No dvp wmf filt Image buffer !\n");

	memcpy(&pConfigToKernel->TuningKernel_DVGF,
	&pDpeConfig->Dpe_DVGFSettings.TuningBuf_CORE.DVGF_CORE_00,
	sizeof(pConfigToKernel->TuningKernel_DVGF));//sizeof(pDpeConfig->Dpe_DVGFSettings.TuningBuf_CORE)
	return 0;
}

void DPE_DumptoHWReg(struct DPE_Kernel_Config *pConfigToKernel)
{
	LOG_INF("Dump DVS_Ctrl\n");
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL00_HW),
	pConfigToKernel->DVS_CTRL00); //DVS
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL01_HW),
	pConfigToKernel->DVS_CTRL01);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL02_HW),
	pConfigToKernel->DVS_CTRL02);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL03_HW),
	pConfigToKernel->DVS_CTRL03);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL06_HW),
	pConfigToKernel->DVS_CTRL06);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL07_HW),
	pConfigToKernel->DVS_CTRL07);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL08_HW),
	pConfigToKernel->DVS_CTRL08);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_STATUS3_HW),
	pConfigToKernel->DVS_CTRL_STATUS3);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_IRQ_00_HW),
	pConfigToKernel->DVS_IRQ_00);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_IRQ_01_HW),
	pConfigToKernel->DVS_IRQ_01);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_STATUS0_HW),
	pConfigToKernel->DVS_CTRL_STATUS0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_STATUS2_HW),
	pConfigToKernel->DVS_CTRL_STATUS2);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_IRQ_STATUS_HW),
	pConfigToKernel->DVS_IRQ_STATUS);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS0_HW),
	pConfigToKernel->DVS_FRM_STATUS0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS1_HW),
	pConfigToKernel->DVS_FRM_STATUS1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS2_HW),
	pConfigToKernel->DVS_FRM_STATUS2);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS3_HW),
	pConfigToKernel->DVS_FRM_STATUS3);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS4_HW),
	pConfigToKernel->DVS_FRM_STATUS4);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_EXT_STATUS0_HW),
	pConfigToKernel->DVS_EXT_STATUS0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_EXT_STATUS1_HW),
	pConfigToKernel->DVS_EXT_STATUS1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CUR_STATUS_HW),
	pConfigToKernel->DVS_CUR_STATUS);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_CTRL_HW),
	pConfigToKernel->DVS_SRC_CTRL);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_CTRL_HW),
	pConfigToKernel->DVS_CRC_CTRL);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_IN_HW),
	pConfigToKernel->DVS_CRC_IN);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_STA0_HW),
	pConfigToKernel->DVS_DRAM_STA0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_STA1_HW),
	pConfigToKernel->DVS_DRAM_STA1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_ULT_HW),
	pConfigToKernel->DVS_DRAM_ULT);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_PITCH_HW),
	pConfigToKernel->DVS_DRAM_PITCH);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_PITCH1_HW),
	pConfigToKernel->DVS_DRAM_PITCH1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_00_HW),
	pConfigToKernel->DVS_SRC_00);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_01_HW),
	pConfigToKernel->DVS_SRC_01);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_02_HW),
	pConfigToKernel->DVS_SRC_02);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_03_HW),
	pConfigToKernel->DVS_SRC_03);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_04_HW),
	pConfigToKernel->DVS_SRC_04);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_05_L_FRM0_HW),
	pConfigToKernel->DVS_SRC_05_L_FRM0);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_06_L_FRM1_HW),
	pConfigToKernel->DVS_SRC_06_L_FRM1);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_07_L_FRM2_HW),
	pConfigToKernel->DVS_SRC_07_L_FRM2);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_08_L_FRM3_HW),
	pConfigToKernel->DVS_SRC_08_L_FRM3);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_09_R_FRM0_HW),
	pConfigToKernel->DVS_SRC_09_R_FRM0);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_10_R_FRM1_HW),
	pConfigToKernel->DVS_SRC_10_R_FRM1);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_11_R_FRM2_HW),
	pConfigToKernel->DVS_SRC_11_R_FRM2);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_12_R_FRM3_HW),
	pConfigToKernel->DVS_SRC_12_R_FRM3);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_13_Hist0_HW),
	pConfigToKernel->DVS_SRC_13_Hist0);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_14_Hist1_HW),
	pConfigToKernel->DVS_SRC_14_Hist1);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_15_Hist2_HW),
	pConfigToKernel->DVS_SRC_15_Hist2);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_16_Hist3_HW),
	pConfigToKernel->DVS_SRC_16_Hist3);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_17_OCCDV_EXT0_HW),
	pConfigToKernel->DVS_SRC_17_OCCDV_EXT0);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_18_OCCDV_EXT1_HW),
	pConfigToKernel->DVS_SRC_18_OCCDV_EXT1);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_19_OCCDV_EXT2_HW),
	pConfigToKernel->DVS_SRC_19_OCCDV_EXT2);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_20_OCCDV_EXT3_HW),
	pConfigToKernel->DVS_SRC_20_OCCDV_EXT3);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_21_P4_L_DV_ADR_HW),
	pConfigToKernel->DVS_SRC_21_P4_L_DV_ADR);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_22_OCCDV0_HW),
	pConfigToKernel->DVS_SRC_22_OCCDV0);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_23_OCCDV1_HW),
	pConfigToKernel->DVS_SRC_23_OCCDV1);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_24_OCCDV2_HW),
	pConfigToKernel->DVS_SRC_24_OCCDV2);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_25_OCCDV3_HW),
	pConfigToKernel->DVS_SRC_25_OCCDV3);
	LOG_INF("[0x%08X %08llu]\n", (unsigned int)(DVS_SRC_26_P4_R_DV_ADR_HW),
	pConfigToKernel->DVS_SRC_26_P4_R_DV_ADR);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_27_HW),
	pConfigToKernel->DVS_SRC_27);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_28_HW),
	pConfigToKernel->DVS_SRC_28);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_29_HW),
	pConfigToKernel->DVS_SRC_29);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_30_HW),
	pConfigToKernel->DVS_SRC_30);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_31_HW),
	pConfigToKernel->DVS_SRC_31);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_32_HW),
	pConfigToKernel->DVS_SRC_32);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_33_HW),
	pConfigToKernel->DVS_SRC_33);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_34_HW),
	pConfigToKernel->DVS_SRC_34);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_35_HW),
	pConfigToKernel->DVS_SRC_35);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_36_HW),
	pConfigToKernel->DVS_SRC_36);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_37_HW),
	pConfigToKernel->DVS_SRC_37);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_38_HW),
	pConfigToKernel->DVS_SRC_38);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_39_HW),
	pConfigToKernel->DVS_SRC_39);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_OUT_0_HW),
	pConfigToKernel->DVS_CRC_OUT_0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_OUT_1_HW),
	pConfigToKernel->DVS_CRC_OUT_1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_OUT_2_HW),
	pConfigToKernel->DVS_CRC_OUT_2);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_OUT_3_HW),
	pConfigToKernel->DVS_CRC_OUT_3);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_SEC_0_HW),
	pConfigToKernel->DVS_DRAM_SEC_0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_SEC_1_HW),
	pConfigToKernel->DVS_DRAM_SEC_1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_SEC_2_HW),
	pConfigToKernel->DVS_DRAM_SEC_2);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_SEC_3_HW),
	pConfigToKernel->DVS_DRAM_SEC_3);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_AXSLC_0_HW),
	pConfigToKernel->DVS_DRAM_AXSLC_0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_AXSLC_1_HW),
	pConfigToKernel->DVS_DRAM_AXSLC_1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DEQ_FORCE_HW),
	pConfigToKernel->DVS_DEQ_FORCE);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_RESERVED_HW),
	pConfigToKernel->DVS_CTRL_RESERVED);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_ATPG_HW),
	pConfigToKernel->DVS_CTRL_ATPG);
	LOG_INF("Dump DVS_ME\n");
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_00_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_00);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_01_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_01);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_02_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_02);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_03_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_03);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_04_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_04);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_05_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_05);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_06_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_06);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_07_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_07);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_08_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_08);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_09_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_09);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_10_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_10);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_11_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_11);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_12_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_12);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_13_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_13);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_14_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_14);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_15_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_15);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_16_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_16);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_17_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_17);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_18_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_18);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_19_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_19);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_20_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_20);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_21_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_21);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_22_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_22);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_23_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_23);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_24_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_24);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_25_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_25);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_26_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_26);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_27_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_27);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_28_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_28);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_29_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_29);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_30_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_30);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_31_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_31);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_32_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_32);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_33_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_33);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_34_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_34);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_35_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_35);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_36_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_36);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_37_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_37);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_38_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_38);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_39_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_39);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_40_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_40);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_41_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_41);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_42_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_42);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_43_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_43);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_44_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_44);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_45_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_45);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_46_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_46);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_47_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_47);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_48_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_48);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_49_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_49);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_50_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_50);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_51_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_51);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_52_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_52);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_53_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_53);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_54_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_54);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_55_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_55);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_56_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_56);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_57_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_57);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_58_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_58);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_59_HW),
	pConfigToKernel->TuningBuf_ME.DVS_ME_59);
	LOG_INF("Dump DVS_OCC\n");
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_0_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_PQ_0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_1_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_PQ_1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_2_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_PQ_2);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_3_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_PQ_3);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_4_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_PQ_4);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_5_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_PQ_5);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_10_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_PQ_10);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_11_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_PQ_11);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_12_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_PQ_12);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_ATPG_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_ATPG);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST0_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST1_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST2_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST2);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST3_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST3);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST4_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST4);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST5_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST5);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST6_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST6);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST7_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST7);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST8_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST8);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST9_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST9);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST10_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST10);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST11_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST11);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST12_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST12);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST13_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST13);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST14_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST14);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST15_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST15);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST16_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST16);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST17_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST17);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST18_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_HIST18);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_LDV0_HW),
	pConfigToKernel->TuningKernel_OCC.DVS_OCC_LDV0);

	LOG_INF("Dump DVGF\n");
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_00_HW),
	pConfigToKernel->DVGF_CTRL_00);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_01_HW),
	pConfigToKernel->DVGF_CTRL_01);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_02_HW),
	pConfigToKernel->DVGF_CTRL_02);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_03_HW),
	pConfigToKernel->DVGF_CTRL_03);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_05_HW),
	pConfigToKernel->DVGF_CTRL_05);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_07_HW),
	pConfigToKernel->DVGF_CTRL_07);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_IRQ_00_HW),
	pConfigToKernel->DVGF_IRQ_00);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_IRQ_01_HW),
	pConfigToKernel->DVGF_IRQ_01);

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_STATUS0_HW),
	pConfigToKernel->DVGF_CTRL_STATUS0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_STATUS1_HW),
	pConfigToKernel->DVGF_CTRL_STATUS1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_IRQ_STATUS_HW),
	pConfigToKernel->DVGF_IRQ_STATUS);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_FRM_STATUS_HW),
	pConfigToKernel->DVGF_FRM_STATUS);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CUR_STATUS_HW),
	pConfigToKernel->DVGF_CUR_STATUS);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_CTRL_HW),
	pConfigToKernel->DVGF_CRC_CTRL);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_OUT_HW),
	pConfigToKernel->DVGF_CRC_OUT);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_IN_HW),
	pConfigToKernel->DVGF_CRC_IN);

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_OUT_0_HW),
	pConfigToKernel->DVGF_CRC_OUT_0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_OUT_1_HW),
	pConfigToKernel->DVGF_CRC_OUT_1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_OUT_2_HW),
	pConfigToKernel->DVGF_CRC_OUT_2);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_CRC_IN_HW),
	pConfigToKernel->DVGF_CORE_CRC_IN);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_DRAM_STA_HW),
	pConfigToKernel->DVGF_DRAM_STA);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_DRAM_PITCH_HW),
	pConfigToKernel->DVGF_DRAM_PITCH);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_DRAM_SEC_0_HW),
	pConfigToKernel->DVGF_DRAM_SEC_0);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_DRAM_SEC_1_HW),
	pConfigToKernel->DVGF_DRAM_SEC_1);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_DRAM_AXSLC_HW),
	pConfigToKernel->DVGF_DRAM_AXSLC);

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_RESERVED_HW),
	pConfigToKernel->DVGF_CTRL_RESERVED);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_ATPG_HW),
	pConfigToKernel->DVGF_CTRL_ATPG);

	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_00_HW),
	pConfigToKernel->DVGF_SRC_00);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_01_HW),
	pConfigToKernel->DVGF_SRC_01);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_02_HW),
	pConfigToKernel->DVGF_SRC_02);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_04_HW),
	pConfigToKernel->DVGF_SRC_04);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_05_HW),
	pConfigToKernel->DVGF_SRC_05);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_06_HW),
	pConfigToKernel->DVGF_SRC_06);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_07_HW),
	pConfigToKernel->DVGF_SRC_07);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_08_HW),
	pConfigToKernel->DVGF_SRC_08);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_09_HW),
	pConfigToKernel->DVGF_SRC_09);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_10_HW),
	pConfigToKernel->DVGF_SRC_10);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_11_HW),
	pConfigToKernel->DVGF_SRC_11);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_12_HW),
	pConfigToKernel->DVGF_SRC_12);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_13_HW),
	pConfigToKernel->DVGF_SRC_13);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_14_HW),
	pConfigToKernel->DVGF_SRC_14);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_15_HW),
	pConfigToKernel->DVGF_SRC_15);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_16_HW),
	pConfigToKernel->DVGF_SRC_16);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_17_HW),
	pConfigToKernel->DVGF_SRC_17);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_18_HW),
	pConfigToKernel->DVGF_SRC_18);
   //
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_19_HW),
	pConfigToKernel->DVGF_SRC_19);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_20_HW),
	pConfigToKernel->DVGF_SRC_20);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_21_HW),
	pConfigToKernel->DVGF_SRC_21);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_22_HW),
	pConfigToKernel->DVGF_SRC_22);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_23_HW),
	pConfigToKernel->DVGF_SRC_23);
	LOG_INF("[0x%08X %llu]\n", (unsigned int)(DVGF_SRC_24_HW),
	pConfigToKernel->DVGF_SRC_24);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_00_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_00);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_01_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_01);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_02_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_02);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_03_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_03);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_05_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_05);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_06_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_06);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_07_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_07);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_08_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_08);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_09_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_09);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_10_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_10);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_11_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_11);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_12_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_12);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_13_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_13);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_14_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_14);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_15_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_15);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_16_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_16);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_17_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_17);
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_18_HW),
	pConfigToKernel->TuningKernel_DVGF.DVGF_CORE_18);
}

static signed int DPE_DumpReg(void)
#if BYPASS_REG
{
	return 0;
}
#else
{
	signed int Ret = 0;
	/*  */
	LOG_INF("- E.");
	spin_lock(&(DPEInfo.SpinLockDPE));
	if (g_u4EnableClockCount == 0) {
		spin_unlock(&(DPEInfo.SpinLockDPE));
		return 0;
	}
	spin_unlock(&(DPEInfo.SpinLockDPE));
//#if 1
	/*  */
	LOG_INF("DPE Register Dump\n");
	/* DPE Config0 */
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL00_HW),
		(unsigned int)DPE_RD32(DVS_CTRL00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL01_HW),
		(unsigned int)DPE_RD32(DVS_CTRL01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL02_HW),
		(unsigned int)DPE_RD32(DVS_CTRL02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL03_HW),
		(unsigned int)DPE_RD32(DVS_CTRL03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL06_HW),
		(unsigned int)DPE_RD32(DVS_CTRL06_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL07_HW),
		(unsigned int)DPE_RD32(DVS_CTRL07_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL08_HW),
		(unsigned int)DPE_RD32(DVS_CTRL08_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_STATUS3_HW),
		(unsigned int)DPE_RD32(DVS_CTRL_STATUS3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_IRQ_00_HW),
		(unsigned int)DPE_RD32(DVS_IRQ_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_IRQ_01_HW),
		(unsigned int)DPE_RD32(DVS_IRQ_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_STATUS0_HW),
		(unsigned int)DPE_RD32(DVS_CTRL_STATUS0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_STATUS2_HW),
		(unsigned int)DPE_RD32(DVS_CTRL_STATUS2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_IRQ_STATUS_HW),
		(unsigned int)DPE_RD32(DVS_IRQ_STATUS_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS0_HW),
		(unsigned int)DPE_RD32(DVS_FRM_STATUS0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS1_HW),
		(unsigned int)DPE_RD32(DVS_FRM_STATUS1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS2_HW),
		(unsigned int)DPE_RD32(DVS_FRM_STATUS2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS3_HW),
		(unsigned int)DPE_RD32(DVS_FRM_STATUS3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS4_HW),
		(unsigned int)DPE_RD32(DVS_FRM_STATUS4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_EXT_STATUS0_HW),
		(unsigned int)DPE_RD32(DVS_EXT_STATUS0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_EXT_STATUS1_HW),
		(unsigned int)DPE_RD32(DVS_EXT_STATUS1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CUR_STATUS_HW),
		(unsigned int)DPE_RD32(DVS_CUR_STATUS_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_CTRL_HW),
		(unsigned int)DPE_RD32(DVS_SRC_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_CTRL_HW),
		(unsigned int)DPE_RD32(DVS_CRC_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_IN_HW),
		(unsigned int)DPE_RD32(DVS_CRC_IN_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_STA0_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_STA0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_STA1_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_STA1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_ULT_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_ULT_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_PITCH_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_PITCH_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_PITCH1_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_PITCH1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_00_HW),
		(unsigned int)DPE_RD32(DVS_SRC_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_01_HW),
		(unsigned int)DPE_RD32(DVS_SRC_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_02_HW),
		(unsigned int)DPE_RD32(DVS_SRC_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_03_HW),
		(unsigned int)DPE_RD32(DVS_SRC_03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_04_HW),
		(unsigned int)DPE_RD32(DVS_SRC_04_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_05_L_FRM0_HW),
		(unsigned int)DPE_RD32(DVS_SRC_05_L_FRM0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_06_L_FRM1_HW),
		(unsigned int)DPE_RD32(DVS_SRC_06_L_FRM1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_07_L_FRM2_HW),
		(unsigned int)DPE_RD32(DVS_SRC_07_L_FRM2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_08_L_FRM3_HW),
		(unsigned int)DPE_RD32(DVS_SRC_08_L_FRM3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_09_R_FRM0_HW),
		(unsigned int)DPE_RD32(DVS_SRC_09_R_FRM0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_10_R_FRM1_HW),
		(unsigned int)DPE_RD32(DVS_SRC_10_R_FRM1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_11_R_FRM2_HW),
		(unsigned int)DPE_RD32(DVS_SRC_11_R_FRM2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_12_R_FRM3_HW),
		(unsigned int)DPE_RD32(DVS_SRC_12_R_FRM3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_13_Hist0_HW),
		(unsigned int)DPE_RD32(DVS_SRC_13_Hist0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_14_Hist1_HW),
		(unsigned int)DPE_RD32(DVS_SRC_14_Hist1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_15_Hist2_HW),
		(unsigned int)DPE_RD32(DVS_SRC_15_Hist2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_16_Hist3_HW),
		(unsigned int)DPE_RD32(DVS_SRC_16_Hist3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_17_OCCDV_EXT0_HW),
		(unsigned int)DPE_RD32(DVS_SRC_17_OCCDV_EXT0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_18_OCCDV_EXT1_HW),
		(unsigned int)DPE_RD32(DVS_SRC_18_OCCDV_EXT1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_19_OCCDV_EXT2_HW),
		(unsigned int)DPE_RD32(DVS_SRC_19_OCCDV_EXT2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_20_OCCDV_EXT3_HW),
		(unsigned int)DPE_RD32(DVS_SRC_20_OCCDV_EXT3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_21_P4_L_DV_ADR_HW),
		(unsigned int)DPE_RD32(DVS_SRC_21_P4_L_DV_ADR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_22_OCCDV0_HW),
		(unsigned int)DPE_RD32(DVS_SRC_22_OCCDV0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_23_OCCDV1_HW),
		(unsigned int)DPE_RD32(DVS_SRC_23_OCCDV1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_24_OCCDV2_HW),
		(unsigned int)DPE_RD32(DVS_SRC_24_OCCDV2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_25_OCCDV3_HW),
		(unsigned int)DPE_RD32(DVS_SRC_25_OCCDV3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_26_P4_R_DV_ADR_HW),
		(unsigned int)DPE_RD32(DVS_SRC_26_P4_R_DV_ADR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_27_HW),
		(unsigned int)DPE_RD32(DVS_SRC_27_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_28_HW),
		(unsigned int)DPE_RD32(DVS_SRC_28_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_29_HW),
		(unsigned int)DPE_RD32(DVS_SRC_29_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_30_HW),
		(unsigned int)DPE_RD32(DVS_SRC_30_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_31_HW),
		(unsigned int)DPE_RD32(DVS_SRC_31_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_32_HW),
		(unsigned int)DPE_RD32(DVS_SRC_32_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_33_HW),
		(unsigned int)DPE_RD32(DVS_SRC_33_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_34_HW),
		(unsigned int)DPE_RD32(DVS_SRC_34_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_35_HW),
		(unsigned int)DPE_RD32(DVS_SRC_35_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_36_HW),
		(unsigned int)DPE_RD32(DVS_SRC_36_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_37_HW),
		(unsigned int)DPE_RD32(DVS_SRC_37_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_38_HW),
		(unsigned int)DPE_RD32(DVS_SRC_38_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_SRC_39_HW),
		(unsigned int)DPE_RD32(DVS_SRC_39_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_OUT_0_HW),
		(unsigned int)DPE_RD32(DVS_CRC_OUT_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_OUT_1_HW),
		(unsigned int)DPE_RD32(DVS_CRC_OUT_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_OUT_2_HW),
		(unsigned int)DPE_RD32(DVS_CRC_OUT_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CRC_OUT_3_HW),
		(unsigned int)DPE_RD32(DVS_CRC_OUT_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_SEC_0_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_SEC_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_SEC_1_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_SEC_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_SEC_2_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_SEC_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_SEC_3_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_SEC_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_AXSLC_0_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_AXSLC_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_AXSLC_1_HW),
		(unsigned int)DPE_RD32(DVS_DRAM_AXSLC_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DEQ_FORCE_HW),
		(unsigned int)DPE_RD32(DVS_DEQ_FORCE_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_RESERVED_HW),
		(unsigned int)DPE_RD32(DVS_CTRL_RESERVED_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_ATPG_HW),
		(unsigned int)DPE_RD32(DVS_CTRL_ATPG_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_00_HW),
		(unsigned int)DPE_RD32(DVS_ME_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_01_HW),
		(unsigned int)DPE_RD32(DVS_ME_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_02_HW),
		(unsigned int)DPE_RD32(DVS_ME_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_03_HW),
		(unsigned int)DPE_RD32(DVS_ME_03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_04_HW),
		(unsigned int)DPE_RD32(DVS_ME_04_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_05_HW),
		(unsigned int)DPE_RD32(DVS_ME_05_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_06_HW),
		(unsigned int)DPE_RD32(DVS_ME_06_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_07_HW),
		(unsigned int)DPE_RD32(DVS_ME_07_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_08_HW),
		(unsigned int)DPE_RD32(DVS_ME_08_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_09_HW),
		(unsigned int)DPE_RD32(DVS_ME_09_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_10_HW),
		(unsigned int)DPE_RD32(DVS_ME_10_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_11_HW),
		(unsigned int)DPE_RD32(DVS_ME_11_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_12_HW),
		(unsigned int)DPE_RD32(DVS_ME_12_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_13_HW),
		(unsigned int)DPE_RD32(DVS_ME_13_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_14_HW),
		(unsigned int)DPE_RD32(DVS_ME_14_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_15_HW),
		(unsigned int)DPE_RD32(DVS_ME_15_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_16_HW),
		(unsigned int)DPE_RD32(DVS_ME_16_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_17_HW),
		(unsigned int)DPE_RD32(DVS_ME_17_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_18_HW),
		(unsigned int)DPE_RD32(DVS_ME_18_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_19_HW),
		(unsigned int)DPE_RD32(DVS_ME_19_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_20_HW),
		(unsigned int)DPE_RD32(DVS_ME_20_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_21_HW),
		(unsigned int)DPE_RD32(DVS_ME_21_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_22_HW),
		(unsigned int)DPE_RD32(DVS_ME_22_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_23_HW),
		(unsigned int)DPE_RD32(DVS_ME_23_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_24_HW),
		(unsigned int)DPE_RD32(DVS_ME_24_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_25_HW),
		(unsigned int)DPE_RD32(DVS_ME_25_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_26_HW),
		(unsigned int)DPE_RD32(DVS_ME_26_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_27_HW),
		(unsigned int)DPE_RD32(DVS_ME_27_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_28_HW),
		(unsigned int)DPE_RD32(DVS_ME_28_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_29_HW),
		(unsigned int)DPE_RD32(DVS_ME_29_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_30_HW),
		(unsigned int)DPE_RD32(DVS_ME_30_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_31_HW),
		(unsigned int)DPE_RD32(DVS_ME_31_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_32_HW),
		(unsigned int)DPE_RD32(DVS_ME_32_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_33_HW),
		(unsigned int)DPE_RD32(DVS_ME_33_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_34_HW),
		(unsigned int)DPE_RD32(DVS_ME_34_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_35_HW),
		(unsigned int)DPE_RD32(DVS_ME_35_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_36_HW),
		(unsigned int)DPE_RD32(DVS_ME_36_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_37_HW),
		(unsigned int)DPE_RD32(DVS_ME_37_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_38_HW),
		(unsigned int)DPE_RD32(DVS_ME_38_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_39_HW),
		(unsigned int)DPE_RD32(DVS_ME_39_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_DEBUG_HW),
		(unsigned int)DPE_RD32(DVS_DEBUG_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_RESERVED_HW),
		(unsigned int)DPE_RD32(DVS_ME_RESERVED_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_ATPG_HW),
		(unsigned int)DPE_RD32(DVS_ME_ATPG_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_40_HW),
		(unsigned int)DPE_RD32(DVS_ME_40_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_41_HW),
		(unsigned int)DPE_RD32(DVS_ME_41_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_42_HW),
		(unsigned int)DPE_RD32(DVS_ME_42_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_43_HW),
		(unsigned int)DPE_RD32(DVS_ME_43_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_44_HW),
		(unsigned int)DPE_RD32(DVS_ME_44_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_45_HW),
		(unsigned int)DPE_RD32(DVS_ME_45_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_46_HW),
		(unsigned int)DPE_RD32(DVS_ME_46_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_47_HW),
		(unsigned int)DPE_RD32(DVS_ME_47_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_48_HW),
		(unsigned int)DPE_RD32(DVS_ME_48_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_49_HW),
		(unsigned int)DPE_RD32(DVS_ME_49_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_50_HW),
		(unsigned int)DPE_RD32(DVS_ME_50_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_51_HW),
		(unsigned int)DPE_RD32(DVS_ME_51_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_52_HW),
		(unsigned int)DPE_RD32(DVS_ME_52_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_53_HW),
		(unsigned int)DPE_RD32(DVS_ME_53_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_54_HW),
		(unsigned int)DPE_RD32(DVS_ME_54_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_55_HW),
		(unsigned int)DPE_RD32(DVS_ME_55_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_56_HW),
		(unsigned int)DPE_RD32(DVS_ME_56_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_57_HW),
		(unsigned int)DPE_RD32(DVS_ME_57_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_58_HW),
		(unsigned int)DPE_RD32(DVS_ME_58_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_ME_59_HW),
		(unsigned int)DPE_RD32(DVS_ME_59_REG));

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_0_HW),
		(unsigned int)DPE_RD32(DVS_OCC_PQ_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_1_HW),
		(unsigned int)DPE_RD32(DVS_OCC_PQ_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_2_HW),
		(unsigned int)DPE_RD32(DVS_OCC_PQ_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_3_HW),
		(unsigned int)DPE_RD32(DVS_OCC_PQ_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_4_HW),
		(unsigned int)DPE_RD32(DVS_OCC_PQ_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_5_HW),
		(unsigned int)DPE_RD32(DVS_OCC_PQ_5_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_10_HW),
		(unsigned int)DPE_RD32(DVS_OCC_PQ_10_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_11_HW),
		(unsigned int)DPE_RD32(DVS_OCC_PQ_11_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_PQ_12_HW),
		(unsigned int)DPE_RD32(DVS_OCC_PQ_12_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_ATPG_HW),
		(unsigned int)DPE_RD32(DVS_OCC_ATPG_REG));

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST0_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST1_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST2_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST3_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST4_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST5_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST5_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST6_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST6_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST7_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST7_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST8_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST8_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST9_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST9_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST10_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST10_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST11_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST11_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST12_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST12_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST13_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST13_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST14_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST14_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST15_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST15_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST16_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST16_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST17_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST17_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_HIST18_HW),
		(unsigned int)DPE_RD32(DVS_OCC_HIST18_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVS_OCC_LDV0_HW),
		(unsigned int)DPE_RD32(DVS_OCC_LDV0_REG));

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL00_HW),
		(unsigned int)DPE_RD32(DVP_CTRL00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL01_HW),
		(unsigned int)DPE_RD32(DVP_CTRL01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL02_HW),
		(unsigned int)DPE_RD32(DVP_CTRL02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL03_HW),
		(unsigned int)DPE_RD32(DVP_CTRL03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL04_HW),
		(unsigned int)DPE_RD32(DVP_CTRL04_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL05_HW),
		(unsigned int)DPE_RD32(DVP_CTRL05_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL07_HW),
		(unsigned int)DPE_RD32(DVP_CTRL07_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_IRQ_00_HW),
		(unsigned int)DPE_RD32(DVP_IRQ_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_IRQ_01_HW),
		(unsigned int)DPE_RD32(DVP_IRQ_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL_STATUS0_HW),
		(unsigned int)DPE_RD32(DVP_CTRL_STATUS0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL_STATUS1_HW),
		(unsigned int)DPE_RD32(DVP_CTRL_STATUS1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_IRQ_STATUS_HW),
		(unsigned int)DPE_RD32(DVP_IRQ_STATUS_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_FRM_STATUS0_HW),
		(unsigned int)DPE_RD32(DVP_FRM_STATUS0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_FRM_STATUS1_HW),
		(unsigned int)DPE_RD32(DVP_FRM_STATUS1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_FRM_STATUS2_HW),
		(unsigned int)DPE_RD32(DVP_FRM_STATUS2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_FRM_STATUS3_HW),
		(unsigned int)DPE_RD32(DVP_FRM_STATUS3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CUR_STATUS_HW),
		(unsigned int)DPE_RD32(DVP_CUR_STATUS_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_00_HW),
		(unsigned int)DPE_RD32(DVP_SRC_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_01_HW),
		(unsigned int)DPE_RD32(DVP_SRC_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_02_HW),
		(unsigned int)DPE_RD32(DVP_SRC_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_03_HW),
		(unsigned int)DPE_RD32(DVP_SRC_03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_04_HW),
		(unsigned int)DPE_RD32(DVP_SRC_04_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_05_Y_FRM0_HW),
		(unsigned int)DPE_RD32(DVP_SRC_05_Y_FRM0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_06_Y_FRM1_HW),
		(unsigned int)DPE_RD32(DVP_SRC_06_Y_FRM1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_07_Y_FRM2_HW),
		(unsigned int)DPE_RD32(DVP_SRC_07_Y_FRM2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_08_Y_FRM3_HW),
		(unsigned int)DPE_RD32(DVP_SRC_08_Y_FRM3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_09_C_FRM0_HW),
		(unsigned int)DPE_RD32(DVP_SRC_09_C_FRM0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_10_C_FRM1_HW),
		(unsigned int)DPE_RD32(DVP_SRC_10_C_FRM1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_11_C_FRM2_HW),
		(unsigned int)DPE_RD32(DVP_SRC_11_C_FRM2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_12_C_FRM3_HW),
		(unsigned int)DPE_RD32(DVP_SRC_12_C_FRM3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_13_OCCDV0_HW),
		(unsigned int)DPE_RD32(DVP_SRC_13_OCCDV0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_14_OCCDV1_HW),
		(unsigned int)DPE_RD32(DVP_SRC_14_OCCDV1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_15_OCCDV2_HW),
		(unsigned int)DPE_RD32(DVP_SRC_15_OCCDV2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_16_OCCDV3_HW),
		(unsigned int)DPE_RD32(DVP_SRC_16_OCCDV3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_17_CRM_HW),
		(unsigned int)DPE_RD32(DVP_SRC_17_CRM_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_18_ASF_RMDV_HW),
		(unsigned int)DPE_RD32(DVP_SRC_18_ASF_RMDV_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_19_ASF_RDDV_HW),
		(unsigned int)DPE_RD32(DVP_SRC_19_ASF_RDDV_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_20_ASF_DV0_HW),
		(unsigned int)DPE_RD32(DVP_SRC_20_ASF_DV0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_21_ASF_DV1_HW),
		(unsigned int)DPE_RD32(DVP_SRC_21_ASF_DV1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_22_ASF_DV2_HW),
		(unsigned int)DPE_RD32(DVP_SRC_22_ASF_DV2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_23_ASF_DV3_HW),
		(unsigned int)DPE_RD32(DVP_SRC_23_ASF_DV3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_25_WMF_HFDV_HW),
		(unsigned int)DPE_RD32(DVP_SRC_25_WMF_HFDV_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_26_WMF_DV0_HW),
		(unsigned int)DPE_RD32(DVP_SRC_26_WMF_DV0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_27_WMF_DV1_HW),
		(unsigned int)DPE_RD32(DVP_SRC_27_WMF_DV1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_28_WMF_DV2_HW),
		(unsigned int)DPE_RD32(DVP_SRC_28_WMF_DV2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_29_WMF_DV3_HW),
		(unsigned int)DPE_RD32(DVP_SRC_29_WMF_DV3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_00_HW),
		(unsigned int)DPE_RD32(DVP_CORE_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_01_HW),
		(unsigned int)DPE_RD32(DVP_CORE_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_02_HW),
		(unsigned int)DPE_RD32(DVP_CORE_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_03_HW),
		(unsigned int)DPE_RD32(DVP_CORE_03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_04_HW),
		(unsigned int)DPE_RD32(DVP_CORE_04_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_05_HW),
		(unsigned int)DPE_RD32(DVP_CORE_05_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_06_HW),
		(unsigned int)DPE_RD32(DVP_CORE_06_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_07_HW),
		(unsigned int)DPE_RD32(DVP_CORE_07_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_08_HW),
		(unsigned int)DPE_RD32(DVP_CORE_08_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_09_HW),
		(unsigned int)DPE_RD32(DVP_CORE_09_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_10_HW),
		(unsigned int)DPE_RD32(DVP_CORE_10_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_11_HW),
		(unsigned int)DPE_RD32(DVP_CORE_11_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_12_HW),
		(unsigned int)DPE_RD32(DVP_CORE_12_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_13_HW),
		(unsigned int)DPE_RD32(DVP_CORE_13_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_14_HW),
		(unsigned int)DPE_RD32(DVP_CORE_14_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_15_HW),
		(unsigned int)DPE_RD32(DVP_CORE_15_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_16_HW),
		(unsigned int)DPE_RD32(DVP_CORE_16_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_17_HW),
		(unsigned int)DPE_RD32(DVP_CORE_17_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_18_HW),
		(unsigned int)DPE_RD32(DVP_CORE_18_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_19_HW),
		(unsigned int)DPE_RD32(DVP_CORE_19_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_SRC_CTRL_HW),
		(unsigned int)DPE_RD32(DVP_SRC_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL_RESERVED_HW),
		(unsigned int)DPE_RD32(DVP_CTRL_RESERVED_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CTRL_ATPG_HW),
		(unsigned int)DPE_RD32(DVP_CTRL_ATPG_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CRC_OUT_0_HW),
		(unsigned int)DPE_RD32(DVP_CRC_OUT_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CRC_OUT_1_HW),
		(unsigned int)DPE_RD32(DVP_CRC_OUT_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CRC_OUT_2_HW),
		(unsigned int)DPE_RD32(DVP_CRC_OUT_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CRC_CTRL_HW),
		(unsigned int)DPE_RD32(DVP_CRC_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CRC_OUT_HW),
		(unsigned int)DPE_RD32(DVP_CRC_OUT_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CRC_IN_HW),
		(unsigned int)DPE_RD32(DVP_CRC_IN_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_DRAM_STA_HW),
		(unsigned int)DPE_RD32(DVP_DRAM_STA_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_DRAM_ULT_HW),
		(unsigned int)DPE_RD32(DVP_DRAM_ULT_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_DRAM_PITCH_HW),
		(unsigned int)DPE_RD32(DVP_DRAM_PITCH_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_DRAM_SEC_0_HW),
		(unsigned int)DPE_RD32(DVP_DRAM_SEC_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_DRAM_SEC_1_HW),
		(unsigned int)DPE_RD32(DVP_DRAM_SEC_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_DRAM_AXSLC_HW),
		(unsigned int)DPE_RD32(DVP_DRAM_AXSLC_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_CORE_CRC_IN_HW),
		(unsigned int)DPE_RD32(DVP_CORE_CRC_IN_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_13_OCCDV0_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_13_OCCDV0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_14_OCCDV1_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_14_OCCDV1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_15_OCCDV2_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_15_OCCDV2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_16_OCCDV3_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_16_OCCDV3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_18_ASF_RMDV_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_18_ASF_RMDV_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_19_ASF_RDDV_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_19_ASF_RDDV_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_20_ASF_DV0_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_20_ASF_DV0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_21_ASF_DV1_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_21_ASF_DV1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_22_ASF_DV2_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_22_ASF_DV2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_23_ASF_DV3_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_23_ASF_DV3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVP_EXT_SRC_24_WMF_RDDV_HW),
		(unsigned int)DPE_RD32(DVP_EXT_SRC_24_WMF_RDDV_REG));
//DVGF
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_00_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_01_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_02_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_03_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_05_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_05_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_07_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_07_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_IRQ_00_HW),
		(unsigned int)DPE_RD32(DVGF_IRQ_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_IRQ_01_HW),
		(unsigned int)DPE_RD32(DVGF_IRQ_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_STATUS0_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_STATUS0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_STATUS1_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_STATUS1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_IRQ_STATUS_HW),
		(unsigned int)DPE_RD32(DVGF_IRQ_STATUS_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_FRM_STATUS_HW),
		(unsigned int)DPE_RD32(DVGF_FRM_STATUS_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CUR_STATUS_HW),
		(unsigned int)DPE_RD32(DVGF_CUR_STATUS_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_CTRL_HW),
		(unsigned int)DPE_RD32(DVGF_CRC_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_OUT_HW),
		(unsigned int)DPE_RD32(DVGF_CRC_OUT_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_IN_HW),
		(unsigned int)DPE_RD32(DVGF_CRC_IN_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_OUT_0_HW),
		(unsigned int)DPE_RD32(DVGF_CRC_OUT_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_OUT_1_HW),
		(unsigned int)DPE_RD32(DVGF_CRC_OUT_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CRC_OUT_2_HW),
		(unsigned int)DPE_RD32(DVGF_CRC_OUT_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_CRC_IN_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_CRC_IN_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_DRAM_STA_HW),
		(unsigned int)DPE_RD32(DVGF_DRAM_STA_REG));
//
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_DRAM_PITCH_HW),
		(unsigned int)DPE_RD32(DVGF_DRAM_PITCH_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_DRAM_SEC_0_HW),
		(unsigned int)DPE_RD32(DVGF_DRAM_SEC_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_DRAM_SEC_1_HW),
		(unsigned int)DPE_RD32(DVGF_DRAM_SEC_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_DRAM_AXSLC_HW),
		(unsigned int)DPE_RD32(DVGF_DRAM_AXSLC_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_STATUS_32b_00_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_STATUS_32b_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_STATUS_32b_01_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_STATUS_32b_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_STATUS_32b_02_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_STATUS_32b_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_STATUS_32b_03_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_STATUS_32b_03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_STATUS_32b_04_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_STATUS_32b_04_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_RESERVED_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_RESERVED_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CTRL_ATPG_HW),
		(unsigned int)DPE_RD32(DVGF_CTRL_ATPG_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_00_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_01_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_02_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_04_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_04_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_05_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_05_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_06_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_06_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_07_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_07_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_08_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_08_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_09_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_09_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_10_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_10_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_11_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_11_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_12_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_12_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_13_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_13_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_14_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_14_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_15_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_15_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_16_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_16_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_17_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_17_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_18_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_18_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_19_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_19_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_20_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_20_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_21_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_21_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_22_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_22_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_23_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_23_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_SRC_24_HW),
		(unsigned int)DPE_RD32(DVGF_SRC_24_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_00_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_01_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_02_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_03_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_05_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_05_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_06_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_06_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_07_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_07_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_08_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_08_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_09_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_09_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_10_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_10_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_11_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_11_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_12_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_12_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_13_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_13_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_14_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_14_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_15_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_15_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_16_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_16_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_17_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_17_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DVGF_CORE_18_HW),
		(unsigned int)DPE_RD32(DVGF_CORE_18_REG));

//#endif
	LOG_INF("- X.");
	/*  */
	return Ret;
}
#endif
void cmdq_cb_destroy(struct cmdq_cb_data data)
{
	//LOG_INF("%s DPE cmdq_cb_Dump statr", __func__);
	cmdq_pkt_destroy((struct cmdq_pkt *)data.data);
}

static void DPE_callback_work_func(struct work_struct *work)
{
	struct my_callback_data *my_data = NULL;
	struct dpe_deque_done_in_data dpe_in_data;
	unsigned int i = 0;
#if DPE_IRQ_ENABLE
#else
	struct engine_requests *dpe_reqs = NULL;
	struct work_struct *dpe_work = NULL;
	bool bResulst = MFALSE;
	pid_t ProcessID;
	unsigned int p = 0;
#endif

	my_data = container_of(work, struct my_callback_data, cmdq_cb_work);

	if ((my_data->err != 0)) {
		LOG_INF("%s: [ERROR] cb(%p) DPE mode %d timeout with err %d\n",
			__func__, my_data, my_data->dpe_mode, my_data->err);
	} else {
#if DPE_IRQ_ENABLE
		goto EXIT;
#else
		/* Call related IRQ funciton according to dpe_mode */
		if (my_data->dpe_mode == 1) {
			/* DVS */
			dpe_reqs = &dpe_reqs_dvs;
			dpe_work = &DPEInfo.ScheduleDpeWork;
		} else if (my_data->dpe_mode == 3) {
			/* DVGF */
			dpe_reqs = &dpe_reqs_dvgf;
			dpe_work = &DPEInfo.DVGF_ScheduleDpeWork;
		} else {
			/* DVP */
			dpe_reqs = &dpe_reqs_dvp;
			dpe_work = &DPEInfo.DVP_ScheduleDpeWork;
		}

		spin_lock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
		/* Update the frame status. */
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("dpe_irq");
#endif
		if (dpe_update_request_isp8(dpe_reqs, &ProcessID) == 0) {
			bResulst = MTRUE;
		} else {
			LOG_INF("mode %d dpe_update_request bResulst fail = %d\n",
				my_data->dpe_mode, bResulst);
			spin_unlock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
			goto EXIT;
		}

		if (bResulst == MTRUE) {
			#if REQUEST_REGULATION == REQUEST_BASE_REGULATION
			queue_work(DPEInfo.wkqueue, dpe_work);
			#endif
			p = ProcessID % IRQ_USER_NUM_MAX;
			DPEInfo.IrqInfo.Status[DPE_IRQ_TYPE_INT_DVP_ST] |=
				DPE_INT_ST;
			DPEInfo.IrqInfo.ProcessID[p] =
				ProcessID;

			DPEInfo.IrqInfo.DpeIrqCnt[p]++;

			DPEInfo.ProcessID[DPEInfo.WriteReqIdx] = ProcessID;
			DPEInfo.WriteReqIdx =
				(DPEInfo.WriteReqIdx + 1) %
				_SUPPORT_MAX_DPE_FRAME_REQUEST_;
		} else {
			LOG_INF("dvs_update_request_isp8 bResulst fail = %d\n",
			bResulst);
		}
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif

		spin_unlock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
		for (i = 0; i < my_data->token_info.token_cnt; i++) {
			dpe_in_data.req_fd = my_data->token_info.token_index[i];
			LOG_INF("dpe_isp8(%d/%d)\n", my_data->token_info.token_index[i],
				my_data->token_info.token_cnt);
		release_frame_token_DPE(DPE_devs[0].frm_sync_pdev , &dpe_in_data);
		}

		if (bResulst == MTRUE)
			wake_up_interruptible(&DPEInfo.WaitQueueHead);
		#if (REQUEST_REGULATION == FRAME_BASE_REGULATION)
		queue_work(DPEInfo.wkqueue, dpe_work);
		#endif
#endif
	}

EXIT:
	cmdq_pkt_wait_complete(my_data->pkt);
	cmdq_pkt_destroy(my_data->pkt);
	kfree(my_data);
}

void DPE_callback_func(struct cmdq_cb_data data)
{
	struct my_callback_data *my_data = (struct my_callback_data *)data.data;

	my_data->err = data.err;

	if ((my_data->err != 0)) {
		LOG_INF("%s: [ERROR] cb(%p) DPE mode %d timeout with err %d\n",
			__func__, my_data, my_data->dpe_mode, my_data->err);
		if (g_u4EnableClockCount > 0 && !g_isshutdown) {
			LOG_INF("DPE_callback_func 2\n");
			#ifdef CMASYS_CLK_Debug
			LOG_INF("cmd_pkt[0x3A000000 %08X]\n",
			(unsigned int)DPE_RD32(CAMSYS_MAIN_BASE));
			LOG_INF("cmd_pkt[0x3A7A0000 %08X]\n",
			(unsigned int)DPE_RD32(IPE_MAIN_BASE));
			#endif
			DPE_DumpReg();//!test
			// sw token timeout
			mtk_imgsys_frm_sync_timeout(DPE_devs[0].frm_sync_pdev, my_data->pkt->err_data.event);
			// do error handling
			cmdq_dump_pkt(my_data->pkt, 0 , 1);
		} else {
			LOG_INF("DPE Power not Enable or is shutdown(%d)\n", g_isshutdown);
		}
	}

	INIT_WORK(&my_data->cmdq_cb_work, DPE_callback_work_func);
	queue_work(DPEInfo.cmdq_wq, &my_data->cmdq_cb_work);
}

void my_wait(struct my_callback_data *my_data)
{
	cmdq_pkt_wait_complete(my_data->pkt);
	cmdq_pkt_destroy(my_data->pkt);
	kfree(my_data);
}

#define enq_out_data_size 2
signed int CmdqDPEHW(struct frame *frame)
{
	struct DPE_Kernel_Config *pDpeConfig;
	struct DPE_Kernel_Config DpeConfig;
	struct DPE_Config_ISP8 *pDpeUserConfig;
	int result = 0;

	#ifdef CMdq_en
	struct cmdq_pkt *handle;
	#endif
	/*frm sync token variable*/
	struct dpe_in_data enq_in_data;
	struct dpe_out_data enq_out_data[enq_out_data_size] = {0};
	int k = 0;
	int dpe_sw_token_cnt = 0;
	//int ret = 0;
	//int event_type = 0;

	//!
	struct my_callback_data *my_data = kzalloc(sizeof(*my_data), GFP_KERNEL);

	//struct cmdqRecStruct *handle;
	//uint64_t engineFlag = (uint64_t)(1LL << CMDQ_ENG_DPE);
#if defined(DPE_PMQOS_EN) && defined(CONFIG_MTK_QOS_SUPPORT)
	unsigned int w_imgi, h_imgi, w_mvio, h_mvio, w_bvo, h_bvo;
	unsigned int dma_bandwidth, trig_num;
#endif
	unsigned int frame_w = 0, frame_h = 0;
	unsigned int rdma_bandwidth = 0, wdma_bandwidth = 0;
	//int cmd_cnt = 0;

	//LOG_INF("%s CmdqtoHw start", __func__);

	if (g_isshutdown) {
		LOG_INF("%s : system is shutdown: %d", __func__, g_isshutdown);
		kfree((struct my_callback_data *)my_data);
		return -1;
	}

	if (frame == NULL || frame->data == NULL || my_data == NULL) {
		LOG_INF("frame->data = NULL or my_date = NULL");
		kfree((struct my_callback_data *)my_data);
		return -1;
	}

	pDpeUserConfig = (struct DPE_Config_ISP8 *) frame->data;

	pDpeConfig = &DpeConfig;

	/**/

	enq_in_data.req_fd = pDpeUserConfig->req_fd;
	enq_in_data.req_no = pDpeUserConfig->req_no;
	enq_in_data.frm_no = pDpeUserConfig->frm_no;
	enq_in_data.frm_owner = 0x6666;
	enq_in_data.imgstm_inst = 0x5555;

	for (k = 0;k < enq_out_data_size;k++) {
		if (pDpeUserConfig->DPE_Token_Info[k].token_id != 0) {
			if (pDpeUserConfig->DPE_Token_Info[k].d_token == token_wait ||
			pDpeUserConfig->DPE_Token_Info[k].d_token == token_set) {
				enq_in_data.token_info.mSyncTokenList[0].token_value =
					pDpeUserConfig->DPE_Token_Info[k].token_id;
				enq_in_data.token_info.mSyncTokenList[0].type =
				pDpeUserConfig->DPE_Token_Info[k].d_token == token_wait ?
					imgsys_token_wait : imgsys_token_set;
				dpe_sw_token_cnt = 1;
				//event_type = token_wait;
				enq_in_data.token_info.mSyncTokenNum = dpe_sw_token_cnt;
				result = Handler_frame_token_sync_DPE(
					DPE_devs[0].frm_sync_pdev,
					&enq_in_data, &enq_out_data[k]);

				LOG_INF("%s-%d-%d-%d-%d-%d", __func__,
				k,
				pDpeUserConfig->DPE_Token_Info[k].token_id,
				pDpeUserConfig->DPE_Token_Info[k].d_token,
				enq_in_data.token_info.mSyncTokenNum,
				enq_out_data[k].event_id);
			} else {
				LOG_ERR("not support event");
			}
		}

		//LOG_INF("dpe-Handler_frame_token_sync_DPE(%d/%d)", enq_in_data.token_info.mSyncTokenNum,
		//	enq_out_data[k].event_id);
	}

/************** Pass User info to DPE_Kernel_Config **************/

	if (pDpeUserConfig->Dpe_engineSelect == MODE_DVS_DVP_BOTH) {
		result = DPE_Config_DVS(pDpeUserConfig, pDpeConfig);
		if (result != 0)
			return -1;
		result = DPE_Config_DVP(pDpeUserConfig, pDpeConfig);
		if (result != 0)
			return -1;
		pDpeConfig->DPE_MODE = 0;
	} else if (pDpeUserConfig->Dpe_engineSelect == MODE_DVS_ONLY) {
		if (pDpeUserConfig->Dpe_DVSSettings.frm_width == 0) {
			LOG_ERR("DVS frm_width =%d ", pDpeUserConfig->Dpe_DVSSettings.frm_width);
			return -1;
		}

		result = DPE_Config_DVS(pDpeUserConfig, pDpeConfig);
		if (result != 0)
			return -1;
		pDpeConfig->DPE_MODE = 1;

		frame_w = pDpeUserConfig->Dpe_DVSSettings.frm_width;
		frame_h = pDpeUserConfig->Dpe_DVSSettings.frm_height;
		rdma_bandwidth = (((frame_w * frame_h * 2) + (frame_w * frame_h / 4)) * 2) * 30 / 1000000;
		wdma_bandwidth = ((frame_w * frame_h * 2) + (frame_w * frame_h / 4 * 2)) * 30 / 1000000;
	} else if (pDpeUserConfig->Dpe_engineSelect == MODE_DVP_ONLY) {
		if (pDpeUserConfig->Dpe_DVPSettings.frm_width == 0) {
			LOG_ERR("DVP frm_width =%d ", pDpeUserConfig->Dpe_DVPSettings.frm_width);
			return -1;
		}
		result = DPE_Config_DVP(pDpeUserConfig, pDpeConfig);
		if (result != 0)
			return -1;
		pDpeConfig->DPE_MODE = 2;

		frame_w = pDpeUserConfig->Dpe_DVPSettings.frm_width;
		frame_h = pDpeUserConfig->Dpe_DVPSettings.frm_height;
		rdma_bandwidth = ((frame_w * frame_h * 2) + (frame_w * frame_h / 2)) * 30 / 1000000;
		wdma_bandwidth = frame_w * frame_h * 30 / 1000000;

	} else if (pDpeUserConfig->Dpe_engineSelect == MODE_DVGF_ONLY) {
		if (pDpeUserConfig->Dpe_DVGFSettings.frm_width == 0) {
			LOG_ERR("DVGF frm_width =%d ", pDpeUserConfig->Dpe_DVGFSettings.frm_width);
			return -1;
		}
		result = DPE_Config_DVGF(pDpeUserConfig, pDpeConfig);
		if (result != 0)
			return -1;
		pDpeConfig->DPE_MODE = 3;

		frame_w = pDpeUserConfig->Dpe_DVGFSettings.frm_width;
		frame_h = pDpeUserConfig->Dpe_DVGFSettings.frm_height;
		rdma_bandwidth = ((frame_w * frame_h * 5) + (frame_w * frame_h / 2 * 2)) * 30 / 1000000;
		wdma_bandwidth = frame_w * frame_h * 2 * 30 / 1000000;
	}

	if (DPE_debug_log_en == 1) {
		LOG_INF("%s CmdqtoHw dumpreg before add gce cmd", __func__);
		DPE_DumptoHWReg(pDpeConfig);
	}
	//if (g_isDPELogEnable)
	//cmdqRecCreate(CMDQ_SCENARIO_ISP_DPE, &handle);
	//cmdqRecSetEngine(handle, engineFlag);
	//cmdq_pkt_cl_create(&handle, dpe_clt);
#ifdef CMdq_en
	handle = cmdq_pkt_create(dpe_clt);
#endif
#define CMDQWR(REG) \
	cmdq_pkt_write(handle, dpe_clt_base, \
	REG ##_HW, pDpeConfig->REG, CMDQ_REG_MASK)
#define CMDQWR_DPE_DRAM_ADDR(REG) \
	cmdq_pkt_write(handle, dpe_clt_base, \
	REG ##_HW, (pDpeConfig->REG)>>4, CMDQ_REG_MASK)
#define CMDQWR_DPE_ME_ADDR(REG) \
	cmdq_pkt_write(handle, dpe_clt_base, \
	REG ##_HW, pDpeConfig->TuningBuf_ME.REG, CMDQ_REG_MASK)
#define CMDQWR_DPE_OCC_ADDR(REG) \
	cmdq_pkt_write(handle, dpe_clt_base, \
	REG ##_HW, pDpeConfig->TuningKernel_OCC.REG, CMDQ_REG_MASK)
#define CMDQWR_DPE_DVP_ADDR(REG) \
	cmdq_pkt_write(handle, dpe_clt_base, \
	REG ##_HW, pDpeConfig->TuningKernel_DVP.REG, CMDQ_REG_MASK)
#define CMDQWR_DPE_DVGF_ADDR(REG) \
	cmdq_pkt_write(handle, dpe_clt_base, \
	REG ##_HW, pDpeConfig->TuningKernel_DVGF.REG, CMDQ_REG_MASK)
//LOG_INF("pDpeConfig->DPE_MODE = %x\n", pDpeConfig->DPE_MODE);

if ((pDpeConfig->DPE_MODE != 2) && (pDpeConfig->DPE_MODE != 3)) {
	if (pDpeConfig->DPE_MODE == 1) {
		/* DVS Only Mode*/
		/* dvp_en = 1 */
		#ifdef CMdq_en
		cmdq_pkt_write(handle, dpe_clt_base,
		DVP_CTRL00_HW, 0x80000000, 0x80000000);
		#endif
	}
	/* mask trigger bit */
	#ifdef CMdq_en
	cmdq_pkt_write(handle, dpe_clt_base,
	DVS_CTRL00_HW, pDpeConfig->DVS_CTRL00, 0xDFF5FC00);//0xDBF5FC00
	cmdq_pkt_write(handle, dpe_clt_base,
	DVS_CTRL02_HW, 0x70310001, CMDQ_REG_MASK);
	#endif

	//LOG_INF("mask trigger bit\n");
	/* cmdq_pkt_write(handle, dpe_clt_base, */
	/* DVS_CTRL07_HW, 0x0000FF1F, CMDQ_REG_MASK); */
	/* cmdq_pkt_write(handle, dpe_clt_base, */
	/* DVS_SRC_CTRL_HW, 0x00000040, CMDQ_REG_MASK); */
	/* DVS Frame Done IRQ */
	if (pDpeConfig->DPE_MODE == 1) { // MODE_DVS_ONLY
		//LOG_INF("MODE_DVS_ONLY\n");
		#ifdef CMdq_en
		#if DPE_IRQ_ENABLE
		cmdq_pkt_write(handle, dpe_clt_base,
		DVS_IRQ_00_HW, 0x00000E00, 0x00000F00);
		#else
		cmdq_pkt_write(handle, dpe_clt_base,
		DVS_IRQ_00_HW, 0x00000F00, 0x00000F00);
		#endif
		#endif
	} else { // MODE_DVS_DVP_BOTH
		#ifdef CMdq_en
		cmdq_pkt_write(handle, dpe_clt_base,
		DVS_IRQ_00_HW, 0x00000F00, 0x00000F00);
		#endif
	}
	//LOG_INF("star CMDQWR\n");
	#ifdef CMdq_en
	CMDQWR(DVS_DRAM_PITCH);
	CMDQWR(DVS_DRAM_PITCH1);
	CMDQWR(DVS_SRC_00);
	CMDQWR(DVS_SRC_01);
	CMDQWR(DVS_SRC_02);
	CMDQWR(DVS_SRC_03);
	CMDQWR(DVS_SRC_04);
	CMDQWR_DPE_DRAM_ADDR(DVS_SRC_05_L_FRM0);
	CMDQWR_DPE_DRAM_ADDR(DVS_SRC_06_L_FRM1);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_07_L_FRM2);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_08_L_FRM3);
	CMDQWR_DPE_DRAM_ADDR(DVS_SRC_09_R_FRM0);
	CMDQWR_DPE_DRAM_ADDR(DVS_SRC_10_R_FRM1);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_11_R_FRM2);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_12_R_FRM3);
	CMDQWR_DPE_DRAM_ADDR(DVS_SRC_13_Hist0);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_14_Hist1);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_15_Hist2);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_16_Hist3);
	CMDQWR_DPE_DRAM_ADDR(DVS_SRC_17_OCCDV_EXT0);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_18_OCCDV_EXT1);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_19_OCCDV_EXT2);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_20_OCCDV_EXT3);
	CMDQWR_DPE_DRAM_ADDR(DVS_SRC_21_P4_L_DV_ADR);
	CMDQWR_DPE_DRAM_ADDR(DVS_SRC_22_OCCDV0);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_23_OCCDV1);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_24_OCCDV2);
	//CMDQWR_DPE_DRAM_ADDR(DVS_SRC_25_OCCDV3);
	CMDQWR_DPE_DRAM_ADDR(DVS_SRC_26_P4_R_DV_ADR);
	CMDQWR(DVS_SRC_27);
	CMDQWR(DVS_SRC_28);
	CMDQWR(DVS_SRC_29);
	CMDQWR(DVS_SRC_30);
	CMDQWR(DVS_SRC_31);
	CMDQWR(DVS_SRC_32);
	CMDQWR(DVS_SRC_33);
	CMDQWR(DVS_SRC_34);
	CMDQWR(DVS_SRC_35);
	CMDQWR(DVS_SRC_36);
	CMDQWR(DVS_SRC_37);
	CMDQWR(DVS_SRC_38);
	CMDQWR(DVS_SRC_39);
	CMDQWR(DVS_DRAM_SEC_0);
	CMDQWR(DVS_DRAM_SEC_1);
	CMDQWR(DVS_DRAM_SEC_2);
	CMDQWR(DVS_DRAM_SEC_3);
	// CMDQWR(DVS_DRAM_AXSLC_0);
	// CMDQWR(DVS_DRAM_AXSLC_1);
	CMDQWR(DVS_DEQ_FORCE);
	CMDQWR(DVS_CTRL_RESERVED);
	CMDQWR(DVS_CTRL_ATPG);

	if (pDpeUserConfig->Dpe_DVSSettings.is_pd_mode) {
		//CMDQWR_DPE_DRAM_ADDR(DVS_PD_SRC_04_R_FRM0);
		//CMDQWR_DPE_DRAM_ADDR(DVS_PD_SRC_08_OCCDV0);
		//CMDQWR_DPE_DRAM_ADDR(DVS_PD_SRC_16_DCV_CONF0);
	}
	CMDQWR_DPE_ME_ADDR(DVS_ME_00);
	CMDQWR_DPE_ME_ADDR(DVS_ME_01);
	CMDQWR_DPE_ME_ADDR(DVS_ME_02);
	CMDQWR_DPE_ME_ADDR(DVS_ME_03);
	CMDQWR_DPE_ME_ADDR(DVS_ME_04);
	CMDQWR_DPE_ME_ADDR(DVS_ME_06);
	CMDQWR_DPE_ME_ADDR(DVS_ME_07);
	CMDQWR_DPE_ME_ADDR(DVS_ME_08);
	CMDQWR_DPE_ME_ADDR(DVS_ME_09);
	CMDQWR_DPE_ME_ADDR(DVS_ME_10);
	CMDQWR_DPE_ME_ADDR(DVS_ME_11);
	CMDQWR_DPE_ME_ADDR(DVS_ME_12);
	CMDQWR_DPE_ME_ADDR(DVS_ME_13);
	CMDQWR_DPE_ME_ADDR(DVS_ME_14);
	CMDQWR_DPE_ME_ADDR(DVS_ME_15);
	CMDQWR_DPE_ME_ADDR(DVS_ME_16);
	CMDQWR_DPE_ME_ADDR(DVS_ME_17);
	CMDQWR_DPE_ME_ADDR(DVS_ME_18);
	CMDQWR_DPE_ME_ADDR(DVS_ME_19);
	CMDQWR_DPE_ME_ADDR(DVS_ME_20);
	CMDQWR_DPE_ME_ADDR(DVS_ME_21);
	CMDQWR_DPE_ME_ADDR(DVS_ME_22);
	CMDQWR_DPE_ME_ADDR(DVS_ME_23);
	CMDQWR_DPE_ME_ADDR(DVS_ME_24);
	CMDQWR_DPE_ME_ADDR(DVS_ME_25);
	CMDQWR_DPE_ME_ADDR(DVS_ME_26);
	CMDQWR_DPE_ME_ADDR(DVS_ME_27);
	CMDQWR_DPE_ME_ADDR(DVS_ME_28);
	CMDQWR_DPE_ME_ADDR(DVS_ME_29);
	CMDQWR_DPE_ME_ADDR(DVS_ME_30);
	CMDQWR_DPE_ME_ADDR(DVS_ME_31);
	CMDQWR_DPE_ME_ADDR(DVS_ME_32);
	CMDQWR_DPE_ME_ADDR(DVS_ME_33);
	CMDQWR_DPE_ME_ADDR(DVS_ME_34);
	CMDQWR_DPE_ME_ADDR(DVS_ME_35);
	CMDQWR_DPE_ME_ADDR(DVS_ME_36);
	CMDQWR_DPE_ME_ADDR(DVS_ME_37);
	CMDQWR_DPE_ME_ADDR(DVS_ME_38);
	CMDQWR_DPE_ME_ADDR(DVS_ME_39);
	CMDQWR_DPE_ME_ADDR(DVS_DEBUG);
	CMDQWR_DPE_ME_ADDR(DVS_ME_RESERVED);
	CMDQWR_DPE_ME_ADDR(DVS_ME_ATPG);
	CMDQWR_DPE_ME_ADDR(DVS_ME_40);
	CMDQWR_DPE_ME_ADDR(DVS_ME_41);
	CMDQWR_DPE_ME_ADDR(DVS_ME_42);
	CMDQWR_DPE_ME_ADDR(DVS_ME_43);
	CMDQWR_DPE_ME_ADDR(DVS_ME_44);
	CMDQWR_DPE_ME_ADDR(DVS_ME_45);
	CMDQWR_DPE_ME_ADDR(DVS_ME_46);
	CMDQWR_DPE_ME_ADDR(DVS_ME_47);
	CMDQWR_DPE_ME_ADDR(DVS_ME_48);
	CMDQWR_DPE_ME_ADDR(DVS_ME_49);
	CMDQWR_DPE_ME_ADDR(DVS_ME_50);
	CMDQWR_DPE_ME_ADDR(DVS_ME_51);
	CMDQWR_DPE_ME_ADDR(DVS_ME_52);
	CMDQWR_DPE_ME_ADDR(DVS_ME_53);
	CMDQWR_DPE_ME_ADDR(DVS_ME_54);
	CMDQWR_DPE_ME_ADDR(DVS_ME_55);
	CMDQWR_DPE_ME_ADDR(DVS_ME_56);
	CMDQWR_DPE_ME_ADDR(DVS_ME_57);
	CMDQWR_DPE_ME_ADDR(DVS_ME_58);
	CMDQWR_DPE_ME_ADDR(DVS_ME_59);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_PQ_0);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_PQ_1);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_PQ_2);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_PQ_3);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_PQ_4);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_PQ_5);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_PQ_10);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_PQ_11);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_PQ_12);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_ATPG);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST0);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST1);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST2);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST3);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST4);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST5);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST6);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST7);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST8);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST9);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST10);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST11);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST12);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST13);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST14);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST15);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST16);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST17);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_HIST18);
	CMDQWR_DPE_OCC_ADDR(DVS_OCC_LDV0);
	#endif
/* CRC EN, CRC SEL =0x0 */
/* cmdq_pkt_write(handle, dpe_clt_base, */
/* DVS_CRC_CTRL_HW, 0x00000001, 0x00000F01); */
/* CRC CLEAR  = 1 */
/* cmdq_pkt_write(handle, dpe_clt_base, */
/* DVS_CRC_CTRL_HW, 0x00000010, 0x00000010); */
/* CRC CLEAR  = 0 */
/* cmdq_pkt_write(handle, dpe_clt_base, */
/* DVS_CRC_CTRL_HW, 0x00000000, 0x00000010); */
}
	/*================= DVP Settings ==================*/
if ((pDpeConfig->DPE_MODE != 1) && (pDpeConfig->DPE_MODE != 3)) {
	if (pDpeConfig->DPE_MODE == 2) {
		/* DVP Only Mode*/
		/* dvs_en = 1, DPE FW Tri En = 1, */
		/* dvp_trig_en = 1, dvs_trig_en = 0  */
		#ifdef CMdq_en
		cmdq_pkt_write(handle, dpe_clt_base,
		DVS_CTRL00_HW, 0xC8000000, 0xDC300000);
		cmdq_pkt_write(handle, dpe_clt_base,
		DVS_CTRL02_HW, 0x70310001, CMDQ_REG_MASK);
		#endif
	}
	#ifdef CMdq_en
	cmdq_pkt_write(handle, dpe_clt_base,
	DVP_CTRL00_HW, 0x80000080, CMDQ_REG_MASK);
	cmdq_pkt_write(handle, dpe_clt_base,
	DVP_CTRL02_HW, 0x70310001, CMDQ_REG_MASK);
	/* cmdq_pkt_write(handle, dpe_clt_base, */
	/* DVP_CTRL07_HW, 0x00000707, CMDQ_REG_MASK); */
	/* DVP Frame Done IRQ */
	#if DPE_IRQ_ENABLE
	cmdq_pkt_write(handle, dpe_clt_base,
	DVP_IRQ_00_HW, 0x00000E00, 0x00000F00);
	#else
	cmdq_pkt_write(handle, dpe_clt_base,
	DVP_IRQ_00_HW, 0x00000F00, 0x00000F00);
	#endif
	//LOG_INF("star CMDQWR DVP Settings\n");


	CMDQWR(DVP_DRAM_PITCH);
	CMDQWR(DVP_SRC_00);
	CMDQWR(DVP_SRC_01);
	CMDQWR(DVP_SRC_02);
	CMDQWR(DVP_SRC_03);
	CMDQWR(DVP_SRC_04);
	CMDQWR_DPE_DRAM_ADDR(DVP_SRC_05_Y_FRM0);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_06_Y_FRM1);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_07_Y_FRM2);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_08_Y_FRM3);
	CMDQWR_DPE_DRAM_ADDR(DVP_SRC_09_C_FRM0);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_10_C_FRM1);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_11_C_FRM2);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_12_C_FRM3);
	CMDQWR_DPE_DRAM_ADDR(DVP_SRC_13_OCCDV0);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_14_OCCDV1);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_15_OCCDV2);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_16_OCCDV3);
	CMDQWR_DPE_DRAM_ADDR(DVP_SRC_17_CRM);
	CMDQWR_DPE_DRAM_ADDR(DVP_SRC_18_ASF_RMDV);
	CMDQWR_DPE_DRAM_ADDR(DVP_SRC_19_ASF_RDDV);
	CMDQWR_DPE_DRAM_ADDR(DVP_SRC_20_ASF_DV0);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_21_ASF_DV1);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_22_ASF_DV2);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_23_ASF_DV3);
	CMDQWR_DPE_DRAM_ADDR(DVP_SRC_24_WMF_RDDV);
	CMDQWR_DPE_DRAM_ADDR(DVP_SRC_25_WMF_HFDV);
	CMDQWR_DPE_DRAM_ADDR(DVP_SRC_26_WMF_DV0);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_27_WMF_DV1);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_28_WMF_DV2);
	//CMDQWR_DPE_DRAM_ADDR(DVP_SRC_29_WMF_DV3);
	CMDQWR_DPE_DRAM_ADDR(DVP_EXT_SRC_13_OCCDV0);
	//CMDQWR_DPE_DRAM_ADDR(DVP_EXT_SRC_14_OCCDV1);
	//CMDQWR_DPE_DRAM_ADDR(DVP_EXT_SRC_15_OCCDV2);
	//CMDQWR_DPE_DRAM_ADDR(DVP_EXT_SRC_16_OCCDV3);
	CMDQWR_DPE_DRAM_ADDR(DVP_EXT_SRC_18_ASF_RMDV);
	CMDQWR_DPE_DRAM_ADDR(DVP_EXT_SRC_19_ASF_RDDV);
	CMDQWR_DPE_DRAM_ADDR(DVP_EXT_SRC_20_ASF_DV0);
	//CMDQWR_DPE_DRAM_ADDR(DVP_EXT_SRC_21_ASF_DV1);
	//CMDQWR_DPE_DRAM_ADDR(DVP_EXT_SRC_22_ASF_DV2);
	//CMDQWR_DPE_DRAM_ADDR(DVP_EXT_SRC_23_ASF_DV3);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_00);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_01);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_02);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_03);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_04);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_05);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_06);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_07);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_08);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_09);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_10);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_11);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_12);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_13);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_14);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_15);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_16);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_17);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_18);
	CMDQWR_DPE_DVP_ADDR(DVP_CORE_19);
	#endif
/*CRC EN, CRC SEL = 0x1000*/
/* cmdq_pkt_write(handle, dpe_clt_base, */
/* DVP_CRC_CTRL_HW, 0x00000801, 0x00000F01); */
/* CRC CLEAR  = 1 */
/* cmdq_pkt_write(handle, dpe_clt_base, */
/* DVP_CRC_CTRL_HW, 0x00000010, 0x00000010); */
/* CRC CLEAR  = 0 */
/* cmdq_pkt_write(handle, dpe_clt_base, */
/* DVP_CRC_CTRL_HW, 0x00000000, 0x00000010); */
}
/*================= DVGF Settings ==================*/
if (pDpeConfig->DPE_MODE == 3) {

	//LOG_INF("star DVGF CMDQWR\n");
	#ifdef CMdq_en
	cmdq_pkt_write(handle, dpe_clt_base,
	DVS_CTRL00_HW, 0xC4040000, 0xDC040000); //dvgf_trig_en

	//cmdq_pkt_write(handle, dpe_clt_base,
	//DVGF_CTRL_00_HW, 0x80000000, CMDQ_REG_MASK);
	//cmdq_pkt_write(handle, dpe_clt_base,
	//DVGF_CTRL_02_HW, 0x70310001, CMDQ_REG_MASK);

	#if DPE_IRQ_ENABLE
	cmdq_pkt_write(handle, dpe_clt_base,
	DVGF_IRQ_00_HW, 0x00000E00, 0x00000F00);
	#else
	cmdq_pkt_write(handle, dpe_clt_base,
	DVGF_IRQ_00_HW, 0x00000F00, 0x00000F00);
	#endif

	cmdq_pkt_write(handle, dpe_clt_base,
	DVGF_CTRL_00_HW, 0x80000000, 0x80000000);

	///CMDQWR(DVGF_CTRL_00);
	///CMDQWR(DVGF_CTRL_01);
	CMDQWR(DVGF_CTRL_02);
	CMDQWR(DVGF_CTRL_03);
	CMDQWR(DVGF_CTRL_05);
	CMDQWR(DVGF_CTRL_07);
	//CMDQWR(DVGF_IRQ_00);
	CMDQWR(DVGF_DRAM_PITCH);
	CMDQWR(DVGF_SRC_00);
	CMDQWR(DVGF_SRC_01);
	CMDQWR(DVGF_SRC_02);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_04);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_05);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_06);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_07);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_08);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_09);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_10);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_11);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_12);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_13);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_14);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_15);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_16);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_17);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_18);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_19);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_20);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_21);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_22);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_23);
	CMDQWR_DPE_DRAM_ADDR(DVGF_SRC_24);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_00);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_01);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_02);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_03);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_05);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_06);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_07);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_08);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_09);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_10);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_11);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_12);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_13);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_14);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_15);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_16);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_17);
	CMDQWR_DPE_DVGF_ADDR(DVGF_CORE_18);
	#endif
}
/* DPE FW Tri = 1*/
#ifdef CMdq_en
//need to decide notify or wait or do nothing
for (k = 0;k < enq_out_data_size;k++) {
	if (pDpeUserConfig->DPE_Token_Info[k].token_id && pDpeUserConfig->DPE_Token_Info[k].d_token == token_wait) {
		LOG_INF("%s-%d-%d-%d-%d-%d-dpe_isp8-wait", __func__,
			k,
			pDpeUserConfig->DPE_Token_Info[k].token_id,
			pDpeUserConfig->DPE_Token_Info[k].d_token,
			enq_in_data.token_info.mSyncTokenNum,
			enq_out_data[k].event_id);
		cmdq_pkt_wfe(handle, enq_out_data[k].event_id);
	}
}
cmdq_pkt_write(handle, dpe_clt_base, DVS_CTRL00_HW, 0x20000000, 0x20000000);
//LOG_INF("DPE FW Tri = %x\n", pDpeConfig->DVS_CTRL00);
	if (pDpeConfig->DPE_MODE == 1) /* DVS ONLY MODE */
		cmdq_pkt_wfe(handle, dvs_event_id);
	else if (pDpeConfig->DPE_MODE == 3)
		cmdq_pkt_wfe(handle, dvgf_event_id);
	else
		cmdq_pkt_wfe(handle, dvp_event_id);
for (k = 0;k < enq_out_data_size;k++) {
	if (pDpeUserConfig->DPE_Token_Info[k].token_id && pDpeUserConfig->DPE_Token_Info[k].d_token == token_set) {
		LOG_INF("%s-%d-%d-%d-%d-%d-%d-dpe_isp8-set", __func__,
			k,
			pDpeUserConfig->DPE_Token_Info[k].token_id,
			pDpeUserConfig->DPE_Token_Info[k].d_token,
			enq_in_data.token_info.mSyncTokenNum,
			enq_out_data[k].event_id,
			enq_out_data[k].req_fd);
		cmdq_pkt_set_event(handle, enq_out_data[k].event_id);
	}
}
//cmdq_pkt_write(handle, dpe_clt_base, DVS_CTRL00_HW, 0x00000000, 0x20000000);
#endif
#if defined(DPE_PMQOS_EN) && defined(CONFIG_MTK_QOS_SUPPORT)
	LOG_INF("DPE_PMQOS_EN =1\n");
	trig_num = (pDpeConfig->DPE_CTRL & 0x00000F00) >> 8;
	w_imgi = pDpeConfig->DPE_SIZE & 0x000001FF;
	h_imgi = (pDpeConfig->DPE_SIZE & 0x01FF0000) >> 16;
	w_mvio = ((w_imgi + 1) >> 1) - 1;
	w_mvio = ((w_mvio / 7) << 4) + (((((w_mvio % 7) + 1) * 18) + 7) >> 3);
	h_mvio = (h_imgi + 1) >> 1;
	w_bvo =  (w_imgi + 1) >> 1;
	h_bvo =  (h_imgi + 1) >> 1;
	dma_bandwidth = ((w_imgi * h_imgi) * 2 + (w_mvio * h_mvio) * 2 * 16 +
			(w_bvo * h_bvo)) * trig_num * 30 / 1000000;
	cmdq_task_update_property(handle, &dma_bandwidth, sizeof(unsigned int));
#endif

	// MMQOS set bw
	if (pDpeConfig->DPE_MODE == 1) /* DVS ONLY MODE */ {
		if (g_dvs_rdma_ttl_bw == 0 || g_dvs_wdma_ttl_bw == 0) {
			g_dvs_rdma_ttl_bw = (unsigned int)(rdma_bandwidth);
			if (icc_path_dpe_rdma[0]) {
				mtk_icc_set_bw(icc_path_dpe_rdma[0],
					(int)(rdma_bandwidth*1000), 0);
			}
			g_dvs_wdma_ttl_bw = (unsigned int)(wdma_bandwidth);
			if (icc_path_dpe_wdma[0]) {
				mtk_icc_set_bw(icc_path_dpe_wdma[0],
					(int)(wdma_bandwidth*1000), 0);
			}
			// larb19 setting
			if (DPE_devs[0].dev_ver == 0) {
				mtk_cam_bwr_set_chn_bw(dpe_bwr_device, ENGINE_DPE, DISP_PORT,
					(int)(g_dvs_rdma_ttl_bw), (int)(g_dvs_wdma_ttl_bw),
					0, 0, false);
				mtk_cam_bwr_set_ttl_bw(dpe_bwr_device, ENGINE_DPE,
					(int)(g_dvs_rdma_ttl_bw + g_dvs_wdma_ttl_bw), 0, false);
			}
		}
	} else if (pDpeConfig->DPE_MODE == 3) {
		if (g_dvgf_rdma_ttl_bw == 0 || g_dvgf_wdma_ttl_bw == 0) {
			g_dvgf_rdma_ttl_bw = (unsigned int)(rdma_bandwidth);
			if (icc_path_dpe_rdma[2]) {
				mtk_icc_set_bw(icc_path_dpe_rdma[2],
					(int)(rdma_bandwidth*1000), 0);
			}
			g_dvgf_wdma_ttl_bw = (unsigned int)(wdma_bandwidth);
			if (icc_path_dpe_wdma[2]) {
				mtk_icc_set_bw(icc_path_dpe_wdma[2],
					(int)(wdma_bandwidth*1000), 0);
			}
			// larb19 setting
			if (DPE_devs[0].dev_ver == 0) {
				mtk_cam_bwr_set_chn_bw(dpe_bwr_device, ENGINE_DPE, DISP_PORT,
					(int)(g_dvgf_rdma_ttl_bw), (int)(g_dvgf_wdma_ttl_bw),
					0, 0, false);
				mtk_cam_bwr_set_ttl_bw(dpe_bwr_device, ENGINE_DPE,
					(int)(g_dvgf_rdma_ttl_bw + g_dvgf_wdma_ttl_bw), 0, false);
			}
		}
	} else {
		if (g_dvp_rdma_ttl_bw == 0 || g_dvp_wdma_ttl_bw == 0) {
			g_dvp_rdma_ttl_bw = (unsigned int)(rdma_bandwidth);
			if (icc_path_dpe_rdma[1]) {
				mtk_icc_set_bw(icc_path_dpe_rdma[1],
					(int)(rdma_bandwidth*1000), 0);
			}
			g_dvp_wdma_ttl_bw = (unsigned int)(wdma_bandwidth);
			if (icc_path_dpe_wdma[1]) {
				mtk_icc_set_bw(icc_path_dpe_wdma[1],
					(int)(wdma_bandwidth*1000), 0);
			}
			// larb19 setting
			if (DPE_devs[0].dev_ver == 0) {
				mtk_cam_bwr_set_chn_bw(dpe_bwr_device, ENGINE_DPE, DISP_PORT,
					(int)(g_dvp_rdma_ttl_bw), (int)(g_dvp_wdma_ttl_bw),
					0, 0, false);
				mtk_cam_bwr_set_ttl_bw(dpe_bwr_device, ENGINE_DPE,
					(int)(g_dvp_rdma_ttl_bw + g_dvp_wdma_ttl_bw), 0, false);
			}
		}
	}

	/* non-blocking API, Please  use cmdqRecFlushAsync() */
	//cmdq_task_flush_async_destroy(handle);
	/* flush and destroy in cmdq */
	//LOG_INF("cmd_pkt start\n");

	my_data->pkt = handle;
	my_data->dpe_mode = pDpeConfig->DPE_MODE;
	for (k = 0;k < enq_out_data_size;k++) {
		if (enq_out_data[k].event_id != 0) {
			my_data->token_info.token_index[k] = enq_out_data[k].req_fd;
			my_data->token_info.token_cnt += 1;
		}
	}

	LOG_INF("%s-%p\n", __func__, my_data);

	cmdq_pkt_flush_async(handle, DPE_callback_func, (void *)my_data);
	//my_wait(my_data);

//#ifdef CMdq_en
//	cmdq_pkt_flush_threaded(handle,
//	cmdq_cb_destroy, (void *)handle);
//#endif
//LOG_INF("cmd_pkt end\n");
	return 0;
}
signed int dpe_feedback(struct frame *frame)
{
	struct DPE_Config_ISP8 *pDpeConfig;

	pDpeConfig = (struct DPE_Config_ISP8 *) frame->data;
	/* TODO: read statistics and write to the frame data */
	// pDpeConfig->DVS_IRQ_STATUS = DPE_RD32(DVS_IRQ_STATUS_REG);
	return 0;
}
static const struct engine_ops dpe_ops = {
	.req_enque_cb = dpe_enque_cb,
	.req_deque_cb = dpe_deque_cb,
	.frame_handler = CmdqDPEHW,
	.req_feedback_cb = dpe_feedback,
};
#if defined(DPE_PMQOS_EN) && defined(CONFIG_MTK_QOS_SUPPORT)
void cmdq_pm_qos_start(struct TaskStruct *task, struct TaskStruct *task_list[],
								u32 size)
{
	unsigned int dma_bandwidth;

	dma_bandwidth = *(unsigned int *) task->prop_addr;
	pm_qos_update_request(&dpe_pm_qos_request, dma_bandwidth);
	LOG_INF("+ PMQOS Bandwidth : %d MB/sec\n", dma_bandwidth);
}
void cmdq_pm_qos_stop(struct TaskStruct *task, struct TaskStruct *task_list[],
								u32 size)
{
	pm_qos_update_request(&dpe_pm_qos_request, 0);
	LOG_DBG("- PMQOS Bandwidth : %d\n", 0);
}
#endif

void dpe_mmqos_init(struct device *pdev)
{
	int i = 0;

	// get interconnect path for MMQOS
	for (i = 0; i < DPE_MMQOS_RDMA_NUM; ++i) {
		LOG_INF("rdma index: %d, mmqos name: %s\n", i, mmqos_dpe_names_rdma[i]);
		icc_path_dpe_rdma[i] = of_mtk_icc_get(pdev, mmqos_dpe_names_rdma[i]);
	}
	for (i = 0; i < DPE_MMQOS_WDMA_NUM; ++i) {
		LOG_INF("wdma index: %d, mmqos name: %s\n", i, mmqos_dpe_names_wdma[i]);
		icc_path_dpe_wdma[i] = of_mtk_icc_get(pdev, mmqos_dpe_names_wdma[i]);
	}

	g_dvs_rdma_ttl_bw = 0;
	g_dvs_wdma_ttl_bw = 0;
	g_dvp_rdma_ttl_bw = 0;
	g_dvp_wdma_ttl_bw = 0;
	g_dvgf_rdma_ttl_bw = 0;
	g_dvgf_wdma_ttl_bw = 0;
}

unsigned int Compute_Para(struct DPE_Config_ISP8 *pDpeConfig,
	unsigned int tile_occ_width)
{
	unsigned int w_width; //!full_tile_width
	unsigned int tile_num = MAX_NUM_TILE;
	unsigned int egn_st_x = pDpeConfig->Dpe_DVSSettings.l_eng_start_x;

	w_width = (tile_num*tile_occ_width)+(2*egn_st_x);
	while (w_width > pDpeConfig->Dpe_DVSSettings.dram_pxl_pitch) {
		tile_num = tile_num - 1;
		w_width = (tile_num*tile_occ_width)+(2*egn_st_x);
	}
	if (tile_num > 0) {
		pDpeConfig->Dpe_DVSSettings.pd_frame_num = tile_num;
		return w_width;
	}
	pDpeConfig->Dpe_DVSSettings.pd_frame_num = 1;
	return 0;
}

void Get_Tile_Info(struct DPE_Config_ISP8 *pDpeConfig)
{
	unsigned int tile_occ_width[TILE_WITH_NUM] = {640, 512, 384};
	unsigned int w_width[TILE_WITH_NUM] = {0};
	unsigned int tile_num[TILE_WITH_NUM] = {0};
	unsigned int idx = 0, i = 0;
	unsigned int max_width = 0, interval = 0, st_x = 0;
	unsigned int engStart_x_L, engStart_x_R, frmHeight;
	unsigned int engWidth;

	engStart_x_L = pDpeConfig->Dpe_DVSSettings.l_eng_start_x;
	engStart_x_R = pDpeConfig->Dpe_DVSSettings.r_eng_start_x;
	frmHeight = pDpeConfig->Dpe_DVSSettings.frm_height;
	engWidth = pDpeConfig->Dpe_DVSSettings.eng_width;
#if IS_ENABLED(CONFIG_MTK_LEGACY)
	if (pDpeConfig->Dpe_DVSSettings.dram_pxl_pitch <
			(tile_occ_width[TILE_WITH_NUM-1]+(2*engStart_x_L))) {
		LOG_ERR("Frame size [%d] is smaller than 384\n",
		pDpeConfig->Dpe_DVSSettings.dram_pxl_pitch);
		pDpeConfig->Dpe_DVSSettings.pd_frame_num = 1;
	} else {
		for (i = 0; i < TILE_WITH_NUM; i++) {
			w_width[i] =
			Compute_Para(pDpeConfig, tile_occ_width[i]);
			tile_num[i] =
			pDpeConfig->Dpe_DVSSettings.pd_frame_num;
			if (w_width[i] > max_width) {
				max_width = w_width[i];
				idx = i;
			}
		}

			interval = (pDpeConfig->Dpe_DVSSettings.dram_pxl_pitch - w_width[idx])/2;
			st_x = ((interval%16) == 0) ? (interval) : ((interval/16)*16);
			//pDpeConfig->Dpe_DVSSettings.eng_start_y = 0;
			pDpeConfig->Dpe_DVSSettings.frm_width =
			tile_occ_width[idx] + (2*engStart_x_L);
			pDpeConfig->Dpe_DVSSettings.eng_width =
			pDpeConfig->Dpe_DVSSettings.frm_width -
			engStart_x_L - engStart_x_R;
			pDpeConfig->Dpe_DVSSettings.eng_height
			= frmHeight -
			(2*pDpeConfig->Dpe_DVSSettings.eng_start_y);
			pDpeConfig->Dpe_DVSSettings.occ_width = tile_occ_width[idx];
			pDpeConfig->Dpe_DVSSettings.occ_start_x = engStart_x_L;
			pDpeConfig->Dpe_DVSSettings.pd_frame_num = tile_num[idx];
#if defined(UT_CASE)
		pDpeConfig->Dpe_DVSSettings.pd_st_x = 0;
#else
		pDpeConfig->Dpe_DVSSettings.pd_st_x = st_x;
#endif
	}
#else
	//!ISP7 tile mode
	if (pDpeConfig->Dpe_DVSSettings.dram_pxl_pitch <
			(tile_occ_width[TILE_WITH_NUM-1]+(2*engStart_x_L))) {
		LOG_ERR("Frame size [%d] is smaller than 384\n",
		pDpeConfig->Dpe_DVSSettings.dram_pxl_pitch);
		pDpeConfig->Dpe_DVSSettings.pd_frame_num = 1;
	} else {//!ISP7 tile mode
		for (i = 0; i < TILE_WITH_NUM; i++) {
			w_width[i] =
			Compute_Para(pDpeConfig, tile_occ_width[i]);
			LOG_INF("a w_width[%d] = %d\n", i, w_width[i]);
			tile_num[i] =
			pDpeConfig->Dpe_DVSSettings.pd_frame_num;
			LOG_INF("a tile_num[%d] = %d\n", i, tile_num[i]);
			if (w_width[i] > max_width) {
				max_width = w_width[i];
				idx = i;
			}
		}

		interval = (pDpeConfig->Dpe_DVSSettings.dram_pxl_pitch - w_width[idx])/2;
		st_x = ((interval%16) == 0) ? (interval) : ((interval/16)*16);
		pDpeConfig->Dpe_DVSSettings.frm_width = engWidth; //!ISP7
		//!pDpeConfig->Dpe_DVSSettings.eng_width =
		//!pDpeConfig->Dpe_DVSSettings.frm_width -
		//!engStart_x_L - engStart_x_R;
		pDpeConfig->Dpe_DVSSettings.eng_height
		= frmHeight -
		(2*pDpeConfig->Dpe_DVSSettings.eng_start_y);
		pDpeConfig->Dpe_DVSSettings.occ_width = tile_occ_width[idx];
		pDpeConfig->Dpe_DVSSettings.occ_start_x = engStart_x_L;
		pDpeConfig->Dpe_DVSSettings.pd_frame_num = tile_num[idx];


		LOG_INF("a occ_width = %d,pd_frame_num  = %d\n",
		pDpeConfig->Dpe_DVSSettings.occ_width,
		pDpeConfig->Dpe_DVSSettings.pd_frame_num);
	if ((pDpeConfig->Dpe_DVSSettings.dram_out_pitch_en) == 0 &&
			(pDpeConfig->Dpe_DVSSettings.occ_width > 640))
		LOG_INF("Dram_out_pitch_en not turn on, but occwidth over 640\n");
	if ((pDpeConfig->Dpe_DVSSettings.dram_out_pitch_en == 1) &&
			(pDpeConfig->Dpe_DVSSettings.occ_width < 640))
		LOG_INF("Dram_out_pitch_en turn on, but occwidth is smaller than 640\n");

#if defined(UT_CASE)
		pDpeConfig->Dpe_DVSSettings.pd_st_x = 0;
#else
		pDpeConfig->Dpe_DVSSettings.pd_st_x = st_x;
#endif
	}
#endif
}

static signed int DPE_Dump_kernelReg(struct DPE_Config_ISP8 *cfg)
{
	//DVS Register
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL00 = (unsigned int)DPE_RD32(DVS_CTRL00_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL01 = (unsigned int)DPE_RD32(DVS_CTRL01_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL02 = (unsigned int)DPE_RD32(DVS_CTRL02_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL03 = (unsigned int)DPE_RD32(DVS_CTRL03_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL06 = (unsigned int)DPE_RD32(DVS_CTRL06_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL07 = (unsigned int)DPE_RD32(DVS_CTRL07_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL08 = (unsigned int)DPE_RD32(DVS_CTRL08_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL_STATUS3 = (unsigned int)DPE_RD32(DVS_CTRL_STATUS3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_IRQ_00 = (unsigned int)DPE_RD32(DVS_IRQ_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_IRQ_01 = (unsigned int)DPE_RD32(DVS_IRQ_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL_STATUS0 = (unsigned int)DPE_RD32(DVS_CTRL_STATUS0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL_STATUS2 = (unsigned int)DPE_RD32(DVS_CTRL_STATUS2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_IRQ_STATUS = (unsigned int)DPE_RD32(DVS_IRQ_STATUS_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_FRM_STATUS0 = (unsigned int)DPE_RD32(DVS_FRM_STATUS0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_FRM_STATUS1 = (unsigned int)DPE_RD32(DVS_FRM_STATUS1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_FRM_STATUS2 = (unsigned int)DPE_RD32(DVS_FRM_STATUS2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_FRM_STATUS3 = (unsigned int)DPE_RD32(DVS_FRM_STATUS3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_FRM_STATUS4 = (unsigned int)DPE_RD32(DVS_FRM_STATUS4_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_EXT_STATUS0 = (unsigned int)DPE_RD32(DVS_EXT_STATUS0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_EXT_STATUS1 = (unsigned int)DPE_RD32(DVS_EXT_STATUS1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CUR_STATUS = (unsigned int)DPE_RD32(DVS_CUR_STATUS_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_CTRL = (unsigned int)DPE_RD32(DVS_SRC_CTRL_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CRC_CTRL = (unsigned int)DPE_RD32(DVS_CRC_CTRL_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CRC_IN = (unsigned int)DPE_RD32(DVS_CRC_IN_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_STA0 = (unsigned int)DPE_RD32(DVS_DRAM_STA0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_STA1 = (unsigned int)DPE_RD32(DVS_DRAM_STA1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_ULT = (unsigned int)DPE_RD32(DVS_DRAM_ULT_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_PITCH = (unsigned int)DPE_RD32(DVS_DRAM_PITCH_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_PITCH1 = (unsigned int)DPE_RD32(DVS_DRAM_PITCH1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_00 = (unsigned int)DPE_RD32(DVS_SRC_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_01 = (unsigned int)DPE_RD32(DVS_SRC_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_02 = (unsigned int)DPE_RD32(DVS_SRC_02_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_03 = (unsigned int)DPE_RD32(DVS_SRC_03_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_04 = (unsigned int)DPE_RD32(DVS_SRC_04_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_05_L_FRM0 = (unsigned int)DPE_RD32(DVS_SRC_05_L_FRM0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_06_L_FRM1 = (unsigned int)DPE_RD32(DVS_SRC_06_L_FRM1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_07_L_FRM2 = (unsigned int)DPE_RD32(DVS_SRC_07_L_FRM2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_08_L_FRM3 = (unsigned int)DPE_RD32(DVS_SRC_08_L_FRM3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_09_R_FRM0 = (unsigned int)DPE_RD32(DVS_SRC_09_R_FRM0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_10_R_FRM1 = (unsigned int)DPE_RD32(DVS_SRC_10_R_FRM1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_11_R_FRM2 = (unsigned int)DPE_RD32(DVS_SRC_11_R_FRM2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_12_R_FRM3 = (unsigned int)DPE_RD32(DVS_SRC_12_R_FRM3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_13_Hist0 = (unsigned int)DPE_RD32(DVS_SRC_13_Hist0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_14_Hist1 = (unsigned int)DPE_RD32(DVS_SRC_14_Hist1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_15_Hist2 = (unsigned int)DPE_RD32(DVS_SRC_15_Hist2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_16_Hist3 = (unsigned int)DPE_RD32(DVS_SRC_16_Hist3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_17_OCCDV_EXT0 =
	(unsigned int)DPE_RD32(DVS_SRC_17_OCCDV_EXT0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_18_OCCDV_EXT1 =
	(unsigned int)DPE_RD32(DVS_SRC_18_OCCDV_EXT1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_19_OCCDV_EXT2 =
	(unsigned int)DPE_RD32(DVS_SRC_19_OCCDV_EXT2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_20_OCCDV_EXT3 =
	(unsigned int)DPE_RD32(DVS_SRC_20_OCCDV_EXT3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_21_P4_L_DV_ADR =
	(unsigned int)DPE_RD32(DVS_SRC_21_P4_L_DV_ADR_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_22_OCCDV0 = (unsigned int)DPE_RD32(DVS_SRC_22_OCCDV0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_23_OCCDV1 = (unsigned int)DPE_RD32(DVS_SRC_23_OCCDV1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_24_OCCDV2 = (unsigned int)DPE_RD32(DVS_SRC_24_OCCDV2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_25_OCCDV3 = (unsigned int)DPE_RD32(DVS_SRC_25_OCCDV3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_26_P4_R_DV_ADR =
	(unsigned int)DPE_RD32(DVS_SRC_26_P4_R_DV_ADR_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_27 = (unsigned int)DPE_RD32(DVS_SRC_27_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_28 = (unsigned int)DPE_RD32(DVS_SRC_28_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_29 = (unsigned int)DPE_RD32(DVS_SRC_29_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_30 = (unsigned int)DPE_RD32(DVS_SRC_30_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_31 = (unsigned int)DPE_RD32(DVS_SRC_31_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_32 = (unsigned int)DPE_RD32(DVS_SRC_32_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_33 = (unsigned int)DPE_RD32(DVS_SRC_33_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_34 = (unsigned int)DPE_RD32(DVS_SRC_34_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_35 = (unsigned int)DPE_RD32(DVS_SRC_35_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_36 = (unsigned int)DPE_RD32(DVS_SRC_36_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_37 = (unsigned int)DPE_RD32(DVS_SRC_37_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_38 = (unsigned int)DPE_RD32(DVS_SRC_38_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_39 = (unsigned int)DPE_RD32(DVS_SRC_39_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_40_LDV_HIST0 = (unsigned int)DPE_RD32(DVS_SRC_40_LDV_HIST0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_41_LDV_HIST1 = (unsigned int)DPE_RD32(DVS_SRC_41_LDV_HIST1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_42_LDV_HIST2 = (unsigned int)DPE_RD32(DVS_SRC_42_LDV_HIST2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_43_LDV_HIST3 = (unsigned int)DPE_RD32(DVS_SRC_43_LDV_HIST3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_SRC_44 = (unsigned int)DPE_RD32(DVS_SRC_44_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CRC_OUT_0 = (unsigned int)DPE_RD32(DVS_CRC_OUT_0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CRC_OUT_1 = (unsigned int)DPE_RD32(DVS_CRC_OUT_1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CRC_OUT_2 = (unsigned int)DPE_RD32(DVS_CRC_OUT_2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CRC_OUT_3 = (unsigned int)DPE_RD32(DVS_CRC_OUT_3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_SEC_0 = (unsigned int)DPE_RD32(DVS_DRAM_SEC_0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_SEC_1 = (unsigned int)DPE_RD32(DVS_DRAM_SEC_1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_SEC_2 = (unsigned int)DPE_RD32(DVS_DRAM_SEC_2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_SEC_3 = (unsigned int)DPE_RD32(DVS_DRAM_SEC_3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_AXSLC_0 = (unsigned int)DPE_RD32(DVS_DRAM_SEC_1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DRAM_AXSLC_1 = (unsigned int)DPE_RD32(DVS_DRAM_SEC_2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DEQ_FORCE = (unsigned int)DPE_RD32(DVS_DEQ_FORCE_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL_RESERVED = (unsigned int)DPE_RD32(DVS_CTRL_RESERVED_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_CTRL_ATPG = (unsigned int)DPE_RD32(DVS_CTRL_ATPG_REG);
	//ME
	cfg->DPE_Kernel_DpeConfig.DVS_ME_00 = (unsigned int)DPE_RD32(DVS_ME_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_01 = (unsigned int)DPE_RD32(DVS_ME_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_02 = (unsigned int)DPE_RD32(DVS_ME_02_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_03 = (unsigned int)DPE_RD32(DVS_ME_03_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_04 = (unsigned int)DPE_RD32(DVS_ME_04_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_05 = (unsigned int)DPE_RD32(DVS_ME_05_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_06 = (unsigned int)DPE_RD32(DVS_ME_06_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_07 = (unsigned int)DPE_RD32(DVS_ME_07_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_08 = (unsigned int)DPE_RD32(DVS_ME_08_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_09 = (unsigned int)DPE_RD32(DVS_ME_09_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_10 = (unsigned int)DPE_RD32(DVS_ME_10_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_11 = (unsigned int)DPE_RD32(DVS_ME_11_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_12 = (unsigned int)DPE_RD32(DVS_ME_12_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_13 = (unsigned int)DPE_RD32(DVS_ME_13_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_14 = (unsigned int)DPE_RD32(DVS_ME_14_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_15 = (unsigned int)DPE_RD32(DVS_ME_15_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_16 = (unsigned int)DPE_RD32(DVS_ME_16_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_17 = (unsigned int)DPE_RD32(DVS_ME_17_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_18 = (unsigned int)DPE_RD32(DVS_ME_18_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_19 = (unsigned int)DPE_RD32(DVS_ME_19_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_20 = (unsigned int)DPE_RD32(DVS_ME_20_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_21 = (unsigned int)DPE_RD32(DVS_ME_21_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_22 = (unsigned int)DPE_RD32(DVS_ME_22_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_23 = (unsigned int)DPE_RD32(DVS_ME_23_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_24 = (unsigned int)DPE_RD32(DVS_ME_24_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_25 = (unsigned int)DPE_RD32(DVS_ME_25_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_26 = (unsigned int)DPE_RD32(DVS_ME_26_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_27 = (unsigned int)DPE_RD32(DVS_ME_27_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_28 = (unsigned int)DPE_RD32(DVS_ME_28_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_29 = (unsigned int)DPE_RD32(DVS_ME_29_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_30 = (unsigned int)DPE_RD32(DVS_ME_30_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_31 = (unsigned int)DPE_RD32(DVS_ME_31_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_32 = (unsigned int)DPE_RD32(DVS_ME_32_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_33 = (unsigned int)DPE_RD32(DVS_ME_33_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_34 = (unsigned int)DPE_RD32(DVS_ME_34_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_35 = (unsigned int)DPE_RD32(DVS_ME_35_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_36 = (unsigned int)DPE_RD32(DVS_ME_36_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_37 = (unsigned int)DPE_RD32(DVS_ME_37_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_38 = (unsigned int)DPE_RD32(DVS_ME_38_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_39 = (unsigned int)DPE_RD32(DVS_ME_39_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_DEBUG = (unsigned int)DPE_RD32(DVS_DEBUG_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_RESERVED = (unsigned int)DPE_RD32(DVS_ME_RESERVED_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_ATPG = (unsigned int)DPE_RD32(DVS_ME_ATPG_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_40 = (unsigned int)DPE_RD32(DVS_ME_40_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_41 = (unsigned int)DPE_RD32(DVS_ME_41_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_42 = (unsigned int)DPE_RD32(DVS_ME_42_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_43 = (unsigned int)DPE_RD32(DVS_ME_43_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_44 = (unsigned int)DPE_RD32(DVS_ME_44_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_45 = (unsigned int)DPE_RD32(DVS_ME_45_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_46 = (unsigned int)DPE_RD32(DVS_ME_46_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_47 = (unsigned int)DPE_RD32(DVS_ME_47_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_48 = (unsigned int)DPE_RD32(DVS_ME_48_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_49 = (unsigned int)DPE_RD32(DVS_ME_49_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_50 = (unsigned int)DPE_RD32(DVS_ME_50_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_51 = (unsigned int)DPE_RD32(DVS_ME_51_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_52 = (unsigned int)DPE_RD32(DVS_ME_52_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_53 = (unsigned int)DPE_RD32(DVS_ME_53_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_54 = (unsigned int)DPE_RD32(DVS_ME_54_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_55 = (unsigned int)DPE_RD32(DVS_ME_55_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_56 = (unsigned int)DPE_RD32(DVS_ME_56_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_57 = (unsigned int)DPE_RD32(DVS_ME_57_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_58 = (unsigned int)DPE_RD32(DVS_ME_58_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_ME_59 = (unsigned int)DPE_RD32(DVS_ME_59_REG);
	//OCC
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_PQ_0 = (unsigned int)DPE_RD32(DVS_OCC_PQ_0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_PQ_1 = (unsigned int)DPE_RD32(DVS_OCC_PQ_1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_PQ_2 = (unsigned int)DPE_RD32(DVS_OCC_PQ_2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_PQ_3 = (unsigned int)DPE_RD32(DVS_OCC_PQ_3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_PQ_4 = (unsigned int)DPE_RD32(DVS_OCC_PQ_4_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_PQ_5 = (unsigned int)DPE_RD32(DVS_OCC_PQ_5_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_PQ_10 = (unsigned int)DPE_RD32(DVS_OCC_PQ_10_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_PQ_11 = (unsigned int)DPE_RD32(DVS_OCC_PQ_11_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_PQ_12 = (unsigned int)DPE_RD32(DVS_OCC_PQ_12_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_ATPG = (unsigned int)DPE_RD32(DVS_OCC_ATPG_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST0 = (unsigned int)DPE_RD32(DVS_OCC_HIST0_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST1 = (unsigned int)DPE_RD32(DVS_OCC_HIST1_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST2 = (unsigned int)DPE_RD32(DVS_OCC_HIST2_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST3 = (unsigned int)DPE_RD32(DVS_OCC_HIST3_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST4 = (unsigned int)DPE_RD32(DVS_OCC_HIST4_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST5 = (unsigned int)DPE_RD32(DVS_OCC_HIST5_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST6 = (unsigned int)DPE_RD32(DVS_OCC_HIST6_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST7 = (unsigned int)DPE_RD32(DVS_OCC_HIST7_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST8 = (unsigned int)DPE_RD32(DVS_OCC_HIST8_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST9 = (unsigned int)DPE_RD32(DVS_OCC_HIST9_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST10 = (unsigned int)DPE_RD32(DVS_OCC_HIST10_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST11 = (unsigned int)DPE_RD32(DVS_OCC_HIST11_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST12 = (unsigned int)DPE_RD32(DVS_OCC_HIST12_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST13 = (unsigned int)DPE_RD32(DVS_OCC_HIST13_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST14 = (unsigned int)DPE_RD32(DVS_OCC_HIST14_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST15 = (unsigned int)DPE_RD32(DVS_OCC_HIST15_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST16 = (unsigned int)DPE_RD32(DVS_OCC_HIST16_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST17 = (unsigned int)DPE_RD32(DVS_OCC_HIST17_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_HIST18 = (unsigned int)DPE_RD32(DVS_OCC_HIST18_REG);
	cfg->DPE_Kernel_DpeConfig.DVS_OCC_LDV0 = (unsigned int)DPE_RD32(DVS_OCC_LDV0_REG);
	//DVP
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL00 = (unsigned int)DPE_RD32(DVP_CTRL00_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL01 = (unsigned int)DPE_RD32(DVP_CTRL01_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL02 = (unsigned int)DPE_RD32(DVP_CTRL02_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL03 = (unsigned int)DPE_RD32(DVP_CTRL03_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL04 = (unsigned int)DPE_RD32(DVP_CTRL04_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL05 = (unsigned int)DPE_RD32(DVP_CTRL05_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL07 = (unsigned int)DPE_RD32(DVP_CTRL07_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_IRQ_00 = (unsigned int)DPE_RD32(DVP_IRQ_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_IRQ_01 = (unsigned int)DPE_RD32(DVP_IRQ_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL_STATUS0 = (unsigned int)DPE_RD32(DVP_CTRL_STATUS0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL_STATUS1 = (unsigned int)DPE_RD32(DVP_CTRL_STATUS1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_IRQ_STATUS = (unsigned int)DPE_RD32(DVP_IRQ_STATUS_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_FRM_STATUS0 = (unsigned int)DPE_RD32(DVP_FRM_STATUS0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_FRM_STATUS1 = (unsigned int)DPE_RD32(DVP_FRM_STATUS1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_FRM_STATUS2 = (unsigned int)DPE_RD32(DVP_FRM_STATUS2_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_FRM_STATUS3 = (unsigned int)DPE_RD32(DVP_FRM_STATUS3_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CUR_STATUS = (unsigned int)DPE_RD32(DVP_CUR_STATUS_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_00 = (unsigned int)DPE_RD32(DVP_SRC_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_01 = (unsigned int)DPE_RD32(DVP_SRC_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_02 = (unsigned int)DPE_RD32(DVP_SRC_02_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_03 = (unsigned int)DPE_RD32(DVP_SRC_03_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_04 = (unsigned int)DPE_RD32(DVP_SRC_04_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_05_Y_FRM0 = (unsigned int)DPE_RD32(DVP_SRC_05_Y_FRM0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_06_Y_FRM1 = (unsigned int)DPE_RD32(DVP_SRC_06_Y_FRM1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_07_Y_FRM2 = (unsigned int)DPE_RD32(DVP_SRC_07_Y_FRM2_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_08_Y_FRM3 = (unsigned int)DPE_RD32(DVP_SRC_08_Y_FRM3_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_09_C_FRM0 = (unsigned int)DPE_RD32(DVP_SRC_09_C_FRM0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_10_C_FRM1 = (unsigned int)DPE_RD32(DVP_SRC_10_C_FRM1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_11_C_FRM2 = (unsigned int)DPE_RD32(DVP_SRC_11_C_FRM2_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_12_C_FRM3 = (unsigned int)DPE_RD32(DVP_SRC_12_C_FRM3_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_13_OCCDV0 = (unsigned int)DPE_RD32(DVP_SRC_13_OCCDV0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_14_OCCDV1 = (unsigned int)DPE_RD32(DVP_SRC_14_OCCDV1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_15_OCCDV2 = (unsigned int)DPE_RD32(DVP_SRC_15_OCCDV2_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_16_OCCDV3 = (unsigned int)DPE_RD32(DVP_SRC_16_OCCDV3_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_17_CRM = (unsigned int)DPE_RD32(DVP_SRC_17_CRM_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_18_ASF_RMDV =
	(unsigned int)DPE_RD32(DVP_SRC_18_ASF_RMDV_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_19_ASF_RDDV =
	(unsigned int)DPE_RD32(DVP_SRC_19_ASF_RDDV_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_20_ASF_DV0 =
	(unsigned int)DPE_RD32(DVP_SRC_20_ASF_DV0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_21_ASF_DV1 =
	(unsigned int)DPE_RD32(DVP_SRC_21_ASF_DV1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_22_ASF_DV2 =
	(unsigned int)DPE_RD32(DVP_SRC_22_ASF_DV2_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_23_ASF_DV3 =
	(unsigned int)DPE_RD32(DVP_SRC_23_ASF_DV3_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_25_WMF_HFDV =
	(unsigned int)DPE_RD32(DVP_SRC_25_WMF_HFDV_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_26_WMF_DV0 =
	(unsigned int)DPE_RD32(DVP_SRC_26_WMF_DV0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_27_WMF_DV1 =
	(unsigned int)DPE_RD32(DVP_SRC_27_WMF_DV1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_28_WMF_DV2 =
	(unsigned int)DPE_RD32(DVP_SRC_28_WMF_DV2_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_29_WMF_DV3 =
	(unsigned int)DPE_RD32(DVP_SRC_29_WMF_DV3_REG);

	//CORE
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_00 = (unsigned int)DPE_RD32(DVP_CORE_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_01 = (unsigned int)DPE_RD32(DVP_CORE_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_02 = (unsigned int)DPE_RD32(DVP_CORE_02_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_03 = (unsigned int)DPE_RD32(DVP_CORE_03_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_04 = (unsigned int)DPE_RD32(DVP_CORE_04_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_05 = (unsigned int)DPE_RD32(DVP_CORE_05_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_06 = (unsigned int)DPE_RD32(DVP_CORE_06_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_07 = (unsigned int)DPE_RD32(DVP_CORE_07_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_08 = (unsigned int)DPE_RD32(DVP_CORE_08_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_09 = (unsigned int)DPE_RD32(DVP_CORE_09_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_10 = (unsigned int)DPE_RD32(DVP_CORE_10_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_11 = (unsigned int)DPE_RD32(DVP_CORE_11_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_12 = (unsigned int)DPE_RD32(DVP_CORE_12_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_13 = (unsigned int)DPE_RD32(DVP_CORE_13_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_14 = (unsigned int)DPE_RD32(DVP_CORE_14_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_15 = (unsigned int)DPE_RD32(DVP_CORE_15_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_16 = (unsigned int)DPE_RD32(DVP_CORE_16_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_17 = (unsigned int)DPE_RD32(DVP_CORE_17_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_18 = (unsigned int)DPE_RD32(DVP_CORE_18_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_19 = (unsigned int)DPE_RD32(DVP_CORE_19_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_SRC_CTRL = (unsigned int)DPE_RD32(DVP_SRC_CTRL_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL_RESERVED = (unsigned int)DPE_RD32(DVP_CTRL_RESERVED_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CTRL_ATPG = (unsigned int)DPE_RD32(DVP_CTRL_ATPG_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CRC_OUT_0 = (unsigned int)DPE_RD32(DVP_CRC_OUT_0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CRC_OUT_1 = (unsigned int)DPE_RD32(DVP_CRC_OUT_1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CRC_OUT_2 = (unsigned int)DPE_RD32(DVP_CRC_OUT_2_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CRC_CTRL = (unsigned int)DPE_RD32(DVP_CRC_CTRL_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CRC_OUT = (unsigned int)DPE_RD32(DVP_CRC_OUT_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CRC_IN = (unsigned int)DPE_RD32(DVP_CRC_IN_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_DRAM_STA = (unsigned int)DPE_RD32(DVP_DRAM_STA_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_DRAM_ULT = (unsigned int)DPE_RD32(DVP_DRAM_ULT_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_DRAM_PITCH = (unsigned int)DPE_RD32(DVP_DRAM_PITCH_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_DRAM_SEC_0 = (unsigned int)DPE_RD32(DVP_DRAM_SEC_0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_DRAM_SEC_1 = (unsigned int)DPE_RD32(DVP_DRAM_SEC_1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_DRAM_AXSLC = (unsigned int)DPE_RD32(DVP_DRAM_AXSLC_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_CORE_CRC_IN = (unsigned int)DPE_RD32(DVP_CORE_CRC_IN_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_13_OCCDV0 =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_13_OCCDV0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_14_OCCDV1 =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_14_OCCDV1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_15_OCCDV2 =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_15_OCCDV2_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_16_OCCDV3 =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_16_OCCDV3_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_18_ASF_RMDV =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_18_ASF_RMDV_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_19_ASF_RDDV =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_19_ASF_RDDV_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_20_ASF_DV0 =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_20_ASF_DV0_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_21_ASF_DV1 =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_21_ASF_DV1_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_22_ASF_DV2 =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_22_ASF_DV2_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_23_ASF_DV3 =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_23_ASF_DV3_REG);
	cfg->DPE_Kernel_DpeConfig.DVP_EXT_SRC_24_WMF_RDDV =
	(unsigned int)DPE_RD32(DVP_EXT_SRC_24_WMF_RDDV_REG);
	// DVGF
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_00 =
	(unsigned int)DPE_RD32(DVGF_CTRL_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_01 =
	(unsigned int)DPE_RD32(DVGF_CTRL_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_02 =
	(unsigned int)DPE_RD32(DVGF_CTRL_02_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_03 =
	(unsigned int)DPE_RD32(DVGF_CTRL_03_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_05 =
	(unsigned int)DPE_RD32(DVGF_CTRL_05_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_07 =
	(unsigned int)DPE_RD32(DVGF_CTRL_07_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_IRQ_00 =
	(unsigned int)DPE_RD32(DVGF_IRQ_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_IRQ_01 =
	(unsigned int)DPE_RD32(DVGF_IRQ_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_STATUS0 =
	(unsigned int)DPE_RD32(DVGF_CTRL_STATUS0_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_STATUS1 =
	(unsigned int)DPE_RD32(DVGF_CTRL_STATUS1_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_IRQ_STATUS =
	(unsigned int)DPE_RD32(DVGF_IRQ_STATUS_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_FRM_STATUS =
	(unsigned int)DPE_RD32(DVGF_FRM_STATUS_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CUR_STATUS =
	(unsigned int)DPE_RD32(DVGF_CUR_STATUS_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CRC_CTRL =
	(unsigned int)DPE_RD32(DVGF_CRC_CTRL_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CRC_OUT =
	(unsigned int)DPE_RD32(DVGF_CRC_OUT_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CRC_IN =
	(unsigned int)DPE_RD32(DVGF_CRC_IN_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CRC_OUT_0 =
	(unsigned int)DPE_RD32(DVGF_CRC_OUT_0_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CRC_OUT_1 =
	(unsigned int)DPE_RD32(DVGF_CRC_OUT_1_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CRC_OUT_2 =
	(unsigned int)DPE_RD32(DVGF_CRC_OUT_2_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_CRC_IN =
	(unsigned int)DPE_RD32(DVGF_CORE_CRC_IN_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_DRAM_STA =
	(unsigned int)DPE_RD32(DVGF_DRAM_STA_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_DRAM_PITCH =
	(unsigned int)DPE_RD32(DVGF_DRAM_PITCH_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_DRAM_SEC_0 =
	(unsigned int)DPE_RD32(DVGF_DRAM_SEC_0_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_DRAM_SEC_1 =
	(unsigned int)DPE_RD32(DVGF_DRAM_SEC_1_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_DRAM_AXSLC =
	(unsigned int)DPE_RD32(DVGF_DRAM_AXSLC_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_STATUS_32b_00 =
	(unsigned int)DPE_RD32(DVGF_CTRL_STATUS_32b_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_STATUS_32b_01 =
	(unsigned int)DPE_RD32(DVGF_CTRL_STATUS_32b_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_STATUS_32b_02 =
	(unsigned int)DPE_RD32(DVGF_CTRL_STATUS_32b_02_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_STATUS_32b_03 =
	(unsigned int)DPE_RD32(DVGF_CTRL_STATUS_32b_03_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_STATUS_32b_04 =
	(unsigned int)DPE_RD32(DVGF_CTRL_STATUS_32b_04_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_RESERVED =
	(unsigned int)DPE_RD32(DVGF_CTRL_RESERVED_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CTRL_ATPG =
	(unsigned int)DPE_RD32(DVGF_CTRL_ATPG_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_00 =
	(unsigned int)DPE_RD32(DVGF_SRC_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_01 =
	(unsigned int)DPE_RD32(DVGF_SRC_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_02 =
	(unsigned int)DPE_RD32(DVGF_SRC_02_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_04 =
	(unsigned int)DPE_RD32(DVGF_SRC_04_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_05 =
	(unsigned int)DPE_RD32(DVGF_SRC_05_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_06 =
	(unsigned int)DPE_RD32(DVGF_SRC_06_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_07 =
	(unsigned int)DPE_RD32(DVGF_SRC_07_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_08 =
	(unsigned int)DPE_RD32(DVGF_SRC_08_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_09 =
	(unsigned int)DPE_RD32(DVGF_SRC_09_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_10 =
	(unsigned int)DPE_RD32(DVGF_SRC_10_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_11 =
	(unsigned int)DPE_RD32(DVGF_SRC_11_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_12 =
	(unsigned int)DPE_RD32(DVGF_SRC_12_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_13 =
	(unsigned int)DPE_RD32(DVGF_SRC_13_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_14 =
	(unsigned int)DPE_RD32(DVGF_SRC_14_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_15 =
	(unsigned int)DPE_RD32(DVGF_SRC_15_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_16 =
	(unsigned int)DPE_RD32(DVGF_SRC_16_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_17 =
	(unsigned int)DPE_RD32(DVGF_SRC_17_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_18 =
	(unsigned int)DPE_RD32(DVGF_SRC_18_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_19 =
	(unsigned int)DPE_RD32(DVGF_SRC_19_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_20 =
	(unsigned int)DPE_RD32(DVGF_SRC_20_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_21 =
	(unsigned int)DPE_RD32(DVGF_SRC_21_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_22 =
	(unsigned int)DPE_RD32(DVGF_SRC_22_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_23 =
	(unsigned int)DPE_RD32(DVGF_SRC_23_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_SRC_24 =
	(unsigned int)DPE_RD32(DVGF_SRC_24_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_00 =
	(unsigned int)DPE_RD32(DVGF_CORE_00_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_01 =
	(unsigned int)DPE_RD32(DVGF_CORE_01_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_02 =
	(unsigned int)DPE_RD32(DVGF_CORE_02_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_03 =
	(unsigned int)DPE_RD32(DVGF_CORE_03_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_05 =
	(unsigned int)DPE_RD32(DVGF_CORE_05_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_06 =
	(unsigned int)DPE_RD32(DVGF_CORE_06_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_07 =
	(unsigned int)DPE_RD32(DVGF_CORE_07_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_08 =
	(unsigned int)DPE_RD32(DVGF_CORE_08_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_09 =
	(unsigned int)DPE_RD32(DVGF_CORE_09_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_10 =
	(unsigned int)DPE_RD32(DVGF_CORE_10_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_11 =
	(unsigned int)DPE_RD32(DVGF_CORE_11_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_12 =
	(unsigned int)DPE_RD32(DVGF_CORE_12_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_13 =
	(unsigned int)DPE_RD32(DVGF_CORE_13_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_14 =
	(unsigned int)DPE_RD32(DVGF_CORE_14_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_15 =
	(unsigned int)DPE_RD32(DVGF_CORE_15_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_16 =
	(unsigned int)DPE_RD32(DVGF_CORE_16_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_17 =
	(unsigned int)DPE_RD32(DVGF_CORE_17_REG);
	cfg->DPE_Kernel_DpeConfig.DVGF_CORE_18 =
	(unsigned int)DPE_RD32(DVGF_CORE_18_REG);
	return 0;
}

static inline int DPE_Prepare_Enable_ccf_clock(void)
{
	int ret;
	struct device *dev = gdev;
	// struct DPE_device *dpe_dev = dev_get_drvdata(dev);

	LOG_INF("DPE_Prepare_Enable_ccf_clock_star\n");
	/* mtk_mmdvfs_enable_vcp(true, VCP_PWR_USR_CAM); */

	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_info(dev, "pm_runtime_get_sync fail, %d\n", ret);
		return ret;
	}

	// dev_info(dev, "enable DPE clock:%d\n", dpe_dev->clk_num);
	// ret = clk_bulk_prepare_enable(dpe_dev->clk_num, dpe_dev->clks);
	// if (ret) {
		// dev_info(dev, "failed to enable DPE clock:%d\n", ret);
		// return ret;
	// }

	if (DPE_devs[0].dev_ver == 0)
		mtk_cam_bwr_enable(dpe_bwr_device);

	// ret = clk_prepare_enable(dpe_clk.CLK_CK2_DPE_SEL);
	// if (ret)
	// LOG_INF("cannot prepare and enable CLK_CK2_DPE_SEL clock\n");

	ret = clk_prepare_enable(dpe_clk.CLK_CAM_MAIN_CAM);
	if (ret)
		LOG_INF("cannot prepare and enable CLK_CAM_MAIN_CAM clock\n");

	if (DPE_devs[0].dev_ver == 1) {
		ret = clk_prepare_enable(dpe_clk.CLK_CAMSYS_IPE_LARB19_CAMERA_P2);
		if (ret)
			LOG_INF("cannot prepare and enable CLK_CAMSYS_IPE_LARB19_CAMERA_P2 clock\n");
	}

	ret = clk_prepare_enable(dpe_clk.CLK_CAMSYS_IPE_DPE_CAMERA_P2);
	if (ret)
		LOG_INF("cannot prepare and enable CLK_CAMSYS_IPE_DPE_CAMERA_P2 clock\n");

	ret = clk_prepare_enable(dpe_clk.CLK_CAMSYS_IPE_FUS_CAMERA_P2);
	if (ret)
		LOG_INF("cannot prepare and enable CLK_CAMSYS_IPE_FUS_CAMERA_P2 clock\n");

	if (DPE_devs[0].dev_ver == 0) {
		ret = clk_prepare_enable(dpe_clk.CLK_CAMSYS_IPE_DHZE_CAMERA_P2);
		if (ret)
			LOG_INF("cannot prepare and enable CLK_CAMSYS_IPE_DHZE_CAMERA_P2 clock\n");
	}

	ret = clk_prepare_enable(dpe_clk.CLK_CAMSYS_IPE_GALS_CAMERA_P2);
	if (ret)
		LOG_INF("cannot prepare and enable CLK_CAMSYS_IPE_GALS_CAMERA_P2 clock\n");

	return ret;
}
static inline void DPE_Disable_Unprepare_ccf_clock(void)
{
	// struct device *dev = gdev;
	// struct DPE_device *dpe_dev = dev_get_drvdata(dev);

	LOG_INF("Disable_Unprepare_ccf_clock start\n");

	// clk_bulk_disable_unprepare(dpe_dev->clk_num, dpe_dev->clks);

	clk_disable_unprepare(dpe_clk.CLK_CAMSYS_IPE_GALS_CAMERA_P2);
	if (DPE_devs[0].dev_ver == 0)
		clk_disable_unprepare(dpe_clk.CLK_CAMSYS_IPE_DHZE_CAMERA_P2);
	clk_disable_unprepare(dpe_clk.CLK_CAMSYS_IPE_FUS_CAMERA_P2);
	clk_disable_unprepare(dpe_clk.CLK_CAMSYS_IPE_DPE_CAMERA_P2);
	if (DPE_devs[0].dev_ver == 1)
		clk_disable_unprepare(dpe_clk.CLK_CAMSYS_IPE_LARB19_CAMERA_P2);
	clk_disable_unprepare(dpe_clk.CLK_CAM_MAIN_CAM);
	// clk_disable_unprepare(dpe_clk.CLK_CK2_DPE_SEL);

	if (DPE_devs[0].dev_ver == 0)
		mtk_cam_bwr_disable(dpe_bwr_device);

	pm_runtime_put_sync(gdev);
	// mtk_mmdvfs_enable_vcp(false, VCP_PWR_USR_CAM);

	LOG_INF("Disable_Unprepare_ccf_clock end\n");
}

/**************************************************************
 *
 **************************************************************/
static void DPE_EnableClock(bool En)
{

#if defined(EP_NO_CLKMGR)
	unsigned int setReg;
#endif
#if IS_ENABLED(CONFIG_MTK_IOMMU_V2)
	int ret = 0;
#endif
	if (g_isshutdown) {
		LOG_INF("%s : system is shutdown: %d", __func__, g_isshutdown);
		return;
	}
	if (En) { /* Enable clock. */
		/* LOG_DBG("clock enbled. g_u4EnableClockCount: %d.", g_u4EnableClockCount); */
		//mutex_lock(&gDpeMutex);	//!
		spin_lock(&(DPEInfo.SpinLockDPE));
		switch (g_u4EnableClockCount) {
		case 0:
			spin_unlock(&(DPEInfo.SpinLockDPE));

			LOG_INF("[Debug]DPE_EnClock CLK OPEN");
			DPE_Prepare_Enable_ccf_clock();
			//mutex_unlock(&gDpeMutex);//!

			spin_lock(&(DPEInfo.SpinLockDPE));
			g_u4EnableClockCount++;
			spin_unlock(&(DPEInfo.SpinLockDPE));
			break;
		default:
			g_u4EnableClockCount++;
			spin_unlock(&(DPEInfo.SpinLockDPE));
			//mutex_unlock(&gDpeMutex);
			break;
		}
#if IS_ENABLED(CONFIG_MTK_IOMMU_V2)
		spin_lock(&(DPEInfo.SpinLockDPE));
		if (g_u4EnableClockCount == 1) {
			spin_unlock(&(DPEInfo.SpinLockDPE));
			ret = m4u_control_iommu_port();
			if (ret)
				LOG_ERR("cannot config M4U IOMMU PORTS\n");
		} else {
			spin_unlock(&(DPEInfo.SpinLockDPE));
		}
#endif
	} else {		/* Disable clock. */
		/* LOG_DBG("Dpe clock disabled. g_u4EnableClockCount: %d.",
		 * g_u4EnableClockCount);
		 */
		spin_lock(&(DPEInfo.SpinLockDPE));
		g_u4EnableClockCount--;
		switch (g_u4EnableClockCount) {
		case 0:
			spin_unlock(&(DPEInfo.SpinLockDPE));
			DPE_Disable_Unprepare_ccf_clock();
			break;
		default:
			spin_unlock(&(DPEInfo.SpinLockDPE));
			break;
		}
	}
}
/*******************************************************************************
 *
 ******************************************************************************/
static inline void DPE_Reset(void)
{
	LOG_DBG("- E.");
	LOG_DBG(" DPE Reset start!\n");
	//mutex_lock(&(MutexDPERef));
	if (DPEInfo.UserCount > 1) {
//		mutex_unlock(&(MutexDPERef));
		LOG_DBG("Curr UserCount(%d) users exist", DPEInfo.UserCount);
	} else {
//		mutex_unlock(&(MutexDPERef));
		/* Reset DPE flow */
		#ifdef CMdq_en
		DPE_MASKWR(DVS_CTRL01_REG, 0x70000000, 0x70000000);
		DPE_MASKWR(DVP_CTRL01_REG, 0x70000000, 0x70000000);
		DPE_MASKWR(DVS_CTRL01_REG, 0x00000000, 0x70000000);
		DPE_MASKWR(DVP_CTRL01_REG, 0x00000000, 0x70000000);
		#endif
		LOG_DBG(" DPE Reset end!\n");
	}
}
/*******************************************************************************
 *
 ******************************************************************************/
#ifdef DPE_ioctl_en
static signed int DPE_ReadReg(struct DPE_REG_IO_STRUCT *pRegIo)
{
	unsigned int i;
	signed int Ret = 0;
	/*  */
	struct DPE_REG_STRUCT reg;
	/* unsigned int* pData = (unsigned int*)pRegIo->Data; */
	struct DPE_REG_STRUCT *pData = (struct DPE_REG_STRUCT *) pRegIo->pData;

	if ((pRegIo->pData == NULL) ||
		(pRegIo->Count == 0) ||
		(pRegIo->Count > DPE_MAX_REG_CNT)) {
		LOG_ERR("ERROR: pRegIo->pData is NULL or Count:%d\n",
			pRegIo->Count);
		Ret = -EFAULT;
		goto EXIT;
	}
	for (i = 0; i < pRegIo->Count; i++) {
		if (get_user(reg.Addr, (unsigned int *) &pData->Addr) != 0) {
			LOG_ERR("get_user failed");
			Ret = -EFAULT;
			goto EXIT;
		}
		/* pData++; */
		/*  */
		if ((ISP_DPE_BASE + reg.Addr >= ISP_DPE_BASE)
		    && (ISP_DPE_BASE + reg.Addr <
						(ISP_DPE_BASE + DPE_REG_RANGE))
			&& ((reg.Addr & 0x3) == 0)) {
			reg.Val = DPE_RD32(ISP_DPE_BASE + reg.Addr);
		} else {
			LOG_ERR(
			"Wrong address(0x%p)", (ISP_DPE_BASE + reg.Addr));
			reg.Val = 0;
		}
		/*  */
		if (put_user(reg.Val, (unsigned int *) &(pData->Val)) != 0) {
			LOG_ERR("put_user failed");
			Ret = -EFAULT;
			goto EXIT;
		}
		pData++;
		/*  */
	}
	/*  */
EXIT:
	return Ret;
}
#endif
/*******************************************************************************
 *
 ******************************************************************************/
#ifdef DPE_ioctl_en
static signed int DPE_WriteRegToHw(struct DPE_REG_STRUCT *pReg,
							unsigned int Count)
{
	signed int Ret = 0;
	unsigned int i;
	bool dbgWriteReg;

	spin_lock(&(DPEInfo.SpinLockDPE));
	dbgWriteReg = DPEInfo.DebugMask & DPE_DBG_WRITE_REG;
	spin_unlock(&(DPEInfo.SpinLockDPE));
	/*  */
	if (dbgWriteReg)
		LOG_DBG("- E.\n");
	/*  */
	for (i = 0; i < Count; i++) {
		if (dbgWriteReg) {
			LOG_DBG("Addr(%llu), Val(0x%x)\n",
				(unsigned long)(ISP_DPE_BASE + pReg[i].Addr),
				(unsigned int) (pReg[i].Val));
		}
		if (((ISP_DPE_BASE + pReg[i].Addr) <
						(ISP_DPE_BASE + DPE_REG_RANGE))
			&& ((pReg[i].Addr & 0x3) == 0)) {
			#ifdef DPE_WR32_en
			DPE_WR32(ISP_DPE_BASE + pReg[i].Addr, pReg[i].Val);
			#endif
		} else {
			LOG_ERR("wrong address(%lu)\n",
				(unsigned long)(ISP_DPE_BASE + pReg[i].Addr));
		}
	}
	/*  */
	return Ret;
}
#endif
/*******************************************************************************
 *
 ******************************************************************************/
#ifdef DPE_ioctl_en
static signed int DPE_WriteReg(struct DPE_REG_IO_STRUCT *pRegIo)
{
	signed int Ret = 0;
	/*
	 *  signed int TimeVd = 0;
	 *  signed int TimeExpdone = 0;
	 *  signed int TimeTasklet = 0;
	 */
	/* unsigned char* pData = NULL; */
	struct DPE_REG_STRUCT *pData = NULL;
	/*  */
	if (DPEInfo.DebugMask & DPE_DBG_WRITE_REG)
		LOG_DBG("Data(0x%p), Count(%d)\n", (pRegIo->pData),
							(pRegIo->Count));
	/*  */
	if ((pRegIo->pData == NULL) ||
		(pRegIo->Count == 0) ||
		(pRegIo->Count > DPE_MAX_REG_CNT)) {
		LOG_ERR("ERROR: pRegIo->pData is NULL or Count:%d\n",
			pRegIo->Count);
		Ret = -EFAULT;
		goto EXIT;
	}
	pData = kmalloc((pRegIo->Count) *
		sizeof(struct DPE_REG_STRUCT),
		GFP_KERNEL); /* Use GFP_KERNEL instead of GFP_ATOMIC */
if (pData == NULL) {
	LOG_INF("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)\n",
		current->comm,
		current->pid,
		current->tgid);
		Ret = -ENOMEM;
		goto EXIT;
}
	if (copy_from_user(pData,
		(void __user *)(pRegIo->pData),
		pRegIo->Count * sizeof(struct DPE_REG_STRUCT)) != 0) {
		LOG_ERR("copy_from_user failed\n");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	Ret = DPE_WriteRegToHw(pData, pRegIo->Count);
	/*  */
EXIT:
	if (pData != NULL) {
		kfree(pData);
		pData = NULL;
	}
	return Ret;
}
#endif
/*******************************************************************************
 *
 ******************************************************************************/
#ifdef DPE_ioctl_en
static signed int DPE_WaitIrq(struct DPE_WAIT_IRQ_STRUCT *WaitIrq)
{
	signed int Ret = 0;
	signed int Timeout = WaitIrq->Timeout;
	enum DPE_PROCESS_ID_ENUM whichReq = DPE_PROCESS_ID_NONE;
	/*unsigned int i;*/
	unsigned long flags; /* old: unsigned int flags;*/
	unsigned int irqStatus;
	/*int cnt = 0;*/
	//struct timeval time_getrequest;
	struct timespec64 time_getrequest;
	//unsigned long long sec = 0;
	//unsigned long usec = 0;
	unsigned int p;
	/* do_gettimeofday(&time_getrequest); */
	//sec = cpu_clock(0);	/* ns */
	//do_div(sec, 1000);	/* usec */
	//usec = do_div(sec, 1000000);	/* sec and usec */
	//time_getrequest.tv_usec = usec;
	//time_getrequest.tv_sec = sec;
	ktime_get_ts64(&time_getrequest);
	/* Debug interrupt */
	if (DPEInfo.DebugMask & DPE_DBG_INT) {
		if (WaitIrq->Status & DPEInfo.IrqInfo.Mask[WaitIrq->Type]) {
			if (WaitIrq->UserKey > 0) {
				LOG_DBG(
				"+WaitIrq clr(%d), Type(%d), Stat(0x%08X), Timeout(%d),usr(%d), ProcID(%d)\n",
				WaitIrq->Clear, WaitIrq->Type, WaitIrq->Status,
				WaitIrq->Timeout, WaitIrq->UserKey,
				WaitIrq->ProcessID);
			}
		}
	}
	/* 1. wait type update */
	if (WaitIrq->Clear == DPE_IRQ_CLEAR_STATUS) {
		spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		/* LOG_DBG(
		 * "WARNING: Clear(%d), Type(%d): IrqStatus(0x%08X) has been
		 * cleared" ,WaitIrq->EventInfo.Clear,WaitIrq->Type,
		 * DPEInfo.IrqInfo.Status[WaitIrq->Type]);
		 * DPEInfo.IrqInfo.Status[WaitIrq->Type][
		 * WaitIrq->EventInfo.UserKey] &= (~WaitIrq->EventInfo.Status);
		 */
		DPEInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]),
									flags);
		return Ret;
	}
	if (WaitIrq->Clear == DPE_IRQ_CLEAR_WAIT) {
		spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		if (DPEInfo.IrqInfo.Status[WaitIrq->Type] & WaitIrq->Status)
			DPEInfo.IrqInfo.Status[WaitIrq->Type] &=
							(~WaitIrq->Status);
		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]),
									flags);
	} else if (WaitIrq->Clear == DPE_IRQ_CLEAR_ALL) {
		spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		DPEInfo.IrqInfo.Status[WaitIrq->Type] = 0;
		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]),
									flags);
	}
	/* DPE_IRQ_WAIT_CLEAR ==> do nothing */
	/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
	spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
	irqStatus = DPEInfo.IrqInfo.Status[WaitIrq->Type];
	spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
	if (WaitIrq->Status & DPE_INT_ST) {
		whichReq = DPE_PROCESS_ID_DPE;
	} else {
		LOG_ERR(
		"No Such Stats can be waited!! irq Type/User/Sts/Pid(0x%x/%d/0x%x/%d)\n",
			WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status,
			WaitIrq->ProcessID);
	}
	/* 2. start to wait signal */
	Timeout = wait_event_interruptible_timeout(DPEInfo.WaitQueueHead,
						   DPE_GetIRQState(
							WaitIrq->Type,
							WaitIrq->UserKey,
							WaitIrq->Status,
							whichReq,
							WaitIrq->ProcessID),
						   DPE_MsToJiffies(
							WaitIrq->Timeout));
	p = WaitIrq->ProcessID % IRQ_USER_NUM_MAX;
	/* check if user is interrupted by system signal */
	if ((Timeout != 0) &&
		(!DPE_GetIRQState(WaitIrq->Type, WaitIrq->UserKey,
			WaitIrq->Status, whichReq, WaitIrq->ProcessID))) {
		LOG_ERR(
		"interrupted by system, timeout(%d),irq Type/User/Sts/whichReq/Pid(0x%x/%d/0x%x/%d/%d)\n",
		Timeout, WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status,
						whichReq, WaitIrq->ProcessID);
		Ret = -ERESTARTSYS;	/* actually it should be -ERESTARTSYS */
		goto EXIT;
	}
	/* timeout */
	if (Timeout == 0) {
		spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]),
									flags);
		irqStatus = DPEInfo.IrqInfo.Status[WaitIrq->Type];
		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]),
									flags);
		LOG_ERR(
		"WaitIrq Timeout:Tout(%d) clr(%d) Type(%d) IrqStat(0x%08X) WaitStat(0x%08X) usrKey(%d)\n",
		     WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type, irqStatus,
			WaitIrq->Status, WaitIrq->UserKey);
		LOG_ERR(
		"WaitIrq Timeout:whichReq(%d),ProcID(%d) DpeIrqCnt[%d](0x%08X) WriteReq(0x%08X) ReadReq(0x%08X)\n",
			whichReq, WaitIrq->ProcessID,
			p, DPEInfo.IrqInfo.DpeIrqCnt[p],
			DPEInfo.WriteReqIdx, DPEInfo.ReadReqIdx);
		if (WaitIrq->bDumpReg) {
			DPE_DumpReg();
			dpe_request_dump_isp8(&dpe_reqs_dvs);
		}
		Ret = -EFAULT;
		goto EXIT;
	} else {
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("DPE WaitIrq");
#endif
		spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus = DPEInfo.IrqInfo.Status[WaitIrq->Type];
		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]),
									flags);
		if (WaitIrq->Clear == DPE_IRQ_WAIT_CLEAR) {
			spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]),
									flags);
			if (WaitIrq->Status & DPE_INT_ST) {
				//LOG_INF("DpeIrqCnt--  1\n");
				//DPEInfo.IrqInfo.DpeIrqCnt[p]--;
				if (DPEInfo.IrqInfo.DpeIrqCnt[p] == 0)
					DPEInfo.IrqInfo.Status[WaitIrq->Type] &=
							(~WaitIrq->Status);
			} else {
				LOG_ERR(
				"DPE_IRQ_WAIT_CLEAR Error, Type(%d), WaitStatus(0x%08X)",
					WaitIrq->Type, WaitIrq->Status);
			}
			spin_unlock_irqrestore(
				&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		}
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif
	}
EXIT:
	return Ret;
}
#endif
/*******************************************************************************
 *
 ******************************************************************************/
static long DPE_ioctl(struct file *pFile, unsigned int Cmd, unsigned long Param)
{
	signed int Ret = 0;
	/*unsigned int pid = 0;*/
	#ifdef DPE_ioctl_en
	static struct DPE_REG_IO_STRUCT RegIo;
	static struct DPE_WAIT_IRQ_STRUCT IrqInfo;
	static struct DPE_CLEAR_IRQ_STRUCT ClearIrq;
	#endif
	static struct DPE_Config_ISP8 dpe_DpeConfig;
	static struct DPE_Request dpe_DpeReq;
	// signed int enqnum;
	struct DPE_USER_INFO_STRUCT *pUserInfo;
	int enqueNum;
	int dequeNum;
	unsigned long flags;
	int req_temp;
	/* old: unsigned int flags;*//* FIX to avoid build warning */
	/*  */
	if (pFile->private_data == NULL) {
		LOG_ERR("private_data NULL,(process, pid, tgid)=(%s, %d, %d)",
			current->comm,
				current->pid, current->tgid);
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	pUserInfo = (struct DPE_USER_INFO_STRUCT *) (pFile->private_data);
	/*  */


	switch (Cmd) {
	case DPE_RESET:
		{
			LOG_INF("Not support DPE ioctl DPE_RESET\n");
			//DPE_Reset();
			//spin_unlock(&(DPEInfo.SpinLockDPE));
			break;
		}
		/*  */
	case DPE_DUMP_REG:
		{
			LOG_INF("Not support DPE ioctl DPE_DUMP_REG\n");
			//Ret = DPE_DumpReg();
			break;
		}
	case DPE_DUMP_ISR_LOG:
		{
			unsigned int currentPPB = m_CurrentPPB;

			spin_lock_irqsave(
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
									flags);
			m_CurrentPPB = (m_CurrentPPB + 1) % LOG_PPNUM;
			spin_unlock_irqrestore(
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
									flags);
			IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVP_ST, currentPPB,
								_LOG_INF);
			IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVP_ST, currentPPB,
								_LOG_ERR);
			break;
		}
	case DPE_READ_REGISTER:
		{
			LOG_INF("Not support DPE ioctl DPE_READ_REGISTER\n");
			#ifdef DPE_ioctl_en
			if (copy_from_user(&RegIo, (void *)Param,
				sizeof(struct DPE_REG_IO_STRUCT)) == 0) {
				Ret = DPE_ReadReg(&RegIo);
			} else {
				LOG_ERR(
				"DPE_READ_REGISTER copy_from_user failed");
				Ret = -EFAULT;
			}
			#endif
			break;
		}
	case DPE_WRITE_REGISTER:
		{
			LOG_INF("Not support DPE ioctl DPE_WRITE_REGISTER\n");
			#ifdef DPE_ioctl_en
			if (copy_from_user(&RegIo, (void *)Param,
				sizeof(struct DPE_REG_IO_STRUCT)) == 0) {
				Ret = DPE_WriteReg(&RegIo);
			} else {
				LOG_ERR(
				"DPE_WRITE_REGISTER copy_from_user failed");
				Ret = -EFAULT;
			}
			#endif
			break;
		}
	case DPE_WAIT_IRQ:
		{
			LOG_INF("Not support DPE ioctl DPE_WAIT_IRQ\n");
			#ifdef DPE_ioctl_en
			if (copy_from_user(&IrqInfo, (void *)Param,
				sizeof(struct DPE_WAIT_IRQ_STRUCT)) == 0) {

				if ((IrqInfo.Type >= DPE_IRQ_TYPE_AMOUNT) ||
							(IrqInfo.Type < 0)) {
					Ret = -EFAULT;
					LOG_ERR("invalid type(%d)",
								IrqInfo.Type);
					goto EXIT;
				}
				if ((IrqInfo.UserKey >= IRQ_USER_NUM_MAX) ||
							(IrqInfo.UserKey < 0)) {
					LOG_ERR(
					"invalid userKey(%d), max(%d), force userkey = 0\n",
						IrqInfo.UserKey,
						IRQ_USER_NUM_MAX);
						IrqInfo.UserKey = 0;
				}
				LOG_INF(
				"IRQ clear(%d), type(%d), userKey(%d), timeout(%d), status(0x%x)\n",
					IrqInfo.Clear, IrqInfo.Type,
					IrqInfo.UserKey, IrqInfo.Timeout,
					IrqInfo.Status);
				IrqInfo.ProcessID = pUserInfo->Pid;
				Ret = DPE_WaitIrq(&IrqInfo);
				if (copy_to_user((void *)Param, &IrqInfo,
				sizeof(struct DPE_WAIT_IRQ_STRUCT)) != 0) {
					LOG_ERR("copy_to_user failed\n");
					Ret = -EFAULT;
				}
			} else {
				LOG_ERR("DPE_WAIT_IRQ copy_from_user failed");
				Ret = -EFAULT;
			}
			#endif
			break;
		}
	case DPE_CLEAR_IRQ:
		{
			LOG_INF("Not support DPE ioctl DPE_CLEAR_IRQ\n");
			#ifdef DPE_ioctl_en
			if (copy_from_user(&ClearIrq, (void *)Param,
				sizeof(struct DPE_CLEAR_IRQ_STRUCT)) == 0) {
				LOG_INF("DPE_CLEAR_IRQ Type(%d)",
								ClearIrq.Type);
				if ((ClearIrq.Type >= DPE_IRQ_TYPE_AMOUNT) ||
							(ClearIrq.Type < 0)) {
					Ret = -EFAULT;
					LOG_ERR("invalid type(%d)",
								ClearIrq.Type);
					goto EXIT;
				}

				if ((ClearIrq.UserKey >= IRQ_USER_NUM_MAX)
				    || (ClearIrq.UserKey < 0)) {
					LOG_ERR("errUserEnum(%d)",
							ClearIrq.UserKey);
					Ret = -EFAULT;
					goto EXIT;
				}
				LOG_INF(
				"DPE_CLEAR_IRQ:Type(%d),Status(0x%08X),IrqStatus(0x%08X)\n",
					ClearIrq.Type, ClearIrq.Status,
					DPEInfo.IrqInfo.Status[ClearIrq.Type]);
				spin_lock_irqsave(
				&(DPEInfo.SpinLockIrq[ClearIrq.Type]), flags);
				DPEInfo.IrqInfo.Status[ClearIrq.Type] &=
							(~ClearIrq.Status);
				spin_unlock_irqrestore(
				&(DPEInfo.SpinLockIrq[ClearIrq.Type]), flags);
			} else {
				LOG_ERR(
				"DPE_CLEAR_IRQ copy_from_user failed\n");
				Ret = -EFAULT;
			}
			#endif
			break;
		}
	case DPE_ENQNUE_NUM:
		{
			if (copy_from_user(&enqueNum, (void *)Param,
				sizeof(int)) == 0) {
				if (DPE_REQUEST_STATE_EMPTY ==
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.WriteIdx].
				  State) {
					if ((enqueNum < 0) || (enqueNum >
						_SUPPORT_MAX_DPE_FRAME_REQUEST_)) {
						LOG_ERR(
						"DPE Enque Num is bigger Num:%d\n", enqueNum);
						break;
					}
					spin_lock_irqsave(
					&(DPEInfo.SpinLockIrq[
						DPE_IRQ_TYPE_INT_DVP_ST]),
									flags);
					g_DPE_ReqRing.DPEReq_Struct[
						g_DPE_ReqRing.WriteIdx].
							processID =
								pUserInfo->Pid;
					g_DPE_ReqRing.DPEReq_Struct[
						g_DPE_ReqRing.WriteIdx].
							enqueReqNum = enqueNum;
					spin_unlock_irqrestore(
					&(DPEInfo.SpinLockIrq[
					DPE_IRQ_TYPE_INT_DVP_ST]), flags);
					LOG_INF(
					"DPE_ENQNUE_NUM:%d\n", enqueNum);
				} else {
					LOG_ERR(
					"WFME Enque request state is not empty:%d, writeIdx:%d, readIdx:%d\n",
					     g_DPE_ReqRing.DPEReq_Struct[
						g_DPE_ReqRing.WriteIdx].State,
						g_DPE_ReqRing.WriteIdx,
						g_DPE_ReqRing.ReadIdx);
				}
			} else {
				LOG_ERR(
				"DPE_EQNUE_NUM copy_from_user failed\n");
				Ret = -EFAULT;
			}
			break;
		}
		/* struct DPE_Config_ISP8 */
	case DPE_ENQUE:
		{
			if (copy_from_user(&dpe_DpeConfig, (void *)Param,
					sizeof(struct DPE_Config_ISP8)) == 0) {
				/* LOG_DBG(
				 * "DPE_CLEAR_IRQ:Type(%d),Status(0x%08X),
				 * IrqStatus(0x%08X)",
				 * ClearIrq.Type, ClearIrq.Status,
				 * DPEInfo.IrqInfo.Status[ClearIrq.Type]);
				 */
				spin_lock_irqsave(
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
						  flags);
				if ((DPE_REQUEST_STATE_EMPTY ==
				    g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.WriteIdx].State)
				    && (g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.WriteIdx].FrameWRIdx <
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.WriteIdx].enqueReqNum)) {
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.WriteIdx].DpeFrameStatus[
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.WriteIdx].FrameWRIdx] =
					DPE_FRAME_STATUS_ENQUE;
					memcpy(&g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.WriteIdx].DpeFrameConfig[
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.WriteIdx].FrameWRIdx++],
					&dpe_DpeConfig,
					sizeof(struct DPE_Config_ISP8));
					if (g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.WriteIdx].FrameWRIdx ==
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.WriteIdx].enqueReqNum) {
						g_DPE_ReqRing.DPEReq_Struct[
						g_DPE_ReqRing.WriteIdx].State =
						DPE_REQUEST_STATE_PENDING;
						g_DPE_ReqRing.WriteIdx =
						    (g_DPE_ReqRing.WriteIdx +
						     1) %
					_SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
						LOG_INF("DPE enque done!!\n");
					} else {
						LOG_INF("DPE enque frame!!\n");
					}
				} else {
					LOG_ERR(
					"No Buffer! WriteIdx(%d), Stat(%d), FrameWRIdx(%d), enqueReqNum(%d)\n",
						g_DPE_ReqRing.WriteIdx,
						g_DPE_ReqRing.DPEReq_Struct[
						g_DPE_ReqRing.WriteIdx].State,
						g_DPE_ReqRing.DPEReq_Struct[
						g_DPE_ReqRing.WriteIdx].
								FrameWRIdx,
						g_DPE_ReqRing.DPEReq_Struct[
							g_DPE_ReqRing.WriteIdx].
								enqueReqNum);
				}
				spin_unlock_irqrestore(
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
						       flags);
				LOG_ERR("ConfigDPE Not Support\n");
			} else {
				LOG_ERR("DPE_ENQUE copy_from_user failed\n");
				Ret = -EFAULT;
			}
			break;
		}
	case DPE_ENQUE_REQ:
		{
			if (copy_from_user(&dpe_DpeReq, (void *)Param,
					sizeof(struct DPE_Request)) == 0) {
				LOG_INF("DPE_ENQNUE_NUM:%d, pid:%d\n",
					dpe_DpeReq.m_ReqNum, pUserInfo->Pid);
				if (dpe_DpeReq.m_ReqNum >
					_SUPPORT_MAX_DPE_FRAME_REQUEST_) {
					LOG_ERR(
					"DPE Enque Num is bigger than enqueNum:%d\n",
						dpe_DpeReq.m_ReqNum);
					Ret = -EFAULT;
					goto EXIT;
				}
				if (copy_from_user
					(g_DpeEnqueReq_Struct.DpeFrameConfig,
					(void *)dpe_DpeReq.m_pDpeConfig,
					dpe_DpeReq.m_ReqNum *
					sizeof(struct DPE_Config_ISP8)) != 0) {
					LOG_ERR(
					"copy DPEConfig from request fail!!\n");
					Ret = -EFAULT;
					goto EXIT;
				}
				//!mutex_lock(&gDpeMutex);
				//mutex_lock(&gDVSMutex);
				spin_lock_irqsave(
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
						  flags);
				kDpeReq.m_ReqNum = dpe_DpeReq.m_ReqNum;
				kDpeReq.m_pDpeConfig =
					g_DpeEnqueReq_Struct.DpeFrameConfig;
				LOG_INF("[DPE ioctl DPE ENQUE REQ] Dpe_engineSelect = %d\n",
				kDpeReq.m_pDpeConfig->Dpe_engineSelect);

				spin_unlock_irqrestore(
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
						       flags);
				LOG_INF("Config DPE Request!!\n");
				/* Use a workqueue to set CMDQ to prevent
				 * HW CMDQ request consuming speed from being
				 * faster than SW frame-queue update speed.
				 */
				if (kDpeReq.m_pDpeConfig->Dpe_engineSelect == MODE_DVS_ONLY) {
					req_temp = dpe_request_running_isp8(&dpe_reqs_dvs);
					//LOG_INF("[DPE ioctl]dpe_request_running star = %d\n",
					//req_temp);
					if (!req_temp) {
						//if (!dpe_request_running_isp8(&dpe_reqs)) {
						LOG_INF("[dvs]direct request_handler\n");
						dpe_request_handler_isp8(&dpe_reqs_dvs,
						&(DPEInfo.SpinLockIrq[
						DPE_IRQ_TYPE_INT_DVP_ST]));
					}
				}

				if ((kDpeReq.m_pDpeConfig->Dpe_engineSelect == MODE_DVP_ONLY) ||
					(kDpeReq.m_pDpeConfig->Dpe_engineSelect ==
					MODE_DVS_DVP_BOTH)) {
					req_temp = dpe_request_running_isp8(&dpe_reqs_dvp);
					LOG_INF("[DPE ioctl]dpe_request_running_isp8 start = %d\n",
					req_temp);
					if (!req_temp) {
						LOG_INF("[DPE ioctl DVP]direct request_handler\n");
						dpe_request_handler_isp8(&dpe_reqs_dvp,
						&(DPEInfo.SpinLockIrq[
						DPE_IRQ_TYPE_INT_DVP_ST]));
					}
				}
				if (kDpeReq.m_pDpeConfig->Dpe_engineSelect == MODE_DVGF_ONLY) {
					req_temp = dpe_request_running_isp8(&dpe_reqs_dvgf);
					LOG_INF("[DPE ioctl]dpe_request_running_isp8 start = %d\n",
					req_temp);
					if (!req_temp) {
						LOG_INF("[DPE ioctl DVP]direct request_handler\n");
						dpe_request_handler_isp8(&dpe_reqs_dvgf,
						&(DPEInfo.SpinLockIrq[
						DPE_IRQ_TYPE_INT_DVP_ST]));
					}
				}
//				mutex_unlock(&gDVSMutex);
				//mutex_unlock(&gDpeMutex);
			} else {
				LOG_ERR(
				"DPE_ENQUE_REQ copy_from_user failed\n");
				Ret = -EFAULT;
			}
			break;
		}
	case DPE_DEQUE_NUM:
		{
			if (DPE_REQUEST_STATE_FINISHED ==
			    g_DPE_ReqRing.DPEReq_Struct[
				g_DPE_ReqRing.ReadIdx].State) {
				dequeNum =
				    g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].enqueReqNum;
				LOG_INF("DPE_DEQUE_NUM(%d)\n", dequeNum);
			} else {
				dequeNum = 0;
				LOG_ERR(
				"DEQUE_NUM:No Buffer: ReadIdx(%d) State(%d) RrameRDIdx(%d) enqueReqNum(%d)\n",
					g_DPE_ReqRing.ReadIdx,
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].State,
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].RrameRDIdx,
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].enqueReqNum);
			}
			if (copy_to_user((void *)Param, &dequeNum,
						sizeof(unsigned int)) != 0) {
				LOG_ERR("DPE_DEQUE_NUM copy_to_user failed\n");
				Ret = -EFAULT;
			}
			break;
		}
	case DPE_DEQUE:
		{
			spin_lock_irqsave(
			&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]), flags);
			if ((DPE_REQUEST_STATE_FINISHED ==
				g_DPE_ReqRing.DPEReq_Struct[
				g_DPE_ReqRing.ReadIdx].State)
				&& (g_DPE_ReqRing.DPEReq_Struct[
				g_DPE_ReqRing.ReadIdx].RrameRDIdx <
				g_DPE_ReqRing.DPEReq_Struct[
				g_DPE_ReqRing.ReadIdx].enqueReqNum)) {
				if (DPE_FRAME_STATUS_FINISHED ==
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].DpeFrameStatus[
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].RrameRDIdx]) {
					memcpy(&dpe_DpeConfig,
					&g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].DpeFrameConfig[
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].RrameRDIdx],
					sizeof(struct DPE_Config_ISP8));
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].DpeFrameStatus[
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].RrameRDIdx++] =
					DPE_FRAME_STATUS_EMPTY;
				}
				if (g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].RrameRDIdx ==
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].enqueReqNum) {
					g_DPE_ReqRing.DPEReq_Struct[
						g_DPE_ReqRing.ReadIdx].State =
						DPE_REQUEST_STATE_EMPTY;
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].FrameWRIdx = 0;
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].RrameRDIdx = 0;
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].enqueReqNum = 0;
					g_DPE_ReqRing.ReadIdx =
					(g_DPE_ReqRing.ReadIdx + 1) %
					_SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
					LOG_INF("DPE ReadIdx(%d)\n",
							g_DPE_ReqRing.ReadIdx);
				}
				spin_unlock_irqrestore(
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
								flags);
				if (copy_to_user
					((void *)Param,
					&g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].DpeFrameConfig[
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].RrameRDIdx],
					sizeof(struct DPE_Config_ISP8)) != 0) {
					LOG_ERR(
					"DPE_DEQUE copy_to_user fail\n");
					Ret = -EFAULT;
				}
			} else {
				spin_unlock_irqrestore(
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
								flags);
				LOG_ERR("DPE_DEQUE No Buf: (%d)(%d)(%d)(%d)\n",
					g_DPE_ReqRing.ReadIdx,
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].State,
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].RrameRDIdx,
					g_DPE_ReqRing.DPEReq_Struct[
					g_DPE_ReqRing.ReadIdx].enqueReqNum);
			}
			break;
		}
	case DPE_DEQUE_REQ:
		{
			if (copy_from_user(&dpe_DpeReq, (void *)Param,
				sizeof(struct DPE_Request)) == 0) {
				//mutex_lock(&gDpeDequeMutex);
				//mutex_lock(&gDVSMutex);
				spin_lock_irqsave(
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
							flags);
				kDpeReq.m_pDpeConfig =
					g_DpeDequeReq_Struct.DpeFrameConfig;

				if (kDpeReq.m_pDpeConfig->Dpe_engineSelect == MODE_DVS_ONLY)
					dpe_deque_request_isp8(&dpe_reqs_dvs,
					&kDpeReq.m_ReqNum, &kDpeReq, pUserInfo->Pid);


				if ((kDpeReq.m_pDpeConfig->Dpe_engineSelect == MODE_DVP_ONLY) ||
					(kDpeReq.m_pDpeConfig->Dpe_engineSelect == 0))
					dpe_deque_request_isp8(&dpe_reqs_dvp,
					&kDpeReq.m_ReqNum, &kDpeReq, pUserInfo->Pid);

				if ((kDpeReq.m_pDpeConfig->Dpe_engineSelect == MODE_DVGF_ONLY) ||
					(kDpeReq.m_pDpeConfig->Dpe_engineSelect == 0))
					dpe_deque_request_isp8(&dpe_reqs_dvgf,
					&kDpeReq.m_ReqNum, &kDpeReq, pUserInfo->Pid);

				dequeNum = kDpeReq.m_ReqNum;
				dpe_DpeReq.m_ReqNum = dequeNum;
				spin_unlock_irqrestore(
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
						       flags);
				//mutex_unlock(&gDpeDequeMutex);
//				mutex_unlock(&gDVSMutex);
				if (dpe_DpeReq.m_pDpeConfig == NULL) {
					LOG_ERR("NULL ptr:DpeReq.m_pDpeConfig");
					Ret = -EFAULT;
					goto EXIT;
				}
				if (copy_to_user
					((void *)dpe_DpeReq.m_pDpeConfig,
					&g_DpeDequeReq_Struct.DpeFrameConfig[0],
					dequeNum *
					sizeof(struct DPE_Config_ISP8)) != 0) {
					LOG_ERR
					("DPE_DEQUE_REQ frmcfg failed\n");
					Ret = -EFAULT;
				}
				if (copy_to_user
					((void *)Param, &dpe_DpeReq,
					sizeof(struct DPE_Request)) != 0) {
					LOG_ERR("DPE_DEQUE_REQ DpeReq fail\n");
					Ret = -EFAULT;
				}
			} else {
				LOG_ERR("DPE_CMD_DPE_DEQUE_REQ failed\n");
				Ret = -EFAULT;
			}
			break;
		}
	default:
		{
			LOG_ERR("Unknown Cmd(%d)", Cmd);
			LOG_ERR("Cmd(%d),Dir(%d),Typ(%d),Nr(%d),Size(%d)\n",
				Cmd, _IOC_DIR(Cmd),
				_IOC_TYPE(Cmd), _IOC_NR(Cmd), _IOC_SIZE(Cmd));
			Ret = -EPERM;
			break;
		}
	}
	/*  */
EXIT:
	if (Ret != 0) {
		LOG_ERR("Fail Cmd(%d), Pid(%d), (proc, pid, tgid)=(%s, %d, %d)",
			Cmd,
			pUserInfo->Pid,
			current->comm,
			current->pid,
			current->tgid);
	}
	/*  */
	return Ret;
}
#if IS_ENABLED(CONFIG_COMPAT)
/*******************************************************************************
 *
 ******************************************************************************/
#ifdef alloc_user_space
static int compat_get_DPE_read_register_data(
			struct compat_DPE_REG_IO_STRUCT __user *data32,
					struct DPE_REG_IO_STRUCT __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err;

	err = get_user(uptr, &data32->pData);
	err |= put_user(compat_ptr(uptr), &data->pData);
	err |= get_user(count, &data32->Count);
	err |= put_user(count, &data->Count);
	return err;
}
static int compat_put_DPE_read_register_data(
			struct compat_DPE_REG_IO_STRUCT __user *data32,
					struct DPE_REG_IO_STRUCT __user *data)
{
	compat_uint_t count;
	/*compat_uptr_t uptr;*/
	int err = 0;

	/* Assume data pointer is unchanged. */
	/* err = get_user(compat_ptr(uptr), &data->pData); */
	/* err |= put_user(uptr, &data32->pData); */
	err |= get_user(count, &data->Count);
	err |= put_user(count, &data32->Count);
	return err;
}
static int compat_get_DPE_enque_req_data(
			struct compat_DPE_Request __user *data32,
					      struct DPE_Request __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pDpeConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pDpeConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}
static int compat_put_DPE_enque_req_data(
			struct compat_DPE_Request __user *data32,
					      struct DPE_Request __user *data)
{
	compat_uint_t count;
	/*compat_uptr_t uptr;*/
	int err = 0;

	/* Assume data pointer is unchanged. */
	/* err = get_user(compat_ptr(uptr), &data->m_pDpeConfig); */
	/* err |= put_user(uptr, &data32->m_pDpeConfig); */
	err |= get_user(count, &data->m_ReqNum);
	err |= put_user(count, &data32->m_ReqNum);
	return err;
}
static int compat_get_DPE_deque_req_data(
			struct compat_DPE_Request __user *data32,
					      struct DPE_Request __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pDpeConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pDpeConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}
static int compat_put_DPE_deque_req_data(
		struct compat_DPE_Request __user *data32,
					struct DPE_Request __user *data)
{
	compat_uint_t count;
	/*compat_uptr_t uptr;*/
	int err = 0;

	/* Assume data pointer is unchanged. */
	/* err = get_user(compat_ptr(uptr), &data->m_pDpeConfig); */
	/* err |= put_user(uptr, &data32->m_pDpeConfig); */
	err |= get_user(count, &data->m_ReqNum);
	err |= put_user(count, &data32->m_ReqNum);
	return err;
}
#endif
static long DPE_ioctl_compat(struct file *filp, unsigned int cmd,
							unsigned long arg)
{
	//!long ret;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		LOG_ERR("no f_op !!!\n");
		return -ENOTTY;
	}
	switch (cmd) {
	case COMPAT_DPE_READ_REGISTER:
		{
			#ifdef alloc_user_space
			struct compat_DPE_REG_IO_STRUCT __user *data32;
			struct DPE_REG_IO_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;
			err = compat_get_DPE_read_register_data(data32, data);
			if (err) {
				LOG_INF("compat_get_read_register_data err.\n");
				return err;
			}
			ret =
				filp->f_op->unlocked_ioctl(filp, DPE_READ_REGISTER,
								(unsigned long)data);
			err = compat_put_DPE_read_register_data(data32, data);
			if (err) {
				LOG_INF("compat_put_read_register_data err.\n");
				return err;
			}
			return ret;
			#endif
		}
		fallthrough;
	case COMPAT_DPE_WRITE_REGISTER:
		{
			#ifdef alloc_user_space
			struct compat_DPE_REG_IO_STRUCT __user *data32;
			struct DPE_REG_IO_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;
			err = compat_get_DPE_read_register_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_WRITE_REGISTER error!!!\n");
				return err;
			}
			ret =
				filp->f_op->unlocked_ioctl(filp, DPE_WRITE_REGISTER,
								(unsigned long)data);
			return ret;
			#endif
		}
		fallthrough;
	case COMPAT_DPE_ENQUE_REQ:
		{
			#ifdef alloc_user_space
			struct compat_DPE_Request __user *data32;
			struct DPE_Request __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;
			err = compat_get_DPE_enque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_ENQUE_REQ error!!!\n");
				return err;
			}
			ret =
				filp->f_op->unlocked_ioctl(filp, DPE_ENQUE_REQ,
								(unsigned long)data);
			err = compat_put_DPE_enque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_ENQUE_REQ error!!!\n");
				return err;
			}
			return ret;
		#endif
		}
		fallthrough;
	case COMPAT_DPE_DEQUE_REQ:
		{
			#ifdef alloc_user_space
			struct compat_DPE_Request __user *data32;
			struct DPE_Request __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;
			err = compat_get_DPE_deque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_DEQUE_REQ error!!!\n");
				return err;
			}
			ret =
				filp->f_op->unlocked_ioctl(filp, DPE_DEQUE_REQ,
								(unsigned long)data);
			err = compat_put_DPE_deque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_DEQUE_REQ error!!!\n");
				return err;
			}
			return ret;
			#endif
		}
		fallthrough;
	case DPE_WAIT_IRQ:
	case DPE_CLEAR_IRQ:	/* structure (no pointer) */
	case DPE_ENQNUE_NUM:
	case DPE_ENQUE:
	case DPE_DEQUE_NUM:
	case DPE_DEQUE:
	case DPE_RESET:
	case DPE_DUMP_REG:
	case DPE_DUMP_ISR_LOG:
		return filp->f_op->unlocked_ioctl(filp, cmd, arg);
	default:
		return -ENOIOCTLCMD;
		/* return DPE_ioctl(filep, cmd, arg); */
	}
}
#endif
/*******************************************************************************
 *
 ******************************************************************************/
static signed int DPE_open(struct inode *pInode, struct file *pFile)
{
	signed int Ret = 0;
	unsigned int i, j;
	/*int q = 0, p = 0;*/
	struct DPE_USER_INFO_STRUCT *pUserInfo;
	struct group token_group;

	LOG_INF("- E. UserCount: %d.", DPEInfo.UserCount);

	/*  */
	mutex_lock(&(MutexDPERef));
	pFile->private_data = NULL;
	pFile->private_data = kmalloc(sizeof(struct DPE_USER_INFO_STRUCT),
								GFP_ATOMIC);
	if (pFile->private_data == NULL) {
		LOG_DBG("kmalloc failed, (proc, pid, tgid)=(%s, %d, %d)",
								current->comm,
						current->pid, current->tgid);
		Ret = -ENOMEM;
		mutex_unlock(&(MutexDPERef));
		goto EXIT;
	} else {
		pUserInfo = (struct DPE_USER_INFO_STRUCT *) pFile->private_data;
		// pUserInfo->Pid = DPEInfo.UserCount;
		pUserInfo->Pid = DPEInfo.ServeCount;
		DPEInfo.ServeCount = (DPEInfo.ServeCount+1) % IRQ_USER_NUM_MAX;
		pUserInfo->Tid = current->tgid;
	}
	/*  */
	if (DPEInfo.UserCount > 0) {
		DPEInfo.UserCount++;
		mutex_unlock(&(MutexDPERef));
		LOG_DBG("Cur Usr(%d), (proc, pid, tgid)=(%s, %d, %d), exist",
			DPEInfo.UserCount, current->comm, current->pid,
								current->tgid);

		spin_lock(&(DPEInfo.SpinLockDPE));
		if (g_u4EnableClockCount == 0) {
			LOG_INF("wait for clock/power enable: %d", g_u4EnableClockCount);
			spin_unlock(&(DPEInfo.SpinLockDPE));
			wait_for_completion(&DPEinit_done);
		} else
			spin_unlock(&(DPEInfo.SpinLockDPE));

		goto EXIT;
	} else {
		DPEInfo.UserCount++;
		/* do wait queue head init when re-enter in camera */
		/*  */
		for (i = 0; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
			/* DPE */
			g_DPE_ReqRing.DPEReq_Struct[i].processID = 0x0;
			g_DPE_ReqRing.DPEReq_Struct[i].callerID = 0x0;
			g_DPE_ReqRing.DPEReq_Struct[i].enqueReqNum = 0x0;
			/* g_DPE_ReqRing.DPEReq_Struct[i].enqueIdx = 0x0; */
			g_DPE_ReqRing.DPEReq_Struct[i].State =
				DPE_REQUEST_STATE_EMPTY;
			g_DPE_ReqRing.DPEReq_Struct[i].FrameWRIdx = 0x0;
			g_DPE_ReqRing.DPEReq_Struct[i].RrameRDIdx = 0x0;
			for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_; j++) {
				g_DPE_ReqRing.DPEReq_Struct[i].DpeFrameStatus[
					j] = DPE_FRAME_STATUS_EMPTY;
			}
		}
		g_DPE_ReqRing.WriteIdx = 0x0;
		g_DPE_ReqRing.ReadIdx = 0x0;
		g_DPE_ReqRing.HWProcessIdx = 0x0;
		for (i = 0; i < DPE_IRQ_TYPE_AMOUNT; i++)
			DPEInfo.IrqInfo.Status[i] = 0;
		for (i = 0; i < _SUPPORT_MAX_DPE_FRAME_REQUEST_; i++)
			DPEInfo.ProcessID[i] = 0;
		DPEInfo.WriteReqIdx = 0;
		DPEInfo.ReadReqIdx = 0;
		/* DPEInfo.IrqInfo.DpeIrqCnt = 0; */
		for (i = 0; i < IRQ_USER_NUM_MAX; i++)
			DPEInfo.IrqInfo.DpeIrqCnt[i] = 0;

		/*dpe cmdq buf index reset 0*/
		DPE_cmdq_buf_idx = 0;
		/*  */
		//open dvs
		dpe_register_requests_isp8(&dpe_reqs_dvs, sizeof(struct DPE_Config_ISP8));
		dpe_set_engine_ops_isp8(&dpe_reqs_dvs, &dpe_ops);
		//open dvp
		dpe_register_requests_isp8(&dpe_reqs_dvp, sizeof(struct DPE_Config_ISP8));
		dpe_set_engine_ops_isp8(&dpe_reqs_dvp, &dpe_ops);

		dpe_register_requests_isp8(&dpe_reqs_dvgf, sizeof(struct DPE_Config_ISP8));
		dpe_set_engine_ops_isp8(&dpe_reqs_dvgf, &dpe_ops);

		//frame sync token init
		token_group.hw_group_id = dpe_engine;
		token_group.algo_group_id = mtk_imgsys_frm_sync_event_group_vsdof;
		mtk_imgsys_frm_sync_init(DPE_devs[0].frm_sync_pdev, token_group);
		mutex_unlock(&(MutexDPERef));
		//
		LOG_DBG("Cur Usr(%d), (proc, pid, tgid)=(%s, %d, %d), 1st user",
			DPEInfo.UserCount, current->comm, current->pid,
								current->tgid);
	}

	spin_lock(&(DPEInfo.SpinLockFD));
	DVS_only_en = 0;
	DVS_Num = 0;
	DVP_only_en = 0;
	DVP_Num = 0;
	DVGF_only_en = 0;
	DVGF_Num = 0;

	for (i = 0 ; i < DVS_BUF_TOTAL ; i++)
		get_dvs_iova[i] = 0;

	for (i = 0 ; i < DVP_BUF_TOTAL ; i++)
		get_dvp_iova[i] = 0;

	for (i = 0 ; i < DVGF_BUF_TOTAL ; i++)
		get_dvgf_iova[i] = 0;

	spin_unlock(&(DPEInfo.SpinLockFD));

	g_dvs_rdma_ttl_bw = 0;
	g_dvs_wdma_ttl_bw = 0;
	g_dvp_rdma_ttl_bw = 0;
	g_dvp_wdma_ttl_bw = 0;
	g_dvgf_rdma_ttl_bw = 0;
	g_dvgf_wdma_ttl_bw = 0;

	/* Enable clock */
	LOG_INF("DPE OPNE CLK UserCount: %d\n", DPEInfo.UserCount);
	DPE_EnableClock(MTRUE);
	cmdq_mbox_enable(dpe_clt->chan);
	//DPE_debug_log_en = 1;
	spin_lock(&(DPEInfo.SpinLockDPE));
	g_SuspendCnt = 0;
	LOG_INF("DPE open g_u4EnableClockCount: %d", g_u4EnableClockCount);
	spin_unlock(&(DPEInfo.SpinLockDPE));
	complete_all(&DPEinit_done);
	/*  */
/*#define KERNEL_LOG*/
#ifdef KERNEL_LOG
    /* In EP, Add DPE_DBG_WRITE_REG for debug. Should remove it after EP */
	DPEInfo.DebugMask = (DPE_DBG_INT | DPE_DBG_DBGLOG | DPE_DBG_WRITE_REG);
#endif
EXIT:
	LOG_INF("- X. Ret: %d. UserCount: %d.", Ret, DPEInfo.UserCount);
	return Ret;
}
/******************************************************************************
 *
 ******************************************************************************/
static signed int DPE_release(struct inode *pInode, struct file *pFile)
{
	struct DPE_USER_INFO_STRUCT *pUserInfo;
	struct group token_group;
	int i = 0;
	/*unsigned int Reg;*/
	LOG_DBG("- E. release UserCount: %d.", DPEInfo.UserCount);
	/*  */
	if (pFile->private_data != NULL) {
		pUserInfo =
			(struct  DPE_USER_INFO_STRUCT *) pFile->private_data;
		kfree(pFile->private_data);
		pFile->private_data = NULL;
	}
	/*  */
	mutex_lock(&(MutexDPERef));
	DPEInfo.UserCount--;
	if (DPEInfo.UserCount > 0) {
		LOG_INF("Cur UsrCnt(%d), (proc, pid, tgid)=(%s, %d, %d), exist",
			DPEInfo.UserCount, current->comm, current->pid,
								current->tgid);
		mutex_unlock(&(MutexDPERef));
		goto EXIT;
	} else {
		reinit_completion(&DPEinit_done);
		dpe_unregister_requests_isp8(&dpe_reqs_dvs);
		dpe_unregister_requests_isp8(&dpe_reqs_dvp);
		dpe_unregister_requests_isp8(&dpe_reqs_dvgf);
		//frame sync token uninit
		token_group.hw_group_id = dpe_engine;
		token_group.algo_group_id = mtk_imgsys_frm_sync_event_group_vsdof;
		mtk_imgsys_frm_sync_uninit(DPE_devs[0].frm_sync_pdev, token_group);
	}
	/*  */
	LOG_INF("Curr UsrCnt(%d), (process, pid, tgid)=(%s, %d, %d), last user",
		DPEInfo.UserCount, current->comm, current->pid, current->tgid);

	mutex_unlock(&(MutexDPERef));

	// MMQOS reset bw
	for (i = 0; i < DPE_MMQOS_RDMA_NUM; ++i) {
		if (icc_path_dpe_rdma[i])
			mtk_icc_set_bw(icc_path_dpe_rdma[i], 0, 0);
	}
	for (i = 0; i < DPE_MMQOS_WDMA_NUM; ++i) {
		if (icc_path_dpe_wdma[i])
			mtk_icc_set_bw(icc_path_dpe_wdma[i], 0, 0);
	}

	// larb19 setting
	if (DPE_devs[0].dev_ver == 0)
		mtk_cam_bwr_clr_bw(dpe_bwr_device, ENGINE_DPE, DISP_PORT);

	cmdq_mbox_disable(dpe_clt->chan);
	/* Disable clock. */
	DPE_EnableClock(MFALSE);
	spin_lock(&(DPEInfo.SpinLockDPE));
	LOG_INF("DPE release g_u4EnableClockCount: %d", g_u4EnableClockCount);
	spin_unlock(&(DPEInfo.SpinLockDPE));
	/*  */
EXIT:
	LOG_DBG("- X. UserCount: %d.", DPEInfo.UserCount);
	return 0;
}
/*******************************************************************************
 *
 ******************************************************************************/
static dev_t DPEDevNo;
static struct cdev *pDPECharDrv;
static struct class *pDPEClass;
static const struct file_operations DPEFileOper = {
	.owner = THIS_MODULE,
	.open = DPE_open,
	.release = DPE_release,
	/* .flush   = mt_DPE_flush, */
	/* .mmap = DPE_mmap, */
	.unlocked_ioctl = DPE_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = DPE_ioctl_compat,
#endif
};
/*******************************************************************************
 *
 ******************************************************************************/
static inline void DPE_UnregCharDev(void)
{
	LOG_DBG("- E.");
	/*  */
	/* Release char driver */
	if (pDPECharDrv != NULL) {
		cdev_del(pDPECharDrv);
		pDPECharDrv = NULL;
	}
	/*  */
	unregister_chrdev_region(DPEDevNo, 1);
}
/*******************************************************************************
 *
 ******************************************************************************/
static inline signed int DPE_RegCharDev(void)
{
	signed int Ret = 0;
	/*  */
	LOG_INF("- E.");
	/*  */
	Ret = alloc_chrdev_region(&DPEDevNo, 0, 1, DPE_DEV_NAME);
	if (Ret < 0) {
		LOG_ERR("alloc_chrdev_region failed, %d", Ret);
		return Ret;
	}
	/* Allocate driver */
	pDPECharDrv = cdev_alloc();
	if (pDPECharDrv == NULL) {
		LOG_ERR("cdev_alloc failed");
		Ret = -ENOMEM;
		goto EXIT;
	}
	/* Attath file operation. */
	cdev_init(pDPECharDrv, &DPEFileOper);
	/*  */
	pDPECharDrv->owner = THIS_MODULE;
	/* Add to system */
	Ret = cdev_add(pDPECharDrv, DPEDevNo, 1);
	if (Ret < 0) {
		LOG_ERR("Attath file operation failed, %d", Ret);
		goto EXIT;
	}
	/*  */
EXIT:
	if (Ret < 0)
		DPE_UnregCharDev();
	/*  */
	LOG_INF("- X.");
	return Ret;
}
/*******************************************************************************
 * V4L2
 ******************************************************************************/
static int dpe_fop_open(struct file *filp)
{
	DPE_open(NULL, filp);
	return 0;
}
static int dpe_fop_release(struct file *file)
{
	DPE_release(NULL, file);
	return 0;
}
unsigned int dpe_fop_poll(struct file *file, poll_table *wait)
{
	struct DPE_USER_INFO_STRUCT *pUserInfo;
	unsigned int buf_rdy = 0;
	unsigned long flags;
	unsigned int p = 0;

	if (DPE_debug_log_en == 1)
		LOG_INF("DPE Poll start\n");

	//DPE_DumpReg();
	pUserInfo = (struct DPE_USER_INFO_STRUCT *) (file->private_data);
	poll_wait(file, &DPEInfo.WaitQueueHead, wait);

	buf_rdy = DPE_GetIRQState(DPE_IRQ_TYPE_INT_DVP_ST,
			0x0,
			DPE_INT_ST, DPE_PROCESS_ID_DPE,
			pUserInfo->Pid);

	p = pUserInfo->Pid % IRQ_USER_NUM_MAX;
	LOG_INF("buf_rdy = %d, p=%d\n", buf_rdy, p);

	if (buf_rdy) {
		spin_lock_irqsave
		(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]), flags);
		DPEInfo.IrqInfo.DpeIrqCnt[p]--;
		//LOG_INF("DpeIrqCnt--\n");
		if (DPEInfo.IrqInfo.DpeIrqCnt[p] == 0)
			DPEInfo.IrqInfo.Status[DPE_IRQ_TYPE_INT_DVP_ST] &= (~DPE_INT_ST);
		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]), flags);
		return POLLIN | POLLRDNORM;
	} else
		return 0;
}
static const struct v4l2_file_operations vid_fops = {
	.owner          = THIS_MODULE,
	.open           = dpe_fop_open,
	.release        = dpe_fop_release,
	.poll           = dpe_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	/* TODO */
	//.mmap           = vb2_fop_mmap,
	//.read           = vb2_fop_read,
	//.write          = vb2_fop_write,
};

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	/*struct video_device *vdev = video_devdata(file);*/
	unsigned long ret;
	struct DPE_USER_INFO_STRUCT *pUserInfo;
	static struct DPE_Request ureq[MAX_REQ];
	static struct DPE_Request kreq[MAX_REQ];
	/* size of cfgs = 3 owing to call stact limitation*/
	static struct DPE_Config_ISP8 cfgs[MAX_REQ][3];//[MAX_FRAMES_PER_REQUEST];
	struct DPE_Config_ISP8 *pcfgs;
	//unsigned long flags;
	unsigned int m_real_ReqNum, f;
	int temp_req;
	//unsigned int p_cnt;
	//pid_t ProcessID;

	unsigned int qq;

	if (DPEInfo.UserCount <= 0) {
		LOG_ERR("[%s]UserCount is zero\n", __func__);
		ret = -1;
		goto EXIT;
	}

	spin_lock(&REQ_LOCK);
	qq = reqidx;
	reqidx = (reqidx+1) % MAX_REQ;
	// LOG_INF("[%s][ERIC]qq= %d\n", __func__, qq);
	spin_unlock(&REQ_LOCK);

	//int tmep_cnt;

	if ((p == NULL) || (file == NULL)) {
		LOG_ERR("[%s]input pointer is NULL\n", __func__);
		ret = -EFAULT;
		goto EXIT;
	}

	if (DPE_debug_log_en == 1) {
		LOG_INF("[%s]buf address/len = %lu/0x%x\n",
		__func__, p->m.userptr,  p->length);
	}
	pUserInfo = (struct DPE_USER_INFO_STRUCT *) (file->private_data);
	ret = copy_from_user(&ureq[qq], (void __user *)p->m.userptr, sizeof(struct DPE_Request));
	if (ret != 0) {
		LOG_ERR("[%s]copy_from_user fail\n", __func__);
		goto EXIT;
	}

	if (ureq[qq].m_ReqNum > 3) {
		LOG_ERR("[%s]user req nums is bigger than 3\n", __func__);
		ret = -1;
		goto EXIT;
	}

	//LOG_INF("[%s]This request has %d configs.\n", __func__, ureq[qq].m_ReqNum);
	if (ureq[qq].m_pDpeConfig == NULL) {
		LOG_ERR("[%s]user's DpeConfig is NULL\n", __func__);
		ret = -EFAULT;
		goto EXIT;
	}
	ret = copy_from_user(&cfgs[qq][0], (void __user *)ureq[qq].m_pDpeConfig,
				ureq[qq].m_ReqNum * sizeof(struct DPE_Config_ISP8));
	if (ret != 0) {
		LOG_ERR("[%s]DpeConfig copy_from_user fail\n", __func__);
		ret = -EFAULT;
		goto EXIT;
	}

	m_real_ReqNum = ureq[qq].m_ReqNum;
	for (f = 0; f < ureq[qq].m_ReqNum; f++) {
		if (cfgs[qq][f].Dpe_DVSSettings.is_pd_mode) {
			pcfgs = &cfgs[qq][f];
			Get_Tile_Info(pcfgs);
			m_real_ReqNum += (cfgs[qq][f].Dpe_DVSSettings.pd_frame_num-1);
		}
	}
	kreq[qq].m_pDpeConfig = &cfgs[qq][0];
	kreq[qq].m_ReqNum = m_real_ReqNum;

	//LOG_INF("[vidi qbuf] Dpe engineSelect = %d\n",
	//cfgs[qq].Dpe_engineSelect);
	//LOG_INF("[vidi qbuf] Dpe_RegDump = %d\n",
	//cfgs[qq].Dpe_RegDump);
	/* DPE_debug_log_en = cfgs[qq][0].Dpe_RegDump; */

	//kreq.m_ReqNum = ureq[qq].m_ReqNum;
	//mutex_lock(&gDpeMutex);	/* Protect the Multi Process */
	//mutex_lock(&gDVSMutex);

	//spin_lock_irqsave(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
	// flags);
	if (cfgs[qq][0].Dpe_engineSelect == MODE_DVS_ONLY)
		dpe_enque_request_isp8(&dpe_reqs_dvs, kreq[qq].m_ReqNum, &kreq[qq],
			pUserInfo->Pid, &(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));

	if ((cfgs[qq][0].Dpe_engineSelect == MODE_DVP_ONLY) ||
			(cfgs[qq][0].Dpe_engineSelect == MODE_DVS_DVP_BOTH))
		dpe_enque_request_isp8(&dpe_reqs_dvp, kreq[qq].m_ReqNum, &kreq[qq],
			pUserInfo->Pid, &(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));

	if (cfgs[qq][0].Dpe_engineSelect == MODE_DVGF_ONLY)
		dpe_enque_request_isp8(&dpe_reqs_dvgf, kreq[qq].m_ReqNum, &kreq[qq],
			pUserInfo->Pid, &(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));


	//spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
	// flags);
	/* Use a workqueue to set CMDQ to prevent HW CMDQ request
	 *  consuming speed from being faster than SW frame-queue update speed.
	 */

	if (cfgs[qq][0].Dpe_engineSelect == MODE_DVS_ONLY) {
		temp_req = dpe_request_running_isp8(&dpe_reqs_dvs);
		if (DPE_debug_log_en == 1)
			LOG_INF("[vidioc qbuf]dpe_request_running_isp8 stat = %d\n", temp_req);

		if (!temp_req) {
			//if (!dpe_request_running_isp8(&dpe_reqs)) {
			//LOG_INF("[vidi_qbuf]direct request_handler\n");
			dpe_request_handler_isp8(&dpe_reqs_dvs,
			&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
		}
	}
	if ((cfgs[qq][0].Dpe_engineSelect == MODE_DVP_ONLY) ||
		(cfgs[qq][0].Dpe_engineSelect == MODE_DVS_DVP_BOTH)) {
		temp_req = dpe_request_running_isp8(&dpe_reqs_dvp);
		if (DPE_debug_log_en == 1)
			LOG_INF("[vidioc qbuf]dpe_request_running_isp8 stat = %d\n", temp_req);

		if (!temp_req) {
			//if (!dpe_request_running_isp8(&dpe_reqs)) {
			//LOG_INF("[vidioc_qbuf]direct request_handler\n");
			dpe_request_handler_isp8(&dpe_reqs_dvp,
			&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
			}
		}

		if (cfgs[qq][0].Dpe_engineSelect == MODE_DVGF_ONLY) {
			temp_req = dpe_request_running_isp8(&dpe_reqs_dvgf);
			if (DPE_debug_log_en == 1)
				LOG_INF("[vidioc qbuf]dpe_request_running_isp8 stat = %d\n",
				temp_req);

			if (!temp_req) {
				//if (!dpe_request_running_isp8(&dpe_reqs)) {
				//LOG_INF("[vidioc_qbuf]direct request_handler\n");
				dpe_request_handler_isp8(&dpe_reqs_dvgf,
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
			}
		}
	//mutex_unlock(&gDpeMutex);
//	mutex_unlock(&gDVSMutex);

EXIT:
	return ret;
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	/*struct video_device *vdev = video_devdata(file);*/
	signed int Ret = 0;
	static struct DPE_Request ureq[MAX_REQ];
	static struct DPE_Request kreq[MAX_REQ];
	/* size of cfgs = 3 owing to call stact limitation*/
	static struct DPE_Config_ISP8 cfgs[MAX_REQ][3];//[MAX_FRAMES_PER_REQUEST];
	//unsigned long flags;
	//unsigned int m_real_ReqNum;
	struct DPE_USER_INFO_STRUCT *pUserInfo;

	unsigned int dd;

	if (DPEInfo.UserCount <= 0) {
		LOG_ERR("[%s]UserCount is zero\n", __func__);
		Ret = -1;
		goto EXIT;
	}

	spin_lock(&REQ_LOCK);
	dd = reqidx;
	reqidx = (reqidx+1) % MAX_REQ;
	// LOG_INF("[%s][ERIC]dd= %d\n", __func__, dd);
	spin_unlock(&REQ_LOCK);
	//struct DPE_Config_ISP8 *pDpeConfig;

	if ((p == NULL) || (file == NULL)) {
		LOG_ERR("[%s]input pointer is NULL\n", __func__);
		Ret = -EFAULT;
		goto EXIT;
	}

	pUserInfo = (struct DPE_USER_INFO_STRUCT *) (file->private_data);

	if (DPE_debug_log_en == 1) {
		LOG_INF("DPE_DumpReg  start\n");
		DPE_DumpReg();//!test
		LOG_INF("DPE_DumpReg end\n");
	}

	Ret = copy_from_user(&ureq[dd], (void __user *)p->m.userptr, sizeof(struct DPE_Request));

	if (Ret != 0) {
		LOG_ERR("DPE_deque_request copy_from_user failed\n");
		// Ret = -EFAULT;
		goto EXIT;
	}

	if (ureq[dd].m_ReqNum > 3) {
		LOG_ERR("[%s]user req nums is bigger than 3\n", __func__);
		Ret = -1;
		goto EXIT;
	}

	if (ureq[dd].m_pDpeConfig == NULL) {
		LOG_ERR("[%s]user's DpeConfig is NULL\n", __func__);
		Ret = -EFAULT;
		goto EXIT;
	}

	//LOG_INF("[%s]buf address/len = 0x%llu/0x%x, ureq[reqidx] =0x%x\n",
	//__func__, p->m.userptr,  p->length, sizeof(ureq[reqidx]));
	Ret = copy_from_user(&cfgs[dd][0], (void __user *)ureq[dd].m_pDpeConfig,
				ureq[dd].m_ReqNum * sizeof(struct DPE_Config_ISP8));

	if (Ret == 0) {

		//!mutex_lock(&gDpeDequeMutex);
		//mutex_lock(&gDVSMutex);
		kreq[dd].m_pDpeConfig = &cfgs[dd][0];
		if (DPE_debug_log_en == 1) {
			LOG_INF("[vidioc dqbuf] Dpe_engineSelect = %d\n",
			cfgs[dd][0].Dpe_engineSelect);
		}
		if (cfgs[dd][0].Dpe_engineSelect == MODE_DVS_ONLY)
			dpe_deque_request_isp8(&dpe_reqs_dvs, &kreq[dd].m_ReqNum,
			&kreq[dd], pUserInfo->Pid);

		if ((cfgs[dd][0].Dpe_engineSelect == MODE_DVP_ONLY) ||
				(cfgs[dd][0].Dpe_engineSelect == MODE_DVS_DVP_BOTH))
			dpe_deque_request_isp8(&dpe_reqs_dvp, &kreq[dd].m_ReqNum,
			&kreq[dd], pUserInfo->Pid);

		if (cfgs[dd][0].Dpe_engineSelect == MODE_DVGF_ONLY)
			dpe_deque_request_isp8(&dpe_reqs_dvgf, &kreq[dd].m_ReqNum,
			&kreq[dd], pUserInfo->Pid);


		//spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]),
		//		flags);
		//!mutex_unlock(&gDpeDequeMutex);
//		mutex_unlock(&gDVSMutex);
		ureq[dd].m_ReqNum = kreq[0].m_ReqNum;
		if (ureq[dd].m_pDpeConfig == NULL) {
			LOG_ERR("NULL user pointer");
			Ret = -EFAULT;
			goto EXIT;
		}
		//For Register Dump
		//LOG_INF("[vidioc dqbuf] b Dpe_RegDump = %d\n",
		//cfgs[dd].Dpe_RegDump);

		if (cfgs[dd][0].Dpe_RegDump == 1)
			DPE_Dump_kernelReg(&cfgs[dd][0]);//!Kernel Dump

		//LOG_INF("[vidioc dqbuf] b DVS_CTRL00 = 0x%x\n",
		//cfgs[dd].DPE_Kernel_DpeConfig.DVS_CTRL00);
		if (copy_to_user
		    ((void *)ureq[dd].m_pDpeConfig, kreq[dd].m_pDpeConfig,
		     kreq[dd].m_ReqNum * sizeof(struct DPE_Config_ISP8)) != 0) {
			LOG_ERR
			    ("DPE_DEQUE_REQ copy_to_user frameconfig failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}
		if (copy_to_user
		    ((void *)p->m.userptr, &ureq[dd], sizeof(struct DPE_Request)) != 0) {
			LOG_ERR("DPE_DEQUE_REQ copy_to_user failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}

	} else {
		LOG_ERR("DPE_CMD_DPE_DEQUE_REQ copy_from_user failed\n");
		Ret = -EFAULT;
		goto EXIT;
	}

	//LOG_INF("[%s]buf address/len = %lu/0x%x\n",
	//	__func__, p->m.userptr,  p->length);
EXIT:
	return Ret;
}

static int vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	struct DPE_device *dev = video_drvdata(file);
	/*struct video_device *vdev = video_devdata(file);*/
	strscpy(cap->driver, "dpe", sizeof(cap->driver));
	strscpy(cap->card, "dpe", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
			"platform:%s", dev->v4l2_dev.name);

cap->device_caps =
	V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

int vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	/*struct DPE_device *dev = video_drvdata(file);*/
	f->fmt.pix.width       = 777;
	f->fmt.pix.height      = 555;
	f->fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	f->fmt.pix.field       = V4L2_FIELD_INTERLACED;
	f->fmt.pix.sizeimage = f->fmt.pix.width * f->fmt.pix.height;
	LOG_ERR("[%s] sizeimage(%d)\n", __func__, f->fmt.pix.sizeimage);
	return 0;
}

static const struct v4l2_ioctl_ops vid_ioctl_ops = {
	.vidioc_querycap		   = vidioc_querycap,
	.vidioc_g_fmt_vid_cap	= vidioc_g_fmt_vid_cap,
	.vidioc_qbuf			   = vidioc_qbuf,
	.vidioc_dqbuf			   = vidioc_dqbuf,
	//.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	//.vidioc_create_bufs		= vb2_ioctl_create_bufs,
	//.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	//.vidioc_querybuf		   = vb2_ioctl_querybuf,
	//.vidioc_expbuf			   = vb2_ioctl_expbuf,
	//.vidioc_streamon		   = vb2_ioctl_streamon,
	//.vidioc_streamoff		   = vb2_ioctl_streamoff,
};
static void dpe_dev_release(struct v4l2_device *v4l2_dev)
{
	struct DPE_device *dev =
		container_of(v4l2_dev, struct DPE_device, v4l2_dev);
	v4l2_device_unregister(&dev->v4l2_dev);
}
/*******************************************************************************
 *
 ******************************************************************************/
struct platform_device *dpe_get_frm_sync_pdev(struct device *dev)
{
	struct device_node *dev_node;
	struct platform_device *img_frm_sync_pdev = NULL;

	dev_node = of_parse_phandle(dev->of_node, "mtk,img-frm-sync", 0);

	if (dev_node == NULL) {
		dev_err(dev, "%s can not get dev node", __func__);
		return NULL;
	}

	img_frm_sync_pdev = of_find_device_by_node(dev_node);
	//_dpe_dev->frm_sync_pdev = img_frm_sync_pdev;
	if (img_frm_sync_pdev == NULL) {
		dev_err(dev, "%s img_frm_sync_pdev failed\n", __func__);
		of_node_put(dev_node);
		return NULL;
	}

	return img_frm_sync_pdev;
}
/*******************************************************************************
 *
 ******************************************************************************/
static signed int DPE_probe(struct platform_device *pDev)
{
	signed int Ret = 0;
	/*struct resource *pRes = NULL;*/
	signed int i = 0;
	unsigned char n;
	int larbs_num = 0;
#if DPE_IRQ_ENABLE
	unsigned int irq_info[3];
#endif
	struct device *dev = NULL;
	struct DPE_device *_dpe_dev;
	int Get_SMMU = 0;

#ifdef CMASYS_CLK_Debug
	struct CAM_device *_cam_dev;
	struct IPE_device *_ipe_dev;
#endif

	struct device_link *link;
	struct video_device *vfd = NULL;
#ifndef EP_NO_CLKMGR
	struct device_node *node;
#endif
	int ret;
#if IS_ENABLED(CONFIG_OF)
	struct DPE_device *DPE_dev;
	#ifdef CMASYS_CLK_Debug
	struct CAM_device *Cam_dev;
	struct IPE_device *Ipe_dev;
	#endif
	const struct of_device_id *dts_match = NULL;
#endif
	LOG_INF("- E. DPE driver probe.\n");
	/* Check platform_device parameters */
#if IS_ENABLED(CONFIG_OF)
	if (pDev == NULL) {
		dev_dbg(&pDev->dev, "pDev is NULL");
		return -ENXIO;
	}
	nr_DPE_devs += 1;
	_dpe_dev = krealloc(DPE_devs, sizeof(struct DPE_device) * nr_DPE_devs,
							GFP_KERNEL|__GFP_ZERO);
	if (!_dpe_dev) {
		dev_dbg(&pDev->dev, "Unable to allocate DPE_devs\n");
		return -ENOMEM;
	}
	DPE_devs = _dpe_dev;
	DPE_dev = &(DPE_devs[nr_DPE_devs - 1]);
	DPE_dev->dev = &pDev->dev;
	//--------------
#ifdef CMASYS_CLK_Debug
	_cam_dev = krealloc(CAM_devs, sizeof(struct CAM_device),
							GFP_KERNEL|__GFP_ZERO);
	if (!_cam_dev) {
		dev_dbg(&pDev->dev, "Unable to allocate CAM_devs\n");
		return -ENOMEM;
	}
	CAM_devs = _cam_dev;
	Cam_dev = &(CAM_devs[0]);
	Cam_dev->dev = &pDev->dev;

	//--------------
	_ipe_dev = krealloc(IPE_devs, sizeof(struct IPE_device),
							GFP_KERNEL|__GFP_ZERO);
	if (!_ipe_dev) {
		dev_dbg(&pDev->dev, "Unable to allocate IPE_devs\n");
		return -ENOMEM;
	}
	IPE_devs = _ipe_dev;
	Ipe_dev = &(IPE_devs[0]);
	Ipe_dev->dev = &pDev->dev;
#endif

	//_dpe_dev = NULL;
	//DPE_devs = NULL;
	/* iomap registers */
	DPE_dev->regs = of_iomap(pDev->dev.of_node, 0);
		LOG_INF("- E. DPE_dev->regs = 0x%p\n", DPE_dev->regs);
	if (nr_DPE_devs == 1) {
		#ifdef CMASYS_CLK_Debug
		Cam_dev->regs = of_iomap(pDev->dev.of_node, 1);
			LOG_INF("- E. CAM_CLK = 0x%p\n", Cam_dev->regs);
		Ipe_dev->regs = of_iomap(pDev->dev.of_node, 2);
			LOG_INF("- E. IPE_CLK = 0x%p\n", Ipe_dev->regs);
		#endif
		DPE_dev->frm_sync_pdev = dpe_get_frm_sync_pdev(DPE_dev->dev);
		if (!DPE_dev->frm_sync_pdev)
			LOG_INF("get frm sync pdev fail\n");
	}
	if (!DPE_dev->regs) {
		dev_dbg(&pDev->dev,
			"of_iomap fail, nr_DPE_devs=%d, devnode(%s).\n",
			nr_DPE_devs, pDev->dev.of_node->name);
		return -ENOMEM;
	}

	LOG_INF("nr_DPE_devs=%d, devnode(%s), map_addr=%lu\n", nr_DPE_devs,
		pDev->dev.of_node->name, (unsigned long)DPE_dev->regs);
	//for cmdq malibox
	if (nr_DPE_devs == 1) {
		/* register device by node */
		dpe_clt_base = NULL;
		/* request thread by index (in dts) 0 */
		#ifdef CMdq_en
		dpe_clt = cmdq_mbox_create(&pDev->dev, 0);
		#endif
		LOG_INF("[Debug]cmdq_mbox_create %lu\n",
			(unsigned long)dpe_clt);
/* parse hardware event */
		of_property_read_u32(pDev->dev.of_node,
				"dvsdoneasyncshot",
				&dvs_event_id);
		LOG_INF("[Debug]dvs_event_id %d\n", dvs_event_id);

		dts_match = &DPE_of_ids[1];
		if (of_device_is_compatible(pDev->dev.of_node, dts_match->compatible)) {
			if (strcmp(dts_match->compatible, "mediatek,dvs") == 0) {
				DPE_BASE_HW = 0x3A770000;
				DPE_devs[0].dev_ver = 0;
				LOG_INF("[Debug]mt6991 match\n");
			}
		}
		dts_match = &DPE_of_ids[4];
		if (of_device_is_compatible(pDev->dev.of_node, dts_match->compatible)) {
			if (strcmp(dts_match->compatible, "mediatek,dvs_mt6899") == 0) {
				DPE_BASE_HW = 0x1A770000;
				DPE_devs[0].dev_ver = 1;
				LOG_INF("[Debug]mt6899 match\n");
			}
		}
	} else if (nr_DPE_devs == 2) {
/* parse hardware event */
		of_property_read_u32(pDev->dev.of_node,
				"dvpdoneasyncshot",
				&dvp_event_id);
		LOG_INF("[Debug]dvp_event_id %d\n", dvp_event_id);
	} else if (nr_DPE_devs == 3) {
/* parse hardware event */
		of_property_read_u32(pDev->dev.of_node,
				"dvgfdoneasyncshot",
				&dvgf_event_id);
		LOG_INF("[Debug]dvgf_event_id %d\n", dvgf_event_id);
	}
#if DPE_IRQ_ENABLE
	/* get IRQ ID and request IRQ */
	DPE_dev->irq = irq_of_parse_and_map(pDev->dev.of_node, 0);
if (DPE_dev->irq > 0) {
	/* Get IRQ Flag from device node */
	if (of_property_read_u32_array(pDev->dev.of_node,
		"interrupts",
		irq_info,
		ARRAY_SIZE(irq_info))) {
		dev_dbg(&pDev->dev, "get irq flags from DTS fail!!\n");
		return -ENODEV;
	}
	for (i = 0; i < DPE_IRQ_TYPE_AMOUNT; i++) {
		if (strcmp(pDev->dev.of_node->name,
			DPE_IRQ_CB_TBL[i].device_name) == 0) {
			Ret =
			request_irq(DPE_dev->irq,
				(irq_handler_t)
				DPE_IRQ_CB_TBL[i].isr_fp,
				irq_info[2],
				(const char *)DPE_IRQ_CB_TBL[i].device_name,
				NULL);
			if (Ret) {
				dev_dbg(&pDev->dev,
				"devdbg: nr_DPE_devs=%d, devnode(%s), irq=%d, ISR: %s\n",
				nr_DPE_devs,
				pDev->dev.of_node->name,
				DPE_dev->irq,
				DPE_IRQ_CB_TBL[i].device_name);
				return Ret;
			}
			LOG_INF(
			"nr_DPE_devs=%d, devnode(%s), irq=%d, ISR: %s\n",
			nr_DPE_devs,
			pDev->dev.of_node->name,
			DPE_dev->irq,
			DPE_IRQ_CB_TBL[i].device_name);
			break;
		}
	}
	if (i >= DPE_IRQ_TYPE_AMOUNT) {
		LOG_INF("No ISR:nr_DPE_devs=%d, devnode(%s), irq=%d\n",
			nr_DPE_devs, pDev->dev.of_node->name,
							DPE_dev->irq);
	}
	} else {
		LOG_INF("No IRQ!!: nr_DPE_devs=%d, devnode(%s), irq=%d\n",
			nr_DPE_devs,
			pDev->dev.of_node->name, DPE_dev->irq);
	}
#endif
#endif
	// if (!pm_runtime_enabled(DPE_dev->dev))
		// goto EXIT;
	ret = dma_set_max_seg_size(DPE_dev->dev, (unsigned int)DMA_BIT_MASK(34));
	if (ret) {
		dev_dbg(DPE_dev->dev, "Failed to set DMA segment size\n");
		goto EXIT;
	}
	/* Only register char driver in the 1st time */
	if (nr_DPE_devs == 3) {
		DPE_dev->clks = isp8_dpe_clks;
		DPE_dev->clk_num = ARRAY_SIZE(isp8_dpe_clks);
		dev_set_drvdata(&pDev->dev, DPE_dev);
		pm_runtime_enable(DPE_dev->dev);

		/* Register char driver */
		Ret = DPE_RegCharDev();
		if (Ret) {
			dev_dbg(&pDev->dev, "register char failed");
			return Ret;
		}
#ifndef EP_NO_CLKMGR
#if !IS_ENABLED(CONFIG_MTK_LEGACY) && IS_ENABLED(CONFIG_COMMON_CLK) /*CCF*/
#ifdef SMI_CLK
		LOG_INF("nr_DPE_devs=%d, devnode(%s)\n", nr_DPE_devs,
		pDev->dev.of_node->name);
#if CHECK_SERVICE_IF_0
		node = of_parse_phandle(pDev->dev.of_node, "mediatek,camisp-vcore", 0);
		LOG_INF("camisp_vcore node get\n");
		if (!node) {
			LOG_INF("no get camisp_vcore node\n");
			return -EINVAL;
		}

		DPE_pdev = of_find_device_by_node(node);
		if (WARN_ON(!DPE_pdev)) {
			of_node_put(node);
			return -EINVAL;
		}
		of_node_put(node);
		DPE_devs->camisp_vcore = &DPE_pdev->dev;

		LOG_INF("Get camisp_vcore device link\n");
		link = device_link_add(&pDev->dev, &DPE_pdev->dev,
		DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);

		if (!link) {
			LOG_INF("%s camisp_vcore device link fail", __func__);
			return -EPROBE_DEFER;
		}
#endif
		larbs_num = of_count_phandle_with_args(pDev->dev.of_node,
											"mediatek-larb-supply", NULL);
		LOG_INF("Find %d larbs", larbs_num);
		if (larbs_num <= 0) {
			larbs_num = 0;
			goto bypass_larbs;
		}
		node = of_parse_phandle(pDev->dev.of_node, "mediatek-larb-supply", 0);
		LOG_INF("larb19 node get\n");
		if (!node) {
			LOG_INF("no get larb19 node\n");
			return -EINVAL;
		}

		DPE_pdev = of_find_device_by_node(node);
		if (WARN_ON(!DPE_pdev)) {
			of_node_put(node);
			return -EINVAL;
		}
		of_node_put(node);
		DPE_devs->larb19 = &DPE_pdev->dev;

		LOG_INF("Get larb19 device link\n");
		link = device_link_add(&pDev->dev, &DPE_pdev->dev,
		DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);

		if (!link) {
			LOG_INF("%s smi larb device link fail", __func__);
			return -EPROBE_DEFER;
		}
#endif
bypass_larbs:
		/*CCF: Grab clock pointer (struct clk*) */
		LOG_INF(" get clock node star\n");
///

// #if CHECK_SERVICE_IF_0
		// dpe_clk.CLK_CK2_DPE_SEL = devm_clk_get(&pDev->dev,
		// "CLK_CK2_DPE_SEL");
		// if (IS_ERR(dpe_clk.CLK_CK2_DPE_SEL))
		// LOG_ERR("cannot get CLK_CK2_DPE_SEL clock\n");

		dpe_clk.CLK_CAM_MAIN_CAM = devm_clk_get(&pDev->dev,
							"CLK_CAM_MAIN_CAM");
		if (IS_ERR(dpe_clk.CLK_CAM_MAIN_CAM))
			LOG_ERR("cannot get CLK_CAM_MAIN_CAM clock\n");

		if (DPE_devs[0].dev_ver == 1) {
			dpe_clk.CLK_CAMSYS_IPE_LARB19_CAMERA_P2 = devm_clk_get(&pDev->dev,
								"CLK_CAMSYS_IPE_LARB19");
			if (IS_ERR(dpe_clk.CLK_CAMSYS_IPE_LARB19_CAMERA_P2))
				LOG_ERR("cannot get CLK_CAMSYS_IPE_LARB19 clock\n");
		}


		dpe_clk.CLK_CAMSYS_IPE_DPE_CAMERA_P2 = devm_clk_get(&pDev->dev,
							"CLK_CAMSYS_IPE_DPE");
		if (IS_ERR(dpe_clk.CLK_CAMSYS_IPE_DPE_CAMERA_P2))
			LOG_ERR("cannot get CLK_CAMSYS_IPE_DPE clock\n");

		dpe_clk.CLK_CAMSYS_IPE_FUS_CAMERA_P2 = devm_clk_get(&pDev->dev,
							"CLK_CAMSYS_IPE_FUS");
		if (IS_ERR(dpe_clk.CLK_CAMSYS_IPE_FUS_CAMERA_P2))
			LOG_ERR("cannot get CLK_CAMSYS_IPE_FUS clock\n");

		if (DPE_devs[0].dev_ver == 0) {
			dpe_clk.CLK_CAMSYS_IPE_DHZE_CAMERA_P2 = devm_clk_get(&pDev->dev,
								"CLK_CAMSYS_IPE_DHZE");
			if (IS_ERR(dpe_clk.CLK_CAMSYS_IPE_DHZE_CAMERA_P2))
				LOG_ERR("cannot get CLK_CAMSYS_IPE_DHZE clock\n");
		}

		dpe_clk.CLK_CAMSYS_IPE_GALS_CAMERA_P2 = devm_clk_get(&pDev->dev,
							"CLK_CAMSYS_IPE_GALS");
		if (IS_ERR(dpe_clk.CLK_CAMSYS_IPE_GALS_CAMERA_P2))
			LOG_ERR("cannot get CLK_CAMSYS_IPE_GALS clock\n");
// #endif
#endif
#endif

		dpe_mmqos_init(&pDev->dev);

		//get bwr device
		if (DPE_devs[0].dev_ver == 0)
			dpe_bwr_device = mtk_cam_bwr_get_dev(pDev);

		/* Create class register */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
		pDPEClass = class_create("DPEdrv");
#else
		pDPEClass = class_create(THIS_MODULE, "DPEdrv");
#endif
		if (IS_ERR(pDPEClass)) {
			Ret = PTR_ERR(pDPEClass);
			LOG_ERR("Unable to create class, err = %d", Ret);
			goto EXIT;
		}
		dev = device_create(pDPEClass, NULL, DPEDevNo, NULL,
								DPE_DEV_NAME);
		if (IS_ERR(dev)) {
			Ret = PTR_ERR(dev);
			dev_dbg(&pDev->dev, "create dev err: /dev/%s, err = %d",
				DPE_DEV_NAME, Ret);
			goto EXIT;
		}
	//!SMMU
	LOG_INF("Star SMMU init\n");
	Get_SMMU = 0;
	_dpe_dev->smmu_dev = mtk_smmu_get_shared_device(&pDev->dev);
	if (!_dpe_dev) {
		dev_dbg(&pDev->dev,
			"%s: failed to get dpe smmu device\n",
			__func__);
		return -EINVAL;
	}
	LOG_INF("smmu_get done\n");
	smmudev = _dpe_dev->smmu_dev;
	LOG_INF("Star SMMU end\n");
#ifdef KERNEL_DMA_BUFFER

	gdev = &pDev->dev;
	if (dma_set_mask_and_coherent(gdev, DMA_BIT_MASK(34)))
		LOG_ERR("%s: No suitable DMA available\n", __func__);
	Get_SMMU = smmu_v3_enabled();
	kernel_dpebuf =
	vb2_dc_alloc(NULL, gdev, WB_TOTAL_SIZE);
	dbuf = vb2_dc_get_dmabuf(NULL, kernel_dpebuf, O_RDWR);
	refcount_dec(&kernel_dpebuf->refcount);
	if(Get_SMMU == 1) {
		dpebuf =
		vb2_dc_attach_dmabuf(NULL, smmudev, dbuf, WB_TOTAL_SIZE);
	} else {
		dpebuf =
		vb2_dc_attach_dmabuf(NULL, gdev, dbuf, WB_TOTAL_SIZE);
	}

	if (vb2_dc_map_dmabuf(dpebuf) != 0)
		LOG_INF("Allocate Buffer Fail!");
	g_dpewb_asfrm_Buffer_pa = (dma_addr_t *)dpebuf->dma_addr;
	g_dpewb_asfrmext_Buffer_pa =
		(dma_addr_t *)(((uintptr_t)g_dpewb_asfrm_Buffer_pa) +
		WB_ASFRM_SIZE);
	g_dpewb_wmfhf_Buffer_pa =
		(dma_addr_t *)(((uintptr_t)g_dpewb_asfrmext_Buffer_pa) +
		WB_ASFRMExt_SIZE);

	//LOG_INF("g_dpewb_asfrm_Buffer_pa = %llu\n",
	//g_dpewb_asfrm_Buffer_pa);

	//LOG_INF("g_dpewb_asfrmext_Buffer_pa = %llu\n",
	//g_dpewb_asfrmext_Buffer_pa);

	//LOG_INF("g_dpewb_wmfhf_Buffer_pa = %llu\n",
	//g_dpewb_wmfhf_Buffer_pa);

#endif
	//pm_runtime_enable(gdev);

		/* Init spinlocks */
		reqidx = 0;
		spin_lock_init(&(REQ_LOCK));
		spin_lock_init(&(DPEInfo.SpinLockDPE));
		spin_lock_init(&(DPEInfo.SpinLockFD));//!
		for (n = 0; n < DPE_IRQ_TYPE_AMOUNT; n++)
			spin_lock_init(&(DPEInfo.SpinLockIrq[n]));
		/*  */
		init_completion(&DPEinit_done);
		init_waitqueue_head(&DPEInfo.WaitQueueHead);
		INIT_WORK(&DPEInfo.ScheduleDpeWork, DVS_ScheduleWork);
		INIT_WORK(&DPEInfo.DVP_ScheduleDpeWork, DVP_ScheduleWork);
		INIT_WORK(&DPEInfo.DVGF_ScheduleDpeWork, DVGF_ScheduleWork);
		DPEInfo.wkqueue = create_singlethread_workqueue("DPE-CMDQ-WQ");
		if (!DPEInfo.wkqueue)
			LOG_ERR("NULL DPE-CMDQ-WQ\n");
		//!wakeup_source_init(&DPE_wake_lock, "dpe_lock_wakelock"); //!
		DPEInfo.cmdq_wq = alloc_ordered_workqueue("%s",
						__WQ_LEGACY | WQ_MEM_RECLAIM |
						WQ_FREEZABLE | WQ_HIGHPRI,
						"dpe_cmdq_cb_wq");
		if (!DPEInfo.cmdq_wq)
			LOG_INF("%s: Create workquque DPE-CMDQ fail!\n", __func__);

		INIT_WORK(&logWork, logPrint);
		for (i = 0; i < DPE_IRQ_TYPE_AMOUNT; i++)
			tasklet_init(DPE_tasklet[i].pDPE_tkt,
					DPE_tasklet[i].tkt_cb, 0);
		/* Init DPEInfo */
		//mutex_lock(&(MutexDPERef));
		DPEInfo.UserCount = 0;
		DPEInfo.ServeCount = 0;
//		mutex_unlock(&(MutexDPERef));
		/*  */
		DPEInfo.IrqInfo.Mask[DPE_IRQ_TYPE_INT_DVP_ST] = INT_ST_MASK_DPE;
#if defined(DPE_PMQOS_EN) && defined(CONFIG_MTK_QOS_SUPPORT)
		pm_qos_add_request(&dpe_pm_qos_request,
			PM_QOS_MM_MEMORY_BANDWIDTH, PM_QOS_DEFAULT_VALUE);
		cmdqCoreRegisterTaskCycleCB(CMDQ_GROUP_DPE, cmdq_pm_qos_start,
							cmdq_pm_qos_stop);
#endif
		seqlock_init(&(dpe_reqs_dvs.seqlock));
		seqlock_init(&(dpe_reqs_dvp.seqlock));
		seqlock_init(&(dpe_reqs_dvgf.seqlock));
		snprintf(DPE_dev->v4l2_dev.name, sizeof(DPE_dev->v4l2_dev.name),
			"%s-%03d", DPE_DEV_NAME, 0);
		Ret = v4l2_device_register(&pDev->dev, &DPE_dev->v4l2_dev);

		if (Ret) {
			LOG_INF("Failed to register v4l2 device\n");
			return Ret;
		}
		LOG_INF("get v4l2_device_register = %d\n", Ret);

		DPE_dev->v4l2_dev.release = dpe_dev_release;
		/* initialize locks */
		mutex_init(&DPE_dev->mutex);
		vfd = &DPE_dev->vid_dpe_dev;
		memset(vfd, 0, sizeof(*vfd));
		strscpy(vfd->name, "dpe-vid", sizeof(vfd->name));
		vfd->fops = &vid_fops;
		vfd->ioctl_ops = &vid_ioctl_ops;
		vfd->release = video_device_release_empty;
		vfd->v4l2_dev = &DPE_dev->v4l2_dev;
		vfd->vfl_dir = VFL_DIR_M2M;
		vfd->device_caps =
		V4L2_CAP_VIDEO_CAPTURE |
		V4L2_CAP_STREAMING |
		V4L2_CAP_READWRITE;
		/*
		 * Provide a mutex to v4l2 core. It will be used to protect
		 * all fops and v4l2 ioctls.
		 */
		vfd->lock = &DPE_dev->mutex;
		video_set_drvdata(vfd, DPE_dev);
		Ret = video_register_device(vfd, VFL_TYPE_VIDEO, -1);
		//Ret = video_register_device(vfd, VFL_TYPE_GRABBER, -1);

		LOG_INF("video_register_device = %d\n", Ret);
		if (Ret < 0) {
			video_unregister_device(vfd);
			LOG_INF("video_register_device failed\n");
		}
	}
	g_isshutdown = 0;
	g_DPE_PMState = 0;
	DPE_cmdq_buf_idx = 0;
	//Get_DVS_IRQ = 0;
	//Get_DVP_IRQ = 0;
	//Get_DVGF_IRQ = 0;
	//DVGF_Frame_cnt = 0;
EXIT:
	if (Ret < 0)
		DPE_UnregCharDev();
	LOG_INF("- X. DPE driver probe.");
	return Ret;
}
/*******************************************************************************
 * Called when the device is being detached from the driver
 ******************************************************************************/
static signed int DPE_remove(struct platform_device *pDev)
{
	/*struct resource *pRes;*/
	signed int IrqNum;
	int i;
	/*  */
	LOG_DBG("- E.");
	pm_runtime_disable(&pDev->dev);
	/* wait for unfinished works in the workqueue. */
	destroy_workqueue(DPEInfo.wkqueue);
	DPEInfo.wkqueue = NULL;

	flush_workqueue(DPEInfo.cmdq_wq);
	destroy_workqueue(DPEInfo.cmdq_wq);
	DPEInfo.cmdq_wq = NULL;
	/* unregister char driver. */
	DPE_UnregCharDev();
	/* Release IRQ */
	disable_irq(DPEInfo.IrqNum);
	IrqNum = platform_get_irq(pDev, 0);
	free_irq(IrqNum, NULL);
	/* kill tasklet */
	for (i = 0; i < DPE_IRQ_TYPE_AMOUNT; i++)
		tasklet_kill(DPE_tasklet[i].pDPE_tkt);
#ifdef KERNEL_DMA_BUFFER
	vb2_dc_unmap_dmabuf(dpebuf);
	vb2_dc_detach_dmabuf(dpebuf);
	vb2_dc_put(kernel_dpebuf);
	dpebuf = NULL;
	kernel_dpebuf = NULL;
	dbuf = NULL;
	gdev = NULL;
#endif
	/*  */
	device_destroy(pDPEClass, DPEDevNo);
	/*  */
	class_destroy(pDPEClass);
	pDPEClass = NULL;
	/*  */
	if (DPE_devs != NULL) {
		kfree(DPE_devs);
		DPE_devs = NULL;
	}
#ifdef CMASYS_CLK_Debug
	if (CAM_devs != NULL) {
		kfree(CAM_devs);
		CAM_devs = NULL;
	}
	if (IPE_devs != NULL) {
		kfree(IPE_devs);
		IPE_devs = NULL;
	}
#endif

#if defined(DPE_PMQOS_EN) && defined(CONFIG_MTK_QOS_SUPPORT)
	pm_qos_remove_request(&dpe_pm_qos_request);
#endif
	//video_unregister_device(&DPE_devs[nr_DPE_devs - 1].vid_dpe_dev);
	return 0;
}
/*******************************************************************************
 *
 ******************************************************************************/

static signed int DPE_suspend(struct platform_device *pDev, pm_message_t Mesg)
{

	return 0;
}
/*******************************************************************************
 *
 ******************************************************************************/
static signed int DPE_resume(struct platform_device *pDev)
{

	return 0;
}

static void DPE_shutdown(struct platform_device *pdev)
{
	g_isshutdown = 1;

	if (dpe_clt)
		cmdq_mbox_stop(dpe_clt);
	else
		dev_info(&pdev->dev, "%s: dpe cmdq client is NULL\n", __func__);

	LOG_INF("DPE shutdown callback: %d", g_isshutdown);
}
/*---------------------------------------------------------------------------*/
#if IS_ENABLED(CONFIG_PM)
/*---------------------------------------------------------------------------*/
static signed int bPass1_On_In_Resume_TG1;
static int dpe_suspend_pm_event(struct notifier_block *notifier,
			unsigned long pm_event, void *unused)
{
	struct timespec64 ts;
	struct rtc_time tm;

	ktime_get_ts64(&ts);
	rtc_time64_to_tm(ts.tv_sec, &tm);

	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:
		return NOTIFY_DONE;
	case PM_RESTORE_PREPARE:
		return NOTIFY_DONE;
	case PM_POST_HIBERNATION:
		return NOTIFY_DONE;
	case PM_SUSPEND_PREPARE: /*enter suspend*/
		spin_lock(&(DPEInfo.SpinLockDPE));
		if (g_u4EnableClockCount > 0) {
			spin_unlock(&(DPEInfo.SpinLockDPE));
			DPE_EnableClock(MFALSE);

			spin_lock(&(DPEInfo.SpinLockDPE));
			g_SuspendCnt++;
			spin_unlock(&(DPEInfo.SpinLockDPE));
		} else
			spin_unlock(&(DPEInfo.SpinLockDPE));

		spin_lock(&(DPEInfo.SpinLockDPE));
		bPass1_On_In_Resume_TG1 = 0;
		if (g_DPE_PMState == 0) {
			LOG_INF("%s:suspend g_u4EnableClockCount(%d) g_SuspendCnt(%d).\n",
				__func__,
				g_u4EnableClockCount,
				g_SuspendCnt);
			g_DPE_PMState = 1;
		}
		spin_unlock(&(DPEInfo.SpinLockDPE));
		return NOTIFY_DONE;
	case PM_POST_SUSPEND:    /*after resume*/
		spin_lock(&(DPEInfo.SpinLockDPE));
		if (g_SuspendCnt > 0) {
			spin_unlock(&(DPEInfo.SpinLockDPE));
			DPE_EnableClock(MTRUE);

			spin_lock(&(DPEInfo.SpinLockDPE));
			g_SuspendCnt--;
			spin_unlock(&(DPEInfo.SpinLockDPE));
		} else
			spin_unlock(&(DPEInfo.SpinLockDPE));

		spin_lock(&(DPEInfo.SpinLockDPE));
		if (g_DPE_PMState == 1) {
			LOG_INF("%s:resume g_u4EnableClockCount(%d) g_SuspendCnt(%d).\n",
				__func__,
				g_u4EnableClockCount,
				g_SuspendCnt);
			g_DPE_PMState = 0;
		}
		spin_unlock(&(DPEInfo.SpinLockDPE));
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

int DPE_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);
	LOG_DBG("calling %s()\n", __func__);
	return DPE_suspend(pdev, PMSG_SUSPEND);
}
int DPE_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);
	LOG_DBG("calling %s()\n", __func__);
	return DPE_resume(pdev);
}
#if !IS_ENABLED(CONFIG_OF)
/*extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);*/
/*extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);*/
#endif
int DPE_pm_restore_noirq(struct device *device)
{
	pr_debug("calling %s()\n", __func__);
#if !IS_ENABLED(CONFIG_OF)
/*	mt_irq_set_sens(DPE_IRQ_BIT_ID, MT_LEVEL_SENSITIVE);*/
/*	mt_irq_set_polarity(DPE_IRQ_BIT_ID, MT_POLARITY_LOW);*/
#endif
	return 0;
}
/*---------------------------------------------------------------------------*/
#else				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#define DPE_pm_suspend NULL
#define DPE_pm_resume  NULL
#define DPE_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id DPE_of_ids[] = {
	//{.compatible = "mediatek,ipesys_config",},
	{.compatible = "mediatek,dvp",},
	{.compatible = "mediatek,dvs",},
	{.compatible = "mediatek,dvgf",},
	{.compatible = "mediatek,dvp_mt6899",},
	{.compatible = "mediatek,dvs_mt6899",},
	{.compatible = "mediatek,dvgf_mt6899",},
	{}
};
#endif
const struct dev_pm_ops DPE_pm_ops = {
	.suspend = DPE_pm_suspend,
	.resume = DPE_pm_resume,
	.freeze = DPE_pm_suspend,
	.thaw = DPE_pm_resume,
	.poweroff = DPE_pm_suspend,
	.restore = DPE_pm_resume,
	.restore_noirq = DPE_pm_restore_noirq,
};
/*******************************************************************************
 *
 ******************************************************************************/
static struct platform_driver DPEDriver = {
	.probe = DPE_probe,
	.remove = DPE_remove,
	.shutdown = DPE_shutdown,
	.suspend = DPE_suspend,
	.resume = DPE_resume,
	.driver = {
		   .name = DPE_DEV_NAME,
		   .owner = THIS_MODULE,
#if IS_ENABLED(CONFIG_OF)
		   .of_match_table = DPE_of_ids,
#endif
#if IS_ENABLED(CONFIG_PM)
		   .pm = &DPE_pm_ops,
#endif
		}
};

#if IS_ENABLED(CONFIG_PM)
static struct notifier_block dpe_suspend_pm_notifier_func = {
	.notifier_call = dpe_suspend_pm_event,
	.priority = 0,
};
#endif

static int dpe_dump_read(struct seq_file *m, void *v)
{
/* fix unexpected close clock issue */
#ifdef dpe_dump_read_en
	int i, j;

	if (DPEInfo.UserCount <= 0)
		return 0;
	seq_puts(m, "\n============ dpe dump register============\n");
	seq_puts(m, "DPE Config Info\n");
	for (i = 0x2C; i < 0x8C; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n",
				(unsigned int)(DPE_BASE_HW + i),
			   (unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}
	seq_puts(m, "DPE Debug Info\n");
	for (i = 0x120; i < 0x148; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n",
				(unsigned int)(DPE_BASE_HW + i),
			   (unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}
	seq_puts(m, "DPE Config Info\n");
	for (i = 0x230; i < 0x2D8; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n",
				(unsigned int)(DPE_BASE_HW + i),
			   (unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}
	seq_puts(m, "DPE Debug Info\n");
	for (i = 0x2F4; i < 0x30C; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n",
				(unsigned int)(DPE_BASE_HW + i),
			   (unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}
	seq_puts(m, "\n");
	/*seq_printf(m, "Dpe Clock Count:%d\n", g_u4EnableClockCount);*/
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVS_IRQ_STATUS_HW),
		   (unsigned int)DPE_RD32(DVS_IRQ_STATUS_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_STATUS0_HW),
		   (unsigned int)DPE_RD32(DVS_CTRL_STATUS0_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVS_CTRL_STATUS2_HW),
		   (unsigned int)DPE_RD32(DVS_CTRL_STATUS2_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVS_CUR_STATUS_HW),
		   (unsigned int)DPE_RD32(DVS_CUR_STATUS_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS0_HW),
		   (unsigned int)DPE_RD32(DVS_FRM_STATUS0_REG));
//	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVS_FRM_STATUS2_HW),
//		   (unsigned int)DPE_RD32(DVS_FRM_STATUS2_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_STA0_HW),
		   (unsigned int)DPE_RD32(DVS_DRAM_STA0_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVS_DRAM_STA1_HW),
		   (unsigned int)DPE_RD32(DVS_DRAM_STA1_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVP_IRQ_STATUS_HW),
		   (unsigned int)DPE_RD32(DVP_IRQ_STATUS_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVP_CTRL_STATUS0_HW),
		   (unsigned int)DPE_RD32(DVP_CTRL_STATUS0_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVP_CTRL_STATUS1_HW),
		   (unsigned int)DPE_RD32(DVP_CTRL_STATUS1_REG));
	//seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVP_CTRL_STATUS2_HW),
		   //(unsigned int)DPE_RD32(DVP_CTRL_STATUS2_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVP_CUR_STATUS_HW),
		   (unsigned int)DPE_RD32(DVP_CUR_STATUS_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVP_FRM_STATUS0_HW),
		   (unsigned int)DPE_RD32(DVP_FRM_STATUS0_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DVP_FRM_STATUS2_HW),
		   (unsigned int)DPE_RD32(DVP_FRM_STATUS2_REG));
	seq_printf(m, "DPE:HWProcessIdx:%d, WriteIdx:%d, ReadIdx:%d\n",
		   g_DPE_ReqRing.HWProcessIdx, g_DPE_ReqRing.WriteIdx,
		   g_DPE_ReqRing.ReadIdx);
	for (i = 0; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
		seq_printf(m,
			   "DPE:State:%d, processID:0x%08X, callerID:0x%08X, enqueReqNum:%d, FrameWRIdx:%d, RrameRDIdx:%d\n",
			   g_DPE_ReqRing.DPEReq_Struct[i].State,
			   g_DPE_ReqRing.DPEReq_Struct[i].processID,
			   g_DPE_ReqRing.DPEReq_Struct[i].callerID,
			   g_DPE_ReqRing.DPEReq_Struct[i].enqueReqNum,
			   g_DPE_ReqRing.DPEReq_Struct[i].FrameWRIdx,
			   g_DPE_ReqRing.DPEReq_Struct[i].RrameRDIdx);
		for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_;) {
			seq_printf(m,
				   "DPE:FrmStat[%d]:%d, FrmStat[%d]:%d\n",
				   j,
			g_DPE_ReqRing.DPEReq_Struct[i].DpeFrameStatus[j],
				j + 1,
			g_DPE_ReqRing.DPEReq_Struct[i].DpeFrameStatus[j + 1]);
				j = j + 2;
		}
	}
	seq_puts(m, "\n============ dpe dump debug ============\n");
#endif
	return 0;
}
static int proc_dpe_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, dpe_dump_read, NULL);
}
static const struct file_operations dpe_dump_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_dpe_dump_open,
	.read = seq_read,
};
static int dpe_reg_read(struct seq_file *m, void *v)
{
/* fix unexpected close clock issue */
#ifdef dpe_dump_read_en
	unsigned int i;

	if (DPEInfo.UserCount <= 0)
		return 0;
	seq_puts(m, "======== read dpe register ========\n");
	for (i = 0x000; i <= 0x1CC; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n",
				(unsigned int)(DPE_BASE_HW + i),
				(unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}
	for (i = 0x800; i <= 0xBDC; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n",
				(unsigned int)(DPE_BASE_HW + i),
				(unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}
#endif
	return 0;
}
static ssize_t dpe_reg_write(struct file *file, const char __user *buffer,
						size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	/*char *pEnd;*/
	char addrSzBuf[24];
	char valSzBuf[24];
	char *pszTmp;
	int addr = 0, val = 0;

	if (DPEInfo.UserCount <= 0)
		return 0;
	spin_lock(&(DPEInfo.SpinLockDPE));
	if (g_u4EnableClockCount == 0) {
		spin_unlock(&(DPEInfo.SpinLockDPE));
		return 0;
	}
	spin_unlock(&(DPEInfo.SpinLockDPE));
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';
	addrSzBuf[23] = '\0';
	valSzBuf[23] = '\0';
	if (sscanf(desc, "%23s %23s", addrSzBuf, valSzBuf) == 2) {
		pszTmp = strstr(addrSzBuf, "0x");
		if (pszTmp == NULL) {
			/*if (1 != sscanf(addrSzBuf, "%d", &addr))*/
#ifdef dpe_dump_read_en
			if (kstrtoint(addrSzBuf, 0, &addr) != 0)
				LOG_ERR("scan decimal addr is wrong !!:%s",
								addrSzBuf);
#else
				LOG_ERR("hex address only:%s", addrSzBuf);
#endif
		} else {
			if (strlen(addrSzBuf) > 2) {
				if (sscanf(addrSzBuf + 2, "%x", &addr) != 1)
					LOG_ERR("error hex addr:%s", addrSzBuf);
			} else {
				LOG_INF("DPE Write Addr Error!!:%s", addrSzBuf);
			}
		}
		pszTmp = strstr(valSzBuf, "0x");
		if (pszTmp == NULL) {
			/*if (1 != sscanf(valSzBuf, "%d", &val))*/
#ifdef dpe_dump_read_en
			if (kstrtoint(valSzBuf, 0, &val) != 0)
				LOG_ERR("scan decimal value is wrong !!:%s",
								valSzBuf);
#else
				LOG_ERR("HEX address only :%s", valSzBuf);
#endif
		} else {
			if (strlen(valSzBuf) > 2) {
				if (sscanf(valSzBuf + 2, "%x", &val) != 1)
					LOG_ERR("error hex val:%s", valSzBuf);
			} else {
				LOG_INF("DPE Write Value Error!!:%s\n",
								valSzBuf);
			}
		}
		if ((addr >= DPE_BASE_HW) && (addr <= DVP_CTRL_ATPG_HW)
			&& ((addr & 0x3) == 0)) {
			LOG_INF("Write Request - addr:0x%x, value:0x%x\n", addr,
									val);
			#ifdef DPE_WR32_en
			DPE_WR32((ISP_DPE_BASE + (addr - DPE_BASE_HW)), val);
			#endif
		} else {
			LOG_INF
			    ("write add out-of-range addr:0x%x, value:0x%x\n",
			     addr, val);
		}
	} else if (sscanf(desc, "%23s", addrSzBuf) == 1) {
		pszTmp = strstr(addrSzBuf, "0x");
		if (pszTmp == NULL) {
			/*if (1 != sscanf(addrSzBuf, "%d", &addr))*/
#ifdef dpe_dump_read_en
			if (kstrtoint(addrSzBuf, 0, &addr) != 0)
				LOG_ERR("scan decimal addr is wrong !!:%s",
								addrSzBuf);
#else
				LOG_ERR("HEX address only :%s", addrSzBuf);
#endif
		} else {
			if (strlen(addrSzBuf) > 2) {
				if (sscanf(addrSzBuf + 2, "%x", &addr) != 1)
					LOG_ERR("hex addr err:%s", addrSzBuf);
			} else {
				LOG_INF("DPE Read Addr Error!!:%s", addrSzBuf);
			}
		}
		if ((addr >= DPE_BASE_HW) && (addr <= DVP_CTRL_ATPG_HW)
			&& ((addr & 0x3) == 0)) {
			val = DPE_RD32((ISP_DPE_BASE + (addr - DPE_BASE_HW)));
			LOG_INF("Read Request - addr:0x%x,value:0x%x\n", addr,
									val);
		} else {
			LOG_INF
			    ("Read-Addr out-of-range!! addr:0x%x, value:0x%x\n",
			     addr, val);
		}
	}
	return count;
}
static int proc_dpe_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, dpe_reg_read, NULL);
}
static const struct file_operations dpe_reg_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_dpe_reg_open,
	.read = seq_read,
	.write = dpe_reg_write,
};
/**************************************************************
 *
 **************************************************************/
#if IS_ENABLED(CONFIG_MTK_M4U)
#if IS_ENABLED(CONFIG_MTK_IOMMU_V2)
enum mtk_iommu_callback_ret_t DPE_M4U_TranslationFault_callback(int port,
	unsigned long mva, void *data)
#else
enum m4u_callback_ret_t DPE_M4U_TranslationFault_callback(int port,
	unsigned int mva, void *data)
#endif
{
	pr_info("[ISP_M4U]fault call port=%d, mva=%llu", port, mva);
	switch (port) {
	default:
		DPE_DumpReg();
	break;
	}
#if IS_ENABLED(CONFIG_MTK_IOMMU_V2)
	return MTK_IOMMU_CALLBACK_HANDLED;
#else
	return M4U_CALLBACK_HANDLED;
#endif
}
#endif
/*******************************************************************************
 *
 ******************************************************************************/
int32_t DPE_ClockOnCallback(uint64_t engineFlag)
{
	/* LOG_DBG("DPE_ClockOnCallback"); */
	/* LOG_DBG("+CmdqEn:%d", g_u4EnableClockCount); */
	/* DPE_EnableClock(MTRUE); */
	return 0;
}
int32_t DPE_DumpCallback(uint64_t engineFlag, int level)
{
	LOG_DBG("DumpCallback");
	DPE_DumpReg();
	dpe_request_dump_isp8(&dpe_reqs_dvs);
	return 0;
}
int32_t DPE_ResetCallback(uint64_t engineFlag)
{
	LOG_DBG("ResetCallback");
	DPE_Reset();
	return 0;
}
int32_t DPE_ClockOffCallback(uint64_t engineFlag)
{
	/* LOG_DBG("DPE_ClockOffCallback"); */
	/* DPE_EnableClock(MFALSE); */
	/* LOG_DBG("-CmdqEn:%d", g_u4EnableClockCount); */
	return 0;
}
static signed int __init DPE_Init(void)
{
	signed int Ret = 0, j;
	void *tmp;
#if CHECK_SERVICE_IF_0
	struct device_node *node = NULL;

	/* FIX-ME: linux-3.10 procfs API changed */
	/* use proc_create */
	struct proc_dir_entry *isp_dpe_dir;
#endif
	int i;
	/*  */
	LOG_INF("- E. DPE Init");
	/*  */
	Ret = platform_driver_register(&DPEDriver);
	if (Ret < 0) {
		LOG_ERR("platform_driver_register fail");
		return Ret;
	}
#if CHECK_SERVICE_IF_0
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,DPE");
	if (!node) {
		LOG_ERR("find mediatek,DPE node failed!!!\n");
		return -ENODEV;
	}
	ISP_DPE_BASE = of_iomap(node, 0);
	if (!ISP_DPE_BASE) {
		LOG_ERR("unable to map ISP_DPE_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("ISP_DPE_BASE: %llu\n", ISP_DPE_BASE);

	isp_dpe_dir = proc_mkdir("dpe", NULL);
	if (!isp_dpe_dir) {
		LOG_ERR("[%s]: fail to mkdir /proc/dpe\n", __func__);
		return 0;
	}
	proc_entry = proc_create("dpe_dump", 0444, isp_dpe_dir,
							&dpe_dump_proc_fops);
	proc_entry = proc_create("dpe_reg", 0644, isp_dpe_dir,
							&dpe_reg_proc_fops);
#endif
	/* isr log */
	if (PAGE_SIZE <
	    ((DPE_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN *
		((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1)) * LOG_PPNUM)) {
		i = 0;
		while (i <
		       ((DPE_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN *
			 ((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1)) * LOG_PPNUM)) {
			i += PAGE_SIZE;
		}
	} else {
		i = PAGE_SIZE;
	}
	pLog_kmalloc = kmalloc(i, GFP_KERNEL);
	if (pLog_kmalloc == NULL) {
		LOG_ERR("log mem not enough\n");
		return -ENOMEM;
	}
	memset(pLog_kmalloc, 0x00, i);
	tmp = pLog_kmalloc;
	for (i = 0; i < LOG_PPNUM; i++) {
		for (j = 0; j < DPE_IRQ_TYPE_AMOUNT; j++) {
			gSvLog[j]._str[i][_LOG_DBG] = (char *)tmp;
			tmp = (void *)((char *)tmp +
						(NORMAL_STR_LEN * DBG_PAGE));
			gSvLog[j]._str[i][_LOG_INF] = (char *)tmp;
			tmp = (void *)((char *)tmp +
						(NORMAL_STR_LEN * INF_PAGE));
			gSvLog[j]._str[i][_LOG_ERR] = (char *)tmp;
			tmp = (void *)((char *)tmp +
						(NORMAL_STR_LEN * ERR_PAGE));
		}
		/* tmp = (void*) ((unsigned int)tmp + NORMAL_STR_LEN); */
		tmp = (void *)((char *)tmp + NORMAL_STR_LEN);	/* overflow */
	}
#ifndef CMDQ_COMMON
	/* Cmdq */
	/* Register DPE callback */
	LOG_INF("register dpe callback for CMDQ");
	cmdqCoreRegisterCB(CMDQ_GROUP_DPE,
			   DPE_ClockOnCallback,
			   DPE_DumpCallback, DPE_ResetCallback,
							DPE_ClockOffCallback);
#endif
LOG_INF("CONFIG_MTK_M4U 530");
#if IS_ENABLED(CONFIG_MTK_M4U)
LOG_INF("MTK_DPE_VER = %d", MTK_DPE_VER);
#if IS_ENABLED(CONFIG_MTK_IOMMU_V2)
LOG_INF("- E. MTK_DPE_VER Ster");
	#if (MTK_DPE_VER == 0)
	mtk_iommu_register_fault_callback(M4U_PORT_L19_DVS_RDMA,
					  DPE_M4U_TranslationFault_callback,
					  NULL);
	mtk_iommu_register_fault_callback(M4U_PORT_L19_DVS_WDMA,
					  DPE_M4U_TranslationFault_callback,
					  NULL);
	mtk_iommu_register_fault_callback(M4U_PORT_L19_DVP_RDMA,
					  DPE_M4U_TranslationFault_callback,
					  NULL);
	mtk_iommu_register_fault_callback(M4U_PORT_L19_DVP_WDMA,
					  DPE_M4U_TranslationFault_callback,
					  NULL);
	#else
	mtk_iommu_register_fault_callback(M4U_PORT_L19_DVS_RDMA,
					  DPE_M4U_TranslationFault_callback,
					  NULL);
	mtk_iommu_register_fault_callback(M4U_PORT_L19_DVS_WDMA,
					  DPE_M4U_TranslationFault_callback,
					  NULL);
	mtk_iommu_register_fault_callback(M4U_PORT_L19_DVP_RDMA,
					  DPE_M4U_TranslationFault_callback,
					  NULL);
	mtk_iommu_register_fault_callback(M4U_PORT_L19_DVP_WDMA,
					  DPE_M4U_TranslationFault_callback,
					  NULL);

	#endif
#else
	#if (MTK_DPE_VER == 0)
	LOG_INF("m4u_register_fault_callback");
	m4u_register_fault_callback(M4U_PORT_L19_DVS_RDMA,
			DPE_M4U_TranslationFault_callback, NULL);
	m4u_register_fault_callback(M4U_PORT_L19_DVS_WDMA,
			DPE_M4U_TranslationFault_callback, NULL);
	m4u_register_fault_callback(M4U_PORT_L19_DVP_RDMA,
			DPE_M4U_TranslationFault_callback, NULL);
	m4u_register_fault_callback(M4U_PORT_L19_DVP_WDMA,
			DPE_M4U_TranslationFault_callback, NULL);
	#endif

#endif
#endif

#if IS_ENABLED(CONFIG_PM)
	Ret = register_pm_notifier(&dpe_suspend_pm_notifier_func);
	if (Ret) {
		pr_debug("[Camera DPE] Failed to register PM notifier.\n");
		return Ret;
	}
#endif

	LOG_INF("- X. DPE Init Ret: %d.", Ret);
	return Ret;
}
/*******************************************************************************
 *
 ******************************************************************************/
static void __exit DPE_Exit(void)
{
	/*int i;*/
	LOG_DBG("- E.");
	/*  */
	platform_driver_unregister(&DPEDriver);
	/*  */
	/* Cmdq */
	/* Unregister DPE callback */
	//cmdqCoreRegisterCB(CMDQ_GROUP_DPE, NULL, NULL, NULL, NULL);
	kfree(pLog_kmalloc);
	/*  */
}
/*******************************************************************************
 *
 ******************************************************************************/
void DVS_ScheduleWork(struct work_struct *data)
{

	if (DPE_DBG_DBGLOG & DPEInfo.DebugMask)
		LOG_INF("- E.DVS_Schedule");

	if (DPEInfo.UserCount > 0) {
		dpe_request_handler_isp8(&dpe_reqs_dvs,
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));


		if (!dpe_request_running_isp8(&dpe_reqs_dvs))
			LOG_INF("[%s]no more requests", __func__);
	}
}

void DVP_ScheduleWork(struct work_struct *data)
{

	if (DPE_DBG_DBGLOG & DPEInfo.DebugMask)
		LOG_INF("- E.DVP_Schedule");

	if (DPEInfo.UserCount > 0) {
		dpe_request_handler_isp8(&dpe_reqs_dvp,
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));


		if (!dpe_request_running_isp8(&dpe_reqs_dvp))
			LOG_INF("[%s]no more requests", __func__);
	}
}

void DVGF_ScheduleWork(struct work_struct *data)
{

	if (DPE_DBG_DBGLOG & DPEInfo.DebugMask)
		LOG_INF("- E.DVGF_Schedule");

	if (DPEInfo.UserCount > 0) {
		dpe_request_handler_isp8(&dpe_reqs_dvgf,
				&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));


		if (!dpe_request_running_isp8(&dpe_reqs_dvgf))
			LOG_INF("[%s]no more requests", __func__);
	}
}

#if DPE_IRQ_ENABLE
static irqreturn_t ISP_Irq_DVP(signed int Irq, void *DeviceId)
{
	unsigned int DvsStatus = 0, DvpStatus = 0;
	bool bResulst = MFALSE;
	bool isDvpDone = MFALSE;
	pid_t ProcessID;
	unsigned int p = 0;

	if (g_u4EnableClockCount > 0) {
		DvsStatus = DPE_RD32(DVS_CTRL_STATUS0_REG);	/* DVS Status */
		DvpStatus = DPE_RD32(DVP_CTRL_STATUS0_REG);	/* DVP Status */
	} else {
		LOG_INF("DVPDPE Power not Enable\n");
		return IRQ_HANDLED;
	}

	if ((DvsStatus == 0) || (DvpStatus == 0))
		LOG_INF("DPE Read status fail, IRQ, DvsStatus: 0x%08x, DvpStatus: 0x%08x\n",
		DvsStatus, DvpStatus);

	if (DPE_debug_log_en == 1) {
		LOG_INF("DVP IRQ, DvsStatus: 0x%08x, DvpStatus: 0x%08x\n",
		DvsStatus, DvpStatus);
	}
	/* DPE done status may rise later, so can't use done status now  */
	/* if (DPE_INT_ST == (DPE_INT_ST & DvpStatus)) { */

		DPE_WR32(DVP_IRQ_00_REG, 0x040000F0); /* Clear DVP IRQ */
		DPE_WR32(DVP_IRQ_00_REG, 0x04000F00);

		isDvpDone = MTRUE;
	/* } */
	spin_lock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
	if (isDvpDone == MTRUE) {
		/* Update the frame status. */
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("dpe_irq");
#endif
		if (dpe_update_request_isp8(&dpe_reqs_dvp, &ProcessID) == 0) {
			bResulst = MTRUE;
		} else {
			LOG_INF("dvp_update_request bResulst fail = %d\n",
			bResulst);
			spin_unlock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
			return IRQ_HANDLED;
		}

		if (bResulst == MTRUE) {
			#if REQUEST_REGULATION == REQUEST_BASE_REGULATION
			/* schedule_work(&DPEInfo.ScheduleDpeWork); */
			queue_work(DPEInfo.wkqueue, &DPEInfo.DVP_ScheduleDpeWork);
			#endif
			p = ProcessID % IRQ_USER_NUM_MAX;
			DPEInfo.IrqInfo.Status[DPE_IRQ_TYPE_INT_DVP_ST] |=
				DPE_INT_ST;
			DPEInfo.IrqInfo.ProcessID[p] =
				ProcessID;
			DPEInfo.IrqInfo.DpeIrqCnt[p]++;
			//Get_DVP_IRQ++;
			//LOG_INF("DVP DPEInfo.IrqInfo.DpeIrqCnt[p] =%d,p =%d\n",
			//DPEInfo.IrqInfo.DpeIrqCnt[p], p);
			DPEInfo.ProcessID[DPEInfo.WriteReqIdx] = ProcessID;
			DPEInfo.WriteReqIdx =
				(DPEInfo.WriteReqIdx + 1) %
				_SUPPORT_MAX_DPE_FRAME_REQUEST_;
		} else {
			LOG_INF("dvp_update_request_isp8 bResulst fail = %d\n",
			bResulst);
		}
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif
		/* Config the Next frame */
	}

	spin_unlock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
	if (bResulst == MTRUE)
		wake_up_interruptible(&DPEInfo.WaitQueueHead);
	/* dump log, use tasklet */
	IRQ_LOG_KEEPER(
		DPE_IRQ_TYPE_INT_DVP_ST,
		m_CurrentPPB,
		_LOG_INF,
		"DVP_IRQ:%d,0x%x:0x%x,0x%x:0x%x,Result:%d\n",
		Irq,
		DVS_CTRL_STATUS0_HW,
		DvsStatus,
		DVP_CTRL_STATUS0_HW,
		DvpStatus,
		bResulst);
	IRQ_LOG_KEEPER(
		DPE_IRQ_TYPE_INT_DVP_ST,
		m_CurrentPPB,
		_LOG_INF,
		"DVP_IRQ:IrqCnt[%d]:0x%x,WReq:0x%x,RReq:0x%x\n",
		p, DPEInfo.IrqInfo.DpeIrqCnt[p],
		DPEInfo.WriteReqIdx,
		DPEInfo.ReadReqIdx);
	#if (REQUEST_REGULATION == FRAME_BASE_REGULATION)
	/* schedule_work(&DPEInfo.ScheduleDpeWork); */
	queue_work(DPEInfo.wkqueue, &DPEInfo.DVP_ScheduleDpeWork);
	#endif
	if (isDvpDone == MTRUE)

		schedule_work(&logWork);

		//tasklet_schedule(DPE_tasklet[DPE_IRQ_TYPE_INT_DVP_ST].pDPE_tkt);

	return IRQ_HANDLED;
}
static irqreturn_t ISP_Irq_DVS(signed int Irq, void *DeviceId)
{
	unsigned int DvsStatus = 0, DvpStatus = 0;
	bool bResulst = MFALSE;
	bool isDvsDone = MFALSE;
	pid_t ProcessID;
	unsigned int p = 0;

	if (g_u4EnableClockCount > 0) {
		DvsStatus = DPE_RD32(DVS_CTRL_STATUS0_REG);	/* DVS Status */
		DvpStatus = DPE_RD32(DVP_CTRL_STATUS0_REG);	/* DVP Status */
	} else {
		LOG_INF("DVS DPE Power not Enable\n");
		return IRQ_HANDLED;
	}

	if ((DvsStatus == 0) || (DvpStatus == 0))
		LOG_INF("DPE Read status fail, IRQ, DvsStatus: 0x%08x, DvpStatus: 0x%08x\n",
		DvsStatus, DvpStatus);

	if (DPE_debug_log_en == 1) {
		LOG_INF("DVS IRQ, DvsStatus: 0x%08x, DvpStatus: 0x%08x\n",
		DvsStatus, DvpStatus);
	}

	/* DPE done status may rise later, so can't use done status now  */
	/* if (DPE_INT_ST == (DPE_INT_ST & DvsStatus)) { */

		DPE_WR32(DVS_IRQ_00_REG, 0x040000F0); /* Clear DVS IRQ */
		DPE_WR32(DVS_IRQ_00_REG, 0x04000F00);

		isDvsDone = MTRUE;
		//LOG_INF("DPE isDvsDone");
	/* } */
	spin_lock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
	if (isDvsDone == MTRUE) {
		/* Update the frame status. */
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("dpe_irq");
#endif
			//LOG_INF("DPE isDvsDone 2");
		if (dpe_update_request_isp8(&dpe_reqs_dvs, &ProcessID) == 0) {
			bResulst = MTRUE;
		} else {
			LOG_INF("dvs_update_request bResulst fail = %d\n",
			bResulst);
			spin_unlock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
			return IRQ_HANDLED;
		}


		//LOG_INF("bResulst = %d\n", bResulst);

		if (bResulst == MTRUE) {
			//LOG_INF("bResulst OK\n");
			#if REQUEST_REGULATION == REQUEST_BASE_REGULATION
			/* schedule_work(&DPEInfo.ScheduleDpeWork); */
			queue_work(DPEInfo.wkqueue, &DPEInfo.ScheduleDpeWork);
			#endif
			p = ProcessID % IRQ_USER_NUM_MAX;
			DPEInfo.IrqInfo.Status[DPE_IRQ_TYPE_INT_DVP_ST] |=
				DPE_INT_ST;
			DPEInfo.IrqInfo.ProcessID[p] =
				ProcessID;

			DPEInfo.IrqInfo.DpeIrqCnt[p]++;
			//Get_DVS_IRQ++;
			//LOG_INF("DVS DPEInfo.IrqInfo.DpeIrqCnt[p] =%d, p =%d\n",
			//DPEInfo.IrqInfo.DpeIrqCnt[p], p);

			DPEInfo.ProcessID[DPEInfo.WriteReqIdx] = ProcessID;
			DPEInfo.WriteReqIdx =
				(DPEInfo.WriteReqIdx + 1) %
				_SUPPORT_MAX_DPE_FRAME_REQUEST_;
		} else {
			LOG_INF("dvs_update_request_isp8 bResulst fail = %d\n",
			bResulst);
		}
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif
		/* Config the Next frame */
	}
	spin_unlock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
	if (bResulst == MTRUE)
		wake_up_interruptible(&DPEInfo.WaitQueueHead);
	/* dump log, use tasklet */
	IRQ_LOG_KEEPER(
		DPE_IRQ_TYPE_INT_DVS_ST,
		m_CurrentPPB,
		_LOG_INF,
		"DVS_IRQ:%d,0x%x:0x%x,0x%x:0x%x,Result:%d\n",
		Irq,
		DVS_CTRL_STATUS0_HW,
		DvsStatus,
		DVP_CTRL_STATUS0_HW,
		DvpStatus,
		bResulst);
	IRQ_LOG_KEEPER(
		DPE_IRQ_TYPE_INT_DVS_ST,
		m_CurrentPPB,
		_LOG_INF,
		"DVS_IRQ:IrqCnt[%d]:0x%x,WReq:0x%x,RReq:0x%x\n",
		p, DPEInfo.IrqInfo.DpeIrqCnt[p],
		DPEInfo.WriteReqIdx,
		DPEInfo.ReadReqIdx);
	#if (REQUEST_REGULATION == FRAME_BASE_REGULATION)
	/* schedule_work(&DPEInfo.ScheduleDpeWork); */
	queue_work(DPEInfo.wkqueue, &DPEInfo.ScheduleDpeWork);
	#endif
	if (isDvsDone == MTRUE)
//#if 1
		schedule_work(&logWork);
//#else
//tasklet_schedule(DPE_tasklet[DPE_IRQ_TYPE_INT_DVP_ST].pDPE_tkt);
//#endif
	return IRQ_HANDLED;
}

static irqreturn_t ISP_Irq_DVGF(signed int Irq, void *DeviceId)
{
	unsigned int DvsStatus = 0, DvpStatus = 0, DvgfStatus = 0;
	bool bResulst = MFALSE;
	bool isDvsDone = MFALSE;
	pid_t ProcessID;
	unsigned int p = 0;
	//unsigned int CMDQ_Value = 0;

	if (g_u4EnableClockCount > 0) {
		DvsStatus = DPE_RD32(DVS_CTRL_STATUS0_REG);	/* DVS Status */
		DvpStatus = DPE_RD32(DVP_CTRL_STATUS0_REG);	/* DVP Status */
		DvgfStatus = DPE_RD32(DVGF_CTRL_STATUS0_REG);	/* DVGF Status */
	} else {
		LOG_INF("DVGF DPE Power not Enable\n");
		return IRQ_HANDLED;
	}

		//LOG_INF("DVGF IRQ\n");
		//CMDQ_Value = cmdq_get_event(dpe_clt->chan, 473);
		//LOG_INF("CMDQ_Value = %d\n", CMDQ_Value);

	if ((DvsStatus == 0) || (DvpStatus == 0) || (DvgfStatus == 0))
		LOG_INF("DPE IRQ , DvsSt:0x%08x,DvpSt: 0x%08x,DvgfSt: 0x%08x\n",
		DvsStatus, DvpStatus, DvgfStatus);

	if (DPE_debug_log_en == 1) {
		LOG_INF("DVGF IRQ, DvsSt:0x%08x,DvpSt: 0x%08x,DvgfSt: 0x%08x\n",
		DvsStatus, DvpStatus, DvgfStatus);
	}

	/* DPE done status may rise later, so can't use done status now  */
	/* if (DPE_INT_ST == (DPE_INT_ST & DvsStatus)) { */

		DPE_WR32(DVGF_IRQ_00_REG, 0x040000F0); /* Clear DVGF IRQ */
		DPE_WR32(DVGF_IRQ_00_REG, 0x04000F00);

		isDvsDone = MTRUE;
	/* } */
	spin_lock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
	if (isDvsDone == MTRUE) {
		/* Update the frame status. */
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("dpe_irq");
#endif
		if (dpe_update_request_isp8(&dpe_reqs_dvgf, &ProcessID) == 0) {
			bResulst = MTRUE;
		}	else {
			LOG_INF("dvdf_update_request bResulst fail = %d\n",
			bResulst);
			spin_unlock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
			return IRQ_HANDLED;
		}

		if (bResulst == MTRUE) {
			#if REQUEST_REGULATION == REQUEST_BASE_REGULATION
			/* schedule_work(&DPEInfo.ScheduleDpeWork); */
			queue_work(DPEInfo.wkqueue, &DPEInfo.DVGF_ScheduleDpeWork);
			#endif
			p = ProcessID % IRQ_USER_NUM_MAX;
			DPEInfo.IrqInfo.Status[DPE_IRQ_TYPE_INT_DVP_ST] |=
				DPE_INT_ST;
			DPEInfo.IrqInfo.ProcessID[p] =
			ProcessID;

			DPEInfo.IrqInfo.DpeIrqCnt[p]++;
			//Get_DVGF_IRQ++;
			//LOG_INF("DVGF DPEInfo.IrqInfo.DpeIrqCnt[p] =%d,p =%d\n",
			//DPEInfo.IrqInfo.DpeIrqCnt[p], p);

			DPEInfo.ProcessID[DPEInfo.WriteReqIdx] = ProcessID;
			DPEInfo.WriteReqIdx =
				(DPEInfo.WriteReqIdx + 1) %
				_SUPPORT_MAX_DPE_FRAME_REQUEST_;
		} else {
			LOG_INF("dvgf_update_request_isp8 bResulst fail = %d\n",
			bResulst);
		}
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif
		/* Config the Next frame */
	}
	spin_unlock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DVP_ST]));
	if (bResulst == MTRUE)
		wake_up_interruptible(&DPEInfo.WaitQueueHead);
	/* dump log, use tasklet */
	IRQ_LOG_KEEPER(
		DPE_IRQ_TYPE_INT_DVGF_ST,
		m_CurrentPPB,
		_LOG_INF,
		"DVGF_IRQ:%d,0x%x:0x%x,0x%x:0x%x,Result:%d\n",
		Irq,
		DVS_CTRL_STATUS0_HW,
		DvsStatus,
		DVGF_CTRL_STATUS0_HW,
		DvgfStatus,
		bResulst);
	IRQ_LOG_KEEPER(
		DPE_IRQ_TYPE_INT_DVGF_ST,
		m_CurrentPPB,
		_LOG_INF,
		"DVGF_IRQ:IrqCnt[%d]:0x%x,WReq:0x%x,RReq:0x%x\n",
		p, DPEInfo.IrqInfo.DpeIrqCnt[p],
		DPEInfo.WriteReqIdx,
		DPEInfo.ReadReqIdx);
	#if (REQUEST_REGULATION == FRAME_BASE_REGULATION)
	/* schedule_work(&DPEInfo.ScheduleDpeWork); */
	queue_work(DPEInfo.wkqueue, &DPEInfo.DVGF_ScheduleDpeWork);
	#endif
	if (isDvsDone == MTRUE)
//#if 1
		schedule_work(&logWork);
//#else
//tasklet_schedule(DPE_tasklet[DPE_IRQ_TYPE_INT_DVP_ST].pDPE_tkt);
//#endif
	return IRQ_HANDLED;
}
#endif

static void ISP_TaskletFunc_DVGF(unsigned long data)
{
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVGF_ST, m_CurrentPPB, _LOG_DBG);
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVGF_ST, m_CurrentPPB, _LOG_INF);
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVGF_ST, m_CurrentPPB, _LOG_ERR);
}

static void ISP_TaskletFunc_DVP(unsigned long data)
{
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVP_ST, m_CurrentPPB, _LOG_DBG);
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVP_ST, m_CurrentPPB, _LOG_INF);
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVP_ST, m_CurrentPPB, _LOG_ERR);
}
static void ISP_TaskletFunc_DVS(unsigned long data)
{
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVS_ST, m_CurrentPPB, _LOG_DBG);
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVS_ST, m_CurrentPPB, _LOG_INF);
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DVS_ST, m_CurrentPPB, _LOG_ERR);
}
static void logPrint(struct work_struct *data)
{
	unsigned long arg = 0;

	ISP_TaskletFunc_DVP(arg);
	ISP_TaskletFunc_DVS(arg);
	ISP_TaskletFunc_DVGF(arg);
}
/******************************************************************************
 *
 ******************************************************************************/
module_init(DPE_Init);
module_exit(DPE_Exit);
MODULE_DESCRIPTION("Camera DPE driver");
MODULE_AUTHOR("MM3SW2");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_LICENSE("GPL");
