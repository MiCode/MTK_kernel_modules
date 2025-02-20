// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Frederic Chen <frederic.chen@mediatek.com>
 *         Holmes Chiou <holmes.chiou@mediatek.com>
 *
 */

#include <linux/hashtable.h>
#include <linux/device.h>
#include <linux/freezer.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sync_file.h>
#include <linux/vmalloc.h>
#include <media/v4l2-event.h>
#include "mtk_imgsys-dev.h"
#include "mtk_imgsys-hw.h"
#include "mtk-hcp.h"
#include "mtk_imgsys-sys.h"
#include "mtk_imgsys-cmdq.h"
#include "mtk_imgsys-module.h"
#include "mtk_imgsys-requesttrack.h"
#include "mtk_imgsys-trace.h"
#include "mtk-hcp_kernelfence.h"
#include "mtk_imgsys-engine.h"
#include "mtk_imgsys-v4l2-debug.h"


#if MTK_CM4_SUPPORT
#include <linux/remoteproc/mtk_scp.h>
#endif

static struct gce_timeout_work imgsys_timeout_winfo[VIDEO_MAX_FRAME];
static int imgsys_timeout_idx;
static struct info_list_t frm_info_list = {
	.mymutex = __MUTEX_INITIALIZER(frm_info_list.mymutex),
	.configed_list = LIST_HEAD_INIT(frm_info_list.configed_list),
	.fail_list = LIST_HEAD_INIT(frm_info_list.fail_list)
};
static struct reqfd_cbinfo_list_t reqfd_cbinfo_list = {
	.mymutex = __MUTEX_INITIALIZER(reqfd_cbinfo_list.mymutex),
	.mylist = LIST_HEAD_INIT(reqfd_cbinfo_list.mylist)
};
DECLARE_WAIT_QUEUE_HEAD(frm_info_waitq);

#define	MTK_IMGSYS_VIDEO_NODE_TUNING_OUT	(nodes_num - 5)

static inline bool info_list_is_empty(struct info_list_t *info_list)
{
	bool is_empty = true;

	mutex_lock(&(info_list->mymutex));
	if ((!list_empty(&info_list->configed_list)) ||
		(!list_empty(&info_list->fail_list)))
		is_empty = false;
	mutex_unlock(&(info_list->mymutex));
	return is_empty;
};
static void reqfd_cbinfo_put_work(struct reqfd_cbinfo_t *work);

static int imgsys_send(struct platform_device *pdev, enum hcp_id id,
		    void *buf, unsigned int  len, int req_fd, unsigned int wait)
{
	int ret = 0;
#if MTK_CM4_SUPPORT
	ret = scp_ipi_send(imgsys_dev->scp_pdev, SCP_IPI_DIP, &ipi_param,
			   sizeof(ipi_param), 0);
#else
	if (wait)
		ret = mtk_hcp_send(pdev, id, buf, len, req_fd);
	else
		ret = mtk_hcp_send_async(pdev, id, buf, len, req_fd);
#endif
	return ret;
}

static void imgsys_init_handler(void *data, unsigned int len, void *priv)
{
	pr_info("imgsys_init_handler");
}

int mtk_imgsys_hw_working_buf_pool_reinit(struct mtk_imgsys_dev *imgsys_dev)
{
	int i;
struct list_head *head = NULL;
	struct list_head *temp = NULL;
	struct reqfd_cbinfo_t *reqfdcb_info = NULL;

	spin_lock(&imgsys_dev->imgsys_usedbufferlist.lock);
	INIT_LIST_HEAD(&imgsys_dev->imgsys_usedbufferlist.list);
	imgsys_dev->imgsys_usedbufferlist.cnt = 0;
	spin_unlock(&imgsys_dev->imgsys_usedbufferlist.lock);

	spin_lock(&imgsys_dev->imgsys_freebufferlist.lock);
	INIT_LIST_HEAD(&imgsys_dev->imgsys_freebufferlist.list);
	imgsys_dev->imgsys_freebufferlist.cnt = 0;

	for (i = 0; i < DIP_SUB_FRM_DATA_NUM; i++) {
		struct mtk_imgsys_hw_subframe *buf =
						&imgsys_dev->working_buf[i];

		list_add_tail(&buf->list_entry,
			      &imgsys_dev->imgsys_freebufferlist.list);
		imgsys_dev->imgsys_freebufferlist.cnt++;
	}
	spin_unlock(&imgsys_dev->imgsys_freebufferlist.lock);
        mutex_lock(&(reqfd_cbinfo_list.mymutex));
	list_for_each_safe(head, temp, &(reqfd_cbinfo_list.mylist)) {
		reqfdcb_info = vlist_node_of(head, struct reqfd_cbinfo_t);
		reqfdcb_info->cur_cnt += 1;
		if (reqfdcb_info->cur_cnt == reqfdcb_info->exp_cnt) {
			list_del(head);
			reqfd_cbinfo_put_work(reqfdcb_info);
		}
	}
	INIT_LIST_HEAD(&(reqfd_cbinfo_list.mylist));
	mutex_unlock(&(reqfd_cbinfo_list.mymutex));

	return 0;
}

void mtk_imgsys_hw_working_buf_pool_release(struct mtk_imgsys_dev *imgsys_dev)
{
	/* All the buffer should be in the freebufferlist when release */
	if (imgsys_dev->imgsys_usedbufferlist.cnt)
		dev_info(imgsys_dev->dev,
			 "%s: imgsys_usedbufferlist is not empty (%d)\n",
			__func__, imgsys_dev->imgsys_usedbufferlist.cnt);

#if MTK_CM4_SUPPORT
	dma_unmap_page_attrs(imgsys_dev->dev,
			     imgsys_dev->working_buf_mem_isp_daddr,
			     imgsys_dev->working_buf_mem_size,
				DMA_BIDIRECTIONAL, DMA_ATTR_SKIP_CPU_SYNC);

	dma_free_coherent(&imgsys_dev->scp_pdev->dev,
			  imgsys_dev->working_buf_mem_size,
			  imgsys_dev->working_buf_mem_vaddr,
			  imgsys_dev->working_buf_mem_scp_daddr);
#else
	/* TODO: ontly for header_desc mode */
#ifdef USE_KERNEL_ION_BUFFER
	hcp_free_buffer(imgsys_dev->scp_pdev, IMG_MEM_FOR_HW_ID);
#endif
#endif
}

static void mtk_imgsys_hw_working_buf_free(struct mtk_imgsys_dev *imgsys_dev,
				struct mtk_imgsys_hw_subframe *working_buf,
					bool error_free)
{
	if (!working_buf)
		return;

	spin_lock(&imgsys_dev->imgsys_usedbufferlist.lock);
	list_del(&working_buf->list_entry);
			imgsys_dev->imgsys_usedbufferlist.cnt--;
    if (imgsys_dbg_enable()) {
	dev_dbg(imgsys_dev->dev,
		"%s: Free used buffer(%pad)\n",
		__func__, &working_buf->buffer.scp_daddr);
    }
	spin_unlock(&imgsys_dev->imgsys_usedbufferlist.lock);

	spin_lock(&imgsys_dev->imgsys_freebufferlist.lock);
	list_add_tail(&working_buf->list_entry,
		      &imgsys_dev->imgsys_freebufferlist.list);
	imgsys_dev->imgsys_freebufferlist.cnt++;
	spin_unlock(&imgsys_dev->imgsys_freebufferlist.lock);

}

static struct mtk_imgsys_hw_subframe*
mtk_imgsys_hw_working_buf_alloc(struct mtk_imgsys_dev *imgsys_dev)
{
	struct mtk_imgsys_hw_subframe *working_buf;

	spin_lock(&imgsys_dev->imgsys_freebufferlist.lock);
	if (list_empty(&imgsys_dev->imgsys_freebufferlist.list)) {
		spin_unlock(&imgsys_dev->imgsys_freebufferlist.lock);
		return NULL;
	}

	working_buf = list_first_entry(&imgsys_dev->imgsys_freebufferlist.list,
				       struct mtk_imgsys_hw_subframe,
					list_entry);
	list_del(&working_buf->list_entry);
	imgsys_dev->imgsys_freebufferlist.cnt--;
	spin_unlock(&imgsys_dev->imgsys_freebufferlist.lock);

	spin_lock(&imgsys_dev->imgsys_usedbufferlist.lock);
	list_add_tail(&working_buf->list_entry,
		      &imgsys_dev->imgsys_usedbufferlist.list);
	imgsys_dev->imgsys_usedbufferlist.cnt++;
	spin_unlock(&imgsys_dev->imgsys_usedbufferlist.lock);

	return working_buf;
}

static void mtk_imgsys_iova_map_tbl_unmap(struct mtk_imgsys_request *req)
{
	struct mtk_imgsys_dev *imgsys_dev = req->imgsys_pipe->imgsys_dev;
	struct mtk_imgsys_dev_buffer *dev_buf;
	struct mtk_imgsys_dma_buf_iova_get_info *dmabufiovainfo, *temp;
	int i = 0;

	for (i = 0; i < req->imgsys_pipe->desc->total_queues; i++) {
		dev_buf = req->buf_map[i];
		if (dev_buf) {
			list_for_each_entry_safe(dmabufiovainfo, temp,
				&dev_buf->iova_map_table.list, list_entry) {
				if (imgsys_dbg_enable()) {
				dev_dbg(imgsys_dev->dev,
					"%s:list put(deleted) ionFd(%d)-dma_addr:%pad\n",
					__func__,
					dmabufiovainfo->ionfd,
					&dmabufiovainfo->dma_addr);
				}
				//put dmabuf(iova)
				mtk_imgsys_put_dma_buf(dmabufiovainfo->dma_buf,
						dmabufiovainfo->attach,
						dmabufiovainfo->sgt);
				//free dmabuf
				spin_lock(&dev_buf->iova_map_table.lock);
				list_del(&dmabufiovainfo->list_entry);
				vfree(dmabufiovainfo);
				spin_unlock(&dev_buf->iova_map_table.lock);
			}
		}
	}
}

static void mtk_imgsys_iova_map_tbl_unmap_sd(struct mtk_imgsys_request *req)
{
	struct mtk_imgsys_dev *imgsys_dev = req->imgsys_pipe->imgsys_dev;
	struct mtk_imgsys_dev_buffer *dev_buf;
	struct mtk_imgsys_dma_buf_iova_get_info *dmabufiovainfo, *temp;
	int b;

	b = is_singledev_mode(req);
	dev_buf = req->buf_map[b];
	if (dev_buf) {
		list_for_each_entry_safe(dmabufiovainfo, temp,
			&dev_buf->iova_map_table.list, list_entry) {
			if (imgsys_dbg_enable()) {
			dev_dbg(imgsys_dev->dev,
				"%s:list put(deleted) ionFd(%d)-dma_addr:%pad\n",
				__func__,
				dmabufiovainfo->ionfd,
				&dmabufiovainfo->dma_addr);
			}
			//put dmabuf(iova)
			mtk_imgsys_put_dma_buf(dmabufiovainfo->dma_buf,
					dmabufiovainfo->attach,
					dmabufiovainfo->sgt);
			//free dmabuf
			spin_lock(&dev_buf->iova_map_table.lock);
			list_del(&dmabufiovainfo->list_entry);
			spin_unlock(&dev_buf->iova_map_table.lock);
			vfree(dmabufiovainfo);
		}
	}
}

#if MTK_MDP_SUPPORT /*CHRISTBD: open this after integrating flow with gce com */
#ifdef BATCH_MODE_V3
// V3 batch mode added {
static void mtk_imgsys_notify_batch(struct mtk_imgsys_request *req)
{
	struct mtk_imgsys_dev *imgsys_dev = req->imgsys_pipe->imgsys_dev;
	struct mtk_imgsys_hw_subframe *working_buf;

	if (!is_batch_mode(req)) {
		dev_info(imgsys_dev->dev, "%s:batch mode not batch mode\n",
			__func__);
		return;
	}

	if (list_empty(&req->mdp_done_list.list)) {
		dev_info(imgsys_dev->dev,
			"%s:mdp_done_list is empty:%d\n",
			__func__, req->mdp_done_list.cnt);
		return;
	}
	spin_lock(&req->mdp_done_list.lock);
	working_buf = list_first_entry(&req->mdp_done_list.list,
					   struct mtk_imgsys_hw_subframe,
					   list_entry);
	if (IS_ERR(working_buf)) {
		dev_info(imgsys_dev->dev,
			"%s:fail to find buffer in list\n", __func__);
		return;
	}

	/**
	 * Free the finished frame's working buffer,
	 * delete it from mdp_done_list in imgsys_request,
	 * and add it back to imgsys_usedbufferlist.
	 * free the working buffer then it'll be removed from that list.
	 */
	list_del(&working_buf->list_entry);
	req->mdp_done_list.cnt--;
	spin_unlock(&req->mdp_done_list.lock);

	spin_lock(&imgsys_dev->imgsys_usedbufferlist.lock);
	list_add_tail(&working_buf->list_entry,
			  &imgsys_dev->imgsys_usedbufferlist.list);
	imgsys_dev->imgsys_usedbufferlist.cnt++;
	spin_unlock(&imgsys_dev->imgsys_usedbufferlist.lock);

	mtk_imgsys_hw_working_buf_free(imgsys_dev, working_buf, false);

	/* Return if there are unprocessed frames*/
	if (--req->unprocessed_count != 0)
		return;

	/** Here means the complete of the request job,
	 *  so we can suspend the HW.
	 */
	pm_runtime_mark_last_busy(imgsys_dev->dev);
	pm_runtime_put_autosuspend(imgsys_dev->dev);

	/*
	 * The job may be aleady removed by streamoff, so we need to check
	 * it by id here.
	 */
	if (mtk_imgsys_pipe_get_running_job(req->imgsys_pipe, req->id)) {
		mtk_imgsys_pipe_remove_job(req);
		mtk_imgsys_pipe_job_finish(req, VB2_BUF_STATE_DONE);
		wake_up(&imgsys_dev->flushing_waitq);
	}
}
// } V3 batch mode added
#endif

static void mtk_imgsys_early_notify(struct mtk_imgsys_request *req,
						struct imgsys_event_status *ev)
{
	struct mtk_imgsys_dev *imgsys_dev = req->imgsys_pipe->imgsys_dev;
	struct mtk_imgsys_pipe *pipe = req->imgsys_pipe;
	struct req_frameparam *iparam = &req->img_fparam.frameparam;
	struct v4l2_event event;
	struct imgsys_event_status *status = (void *)event.u.data;
	u32 index = iparam->index;
	u32 frame_no = iparam->frame_no;

	memset(&event, 0, sizeof(event));

	IMGSYS_SYSTRACE_BEGIN("ReqFd:%d subfrm_idx:%d\n", req->tstate.req_fd,
							ev->frame_number);

	event.type = V4L2_EVENT_FRAME_SYNC;
	status->req_fd = ev->req_fd;
	status->frame_number = ev->frame_number;
	v4l2_event_queue(pipe->subdev.devnode, &event);

    if (imgsys_dbg_enable()) {
	dev_dbg(imgsys_dev->dev,
		"%s:%s: job id(%d), frame_no(%d), early_no(%d:%d), finished\n",
		__func__, pipe->desc->name, index, frame_no, ev->req_fd,
							ev->frame_number);
    }
	IMGSYS_SYSTRACE_END();

}

static void mtk_imgsys_notify(struct mtk_imgsys_request *req, uint64_t frm_owner)
{
	struct mtk_imgsys_dev *imgsys_dev = req->imgsys_pipe->imgsys_dev;
	struct mtk_imgsys_pipe *pipe = req->imgsys_pipe;
	struct req_frameparam *iparam = &req->img_fparam.frameparam;
	enum vb2_buffer_state vbf_state;
	u32 index = iparam->index;
	u32 frame_no = iparam->frame_no;
	u64 req_enq, req_done, imgenq;
    union request_track *req_track = NULL;
    req_track = (union request_track *)req->req_stat;

	IMGSYS_SYSTRACE_BEGIN("ReqFd:%d Own:%s\n", req->tstate.req_fd, ((char *)&frm_owner));
#ifdef REQ_TIMESTAMP
	req->tstate.time_notifyStart = ktime_get_boottime_ns()/1000;
#endif
	if (!pipe->streaming)
		goto notify;
    req_track->subflow_kernel++;
	if (is_singledev_mode(req))
		mtk_imgsys_iova_map_tbl_unmap_sd(req);
	else if (is_desc_mode(req))
		mtk_imgsys_iova_map_tbl_unmap(req);

notify:
	if (iparam->state != FRAME_STATE_HW_TIMEOUT)
		vbf_state = VB2_BUF_STATE_DONE;
	else
		vbf_state = VB2_BUF_STATE_ERROR;
	/*
	 * The job may be aleady removed by streamoff, so I need to check
	 * it by id here with mtk_imgsys_pipe_get_running_job()
	 */
	atomic_dec(&imgsys_dev->num_composing);

	mtk_imgsys_hw_working_buf_free(imgsys_dev, req->working_buf,
								false);
	req->working_buf = NULL;
	/*  vb2 buffer done in below function  */
    req_track->subflow_kernel++;
	if (vbf_state == VB2_BUF_STATE_DONE)
		mtk_imgsys_pipe_job_finish(req, vbf_state);
	mtk_imgsys_pipe_remove_job(req);
#ifdef REQ_TIMESTAMP
	req->tstate.time_unmapiovaEnd = ktime_get_boottime_ns()/1000;
#endif
    if (imgsys_dbg_enable()) {
	dev_dbg(imgsys_dev->dev,
			"[K]%s:%d:%s:%6lld %6lld %6lld %6lld %6lld %6lld %6lld %6lld %6lld %6lld %6lld %6lld %6lld %6lld %6lld %6lld %6lld\n",
			__func__, req->tstate.req_fd, ((char *)(&frm_owner)),
			(req->tstate.time_qreq-req->tstate.time_qbuf),
			(req->tstate.time_composingEnd-req->tstate.time_composingStart),
			(req->tstate.time_iovaworkp-req->tstate.time_composingEnd),
			(req->tstate.time_qw2composer-req->tstate.time_iovaworkp),
			(req->tstate.time_compfuncStart-req->tstate.time_qw2composer),
			(req->tstate.time_ipisendStart-req->tstate.time_compfuncStart),
			(req->tstate.time_reddonescpStart-req->tstate.time_ipisendStart),
			(req->tstate.time_qw2runner-req->tstate.time_reddonescpStart),
			(req->tstate.time_send2cmq-req->tstate.time_qw2runner),
			(req->tstate.time_sendtask),
			(req->tstate.time_mdpcbStart-req->tstate.time_send2cmq),
			(req->tstate.time_notifyStart-req->tstate.time_mdpcbStart),
			(req->tstate.time_unmapiovaEnd-req->tstate.time_notifyStart),
			(req->tstate.time_unmapiovaEnd-req->tstate.time_notify2vb2done),
			(req->tstate.time_notify2vb2done-req->tstate.time_composingStart),
			(req->tstate.time_notify2vb2done-req->tstate.time_reddonescpStart),
			(req->tstate.time_ipisendStart-req->tstate.time_composingStart)
			);
    }

	wake_up(&imgsys_dev->flushing_waitq);
    if (imgsys_dbg_enable()) {
	dev_dbg(imgsys_dev->dev,
		"%s:%s:(reqfd-%d) job id(%d), frame_no(%d) finished\n",
		__func__, pipe->desc->name, req->tstate.req_fd, index, frame_no);
    }

	IMGSYS_SYSTRACE_END();
	imgenq = req->tstate.time_qreq - req->tstate.time_qbuf;
	req_enq = req->tstate.time_send2cmq - req->tstate.time_reddonescpStart;
	req_done = req->tstate.time_notify2vb2done - req->tstate.time_reddonescpStart;
	IMGSYS_SYSTRACE_BEGIN("ReqFd:%d Own:%s imgenq:%lld tskdlr2gce:%lld tskhdlr2done:%lld\n",
		req->tstate.req_fd, ((char *)&frm_owner), imgenq, req_enq, req_done);
	IMGSYS_SYSTRACE_END();
	media_request_put(&req->req);
}

