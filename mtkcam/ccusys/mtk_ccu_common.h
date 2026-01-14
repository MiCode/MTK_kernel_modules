/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _MTK_CCU_COMMON_H
#define _MTK_CCU_COMMON_H

#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/remoteproc/mtk_ccu.h>
#include <linux/spinlock.h>
#include <linux/dma-buf.h>
#include <linux/iosys-map.h>

#define SECURE_CCU
/*
 * devm_request_threaded_irq() may return -EINTR and retry fails too.
 * so can not request_irq/free_irq at ccu boot/shutdown.
 */
#define REQUEST_IRQ_IN_INIT

#define MTK_CCU_DEV_NAME            "ccu_rproc"
#define MTK_CCU1_DEV_NAME           "ccu_rproc1"
#define MTK_CCU_CAM_MAIN_NAME       "ccucammain"

#if defined(SECURE_CCU)
#define CCU_FW_NAME					"lib3a.ccu_dummy"
#else
#define CCU_FW_NAME					"lib3a.ccu"
#endif

#define MTK_CCU_MAGICNO             'c'
#define MTK_CCU_IOCTL_WAIT_IRQ               _IOW(MTK_CCU_MAGICNO,   0, int)
#define MTK_CCU_IOCTL_SET_LOG_LEVEL          _IOW(MTK_CCU_MAGICNO,   1, int)
#define MTK_CCU_IOCTL_FLUSH_LOG              _IOW(MTK_CCU_MAGICNO,   2, int)

#define CCU_VER_ISP7	70
#define CCU_VER_ISP71	71
#define CCU_VER_ISP7S	72
#define CCU_VER_ISP7SP	73
#define CCU_VER_ISP7SPL 74
#define CCU_VER_ISP8    80

#define MTK_CCU_CLK_PWR_NUM 20
#define MTK_CCU_CLK_NAME_LEN 32
#define MTK_CCU_MAILBOX_QUEUE_SIZE 8
#define MTK_CCU_MAILBOX_QUEUE_COMPACT_SIZE 4

#define MTK_CCU_SRAM_LOG_OFFSET	 (0x1000)
#define MTK_CCU_DRAM_LOG_BUF_CNT (4)
#define MTK_CCU_DRAM_LOG_BUF_SIZE (128 * 1024) //1MB
#define MTK_CCU_SRAM_LOG_INDRAM_BUF_SIZE (16 * 1024)
#define MTK_CCU_REG_LOG_INDRAM_BUF_SIZE (8 * 1024)
#define MTK_CCU_DRAM_LOG_DBG_BUF_SIZE ((2 * MTK_CCU_DRAM_LOG_BUF_SIZE) + \
		MTK_CCU_SRAM_LOG_INDRAM_BUF_SIZE + MTK_CCU_REG_LOG_INDRAM_BUF_SIZE)
#define MTK_CCU_SRAM_LOG_BUF_SIZE (4 * 1024)
#define MTK_CCU_REG_LOG_BUF_SIZE (4 * 1024)
#define MTK_CCU_EXTRA_REG_LOG_BUF_SIZE (0x100)
#define MTK_CCU_EXTRA_REG_OFFSET (0x8000)
#define MTK_CCU_MRDUMP_SRAM_BUF_SIZE (MTK_CCU_SRAM_LOG_BUF_SIZE + MTK_CCU_REG_LOG_BUF_SIZE)
#define MTK_CCU_MRDUMP_BUF_DRAM_SIZE (128 * 1024)
#define MTK_CCU_MRDUMP_BUF_SIZE (MTK_CCU_MRDUMP_SRAM_BUF_SIZE + 2 * MTK_CCU_MRDUMP_BUF_DRAM_SIZE)
#define LOG_ENDEND 0xDDDDDDDD
#define LOG_DEFAULT_LEVEL 4
#define LOG_DEFAULT_TAG   0x021182A6
#define LOG_BUF_IDX_MAX   2
#define SYSCTRL_IPC_MAGICNO  0x18CD18EC

#define SPARE_REG_OFFSET_SEC	0x20
#define SPARE_REG_OFFSET_AP		(SPARE_REG_OFFSET_SEC + 0x80)
#define SPARE_REG_OFFSET_VM1	(SPARE_REG_OFFSET_AP + 0x80)
#define SPARE_REG_OFFSET_VM2	(SPARE_REG_OFFSET_VM1 + 0x80)

struct mtk_ccu_ipc_desc {
	mtk_ccu_ipc_handle_t handler;
	void *priv;
};

enum mtk_ccu_buffer_type {
	MTK_CCU_DDR,
	MTK_CCU_BUF_MAX
};

struct mtk_ccu_mem_info {
	int fd;
	bool cached;
	unsigned int size;
	dma_addr_t align_mva;
	dma_addr_t mva;
	char *va;
};

struct mtk_ccu_mem_handle {
	struct ion_handle *ionHandleKd;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct mtk_ccu_mem_info meminfo;
	struct iosys_map map;
};

struct mtk_ccu_buffer {
	int buf_idx;
	int32_t fd;
	uint32_t size;
	uint32_t offset;
	dma_addr_t mva;
	char *va;
	u64 ktime;
	uint32_t gtick;
};

struct mtk_ccu_ipc_data {
	bool is_initialized;
	uint32_t *ccuIntTrigPtr;
	struct ap2ccu_ipc *ccuIpcPtr;
	uint8_t *ipcDataPtr;
	uint32_t ipcDataAddrCcu;
	uint32_t ipcDataSize;
};

struct mtk_ccu_msg {
	enum mtk_ccu_feature_type feature_type;
	uint32_t msg_id;
	uint32_t in_data_ptr;
	uint32_t inDataSize;
	uint32_t tg_info;
	uint32_t sensor_idx; //new
};

