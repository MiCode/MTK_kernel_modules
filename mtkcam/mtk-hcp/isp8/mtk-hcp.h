/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef MTK_HCP_H
#define MTK_HCP_H

#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <linux/dma-heap.h>
#include <linux/fdtable.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <uapi/linux/dma-heap.h>
#include <linux/types.h>



#include "mtk-hcp-aee.h"
#include "mtk-img-ipi.h"

//#include "scp_ipi.h"

/**
 * HCP (Hetero Control Processor ) is a tiny processor controlling
 * the methodology of register programming. If the module support
 * to run on CM4 then it will send data to CM4 to program register.
 * Or it will send the data to user library and let RED to program
 * register.
 *
 **/

typedef void (*hcp_handler_t) (void *data,
						 unsigned int len,
						 void *priv);


/**
 * hcp ID definition
 */
enum hcp_id {
	HCP_INIT_ID = 0,
	HCP_ISP_CMD_ID,
	HCP_ISP_FRAME_ID,
	HCP_DIP_INIT_ID, //CHRISTBD
	HCP_IMGSYS_INIT_ID = HCP_DIP_INIT_ID,
	HCP_DIP_FRAME_ID,
	HCP_IMGSYS_FRAME_ID = HCP_DIP_FRAME_ID,
	HCP_DIP_HW_TIMEOUT_ID,
	HCP_IMGSYS_HW_TIMEOUT_ID = HCP_DIP_HW_TIMEOUT_ID,
	HCP_IMGSYS_SW_TIMEOUT_ID,
	HCP_DIP_DEQUE_DUMP_ID,
	HCP_IMGSYS_DEQUE_DUMP_ID = HCP_DIP_DEQUE_DUMP_ID,
	HCP_IMGSYS_DEQUE_DONE_ID,
	HCP_IMGSYS_ASYNC_DEQUE_DONE_ID,
	HCP_IMGSYS_DEINIT_ID,
	HCP_IMGSYS_IOVA_FDS_ADD_ID,
	HCP_IMGSYS_IOVA_FDS_DEL_ID,
	HCP_IMGSYS_UVA_FDS_ADD_ID,
	HCP_IMGSYS_UVA_FDS_DEL_ID,
	HCP_IMGSYS_SET_CONTROL_ID,
	HCP_IMGSYS_GET_CONTROL_ID,
	HCP_IMGSYS_CLEAR_HWTOKEN_ID,
	HCP_IMGSYS_AEE_DUMP_ID,
#if SMVR_DECOUPLE
	HCP_IMGSYS_ALOC_WORKING_BUF_ID,
	HCP_IMGSYS_FREE_WORKING_BUF_ID,
#else
	HCP_FD_CMD_ID,
	HCP_FD_FRAME_ID,
	HCP_RSC_INIT_ID,
	HCP_RSC_FRAME_ID,
	HCP_IMGSYS_FENCE_FDS_ADD_ID,
	HCP_IMGSYS_FENCE_FDS_DEL_ID,
#endif
	HCP_MAX_ID,
};

/**
 * module ID definition
 */
enum module_id {
	MODULE_ISP = 0,
	MODULE_DIP,
	MODULE_IMG = MODULE_DIP,
	MODULE_FD,
	MODULE_RSC,
	MODULE_MAX_ID,
};


/**
 * struct flush_buf_info - DMA buffer need to partial flush
 *
 *
 * @id:             hcp id
 * @len:            share buffer length
 * @share_buf:      share buffer data
 */
struct flush_buf_info {
	unsigned int fd;
	unsigned int offset;
	unsigned int len;
	unsigned int mode;
	bool is_tuning;
	struct dma_buf *dbuf;
};

struct tuning_meta_info {
	unsigned int buf_fd;
	unsigned int offset;
	struct dma_buf *dbuf;
	u64 iova_addr;
};
/**
 * mtk_hcp_register - register an hcp function
 *
 * @pdev: HCP platform device
 * @id: HCP ID
 * @handler: HCP handler
 * @name: HCP name
 * @priv: private data for HCP handler
 *
 * Return: Return 0 if hcp registers successfully, otherwise it is failed.
 */