static void cmdq_cb_timeout_worker(struct work_struct *work)
{
	struct mtk_imgsys_pipe *pipe;
	struct mtk_imgsys_request *req = mtk_imgsys_hw_timeout_work_to_req(work);
	struct img_ipi_param ipi_param;
	struct swfrm_info_t *frm_info = NULL;
	struct gce_timeout_work *swork = NULL;
	struct img_sw_buffer swbuf_data = {0};

	swork = container_of(work, struct gce_timeout_work, work);
	pipe = (struct mtk_imgsys_pipe *)swork->pipe;
	if (!pipe->streaming) {
		pr_info("%s pipe already streamoff\n", __func__);
		goto release_req;
	}

	if (!req) {
		pr_info("%s NULL request Address\n", __func__);
		goto release_work;
	}

	frm_info = (struct swfrm_info_t *)(swork->req_sbuf_kva);

	if (frm_info) {
		frm_info->fail_uinfo_idx = swork->fail_uinfo_idx;
		frm_info->fail_isHWhang = swork->fail_isHWhang;
		frm_info->timeout_event = swork->hang_event;
		dev_info(req->imgsys_pipe->imgsys_dev->dev,
			"%s:%s:req fd/no(%d/%d)frame_no(%d) tfnum(%d) fail idx/sidx(%d/%d) timeout_w(%d)hang_event(%d)\n",
			__func__, (char *)(&(frm_info->frm_owner)), frm_info->request_fd,
			frm_info->request_no,
			frm_info->frame_no,
			frm_info->total_frmnum,
			frm_info->fail_uinfo_idx,
			frm_info->user_info[frm_info->fail_uinfo_idx].subfrm_idx,
			frm_info->fail_isHWhang, swork->hang_event);
		/* DAEMON debug dump */
		ipi_param.usage = IMG_IPI_DEBUG;
		swbuf_data.offset  = frm_info->req_sbuf_goft;
#if SMVR_DECOUPLE
if ((swork->is_capture) || (swork->is_time_shared)) {
            swbuf_data.scp_addr = imgsys_capture;
        } else {
            if (swork->batchnum > 0) {
                swbuf_data.scp_addr = imgsys_smvr;
            } else {
                swbuf_data.scp_addr = imgsys_streaming;
            }
        }
        dev_info(req->imgsys_pipe->imgsys_dev->dev,
            "%s: is_vss(%d)/is_capture(%d), batchnum(%d) scp_addr(%d)\n",
            __func__, swork->is_time_shared, swork->is_capture,
            swork->batchnum, swbuf_data.scp_addr);
#endif
		if (swork->fail_isHWhang) {
			imgsys_send(req->imgsys_pipe->imgsys_dev->scp_pdev,
					HCP_IMGSYS_HW_TIMEOUT_ID,
					&swbuf_data,
					sizeof(struct img_sw_buffer),
					frm_info->request_fd, 1);
		} else {
			imgsys_queue_timeout(&(req->imgsys_pipe->imgsys_dev->runnerque));

			imgsys_send(req->imgsys_pipe->imgsys_dev->scp_pdev,
					HCP_IMGSYS_SW_TIMEOUT_ID,
					&swbuf_data,
					sizeof(struct img_sw_buffer),
					frm_info->request_fd, 1);
		}
		wake_up_interruptible(&frm_info_waitq);
	}

release_req:
	pr_info("req track-%s:req(0x%lx) \n", __func__, (unsigned long)swork->req_sbuf_kva);
	frm_info = (struct swfrm_info_t *)(swork->req_sbuf_kva);
	pr_info("req track-%s:%s:req fd/no(%d/%d)frame_no(%d)\n",
			__func__, (char *)(&(frm_info->frm_owner)), frm_info->request_fd,
			frm_info->request_no,
			frm_info->frame_no);
	media_request_put(&req->req);

release_work:
#if SMVR_DECOUPLE
	if (swork->is_capture) {
	    mtk_hcp_put_gce_buffer(pipe->imgsys_dev->scp_pdev, imgsys_capture);
    } else {
        if (swork->batchnum) {
            mtk_hcp_put_gce_buffer(pipe->imgsys_dev->scp_pdev, imgsys_smvr);
        } else {
            mtk_hcp_put_gce_buffer(pipe->imgsys_dev->scp_pdev, imgsys_streaming);
        }
    }
#else
	mtk_hcp_put_gce_buffer(pipe->imgsys_dev->scp_pdev);
#endif

}

static void imgsys_cmdq_timeout_cb_func(struct cmdq_cb_data data,
						unsigned int fail_subfidx, bool isHWhang,
						unsigned int hang_event)
{
	struct mtk_imgsys_pipe *pipe;
	struct mtk_imgsys_request *req;
	struct mtk_imgsys_dev *imgsys_dev;
	struct swfrm_info_t *frm_info_cb;
	const struct module_ops *imgsys_modules;
	struct gce_timeout_work *swork = NULL;

	if (!data.data) {
		pr_info("%s: data->data is NULL\n",
		       __func__);
		return;
	}

	frm_info_cb = data.data;
	pipe = (struct mtk_imgsys_pipe *)frm_info_cb->pipe;
	req = (struct mtk_imgsys_request *)(frm_info_cb->req);
	if (!req) {
		pr_info("%s NULL request Address\n", __func__);
		return;
	}
	imgsys_dev = req->imgsys_pipe->imgsys_dev;
	#if SMVR_DECOUPLE
	 if (frm_info_cb->is_capture) {
	    mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev, imgsys_capture);
    } else {
        if (frm_info_cb->batchnum) {
            mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev, imgsys_smvr);
        } else {
            mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev, imgsys_streaming);
        }
    }
	#else
	mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev);
	#endif
	/*frm_info_cb->fail_uinfo_idx = fail_subfidx;*/
	dev_info(imgsys_dev->dev,
		"req track-%s:%s:req fd/no(%d/%d) frmNo(%d) tfnum(%d)sidx/fidx/hw(%d/%d_%d/0x%x)timeout(%d/%d)hang_event(%d) dump cb +",
		__func__, (char *)(&(frm_info_cb->frm_owner)), frm_info_cb->request_fd,
		frm_info_cb->request_no,
		frm_info_cb->frame_no,
		frm_info_cb->total_frmnum,
		frm_info_cb->swfrminfo_ridx,
		fail_subfidx,
		frm_info_cb->user_info[fail_subfidx].subfrm_idx,
		frm_info_cb->user_info[fail_subfidx].hw_comb, isHWhang,
		imgsys_timeout_idx, hang_event);

    if (!pipe->streaming) {
		pr_info("gce count-%s pipe already streamoff\n", __func__);
		goto release_req;
	}

	/* DUMP DL CHECKSUM & HW REGISTERS*/
	if (imgsys_dev->dump && isHWhang) {
		imgsys_modules = req->imgsys_pipe->imgsys_dev->modules;
		imgsys_dev->dump(imgsys_dev, imgsys_modules,
		req->imgsys_pipe->imgsys_dev->modules_num,
		frm_info_cb->user_info[fail_subfidx].hw_comb);
	}

release_req:
	/*swork = vzalloc(sizeof(struct gce_timeout_work));*/
	media_request_get(&req->req);
	swork = &(imgsys_timeout_winfo[imgsys_timeout_idx]);
	swork->req = req;
	swork->pipe = frm_info_cb->pipe;
	swork->fail_uinfo_idx = fail_subfidx;
	swork->fail_isHWhang = isHWhang;
	swork->hang_event = hang_event;
#if SMVR_DECOUPLE
	if (frm_info_cb->is_capture) {
		swork->req_sbuf_kva = frm_info_cb->req_sbuf_goft
				+ mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev, imgsys_capture);
	} else {
		if (frm_info_cb->batchnum) {
			swork->req_sbuf_kva = frm_info_cb->req_sbuf_goft
				+ mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev, imgsys_smvr);
		} else {
			swork->req_sbuf_kva = frm_info_cb->req_sbuf_goft
				+ mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev, imgsys_streaming);
		}
	}
	swork->is_time_shared = frm_info_cb->user_info[fail_subfidx].is_time_shared;
    swork->is_capture = frm_info_cb->is_capture;
    swork->batchnum = frm_info_cb->batchnum;
#else
	swork->req_sbuf_kva = frm_info_cb->req_sbuf_goft
			+ mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev);
#endif
	if ((data.err == -800)
		&& (frm_info_cb->user_info[fail_subfidx].hw_comb == 0x800)
		&& isHWhang) {
		cmdq_cb_timeout_worker(&swork->work);
	} else {
		//INIT_WORK(&swork->work, cmdq_cb_timeout_worker);
		if (!work_pending(&swork->work)) {
			queue_work(req->imgsys_pipe->imgsys_dev->mdpcb_wq,
				&swork->work);
		} else {
		    pr_info("imgsys_fw-cmdq work pending not handle");
            media_request_put(&req->req);
#if SMVR_DECOUPLE
		if (swork->is_capture) {
		    mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_capture);
    	} else {
    	    if (swork->batchnum) {
    	        mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_smvr);
    	    } else {
    	        mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_streaming);
    	    }
    	}
#else
		mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev);
#endif
        }
	}
	imgsys_timeout_idx = (imgsys_timeout_idx + 1) % VIDEO_MAX_FRAME;

	dev_info(imgsys_dev->dev,
		"req track-%s:%s:req fd/no(%d/%d) frmNo(%d) tfnum(%d)sidx/fidx/hw(%d/%d_%d/0x%x)timeout(%d/%d)hang_event(%d) dump cb -",
		__func__, (char *)(&(frm_info_cb->frm_owner)), frm_info_cb->request_fd,
		frm_info_cb->request_no,
		frm_info_cb->frame_no,
		frm_info_cb->total_frmnum,
		frm_info_cb->swfrminfo_ridx,
		fail_subfidx,
		frm_info_cb->user_info[fail_subfidx].subfrm_idx,
		frm_info_cb->user_info[fail_subfidx].hw_comb, isHWhang,
		imgsys_timeout_idx, hang_event);
}

static void cmdq_cb_done_worker(struct work_struct *work)
{
	struct mtk_imgsys_pipe *pipe;
	struct swfrm_info_t *gwfrm_info = NULL;
	struct gce_cb_work *gwork = NULL;
	struct img_sw_buffer swbuf_data;

	gwork = container_of(work, struct gce_cb_work, work);

	pipe = (struct mtk_imgsys_pipe *)gwork->pipe;
	if (!pipe->streaming) {
		pr_info("%s pipe already streamoff\n", __func__);
		goto release_work;
	}

	/* send to HCP after frame done & del node from list */
	gwfrm_info = (struct swfrm_info_t *)gwork->req_sbuf_kva;
	swbuf_data.offset = gwfrm_info->req_sbuf_goft;
#if SMVR_DECOUPLE
 if (gwfrm_info->is_capture || (gwfrm_info->user_info[0].is_time_shared)) {
        swbuf_data.scp_addr = imgsys_capture;
    } else {
        if (gwfrm_info->batchnum > 0) {
            swbuf_data.scp_addr = imgsys_smvr;
        } else {
            swbuf_data.scp_addr = imgsys_streaming;
        }
    }

    //pr_info("%s: is_vss(%d)/is_capture(%d), batchnum(%d) scp_addr(%d)\n", __func__,
    //    gwfrm_info->user_info[0].is_time_shared, gwfrm_info->is_capture,
    //    gwfrm_info->batchnum, swbuf_data.scp_addr);
#endif
	if (gwfrm_info->is_ndd)
		imgsys_send(pipe->imgsys_dev->scp_pdev, HCP_IMGSYS_DEQUE_DUMP_ID,
			&swbuf_data, sizeof(struct img_sw_buffer),
			gwork->reqfd, 0);
	else if (gwfrm_info->batchnum > 1 ||
		gwfrm_info->is_capture ||
		gwfrm_info->user_info[0].is_time_shared)
		imgsys_send(pipe->imgsys_dev->scp_pdev, HCP_IMGSYS_ASYNC_DEQUE_DONE_ID,
			&swbuf_data, sizeof(struct img_sw_buffer),
			gwork->reqfd, 0);
	else
	imgsys_send(pipe->imgsys_dev->scp_pdev, HCP_IMGSYS_DEQUE_DONE_ID,
		&swbuf_data, sizeof(struct img_sw_buffer),
		gwork->reqfd, 0);

	wake_up_interruptible(&frm_info_waitq);

release_work:
	return;
}

