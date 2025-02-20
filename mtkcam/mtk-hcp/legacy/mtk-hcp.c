// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <asm/cacheflush.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/file.h>
#include <linux/firmware.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/compat.h>
#include <linux/freezer.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/videodev2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-memops.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/version.h>

#include <aee.h>

#include "mtk_imgsys-dev.h"

#include "mtk-hcp.h"
#include "mtk-hcp-aee.h"
#include "mtk-hcp-support.h"
#include "mtk-hcp_isp71.h"
#include "mtk-hcp_isp7s.h"
#include "mtk-hcp_isp7sp.h"


#ifdef CONFIG_MTK_IOMMU_V2
#include "mtk_iommu_ext.h"
#elif defined(CONFIG_DEVICE_MODULES_MTK_IOMMU)
#include "mach/mt_iommu.h"
#elif defined(CONFIG_MTK_M4U)
#include "m4u.h"
#endif

#ifdef CONFIG_MTK_M4U
#include "m4u.h"
#include "m4u_port.h"
#endif

#include <mtk_heap.h>

#include "slbc_ops.h"

#include <linux/dma-heap.h>
#include <uapi/linux/dma-heap.h>
#include <linux/dma-heap.h>
#include <linux/dma-direction.h>
#include <linux/scatterlist.h>
#include <linux/dma-buf.h>
#include "iommu_debug.h"

/**
 * HCP (Hetero Control Processor ) is a tiny processor controlling
 * the methodology of register programming. If the module support
 * to run on CM4 then it will send data to CM4 to program register.
 * Or it will send the data to user library and let RED to program
 * register.
 *
 **/

#define RED_PATH                "/dev/red"
#define HCP_DEVNAME             "mtk_hcp"

#define HCP_TIMEOUT_MS          4000U
#define HCP_FW_VER_LEN          16
#define MAX_REQUEST_SIZE        10

#define SYNC_SEND               1
#define ASYNC_SEND              0

static struct mtk_hcp *hcp_mtkdev;

#define IPI_MAX_BUFFER_COUNT    (8)

struct packet {
	int32_t module;
	bool more;
	int32_t count;
	struct share_buf *buffer[IPI_MAX_BUFFER_COUNT];
};

#define CTRL_ID_SLB_BASE        (0x01)

struct ctrl_data {
	uint32_t id;
	uint64_t value;
} __packed;

#define HCP_INIT                _IOWR('H', 0, struct share_buf)
#define HCP_GET_OBJECT          _IOWR('H', 1, struct share_buf)
#define HCP_NOTIFY              _IOWR('H', 2, struct share_buf)
#define HCP_COMPLETE            _IOWR('H', 3, struct share_buf)
#define HCP_WAKEUP              _IOWR('H', 4, struct share_buf)
#define HCP_TIMEOUT             _IO('H', 5)

#if IS_ENABLED(CONFIG_COMPAT)
#define COMPAT_HCP_INIT         _IOWR('H', 0, struct share_buf)
#define COMPAT_HCP_GET_OBJECT   _IOWR('H', 1, struct share_buf)
#define COMPAT_HCP_NOTIFY       _IOWR('H', 2, struct share_buf)
#define COMPAT_HCP_COMPLETE     _IOWR('H', 3, struct share_buf)
#define COMPAT_HCP_WAKEUP       _IOWR('H', 4, struct share_buf)
#define COMPAT_HCP_TIMEOUT      _IO('H', 5)
#endif

struct msg {
	struct list_head entry;
	struct share_buf user_obj;
};
#define  MSG_NR (96)

int hcp_dbg_en;
module_param(hcp_dbg_en, int, 0644);

struct mtk_hcp_vb2_buf {
	struct device			*dev;
	void				*vaddr;
	unsigned long			size;
	void				*cookie;
	dma_addr_t			dma_addr;
	unsigned long			attrs;
	enum dma_data_direction		dma_dir;
	struct sg_table			*dma_sgt;
	struct frame_vector		*vec;

	/* MMAP related */
	struct vb2_vmarea_handler	handler;
	refcount_t			refcount;
	struct sg_table			*sgt_base;

	/* DMABUF related */
	struct dma_buf_attachment	*db_attach;

	struct vb2_buffer		*vb;
	bool				non_coherent_mem;
};

static unsigned long mtk_hcp_vb2_get_contiguous_size(struct sg_table *sgt)
{
	struct scatterlist *s;
	dma_addr_t expected = sg_dma_address(sgt->sgl);
	unsigned int i;
	unsigned long size = 0;

	for_each_sgtable_dma_sg(sgt, s, i) {
		if (sg_dma_address(s) != expected)
			break;
		expected += sg_dma_len(s);
		size += sg_dma_len(s);
	}
	return size;
}

/*********************************************/
/*         callbacks for all buffers         */
/*********************************************/

static void *mtk_hcp_vb2_cookie(struct vb2_buffer *vb, void *buf_priv)
{
	struct mtk_hcp_vb2_buf *buf = buf_priv;

	return &buf->dma_addr;
}

static void *mtk_hcp_vb2_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct mtk_hcp_vb2_buf *buf = buf_priv;

	if (buf->vaddr)
		return buf->vaddr;

	if (buf->db_attach) {
		struct iosys_map map;

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
		if (!dma_buf_vmap_unlocked(buf->db_attach->dmabuf, &map))
			buf->vaddr = map.vaddr;
		#else
		if (!dma_buf_vmap(buf->db_attach->dmabuf, &map))
			buf->vaddr = map.vaddr;
		#endif
		return buf->vaddr;
	}

	if (buf->non_coherent_mem)
		buf->vaddr = dma_vmap_noncontiguous(buf->dev, buf->size,
						    buf->dma_sgt);
	return buf->vaddr;
}

static void mtk_hcp_vb2_prepare(void *buf_priv)
{
	struct mtk_hcp_vb2_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;

	/* This takes care of DMABUF and user-enforced cache sync hint */
	if (buf->vb->skip_cache_sync_on_prepare)
		return;

	if (!buf->non_coherent_mem)
		return;

	/* Non-coherent MMAP only */
	if (buf->vaddr)
		flush_kernel_vmap_range(buf->vaddr, buf->size);

	/* For both USERPTR and non-coherent MMAP */
	dma_sync_sgtable_for_device(buf->dev, sgt, buf->dma_dir);
}

static void mtk_hcp_vb2_finish(void *buf_priv)
{
	struct mtk_hcp_vb2_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;

	/* This takes care of DMABUF and user-enforced cache sync hint */
	if (buf->vb->skip_cache_sync_on_finish)
		return;

	if (!buf->non_coherent_mem)
		return;

	/* Non-coherent MMAP only */
	if (buf->vaddr)
		invalidate_kernel_vmap_range(buf->vaddr, buf->size);

	/* For both USERPTR and non-coherent MMAP */
	dma_sync_sgtable_for_cpu(buf->dev, sgt, buf->dma_dir);
}

static int mtk_hcp_vb2_map_dmabuf(void *mem_priv)
{
	struct mtk_hcp_vb2_buf *buf = mem_priv;
	struct sg_table *sgt;
	unsigned long contig_size;

	if (WARN_ON(!buf->db_attach)) {
		pr_err("trying to pin a non attached buffer\n");
		return -EINVAL;
	}

	if (WARN_ON(buf->dma_sgt)) {
		pr_err("dmabuf buffer is already pinned\n");
		return 0;
	}

	/* get the associated scatterlist for this buffer */
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	sgt = dma_buf_map_attachment_unlocked(buf->db_attach, buf->dma_dir);
	#else
	sgt = dma_buf_map_attachment(buf->db_attach, buf->dma_dir);
	#endif
	if (IS_ERR(sgt)) {
		pr_err("Error getting dmabuf scatterlist\n");
		return -EINVAL;
	}

	/* checking if dmabuf is big enough to store contiguous chunk */
	contig_size = mtk_hcp_vb2_get_contiguous_size(sgt);
	if (contig_size < buf->size) {
		pr_err("contiguous chunk is too small %lu/%lu\n",
		       contig_size, buf->size);
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
		dma_buf_unmap_attachment_unlocked(buf->db_attach, sgt, buf->dma_dir);
		#else
		dma_buf_unmap_attachment(buf->db_attach, sgt, buf->dma_dir);
		#endif
		return -EFAULT;
	}

	buf->dma_addr = sg_dma_address(sgt->sgl);
	buf->dma_sgt = sgt;
	buf->vaddr = NULL;

	return 0;
}

static void mtk_hcp_vb2_unmap_dmabuf(void *mem_priv)
{
	struct mtk_hcp_vb2_buf *buf = mem_priv;
	struct sg_table *sgt = buf->dma_sgt;
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(buf->vaddr);

	if (WARN_ON(!buf->db_attach)) {
		pr_err("trying to unpin a not attached buffer\n");
		return;
	}

	if (WARN_ON(!sgt)) {
		pr_err("dmabuf buffer is already unpinned\n");
		return;
	}

	if (buf->vaddr) {
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
		dma_buf_vunmap_unlocked(buf->db_attach->dmabuf, &map);
		#else
		dma_buf_vunmap(buf->db_attach->dmabuf, &map);
		#endif
		buf->vaddr = NULL;
	}
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	dma_buf_unmap_attachment_unlocked(buf->db_attach, sgt, buf->dma_dir);
	#else
	dma_buf_unmap_attachment(buf->db_attach, sgt, buf->dma_dir);
	#endif

	buf->dma_addr = 0;
	buf->dma_sgt = NULL;
}

static void *mtk_hcp_vb2_attach_dmabuf(struct vb2_buffer *vb, struct device *dev,
				  struct dma_buf *dbuf, unsigned long size)
{
	struct mtk_hcp_vb2_buf *buf;
	struct dma_buf_attachment *dba;

	if (dbuf->size < size)
		return ERR_PTR(-EFAULT);

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dev = dev;
	buf->vb = vb;

	/* create attachment for the dmabuf with the user device */
	dba = dma_buf_attach(dbuf, buf->dev);
	if (IS_ERR(dba)) {
		pr_err("failed to attach dmabuf\n");
		kfree(buf);
		return dba;
	}

	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->size = size;
	buf->db_attach = dba;

	return buf;
}