struct __attribute__((__packed__)) mtk_ccu_msg_compact {
    volatile uint8_t feature_type;
    volatile uint8_t msg_id;
    volatile uint16_t in_data_ptr;
    volatile uint16_t inDataSize;
    volatile uint8_t tg_info;
    volatile uint8_t sensor_idx; //new
};

struct mtk_ccu_mailbox {
	uint32_t front;
	uint32_t rear;
	struct mtk_ccu_msg queue[MTK_CCU_MAILBOX_QUEUE_SIZE];
};

struct mtk_ccu_mailbox_compact {
	uint32_t front;
	uint32_t rear;
	struct mtk_ccu_msg_compact queue[MTK_CCU_MAILBOX_QUEUE_SIZE];
};

struct ap2ccu_ipc {
	uint32_t write_cnt;
	uint32_t read_cnt;
	uint32_t ack;
	struct mtk_ccu_msg msg;
};

struct ap2ccu_ipc_compact {
	uint32_t write_cnt;
	uint32_t read_cnt;
	uint32_t ack;
	struct mtk_ccu_msg_compact msg;
};

struct mtk_ccu {
	struct platform_device *pdev;
	struct platform_device *pdev1;
	struct platform_device *pdev_cammainpwr;
	struct device *dev;
	struct device *dev_cammainpwr;
	struct device *ccu_cdev_dev;
	struct class *ccu_class;
	struct cdev ccu_cdev;
	dev_t dev_no;
	struct rproc *rproc;
	uint32_t ccu_hw_base;
	uint32_t ccu_hw_size;
	uint32_t ccu_version;
	uint32_t ccu_sram_size;
	uint32_t ccu_sram_offset;
	uint32_t ccu_sram_con_offset;
	void __iomem *ccu_base;
	void __iomem *bin_base;
	void __iomem *dmem_base;
	void __iomem *pmem_base;
	void __iomem *ddrmem_base;
	void __iomem *spm_base;
	unsigned int irq_num;
	struct icc_path *path_ccug;
	struct icc_path *path_ccuo;
	struct icc_path *path_ccui;
	struct mtk_ccu_ipc_data ccu_ipc;
	struct mtk_ccu_ipc_desc ipc_desc[MTK_CCU_MSG_TO_APMCU_MAX];
	struct mutex ipc_desc_lock;
	spinlock_t ipc_send_lock;
	spinlock_t ccu_poweron_lock;
	spinlock_t ccu_irq_lock;
	unsigned int clock_num;
	struct mtk_ccu_clk_name *clock_name;
	struct clk *ccu_clk_pwr_ctrl[MTK_CCU_CLK_PWR_NUM];
	struct mtk_ccu_mem_handle buffer_handle[MTK_CCU_BUF_MAX];
	struct mtk_ccu_mem_handle ext_buf;
	bool smmu_enabled;
	void *mrdump_buf;
	struct mtk_ccu_mailbox *mb;
	struct mtk_ccu_mailbox_compact *mb_compact;
	struct mtk_ccu_buffer log_info[MTK_CCU_DRAM_LOG_BUF_CNT];
	wait_queue_head_t WaitQueueHead;
	bool cammainpwr_powered;
	bool poweron;
	bool disirq;
	bool bWaitCond;
	bool compact_ipc;
	int g_LogBufIdx;
	int log_level;
	int log_taglevel;
	uint32_t ipc_tout_fid;
	uint32_t ipc_tout_mid;
	u64 ktime;
	uint32_t gtick;
};

struct mtk_ccu_clk_name {
	bool enable;
	char name[MTK_CCU_CLK_NAME_LEN];
};

extern uint32_t ccu_mailbox_max;

/*---------------------------------------------------------------------------*/
/*  CHARDEV FUNCTIONS                                                        */
/*---------------------------------------------------------------------------*/
void mtk_ccu_unreg_chardev(struct mtk_ccu *ccu);
int mtk_ccu_reg_chardev(struct mtk_ccu *ccu);

/*---------------------------------------------------------------------------*/
/*  common FUNCTIONS                                                         */
/*---------------------------------------------------------------------------*/
void mtk_ccu_memclr(void *dst, int len);
void mtk_ccu_readl(void *dst, const void *src, uint32_t len);
void mtk_ccu_memcpy(void *dst, const void *src, uint32_t len);
struct mtk_ccu_mem_info *mtk_ccu_get_meminfo(struct mtk_ccu *ccu,
	enum mtk_ccu_buffer_type type);

/*---------------------------------------------------------------------------*/
/*  IPI FUNCTIONS                                                            */
/*---------------------------------------------------------------------------*/
void mtk_ccu_rproc_ipc_init(struct mtk_ccu *ccu);
void mtk_ccu_rproc_ipc_uninit(struct mtk_ccu *ccu);
irqreturn_t mtk_ccu_isr_handler(int irq, void *priv);
int mtk_ccu_rproc_ipc_send(struct platform_device *pdev,
	enum mtk_ccu_feature_type featureType,
	uint32_t msgId, void *inDataPtr, uint32_t inDataSize);

/*---------------------------------------------------------------------------*/
/*  System IPC FUNCTIONS                                                     */
/*---------------------------------------------------------------------------*/
void mtk_ccu_ipc_log_handle(uint32_t data, uint32_t len, void *priv);
void mtk_ccu_ipc_assert_handle(uint32_t data, uint32_t len, void *priv);
void mtk_ccu_ipc_warning_handle(uint32_t data, uint32_t len, void *priv);

#endif // _MTK_CCU_COMMON_H