/* Maybe in IRQ context of cmdq */
static void imgsys_mdp_cb_func(struct cmdq_cb_data data,
					unsigned int subfidx, bool isLastTaskInReq,
					unsigned int batchnum, unsigned int is_capture)
{
	struct mtk_imgsys_pipe *pipe;
	struct mtk_imgsys_request *req;
	struct mtk_imgsys_dev *imgsys_dev;
	union request_track *req_track = NULL;
	//struct swfrm_info_t *frm_info_cb;
	struct swfrm_info_t *swfrminfo_cb;
	struct imgsys_event_status ev;
	struct gce_cb_work gwork;
	bool need_notify_daemon = false;
	bool lastfrmInMWReq = false;
	bool lastin_errcase = false;
	struct reqfd_cbinfo_t *reqfdcb_info = NULL;
	struct list_head *head = NULL;
	struct list_head *temp = NULL;
	bool can_notify_imgsys = false;
	bool reqfd_record_find = false;
	int exp_cnt = 0;
	int cur_cnt = 0;

	if (!data.data) {
		pr_info("%s: data->data is NULL\n",
		       __func__);
		return;
	}

	swfrminfo_cb = data.data;
	pipe = (struct mtk_imgsys_pipe *)swfrminfo_cb->pipe;
	if (!pipe->streaming) {
		pr_info("req track-%s pipe already streamoff %s-%d-%d\n", __func__,
    		((char *)&swfrminfo_cb->frm_owner), swfrminfo_cb->request_fd,
    		swfrminfo_cb->request_no);
		if (isLastTaskInReq) {
			#if SMVR_DECOUPLE
				if (is_capture) {
					mtk_hcp_put_gce_buffer(pipe->imgsys_dev->scp_pdev, imgsys_capture);
				} else {
					if (batchnum) {
						mtk_hcp_put_gce_buffer(pipe->imgsys_dev->scp_pdev, imgsys_smvr);
					} else {
						mtk_hcp_put_gce_buffer(pipe->imgsys_dev->scp_pdev, imgsys_streaming);
					}
				}
			#else
				mtk_hcp_put_gce_buffer(pipe->imgsys_dev->scp_pdev);
			#endif
		}
		return;
	}

	req = (struct mtk_imgsys_request *)(swfrminfo_cb->req);
	if (!req) {
		pr_info("%s NULL request Address\n", __func__);
		return;
	}

    req_track = (union request_track *)req->req_stat;
	if (swfrminfo_cb->is_lastfrm && isLastTaskInReq
		&& (swfrminfo_cb->fail_isHWhang == -1)
		&& (pipe->streaming)) {
		//req_track = (union request_track *)req->req_stat;
		//req_track->mainflow_from = REQUEST_DONE_FROM_KERNEL_TO_IMGSTREAM;
		req_track->subflow_kernel++;
	}

	IMGSYS_SYSTRACE_BEGIN("ReqFd:%d Own:%s\n", req->tstate.req_fd,
							((char *)&swfrminfo_cb->frm_owner));

	imgsys_dev = req->imgsys_pipe->imgsys_dev;
#ifdef REQ_TIMESTAMP
	req->tstate.time_mdpcbStart = ktime_get_boottime_ns()/1000;
#endif
    if (imgsys_dbg_enable()) {
		dev_info(imgsys_dev->dev, "gce count-%s:(reqfd-%d/%d)frame_no(%d)-Own:%s +", __func__,
			req->tstate.req_fd,
			swfrminfo_cb->request_no,
			swfrminfo_cb->frame_no,
			((char *)&swfrminfo_cb->frm_owner));
    }


#if MTK_CM4_SUPPORT == 0
	#ifdef BATCH_MODE_V3
	// V3 batch mode added {
	if (is_batch_mode(req)) {
		struct mtk_imgsys_hw_subframe *working_buf;
		struct img_ipi_frameparam *frame;

        if (imgsys_dbg_enable())
		dev_dbg(imgsys_dev->dev, "%s:req(%p)", __func__, req);
		working_buf = list_first_entry(&req->runner_done_list,
					       struct mtk_imgsys_hw_subframe,
					       list_entry);

		spin_lock(&req->runner_done_list.lock);
		working_buf = list_first_entry(&req->runner_done_list.list,
					       struct mtk_imgsys_hw_subframe,
					       list_entry);

		if (IS_ERR(working_buf)) {
			dev_info(imgsys_dev->dev,
				"fail to get working buffer:%p\n", working_buf);
			return;
		}

		if (list_empty(&req->runner_done_list.list)) {
			dev_info(imgsys_dev->dev, "%s:list is empty!\n",
								__func__);
			return;
		}

		spin_lock(&req->runner_done_list.lock);
		working_buf = list_first_entry(&req->runner_done_list.list,
					       struct mtk_imgsys_hw_subframe,
					       list_entry);

		if (IS_ERR(working_buf)) {
			dev_info(imgsys_dev->dev,
				"fail to get working buffer:%p\n", working_buf);
			return;
		}

		list_del(&working_buf->list_entry);
		req->runner_done_list.cnt--;
		spin_unlock(&req->runner_done_list.lock);

		spin_lock(&req->mdp_done_list.lock);
		list_add_tail(&working_buf->list_entry,
				  &req->mdp_done_list.list);
		req->mdp_done_list.cnt++;
		spin_unlock(&req->mdp_done_list.lock);

		frame = (struct img_ipi_frameparam *)
						working_buf->frameparam.vaddr;
        if (imgsys_dbg_enable()) {
		dev_dbg(imgsys_dev->dev,
			 "%s: req(%p), idx(%d), no(%d), n_in(%d), n_out(%d)\n",
			 __func__,
			 req,
			 frame->index,
			 frame->frame_no,
			 frame->num_inputs,
			 frame->num_outputs);
        }

		if (data.sta != CMDQ_CB_NORMAL) {
			dev_info(imgsys_dev->dev, "%s: frame no(%d) HW timeout\n",
				__func__, req->img_fparam.frameparam.frame_no);
			req->img_fparam.frameparam.state =
							FRAME_STATE_HW_TIMEOUT;
			mtk_imgsys_notify_batch(req);
		} else {
			mtk_imgsys_notify_batch(req);
		}
		return;
	}
	// } V3 batch mode added
	#endif
#endif
    if (imgsys_dbg_enable()) {
		dev_dbg(imgsys_dev->dev, "%s: req(%p), idx(%d), no(%d), s(%d), n_in(%d), n_out(%d)\n",
			__func__,
			req,
			req->img_fparam.frameparam.index,
			req->img_fparam.frameparam.frame_no,
			req->img_fparam.frameparam.state,
			req->img_fparam.frameparam.num_inputs,
			req->img_fparam.frameparam.num_outputs);
    }

	/*check gce cb cnt*/
	mutex_lock(&(reqfd_cbinfo_list.mymutex));
	list_for_each_safe(head, temp, &(reqfd_cbinfo_list.mylist)) {
		reqfdcb_info = vlist_node_of(head, struct reqfd_cbinfo_t);
		if ((reqfdcb_info->req_fd == swfrminfo_cb->request_fd) &&
			(reqfdcb_info->req_no == swfrminfo_cb->request_no) &&
			(reqfdcb_info->frm_no == swfrminfo_cb->frame_no) &&
			(reqfdcb_info->frm_owner == swfrminfo_cb->frm_owner)) {
			reqfdcb_info->cur_cnt += 1;
			exp_cnt = reqfdcb_info->exp_cnt;
			cur_cnt = reqfdcb_info->cur_cnt;
			if (reqfdcb_info->cur_cnt == reqfdcb_info->exp_cnt) {
				can_notify_imgsys = true;
				list_del(head);
#ifdef REQFD_CBINFO
				vfree(reqfdcb_info);
#else
				reqfd_cbinfo_put_work(reqfdcb_info);
#endif
			}
			reqfd_record_find = true;
		}
	}
	mutex_unlock(&(reqfd_cbinfo_list.mymutex));
    req_track->subflow_kernel++;
	if (!reqfd_record_find) {
		dev_info(imgsys_dev->dev,
			"%s:%s:req fd/no(%d/%d)frame no(%d)no record, kva(0x%lx)group ID/L(%d/%d)e_cb(idx_%d:%d)tfrm(%d) cb/lst(%d/%d)->%d/%d\n",
			__func__, (char *)(&(swfrminfo_cb->frm_owner)), swfrminfo_cb->request_fd,
			swfrminfo_cb->request_no,
			swfrminfo_cb->frame_no,
			(unsigned long)swfrminfo_cb,
			swfrminfo_cb->group_id,
			swfrminfo_cb->user_info[subfidx].is_lastingroup,
			subfidx,
			swfrminfo_cb->user_info[subfidx].is_earlycb,
			swfrminfo_cb->total_frmnum,
			swfrminfo_cb->is_earlycb,
			swfrminfo_cb->is_lastfrm,
			isLastTaskInReq, lastfrmInMWReq);
             return;
	}
	/**/
	if (swfrminfo_cb->fail_isHWhang >= 0) {
		req->img_fparam.frameparam.state = FRAME_STATE_HW_TIMEOUT;
		if (swfrminfo_cb->group_id >= 0) {
			if (isLastTaskInReq) {
				if (swfrminfo_cb->is_lastfrm)
					lastfrmInMWReq = true;
				lastin_errcase = true;
			}

			dev_info(imgsys_dev->dev,
			"%s:%s:%d:req fd/no(%d/%d)frame no(%d)timeout, kva(0x%lx)group ID/L(%d/%d)e_cb(idx_%d:%d)tfrm(%d) cb/lst(%d/%d)->%d/%d,%d(%d/%d)\n",
			__func__, (char *)(&(swfrminfo_cb->frm_owner)), pipe->streaming,
			swfrminfo_cb->request_fd,
			swfrminfo_cb->request_no,
			swfrminfo_cb->frame_no,
			(unsigned long)swfrminfo_cb,
			swfrminfo_cb->group_id,
			swfrminfo_cb->user_info[subfidx].is_lastingroup,
			subfidx,
			swfrminfo_cb->user_info[subfidx].is_earlycb,
			swfrminfo_cb->total_frmnum,
			swfrminfo_cb->is_earlycb,
			swfrminfo_cb->is_lastfrm,
			isLastTaskInReq, lastfrmInMWReq, can_notify_imgsys,
			cur_cnt, exp_cnt);
		} else {
			if (swfrminfo_cb->is_lastfrm) {
				lastfrmInMWReq = true;
				can_notify_imgsys = true;
			}

			dev_info(imgsys_dev->dev,
			"%s:%s:%d:req fd/no(%d/%d)frame no(%d) timeout, kva(0x%lx)lst(%d)e_cb(%d)sidx(%d)tfrm(%d) -> %d,%d(%d/%d)\n",
			__func__, (char *)(&(swfrminfo_cb->frm_owner)), pipe->streaming,
			swfrminfo_cb->request_fd,
			swfrminfo_cb->request_no,
			swfrminfo_cb->frame_no,
			(unsigned long)swfrminfo_cb,
			swfrminfo_cb->is_lastfrm,
			swfrminfo_cb->is_earlycb,
			swfrminfo_cb->user_info[0].subfrm_idx,
			swfrminfo_cb->total_frmnum,
			lastfrmInMWReq, can_notify_imgsys,
			cur_cnt, exp_cnt);

			/*early cb or last frame for non-grouping case*/
			lastin_errcase = true;
		}

		if (/*pipe->streaming && */can_notify_imgsys/*lastfrmInMWReq*/)
			mtk_imgsys_notify(req, swfrminfo_cb->frm_owner);

		if (lastin_errcase) {
    		#if SMVR_DECOUPLE
    			if (swfrminfo_cb->is_capture) {
            	    mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_capture);
                } else {
                    if (swfrminfo_cb->batchnum) {
                        mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_smvr);
                    } else {
                        mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_streaming);
                    }
                }
    		#else
			mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev);
    		#endif
		}
	} else {
		if (swfrminfo_cb->is_lastfrm || swfrminfo_cb->is_earlycb ||
			isLastTaskInReq) {
			if (!pipe->streaming) {
				dev_info(imgsys_dev->dev,
				"%s:%s:req fd/no(%d/%d)frame no(%d)done, kva(0x%lx)lst(%d)e_cb(%d/%d)sidx(%d)tfrm(%d)\n",
				__func__, (char *)(&(swfrminfo_cb->frm_owner)),
				swfrminfo_cb->request_fd,
				swfrminfo_cb->request_no,
				swfrminfo_cb->frame_no,
				(unsigned long)swfrminfo_cb,
				swfrminfo_cb->is_lastfrm,
				swfrminfo_cb->is_earlycb,
				swfrminfo_cb->earlycb_sidx,
				swfrminfo_cb->user_info[0].subfrm_idx,
				swfrminfo_cb->total_frmnum);
			} else {
			    if (imgsys_dbg_enable()) {
				dev_dbg(imgsys_dev->dev,
				"%s:%s:req fd/no(%d/%d)frame no(%d)done, kva(0x%lx)lst(%d)e_cb(%d/%d)sidx(%d)tfrm(%d)\n",
				__func__, (char *)(&(swfrminfo_cb->frm_owner)),
				swfrminfo_cb->request_fd,
				swfrminfo_cb->request_no,
				swfrminfo_cb->frame_no,
				(unsigned long)swfrminfo_cb,
				swfrminfo_cb->is_lastfrm,
				swfrminfo_cb->is_earlycb,
				swfrminfo_cb->earlycb_sidx,
				swfrminfo_cb->user_info[0].subfrm_idx,
				swfrminfo_cb->total_frmnum);
			}
		}
		}
		if (swfrminfo_cb->group_id >= 0) {
			if (!pipe->streaming) {
				dev_info(imgsys_dev->dev,
				"%s:%s:%d:req fd/no(%d/%d)frame no(%d)done, kva(0x%lx)group ID/L(%d/%d)e_cb(idx_%d:%d)tfrm(%d) cb/lst(%d/%d):%d,%d(%d/%d)\n",
				__func__, (char *)(&(swfrminfo_cb->frm_owner)), pipe->streaming,
				swfrminfo_cb->request_fd,
				swfrminfo_cb->request_no,
				swfrminfo_cb->frame_no,
				(unsigned long)swfrminfo_cb,
				swfrminfo_cb->group_id,
				swfrminfo_cb->user_info[subfidx].is_lastingroup,
				subfidx,
				swfrminfo_cb->user_info[subfidx].is_earlycb,
				swfrminfo_cb->total_frmnum,
				swfrminfo_cb->is_earlycb,
				swfrminfo_cb->is_lastfrm,
				isLastTaskInReq, can_notify_imgsys,
				cur_cnt, exp_cnt);
			} else {
			    if (imgsys_dbg_enable()) {
				dev_dbg(imgsys_dev->dev,
				"%s:%s:%d:req fd/no(%d/%d)frame no(%d)done, kva(0x%lx)group ID/L(%d/%d)e_cb(idx_%d:%d)tfrm(%d) cb/lst(%d/%d):%d,%d(%d/%d)\n",
				__func__, (char *)(&(swfrminfo_cb->frm_owner)), pipe->streaming,
				swfrminfo_cb->request_fd,
				swfrminfo_cb->request_no,
				swfrminfo_cb->frame_no,
				(unsigned long)swfrminfo_cb,
				swfrminfo_cb->group_id,
				swfrminfo_cb->user_info[subfidx].is_lastingroup,
				subfidx,
				swfrminfo_cb->user_info[subfidx].is_earlycb,
				swfrminfo_cb->total_frmnum,
				swfrminfo_cb->is_earlycb,
				swfrminfo_cb->is_lastfrm,
				isLastTaskInReq, can_notify_imgsys,
				cur_cnt, exp_cnt);
			}
			}

			if (swfrminfo_cb->user_info[subfidx].is_earlycb) {
				ev.req_fd = swfrminfo_cb->request_fd;
				ev.frame_number = swfrminfo_cb->user_info[subfidx].subfrm_idx;
                req_track->subflow_kernel++;
				mtk_imgsys_early_notify(req, &ev);
			}

			if (isLastTaskInReq) {
				need_notify_daemon = true;
				if (swfrminfo_cb->is_lastfrm)
					lastfrmInMWReq = true;
			}
		} else {
			/* call early notify if need early callback */
			if (swfrminfo_cb->is_earlycb) {
				ev.req_fd = swfrminfo_cb->request_fd;
				ev.frame_number = swfrminfo_cb->earlycb_sidx;
                req_track->subflow_kernel++;
				mtk_imgsys_early_notify(req, &ev);
			}
			if (swfrminfo_cb->is_lastfrm) {
				lastfrmInMWReq = true;
				can_notify_imgsys = true;
			}
			need_notify_daemon = true;
		}
        if (imgsys_dbg_enable()) {
		dev_info(imgsys_dev->dev,
			"%s:%s:req fd/no(%d/%d)frame no(%d)done, kva(0x%lx)lastfrmInMWReq:%d\n",
			__func__, (char *)(&(swfrminfo_cb->frm_owner)), swfrminfo_cb->request_fd,
			swfrminfo_cb->request_no,
			swfrminfo_cb->frame_no,
			(unsigned long)swfrminfo_cb, lastfrmInMWReq);
        }
		/* call dip notify when all package done */
		if (/*pipe->streaming && */can_notify_imgsys/*lastfrmInMWReq*/) {
            req_track->subflow_kernel++;
			mtk_imgsys_notify(req, swfrminfo_cb->frm_owner);
		}

		if (need_notify_daemon) {
			gwork.reqfd = swfrminfo_cb->request_fd;
			//memcpy((void *)(&(gwork->user_info)), (void *)(&(frm_info_cb->user_info)),
			//	sizeof(struct img_swfrm_info));
			#if SMVR_DECOUPLE
			if (swfrminfo_cb->is_capture) {
				gwork.req_sbuf_kva = swfrminfo_cb->req_sbuf_goft
					+ mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev, imgsys_capture);
			} else {
				if (swfrminfo_cb->batchnum) {
					gwork.req_sbuf_kva = swfrminfo_cb->req_sbuf_goft
						+ mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev, imgsys_smvr);
				} else {
					gwork.req_sbuf_kva = swfrminfo_cb->req_sbuf_goft
						+ mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev, imgsys_streaming);
				}
			}
			#else
			gwork.req_sbuf_kva = swfrminfo_cb->req_sbuf_goft
				+ mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev);
			#endif
			gwork.pipe = swfrminfo_cb->pipe;
			cmdq_cb_done_worker(&gwork.work);
			/*grouping, paired with scp_handler*/
			#if SMVR_DECOUPLE
			if (swfrminfo_cb->is_capture) {
        	    mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_capture);
            } else {
                if (swfrminfo_cb->batchnum) {
                    mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_smvr);
                } else {
                    mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_streaming);
                }
            }
			#else
			mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev);
			#endif
		}
	}

	IMGSYS_SYSTRACE_END();
}