static void mtk_hcp_vb2_detach_dmabuf(void *mem_priv)
{
	struct mtk_hcp_vb2_buf *buf = mem_priv;

	/* if vb2 works correctly you should never detach mapped buffer */
	if (WARN_ON(buf->dma_addr))
		mtk_hcp_vb2_unmap_dmabuf(buf);

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

static unsigned int mtk_hcp_vb2_num_users(void *buf_priv)
{
	struct mtk_hcp_vb2_buf *buf = buf_priv;

	return refcount_read(&buf->refcount);
}

/*hcp vb2 dma config ops*/
static const struct vb2_mem_ops mtk_hcp_dma_contig_memops = {
	/* .alloc		= , */
	/* .put		= , */
	/* .get_dmabuf	= , */
	.cookie		= mtk_hcp_vb2_cookie,
	.vaddr		= mtk_hcp_vb2_vaddr,
	/* .mmap		= , */
	/* .get_userptr	= , */
	/* .put_userptr	= , */
	.prepare	= mtk_hcp_vb2_prepare,
	.finish		= mtk_hcp_vb2_finish,
	.map_dmabuf	= mtk_hcp_vb2_map_dmabuf,
	.unmap_dmabuf	= mtk_hcp_vb2_unmap_dmabuf,
	.attach_dmabuf	= mtk_hcp_vb2_attach_dmabuf,
	.detach_dmabuf	= mtk_hcp_vb2_detach_dmabuf,
	.num_users	= mtk_hcp_vb2_num_users,
};

/**
 * struct my_wq_t - work struct to handle daemon notification
 *
 * @hcp_dev:        hcp device
 * @ioctl_event:    ioctl event id
 * @data_addr:      addr about shared data
 * @task_work:      work struct
 */
struct my_wq_t {
	struct mtk_hcp *hcp_dev;
	unsigned int ioctl_event;
	struct share_buf  data_addr;
	struct work_struct task_work;
};

/*  function prototype declaration */
static void module_notify(struct mtk_hcp *hcp_dev,
						struct share_buf *user_data_addr);
static int hcp_send_internal(struct mtk_hcp *hcp_dev,
							enum hcp_id id, void *buf,
							unsigned int len, int req_fd,
							unsigned int wait);
/*  End */

static struct msg *msg_pool_get(struct mtk_hcp *hcp_dev)
{
	unsigned long flag = 0;
	unsigned long empty = 0;
	struct msg *msg = NULL;

	spin_lock_irqsave(&hcp_dev->msglock, flag);
	empty = list_empty(&hcp_dev->msg_list);
	if (!empty) {
		msg = list_first_entry(&hcp_dev->msg_list, struct msg, entry);
		list_del(&msg->entry);
	}
	spin_unlock_irqrestore(&hcp_dev->msglock, flag);

	return msg;
}

static void chans_pool_dump(struct mtk_hcp *hcp_dev)
{
	unsigned long flag = 0;
	struct msg *msg = NULL;
	struct msg *tmp = NULL;
	int i = 0;
	int seq_id = 0;
	int req_fd = 0;
	int hcp_id = 0;

	spin_lock_irqsave(&hcp_dev->msglock, flag);
	for (i = 0; i < MODULE_MAX_ID; i++) {
		dev_info(hcp_dev->dev, "HCP(%d) stalled IPI object+\n", i);

		list_for_each_entry_safe(msg, tmp,
				&hcp_dev->chans[i], entry){

			seq_id = msg->user_obj.info.send.seq;
			req_fd = msg->user_obj.info.send.req;
			hcp_id = msg->user_obj.info.send.hcp;

			dev_info(hcp_dev->dev, "req_fd(%d), seq_id(%d), hcp_id(%d)\n",
				req_fd, seq_id, hcp_id);
		}

		dev_info(hcp_dev->dev, "HCP(%d) stalled IPI object-\n", i);
	}
	spin_unlock_irqrestore(&hcp_dev->msglock, flag);
}

static struct msg *chan_pool_get
	(struct mtk_hcp *hcp_dev, unsigned int module_id)
{
	unsigned long flag = 0;
	unsigned long empty = 0;
	struct msg *msg = NULL;

	spin_lock_irqsave(&hcp_dev->msglock, flag);
	empty = list_empty(&hcp_dev->chans[module_id]);
	if (!empty) {
		msg = list_first_entry(&hcp_dev->chans[module_id], struct msg, entry);
		list_del(&msg->entry);
	}
	empty = list_empty(&hcp_dev->chans[module_id]);
	spin_unlock_irqrestore(&hcp_dev->msglock, flag);

	// dev_info(hcp_dev->dev, "chan pool empty(%d)\n", empty);

	return msg;
}

static bool chan_pool_available(struct mtk_hcp *hcp_dev, int module_id)
{
	unsigned long flag = 0;
	unsigned long empty = 0;

	spin_lock_irqsave(&hcp_dev->msglock, flag);
	empty = list_empty(&hcp_dev->chans[module_id]);
	spin_unlock_irqrestore(&hcp_dev->msglock, flag);

	// dev_info(hcp_dev->dev, "chan pool abailable(%d)\n", !empty);

	return (!empty);
}

inline int hcp_id_to_ipi_id(struct mtk_hcp *hcp_dev, enum hcp_id id)
{
	int ipi_id = -EINVAL;

	if (id >= HCP_MAX_ID) {
		dev_info(hcp_dev->dev, "%s: Invalid hcp id %d\n", __func__, id);
		return -EINVAL;
	}

	switch (id) {

	default:
		break;
	}

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "ipi_id:%d\n", ipi_id);
	return ipi_id;
}

inline int hcp_id_to_module_id(struct mtk_hcp *hcp_dev, enum hcp_id id)
{
	int module_id = -EINVAL;

	if (id >= HCP_MAX_ID) {
		dev_info(hcp_dev->dev, "%s: Invalid hcp id %d\n", __func__, id);
		return -EINVAL;
	}

	switch (id) {
	case HCP_ISP_CMD_ID:
	case HCP_ISP_FRAME_ID:
		module_id = MODULE_ISP;
		break;
	case HCP_DIP_INIT_ID:
	case HCP_DIP_FRAME_ID:
	case HCP_IMGSYS_HW_TIMEOUT_ID:
	case HCP_IMGSYS_SW_TIMEOUT_ID:
	case HCP_DIP_DEQUE_DUMP_ID:
	case HCP_IMGSYS_DEQUE_DONE_ID:
	case HCP_IMGSYS_ASYNC_DEQUE_DONE_ID:
	case HCP_IMGSYS_DEINIT_ID:
	case HCP_IMGSYS_IOVA_FDS_ADD_ID:
	case HCP_IMGSYS_IOVA_FDS_DEL_ID:
	case HCP_IMGSYS_UVA_FDS_ADD_ID:
	case HCP_IMGSYS_UVA_FDS_DEL_ID:
	case HCP_IMGSYS_SET_CONTROL_ID:
	case HCP_IMGSYS_GET_CONTROL_ID:
#if SMVR_DECOUPLE
    case HCP_IMGSYS_ALOC_WORKING_BUF_ID:
    case HCP_IMGSYS_FREE_WORKING_BUF_ID:
#endif
		module_id = MODULE_DIP;
		break;
#if !SMVR_DECOUPLE
	case HCP_RSC_INIT_ID:
	case HCP_RSC_FRAME_ID:
		module_id = MODULE_RSC;
		break;
#endif
	default:
		break;
	}

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "module_id:%d\n", module_id);
	return module_id;
}

inline int ipi_id_to_hcp_id(int id)
{
	int hcp_id = HCP_INIT_ID;

	switch (id) {

	default:
		break;
	}
    if (hcp_dbg_enable())
	pr_debug("[%s]hcp_id:%d\n", __func__, hcp_id);
	return hcp_id;
}

static inline bool mtk_hcp_running(struct mtk_hcp *hcp_dev)
{
	return hcp_dev->is_open;
}

#ifdef AED_SET_EXTRA_FUNC_READY_ON_K515
int hcp_notify_aee(void)
{
	struct msg *msg = NULL;
	char dummy = 0;

	pr_info("HCP trigger AEE dump+\n");
	msg = msg_pool_get(hcp_mtkdev);
	msg->user_obj.id = HCP_IMGSYS_AEE_DUMP_ID;
	msg->user_obj.len = 0;
	msg->user_obj.info.send.hcp = HCP_IMGSYS_AEE_DUMP_ID;
	msg->user_obj.info.send.req = 0;
	msg->user_obj.info.send.ack = 0;

	hcp_send_internal(hcp_mtkdev, HCP_IMGSYS_AEE_DUMP_ID, &dummy, 1, 0, 1);
	module_notify(hcp_mtkdev, &msg->user_obj);

	pr_info("HCP trigger AEE dump-\n");

	return 0;
}
#endif

int mtk_hcp_proc_open(struct inode *inode, struct file *file)
{
	struct mtk_hcp *hcp_dev = hcp_mtkdev;

	const char *name = NULL;

	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	name = file->f_path.dentry->d_name.name;
	if (!strcmp(name, "daemon")) {
		file->private_data
			= &hcp_dev->aee_info.data[HCP_AEE_PROC_FILE_DAEMON];
	} else if (!strcmp(name, "kernel")) {
		file->private_data
			= &hcp_dev->aee_info.data[HCP_AEE_PROC_FILE_KERNEL];
	} else if (!strcmp(name, "stream")) {
		file->private_data
			= &hcp_dev->aee_info.data[HCP_AEE_PROC_FILE_STREAM];
	} else {
		pr_info("unknown proc file(%s)", name);
		module_put(THIS_MODULE);
		return -EPERM;
	}

	if (file->private_data == NULL) {
		pr_info("failed to allocate proc file(%s) buffer", name);
		module_put(THIS_MODULE);
		return -ENOMEM;
	}

	pr_info("%s: %s opened\n", __func__, name);

	return 0;
}

static ssize_t mtk_hcp_proc_read(struct file *file, char __user *buf,
	size_t lbuf, loff_t *ppos)
{
	struct mtk_hcp *hcp_dev = hcp_mtkdev;
	struct hcp_proc_data *data = (struct hcp_proc_data *)file->private_data;
	int remain = 0;
	int len = 0;
	int ret = 0;

	ret = mutex_lock_killable(&data->mtx);
	if (ret == -EINTR) {
		pr_info("mtx lock failed due to process being killed");
		return ret;
	}

	remain = data->cnt - *ppos;
	len = (remain > lbuf) ? lbuf : remain;
	if (len == 0) {
		mutex_unlock(&data->mtx);
        if (hcp_dbg_enable())
		dev_dbg(hcp_dev->dev, "Reached end of the device on a read");
		return 0;
	}

	len = len - copy_to_user(buf, data->buf + *ppos, len);
	*ppos += len;

	mutex_unlock(&data->mtx);

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "Leaving the READ function, len=%d, pos=%d\n",
		len, (int)*ppos);

	return len;
}