int mtk_hcp_register(struct platform_device *pdev, enum hcp_id id,
				 hcp_handler_t handler, const char *name, void *priv);

/**
 * mtk_hcp_unregister - unregister an hcp function
 *
 * @pdev: HCP platform device
 * @id: HCP ID
 *
 * Return: Return 0 if hcp unregisters successfully, otherwise it is failed.
 */
int mtk_hcp_unregister(struct platform_device *pdev, enum hcp_id id);

/**
 * mtk_hcp_send - send data from camera kernel driver to HCP and wait the
 *      command send to demand.
 *
 * @pdev:   HCP platform device
 * @id:     HCP ID
 * @buf:    the data buffer
 * @len:    the data buffer length
 * @frame_no: frame number
 *
 * This function is thread-safe. When this function returns,
 * HCP has received the data and save the data in the workqueue.
 * After that it will schedule work to dequeue to send data to CM4 or
 * RED for programming register.
 * Return: Return 0 if sending data successfully, otherwise it is failed.
 **/
int mtk_hcp_send(struct platform_device *pdev,
		enum hcp_id id, void *buf,
		unsigned int len, int req_fd);

/**
 * mtk_hcp_send_async - send data from camera kernel driver to HCP without
 *      waiting demand receives the command.
 *
 * @pdev:   HCP platform device
 * @id:     HCP ID
 * @buf:    the data buffer
 * @len:    the data buffer length
 * @frame_no: frame number
 *
 * This function is thread-safe. When this function returns,
 * HCP has received the data and save the data in the workqueue.
 * After that it will schedule work to dequeue to send data to CM4 or
 * RED for programming register.
 * Return: Return 0 if sending data successfully, otherwise it is failed.
 **/
int mtk_hcp_send_async(struct platform_device *pdev,
		 enum hcp_id id, void *buf,
		 unsigned int len, int frame_no);


/**
 * Callback from v4l2 layer to notify daemon apu dc status
 */
int mtk_hcp_set_apu_dc(struct platform_device *pdev,
	int32_t value, size_t size);

/**
 * mtk_hcp_get_plat_device - get HCP's platform device
 *
 * @pdev: the platform device of the module requesting HCP platform
 *    device for using HCP API.
 *
 * Return: Return NULL if it is failed.
 * otherwise it is HCP's platform device
 **/
struct platform_device *mtk_hcp_get_plat_device(struct platform_device *pdev);

/**
 * mtk_hcp_get_current_task - get hcp driver's task struct.
 *
 * @pdev: HCP platform device
 *
 * This function returns the current task inside HCP platform device,
 * which is initialized when hcp device being opened.
 **/
struct task_struct *mtk_hcp_get_current_task(struct platform_device *pdev);

/**
 * mtk_hcp_allocate_working_buffer - allocate driver working buffer.
 *
 * @pdev: HCP platform device
 *
 * This function allocate working buffers and store the information
 * in mtk_hcp_reserve_mblock.
 **/
#if !SMVR_DECOUPLE
int mtk_hcp_allocate_working_buffer(struct platform_device *pdev, unsigned int mode);
#else
int mtk_hcp_allocate_working_buffer(struct platform_device *pdev, unsigned int mode, unsigned int gmb_en);
#endif

/**
 * mtk_hcp_release_working_buffer - release driver working buffer.
 *
 * @pdev: HCP platform device
 *
 * This function release working buffers
 **/
int mtk_hcp_release_gce_working_buffer(struct platform_device *pdev);