int mtk_imgsys_hw_working_buf_pool_init(struct mtk_imgsys_dev *imgsys_dev)
{
	int i;
	const int working_buf_size = round_up(DIP_FRM_SZ, PAGE_SIZE);
	phys_addr_t working_buf_paddr;
	const u32 scp_workingbuf_offset = 0;

	dev_info(imgsys_dev->dev, "%s-7sp\n", __func__);

	INIT_LIST_HEAD(&imgsys_dev->imgsys_freebufferlist.list);
	spin_lock_init(&imgsys_dev->imgsys_freebufferlist.lock);
	imgsys_dev->imgsys_freebufferlist.cnt = 0;

	INIT_LIST_HEAD(&imgsys_dev->imgsys_usedbufferlist.list);
	spin_lock_init(&imgsys_dev->imgsys_usedbufferlist.lock);
	imgsys_dev->imgsys_usedbufferlist.cnt = 0;

#if MTK_CM4_SUPPORT
	scp_workingbuf_offset = DIP_SCP_WORKINGBUF_OFFSET;
	imgsys_dev->working_buf_mem_size = DIP_SUB_FRM_DATA_NUM *
		working_buf_size + DIP_SCP_WORKINGBUF_OFFSET;
	imgsys_dev->working_buf_mem_vaddr =
		dma_alloc_coherent(&imgsys_dev->scp_pdev->dev,
				   imgsys_dev->working_buf_mem_size,
				   &imgsys_dev->working_buf_mem_scp_daddr,
				   GFP_KERNEL);
	if (!imgsys_dev->working_buf_mem_vaddr) {
		dev_info(imgsys_dev->dev,
			"memory alloc size %ld failed\n",
			imgsys_dev->working_buf_mem_size);
		return -ENOMEM;
	}

	/*
	 * We got the incorrect physical address mapped when
	 * using dma_map_single() so I used dma_map_page_attrs()
	 * directly to workaround here.
	 *
	 * When I use dma_map_single() to map the address, the
	 * physical address retrieved back with iommu_get_domain_for_dev()
	 * and iommu_iova_to_phys() was not equal to the
	 * SCP dma address (it should be the same as the physical address
	 * since we don't have iommu), and was shifted by 0x4000000.
	 */
	working_buf_paddr = imgsys_dev->working_buf_mem_scp_daddr;
	imgsys_dev->working_buf_mem_isp_daddr =
		dma_map_page_attrs(imgsys_dev->dev,
				   phys_to_page(working_buf_paddr),
				   0, imgsys_dev->working_buf_mem_size,
				   DMA_BIDIRECTIONAL,
				   DMA_ATTR_SKIP_CPU_SYNC);
	if (dma_mapping_error(imgsys_dev->dev,
			      imgsys_dev->working_buf_mem_isp_daddr)) {
		dev_info(imgsys_dev->dev,
			"failed to map buffer: s_daddr(%pad)\n",
			&imgsys_dev->working_buf_mem_scp_daddr);
		dma_free_coherent(&imgsys_dev->scp_pdev->dev,
				  imgsys_dev->working_buf_mem_size,
				  imgsys_dev->working_buf_mem_vaddr,
				  imgsys_dev->working_buf_mem_scp_daddr);

		return -ENOMEM;
	}
#else
	/* TODO: only for header_desc mode; not sigdev */
#ifdef USE_KERNEL_ION_BUFFER
	imgsys_dev->working_buf_mem_size =
				mtk_hcp_get_reserve_mem_size(IMG_MEM_FOR_HW_ID);
	hcp_allocate_buffer(imgsys_dev->scp_pdev, IMG_MEM_FOR_HW_ID,
		imgsys_dev->working_buf_mem_size);
	imgsys_dev->working_buf_mem_scp_daddr =
				mtk_hcp_get_reserve_mem_phys(IMG_MEM_FOR_HW_ID);
	imgsys_dev->working_buf_mem_vaddr =
				mtk_hcp_get_reserve_mem_virt(IMG_MEM_FOR_HW_ID);
	imgsys_dev->working_buf_mem_isp_daddr =
				mtk_hcp_get_reserve_mem_dma(IMG_MEM_FOR_HW_ID);
	if (!imgsys_dev->working_buf_mem_vaddr) {
		dev_info(imgsys_dev->dev,
			"(hcp)memory alloc size %ld failed\n",
			imgsys_dev->working_buf_mem_size);
		return -ENOMEM;
	}
#endif
	working_buf_paddr = imgsys_dev->working_buf_mem_scp_daddr;
#endif
    if (imgsys_dbg_enable()) {
	dev_info(imgsys_dev->dev,
		"%s: working_buf_mem: vaddr(0x%lx), scp_daddr(%pad)\n",
		__func__,
		(unsigned long)imgsys_dev->working_buf_mem_vaddr,
		&imgsys_dev->working_buf_mem_scp_daddr);
    }

	pr_info("%s: working buffer size 0x%lx, pool size should be 0x%lx\n",
		__func__, DIP_FRM_SZ, (DIP_FRM_SZ * DIP_SUB_FRM_DATA_NUM));
	for (i = 0; i < DIP_SUB_FRM_DATA_NUM; i++) {
		struct mtk_imgsys_hw_subframe *buf =
						&imgsys_dev->working_buf[i];

		/*
		 * Total: 0 ~ 72 KB
		 * SubFrame: 0 ~ 16 KB
		 */
		buf->buffer.scp_daddr = imgsys_dev->working_buf_mem_scp_daddr +
			scp_workingbuf_offset + i * working_buf_size;
		buf->buffer.vaddr = imgsys_dev->working_buf_mem_vaddr +
			scp_workingbuf_offset + i * working_buf_size;
		buf->buffer.isp_daddr = imgsys_dev->working_buf_mem_isp_daddr +
			scp_workingbuf_offset + i * working_buf_size;
		buf->size = working_buf_size;

        if (imgsys_dbg_enable()) {
		dev_dbg(imgsys_dev->dev,
			"%s: buf(%d), scp_daddr(%pad), isp_daddr(%pad)\n",
			__func__, i, &buf->buffer.scp_daddr,
			&buf->buffer.isp_daddr);
        }

		/* Tuning: 16 ~ 48 KB */
		buf->tuning_buf.scp_daddr =
			buf->buffer.scp_daddr + DIP_TUNING_OFFSET;
		buf->tuning_buf.vaddr =
			buf->buffer.vaddr + DIP_TUNING_OFFSET;
		buf->tuning_buf.isp_daddr =
			buf->buffer.isp_daddr + DIP_TUNING_OFFSET;

        if (imgsys_dbg_enable()) {
		dev_dbg(imgsys_dev->dev,
			"%s: tuning_buf(%d), scp_daddr(%pad), isp_daddr(%pad)\n",
			__func__, i, &buf->tuning_buf.scp_daddr,
			&buf->tuning_buf.isp_daddr);
        }

		/* Config_data: 48 ~ 72 KB */
		buf->config_data.scp_daddr =
			buf->buffer.scp_daddr + DIP_COMP_OFFSET;
		buf->config_data.vaddr = buf->buffer.vaddr + DIP_COMP_OFFSET;

        if (imgsys_dbg_enable()) {
		dev_dbg(imgsys_dev->dev,
			"%s: config_data(%d), scp_daddr(%pad), vaddr(0x%lx)\n",
			__func__, i, &buf->config_data.scp_daddr,
			(unsigned long)buf->config_data.vaddr);
        }

		/* Frame parameters: 72 ~ 76 KB */
		buf->frameparam.scp_daddr =
			buf->buffer.scp_daddr + DIP_FRAMEPARAM_OFFSET;
		buf->frameparam.vaddr =
			buf->buffer.vaddr + DIP_FRAMEPARAM_OFFSET;

		pr_info("DIP_FRAMEPARAM_SZ:%lu",
			(unsigned long)sizeof(struct img_ipi_frameparam));
        if (imgsys_dbg_enable()) {
		dev_dbg(imgsys_dev->dev,
			"%s: frameparam(%d), scp_daddr(%pad), vaddr(0x%lx)\n",
			__func__, i, &buf->frameparam.scp_daddr,
			(unsigned long)buf->frameparam.vaddr);
        }

		list_add_tail(&buf->list_entry,
			      &imgsys_dev->imgsys_freebufferlist.list);
		imgsys_dev->imgsys_freebufferlist.cnt++;
	}

	for (i = 0; i < VIDEO_MAX_FRAME;i++) {
		INIT_WORK(&imgsys_timeout_winfo[i].work, cmdq_cb_timeout_worker);
	}

	return 0;
}
#else

#ifdef BATCH_MODE_V3
// V3 batch mode added {
static void dip_fake_mdp_cb_func(struct cmdq_cb_data data)
{
	struct mtk_imgsys_request *req;
	struct mtk_imgsys_dev *imgsys_dev;
	struct mtk_imgsys_hw_subframe *working_buf;
	struct img_ipi_frameparam *frame;

	if (!data.data) {
		pr_info("%s: data->data is NULL\n",
		       __func__);
		return;
	}

	req = data.data;
	imgsys_dev = req->imgsys_pipe->imgsys_dev;
    if (imgsys_dbg_enable())
	dev_dbg(imgsys_dev->dev, "%s:req(%p)", __func__, req);

	if (list_empty(&req->runner_done_list.list)) {
		dev_info(imgsys_dev->dev, "%s:list is empty!\n", __func__);
		return;
	}

	spin_lock(&req->runner_done_list.lock);
	working_buf = list_first_entry(&req->runner_done_list.list,
				       struct mtk_imgsys_hw_subframe,
				       list_entry);

	if (IS_ERR(working_buf)) {
		dev_info(imgsys_dev->dev,
			"fail to get working buffer:%p\n", working_buf);
		return;
	}

	list_del(&working_buf->list_entry);
	req->runner_done_list.cnt--;
	spin_unlock(&req->runner_done_list.lock);

	spin_lock(&req->mdp_done_list.lock);
	list_add_tail(&working_buf->list_entry,
			  &req->mdp_done_list.list);
	req->mdp_done_list.cnt++;
	spin_unlock(&req->mdp_done_list.lock);

	frame = (struct img_ipi_frameparam *)working_buf->frameparam.vaddr;
    if (imgsys_dbg_enable()) {
	dev_dbg(imgsys_dev->dev,
		 "%s: req(%p), idx(%d), no(%d), s(%d), n_in(%d), n_out(%d)\n",
		 __func__,
		 req,
		 frame->index,
		 frame->frame_no,
		 frame->state,
		 frame->num_inputs,
		 frame->num_outputs);
    }
	if (data.sta != CMDQ_CB_NORMAL) {
		dev_info(imgsys_dev->dev, "%s: frame no(%d) HW timeout\n",
			__func__, req->img_fparam.frameparam.frame_no);
		/* Skip some action since this is UT purpose. */
	} else {
		mtk_imgsys_notify_batch(req);
        if (imgsys_dbg_enable())
		dev_dbg(imgsys_dev->dev, "%s: -\n", __func__);
	}
}
// } V3 batch mode added
#endif

/* ToDo: use real mdp3 function */
static void dip_fake_mdp_cmdq_sendtask(struct mtk_imgsys_request *req)
{
	struct mtk_imgsys_dev *imgsys_dev;

	imgsys_dev = req->imgsys_pipe->imgsys_dev;

	#ifdef BATCH_MODE_V3
	// V3 batch mode added {
	if (is_batch_mode(req)) {
		struct cmdq_cb_data data;

		pr_info("fake cmdq sendtask");
		data.data = req;
		data.sta = CMDQ_CB_NORMAL;
		dip_fake_mdp_cb_func(data);
		return;
	}
	// } V3 batch mode added
	#endif

    if (imgsys_dbg_enable()) {
	dev_dbg(imgsys_dev->dev, "%s: req(%p), idx(%d), no(%d), s(%d), n_in(%d), n_out(%d)\n",
		__func__,
		req,
		req->img_fparam.frameparam.index,
		req->img_fparam.frameparam.frame_no,
		req->img_fparam.frameparam.state,
		req->img_fparam.frameparam.num_inputs,
		req->img_fparam.frameparam.num_outputs);
    }

	mtk_imgsys_notify(req, 0);
}
#endif

#if MTK_CM4_SUPPORT == 0
#ifdef BATCH_MODE_V3
static void dip_runner_func_batch(struct work_struct *work)
{
	struct mtk_runner_work *runner_work =
		container_of(work, struct mtk_runner_work, frame_work);
	struct mtk_imgsys_hw_subframe *working_buf;
	struct img_ipi_frameparam *frame;
	struct mtk_imgsys_request *req = runner_work->req;
	struct mtk_imgsys_dev *imgsys_dev = req->imgsys_pipe->imgsys_dev;

	/*
	 * Call MDP/GCE API to do HW excecution
	 * Pass the framejob to MDP driver
	 */
	pm_runtime_get_sync(imgsys_dev->dev);

    if (imgsys_dbg_enable())
	dev_dbg(imgsys_dev->dev, "%s +", __func__);
	spin_lock(&req->scp_done_list.lock);

	working_buf = list_first_entry(&req->scp_done_list.list,
				       struct mtk_imgsys_hw_subframe,
				       list_entry);
	if (!working_buf)
		return;

    if (imgsys_dbg_enable())
	dev_dbg(imgsys_dev->dev, "%s ++", __func__);

	list_del(&working_buf->list_entry);
	req->scp_done_list.cnt--;
	spin_unlock(&req->scp_done_list.lock);

	spin_lock(&req->runner_done_list.lock);
	list_add_tail(&working_buf->list_entry,
			  &req->runner_done_list.list);
	req->runner_done_list.cnt++;
	spin_unlock(&req->runner_done_list.lock);

	frame = (struct img_ipi_frameparam *)
					working_buf->frameparam.vaddr;
	/* send working buffer address as frame param to mdp */
	// ToDo: mdp3 porting
	//mdp_cmdq_sendtask(imgsys_dev->mdp_pdev, config_data,
	//			(struct img_ipi_frameparam *)
	//				working_buf->frameparam.vaddr,
	//			NULL, false,
	//			imgsys_mdp_cb_func, req);
	dip_fake_mdp_cmdq_sendtask(req);
    if (imgsys_dbg_enable())
	dev_dbg(imgsys_dev->dev, "%s -", __func__);
	vfree(work);
	return;

}
#endif
#endif

static void work_pool_init(struct work_pool *work_pool, void *cookie)
{
	spin_lock_init(&work_pool->lock);
	INIT_LIST_HEAD(&work_pool->free_list);
	INIT_LIST_HEAD(&work_pool->used_list);

	work_pool->_cookie = cookie;
	init_waitqueue_head(&work_pool->waitq);
	kref_init(&work_pool->kref);
}

static void pool_release(struct kref *kref)
{
	struct list_head *gwork, *g0;
	struct work_pool *pool = container_of(kref, struct work_pool, kref);
	struct mtk_imgsys_dev *dev = container_of(pool, struct mtk_imgsys_dev, gwork_pool);

	spin_lock(&pool->lock);
	list_for_each_safe(gwork, g0, &pool->free_list) {
		list_del(gwork);
		atomic_dec(&pool->num);
	}
	list_for_each_safe(gwork, g0, &pool->used_list) {
		list_del(gwork);
		atomic_dec(&pool->num);
	}
	spin_unlock(&pool->lock);

	if (pool->_cookie) {
		vfree(pool->_cookie);
		pool->_cookie = NULL;
	}

	if (atomic_read(&pool->num))
		dev_info(dev->dev, "%s: %d works not freed", __func__,
					atomic_read(&pool->num));

}

static void work_pool_uninit(struct work_pool *pool)
{

	if (!pool)
		return;

	kref_put(&pool->kref, pool_release);
}

static struct list_head *work_pool_get(struct work_pool *pool)
{
	struct list_head *work = NULL;

	spin_lock(&pool->lock);
	if (!list_empty(&pool->free_list)) {
		work = pool->free_list.next;
		list_del(work);
		list_add_tail(work, &pool->used_list);
		kref_get(&pool->kref);
	}
	spin_unlock(&pool->lock);

	return work;
}

static struct list_head *get_work(struct work_pool *gwork_pool)
{
	struct list_head *work = NULL;
	int ret;

	ret = wait_event_interruptible_timeout(gwork_pool->waitq,
				((work = work_pool_get(gwork_pool)) != NULL),
								msecs_to_jiffies(3000));
	if (!ret) {
		pr_info("%s wait for pool timeout\n", __func__);
		return NULL;
	} else if (-ERESTARTSYS == ret) {
		pr_info("%s wait for pool interrupted !\n", __func__);
		return NULL;
	}

	return work;
}

static int gce_work_pool_init(struct mtk_imgsys_dev *dev)
{
	struct gce_work *gwork = NULL;
	struct work_pool *gwork_pool;
	signed int i;
	int ret = 0;

	gwork_pool = &dev->gwork_pool;

	gwork = vzalloc(sizeof(struct gce_work) * GCE_WORK_NR);
	if (!gwork)
		return -ENOMEM;

	work_pool_init(gwork_pool, (void *)gwork);
	for (i = 0; i < GCE_WORK_NR; i++) {
		list_add_tail(&gwork[i].entry, &gwork_pool->free_list);
		gwork[i].pool = gwork_pool;
	}
	atomic_set(&gwork_pool->num, i);

	return ret;
}

static struct gce_work *gce_get_work(struct mtk_imgsys_dev *dev)
{
	struct list_head *work = NULL;
	struct gce_work *gwork = NULL;
	struct work_pool *gwork_pool;

	gwork_pool = &dev->gwork_pool;

	work = get_work(gwork_pool);
	if (!work)
		return NULL;

	gwork = container_of(work, struct gce_work, entry);

	return gwork;
}

static void gce_put_work(struct gce_work *gwork)
{
	struct work_pool *gwork_pool;

	gwork_pool = gwork->pool;
	spin_lock(&gwork_pool->lock);
	list_del(&gwork->entry);
	list_add_tail(&gwork->entry, &gwork_pool->free_list);
	spin_unlock(&gwork_pool->lock);
	kref_put(&gwork_pool->kref, pool_release);

}

#define REQCB_WORK_NR (64)
static int reqfd_cbinfo_work_pool_init(struct mtk_imgsys_dev *dev)
{
	vlist_type(struct reqfd_cbinfo_t) * work = NULL;
	struct work_pool *work_pool;
	signed int i;
	int ret = 0;

	work_pool = &dev->reqfd_cbinfo_pool;

	work = vzalloc(sizeof(vlist_type(struct reqfd_cbinfo_t)) * REQCB_WORK_NR);
	if (!work)
		return -ENOMEM;

	work_pool_init(work_pool, (void *)work);
	for (i = 0; i < REQCB_WORK_NR; i++) {
		list_add_tail(&work[i].entry, &work_pool->free_list);
		work[i].pool = work_pool;
	}
	atomic_set(&work_pool->num, i);

	return ret;
}

#ifndef REQFD_CBINFO
static struct reqfd_cbinfo_t *reqfd_cbinfo_get_work(struct mtk_imgsys_dev *dev)
{
	vlist_type(struct reqfd_cbinfo_t) * gwork = NULL;
	struct list_head *work = NULL;
	struct work_pool *gwork_pool;

	gwork_pool = &dev->reqfd_cbinfo_pool;

	work = get_work(gwork_pool);
	if (!work)
		return NULL;

	gwork = container_of(work, vlist_type(struct reqfd_cbinfo_t), entry);

	return (struct reqfd_cbinfo_t *)gwork;
}