ssize_t mtk_hcp_kernel_db_write(struct platform_device *pdev,
		const char *buf, size_t sz)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	struct hcp_aee_info *info = &hcp_dev->aee_info;
	struct hcp_proc_data *data
		= (struct hcp_proc_data *)&info->data[HCP_AEE_PROC_FILE_KERNEL];
	size_t remain = 0;
	size_t len = 0;
	int ret = 0;

	ret = mutex_lock_killable(&data->mtx);
	if (ret == -EINTR) {
		pr_info("mtx lock failed due to process being killed");
		return ret;
	}

	remain = data->sz - data->cnt;
	len = (remain > sz) ? sz : remain;

	if (len == 0) {
        if (hcp_dbg_enable())
		dev_dbg(hcp_dev->dev, "Reach end of the file on write");
		mutex_unlock(&data->mtx);
		return 0;
	}

	memcpy(data->buf + data->cnt, buf, len);
	data->cnt += len;

	mutex_unlock(&data->mtx);

	return len;
}
EXPORT_SYMBOL(mtk_hcp_kernel_db_write);

static ssize_t mtk_hcp_proc_write(struct file *file, const char __user *buf,
	size_t lbuf, loff_t *ppos)
{
	struct mtk_hcp *hcp_dev = hcp_mtkdev;
	struct hcp_proc_data *data = (struct hcp_proc_data *)file->private_data;
	ssize_t remain = 0;
	ssize_t len = 0;
	int ret = 0;

	ret = mutex_lock_killable(&data->mtx);
	if (ret == -EINTR) {
		pr_info("mtx lock failed due to process being killed");
		return 0;
	}

	remain = data->sz - data->cnt;
	len = (remain > lbuf) ? lbuf : remain;
	if (len == 0) {
		mutex_unlock(&data->mtx);
        if (hcp_dbg_enable())
		dev_dbg(hcp_dev->dev, "Reached end of the device on a write");
		return 0;
	}

	len = len - copy_from_user(data->buf + data->cnt, buf, len);

	data->cnt += len;
	*ppos = data->cnt;

	mutex_unlock(&data->mtx);

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "Leaving the WRITE function, len=%ld, pos=%lu\n",
		len, data->cnt);

	return len;
}

int mtk_hcp_proc_close(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

static const struct proc_ops aee_ops = {
	.proc_open = mtk_hcp_proc_open,
	.proc_read  = mtk_hcp_proc_read,
	.proc_write = mtk_hcp_proc_write,
	.proc_release = mtk_hcp_proc_close
};

static void hcp_aee_reset(struct mtk_hcp *hcp_dev)
{
	int i = 0;
	struct hcp_aee_info *info = &hcp_dev->aee_info;

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "%s -s\n", __func__);

	if (info == NULL) {
		dev_info(hcp_dev->dev, " %s - aee_info is NULL\n", __func__);
		return;
	}

	for (i = 0 ; i < HCP_AEE_PROC_FILE_NUM ; i++) {
		memset(info->data[i].buf, 0, info->data[i].sz);
		info->data[i].cnt = 0;
	}

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "%s -e\n", __func__);
}

int hcp_aee_init(struct mtk_hcp *hcp_dev)
{
	struct hcp_aee_info *info = NULL;
	struct hcp_proc_data *data = NULL;
	kuid_t uid;
	kgid_t gid;
	int i = 0;

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "%s -s\n", __func__);
	info = &hcp_dev->aee_info;

#ifdef AED_SET_EXTRA_FUNC_READY_ON_K515
	aed_set_extra_func(hcp_notify_aee);
#endif
	info->entry = proc_mkdir("mtk_img_debug", NULL);
	if (info->entry == NULL) {
		pr_info("%s: failed to create imgsys debug node\n", __func__);
		return -1;
	}

	for (i = 0 ; i < HCP_AEE_PROC_FILE_NUM; i++) {
		data = &info->data[i];
		data->sz = HCP_AEE_MAX_BUFFER_SIZE;
		mutex_init(&data->mtx);
	}

	hcp_aee_reset(hcp_dev);

	info->daemon =
		proc_create("daemon", 0660, info->entry, &aee_ops);
	info->stream =
		proc_create("stream", 0660, info->entry, &aee_ops);
	info->kernel =
		proc_create("kernel", 0660, info->entry, &aee_ops);

	uid = make_kuid(&init_user_ns, 0);
	gid = make_kgid(&init_user_ns, 1000);

	if (info->daemon)
		proc_set_user(info->daemon, uid, gid);
	else
		pr_info("%s: mtk_img_dbg/daemon: failed to set u/g", __func__);

	if (info->stream)
		proc_set_user(info->stream, uid, gid);
	else
		pr_info("%s: mtk_img_dbg/stream: failed to set u/g", __func__);

	if (info->kernel)
		proc_set_user(info->kernel, uid, gid);
	else
		pr_info("%s: mtk_img_dbg/kernel: failed to set u/g", __func__);

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "%s - e\n", __func__);
	return 0;
}

int hcp_aee_deinit(struct mtk_hcp *hcp_dev)
{
	struct hcp_aee_info *info = &hcp_dev->aee_info;
	struct hcp_proc_data *data = NULL;
	int i = 0;

	for (i = 0 ; i < HCP_AEE_PROC_FILE_NUM; i++) {
		data = &info->data[i];
		data->sz = HCP_AEE_MAX_BUFFER_SIZE;
		mutex_destroy(&data->mtx);
	}

	if (info->kernel)
		proc_remove(info->kernel);

	if (info->daemon)
		proc_remove(info->daemon);

	if (info->stream)
		proc_remove(info->stream);

	if (info->entry)
		proc_remove(info->entry);

	return 0;
}

#if MTK_CM4_SUPPORT
static void hcp_ipi_handler(int id, void *data, unsigned int len)
{
	int hcp_id = ipi_id_to_hcp_id(id);

	if (hcp_mtkdev->hcp_desc_table[hcp_id].handler)
		hcp_mtkdev->hcp_desc_table[hcp_id].handler(data, len, NULL);
}
#endif

static void cm4_support_table_init(struct mtk_hcp *hcp)
{
	int i = 0;

	for (i = 0; i < MODULE_MAX_ID; i++)
		hcp->cm4_support_list[i] = false;

	i = 0;
	while (CM4_SUPPORT_CONFIGURE_TABLE[i][0] < MODULE_MAX_ID) {
		if (CM4_SUPPORT_CONFIGURE_TABLE[i][1] == 1)
			hcp->cm4_support_list[CM4_SUPPORT_CONFIGURE_TABLE[i][0]]
									= true;
		i++;
	}
}

static bool mtk_hcp_cm4_support(struct mtk_hcp *hcp_dev, enum hcp_id id)
{
	bool is_cm4_support = false;

	switch (id) {
	case HCP_ISP_CMD_ID:
	case HCP_ISP_FRAME_ID:
		is_cm4_support = hcp_dev->cm4_support_list[MODULE_ISP];
		break;
	case HCP_DIP_INIT_ID:
	case HCP_DIP_FRAME_ID:
	case HCP_IMGSYS_HW_TIMEOUT_ID:
	case HCP_IMGSYS_SW_TIMEOUT_ID:
		is_cm4_support = hcp_dev->cm4_support_list[MODULE_DIP];
		break;
#if !SMVR_DECOUPLE
	case HCP_FD_CMD_ID:
	case HCP_FD_FRAME_ID:
		is_cm4_support = hcp_dev->cm4_support_list[MODULE_FD];
		break;
	case HCP_RSC_INIT_ID:
	case HCP_RSC_FRAME_ID:
		is_cm4_support = hcp_dev->cm4_support_list[MODULE_RSC];
		break;
#endif
	default:
		break;
	}

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "cm4 support status:%d\n", is_cm4_support);
	return is_cm4_support;
}

int mtk_hcp_register(struct platform_device *pdev,
				 enum hcp_id id, hcp_handler_t handler,
				 const char *name, void *priv)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	unsigned int idx = 0;

	if (hcp_dev == NULL) {
		pr_info("%s hcp device in not ready\n", __func__);
		return -EPROBE_DEFER;
	}

	if (id < HCP_MAX_ID && handler != NULL) {
#if MTK_CM4_SUPPORT
		if (mtk_hcp_cm4_support(hcp_dev, id) == true) {
			int ipi_id = hcp_id_to_ipi_id(hcp_dev, id);

			scp_ipi_registration(ipi_id, hcp_ipi_handler, name);
		}
#endif
		idx = (unsigned int)id;
		hcp_dev->hcp_desc_table[idx].name = name;
		hcp_dev->hcp_desc_table[idx].handler = handler;
		hcp_dev->hcp_desc_table[idx].priv = priv;
		return 0;
	}

	dev_info(&pdev->dev, "%s register hcp id %d with invalid arguments\n",
								__func__, id);

	return -EINVAL;
}
EXPORT_SYMBOL(mtk_hcp_register);

int mtk_hcp_unregister(struct platform_device *pdev, enum hcp_id id)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	unsigned int idx = (unsigned int)id;

	if (hcp_dev == NULL) {
		dev_info(&pdev->dev, "%s hcp device in not ready\n", __func__);
		return -EPROBE_DEFER;
	}

	if (idx < HCP_MAX_ID) {
		memset((void *)&hcp_dev->hcp_desc_table[idx], 0,
						sizeof(struct hcp_desc));
		return 0;
	}

	dev_info(&pdev->dev, "%s register hcp id %u with invalid arguments\n",
		__func__, idx);

	return -EINVAL;
}
EXPORT_SYMBOL(mtk_hcp_unregister);