#if !SMVR_DECOUPLE
void *mtk_hcp_get_gce_mem_virt(struct platform_device *pdev);
void *mtk_hcp_get_wpe_mem_virt(struct platform_device *pdev);
int mtk_hcp_get_wpe_mem_cq_fd(struct platform_device *pdev);
int mtk_hcp_get_wpe_mem_tdr_fd(struct platform_device *pdev);
void *mtk_hcp_get_omc_mem_virt(struct platform_device *pdev);
int mtk_hcp_get_omc_mem_cq_fd(struct platform_device *pdev);
int mtk_hcp_get_omc_mem_tdr_fd(struct platform_device *pdev);
void *mtk_hcp_get_dip_mem_virt(struct platform_device *pdev);
int mtk_hcp_get_dip_mem_cq_fd(struct platform_device *pdev);
int mtk_hcp_get_dip_mem_tdr_fd(struct platform_device *pdev);
void *mtk_hcp_get_traw_mem_virt(struct platform_device *pdev);
int mtk_hcp_get_traw_mem_cq_fd(struct platform_device *pdev);
int mtk_hcp_get_traw_mem_tdr_fd(struct platform_device *pdev);
void *mtk_hcp_get_pqdip_mem_virt(struct platform_device *pdev);
int mtk_hcp_get_pqdip_mem_cq_fd(struct platform_device *pdev);
int mtk_hcp_get_pqdip_mem_tdr_fd(struct platform_device *pdev);
void *mtk_hcp_get_me_mem_virt(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_me_mem_cq_fd(struct platform_device *pdev, unsigned int mode);
void *mtk_hcp_get_hwid_mem_virt(struct platform_device *pdev);
int mtk_hcp_get_gce_buffer(struct platform_device *pdev);
int mtk_hcp_put_gce_buffer(struct platform_device *pdev);

#else
int mtk_hcp_ioc_release_working_buffer(struct platform_device *pdev, unsigned int mode);
void *mtk_hcp_get_gce_mem_virt(struct platform_device *pdev, unsigned int mode);
void *mtk_hcp_get_gce_token_mem_virt(struct platform_device *pdev, unsigned int mode);
void *mtk_hcp_get_wpe_mem_virt(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_wpe_mem_cq_fd(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_wpe_mem_tdr_fd(struct platform_device *pdev, unsigned int mode);
void *mtk_hcp_get_omc_mem_virt(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_omc_mem_cq_fd(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_omc_mem_tdr_fd(struct platform_device *pdev, unsigned int mode);
void *mtk_hcp_get_dip_mem_virt(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_dip_mem_cq_fd(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_dip_mem_tdr_fd(struct platform_device *pdev, unsigned int mode);
void *mtk_hcp_get_traw_mem_virt(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_traw_mem_cq_fd(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_traw_mem_tdr_fd(struct platform_device *pdev, unsigned int mode);
void *mtk_hcp_get_pqdip_mem_virt(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_pqdip_mem_cq_fd(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_pqdip_mem_tdr_fd(struct platform_device *pdev, unsigned int mode);
void *mtk_hcp_get_me_mem_virt(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_me_mem_cq_fd(struct platform_device *pdev, unsigned int mode);
void *mtk_hcp_get_hwid_mem_virt(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_get_mem_info(struct platform_device *pdev, struct img_init_info *info, unsigned int mode);
int mtk_hcp_get_gce_buffer(struct platform_device *pdev, unsigned int mode);
int mtk_hcp_put_gce_buffer(struct platform_device *pdev, unsigned int mode);

#endif
int mtk_hcp_release_working_buffer(struct platform_device *pdev);

int mtk_hcp_get_init_info(struct platform_device *pdev, struct img_init_info *info);

int mtk_hcp_partial_flush(struct platform_device *pdev, struct flush_buf_info *b_info);

/**
 * mtk_hcp_purge_msg - purge messages
 *
 * @pdev: HCP platform device
 *
 * This function purges messages
 **/
void mtk_hcp_purge_msg(struct platform_device *pdev);

/**
 * mtk_hcp_kernel_db_write - Write a buffer into kernel DB.
 *
 * @pdev:   HCP platform device
 * @buf:    the data buffer
 * @len:    the data buffer length
 *
 * This function is thread-safe. When this function returns
 * Note this function should use in user context.
 *
 * Return: Return actual size which has been write into kernel DB. Caller
 * should compare return value with argument "len" to determine whether
 * whole data has been written successfully.
 **/
ssize_t mtk_hcp_kernel_db_write(struct platform_device *pdev,
		const char *buf, size_t len);

/**
 * struct hcp_mem - HCP memory information
 *
 * @d_va:    the kernel virtual memory address of HCP extended data memory
 * @d_pa:    the physical memory address of HCP extended data memory
 * @d_len:   the length of extended data
 */
struct hcp_mem {
	void *d_va;
	dma_addr_t d_pa;
	unsigned long d_len;
};

/**
 * struct hcp_desc - hcp descriptor
 *
 * @handler:      IPI handler
 * @name:         the name of IPI handler
 * @priv:         the private data of IPI handler
 */
struct hcp_desc {
	hcp_handler_t handler;
	const char *name;
	void *priv;
};

struct object_id {
	union {
		struct send {
			uint32_t hcp: 5;
			uint32_t ack: 1;
			uint32_t req: 10;
			uint32_t seq: 16;
		} __packed send;

		uint32_t cmd;
	};
} __packed;

#if !SMVR_DECOUPLE
#define HCP_SHARE_BUF_SIZE      388
#else
#define HCP_SHARE_BUF_SIZE      1500
#endif

/**
 * struct share_buf - DTCM (Data Tightly-Coupled Memory) buffer shared with
 *                    RED and HCP
 *
 * @id:             hcp id
 * @len:            share buffer length
 * @share_buf:      share buffer data
 */
struct share_buf {
	uint32_t id;
	uint32_t len;
	uint8_t share_data[HCP_SHARE_BUF_SIZE];
	struct object_id info;
};

#if 0
enum Mem_Mode {
	imgsys_streaming = 0,
	imgsys_capture,
	imgsys_smvr
};
#endif
/**
 * struct mtk_hcp - hcp driver data
 * @extmem:              hcp extended memory information
 * @hcp_desc:            hcp descriptor
 * @dev:                 hcp struct device
 * @mem_ops:             memory operations
 * @hcp_mutex:           protect mtk_hcp (except recv_buf) and ensure only
 *                       one client to use hcp service at a time.
 * @data_mutex:          protect shared buffer between kernel user send and
 *                       user thread get&read/copy
 * @file:                hcp daemon file pointer
 * @is_open:             the flag to indicate if hcp device is open.
 * @ack_wq:              the wait queue for each client. When sleeping
 *                       processes wake up, they will check the condition
 *                       "hcp_id_ack" to run the corresponding action or
 *                       go back to sleep.
 * @hcp_id_ack:          The ACKs for registered HCP function.
 * @ipi_got:             The flags for IPI message polling from user.
 * @ipi_done:            The flags for IPI message polling from user again,
 *       which means the previous messages has been dispatched
 *                       done in daemon.
 * @user_obj:            Temporary share_buf used for hcp_msg_get.
 * @hcp_devno:           The hcp_devno for hcp init hcp character device
 * @hcp_cdev:            The point of hcp character device.
 * @hcp_class:           The class_create for create hcp device
 * @hcp_device:          hcp struct device
 * @hcpname:             hcp struct device name in dtsi
 * @ cm4_support_list    to indicate which module can run in cm4 or it will send
 *                       to user space for running action.
 * @ current_task        hcp current task struct
 */
struct mtk_hcp {
	atomic_t have_slb;
	struct hcp_mem extmem;
	struct hcp_desc hcp_desc_table[HCP_MAX_ID];
	struct device *dev;
	struct device *smmu_dev;
	const struct vb2_mem_ops *mem_ops;
	const struct mtk_hcp_data *data;
	/* for protecting vcu data structure */
	struct file *file;
	bool   is_open;
	atomic_t seq;
	wait_queue_head_t ack_wq[MODULE_MAX_ID];
	atomic_t hcp_id_ack[HCP_MAX_ID];
	wait_queue_head_t get_wq[MODULE_MAX_ID];
	wait_queue_head_t poll_wq[MODULE_MAX_ID];
	struct list_head chans[MODULE_MAX_ID];
	struct list_head msg_list;
	spinlock_t msglock;
	wait_queue_head_t msg_wq;
	dev_t hcp_devno;
	struct cdev hcp_cdev;
	struct class *hcp_class;
	struct device *hcp_device;
	const char *hcpname;
	bool cm4_support_list[MODULE_MAX_ID];
	struct task_struct *current_task;
	struct workqueue_struct *daemon_notify_wq[MODULE_MAX_ID];
	struct hcp_aee_info aee_info;
#if SMVR_DECOUPLE
    bool is_mem_alloc_streaming;
    bool is_mem_alloc_capture;
    bool is_mem_alloc_smvr;
	int  alloc_count;
#endif
};

struct mtk_hcp_data {
#if SMVR_DECOUPLE
    struct mtk_hcp_streaming_reserve_mblock *mblock;
	struct mtk_hcp_smvr_reserve_mblock *smblock;
    struct mtk_hcp_capture_reserve_mblock *cmblock;
    struct mtk_hcp_gce_token_reserve_mblock *gmblock;
    unsigned int block_num_gce;
#else
	struct mtk_hcp_reserve_mblock *mblock;
	struct mtk_hcp_reserve_mblock *smblock;
#endif
	unsigned int block_num;
#if SMVR_DECOUPLE
	int (*allocate)(struct mtk_hcp *hcp_dev, unsigned int mode, unsigned int gmb_en);
    int (*release)(struct mtk_hcp *hcp_dev, unsigned int mode);
int (*release_gce_buf)(struct mtk_hcp *hcp_dev);
    int (*get_mem_info)(struct img_init_info *info);
	void* (*get_gce_virt)(unsigned int mode);
	int (*get_gce)(unsigned int mode);
	int (*put_gce)(unsigned int mode);
void* (*get_gce_token_virt)(unsigned int mode);
	void* (*get_hwid_virt)(unsigned int mode);
	void* (*get_wpe_virt)(unsigned int mode);
	int (*get_wpe_cq_fd)(unsigned int mode);
	int (*get_wpe_tdr_fd)(unsigned int mode);
	void* (*get_omc_virt)(unsigned int mode);
	int (*get_omc_cq_fd)(unsigned int mode);
	int (*get_omc_tdr_fd)(unsigned int mode);
	void* (*get_dip_virt)(unsigned int mode);
	int (*get_dip_cq_fd)(unsigned int mode);
	int (*get_dip_tdr_fd)(unsigned int mode);
	void* (*get_traw_virt)(unsigned int mode);
	int (*get_traw_cq_fd)(unsigned int mode);
	int (*get_traw_tdr_fd)(unsigned int mode);
	void* (*get_pqdip_virt)(unsigned int mode);
	int (*get_pqdip_cq_fd)(unsigned int mode);
	int (*get_pqdip_tdr_fd)(unsigned int mode);
	void* (*get_me_virt)(unsigned int mode);
	int (*get_me_cq_fd)(unsigned int mode);
#else
	int (*allocate)(struct mtk_hcp *hcp_dev, unsigned int smvr);
	int (*release)(struct mtk_hcp *hcp_dev);
	void* (*get_gce_virt)(void);
	int (*get_gce)(void);
	int (*put_gce)(void);
	void* (*get_hwid_virt)(void);
	void* (*get_wpe_virt)(void);
	int (*get_wpe_cq_fd)(void);
	int (*get_wpe_tdr_fd)(void);
	void* (*get_omc_virt)(void);
	int (*get_omc_cq_fd)(void);
	int (*get_omc_tdr_fd)(void);
	void* (*get_dip_virt)(void);
	int (*get_dip_cq_fd)(void);
	int (*get_dip_tdr_fd)(void);
	void* (*get_traw_virt)(void);
	int (*get_traw_cq_fd)(void);
	int (*get_traw_tdr_fd)(void);
	void* (*get_pqdip_virt)(void);
	int (*get_pqdip_cq_fd)(void);
	int (*get_pqdip_tdr_fd)(void);
	void* (*get_me_virt)(unsigned int mode);
	int (*get_me_cq_fd)(unsigned int mode);
#endif
	int (*get_init_info)(struct img_init_info *info);

	int (*partial_flush)(struct mtk_hcp *hcp_dev, struct flush_buf_info *b_info);
};

#define HCP_RESERVED_MEM  (1)
#define MTK_CM4_SUPPORT     (0)

#if HCP_RESERVED_MEM
/**
 * struct mtk_hcp_reserve_mblock - info about memory buffer allocated in kernel
 *
 * @num:        vb2_dc_buf
 * @start_phys:     starting addr(phy/iova) about allocated buffer
 * @start_virt:     starting addr(kva) about allocated buffer
 * @start_dma:      starting addr(iova) about allocated buffer
 * @size:       allocated buffer size
 * @is_dma_buf:     attribute: is_dma_buf or not
 * @mmap_cnt:     counter about mmap times
 * @mem_priv:     vb2_dc_buf
 * @d_buf:        dma_buf
 * @fd:         buffer fd
 */
struct mtk_hcp_reserve_mblock {
	const char *name;
	unsigned int num;
	phys_addr_t start_phys;
	void *start_virt;
	phys_addr_t start_dma;
	phys_addr_t size;
	uint8_t is_dma_buf;
	struct iosys_map map;
	/*new add*/
	int mmap_cnt;
	void *mem_priv;
	struct dma_buf *d_buf;
	int fd;
	struct ion_handle *pIonHandle;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct kref kref;
};
#if SMVR_DECOUPLE
struct mtk_hcp_streaming_reserve_mblock {
	const char *name;
	unsigned int num;
	phys_addr_t start_phys;
	void *start_virt;
	phys_addr_t start_dma;
	phys_addr_t size;
	uint8_t is_dma_buf;
	struct iosys_map map;
	/*new add*/
	int mmap_cnt;
	void *mem_priv;
	struct dma_buf *d_buf;
	int fd;
	struct ion_handle *pIonHandle;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct kref kref;
    uint8_t streaming;
};

struct mtk_hcp_capture_reserve_mblock {
	const char *name;
	unsigned int num;
	phys_addr_t start_phys;
	void *start_virt;
	phys_addr_t start_dma;
	phys_addr_t size;
	uint8_t is_dma_buf;
	struct iosys_map map;
	/*new add*/
	int mmap_cnt;
	void *mem_priv;
	struct dma_buf *d_buf;
	int fd;
	struct ion_handle *pIonHandle;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct kref kref;
    uint8_t capture;
};

struct mtk_hcp_smvr_reserve_mblock {
	const char *name;
	unsigned int num;
	phys_addr_t start_phys;
	void *start_virt;
	phys_addr_t start_dma;
	phys_addr_t size;
	uint8_t is_dma_buf;
	struct iosys_map map;
	/*new add*/
	int mmap_cnt;
	void *mem_priv;
	struct dma_buf *d_buf;
	int fd;
	struct ion_handle *pIonHandle;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct kref kref;
    uint8_t smvr;
};

struct mtk_hcp_gce_token_reserve_mblock {
	const char *name;
	unsigned int num;
	phys_addr_t start_phys;
	void *start_virt;
	phys_addr_t start_dma;
	phys_addr_t size;
	uint8_t is_dma_buf;
	struct iosys_map map;
	/*new add*/
	int mmap_cnt;
	void *mem_priv;
	struct dma_buf *d_buf;
	int fd;
	struct ion_handle *pIonHandle;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct kref kref;
    uint8_t gce;
};
#endif
int mtk_hcp_set_apu_dc(struct platform_device *pdev,
	int32_t value, size_t size);


extern phys_addr_t mtk_hcp_get_reserve_mem_phys(
					unsigned int id);
extern void *mtk_hcp_get_reserve_mem_virt(unsigned int id);
extern void mtk_hcp_set_reserve_mem_virt(unsigned int id,
	void *virmem);
extern phys_addr_t mtk_hcp_get_reserve_mem_dma(
					unsigned int id);
extern phys_addr_t mtk_hcp_get_reserve_mem_size(
					unsigned int id);
extern uint32_t mtk_hcp_get_reserve_mem_fd(
					unsigned int id);
extern void mtk_hcp_set_reserve_mem_fd(unsigned int id,
	uint32_t fd);
#endif

bool hcp_dbg_enable(void);

#endif /* _MTK_HCP_H */