static void reqfd_cbinfo_put_work(struct reqfd_cbinfo_t *work)
{
	vlist_type(struct reqfd_cbinfo_t) * gwork;
	struct work_pool *work_pool;

	gwork = (vlist_type(struct reqfd_cbinfo_t) *)work;
	work_pool = gwork->pool;

	spin_lock(&work_pool->lock);
	list_del(&gwork->entry);
	list_add_tail(&gwork->entry, &work_pool->free_list);
	spin_unlock(&work_pool->lock);
	kref_put(&work_pool->kref, pool_release);

}
#endif

static u64 transform_tuning_iova(struct mtk_imgsys_dev *imgsys_dev, struct mtk_imgsys_request *req,
	struct tuning_meta_info *tuning_info, int frm_index)
{
	//struct dma_buf *dbuf = NULL;
	struct mtk_imgsys_dev_buffer *dev_b = 0;
	struct singlenode_desc *desc_sd = NULL;
	struct singlenode_desc_norm *desc_sd_norm = NULL;
	//int buf_fd;
	//u64 iova_addr = 0;
	//unsigned int tmax, tnum;
	struct header_desc *input_smvr = NULL;
	struct header_desc_norm *input_norm = NULL;
	unsigned int j = 0;
	//buf_fd = frm_info->user_info[frm_index]->priv[IMGSYS_DIP].buf_fd;

	dev_b = req->buf_map[is_singledev_mode(req)];
	switch (dev_b->dev_fmt->format) {
	default:
	case V4L2_META_FMT_MTISP_SD:
		desc_sd = (struct singlenode_desc *)dev_b->va_daddr[0];
		input_smvr = (struct header_desc *)(&desc_sd->tuning_meta);
		break;
	case V4L2_META_FMT_MTISP_SDNORM:
		desc_sd_norm =
			(struct singlenode_desc_norm *)dev_b->va_daddr[0];
		input_norm = (struct header_desc_norm *)(&desc_sd_norm->tuning_meta);
		break;
	}
	if (input_smvr) {
		if (desc_sd->dmas_enable[MTK_IMGSYS_VIDEO_NODE_TUNING_OUT][frm_index]) {
			for (j = 0; j < input_smvr->fparams[frm_index][0].bufs[0].buf.num_planes;
				j++) {
				tuning_info->buf_fd =
			input_smvr->fparams[frm_index][0].bufs[0].buf.planes[j].m.dma_buf.fd;
				tuning_info->offset =
			input_smvr->fparams[frm_index][0].bufs[0].buf.planes[j].m.dma_buf.offset;
				tuning_info->dbuf = dma_buf_get(tuning_info->buf_fd);
				tuning_info->iova_addr =
					imgsys_dev->imgsys_get_iova(
					tuning_info->dbuf, tuning_info->buf_fd, imgsys_dev, dev_b);
                if (imgsys_dbg_enable()) {
				pr_debug("imgsys_fw: smvr iova addr(0x%llx/%d/0x%x)",
					tuning_info->iova_addr, tuning_info->buf_fd,
					tuning_info->offset);
			}
		}
		}
	} else {
		if (desc_sd_norm->dmas_enable[MTK_IMGSYS_VIDEO_NODE_TUNING_OUT][frm_index]) {
			for (j = 0; j < input_norm->fparams[frm_index][0].bufs[0].buf.num_planes;
				j++) {
				tuning_info->buf_fd =
			input_norm->fparams[frm_index][0].bufs[0].buf.planes[j].m.dma_buf.fd;
				tuning_info->offset =
			input_norm->fparams[frm_index][0].bufs[0].buf.planes[j].m.dma_buf.offset;
				tuning_info->dbuf = dma_buf_get(tuning_info->buf_fd);
				tuning_info->iova_addr =
					imgsys_dev->imgsys_get_iova(
					tuning_info->dbuf, tuning_info->buf_fd, imgsys_dev, dev_b);
                if (imgsys_dbg_enable()) {
				pr_debug("imgsys_fw: normal iova addr(0x%llx/%d/0x%x)",
					tuning_info->iova_addr, tuning_info->buf_fd,
					tuning_info->offset);
			}
		}
	}
	}
    if (imgsys_dbg_enable()) {
	pr_debug("imgsys_fw: iova addr(0x%llx/%d/0x%x)",
		tuning_info->iova_addr, tuning_info->buf_fd, tuning_info->offset);
    }
	return tuning_info->iova_addr;
}

static void imgsys_runner_func(void *data)
{
	struct imgsys_work *iwork = (struct imgsys_work *) data;
	struct gce_work *work = container_of(iwork, struct gce_work, work);
	struct mtk_imgsys_request *req = work->req;
	struct mtk_imgsys_dev *imgsys_dev = req->imgsys_pipe->imgsys_dev;
	union request_track *req_track = NULL;
	struct swfrm_info_t *frm_info;
	int swfrm_cnt;
	int i, ret;
	unsigned int subfidx;
#ifdef MTK_IOVA_SINK2KERNEL
	struct tuning_meta_info module_tuning_info;
	/* mark by bj due to buffer usage need to do further discuss */
	/* struct flush_buf_info tuning_meta_buf_info; */
	unsigned int mode;
	u64 iova_addr = 0;
#endif

#ifdef REQ_TIMESTAMP
	req->tstate.time_runnerStart = ktime_get_boottime_ns()/1000;
#endif
	swfrm_cnt = atomic_read(&req->swfrm_cnt);
	/* get corresponding setting, adopt is_sent to judge different frames
	 * in same package/mtk_dip_request
	 */
	frm_info = (struct swfrm_info_t *)(work->req_sbuf_kva);
	frm_info->is_sent = true;
	if (frm_info->is_lastfrm) {
		req_track = (union request_track *)req->req_stat;
		req_track->subflow_kernel++;
	}

#ifdef MTK_IOVA_SINK2KERNEL
	mode = (frm_info->batchnum > 0 ? imgsys_smvr :
		(frm_info->is_capture == 1 ? imgsys_capture : imgsys_streaming));
	for (subfidx = 0 ; subfidx < frm_info->total_frmnum ; subfidx++) {
		iova_addr = transform_tuning_iova(imgsys_dev, req, &module_tuning_info,
					frm_info->user_info[subfidx].subfrm_idx);
        if (imgsys_dbg_enable()) {
        //if (1) {
		pr_debug("imgsys_fw:tuning_index/frm_num/index/sub_frm(%d/%d/%d/%d)",
		MTK_IMGSYS_VIDEO_NODE_TUNING_OUT, frm_info->total_frmnum, subfidx,
		frm_info->user_info[subfidx].subfrm_idx);
		pr_debug("imgsys_fw:module tuning iova addr(0x%llx/%d/0x%x), mode(%d), hw_comb(0x%x)",
			module_tuning_info.iova_addr, module_tuning_info.buf_fd,
			module_tuning_info.offset, mode, frm_info->user_info[subfidx].hw_comb);
        }
		for (i = 0; i < (imgsys_dev->modules_num); i++) {
			if (imgsys_dev->modules[i].updatecq) {
				imgsys_dev->modules[i].updatecq(imgsys_dev,
					&frm_info->user_info[subfidx], frm_info->request_fd,
					iova_addr, mode);
		}
	}
		/* mark by bj due to buffer usage need to do further discuss */
		/* if ((frm_info->user_info[subfidx].hw_comb != IMGSYS_ENG_ME) &&
		 *	(module_tuning_info.iova_addr != 0)) {
		 *	tuning_meta_buf_info.fd = module_tuning_info.buf_fd;
		 *	tuning_meta_buf_info.offset = module_tuning_info.offset;
		 *	tuning_meta_buf_info.len =
		 *		frm_info->user_info[subfidx].tunmeta_size;//0x3e450;
		 *	tuning_meta_buf_info.mode = mode;
		 *	tuning_meta_buf_info.is_tuning = true;
		 *	tuning_meta_buf_info.dbuf = module_tuning_info.dbuf;
		 *	mtk_hcp_partial_flush(imgsys_dev->scp_pdev, &tuning_meta_buf_info);
		 *}
		 */
	}
#endif
	/*
	 * Call MDP/GCE API to do HW excecution
	 * Pass the framejob to MDP driver
	 */
	/*call gce communicator api*/
#ifdef REQ_TIMESTAMP
	if (!frm_info->user_info[0].subfrm_idx)
		req->tstate.time_send2cmq = ktime_get_boottime_ns()/1000;
	stime = ktime_get_boottime_ns()/1000;
#endif
	IMGSYS_SYSTRACE_BEGIN("MWFrame:#%d MWReq:#%d ReqFd:%d owner:%s subfrm_idx:%d\n",
			frm_info->frame_no, frm_info->request_no, req->tstate.req_fd,
		((char *)&frm_info->frm_owner), frm_info->user_info[0].subfrm_idx);

	#if SMVR_DECOUPLE
	if (frm_info->is_capture) {
	    mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev, imgsys_capture);
    } else {
        if (frm_info->batchnum) {
            mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev, imgsys_smvr);
        } else {
            mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev, imgsys_streaming);
        }
    }
	#else
	mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev);
	#endif
	ret = imgsys_cmdq_sendtask(imgsys_dev, frm_info, imgsys_mdp_cb_func,
		imgsys_cmdq_timeout_cb_func, mtk_imgsys_get_iova, is_singledev_mode);
	IMGSYS_SYSTRACE_END();
#ifdef REQ_TIMESTAMP
	req->tstate.time_cmqret = ktime_get_boottime_ns()/1000;
	req->tstate.time_sendtask +=
		(req->tstate.time_cmqret - stime);
#endif
    if (imgsys_dbg_enable())
	dev_info(imgsys_dev->dev,
		"req track-%s:(reqfd-%d) send2GCE tfnum/fidx(%d/%d) MWFrame:#%d MWReq:#%d owner:%s\n",
		__func__, req->tstate.req_fd,
		frm_info->total_frmnum,
		frm_info->user_info[0].subfrm_idx,
		frm_info->frame_no, frm_info->request_no,
		((char *)&frm_info->frm_owner));
	if (ret < 0) {
		dev_info(imgsys_dev->dev,
			"%s: imgsys_cmdq_sendtask fail(%d)\n", __func__, ret);
        #if SMVR_DECOUPLE
        	if (frm_info->is_capture) {
        	    mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_capture);
            } else {
                if (frm_info->batchnum) {
                    mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_smvr);
                } else {
                    mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_streaming);
                }
            }
        #else
        	mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev);
        #endif
	}

	if (req_track)
		req_track->subflow_kernel++;

	gce_put_work(work);
	#if SMVR_DECOUPLE
	if (frm_info->is_capture) {
        mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_capture);
    } else {
        if (frm_info->batchnum) {
            mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_smvr);
        } else {
            mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev, imgsys_streaming);
        }
    }
	#else
	mtk_hcp_put_gce_buffer(imgsys_dev->scp_pdev);
	#endif
}

static void imgsys_scp_handler(void *data, unsigned int len, void *priv)
{
	int job_id;
	struct mtk_imgsys_pipe *pipe;
	int pipe_id;
	struct mtk_imgsys_request *req;
	struct mtk_imgsys_dev *imgsys_dev = (struct mtk_imgsys_dev *)priv;
	struct img_sw_buffer *swbuf_data = NULL;
	struct swfrm_info_t *swfrm_info = NULL;
	struct gce_work *gwork;
	int swfrm_cnt;
	u64 time_local_reddonescpStart = 0;
	int i = 0;
	void *gce_virt = NULL;
	struct reqfd_cbinfo_t *cb_info = NULL;
	struct reqfd_cbinfo_t *reqfdcb_info = NULL;
	struct list_head *head = NULL;
	struct list_head *temp = NULL;
	bool reqfd_find = false;
	int f_lop_idx = 0, fence_num = 0;
	struct fence_event *fence_evt = NULL;
	union request_track *req_track = NULL;
	int total_framenum = 0;
#if SMVR_DECOUPLE
unsigned int mode = imgsys_streaming;
#endif
#ifdef MTK_IOVA_SINK2KERNEL
    struct mtk_imgsys_req_fd_list *fd_list = &imgsys_dev->req_fd_cache;
	u32	req_fd = 0;
#endif
    pipe = &imgsys_dev->imgsys_pipe[0];

    if (!pipe->streaming) {
        dev_info(imgsys_dev->dev, "imgsys already streaming off");
        return;
    }

	if (!data) {
		WARN_ONCE(!data, "%s: failed due to NULL data\n", __func__);
		return;
	}

	if (WARN_ONCE(len != sizeof(struct img_sw_buffer),
		      "%s: len(%d) not match img_sw_buffer\n", __func__, len))
		return;

	swbuf_data = (struct img_sw_buffer *)data;
	#if SMVR_DECOUPLE
	switch (swbuf_data->scp_addr) {
        case imgsys_streaming:
        case imgsys_capture:
        case imgsys_smvr:
            mode  = swbuf_data->scp_addr;
            break;
        default:
            dev_warn(imgsys_dev->dev,
                "%s: unexpected mode (%d/%d)\n",
                __func__, swbuf_data->scp_addr, mode);
            break;
    }
    //dev_dbg(imgsys_dev->dev,
    //    "%s: scp_addr/ mode (%d/%d)\n",
    //    __func__, swbuf_data->scp_addr, mode);
    gce_virt = mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev, mode);
	#else
	gce_virt = mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev);
	#endif
	swfrm_info = (struct swfrm_info_t *)(gce_virt + (swbuf_data->offset));
#if SMVR_DECOUPLE
 //dev_info(imgsys_dev->dev,
 //       "%s: swfrm_info/ gce_virt (%p/%p) scp_addr/ mode(%d/%d)\n",
 //       __func__, swfrm_info, gce_virt, swbuf_data->scp_addr, mode);
#endif
	if (!swfrm_info) {
		pr_info("%s: invalid swfrm_info\n", __func__);
		return;
	}

	swfrm_info->req_sbuf_goft = swbuf_data->offset;

#if MTK_CM4_SUPPORT == 0

	#ifdef BATCH_MODE_V3
	// V3 batch mode added {
	if (ipi_param->is_batch_mode) {
		struct mtk_imgsys_hw_subframe *working_buf, *temp_wbuf;
		struct mtk_runner_work *runner_work;

		if (ipi_param->req_addr_va) {
			req =
			(struct mtk_imgsys_request *)ipi_param->req_addr_va;
		} else {
			dev_info(imgsys_dev->dev,
				"%s: fail to get ipi_param.req_addr_va\n",
				__func__);
			return;
		}

		/* fill the frameparam with the returned one */
		list_for_each_entry_safe(working_buf, temp_wbuf,
					 &req->working_buf_list.list,
					 list_entry) {
			if (ipi_param->frm_param_offset ==
				(working_buf->frameparam.scp_daddr -
				imgsys_dev->working_buf_mem_scp_daddr)) {
				/**
				 * make use of the FIFO list, move the hw_buf
				 * into the scp_done_list, and trigger the
				 * imgsys_runner_func to handle it.
				 */

				spin_lock(&req->working_buf_list.lock);
				list_del(&working_buf->list_entry);
				req->working_buf_list.cnt--;
				spin_unlock(&req->working_buf_list.lock);

				spin_lock(&req->scp_done_list.lock);
				list_add_tail(&working_buf->list_entry,
					      &req->scp_done_list.list);
				req->scp_done_list.cnt++;
				spin_unlock(&req->scp_done_list.lock);

				break;
			}

			dev_info(imgsys_dev->dev,
				"%s: Fail to find working buffer, offset: 0x%llx\n",
				__func__,
				ipi_param->frm_param_offset);
			return;
		}

		frameparam = (struct img_ipi_frameparam *)
						working_buf->frameparam.vaddr;
		if (!frameparam->dip_param.next_frame)
			num = atomic_dec_return(&imgsys_dev->num_composing);
		up(&imgsys_dev->sem);
        if (imgsys_dbg_enable())
		dev_dbg(imgsys_dev->dev,
			 "%s: frame_no(%d) back, idx(%d), composing num(%d)\n",
			 __func__, frameparam->frame_no, frameparam->index,
			 num);
		runner_work = vzalloc(sizeof(struct mtk_mdpcb_work));
		runner_work->req = req;
		INIT_WORK(&runner_work->frame_work, dip_runner_func_batch);
		queue_work(imgsys_dev->mdp_wq, &runner_work->frame_work);
        if (imgsys_dbg_enable())
		dev_dbg(imgsys_dev->dev,
			 "%s return batch_mode(-)\n", __func__);
		return;
	}
	// } V3 batch mode added
	#endif

#endif

#ifdef REQ_TIMESTAMP
	time_local_reddonescpStart = ktime_get_boottime_ns()/1000;
#endif

#ifndef MTK_IOVA_SINK2KERNEL
	job_id = swfrm_info->handle;
#else
	req_fd = (u32) swfrm_info->request_fd;
	job_id = fd_list->info_array[req_fd].handle;
#endif
	pipe_id = mtk_imgsys_pipe_get_pipe_from_job_id(job_id);
	pipe = mtk_imgsys_dev_get_pipe(imgsys_dev, pipe_id);
	if (!pipe) {
		dev_info(imgsys_dev->dev,
			 "%s: get invalid img_ipi_frameparam index(%d) from firmware\n",
			 __func__, job_id);
		return;
	}

#ifndef MTK_IOVA_SINK2KERNEL
	req = (struct mtk_imgsys_request *) swfrm_info->req_vaddr;
#else
	req = (struct mtk_imgsys_request *)fd_list->info_array[req_fd].req_addr_va;