static int hcp_send_internal(struct mtk_hcp *hcp_dev,
		 enum hcp_id id, void *buf,
		 unsigned int len, int req_fd,
		 unsigned int wait)
{
	struct share_buf send_obj = {0};
	unsigned long timeout = 0;
	unsigned long flag = 0;
	struct msg *msg = NULL;
	int ret = 0;
	unsigned int no = 0;

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "%s id:%d len %d\n",
		__func__, id, len);

	if (id >= HCP_MAX_ID || len > sizeof(send_obj.share_data) || buf == NULL) {
		dev_info(hcp_dev->dev,
			"%s failed to send hcp message (Invalid arg.), len/sz(%d/%lu)\n",
			__func__, len, sizeof(send_obj.share_data));
		return -EINVAL;
	}

	if (mtk_hcp_running(hcp_dev) == false) {
		dev_info(hcp_dev->dev, "%s hcp is not running\n", __func__);
		return -EPERM;
	}

	if (mtk_hcp_cm4_support(hcp_dev, id) == true) {
	#if MTK_CM4_SUPPORT
		int ipi_id = hcp_id_to_ipi_id(hcp_dev, id);
        if (hcp_dbg_enable())
		dev_dbg(hcp_dev->dev, "%s cm4 is support !!!\n", __func__);
		return scp_ipi_send(ipi_id, buf, len, 0, SCP_A_ID);
	#endif
	} else {
		int module_id = hcp_id_to_module_id(hcp_dev, id);
		if (module_id < MODULE_ISP || module_id >= MODULE_MAX_ID) {
			dev_info(hcp_dev->dev, "%s invalid module id %d", __func__, module_id);
			return -EINVAL;
		}

		timeout = msecs_to_jiffies(HCP_TIMEOUT_MS);
		ret = wait_event_timeout(hcp_dev->msg_wq,
			((msg = msg_pool_get(hcp_dev)) != NULL), timeout);
		if (ret == 0) {
			dev_info(hcp_dev->dev, "%s id:%d refill time out !\n",
				__func__, id);
			return -EIO;
		} else if (-ERESTARTSYS == ret) {
			dev_info(hcp_dev->dev, "%s id:%d refill interrupted !\n",
				__func__, id);
			return -ERESTARTSYS;
		}

		if (msg == NULL) {
			dev_info(hcp_dev->dev, "%s id:%d msg poll is full!\n",
				__func__, id);
			return -EAGAIN;
		}

		atomic_set(&hcp_dev->hcp_id_ack[id], 0);

		//spin_lock_irqsave(&hcp_dev->msglock, flag);
		//msg = list_first_entry(&hcp_dev->msg_list, struct msg, entry);
		//list_del(&msg->entry);
		//spin_unlock_irqrestore(&hcp_dev->msglock, flag);

		memcpy((void *)msg->user_obj.share_data, buf, len);
		msg->user_obj.len = len;

		spin_lock_irqsave(&hcp_dev->msglock, flag);

		no = atomic_inc_return(&hcp_dev->seq);

		msg->user_obj.info.send.hcp = id;
		msg->user_obj.info.send.req = req_fd;
		msg->user_obj.info.send.seq = no;
		msg->user_obj.info.send.ack = (wait ? 1 : 0);
		msg->user_obj.id = id;

		list_add_tail(&msg->entry, &hcp_dev->chans[module_id]);
		spin_unlock_irqrestore(&hcp_dev->msglock, flag);

		if (id != HCP_IMGSYS_DEQUE_DONE_ID) {
            if (hcp_dbg_enable())
			dev_dbg(hcp_dev->dev, "wake up id(%d)\n",
				id);
		wake_up(&hcp_dev->poll_wq[module_id]);
		} else {
		    if (hcp_dbg_enable())
			dev_dbg(hcp_dev->dev, "no wake_up deque_done, id(%d)\n",
				id);
		}

        if (hcp_dbg_enable())
		dev_dbg(hcp_dev->dev,
			"%s frame_no_%d, message(%d)size(%d) send to user space !!!\n",
			__func__, no, id, len);

		if (id == HCP_IMGSYS_SW_TIMEOUT_ID)
			chans_pool_dump(hcp_dev);

		if (!wait)
			return 0;

		/* wait for RED's ACK */
		timeout = msecs_to_jiffies(HCP_TIMEOUT_MS);
		ret = wait_event_timeout(hcp_dev->ack_wq[module_id],
			atomic_cmpxchg(&(hcp_dev->hcp_id_ack[id]), 1, 0), timeout);
		if (ret == 0) {
			dev_info(hcp_dev->dev, "%s hcp id:%d ack time out !\n",
				__func__, id);
			/*
			* clear un-success event to prevent unexpected flow
			* cauesd be remaining data
			*/
			return -EIO;
		} else if (-ERESTARTSYS == ret) {
			dev_info(hcp_dev->dev, "%s hcp id:%d ack wait interrupted !\n",
				__func__, id);
			return -ERESTARTSYS;
		} else {
			return 0;
		}
	}
	return 0;
}

struct task_struct *mtk_hcp_get_current_task(struct platform_device *pdev)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	return hcp_dev->current_task;
}
EXPORT_SYMBOL(mtk_hcp_get_current_task);

int mtk_hcp_send(struct platform_device *pdev,
		 enum hcp_id id, void *buf,
		 unsigned int len, int req_fd)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	return hcp_send_internal(hcp_dev, id, buf, len, req_fd, SYNC_SEND);
}
EXPORT_SYMBOL(mtk_hcp_send);

int mtk_hcp_send_async(struct platform_device *pdev,
		 enum hcp_id id, void *buf,
		 unsigned int len, int req_fd)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	return hcp_send_internal(hcp_dev, id, buf, len, req_fd, ASYNC_SEND);
}
EXPORT_SYMBOL(mtk_hcp_send_async);

int mtk_hcp_set_apu_dc(struct platform_device *pdev,
	int32_t value, size_t size)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	struct slbc_data slb = {0};
	struct ctrl_data ctrl = {0};
	int ret = 0;

	if (value) {
		if (atomic_inc_return(&(hcp_dev->have_slb)) == 1) {
			slb.uid = UID_SH_P2;
			slb.type = TP_BUFFER;
			ret = slbc_request(&slb);
			if (ret < 0) {
				dev_info(hcp_dev->dev, "%s: Failed to allocate SLB buffer",
					__func__);
				return -1;
			}

			dev_info(hcp_dev->dev, "%s: SLB buffer base(0x%lx), size(%ld)",
				__func__, (uintptr_t)slb.paddr, slb.size);

			ctrl.id    = CTRL_ID_SLB_BASE;
			ctrl.value = ((slb.size << 32) |
				((uintptr_t)slb.paddr & 0x0FFFFFFFFULL));

			return hcp_send_internal(hcp_dev,
				HCP_IMGSYS_SET_CONTROL_ID, &ctrl, sizeof(ctrl), 0, 0);
		}
	} else {
		if (atomic_dec_return(&(hcp_dev->have_slb)) == 0) {
			slb.uid  = UID_SH_P2;
			slb.type = TP_BUFFER;
			ret = slbc_release(&slb);
			if (ret < 0) {
				dev_info(hcp_dev->dev, "Failed to release SLB buffer");
				return -1;
			}

			ctrl.id    = CTRL_ID_SLB_BASE;
			ctrl.value = 0;

			return hcp_send_internal(hcp_dev,
				HCP_IMGSYS_SET_CONTROL_ID, &ctrl, sizeof(ctrl), 0, 0);
		}
	}
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_hcp_set_apu_dc);

struct platform_device *mtk_hcp_get_plat_device(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *hcp_node = NULL;
	struct platform_device *hcp_pdev = NULL;

    if (hcp_dbg_enable())
	dev_dbg(&pdev->dev, "- E. hcp get platform device.\n");

	hcp_node = of_parse_phandle(dev->of_node, "mediatek,hcp", 0);
	if (hcp_node == NULL) {
		dev_info(&pdev->dev, "%s can't get hcp node.\n", __func__);
		return NULL;
	}

	hcp_pdev = of_find_device_by_node(hcp_node);
	if (WARN_ON(hcp_pdev == NULL) == true) {
		dev_info(&pdev->dev, "%s hcp pdev failed.\n", __func__);
		of_node_put(hcp_node);
		return NULL;
	}

	return hcp_pdev;
}
EXPORT_SYMBOL(mtk_hcp_get_plat_device);

#if HCP_RESERVED_MEM
phys_addr_t mtk_hcp_get_reserve_mem_phys(unsigned int id)
{
	return 0;
}
EXPORT_SYMBOL(mtk_hcp_get_reserve_mem_phys);

void mtk_hcp_set_reserve_mem_virt(unsigned int id,
	void *virmem)
{
	return;
}
EXPORT_SYMBOL(mtk_hcp_set_reserve_mem_virt);

void *mtk_hcp_get_reserve_mem_virt(unsigned int id)
{
	return NULL;
}
EXPORT_SYMBOL(mtk_hcp_get_reserve_mem_virt);

phys_addr_t mtk_hcp_get_reserve_mem_dma(unsigned int id)
{
	return 0;
}
EXPORT_SYMBOL(mtk_hcp_get_reserve_mem_dma);

phys_addr_t mtk_hcp_get_reserve_mem_size(unsigned int id)
{
	return 0;
}
EXPORT_SYMBOL(mtk_hcp_get_reserve_mem_size);

void mtk_hcp_set_reserve_mem_fd(unsigned int id, uint32_t fd)
{
	return;
}
EXPORT_SYMBOL(mtk_hcp_set_reserve_mem_fd);

uint32_t mtk_hcp_get_reserve_mem_fd(unsigned int id)
{
	return 0;
}
EXPORT_SYMBOL(mtk_hcp_get_reserve_mem_fd);
#if SMVR_DECOUPLE
void *mtk_hcp_get_gce_mem_virt(struct platform_device *pdev, unsigned int mode)
#else
void *mtk_hcp_get_gce_mem_virt(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	void *buffer = NULL;

	if (!hcp_dev->data->get_gce_virt) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return NULL;
	}
#if SMVR_DECOUPLE
	buffer = hcp_dev->data->get_gce_virt(mode);
#else
	buffer = hcp_dev->data->get_gce_virt();
#endif
	if (!buffer)
		dev_info(&pdev->dev, "%s: gce buffer is null\n", __func__);

	return buffer;
}
EXPORT_SYMBOL(mtk_hcp_get_gce_mem_virt);

#if SMVR_DECOUPLE
void *mtk_hcp_get_gce_token_mem_virt(struct platform_device *pdev, unsigned int mode)
{
    struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
    void *buffer = NULL;
    if (!hcp_dev->data->get_gce_token_virt) {
        dev_info(&pdev->dev, "%s: not supported\n", __func__);
        return NULL;
    }
    buffer = hcp_dev->data->get_gce_token_virt(mode);
    if (!buffer)
        dev_info(&pdev->dev, "%s: token buffer is null\n", __func__);

        return buffer;
}
EXPORT_SYMBOL(mtk_hcp_get_gce_token_mem_virt);

void*mtk_hcp_get_wpe_mem_virt(struct platform_device *pdev, unsigned int mode)
#else
void*mtk_hcp_get_wpe_mem_virt(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	void *buffer = NULL;

	if (!hcp_dev->data->get_wpe_virt) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return NULL;
	}