#endif
	if (!req) {
		WARN_ONCE(!req, "%s: frame_no(%d) is lost\n", __func__, job_id);
		return;
	}

	if (swfrm_info->is_lastfrm) {
        req_track = (union request_track *)req->req_stat;
		req_track->mainflow_to = REQUEST_FROM_DAEMON_TO_KERNEL;
		req_track->subflow_kernel++;
	}

	IMGSYS_SYSTRACE_BEGIN("MWFrame:#%d MWReq:#%d ReqFd:%d owner:%s subfrm_idx:%d\n",
			swfrm_info->frame_no, swfrm_info->request_no, req->tstate.req_fd,
		((char *)&swfrm_info->frm_owner), swfrm_info->user_info[0].subfrm_idx);

	if (!swfrm_info->user_info[0].subfrm_idx)
		req->tstate.time_reddonescpStart = time_local_reddonescpStart;

	swfrm_cnt = atomic_inc_return(&req->swfrm_cnt);
	if (swfrm_cnt == 1) {
        if (imgsys_dbg_enable())
		dev_dbg(imgsys_dev->dev,
		"%d:%d:%s: request num(%d)/frame no(%d), request fd(%d) kva(0x%lx) tfnum(%d) sidx(%d)\n",
		current->pid, current->tgid, __func__, swfrm_info->request_no,
		swfrm_info->frame_no, swfrm_info->request_fd, (unsigned long)swfrm_info,
		swfrm_info->total_frmnum, swfrm_info->user_info[0].subfrm_idx);
	}

	/* up(&imgsys_dev->sem);*/
	/* TODO: log only safe to remove */
	if (!req->working_buf) {
        if (imgsys_dbg_enable())
		dev_dbg(imgsys_dev->dev,
			"%s: (reqfd-%d) composing\n",
			__func__, req->tstate.req_fd);
	}

#ifdef REQ_TIMESTAMP
	req->tstate.time_doframeinfo = ktime_get_boottime_ns()/1000;
#endif
	/**/
	swfrm_info->is_sent = false;
	swfrm_info->req = (void *)req;
	swfrm_info->pipe = (void *)pipe;
	swfrm_info->cb_frmcnt = 0;
	swfrm_info->total_taskcnt = 0;
	swfrm_info->chan_id = 0;
	swfrm_info->fail_isHWhang = -1;
	total_framenum = swfrm_info->total_frmnum;
	if (swfrm_info->batchnum > 0) {
		if ((total_framenum < 0) || (total_framenum > TIME_MAX)) {
			dev_info(imgsys_dev->dev,
				"%s:unexpected total_framenum (%d -> %d), batchnum(%d) MAX (%d/%d)\n",
				__func__, swfrm_info->total_frmnum,
				total_framenum,
				swfrm_info->batchnum,
				TMAX, TIME_MAX);
			return;
		}
	} else {
		if ((total_framenum < 0) || (total_framenum > TMAX)) {
			dev_info(imgsys_dev->dev,
				"%s:unexpected total_framenum (%d -> %d), batchnum(%d) MAX (%d/%d)\n",
				__func__, swfrm_info->total_frmnum,
				total_framenum,
				swfrm_info->batchnum,
				TMAX, TIME_MAX);
			return;
		}
	}
	for (i = 0 ; i < swfrm_info->total_frmnum ; i++) {
		swfrm_info->user_info[i].g_swbuf = gce_virt + (swfrm_info->user_info[i].sw_goft);
		swfrm_info->user_info[i].bw_swbuf = gce_virt + (swfrm_info->user_info[i].sw_bwoft);

		//K Fence - notify
		fence_num = swfrm_info->user_info[i].notify_fence_num;
		if (fence_num > KFENCE_MAX) {
			dev_info(imgsys_dev->dev,
				"%s: [ERROR][Frm%d-notify]fence number(%d) > %d\n",
				__func__, i, fence_num, KFENCE_MAX);
			fence_num = KFENCE_MAX;
		}
		for (f_lop_idx = 0; f_lop_idx < fence_num; f_lop_idx++) {
			fence_evt = &(swfrm_info->user_info[i].notify_fence_list[f_lop_idx]);
			if (fence_evt->fence_fd == 0) {
				dev_info(imgsys_dev->dev,
					"%s: [ERROR] Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d:[Frm%d-n#%d] fd=0\n",
					__func__, (char *)(&(swfrm_info->frm_owner)),
					swfrm_info->frame_no, swfrm_info->request_no,
					swfrm_info->request_fd, i, f_lop_idx);
				continue;
			}
			fence_evt->dma_fence = (uint64_t *) sync_file_get_fence(
									fence_evt->fence_fd);
			if (fence_evt->dma_fence == NULL) {
				dev_info(imgsys_dev->dev,
					"%s: [ERROR] Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d:[Frm%d-n#%d]invalid fd(%d)\n",
					__func__, (char *)(&(swfrm_info->frm_owner)),
					swfrm_info->frame_no, swfrm_info->request_no,
					swfrm_info->request_fd, i, f_lop_idx, fence_evt->fence_fd);
				continue;
			}
            if (imgsys_dbg_enable())
			dev_dbg(imgsys_dev->dev,
				"%s: Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d:[Frm%d-n#%d]0x%lx,fencefd:%d\n",
				__func__, (char *)(&(swfrm_info->frm_owner)),
				swfrm_info->frame_no, swfrm_info->request_no,
				swfrm_info->request_fd, i, f_lop_idx, (unsigned long)fence_evt->dma_fence,
				fence_evt->fence_fd);//dbg
		}
		//K Fence - wait
		fence_num = swfrm_info->user_info[i].wait_fence_num;
		if (fence_num > KFENCE_MAX) {
			dev_info(imgsys_dev->dev,
				"%s: [ERROR][Frm%d-wait]fence number(%d) > %d\n",
				__func__, i, fence_num, KFENCE_MAX);
			fence_num = KFENCE_MAX;
		}
		for (f_lop_idx = 0; f_lop_idx < fence_num; f_lop_idx++) {
			fence_evt = &(swfrm_info->user_info[i].wait_fence_list[f_lop_idx]);
			if (fence_evt->fence_fd == 0) {
				dev_info(imgsys_dev->dev,
					"%s: [ERROR] Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d:[Frm%d-w#%d] fd=0\n",
					__func__, (char *)(&(swfrm_info->frm_owner)),
					swfrm_info->frame_no, swfrm_info->request_no,
					swfrm_info->request_fd, i, f_lop_idx);
				continue;
			}
			fence_evt->dma_fence = (uint64_t *) sync_file_get_fence(
									fence_evt->fence_fd);
			if (fence_evt->dma_fence == NULL) {
				dev_info(imgsys_dev->dev,
					"%s: [ERROR] Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d:[Frm%d-w#%d]invalid fd(%d)\n",
					__func__, (char *)(&(swfrm_info->frm_owner)),
					swfrm_info->frame_no, swfrm_info->request_no,
					swfrm_info->request_fd, i, f_lop_idx, fence_evt->fence_fd);
				continue;
			}
            if (imgsys_dbg_enable())
			dev_dbg(imgsys_dev->dev,
				"%s: Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d:[Frm%d-w#%d]0x%lx,fencefd:%d,event:%d\n",
				__func__, (char *)(&(swfrm_info->frm_owner)),
				swfrm_info->frame_no, swfrm_info->request_no,
				swfrm_info->request_fd, i, f_lop_idx, (unsigned long)fence_evt->dma_fence,
				fence_evt->fence_fd, fence_evt->gce_event);//dbg
		}
	}

	/*first group in request*/
	if (!swfrm_info->user_info[0].subfrm_idx) {
#ifdef REQ_TIMESTAMP
		req->tstate.time_qw2runner = ktime_get_boottime_ns()/1000;
#endif
		if (swfrm_info->exp_totalcb_cnt > 0) {
			reqfd_find = false;
			mutex_lock(&(reqfd_cbinfo_list.mymutex));
			list_for_each_safe(head, temp, &(reqfd_cbinfo_list.mylist)) {
				reqfdcb_info = vlist_node_of(head, struct reqfd_cbinfo_t);
				if ((reqfdcb_info->req_fd == swfrm_info->request_fd) &&
					(reqfdcb_info->req_no == swfrm_info->request_no) &&
					(reqfdcb_info->frm_no == swfrm_info->frame_no) &&
					(reqfdcb_info->frm_owner == swfrm_info->frm_owner)) {
					dev_info(imgsys_dev->dev,
					"%s:remaining(%s/%d/%d/%d) in gcecbcnt list. new enque(%s/%d/%d/%d)\n",
					__func__, ((char *)&reqfdcb_info->frm_owner),
					reqfdcb_info->req_fd, reqfdcb_info->req_no,
					reqfdcb_info->frm_no,
					((char *)&swfrm_info->frm_owner), swfrm_info->request_fd,
					swfrm_info->request_no, swfrm_info->frame_no);

					reqfdcb_info->req_fd = swfrm_info->request_fd;
					reqfdcb_info->req_no = swfrm_info->request_no;
					reqfdcb_info->frm_no = swfrm_info->frame_no;
					reqfdcb_info->frm_owner = swfrm_info->frm_owner;
					reqfdcb_info->exp_cnt = swfrm_info->exp_totalcb_cnt;
					reqfdcb_info->cur_cnt = 0;
					reqfd_find = true;
				}
			}
			if (!reqfd_find) {
#ifdef REQFD_CBINFO
				cb_info = vmalloc(
					sizeof(vlist_type(struct reqfd_cbinfo_t)));
#else
				cb_info	= reqfd_cbinfo_get_work(imgsys_dev);
#endif
				if (!cb_info) {
					mutex_unlock(&(reqfd_cbinfo_list.mymutex));
					return;
				}
				INIT_LIST_HEAD(vlist_link(cb_info, struct reqfd_cbinfo_t));
				cb_info->req_fd = swfrm_info->request_fd;
				cb_info->req_no = swfrm_info->request_no;
				cb_info->frm_no = swfrm_info->frame_no;
				cb_info->frm_owner = swfrm_info->frm_owner;
				cb_info->exp_cnt = swfrm_info->exp_totalcb_cnt;
				cb_info->cur_cnt = 0;
				list_add_tail(vlist_link(cb_info, struct reqfd_cbinfo_t),
					&(reqfd_cbinfo_list.mylist));
			}
			mutex_unlock(&(reqfd_cbinfo_list.mymutex));
		}
	}

	gwork = gce_get_work(imgsys_dev);
	if (!gwork) {
		dev_info(imgsys_dev->dev,
		"%s:own(%llx/%s)req fd/no(%d/%d) frame no(%d) group_id(%d): fidx/tfnum(%d/%d) No GceWork max(%d).\n",
		__func__, swfrm_info->frm_owner, ((char *)&swfrm_info->frm_owner),
		swfrm_info->request_fd, swfrm_info->request_no, swfrm_info->frame_no,
		swfrm_info->group_id,
		swfrm_info->user_info[0].subfrm_idx, swfrm_info->total_frmnum,
		GCE_WORK_NR);
		return;
	}
	#if SMVR_DECOUPLE
	if (swfrm_info->is_capture) {
	    mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev, imgsys_capture);
    } else {
        if (swfrm_info->batchnum) {
            mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev, imgsys_smvr);
        } else {
            mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev, imgsys_streaming);
        }
    }
	#else
	mtk_hcp_get_gce_buffer(imgsys_dev->scp_pdev);
	#endif
	gwork->req = req;
	gwork->req_sbuf_kva = (void *)swfrm_info;
	gwork->work.run = imgsys_runner_func;
#ifdef MTK_IOVA_SINK2KERNEL
	if (req_track)
		req_track->subflow_kernel++;
	imgsys_runner_func((void *)(&gwork->work));
#else
	imgsys_queue_add(&imgsys_dev->runnerque, &gwork->work);
	if (req_track)
		req_track->subflow_kernel++;
#endif

	IMGSYS_SYSTRACE_END();

}

static void imgsys_cleartoken_handler(void *data, unsigned int len, void *priv)
{
	struct mtk_imgsys_dev *imgsys_dev = (struct mtk_imgsys_dev *)priv;
	struct img_sw_buffer *swbuf_data = NULL;
	struct cleartoken_info_t *cleartoken_info = NULL;
	int i = 0;
#if SMVR_DECOUPLE
    void *token_virt = NULL;
#else
	void *gce_virt = NULL;
#endif
	int clear_tnum = 0;

	if (!data) {
		WARN_ONCE(!data, "%s: failed due to NULL data\n", __func__);
		return;
	}

	if (WARN_ONCE(len != sizeof(struct img_sw_buffer),
		      "%s: len(%d) not match img_sw_buffer\n", __func__, len))
		return;

	swbuf_data = (struct img_sw_buffer *)data;
	#if SMVR_DECOUPLE
	token_virt = mtk_hcp_get_gce_token_mem_virt(imgsys_dev->scp_pdev, 0);
    cleartoken_info = (struct cleartoken_info_t *)(token_virt + (swbuf_data->offset));
	#else
	gce_virt = mtk_hcp_get_gce_mem_virt(imgsys_dev->scp_pdev);
	cleartoken_info = (struct cleartoken_info_t *)(gce_virt + (swbuf_data->offset));
	#endif


	if (!cleartoken_info) {
		pr_info("%s: invalid swfrm_info\n", __func__);
		return;
	}

	clear_tnum = cleartoken_info->clearnum;
	if (clear_tnum < 0) {
		pr_info("%s: clear_tnum(%d) less than 0\n", __func__, clear_tnum);
		clear_tnum = 0;
	}
	if (clear_tnum > HWTOKEN_MAX) {
		pr_info("%s: clear_tnum(%d) exceed HWTOKEN_MAX(%d)\n", __func__,
			clear_tnum, HWTOKEN_MAX);
		clear_tnum = HWTOKEN_MAX;
	}
	for (i = 0 ; i < clear_tnum ; i++) {
		dev_info(imgsys_dev->dev,
			"%s:force clear swevent(%d).\n",
			__func__, cleartoken_info->token[i]);
		imgsys_cmdq_clearevent(imgsys_dev, cleartoken_info->token[i]);
	}
}

static void imgsys_aee_handler(void *data, unsigned int len, void *priv)
{
    /*TODO: how to notify imgstream? */
}


static void imgsys_set_smvr(struct mtk_imgsys_request *req,
					struct img_ipi_param *ipi)
{
	if (!req || !ipi)
		return;

	ipi->smvr_mode = mtk_imgsys_is_smvr(req);
}

static void imgsys_sd_share_buffer(struct mtk_imgsys_request *req,
					struct img_ipi_param *ipi_param)
{
	struct mtk_imgsys_dev_buffer *buf_in;
	int b;

	b = is_singledev_mode(req);
	buf_in = req->buf_map[b];
	ipi_param->frm_param.fd = buf_in->vbb.vb2_buf.planes[0].m.fd;
	ipi_param->frm_param.offset = buf_in->dataofst;
}

static void imgsys_composer_workfunc(struct work_struct *work)
{
	struct mtk_imgsys_request *req = mtk_imgsys_hw_fw_work_to_req(work);
	struct mtk_imgsys_dev *imgsys_dev = req->imgsys_pipe->imgsys_dev;
	struct img_ipi_param ipi_param;
	struct mtk_imgsys_hw_subframe *buf;
	int ret = 0;
	u32 index, frame_no;
#ifdef MTK_IOVA_SINK2KERNEL
	struct mtk_imgsys_req_fd_list *fd_list = &imgsys_dev->req_fd_cache;
#endif

	IMGSYS_SYSTRACE_BEGIN("ReqFd:%d\n", req->tstate.req_fd);

#ifdef REQ_TIMESTAMP
	req->tstate.time_compfuncStart = ktime_get_boottime_ns()/1000;
#endif
    if (imgsys_dbg_enable())
	dev_dbg(imgsys_dev->dev,
		"%s:(reqfd-%d) to send frame_no(%d)\n",
		__func__, req->tstate.req_fd,
		req->img_fparam.frameparam.frame_no);
	media_request_get(&req->req);
	/* down(&imgsys_dev->sem);*/
#if MTK_CM4_SUPPORT == 0

	#ifdef BATCH_MODE_V3
	// V3 batch mode added {
	if (is_batch_mode(req)) {
		ipi_param.usage = IMG_IPI_FRAME;
		ipi_param.frm_param.handle = req->id;

		/* daemon use offset to access frameparam*/
		ipi_param.frm_param_offset =
					req->img_fparam.frameparam.self_data.pa;
		ipi_param.req_addr_va = (u64)req;
		ipi_param.is_batch_mode = 1;
		ipi_param.num_frames = (u8)req->unprocessed_count;

		mutex_lock(&imgsys_dev->hw_op_lock);
		atomic_inc(&imgsys_dev->num_composing);

		ret = imgsys_send(imgsys_dev->scp_pdev, HCP_DIP_FRAME_ID,
			&ipi_param, sizeof(ipi_param),
			req->tstate.req_fd, 0);

		if (ret)
			dev_info(imgsys_dev->dev,
			"%s: frame_no(%d) send SCP_IPI_DIP_FRAME failed %d\n",
				__func__,
				req->img_fparam.frameparam.frame_no, ret);
		mutex_unlock(&imgsys_dev->hw_op_lock);
		return;
	}
	ipi_param.is_batch_mode = 0;
	// } V3 batch mode added
	#endif

#endif
	/* TODO: */
	buf = req->working_buf;

	mtk_imgsys_wbuf_to_ipi_img_sw_addr(
				&req->img_fparam.frameparam.self_data,
						&buf->frameparam);
	/* TODO: address check LOG. for desc mode only; not sigdev */
	if (buf->frameparam.vaddr >
		(imgsys_dev->working_buf_mem_vaddr +
		imgsys_dev->working_buf_mem_size)) {

		dev_info(imgsys_dev->dev,
		"%s: wakeup frame_no(%d), DIP_FRM_SZ(%d)\n",
		__func__, req->img_fparam.frameparam.frame_no,
		imgsys_dev->working_buf_mem_size);

		dev_info(imgsys_dev->dev,
		"ipi.vaddr(0x%lx), reservedwb.addr(0x%lx), reservedwb.size(%d)\n",
		(unsigned long)buf->frameparam.vaddr,
		(unsigned long)imgsys_dev->working_buf_mem_vaddr,
		imgsys_dev->working_buf_mem_size);
	}

	ipi_param.usage = IMG_IPI_FRAME;
#ifdef MTK_IOVA_SINK2KERNEL
	mutex_lock(&fd_list->lock);
	if ((req->tstate.req_fd > 0) &&
		(req->tstate.req_fd < MTK_REQ_FD_CACHE_ARRAY_MAX)) {
		fd_list->info_array[req->tstate.req_fd].handle = req->id;
		fd_list->info_array[req->tstate.req_fd].req_addr_va = (u64)req;
	}
	mutex_unlock(&fd_list->lock);
	ipi_param.req_addr_va = 0; //tmp for hcp debug flow
#else
	ipi_param.frm_param.handle = req->id;
	ipi_param.req_addr_va = (u64)req;
#endif

	/* FOR DESC and SIGDEV */
	imgsys_set_smvr(req, &ipi_param);

#if MTK_CM4_SUPPORT
	ipi_param.frm_param.scp_addr = (u32)buf->frameparam.scp_daddr;
#else
	/* TODO */
	if (is_singledev_mode(req)) {
		imgsys_sd_share_buffer(req, &ipi_param);
#ifdef USE_KERNEL_ION_BUFFER
		ipi_param.frm_param.offset = (u32)(buf->frameparam.vaddr -
			mtk_hcp_get_reserve_mem_virt(DIP_MEM_FOR_HW_ID));
#endif
	} else
		ipi_param.frm_param.offset = (u32)(buf->frameparam.vaddr -
		#if SMVR_DECOUPLE
			mtk_hcp_get_hwid_mem_virt(imgsys_dev->scp_pdev, 0));
		#else
			mtk_hcp_get_hwid_mem_virt(imgsys_dev->scp_pdev));
		#endif

    if (imgsys_dbg_enable())
	dev_dbg(imgsys_dev->dev, "req-fd:%d, va:0x%lx,sva:0x%lx, offset:%d, fd:%d\n",
		req->tstate.req_fd,
		(unsigned long)buf->frameparam.vaddr,
		#if SMVR_DECOUPLE
		(unsigned long)mtk_hcp_get_hwid_mem_virt(imgsys_dev->scp_pdev, 0),
		#else
		(unsigned long)mtk_hcp_get_hwid_mem_virt(imgsys_dev->scp_pdev),
		#endif
		ipi_param.frm_param.offset, ipi_param.frm_param.fd);