#if SMVR_DECOUPLE
    buffer = hcp_dev->data->get_wpe_virt(mode);
#else
	buffer = hcp_dev->data->get_wpe_virt();
#endif
	if (!buffer)
		dev_info(&pdev->dev, "%s: wpe cq buffer is null\n", __func__);

	return buffer;
}
EXPORT_SYMBOL(mtk_hcp_get_wpe_mem_virt);

#if SMVR_DECOUPLE
int mtk_hcp_get_wpe_mem_cq_fd(struct platform_device *pdev, unsigned int mode)
#else
int mtk_hcp_get_wpe_mem_cq_fd(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	int fd = -1;

	if (!hcp_dev->data->get_wpe_cq_fd) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return -1;
	}
#if SMVR_DECOUPLE
	fd = hcp_dev->data->get_wpe_cq_fd(mode);
#else
	fd = hcp_dev->data->get_wpe_cq_fd();
#endif
	if (fd < 0)
		dev_info(&pdev->dev, "%s: wpe cq fd is wrong\n", __func__);

	return fd;
}
EXPORT_SYMBOL(mtk_hcp_get_wpe_mem_cq_fd);

#if SMVR_DECOUPLE
int mtk_hcp_get_wpe_mem_tdr_fd(struct platform_device *pdev, unsigned int mode)
#else
int mtk_hcp_get_wpe_mem_tdr_fd(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	int fd = -1;

	if (!hcp_dev->data->get_wpe_tdr_fd) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return -1;
	}
#if SMVR_DECOUPLE
	fd = hcp_dev->data->get_wpe_tdr_fd(mode);
#else
	fd = hcp_dev->data->get_wpe_tdr_fd();
#endif
	if (fd < 0)
		dev_info(&pdev->dev, "%s: wpe tdr fd is wrong\n", __func__);

	return fd;
}
EXPORT_SYMBOL(mtk_hcp_get_wpe_mem_tdr_fd);

#if SMVR_DECOUPLE
void*mtk_hcp_get_dip_mem_virt(struct platform_device *pdev, unsigned int mode)
#else
void *mtk_hcp_get_dip_mem_virt(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	void *buffer = NULL;

	if (!hcp_dev->data->get_dip_virt) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return NULL;
	}
#if SMVR_DECOUPLE
	buffer = hcp_dev->data->get_dip_virt(mode);
#else
	buffer = hcp_dev->data->get_dip_virt();
#endif
	if (!buffer)
		dev_info(&pdev->dev, "%s: dip cq buffer is null\n", __func__);

	return buffer;
}
EXPORT_SYMBOL(mtk_hcp_get_dip_mem_virt);

#if SMVR_DECOUPLE
int mtk_hcp_get_dip_mem_cq_fd(struct platform_device *pdev, unsigned int mode)
#else
int mtk_hcp_get_dip_mem_cq_fd(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	int fd = -1;

	if (!hcp_dev->data->get_dip_cq_fd) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return -1;
	}
#if SMVR_DECOUPLE
	fd = hcp_dev->data->get_dip_cq_fd(mode);
#else
	fd = hcp_dev->data->get_dip_cq_fd();
#endif
	if (fd < 0)
		dev_info(&pdev->dev, "%s: dip cq fd is wrong\n", __func__);

	return fd;
}
EXPORT_SYMBOL(mtk_hcp_get_dip_mem_cq_fd);

#if SMVR_DECOUPLE
int mtk_hcp_get_dip_mem_tdr_fd(struct platform_device *pdev, unsigned int mode)
#else
int mtk_hcp_get_dip_mem_tdr_fd(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	int fd = -1;

	if (!hcp_dev->data->get_dip_tdr_fd) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return -1;
	}
#if SMVR_DECOUPLE
	fd = hcp_dev->data->get_dip_tdr_fd(mode);
#else
	fd = hcp_dev->data->get_dip_tdr_fd();
#endif
	if (fd < 0)
		dev_info(&pdev->dev, "%s: dip tdr fd is wrong\n", __func__);

	return fd;
}
EXPORT_SYMBOL(mtk_hcp_get_dip_mem_tdr_fd);

#if SMVR_DECOUPLE
void*mtk_hcp_get_traw_mem_virt(struct platform_device *pdev, unsigned int mode)
#else
void *mtk_hcp_get_traw_mem_virt(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	void *buffer = NULL;

	if (!hcp_dev->data->get_traw_virt) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return NULL;
	}
#if SMVR_DECOUPLE
	buffer = hcp_dev->data->get_traw_virt(mode);
#else
	buffer = hcp_dev->data->get_traw_virt();
#endif
	if (!buffer)
		dev_info(&pdev->dev, "%s: traw cq buffer is null\n", __func__);

	return buffer;
}
EXPORT_SYMBOL(mtk_hcp_get_traw_mem_virt);

#if SMVR_DECOUPLE
int mtk_hcp_get_traw_mem_cq_fd(struct platform_device *pdev, unsigned int mode)
#else
int mtk_hcp_get_traw_mem_cq_fd(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	int fd = -1;

	if (!hcp_dev->data->get_traw_cq_fd) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return -1;
	}
#if SMVR_DECOUPLE
	fd = hcp_dev->data->get_traw_cq_fd(mode);
#else
	fd = hcp_dev->data->get_traw_cq_fd();
#endif
	if (fd < 0)
		dev_info(&pdev->dev, "%s: traw cq fd is wrong\n", __func__);

	return fd;
}
EXPORT_SYMBOL(mtk_hcp_get_traw_mem_cq_fd);

#if SMVR_DECOUPLE
int mtk_hcp_get_traw_mem_tdr_fd(struct platform_device *pdev, unsigned int mode)
#else
int mtk_hcp_get_traw_mem_tdr_fd(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	int fd = -1;

	if (!hcp_dev->data->get_traw_tdr_fd) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return -1;
	}
#if SMVR_DECOUPLE
	fd = hcp_dev->data->get_traw_tdr_fd(mode);
#else
	fd = hcp_dev->data->get_traw_tdr_fd();
#endif
	if (fd < 0)
		dev_info(&pdev->dev, "%s: traw tdr fd is wrong\n", __func__);

	return fd;
}
EXPORT_SYMBOL(mtk_hcp_get_traw_mem_tdr_fd);

#if SMVR_DECOUPLE
void*mtk_hcp_get_pqdip_mem_virt(struct platform_device *pdev, unsigned int mode)
#else
void *mtk_hcp_get_pqdip_mem_virt(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	void *buffer = NULL;

	if (!hcp_dev->data->get_pqdip_virt) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return NULL;
	}
#if SMVR_DECOUPLE
	buffer = hcp_dev->data->get_pqdip_virt(mode);
#else
	buffer = hcp_dev->data->get_pqdip_virt();
#endif
	if (!buffer)
		dev_info(&pdev->dev, "%s: pqdip cq buffer is null\n", __func__);

	return buffer;
}
EXPORT_SYMBOL(mtk_hcp_get_pqdip_mem_virt);

#if SMVR_DECOUPLE
int mtk_hcp_get_pqdip_mem_cq_fd(struct platform_device *pdev, unsigned int mode)
#else
int mtk_hcp_get_pqdip_mem_cq_fd(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	int fd = -1;

	if (!hcp_dev->data->get_pqdip_cq_fd) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return -1;
	}

#if SMVR_DECOUPLE
	fd = hcp_dev->data->get_pqdip_cq_fd(mode);
#else
	fd = hcp_dev->data->get_pqdip_cq_fd();
#endif
	if (fd < 0)
		dev_info(&pdev->dev, "%s: pqdip cq fd is wrong\n", __func__);

	return fd;
}
EXPORT_SYMBOL(mtk_hcp_get_pqdip_mem_cq_fd);

#if SMVR_DECOUPLE
int mtk_hcp_get_pqdip_mem_tdr_fd(struct platform_device *pdev, unsigned int mode)
#else
int mtk_hcp_get_pqdip_mem_tdr_fd(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	int fd = -1;

	if (!hcp_dev->data->get_pqdip_tdr_fd) {
		dev_info(&pdev->dev, "%s: not supported\n", __func__);
		return -1;
	}

#if SMVR_DECOUPLE
	fd = hcp_dev->data->get_pqdip_tdr_fd(mode);
#else
	fd = hcp_dev->data->get_pqdip_tdr_fd();
#endif
	if (fd < 0)
		dev_info(&pdev->dev, "%s: pqdip tdr fd is wrong\n", __func__);

	return fd;
}
EXPORT_SYMBOL(mtk_hcp_get_pqdip_mem_tdr_fd);

#if SMVR_DECOUPLE
int mtk_hcp_get_gce_buffer(struct platform_device *pdev, unsigned int mode)
#else
int mtk_hcp_get_gce_buffer(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	if (!hcp_dev->data->get_gce) {
		dev_info(&pdev->dev, "%s:not supported\n", __func__);
		return -1;
	}
#if SMVR_DECOUPLE
	return hcp_dev->data->get_gce(mode);
#else
	return hcp_dev->data->get_gce();
#endif
}
EXPORT_SYMBOL(mtk_hcp_get_gce_buffer);

#if SMVR_DECOUPLE
int mtk_hcp_put_gce_buffer(struct platform_device *pdev, unsigned int mode)
#else
int mtk_hcp_put_gce_buffer(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	if (!hcp_dev->data->put_gce) {
		dev_info(&pdev->dev, "%s:not supported\n", __func__);
		return -1;
	}
#if SMVR_DECOUPLE
	return hcp_dev->data->put_gce(mode);
#else
	return hcp_dev->data->put_gce();
#endif
}
EXPORT_SYMBOL(mtk_hcp_put_gce_buffer);

#if SMVR_DECOUPLE
void *mtk_hcp_get_hwid_mem_virt(struct platform_device *pdev, unsigned int mode)
#else
void *mtk_hcp_get_hwid_mem_virt(struct platform_device *pdev)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	if (!hcp_dev->data->get_hwid_virt) {
		dev_info(&pdev->dev, "%s:not supported\n", __func__);
		return NULL;
	}
#if SMVR_DECOUPLE
	return hcp_dev->data->get_hwid_virt(mode);
#else
	return hcp_dev->data->get_hwid_virt();
#endif
}
EXPORT_SYMBOL(mtk_hcp_get_hwid_mem_virt);

#endif

static int mtk_hcp_open(struct inode *inode, struct file *file)
{
	struct mtk_hcp *hcp_dev = NULL;

	hcp_dev = container_of(inode->i_cdev, struct mtk_hcp, hcp_cdev);
    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "open inode->i_cdev = 0x%p\n", inode->i_cdev);

	/*  */
	file->private_data = hcp_dev;

	hcp_dev->is_open = true;
	cm4_support_table_init(hcp_dev);

	hcp_dev->current_task = current;

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "- X. hcp open.\n");

	return 0;
}

static unsigned int mtk_hcp_poll(struct file *file, poll_table *wait)
{
	struct mtk_hcp *hcp_dev = (struct mtk_hcp *)file->private_data;

	// dev_info(hcp_dev->dev, "%s: poll start+", __func__);
	if (chan_pool_available(hcp_dev, MODULE_IMG)) {
		// dev_info(hcp_dev->dev, "%s: poll start-: %d", __func__, POLLIN);
		return POLLIN;
	}

	poll_wait(file, &hcp_dev->poll_wq[MODULE_IMG], wait);
	if (chan_pool_available(hcp_dev, MODULE_IMG)) {
		// dev_info(hcp_dev->dev, "%s: poll start-: %d", __func__, POLLIN);
		return POLLIN;
	}

	// dev_info(hcp_dev->dev, "%s: poll start-: 0", __func__);
	return 0;
}

static int mtk_hcp_release(struct inode *inode, struct file *file)
{
	struct mtk_hcp *hcp_dev = (struct mtk_hcp *)file->private_data;
#ifndef CONFIG_FPGA_EARLY_PORTING
	struct slbc_data slb = {0};
	int ret = 0;
#endif

    if (hcp_dbg_enable())
	dev_dbg(hcp_dev->dev, "- E. hcp release.\n");

	hcp_dev->is_open = false;
#ifndef CONFIG_FPGA_EARLY_PORTING
	if (atomic_read(&(hcp_dev->have_slb)) > 0) {
		slb.uid  = UID_SH_P2;
		slb.type = TP_BUFFER;
		ret = slbc_release(&slb);
		if (ret < 0) {
			dev_info(hcp_dev->dev, "Failed to release SLB buffer");
			return -1;
		}
	}
#endif

	kfree(hcp_dev->extmem.d_va);
	return 0;
}

static int mtk_hcp_mmap(struct file *file, struct vm_area_struct *vma)
{
	return -EOPNOTSUPP;
}

static void module_notify(struct mtk_hcp *hcp_dev,
					struct share_buf *user_data_addr)
{
	if (!user_data_addr) {
		dev_info(hcp_dev->dev, "%s invalid null share buffer", __func__);
		return;
	}

	if (user_data_addr->id >= HCP_MAX_ID) {
		dev_info(hcp_dev->dev, "%s invalid hcp id %d", __func__, user_data_addr->id);
		return;
	}

	if (hcp_dbg_enable())
		dev_dbg(hcp_dev->dev, " %s with message id:%d\n",
			__func__, user_data_addr->id);

	if (hcp_dev->hcp_desc_table[user_data_addr->id].handler) {
		hcp_dev->hcp_desc_table[user_data_addr->id].handler(
			user_data_addr->share_data,
			user_data_addr->len,
			hcp_dev->hcp_desc_table[user_data_addr->id].priv);
	}
}

static void module_wake_up(struct mtk_hcp *hcp_dev,
					struct share_buf *user_data_addr)
{
	int module_id = 0;

	if (!user_data_addr) {
		dev_info(hcp_dev->dev, "%s invalid null share buffer", __func__);
		return;
	}

	if (user_data_addr->id >= HCP_MAX_ID) {
		dev_info(hcp_dev->dev, "%s invalid hcp id %d", __func__, user_data_addr->id);
		return;
	}

	module_id = hcp_id_to_module_id(hcp_dev, user_data_addr->id);
	if (module_id < MODULE_ISP || module_id >= MODULE_MAX_ID) {
		dev_info(hcp_dev->dev, "%s invalid module id %d", __func__, module_id);
		return;
	}

	atomic_set(&hcp_dev->hcp_id_ack[user_data_addr->id], 1);
	wake_up(&hcp_dev->ack_wq[module_id]);
}

static long mtk_hcp_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	long ret = -1;
	//void *mem_priv;
	struct mtk_hcp *hcp_dev = (struct mtk_hcp *)file->private_data;
	struct share_buf buffer = {0};
	struct packet data = {0};
	unsigned int index = 0;
	struct msg *msg = NULL;
	unsigned long flag = 0;

	switch (cmd) {
	case HCP_GET_OBJECT:
		(void)copy_from_user(&data, (void *)arg, sizeof(struct packet));
		if (data.count > IPI_MAX_BUFFER_COUNT || data.count < 0) {
			dev_info(hcp_dev->dev, "Get_OBJ # of buf:%u in cmd:%d exceed %u",
				data.count, cmd, IPI_MAX_BUFFER_COUNT);
			return -EINVAL;
		}
		for (index = 0; index < IPI_MAX_BUFFER_COUNT; index++) {
			if (index >= data.count)
				break;

			if (data.buffer[index] == NULL) {
				dev_info(hcp_dev->dev, "Get_OBJ buf[%u] is NULL", index);
				return -EINVAL;
			}
			(void)copy_from_user((void *)&buffer, (void *)data.buffer[index],
				sizeof(struct share_buf));
			if (buffer.info.cmd == HCP_COMPLETE) {
				module_notify(hcp_dev, &buffer);
				module_wake_up(hcp_dev, &buffer);
			} else if (buffer.info.cmd == HCP_NOTIFY) {
				module_notify(hcp_dev, &buffer);
			} else {
				pr_info("[HCP] Unknown commands 0x%p, %d", data.buffer[index],
					buffer.info.cmd);
				return ret;
			}
		}

		index = 0;
		while (chan_pool_available(hcp_dev, MODULE_IMG)) {
			if (index >= IPI_MAX_BUFFER_COUNT)
				break;

			msg = chan_pool_get(hcp_dev, MODULE_IMG);
			if (msg != NULL) {
				ret = copy_to_user((void *)data.buffer[index++], &msg->user_obj,
					(unsigned long)sizeof(struct share_buf));

				 //dev_info(hcp_dev->dev, "copy to user, index(%d)",
				 //	index);

				spin_lock_irqsave(&hcp_dev->msglock, flag);
				list_add_tail(&msg->entry, &hcp_dev->msg_list);
				spin_unlock_irqrestore(&hcp_dev->msglock, flag);
				wake_up(&hcp_dev->msg_wq);
			} else {
				dev_info(hcp_dev->dev, "can't get msg from chan_pool");
			}
		}

		put_user(index, (int32_t *)(arg + offsetof(struct packet, count)));
		ret = chan_pool_available(hcp_dev, MODULE_IMG);
		put_user(ret, (bool *)(arg + offsetof(struct packet, more)));
		//ret = mtk_hcp_get_data(hcp_dev, arg);

		ret = 0;
		break;
	case HCP_COMPLETE:
		(void)copy_from_user(&buffer, (void *)arg, sizeof(struct share_buf));
		module_notify(hcp_dev, &buffer);
		module_wake_up(hcp_dev, &buffer);
		ret = 0;
		break;
	case HCP_NOTIFY:
		(void)copy_from_user(&buffer, (void *)arg, sizeof(struct share_buf));
		module_notify(hcp_dev, &buffer);
		ret = 0;
		break;
	case HCP_WAKEUP:
		//(void)copy_from_user(&buffer, (void*)arg, sizeof(struct share_buf));
		//module_wake_up(hcp_dev, &buffer);
		wake_up(&hcp_dev->poll_wq[MODULE_IMG]);
		ret = 0;
		break;
	case HCP_TIMEOUT:
		chans_pool_dump(hcp_dev);
		ret = 0;
		break;
	default:
		dev_info(hcp_dev->dev, "Invalid cmd_number 0x%x.\n", cmd);
		break;
	}

	return ret;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long mtk_hcp_compat_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	struct mtk_hcp *hcp_dev = (struct mtk_hcp *)file->private_data;
	long ret = -1;
	struct share_buf __user *share_data32 = NULL;

	switch (cmd) {
	case COMPAT_HCP_GET_OBJECT:
	case COMPAT_HCP_NOTIFY:
	case COMPAT_HCP_COMPLETE:
	case COMPAT_HCP_WAKEUP:
		share_data32 = compat_ptr((uint32_t)arg);
		ret = file->f_op->unlocked_ioctl(file,
				cmd, (unsigned long)share_data32);
		break;
	default:
		dev_info(hcp_dev->dev, "Invalid cmd_number 0x%x.\n", cmd);
		break;
	}
	return ret;
}
#endif

static const struct file_operations hcp_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = mtk_hcp_ioctl,
	.open           = mtk_hcp_open,
	.poll           = mtk_hcp_poll,
	.release        = mtk_hcp_release,
	.mmap           = mtk_hcp_mmap,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl   = mtk_hcp_compat_ioctl,
#endif
};

int allocate_working_buffer_helper(struct platform_device *pdev)
{
	unsigned int id = 0;
#if SMVR_DECOUPLE
struct mtk_hcp_streaming_reserve_mblock *mblock = NULL;
#else
	struct mtk_hcp_reserve_mblock *mblock = NULL;
#endif
	unsigned int block_num = 0;
	struct sg_table *sgt = NULL;
	struct dma_buf_attachment *attach = NULL;
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	struct dma_heap *pdma_heap = NULL;
	struct iosys_map map = {0};
	int ret = 0;

	mblock = hcp_dev->data->mblock;
	block_num = hcp_dev->data->block_num;

	/* allocate reserved memory */
	for (id = 0; id < block_num; id++) {
		if (mblock[id].is_dma_buf) {
			switch (id) {
			default:

				/* all supported heap name you can find with cmd */
				/* (ls /dev/dma_heap/) in shell */
				pdma_heap = dma_heap_find("mtk_mm-uncached");
				if (!pdma_heap) {
					pr_info("pdma_heap find fail\n");
					return -1;
				}
				mblock[id].d_buf = dma_heap_buffer_alloc(
					pdma_heap,
					mblock[id].size,
					O_RDWR | O_CLOEXEC,
					DMA_HEAP_VALID_HEAP_FLAGS);
				if (IS_ERR(mblock[id].d_buf)) {
					pr_info("dma_heap_buffer_alloc fail :%ld\n",
					PTR_ERR(mblock[id].d_buf));
					return -1;
				}
				mtk_dma_buf_set_name(mblock[id].d_buf, mblock[id].name);
				mblock[id].attach =
					dma_buf_attach(mblock[id].d_buf, hcp_dev->dev);
				attach = mblock[id].attach;
				if (IS_ERR(attach)) {
					pr_info("dma_buf_attach fail :%ld\n",
					PTR_ERR(attach));
					return -1;
				}

				#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
				mblock[id].sgt =
					dma_buf_map_attachment_unlocked(attach, DMA_TO_DEVICE);
				#else
				mblock[id].sgt = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
				#endif
				sgt = mblock[id].sgt;
				if (IS_ERR(sgt)) {
					dma_buf_detach(mblock[id].d_buf, attach);
					pr_info("dma_buf_map_attachment fail sgt:%ld\n",
					PTR_ERR(sgt));
					return -1;
				}
				mblock[id].start_phys = sg_dma_address(sgt->sgl);
				mblock[id].start_dma = mblock[id].start_phys;
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
				ret = dma_buf_vmap_unlocked(mblock[id].d_buf, &map);
				#else
				ret = dma_buf_vmap(mblock[id].d_buf, &map);
				#endif
				if (ret) {
					pr_info("sg_dma_address fail\n");
					return ret;
				}
				mblock[id].start_virt = (void *)map.vaddr;
				get_dma_buf(mblock[id].d_buf);
				mblock[id].fd =
					dma_buf_fd(mblock[id].d_buf, O_RDWR | O_CLOEXEC);
				break;
			}
		} else {
			mblock[id].start_virt =
				kzalloc(mblock[id].size,
					GFP_KERNEL);
			mblock[id].start_phys =
				virt_to_phys(
					mblock[id].start_virt);
			mblock[id].start_dma = 0;
		}
	}
    #if SMVR_DECOUPLE
	hcp_dev->is_mem_alloc_streaming = 1;
    #else
    #endif
	return 0;
}
EXPORT_SYMBOL(allocate_working_buffer_helper);