#endif

	mutex_lock(&imgsys_dev->hw_op_lock);
	atomic_inc(&imgsys_dev->num_composing);

#ifdef REQ_TIMESTAMP
	req->tstate.time_ipisendStart = ktime_get_boottime_ns()/1000;
#endif

#ifndef MTK_IOVA_SINK2KERNEL
	ret = imgsys_send(imgsys_dev->scp_pdev, HCP_DIP_FRAME_ID,
		&ipi_param, sizeof(ipi_param),
		req->tstate.req_fd, 0);
#endif

	index = req->img_fparam.frameparam.index;
	frame_no = req->img_fparam.frameparam.frame_no;

	if (ret) {
		dev_info(imgsys_dev->dev,
			"%s: frame_no(%d) send SCP_IPI_DIP_FRAME failed %d\n",
			__func__, req->img_fparam.frameparam.frame_no, ret);
		if (is_singledev_mode(req))
			mtk_imgsys_iova_map_tbl_unmap_sd(req);
		else if (is_desc_mode(req))
			mtk_imgsys_iova_map_tbl_unmap(req);
		mtk_imgsys_pipe_remove_job(req);
		mtk_imgsys_hw_working_buf_free(imgsys_dev,
						req->working_buf, true);
		req->working_buf = NULL;
		mtk_imgsys_pipe_job_finish(req, VB2_BUF_STATE_ERROR);
		wake_up(&imgsys_dev->flushing_waitq);
	}
	mutex_unlock(&imgsys_dev->hw_op_lock);

    if (imgsys_dbg_enable())
	dev_dbg(imgsys_dev->dev, "%s:(reqfd-%d) sent\n", __func__,
							req->tstate.req_fd);

	IMGSYS_SYSTRACE_END();

}

static int mtk_imgsys_hw_flush_pipe_jobs(struct mtk_imgsys_pipe *pipe)
{
	struct mtk_imgsys_request *req;
	struct list_head job_list = LIST_HEAD_INIT(job_list);
	int num;
	int ret;
	int req_id;
	u32 frame_no;
	unsigned long flag;

	spin_lock_irqsave(&pipe->running_job_lock, flag);
	list_splice_init(&pipe->pipe_job_running_list, &job_list);
	pipe->num_jobs = 0;
	spin_unlock_irqrestore(&pipe->running_job_lock, flag);

	ret = wait_event_timeout
		(pipe->imgsys_dev->flushing_waitq,
		 !(num = atomic_read(&pipe->imgsys_dev->num_composing)),
		 msecs_to_jiffies(1000 / 30 * DIP_COMPOSING_MAX_NUM * 3));
	if (!ret && num) {
		dev_info(pipe->imgsys_dev->dev,
			"%s: flushing is aborted, num(%d)\n",
			__func__, num);
		return -EINVAL;
	}

	list_for_each_entry(req, &job_list, list) {
		req_id = req->id;
		frame_no = req->img_fparam.frameparam.frame_no;
		mtk_imgsys_pipe_job_finish(req, VB2_BUF_STATE_ERROR);

		dev_info(pipe->imgsys_dev->dev,
			"%s:%s: not run job, id(%d), no(%d), state(%d), job cnt(%d)\n",
			__func__, pipe->desc->name, req_id,
			frame_no,
			VB2_BUF_STATE_ERROR, pipe->num_jobs);
	}

    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
		"%s: wakeup num(%d)\n", __func__, num);
	return 0;
}

static int mtk_imgsys_power_ctrl_ccu(struct mtk_imgsys_dev *imgsys_dev, int on_off)
{
	int ret = 0;

	if (on_off) {
		if (imgsys_dev->rproc_ccu_handle == NULL) {
			dev_info(imgsys_dev->dev, "CCU handle is NULL\n");
			ret = -EINVAL;
			goto out;
		}

#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
		ret = rproc_bootx(imgsys_dev->rproc_ccu_handle, RPROC_UID_IMG);
#else
		ret = rproc_boot(imgsys_dev->rproc_ccu_handle);
#endif
		if (ret)
			dev_info(imgsys_dev->dev, "boot ccu rproc fail\n");
	} else {
		if (imgsys_dev->rproc_ccu_handle)
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
			rproc_shutdownx(imgsys_dev->rproc_ccu_handle, RPROC_UID_IMG);
#else
			rproc_shutdown(imgsys_dev->rproc_ccu_handle);
#endif
		else
			ret = -EINVAL;
	}

out:
	return ret;
}

static void module_uninit(struct kref *kref)
{
	struct mtk_imgsys_dev *imgsys_dev;
	int i, ret;
	struct mtk_imgsys_dvfs *dvfs_info;

	imgsys_dev = container_of(kref, struct mtk_imgsys_dev, init_kref);
	dvfs_info = &imgsys_dev->dvfs_info;

	for (i = 0; i < (imgsys_dev->modules_num); i++)
		if (imgsys_dev->modules[i].uninit)
			imgsys_dev->modules[i].uninit(imgsys_dev);

	if (IS_ERR_OR_NULL(dvfs_info->reg) || !regulator_is_enabled(dvfs_info->reg)) {
        if (imgsys_dbg_enable())
		dev_dbg(dvfs_info->dev,
			"%s: [ERROR] reg is null or disabled\n", __func__);
	}
	else {
		ret = regulator_disable(dvfs_info->reg);
		if (ret)
			dev_info(imgsys_dev->dev,
				"%s: regulater_enable failed\n", __func__);
	}

	mtk_imgsys_power_ctrl_ccu(imgsys_dev, 0);

	if (IS_ERR_OR_NULL(dvfs_info->mmdvfs_clk)) {
        if (imgsys_dbg_enable())
		dev_dbg(dvfs_info->dev,
			"%s: [ERROR] mmdvfs_clk is null\n", __func__);
	}
	else {
		mtk_mmdvfs_enable_ccu(false, CCU_PWR_USR_IMG);
		mtk_mmdvfs_enable_vcp(false, VCP_PWR_USR_IMG);
	}
}

void mtk_imgsys_mod_put(struct mtk_imgsys_dev *imgsys_dev)
{
	struct kref *kref;

	kref = &imgsys_dev->init_kref;
	kref_put(kref, module_uninit);
}

void mtk_imgsys_mod_get(struct mtk_imgsys_dev *imgsys_dev)
{
	struct kref *kref;

	kref = &imgsys_dev->init_kref;
	kref_get(kref);
}

static int mtk_imgsys_worker_power_on(void *data)
{
	int ret, i;
	struct mtk_imgsys_dev *imgsys_dev = data;
	struct mtk_imgsys_dvfs *dvfs_info = &imgsys_dev->dvfs_info;
    ret = 0;
	i= 0;
	dev_info(imgsys_dev->dev, "%s+", __func__);
		mtk_imgsys_power_ctrl_ccu(imgsys_dev, 1);
	if (IS_ERR_OR_NULL(dvfs_info->mmdvfs_clk)) {
        if (imgsys_dbg_enable())
		dev_dbg(dvfs_info->dev,
			"%s: [ERROR] mmdvfs_clk is null\n", __func__);
	}
	else {
		mtk_mmdvfs_enable_vcp(true, VCP_PWR_USR_IMG);
		mtk_mmdvfs_enable_ccu(true, CCU_PWR_USR_IMG);
	}
	if (IS_ERR_OR_NULL(dvfs_info->reg)) {
        if (imgsys_dbg_enable())
		dev_dbg(dvfs_info->dev,
			"%s: [ERROR] reg is err or null\n", __func__);
	}
	else {
		ret = regulator_enable(dvfs_info->reg);
		if (ret)
			dev_info(imgsys_dev->dev, "%s: failed (%d)", __func__, ret);
	}
	pm_runtime_get_sync(imgsys_dev->dev);
	/*set default value for hw module*/
	for (i = 0; i < (imgsys_dev->modules_num); i++)
		imgsys_dev->modules[i].init(imgsys_dev);
	kref_init(&imgsys_dev->init_kref);

	pm_runtime_put_sync(imgsys_dev->dev);
	if (!imgsys_quick_onoff_enable()) {
		#if DVFS_QOS_READY
		mtk_imgsys_power_ctrl(imgsys_dev, true);
		#else
		pm_runtime_get_sync(imgsys_dev->dev);
		#endif
	} else
		dev_info(imgsys_dev->dev,
			"%s: imgsys_quick_onoff_enable(%d)\n",
			__func__, imgsys_quick_onoff_enable());

	complete(&imgsys_dev->comp);
	dev_info(imgsys_dev->dev, "%s-", __func__);
	return 0;
}

static int mtk_imgsys_worker_hcp_init(struct mtk_imgsys_dev *imgsys_dev)
{
	int ret = 0;
#ifndef USE_KERNEL_ION_BUFFER
	struct buf_va_info_t *buf;
	struct dma_buf *dbuf;
#else
	int fd;
#endif
	unsigned int mode = imgsys_streaming;

#if MTK_CM4_SUPPORT
	struct img_ipi_param ipi_param;

	memset(&ipi_param, 0, sizeof(ipi_param));
	ipi_param.usage = IMG_IPI_INIT;
	scp_ipi_register(imgsys_dev->scp_pdev, SCP_IPI_DIP, imgsys_scp_handler,
		imgsys_dev);
	ret = scp_ipi_send(imgsys_dev->scp_pdev, SCP_IPI_DIP, &ipi_param,
			   sizeof(ipi_param), 200);
#else
	{
		struct img_init_info info;
		struct resource *imgsys_resource = imgsys_dev->imgsys_resource;
		#if SMVR_DECOUPLE
        unsigned int gce_buf_en = 0;
		#endif

		mtk_imgsys_hw_working_buf_pool_reinit(imgsys_dev);
		/* ALLOCATE IMGSYS WORKING BUFFER FIRST */
		#if SMVR_DECOUPLE
		if (imgsys_dev->imgsys_pipe[0].imgsys_user_count == 0)
            gce_buf_en = 1;

        if ((imgsys_dev->imgsys_pipe[0].meminfo.is_capture) &&
            (!imgsys_dev->imgsys_pipe[0].capture_alloc)) {
            mode = imgsys_capture;
            ret = mtk_hcp_allocate_working_buffer(imgsys_dev->scp_pdev, mode, gce_buf_en);
            imgsys_dev->imgsys_pipe[0].capture_alloc++;
            imgsys_dev->imgsys_pipe[0].imgsys_user_count++;
        } else {
            if ((imgsys_dev->imgsys_pipe[0].meminfo.is_smvr) &&
                (!imgsys_dev->imgsys_pipe[0].smvr_alloc)) {
				mode = imgsys_smvr;
                ret = mtk_hcp_allocate_working_buffer(imgsys_dev->scp_pdev, mode, gce_buf_en);
                imgsys_dev->imgsys_pipe[0].smvr_alloc++;
                imgsys_dev->imgsys_pipe[0].imgsys_user_count++;
            } else {
                if(!imgsys_dev->imgsys_pipe[0].streaming_alloc) {
					mode = imgsys_streaming;
					ret = mtk_hcp_allocate_working_buffer(imgsys_dev->scp_pdev, mode, gce_buf_en);
                    imgsys_dev->imgsys_pipe[0].streaming_alloc++;
                    imgsys_dev->imgsys_pipe[0].imgsys_user_count++;
                }
            }
        }
        pr_info("imgsys_fw: cap/smvr(%d/%d)"
            "hw_connect_mode/cap_count/smvr_count/streaming_count/user_count(%d/%d/%d/%d/%d)",
            imgsys_dev->imgsys_pipe[0].meminfo.is_capture,
            imgsys_dev->imgsys_pipe[0].meminfo.is_smvr,
            mode,
            imgsys_dev->imgsys_pipe[0].capture_alloc, imgsys_dev->imgsys_pipe[0].smvr_alloc,
            imgsys_dev->imgsys_pipe[0].streaming_alloc, imgsys_dev->imgsys_pipe[0].imgsys_user_count);
		#else
		mode = imgsys_dev->imgsys_pipe[0].init_info.is_smvr;
		ret = mtk_hcp_allocate_working_buffer(imgsys_dev->scp_pdev, mode);
		#endif
		if (ret) {
            if (imgsys_dbg_enable())
			dev_dbg(imgsys_dev->dev, "%s: mtk_hcp_allocate_working_buffer failed %d\n",
				__func__, ret);
			return -1;
			//goto err_power_off;
		}

		mtk_hcp_purge_msg(imgsys_dev->scp_pdev);

		/* IMGSYS HW INIT */
		memset(&info, 0, sizeof(info));
		info.drv_data = (u64)&imgsys_dev;
		info.header_version = HEADER_VER;
		info.dip_param_size = sizeof(struct dip_param);
		info.param_pack_size = sizeof(struct frame_param_pack);
		info.frameparam_size = sizeof(struct img_ipi_frameparam);
		info.reg_phys_addr = imgsys_resource->start;
		info.reg_range = resource_size(imgsys_resource);
		/* TODO */
#ifdef USE_KERNEL_ION_BUFFER
		fd = hcp_get_ion_buffer_fd(imgsys_dev->scp_pdev,
		IMG_MEM_FOR_HW_ID);
		info.hw_buf_fd = fd;
		info.hw_buf_size =
				mtk_hcp_get_reserve_mem_size(DIP_MEM_FOR_HW_ID);
#else
		buf = get_first_sd_buf();
		if (!buf) {
            if (imgsys_dbg_enable()) {
				pr_debug("%s: no single device buff added\n", __func__);
            }
		} else {
			dbuf = (struct dma_buf *)buf->dma_buf_putkva;
			info.hw_buf_size = dbuf->size;
			info.hw_buf_fd = buf->buf_fd;
		}
#endif
#if SMVR_DECOUPLE
        info.smvr_mode = imgsys_dev->imgsys_pipe[0].meminfo.is_smvr;
		info.is_capture = imgsys_dev->imgsys_pipe[0].meminfo.is_capture;
		mtk_hcp_get_init_info(imgsys_dev->scp_pdev, &info);
		info.sec_tag = imgsys_dev->imgsys_pipe[0].ini_info.sec_tag;
		info.full_wd = imgsys_dev->imgsys_pipe[0].ini_info.sensor.full_wd;
		info.full_ht = imgsys_dev->imgsys_pipe[0].ini_info.sensor.full_ht;
#else
		mtk_hcp_get_init_info(imgsys_dev->scp_pdev, &info);
		info.sec_tag = imgsys_dev->imgsys_pipe[0].init_info.sec_tag;
		info.full_wd = imgsys_dev->imgsys_pipe[0].init_info.sensor.full_wd;
		info.full_ht = imgsys_dev->imgsys_pipe[0].init_info.sensor.full_ht;
		info.smvr_mode = imgsys_dev->imgsys_pipe[0].init_info.is_smvr;
#endif
		ret = imgsys_send(imgsys_dev->scp_pdev, HCP_IMGSYS_INIT_ID,
			(void *)&info, sizeof(info), 0, 1);
	}
#endif

	if (ret) {
        if (imgsys_dbg_enable())
			dev_dbg(imgsys_dev->dev, "%s: send SCP_IPI_DIP_FRAME failed %d\n",
				__func__, ret);
		return ret;
		//goto err_power_off;
	}

	//FD cache
	memset(imgsys_dev->req_fd_cache.info_array, 0,
		sizeof(imgsys_dev->req_fd_cache.info_array));
	mutex_init(&imgsys_dev->req_fd_cache.lock);

	ret = gce_work_pool_init(imgsys_dev);
	if (ret) {
		dev_info(imgsys_dev->dev, "%s: gce work pool allocate failed %d\n",
			__func__, ret);
		return ret;
		//goto err_power_off;
	}

	ret = reqfd_cbinfo_work_pool_init(imgsys_dev);
	if (ret) {
		dev_info(imgsys_dev->dev, "%s: reqafd cbinfo work pool allocate failed %d\n",
			__func__, ret);
		return ret;
		//goto err_power_off;
	}

	imgsys_timeout_idx = 0;
	/* calling cmdq stream on */
	imgsys_cmdq_streamon(imgsys_dev);

	imgsys_queue_init(&imgsys_dev->runnerque, imgsys_dev->dev, "imgsys-cmdq");
	imgsys_queue_enable(&imgsys_dev->runnerque);
	//mtk_hcp_init_KernelFence();

	mtk_hcp_register(imgsys_dev->scp_pdev, HCP_IMGSYS_INIT_ID,
		imgsys_init_handler, "imgsys_init_handler", imgsys_dev);
	mtk_hcp_register(imgsys_dev->scp_pdev, HCP_IMGSYS_FRAME_ID,
		imgsys_scp_handler, "imgsys_scp_handler", imgsys_dev);
	mtk_hcp_register(imgsys_dev->scp_pdev, HCP_IMGSYS_CLEAR_HWTOKEN_ID,
		imgsys_cleartoken_handler, "imgsys_cleartoken_handler", imgsys_dev);
	mtk_hcp_register(imgsys_dev->scp_pdev, HCP_IMGSYS_AEE_DUMP_ID,
		imgsys_aee_handler, "imgsys_aee_handler", imgsys_dev);

	return 0;
}