int release_working_buffer_helper(struct platform_device *pdev)
{
	unsigned int id = 0;
#if SMVR_DECOUPLE
struct mtk_hcp_streaming_reserve_mblock *mblock = NULL;
#else
	struct mtk_hcp_reserve_mblock *mblock = NULL;
#endif
	unsigned int block_num = 0;
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	mblock = hcp_dev->data->mblock;
	block_num = hcp_dev->data->block_num;

	/* release reserved memory */
	for (id = 0; id < block_num; id++) {
		if (mblock[id].is_dma_buf) {
			switch (id) {
			default:
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
				/* free va */
				dma_buf_vunmap_unlocked(mblock[id].d_buf,
				mblock[id].start_virt);
				/* free iova */
				dma_buf_unmap_attachment_unlocked(mblock[id].attach,
				mblock[id].sgt, DMA_TO_DEVICE);
				#else
				/* free va */
				dma_buf_vunmap(mblock[id].d_buf,
				mblock[id].start_virt);
				/* free iova */
				dma_buf_unmap_attachment(mblock[id].attach,
				mblock[id].sgt, DMA_TO_DEVICE);
				#endif
				dma_buf_detach(mblock[id].d_buf,
				mblock[id].attach);
				dma_buf_put(mblock[id].d_buf);
				// close fd in user space driver, you can't close fd in kernel site
				// dma_heap_buffer_free(mblock[id].d_buf);
				//dma_buf_put(my_dma_buf);
				//also can use this api, but not recommended
				mblock[id].mem_priv = NULL;
				mblock[id].mmap_cnt = 0;
				mblock[id].start_dma = 0x0;
				mblock[id].start_virt = 0x0;
				mblock[id].start_phys = 0x0;
				mblock[id].d_buf = NULL;
				mblock[id].fd = -1;
				mblock[id].pIonHandle = NULL;
				mblock[id].attach = NULL;
				mblock[id].sgt = NULL;
				break;
			}
		} else {
			kfree(mblock[id].start_virt);
			mblock[id].start_virt = 0x0;
			mblock[id].start_phys = 0x0;
			mblock[id].start_dma = 0x0;
			mblock[id].mmap_cnt = 0;
		}
	}
    #if SMVR_DECOUPLE
	hcp_dev->is_mem_alloc_streaming = 0;
    #endif
	return 0;
}
EXPORT_SYMBOL(release_working_buffer_helper);

#if SMVR_DECOUPLE
int mtk_hcp_allocate_working_buffer(struct platform_device *pdev, unsigned int mode, unsigned int g_gmb_en)
#else
int mtk_hcp_allocate_working_buffer(struct platform_device *pdev, unsigned int mode)
#endif
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

#if SMVR_DECOUPLE
    int ret = 0;

	if ((hcp_dev == NULL)
		|| (hcp_dev->data == NULL)
		|| (hcp_dev->data->allocate == NULL)) {
		dev_info(&pdev->dev, "%s:allocate not supported\n", __func__);
		ret = allocate_working_buffer_helper(pdev);
	} else {
		if (mode == imgsys_streaming) {
			ret = hcp_dev->data->allocate(hcp_dev, imgsys_streaming, g_gmb_en);
			hcp_dev->is_mem_alloc_streaming = (ret == 0);
			hcp_dev->alloc_count++;
		} else if (mode == imgsys_capture) {
			ret = hcp_dev->data->allocate(hcp_dev, imgsys_capture, g_gmb_en);
			hcp_dev->is_mem_alloc_capture = (ret == 0);
			hcp_dev->alloc_count++;
		} else {
			ret = hcp_dev->data->allocate(hcp_dev, imgsys_smvr, g_gmb_en);
			hcp_dev->is_mem_alloc_smvr = (ret == 0);
			hcp_dev->alloc_count++;
		}
    }

	return ret;
#else
	if ((hcp_dev == NULL)
		|| (hcp_dev->data == NULL)
		|| (hcp_dev->data->allocate == NULL)) {
		dev_info(&pdev->dev, "%s:allocate not supported\n", __func__);
		return allocate_working_buffer_helper(pdev);
	}

	return hcp_dev->data->allocate(hcp_dev, mode);
#endif
}
EXPORT_SYMBOL(mtk_hcp_allocate_working_buffer);

#if SMVR_DECOUPLE
int mtk_hcp_release_gce_working_buffer(struct platform_device *pdev)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	if ((hcp_dev == NULL)
		|| (hcp_dev->data == NULL)
		|| (hcp_dev->data->release_gce_buf == NULL)) {
		dev_info(&pdev->dev, "%s:release gce not supported\n", __func__);
		return release_working_buffer_helper(pdev);
	}

	return hcp_dev->data->release_gce_buf(hcp_dev);
}
EXPORT_SYMBOL(mtk_hcp_release_gce_working_buffer);
#endif

int mtk_hcp_release_working_buffer(struct platform_device *pdev)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
#if SMVR_DECOUPLE
	int ret = 0, i = 0;

	if ((hcp_dev == NULL)
		|| (hcp_dev->data == NULL)
		|| (hcp_dev->data->release == NULL)) {
		dev_info(&pdev->dev, "%s:release not supported\n", __func__);
		ret = release_working_buffer_helper(pdev);
	} else {
		for (i = 0; i < hcp_dev->alloc_count; i++) {
			if (hcp_dev->is_mem_alloc_streaming) {
				ret = hcp_dev->data->release(hcp_dev, imgsys_streaming);
				hcp_dev->is_mem_alloc_streaming = 0;
			} else if (hcp_dev->is_mem_alloc_capture) {
				ret = hcp_dev->data->release(hcp_dev, imgsys_capture);
				hcp_dev->is_mem_alloc_capture = 0;
			} else {
				ret = hcp_dev->data->release(hcp_dev, imgsys_smvr);
				hcp_dev->is_mem_alloc_smvr = 0;
			}
		}
		hcp_dev->alloc_count = 0;
	}

	return ret;
#else
	if ((hcp_dev == NULL)
		|| (hcp_dev->data == NULL)
		|| (hcp_dev->data->allocate == NULL)) {
		dev_info(&pdev->dev, "%s:release not supported\n", __func__);
		return release_working_buffer_helper(pdev);
	}
	return hcp_dev->data->release(hcp_dev);
#endif
}
EXPORT_SYMBOL(mtk_hcp_release_working_buffer);

#if SMVR_DECOUPLE
int mtk_hcp_ioc_release_working_buffer(struct platform_device *pdev, unsigned int mode)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if ((hcp_dev == NULL)
		|| (hcp_dev->data == NULL)
		|| (hcp_dev->data->allocate == NULL)) {
		dev_info(&pdev->dev, "%s:release not supported\n", __func__);
		ret = release_working_buffer_helper(pdev);
	} else {
	    switch (mode) {
            case imgsys_streaming:
                ret = hcp_dev->data->release(hcp_dev, imgsys_streaming);
			    hcp_dev->is_mem_alloc_streaming = 0;
			    hcp_dev->alloc_count--;
                break;
            case imgsys_capture:
                ret = hcp_dev->data->release(hcp_dev, imgsys_capture);
			    hcp_dev->is_mem_alloc_capture = 0;
			    hcp_dev->alloc_count--;
                break;
            case imgsys_smvr:
                ret = hcp_dev->data->release(hcp_dev, imgsys_smvr);
			    hcp_dev->is_mem_alloc_smvr = 0;
			    hcp_dev->alloc_count--;
                break;
            default:
                dev_info(&pdev->dev,"not support mode\n");
                break;
        }
	}

	return ret;
}
EXPORT_SYMBOL(mtk_hcp_ioc_release_working_buffer);
#endif

int mtk_hcp_get_init_info(struct platform_device *pdev,
			struct img_init_info *info)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	if ((hcp_dev == NULL)
		|| (hcp_dev->data == NULL)
		|| (hcp_dev->data->get_init_info == NULL)
		|| (info == NULL)) {
		dev_info(&pdev->dev, "%s:not supported\n", __func__);
		return -1;
	}

	return hcp_dev->data->get_init_info(info);
}
EXPORT_SYMBOL(mtk_hcp_get_init_info);

#if SMVR_DECOUPLE
int mtk_hcp_get_mem_info(struct platform_device *pdev,
			struct img_init_info *info, unsigned int mode)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	if ((hcp_dev == NULL)
		|| (hcp_dev->data == NULL)
		|| (hcp_dev->data->get_mem_info == NULL)
		|| (info == NULL)) {
		dev_info(&pdev->dev, "%s:not supported\n", __func__);
		return -1;
	}

	return hcp_dev->data->get_mem_info(info);
}
EXPORT_SYMBOL(mtk_hcp_get_mem_info);
#endif

void mtk_hcp_purge_msg(struct platform_device *pdev)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	unsigned long flag = 0;
	int i = 0;
	struct msg *msg = NULL;
	struct msg *tmp = NULL;

	spin_lock_irqsave(&hcp_dev->msglock, flag);
	for (i = 0; i < MODULE_MAX_ID; i++) {
		list_for_each_entry_safe(msg, tmp,
					&hcp_dev->chans[i], entry){
			list_del(&msg->entry);
			list_add_tail(&msg->entry, &hcp_dev->msg_list);
		}
	}
	atomic_set(&hcp_dev->seq, 0);
	spin_unlock_irqrestore(&hcp_dev->msglock, flag);
}
EXPORT_SYMBOL(mtk_hcp_purge_msg);

int mtk_hcp_partial_flush(struct platform_device *pdev, struct flush_buf_info *b_info)
{
	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);

	if ((hcp_dev == NULL)
		|| (hcp_dev->data == NULL)
		|| (hcp_dev->data->partial_flush == NULL)
		|| (b_info == NULL)) {
		//dev_info(&pdev->dev, "%s:not supported\n", __func__);
		return -1;
	}

	return hcp_dev->data->partial_flush(hcp_dev, b_info);
}
EXPORT_SYMBOL(mtk_hcp_partial_flush);

static int mtk_hcp_probe(struct platform_device *pdev)
{
	struct mtk_hcp *hcp_dev = NULL;
	struct msg *msgs = NULL;
	int ret = 0;
	int i = 0;

	dev_info(&pdev->dev, "- E. hcp driver probe.\n");
	hcp_dev = devm_kzalloc(&pdev->dev, sizeof(*hcp_dev), GFP_KERNEL);
	if (hcp_dev == NULL)
		return -ENOMEM;

	hcp_mtkdev = hcp_dev;
	hcp_dev->dev = &pdev->dev;
	hcp_dev->mem_ops = &mtk_hcp_dma_contig_memops;

	hcp_dev->data = of_device_get_match_data(&pdev->dev);

	platform_set_drvdata(pdev, hcp_dev);
	dev_set_drvdata(&pdev->dev, hcp_dev);

	hcp_dev->smmu_dev = mtk_smmu_get_shared_device(&pdev->dev);
	if (!hcp_dev->smmu_dev) {
		dev_info(hcp_dev->dev,
			"%s: failed to get hcp smmu device\n",
			__func__);
		return -EINVAL;
	}

	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(34)))
		dev_info(&pdev->dev, "%s:No DMA available\n", __func__);

	if (!pdev->dev.dma_parms) {
		pdev->dev.dma_parms =
			devm_kzalloc(hcp_dev->dev, sizeof(*hcp_dev->dev->dma_parms), GFP_KERNEL);
	}
	if (hcp_dev->dev->dma_parms) {
		ret = dma_set_max_seg_size(hcp_dev->dev, (unsigned int)DMA_BIT_MASK(34));
		if (ret)
			dev_info(hcp_dev->dev, "Failed to set DMA segment size\n");
	}

	atomic_set(&(hcp_dev->have_slb), 0);
    #if SMVR_DECOUPLE
	hcp_dev->alloc_count = 0;
    #endif

	hcp_dev->is_open = false;
	for (i = 0; i < MODULE_MAX_ID; i++) {
		init_waitqueue_head(&hcp_dev->ack_wq[i]);
		init_waitqueue_head(&hcp_dev->poll_wq[i]);

		INIT_LIST_HEAD(&hcp_dev->chans[i]);

		switch (i) {
		case MODULE_ISP:
			hcp_dev->daemon_notify_wq[i] =
				alloc_ordered_workqueue("%s",
				__WQ_LEGACY | WQ_MEM_RECLAIM |
				WQ_FREEZABLE,
				"isp_daemon_notify_wq");
			break;
		case MODULE_IMG:
			hcp_dev->daemon_notify_wq[i] =
				alloc_ordered_workqueue("%s",
				__WQ_LEGACY | WQ_MEM_RECLAIM |
				WQ_FREEZABLE,
				"imgsys_daemon_notify_wq");
			break;
		case MODULE_FD:
			hcp_dev->daemon_notify_wq[i] =
				alloc_ordered_workqueue("%s",
				__WQ_LEGACY | WQ_MEM_RECLAIM |
				WQ_FREEZABLE,
				"fd_daemon_notify_wq");
			break;
		case MODULE_RSC:
			hcp_dev->daemon_notify_wq[i] =
				alloc_ordered_workqueue("%s",
				__WQ_LEGACY | WQ_MEM_RECLAIM |
				WQ_FREEZABLE,
				"rsc_daemon_notify_wq");
			break;
		default:
			hcp_dev->daemon_notify_wq[i] = NULL;
			break;
		}
	}
	spin_lock_init(&hcp_dev->msglock);
	init_waitqueue_head(&hcp_dev->msg_wq);
	INIT_LIST_HEAD(&hcp_dev->msg_list);
	msgs = devm_kzalloc(hcp_dev->dev, sizeof(*msgs) * MSG_NR, GFP_KERNEL);
	for (i = 0; i < MSG_NR; i++)
		list_add_tail(&msgs[i].entry, &hcp_dev->msg_list);

	/* init character device */

	ret = alloc_chrdev_region(&hcp_dev->hcp_devno, 0, 1, HCP_DEVNAME);
	if (ret < 0) {
		dev_info(&pdev->dev, "alloc_chrdev_region failed err= %d", ret);
		goto err_alloc;
	}

	cdev_init(&hcp_dev->hcp_cdev, &hcp_fops);
	hcp_dev->hcp_cdev.owner = THIS_MODULE;

	ret = cdev_add(&hcp_dev->hcp_cdev, hcp_dev->hcp_devno, 1);
	if (ret < 0) {
		dev_info(&pdev->dev, "cdev_add fail  err= %d", ret);
		goto err_add;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	hcp_dev->hcp_class = class_create("mtk_hcp_driver");
#else
	hcp_dev->hcp_class = class_create(THIS_MODULE, "mtk_hcp_driver");
#endif

	if (IS_ERR(hcp_dev->hcp_class) == true) {
		ret = (int)PTR_ERR(hcp_dev->hcp_class);
		dev_info(&pdev->dev, "class create fail  err= %d", ret);
		goto err_add;
	}

	hcp_dev->hcp_device = device_create(hcp_dev->hcp_class, NULL,
					hcp_dev->hcp_devno, NULL, HCP_DEVNAME);
	if (IS_ERR(hcp_dev->hcp_device) == true) {
		ret = (int)PTR_ERR(hcp_dev->hcp_device);
		dev_info(&pdev->dev, "device create fail  err= %d", ret);
		goto err_device;
	}

	hcp_aee_init(hcp_mtkdev);
    if (hcp_dbg_enable()) {
	dev_dbg(&pdev->dev, "hcp aee init done\n");
	dev_dbg(&pdev->dev, "- X. hcp driver probe success.\n");
    }

#if HCP_RESERVED_MEM
	/* allocate reserved memory */
	/* allocate shared memory about ipi_param at probe, allocate others at streamon */
		/* update size to be the same with dts */
	/* mtk_hcp_reserve_mblock[IMG_MEM_FOR_HW_ID].start_virt = */
	/* ioremap_wc(rmem_base_phys, rmem_size); */
	/* mtk_hcp_reserve_mblock[IMG_MEM_FOR_HW_ID].start_phys = */
	/* (phys_addr_t)rmem_base_phys; */
	/* mtk_hcp_reserve_mblock[IMG_MEM_FOR_HW_ID].start_dma = */
	/* (phys_addr_t)rmem_base_phys; */
	/* mtk_hcp_reserve_mblock[IMG_MEM_FOR_HW_ID].size = rmem_size; */
#endif
    dev_info(&pdev->dev, "- E. hcp driver probe done.\n");

	return 0;

err_device:
	class_destroy(hcp_dev->hcp_class);
err_add:
	cdev_del(&hcp_dev->hcp_cdev);
err_alloc:
	unregister_chrdev_region(hcp_dev->hcp_devno, 1);
	for (i = 0; i < MODULE_MAX_ID; i++) {
		if (hcp_dev->daemon_notify_wq[i]) {
			destroy_workqueue(hcp_dev->daemon_notify_wq[i]);
			hcp_dev->daemon_notify_wq[i] = NULL;
		}
	}

	devm_kfree(&pdev->dev, hcp_dev);

	dev_info(&pdev->dev, "- X. hcp driver probe fail.\n");

	return ret;
}

static const struct of_device_id mtk_hcp_match[] = {
	{.compatible = "mediatek,hcp", .data = (void *)&isp71_hcp_data},
	{.compatible = "mediatek,hcp7s", .data = (void *)&isp7s_hcp_data},
	{.compatible = "mediatek,hcp7sp", .data = (void *)&isp7sp_hcp_data},
	{}
};
MODULE_DEVICE_TABLE(of, mtk_hcp_match);

static int mtk_hcp_remove(struct platform_device *pdev)
{

	struct mtk_hcp *hcp_dev = platform_get_drvdata(pdev);
	int i = 0;

    if (hcp_dbg_enable())
	dev_dbg(&pdev->dev, "- E. hcp driver remove.\n");

	hcp_aee_deinit(hcp_dev);

	for (i = 0; i < MODULE_MAX_ID; i++) {
		if (hcp_dev->daemon_notify_wq[i]) {
			flush_workqueue(hcp_dev->daemon_notify_wq[i]);
			destroy_workqueue(hcp_dev->daemon_notify_wq[i]);
			hcp_dev->daemon_notify_wq[i] = NULL;
		}
	}

	if (hcp_dev->is_open == true) {
		hcp_dev->is_open = false;
        if (hcp_dbg_enable())
		dev_dbg(&pdev->dev, "%s: opened device found\n", __func__);
	}
	devm_kfree(&pdev->dev, hcp_dev);

	cdev_del(&hcp_dev->hcp_cdev);
	unregister_chrdev_region(hcp_dev->hcp_devno, 1);

    if (hcp_dbg_enable())
	dev_dbg(&pdev->dev, "- X. hcp driver remove.\n");
	return 0;
}

bool hcp_dbg_enable(void)
{
	return hcp_dbg_en;
}

static struct platform_driver mtk_hcp_driver = {
	.probe  = mtk_hcp_probe,
	.remove = mtk_hcp_remove,
	.driver = {
		.name = HCP_DEVNAME,
		.owner  = THIS_MODULE,
		.of_match_table = mtk_hcp_match,
	},
};

module_platform_driver(mtk_hcp_driver);

MODULE_IMPORT_NS(DMA_BUF);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Mediatek hetero control process driver");