static int mtk_imgsys_hw_connect(struct mtk_imgsys_dev *imgsys_dev)
{
	int ret = 0;
	u32 user_cnt = 0;
	struct task_struct *power_task;

IMGSYS_SYSTRACE_BEGIN("imgsys_fw-init:\n");
	user_cnt = atomic_read(&imgsys_dev->imgsys_user_cnt);
	if (user_cnt != 0)
		dev_info(imgsys_dev->dev,
			"%s: [ERROR] imgsys user count is not zero(%d)\n",
			__func__, user_cnt);

	atomic_set(&imgsys_dev->imgsys_user_cnt, 0);
#if 1
	init_completion(&imgsys_dev->comp);
	power_task =
		kthread_create(mtk_imgsys_worker_power_on, (void *)imgsys_dev, "imgsys_power_on");
	if (!IS_ERR_OR_NULL(power_task)) {
		sched_set_normal(power_task, -20);
		wake_up_process(power_task);
	} else
		mtk_imgsys_worker_power_on((void *)imgsys_dev);
	ret = mtk_imgsys_worker_hcp_init(imgsys_dev);
	wait_for_completion(&imgsys_dev->comp);

	IMGSYS_SYSTRACE_END();
	if (ret != 0) {
		dev_info(imgsys_dev->dev, "hcp init fail");
		goto err_power_off;
	}

	dev_info(imgsys_dev->dev, "%s-", __func__);
	return 0;
#endif

err_power_off:
	if (!imgsys_quick_onoff_enable()) {
	#if DVFS_QOS_READY
		mtk_imgsys_power_ctrl(imgsys_dev, false);
	#else
		pm_runtime_put_sync(imgsys_dev->dev);
	#endif
	} else
		dev_info(imgsys_dev->dev,
			"%s: imgsys_quick_onoff_enable(%d)\n",
			__func__, imgsys_quick_onoff_enable());

    #if SMVR_DECOUPLE
    imgsys_dev->imgsys_pipe[0].imgsys_user_count = 0;
	ret = mtk_hcp_release_gce_working_buffer(imgsys_dev->scp_pdev);
	if (imgsys_dev->imgsys_pipe[0].capture_alloc != 0) {
	    ret = mtk_hcp_ioc_release_working_buffer(imgsys_dev->scp_pdev, imgsys_capture);
	    imgsys_dev->imgsys_pipe[0].capture_alloc = 0;
	}
	if (imgsys_dev->imgsys_pipe[0].streaming_alloc != 0) {
	    ret = mtk_hcp_ioc_release_working_buffer(imgsys_dev->scp_pdev, imgsys_streaming);
	    imgsys_dev->imgsys_pipe[0].streaming_alloc = 0;
	}
	if (imgsys_dev->imgsys_pipe[0].smvr_alloc != 0) {
	    ret = mtk_hcp_ioc_release_working_buffer(imgsys_dev->scp_pdev, imgsys_smvr);
	    imgsys_dev->imgsys_pipe[0].smvr_alloc = 0;
	}

#else
	ret = mtk_hcp_release_working_buffer(imgsys_dev->scp_pdev);
#endif
	if (ret) {
		dev_info(imgsys_dev->dev,
			"%s: mtk_hcp_release_working_buffer failed(%d)\n",
			__func__, ret);
	}
    mtk_hcp_purge_msg(imgsys_dev->scp_pdev);

	mutex_destroy(&imgsys_dev->req_fd_cache.lock);
	//work_pool_uninit(&imgsys_dev->gwork_pool);
	//work_pool_uninit(&imgsys_dev->reqfd_cbinfo_pool);
	mtk_imgsys_mod_put(imgsys_dev);

	user_cnt = atomic_read(&imgsys_dev->imgsys_user_cnt);
	if (user_cnt != 0)
		dev_info(imgsys_dev->dev,
			"%s: [ERROR] imgsys user count is not yet return to zero(%d)\n",
			__func__, user_cnt);

	return -EBUSY;

}

static void mtk_imgsys_hw_disconnect(struct mtk_imgsys_dev *imgsys_dev)
{
	int ret;
#if MTK_CM4_SUPPORT
	struct img_ipi_param ipi_param;

	ipi_param.usage = IMG_IPI_DEINIT;
	ret = scp_ipi_send(imgsys_dev->imgsys_dev->scp_pdev, SCP_IPI_DIP,
		&ipi_param, sizeof(ipi_param), 0, 0);
	if (ret) {
		dev_info(imgsys_dev->dev,
			"%s: SCP IMG_IPI_DEINIT failed(%d)\n", __func__, ret);
	}

	scp_ipi_unregister(imgsys_dev->scp_pdev, SCP_IPI_DIP);
#else
	struct img_init_info info = {0};
	u32 user_cnt = 0;
	#if SMVR_DECOUPLE
    info.is_capture = 0;
    info.smvr_mode = 0;
imgsys_dev->imgsys_pipe[0].imgsys_user_count = 0;
	#endif

	ret = imgsys_send(imgsys_dev->scp_pdev, HCP_IMGSYS_DEINIT_ID,
			(void *)&info, sizeof(info),
			0, 1);

	mtk_hcp_unregister(imgsys_dev->scp_pdev, HCP_DIP_INIT_ID);
	mtk_hcp_unregister(imgsys_dev->scp_pdev, HCP_DIP_FRAME_ID);
	//mtk_hcp_uninit_KernelFence();

	imgsys_queue_disable(&imgsys_dev->runnerque);

	/* calling cmdq stream off */
	imgsys_cmdq_streamoff(imgsys_dev);

	/* RELEASE IMGSYS WORKING BUFFER FIRST */
#if SMVR_DECOUPLE
ret = mtk_hcp_release_gce_working_buffer(imgsys_dev->scp_pdev);
if (imgsys_dev->imgsys_pipe[0].capture_alloc != 0) {
    mtk_hcp_ioc_release_working_buffer(imgsys_dev->scp_pdev, imgsys_capture);
    imgsys_dev->imgsys_pipe[0].capture_alloc = 0;
}
if (imgsys_dev->imgsys_pipe[0].streaming_alloc != 0) {
    mtk_hcp_ioc_release_working_buffer(imgsys_dev->scp_pdev, imgsys_streaming);
    imgsys_dev->imgsys_pipe[0].streaming_alloc = 0;
}
if (imgsys_dev->imgsys_pipe[0].smvr_alloc != 0) {
    mtk_hcp_ioc_release_working_buffer(imgsys_dev->scp_pdev, imgsys_smvr);
    imgsys_dev->imgsys_pipe[0].smvr_alloc = 0;
}

#else
	ret = mtk_hcp_release_working_buffer(imgsys_dev->scp_pdev);
#endif
	if (ret) {
		dev_info(imgsys_dev->dev,
			"%s: mtk_hcp_release_working_buffer failed(%d)\n",
			__func__, ret);
	}

#ifdef USE_KERNEL_ION_BUFFER
	hcp_close_ion_buffer_fd(imgsys_dev->scp_pdev, IMG_MEM_FOR_HW_ID);
#endif
#endif

	mtk_hcp_purge_msg(imgsys_dev->scp_pdev);

	mutex_destroy(&imgsys_dev->req_fd_cache.lock);
	work_pool_uninit(&imgsys_dev->gwork_pool);
	work_pool_uninit(&imgsys_dev->reqfd_cbinfo_pool);

	if (!imgsys_quick_onoff_enable()) {
		#if DVFS_QOS_READY
		mtk_imgsys_power_ctrl(imgsys_dev, false);
		#else
		pm_runtime_put_sync(imgsys_dev->dev);
		#endif
	} else
		dev_info(imgsys_dev->dev,
			"%s: imgsys_quick_onoff_enable(%d)\n",
			__func__, imgsys_quick_onoff_enable());

	mtk_imgsys_mod_put(imgsys_dev);

	user_cnt = atomic_read(&imgsys_dev->imgsys_user_cnt);
	if (user_cnt != 0)
		dev_info(imgsys_dev->dev,
			"%s: [ERROR] imgsys user count is not yet return to zero(%d)\n",
			__func__, user_cnt);

}

int mtk_imgsys_hw_streamon(struct mtk_imgsys_pipe *pipe)
{
	struct mtk_imgsys_dev *imgsys_dev = pipe->imgsys_dev;
	int count, ret;

	mutex_lock(&imgsys_dev->hw_op_lock);
	if (!imgsys_dev->imgsys_stream_cnt) {
		ret = mtk_imgsys_hw_connect(pipe->imgsys_dev);
		if (ret) {
			dev_info(pipe->imgsys_dev->dev,
				"%s:%s: pipe(%d) connect to dip_hw failed\n",
				__func__, pipe->desc->name, pipe->desc->id);

			mutex_unlock(&imgsys_dev->hw_op_lock);

			return ret;
		}
		INIT_LIST_HEAD(&pipe->pipe_job_running_list);
	}
	count = imgsys_dev->imgsys_stream_cnt++;
	atomic_set(&imgsys_dev->num_composing, 0);
	mutex_unlock(&imgsys_dev->hw_op_lock);

	pipe->streaming = 1;
    if (imgsys_dbg_enable()) {
	dev_info(pipe->imgsys_dev->dev,
		"%s:%s:%s: started stream, id(%d/%d), stream cnt(%d)\n",
		__func__, pipe->nodes->desc->name, pipe->desc->name, pipe->nodes->desc->id,
		pipe->desc->id, count);
	}

	return 0;
}

int mtk_imgsys_hw_streamoff(struct mtk_imgsys_pipe *pipe)
{
	struct mtk_imgsys_dev *imgsys_dev = pipe->imgsys_dev;
	/* struct mtk_imgsys_dma_buf_iova_get_info *iova_info, *tmp; */
	int ret;

	if (pipe->streaming != 0) {
    if (imgsys_dbg_enable())
	dev_dbg(imgsys_dev->dev,
		"%s:%s: streamoff, removing all running jobs\n",
		__func__, pipe->desc->name);

	pipe->streaming = 0;

	ret = mtk_imgsys_hw_flush_pipe_jobs(pipe);
	if (ret != 0) {
		dev_info(imgsys_dev->dev,
			"%s:%s: mtk_imgsys_hw_flush_pipe_jobs, ret(%d)\n",
			__func__, pipe->desc->name, ret);
	}

	/* Check all daemon releasing flow are done */
	ret = wait_event_interruptible_timeout(
			frm_info_waitq, info_list_is_empty(&frm_info_list),
			msecs_to_jiffies(3000));
	if (ret == 0) {
		dev_info(imgsys_dev->dev, "%s timeout still with frm list\n",
			__func__);
		return -EIO;
	} else if (-ERESTARTSYS == ret) {
		dev_info(imgsys_dev->dev, "%s wait for done  interrupted !\n",
			__func__);
		return -ERESTARTSYS;
	}

	/* Stop the hardware if there is no streaming pipe */
	mutex_lock(&imgsys_dev->hw_op_lock);
	imgsys_dev->imgsys_stream_cnt--;
	if (!imgsys_dev->imgsys_stream_cnt) {
		mtk_imgsys_hw_disconnect(imgsys_dev);

		dev_info(imgsys_dev->dev, "%s: dip_hw disconnected, stream cnt(%d)\n",
			__func__, imgsys_dev->imgsys_stream_cnt);

		flush_fd_kva_list(imgsys_dev);
	}
    if (imgsys_dbg_enable())
	dev_dbg(pipe->imgsys_dev->dev,
		"%s:%s: stopped stream id(%d), stream cnt(%d)\n",
		__func__, pipe->desc->name, pipe->desc->id,
					imgsys_dev->imgsys_stream_cnt);

	mutex_unlock(&imgsys_dev->hw_op_lock);
	}
	return 0;
}

static void iova_worker(struct work_struct *work)
{
	struct mtk_imgsys_request *req;
	struct mtk_imgsys_dev *imgsys_dev;
	struct req_frameparam *req_frame;
	struct mtk_imgsys_hw_subframe *buf;
	struct img_ipi_frameparam *param;

	req = container_of(work, struct mtk_imgsys_request, iova_work);

#ifdef REQ_TIMESTAMP
	req->tstate.time_iovaworkp = ktime_get_boottime_ns()/1000;
#endif
	IMGSYS_SYSTRACE_BEGIN("ReqFd:%d\n", req->tstate.req_fd);

	if (is_singledev_mode(req)) {
		/* TODO: zero shared buffer */
		param = req->working_buf->frameparam.vaddr;
		if (param)
			memset(param, 0, sizeof(*param));

		mtk_imgsys_sd_desc_map_iova(req);
	} else if (is_desc_mode(req)) {
		/* TODO: zero shared buffer */
		param = req->working_buf->frameparam.vaddr;
		if (param)
			memset(param, 0, sizeof(*param));

		mtk_imgsys_desc_map_iova(req);
	}

	imgsys_dev = req->imgsys_pipe->imgsys_dev;
	buf = req->working_buf;
	req_frame = &req->img_fparam.frameparam;
	req_frame->state = FRAME_STATE_INIT;
	req_frame->frame_no =
			atomic_inc_return(&imgsys_dev->imgsys_enqueue_cnt);
#ifdef REQ_TIMESTAMP
	req->tstate.time_qw2composer = ktime_get_boottime_ns()/1000;
#endif
	imgsys_composer_workfunc(&req->fw_work);

	IMGSYS_SYSTRACE_END();

}

void mtk_imgsys_hw_enqueue(struct mtk_imgsys_dev *imgsys_dev,
			struct mtk_imgsys_request *req)
{
	struct mtk_imgsys_hw_subframe *buf;

	IMGSYS_SYSTRACE_BEGIN("ReqFd:%d\n", req->tstate.req_fd);

#ifdef REQ_TIMESTAMP
	req->tstate.time_composingStart = ktime_get_boottime_ns()/1000;
#endif
	/* TODO: use user fd + offset */
	buf = mtk_imgsys_hw_working_buf_alloc(req->imgsys_pipe->imgsys_dev);
	if (!buf) {
        if (imgsys_dbg_enable())
		dev_dbg(req->imgsys_pipe->imgsys_dev->dev,
			"%s:%s:req(0x%lx): no free working buffer available\n",
			__func__, req->imgsys_pipe->desc->name, (unsigned long)req);
		return;
	}
	req->working_buf = buf;
#ifndef USE_KERNEL_ION_BUFFER
	buf->frameparam.vaddr = 0;
#endif

	if (is_singledev_mode(req))
		mtk_imgsys_singledevice_ipi_params_config(req);
	else {
#ifdef BATCH_MODE_V3
		if (!is_batch_mode(req))
			mtk_imgsys_std_ipi_params_config(req);
#else
		if (is_desc_mode(req))
			mtk_imgsys_desc_ipi_params_config(req);
		else
			mtk_imgsys_std_ipi_params_config(req);
#endif
	}

#ifdef REQ_TIMESTAMP
	req->tstate.time_composingEnd = ktime_get_boottime_ns()/1000;
#endif
	INIT_WORK(&req->iova_work, iova_worker);
#ifdef MTK_IOVA_SINK2KERNEL
	iova_worker(&req->iova_work);
#else
	queue_work(req->imgsys_pipe->imgsys_dev->enqueue_wq,
			&req->iova_work);
#endif
	IMGSYS_SYSTRACE_END();
}

int mtk_imgsys_can_enqueue(struct mtk_imgsys_dev *imgsys_dev,
	int unprocessedcnt)
{
	int ret = 1;

	spin_lock(&imgsys_dev->imgsys_freebufferlist.lock);
	if ((imgsys_dev->imgsys_freebufferlist.cnt < unprocessedcnt) ||
		(list_empty(&imgsys_dev->imgsys_freebufferlist.list)))
		ret = false;
	spin_unlock(&imgsys_dev->imgsys_freebufferlist.lock);
	return ret;
}

struct mtk_imgsys_hw_subframe*
imgsys_working_buf_alloc_helper(struct mtk_imgsys_dev *imgsys_dev)
{
	return mtk_imgsys_hw_working_buf_alloc(imgsys_dev);
}
