// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Daniel Huang <daniel.huang@mediatek.com>
 *
 */

#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <dt-bindings/interconnect/mtk,mmqos.h>
#include <mt-plat/aee.h>
//#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include <linux/pm_opp.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/mailbox_controller.h>
#include <mtk_imgsys-engine.h>
#include "mtk_imgsys-cmdq.h"
#include "mtk_imgsys-cmdq-plat.h"
#include "mtk_imgsys-cmdq-plat_def.h"
#include "mtk_imgsys-cmdq-ext.h"
#include "mtk_imgsys-cmdq-qof.h"
#include "mtk_imgsys-cmdq-dvfs.h"
#include "mtk_imgsys-cmdq-qos.h"
#include "mtk_imgsys-trace.h"
#include "mtk-interconnect.h"

#if DVFS_QOS_READY
#include "mtk-smi-dbg.h"
#endif
#include "smi.h"

#if IMGSYS_SECURE_ENABLE
#include "cmdq-sec.h"
#include "cmdq-sec-iwc-common.h"
#endif

#define WPE_BWLOG_HW_COMB (IMGSYS_ENG_WPE_TNR | IMGSYS_ENG_DIP)
#define WPE_BWLOG_HW_COMB_ninA (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_PQDIP_A)
#define WPE_BWLOG_HW_COMB_ninB (IMGSYS_ENG_WPE_EIS | IMGSYS_ENG_PQDIP_B)
#define WPE_BWLOG_HW_COMB_ninC (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_TRAW)
#define WPE_BWLOG_HW_COMB_ninD (IMGSYS_ENG_WPE_LITE | IMGSYS_ENG_LTR)

#define IMGSYS_SMIDUMP_QOF_TRAW	(BIT(0))
#define IMGSYS_SMIDUMP_QOF_DIP	(BIT(1))

#if CMDQ_CB_KTHREAD
static struct kthread_worker imgsys_cmdq_worker;
static struct task_struct *imgsys_cmdq_kworker_task;
#else
static struct workqueue_struct *imgsys_cmdq_wq;
#endif
u32 is_stream_off = 0;
#if IMGSYS_SECURE_ENABLE
static u32 is_sec_task_create;
#endif
static struct imgsys_event_history event_hist[IMGSYS_CMDQ_SYNC_POOL_NUM];

#ifdef IMGSYS_CMDQ_CBPARAM_NUM
static struct mtk_imgsys_cb_param g_cb_param[IMGSYS_CMDQ_CBPARAM_NUM];
static u32 g_cb_param_idx;
static struct mutex g_cb_param_lock;
#endif

#ifdef IMGSYS_ME_CHECK_FUNC_EN
static u32 is_me_read_cmd = 0;
static dma_addr_t g_pkt_me_pa = 0;
static u32 *g_pkt_me_va = NULL;
#endif

void imgsys_cmdq_init_plat7sp(struct mtk_imgsys_dev *imgsys_dev, const int nr_imgsys_dev)
{
	struct device *dev = imgsys_dev->dev;
	u32 idx = 0;

	pr_info("%s: +, dev(0x%lx), num(%d)\n", __func__, (unsigned long)dev, nr_imgsys_dev);

	/* Only first user has to do init work queue */
	if (nr_imgsys_dev == 1) {
#if CMDQ_CB_KTHREAD
		kthread_init_worker(&imgsys_cmdq_worker);
		imgsys_cmdq_kworker_task = kthread_run(kthread_worker_fn,
			&imgsys_cmdq_worker, "imgsys-cmdqcb");
		if (IS_ERR(imgsys_cmdq_kworker_task)) {
			dev_info(dev, "%s: failed to start imgsys_cmdqcb kthread worker\n",
				__func__);
			imgsys_cmdq_kworker_task = NULL;
		} else
			sched_set_normal(imgsys_cmdq_kworker_task, -20);
#else
		imgsys_cmdq_wq = alloc_ordered_workqueue("%s",
				__WQ_LEGACY | WQ_MEM_RECLAIM |
				WQ_FREEZABLE | WQ_HIGHPRI,
			"imgsys_cmdq_cb_wq");
		if (!imgsys_cmdq_wq)
			pr_info("%s: Create workquque IMGSYS-CMDQ fail!\n",
				__func__);
#endif
	}

	switch (nr_imgsys_dev) {
	case 1: /* DIP */
		/* request thread by index (in dts) 0 */
		for (idx = 0; idx < IMGSYS_NOR_THD; idx++) {
			imgsys_clt[idx] = cmdq_mbox_create(dev, idx);
			pr_info("%s: cmdq_mbox_create(%d, 0x%lx)\n", __func__, idx, (unsigned long)imgsys_clt[idx]);
		}
		#if IMGSYS_SECURE_ENABLE
		/* request for imgsys secure gce thread */
		for (idx = IMGSYS_NOR_THD + IMGSYS_PWR_THD; idx < (IMGSYS_NOR_THD + IMGSYS_PWR_THD + IMGSYS_SEC_THD); idx++) {
			imgsys_sec_clt[idx-IMGSYS_NOR_THD - IMGSYS_PWR_THD] = cmdq_mbox_create(dev, idx);
			pr_info(
				"%s: cmdq_mbox_create sec_thd(%d, 0x%lx)\n",
				__func__, idx, (unsigned long)imgsys_sec_clt[idx-IMGSYS_NOR_THD - IMGSYS_PWR_THD]);
		}
		#endif
		/* parse hardware event */
		for (idx = 0; idx < IMGSYS_CMDQ_EVENT_MAX; idx++) {
			of_property_read_u16(dev->of_node,
				imgsys_event[idx].dts_name,
				&imgsys_event[idx].event);
			pr_info("%s: event idx %d is (%s, %d)\n", __func__,
				idx, imgsys_event[idx].dts_name,
				imgsys_event[idx].event);
		}
		break;
	default:
		break;
	}

	mtk_imgsys_cmdq_qof_init(imgsys_dev, imgsys_clt[0]);

	mutex_init(&imgsys_dev->dvfs_qos_lock);
	mutex_init(&imgsys_dev->power_ctrl_lock);
#ifdef IMGSYS_CMDQ_CBPARAM_NUM
	mutex_init(&g_cb_param_lock);
#endif
	mutex_init(&imgsys_dev->vss_blk_lock);
	mutex_init(&imgsys_dev->sec_task_lock);
}

void imgsys_cmdq_release_plat7sp(struct mtk_imgsys_dev *imgsys_dev)
{
	u32 idx = 0;

	pr_info("%s: +\n", __func__);

	/* Destroy cmdq client */
	for (idx = 0; idx < IMGSYS_NOR_THD; idx++) {
		cmdq_mbox_destroy(imgsys_clt[idx]);
		imgsys_clt[idx] = NULL;
	}
	#if IMGSYS_SECURE_ENABLE
	for (idx = 0; idx < IMGSYS_SEC_THD; idx++) {
		cmdq_mbox_destroy(imgsys_sec_clt[idx]);
		imgsys_sec_clt[idx] = NULL;
	}
	#endif

	MTK_IMGSYS_QOF_NEED_RUN(imgsys_dev->qof_ver, mtk_imgsys_cmdq_qof_release(imgsys_dev, imgsys_clt[0]));

	/* Release work_quque */
#if CMDQ_CB_KTHREAD
	if (imgsys_cmdq_kworker_task)
		kthread_stop(imgsys_cmdq_kworker_task);
#else
	flush_workqueue(imgsys_cmdq_wq);
	destroy_workqueue(imgsys_cmdq_wq);
	imgsys_cmdq_wq = NULL;
#endif
	mutex_destroy(&imgsys_dev->dvfs_qos_lock);
	mutex_destroy(&imgsys_dev->power_ctrl_lock);
#ifdef IMGSYS_CMDQ_CBPARAM_NUM
	mutex_destroy(&g_cb_param_lock);
#endif
	mutex_destroy(&imgsys_dev->vss_blk_lock);
	mutex_destroy(&imgsys_dev->sec_task_lock);
}

void imgsys_cmdq_streamon_plat7sp(struct mtk_imgsys_dev *imgsys_dev)
{
	u32 idx = 0;

	dev_info(imgsys_dev->dev,
		"%s: cmdq stream on (%d) quick_pwr(%d)\n",
		__func__, is_stream_off, imgsys_quick_onoff_enable_plat7sp());
	is_stream_off = 0;

	cmdq_mbox_enable(imgsys_clt[0]->chan);

	for (idx = IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_START;
		idx <= IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_END; idx++)
		cmdq_clear_event(imgsys_clt[0]->chan, imgsys_event[idx].event);

	cmdq_mbox_disable(imgsys_clt[0]->chan);

	MTK_IMGSYS_QOF_NEED_RUN(imgsys_dev->qof_ver, mtk_imgsys_cmdq_qof_streamon(imgsys_dev));

	memset((void *)event_hist, 0x0,
		sizeof(struct imgsys_event_history)*IMGSYS_CMDQ_SYNC_POOL_NUM);
#if DVFS_QOS_READY
	mtk_imgsys_mmdvfs_reset_plat7sp(imgsys_dev);
	mtk_imgsys_mmqos_reset_plat7sp(imgsys_dev);
	mtk_imgsys_mmqos_monitor_plat7sp(imgsys_dev, SMI_MONITOR_START_STATE);
#endif

#ifdef IMGSYS_CMDQ_CBPARAM_NUM
	memset((void *)g_cb_param, 0x0,
		sizeof(struct mtk_imgsys_cb_param) * IMGSYS_CMDQ_CBPARAM_NUM);
	g_cb_param_idx = -1;
    if (imgsys_cmdq_dbg_enable_plat7sp()) {
	dev_dbg(imgsys_dev->dev,
		"%s: g_cb_param sz: %d * sizeof mtk_imgsys_cb_param %lu\n",
		__func__, IMGSYS_CMDQ_CBPARAM_NUM, sizeof(struct mtk_imgsys_cb_param));
    }
#endif

#ifdef IMGSYS_ME_CHECK_FUNC_EN
	g_pkt_me_va = cmdq_mbox_buf_alloc(imgsys_clt[0], &g_pkt_me_pa);
#endif
}

void imgsys_cmdq_streamoff_plat7sp(struct mtk_imgsys_dev *imgsys_dev)
{
	u32 idx = 0;

	dev_info(imgsys_dev->dev,
		"%s: cmdq stream off (%d) idx(%d) quick_pwr(%d)\n",
		__func__, is_stream_off, idx, imgsys_quick_onoff_enable_plat7sp());
	is_stream_off = 1;

	#if CMDQ_STOP_FUNC
	for (idx = 0; idx < IMGSYS_NOR_THD; idx++) {
		cmdq_mbox_stop(imgsys_clt[idx]);
        if (imgsys_cmdq_dbg_enable_plat7sp()) {
		dev_dbg(imgsys_dev->dev,
			"%s: calling cmdq_mbox_stop(%d, 0x%lx)\n",
			__func__, idx, (unsigned long)imgsys_clt[idx]);
	}
	}
	#endif

	#ifdef IMGSYS_ME_CHECK_FUNC_EN
	cmdq_mbox_buf_free(imgsys_clt[0], g_pkt_me_va, g_pkt_me_pa);
	is_me_read_cmd = 0;
	#endif

	#if IMGSYS_SECURE_ENABLE
	mutex_lock(&(imgsys_dev->sec_task_lock));
	if (is_sec_task_create) {
		cmdq_sec_mbox_stop(imgsys_sec_clt[0]);
		/* cmdq_pkt_destroy(pkt_sec); */
		/* pkt_sec = NULL; */
		is_sec_task_create = 0;
	}
	mutex_unlock(&(imgsys_dev->sec_task_lock));
	#endif

	MTK_IMGSYS_QOF_NEED_RUN(imgsys_dev->qof_ver, mtk_imgsys_cmdq_qof_streamoff(imgsys_dev));
	//cmdq_mbox_disable(imgsys_clt[0]->chan);

	#if DVFS_QOS_READY
	mtk_imgsys_mmdvfs_reset_plat7sp(imgsys_dev);
	mtk_imgsys_mmqos_reset_plat7sp(imgsys_dev);
	mtk_imgsys_mmqos_monitor_plat7sp(imgsys_dev, SMI_MONITOR_STOP_STATE);
	#endif
}

static void imgsys_cmdq_cmd_dump_plat7sp(struct swfrm_info_t *frm_info, u32 frm_idx)
{
	struct GCERecoder *cmd_buf = NULL;
	struct Command *cmd = NULL;
	u32 cmd_num = 0;
	u32 cmd_idx = 0;
	u32 cmd_ofst = 0;

	cmd_buf = (struct GCERecoder *)frm_info->user_info[frm_idx].g_swbuf;
	cmd_num = cmd_buf->curr_length / sizeof(struct Command);
	cmd_ofst = sizeof(struct GCERecoder);

	pr_info(
	"%s: +, req fd/no(%d/%d) frame no(%d) frm(%d/%d), cmd_oft(0x%x/0x%x), cmd_len(%d), num(%d), sz_per_cmd(%lu), frm_blk(%d), hw_comb(0x%x)\n",
		__func__, frm_info->request_fd, frm_info->request_no, frm_info->frame_no,
		frm_idx, frm_info->total_frmnum, cmd_buf->cmd_offset, cmd_ofst,
		cmd_buf->curr_length, cmd_num, sizeof(struct Command), cmd_buf->frame_block,
		frm_info->user_info[frm_idx].hw_comb);

	if (cmd_ofst != cmd_buf->cmd_offset) {
		pr_info("%s: [ERROR]cmd offset is not match (0x%x/0x%x)!\n",
			__func__, cmd_buf->cmd_offset, cmd_ofst);
		return;
	}

	cmd = (struct Command *)((unsigned long)(frm_info->user_info[frm_idx].g_swbuf) +
		(unsigned long)(cmd_buf->cmd_offset));

	for (cmd_idx = 0; cmd_idx < cmd_num; cmd_idx++) {
		switch (cmd[cmd_idx].opcode) {
		case IMGSYS_CMD_READ:
			pr_info(
			"%s: READ with source(0x%08x) target(0x%08x) mask(0x%08x)\n", __func__,
				cmd[cmd_idx].u.source, cmd[cmd_idx].u.target, cmd[cmd_idx].u.mask);
			break;
		case IMGSYS_CMD_WRITE:
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			pr_debug(
			"%s: WRITE with addr(0x%08x) value(0x%08x) mask(0x%08x)\n", __func__,
				cmd[cmd_idx].u.address, cmd[cmd_idx].u.value, cmd[cmd_idx].u.mask);
            }
			break;
#ifdef MTK_IOVA_SINK2KERNEL
		case IMGSYS_CMD_WRITE_FD:
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			pr_debug(
			"%s: WRITE_FD with addr(0x%08x) msb_ofst(0x%08x) fd(0x%08x) ofst(0x%08x) rshift(%d)\n",
				__func__, cmd[cmd_idx].u.dma_addr,
				cmd[cmd_idx].u.dma_addr_msb_ofst,
				cmd[cmd_idx].u.fd, cmd[cmd_idx].u.ofst,
				cmd[cmd_idx].u.right_shift);
            }
			break;
#endif
		case IMGSYS_CMD_POLL:
			pr_info(
			"%s: POLL with addr(0x%08x) value(0x%08x) mask(0x%08x)\n", __func__,
				cmd[cmd_idx].u.address, cmd[cmd_idx].u.value, cmd[cmd_idx].u.mask);
			break;
		case IMGSYS_CMD_WAIT:
			pr_info(
			"%s: WAIT event(%d/%d) action(%d)\n", __func__,
				cmd[cmd_idx].u.event, imgsys_event[cmd[cmd_idx].u.event].event,
				cmd[cmd_idx].u.action);
			break;
		case IMGSYS_CMD_UPDATE:
			pr_info(
			"%s: UPDATE event(%d/%d) action(%d)\n", __func__,
				cmd[cmd_idx].u.event, imgsys_event[cmd[cmd_idx].u.event].event,
				cmd[cmd_idx].u.action);
			break;
		case IMGSYS_CMD_ACQUIRE:
			pr_info(
			"%s: ACQUIRE event(%d/%d) action(%d)\n", __func__,
				cmd[cmd_idx].u.event, imgsys_event[cmd[cmd_idx].u.event].event,
				cmd[cmd_idx].u.action);
			break;
		case IMGSYS_CMD_TIME:
			pr_info("%s: Get cmdq TIME stamp\n", __func__);
		break;
		case IMGSYS_CMD_STOP:
			pr_info("%s: End Of Cmd!\n", __func__);
			break;
		default:
			pr_info("%s: [ERROR]Not Support Cmd(%d)!\n", __func__, cmd[cmd_idx].opcode);
			break;
		}
	}
}

static void imgsys_cmdq_fence_done_plat7sp(struct dma_fence *fence, struct dma_fence_cb *cb)
{

	u32 event_id = 0L, event_val = 0L, thd_idx = 0L;
	struct mtk_imgsys_fence *imgsys_fence = NULL;

	imgsys_fence = container_of(cb, struct mtk_imgsys_fence, cb);
	thd_idx = imgsys_fence->thd_idx;
	event_id = imgsys_fence->event_id;

	if (is_stream_off == 1) {
		pr_info("%s: [ERROR] pipe had been turned off(%d)! with fence:0x%lx,fencefd:%d,gthrd:%d,event:%d\n",
			__func__, is_stream_off, (unsigned long)fence, imgsys_fence->fence_fd,
			thd_idx, event_id);
		return;
	}

	if (imgsys_fence_dbg_enable_plat7sp()) {
		pr_info("%s: SetEvent+ with fence:0x%lx,fencefd:%d,gthrd:%d,event:%d\n",
			__func__, (unsigned long)fence, imgsys_fence->fence_fd, thd_idx,
			event_id);
	}

	cmdq_set_event(imgsys_clt[thd_idx]->chan, imgsys_event[event_id].event);

	event_val = cmdq_get_event(imgsys_clt[thd_idx]->chan, imgsys_event[event_id].event);
	if (imgsys_fence_dbg_enable_plat7sp()) {
		pr_info("%s: SetEvent- with fence:0x%lx,fencefd:%d,gthrd:%d,event:%d,event_val:%d\n",
			__func__, (unsigned long)fence, imgsys_fence->fence_fd, thd_idx,
			event_id, event_val);
	}

}

static void imgsys_cmdq_fence_set_plat7sp(struct mtk_imgsys_dev *imgsys_dev,
					struct swfrm_info_t *frm_info,
					struct mtk_imgsys_cb_param *cb_param,
					u32 frm_idx, u32 thd_idx)
{
	u32 f_lop_idx = 0;
	u32 waitf_num = 0, notif_num = 0;
	struct fence_event *fence_evt = NULL;
	struct mtk_imgsys_fence *imgsys_fence = NULL;

    if (imgsys_cmdq_dbg_enable_plat7sp()) {
	pr_debug("%s: +\n", __func__);
    }

	notif_num = frm_info->user_info[frm_idx].notify_fence_num;
	if (notif_num <= 0)
		goto CHECK_WAIT_FENCE;

	for (f_lop_idx = 0; f_lop_idx < notif_num; f_lop_idx++) {
		fence_evt = &(frm_info->user_info[frm_idx].notify_fence_list[f_lop_idx]);
		imgsys_fence = &(cb_param->notifence[f_lop_idx]);
		imgsys_fence->kfence = (struct dma_fence *)fence_evt->dma_fence;
		if (imgsys_fence->kfence == NULL) {
			dev_info(imgsys_dev->dev,
				"%s: [ERROR] Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d,[notify-#%d/%d] null fence, fd:%d\n",
				__func__, (char *)(&(frm_info->frm_owner)),
				frm_info->frame_no, frm_info->request_no,
				frm_info->request_fd, frm_idx, f_lop_idx,
				fence_evt->fence_fd);
			return;
		}

		imgsys_fence->thd_idx = thd_idx;
		imgsys_fence->event_id = fence_evt->gce_event;
		imgsys_fence->fence_fd = fence_evt->fence_fd;

		if (imgsys_fence_dbg_enable_plat7sp()) {
			dev_info(imgsys_dev->dev,
				"%s: Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d,[notify-#%d/%d]0x%lx,fencefd:%d,gthrd:%d,ref_ct:%d\n",
				__func__, (char *)(&(frm_info->frm_owner)),
				frm_info->frame_no, frm_info->request_no,
				frm_info->request_fd, frm_idx, f_lop_idx,
				(unsigned long)imgsys_fence->kfence, imgsys_fence->fence_fd,
				imgsys_fence->thd_idx,
				atomic_read(&imgsys_fence->kfence->refcount.refcount.refs));
		}
	}

CHECK_WAIT_FENCE:
	waitf_num = frm_info->user_info[frm_idx].wait_fence_num;
	if (waitf_num <= 0)
		return;

	for (f_lop_idx = 0; f_lop_idx < waitf_num; f_lop_idx++) {
		int ret_f = 0;

		fence_evt = &(frm_info->user_info[frm_idx].wait_fence_list[f_lop_idx]);
		imgsys_fence = &(cb_param->waitfence[f_lop_idx]);
		imgsys_fence->kfence = (struct dma_fence *)fence_evt->dma_fence;
		if (imgsys_fence->kfence == NULL) {
			dev_info(imgsys_dev->dev,
				"%s: [ERROR] Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d:[wait-#%d/%d]null fence, fd:%d\n",
				__func__, (char *)(&(frm_info->frm_owner)),
				frm_info->frame_no, frm_info->request_no,
				frm_info->request_fd, frm_idx, f_lop_idx,
				imgsys_fence->fence_fd);
			return;
		}

		imgsys_fence->thd_idx = thd_idx;
		imgsys_fence->event_id = fence_evt->gce_event;
		imgsys_fence->fence_fd = fence_evt->fence_fd;

		ret_f = dma_fence_add_callback(
							imgsys_fence->kfence,
							&(imgsys_fence->cb),
							imgsys_cmdq_fence_done_plat7sp);
		if (ret_f == -ENOENT) {
			if (imgsys_fence_dbg_enable_plat7sp()) {
				dev_info(imgsys_dev->dev,
					"%s: Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d,[wait-#%d/%d]already signaled! 0x%lx,fencefd:%d,gthrd:%d,event:%d,ref_ct:%d\n",
					__func__, (char *)(&(frm_info->frm_owner)),
					frm_info->frame_no, frm_info->request_no,
					frm_info->request_fd, frm_idx, f_lop_idx,
					(unsigned long)imgsys_fence->kfence, imgsys_fence->fence_fd,
					imgsys_fence->thd_idx, imgsys_fence->event_id,
					atomic_read(&imgsys_fence->kfence->refcount.refcount.refs));
			}
			imgsys_cmdq_fence_done_plat7sp(imgsys_fence->kfence, &(imgsys_fence->cb));

		} else if (ret_f != 0) {
			dev_info(imgsys_dev->dev,
				"%s: [ERROR] Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d,[wait-#%d/%d]dma_fence_add_callback errno:%d\n",
				__func__, (char *)(&(frm_info->frm_owner)),
				frm_info->frame_no, frm_info->request_no,
				frm_info->request_fd, frm_idx, f_lop_idx, ret_f);
				return;
		} else {
			if (imgsys_fence_dbg_enable_plat7sp()) {
				dev_info(imgsys_dev->dev,
					"%s: Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d,[wait-#%d/%d]0x%lx,fencefd:%d,gthrd:%d,event:%d,ref_ct:%d\n",
					__func__, (char *)(&(frm_info->frm_owner)),
					frm_info->frame_no, frm_info->request_no,
					frm_info->request_fd, frm_idx, f_lop_idx,
					(unsigned long)imgsys_fence->kfence, imgsys_fence->fence_fd,
					imgsys_fence->thd_idx, imgsys_fence->event_id,
					atomic_read(&imgsys_fence->kfence->refcount.refcount.refs));
			}
		}

	}
}

static void imgsys_cmdq_fence_notify_plat7sp(struct mtk_imgsys_dev *imgsys_dev,
			struct mtk_imgsys_cb_param *cb_param)
{
	u32 frm_idx = 0;
	u32 f_lop_idx = 0, notif_num = 0;
	struct mtk_imgsys_fence *imgsys_fence = NULL;

	if (cb_param->err != 0)
		return;

	frm_idx = cb_param->frm_idx;
	notif_num = cb_param->frm_info->user_info[frm_idx].notify_fence_num;
	if (notif_num <= 0)
		return;

	for (f_lop_idx = 0; f_lop_idx < notif_num; f_lop_idx++) {
		imgsys_fence = &(cb_param->notifence[f_lop_idx]);
		if (imgsys_fence->kfence == NULL) {
			dev_info(imgsys_dev->dev,
				"%s: [ERROR] Own:%s, MWFrame:#%d MWReq:#%d ReqFd:%d,[notify-#%d/%d]null fence, fd:%d\n",
				__func__, (char *)(&(cb_param->frm_info->frm_owner)),
				cb_param->frm_info->frame_no, cb_param->frm_info->request_no,
				cb_param->frm_info->request_fd, frm_idx, f_lop_idx,
				imgsys_fence->fence_fd);
			continue;
		}

		if (imgsys_fence_dbg_enable_plat7sp()) {
			dev_info(imgsys_dev->dev,
				"%s: Own:%s, MWFrame:#%d MWReq:#%d ReqFd:%d,[notify-#%d/%d]+,0x%lx,fencefd:%d,gthrd:%d,ref_ct:%d\n",
				__func__, (char *)(&(cb_param->frm_info->frm_owner)),
				cb_param->frm_info->frame_no, cb_param->frm_info->request_no,
				cb_param->frm_info->request_fd, frm_idx, f_lop_idx,
				(unsigned long)imgsys_fence->kfence, imgsys_fence->fence_fd, imgsys_fence->thd_idx,
				atomic_read(&imgsys_fence->kfence->refcount.refcount.refs));
		}

		dma_fence_signal(imgsys_fence->kfence);
		if (imgsys_fence_dbg_enable_plat7sp()) {
			dev_info(imgsys_dev->dev,
				"%s: Own:%s, MWFrame:#%d MWReq:#%d ReqFd:%d,[notify-#%d/%d]-,0x%lx,fencefd:%d,gthrd:%d,ref_ct:%d\n",
				__func__, (char *)(&(cb_param->frm_info->frm_owner)),
				cb_param->frm_info->frame_no, cb_param->frm_info->request_no,
				cb_param->frm_info->request_fd, frm_idx, f_lop_idx,
				(unsigned long)imgsys_fence->kfence, imgsys_fence->fence_fd, imgsys_fence->thd_idx,
				atomic_read(&imgsys_fence->kfence->refcount.refcount.refs));
		}

	}
}

static void imgsys_cmdq_fence_rmcb_plat7sp(struct mtk_imgsys_dev *imgsys_dev,
				struct mtk_imgsys_cb_param *cb_param)
{
	u32 frm_idx = 0;
	u32 f_lop_idx = 0, waitf_num = 0;
	struct mtk_imgsys_fence *imgsys_fence = NULL;

	frm_idx = cb_param->frm_idx;
	waitf_num = cb_param->frm_info->user_info[frm_idx].wait_fence_num;
	if (waitf_num <= 0)
		return;

	for (f_lop_idx = 0; f_lop_idx < waitf_num; f_lop_idx++) {
		imgsys_fence = &(cb_param->waitfence[f_lop_idx]);
		if (imgsys_fence->kfence == NULL) {
			dev_info(imgsys_dev->dev,
				"%s: [ERROR] Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d:[wait-#%d/%d]null fence, fd:%d\n",
				__func__, (char *)(&(cb_param->frm_info->frm_owner)),
				cb_param->frm_info->frame_no, cb_param->frm_info->request_no,
				cb_param->frm_info->request_fd, frm_idx, f_lop_idx,
				imgsys_fence->fence_fd);
			return;
		}

		dma_fence_remove_callback(
							imgsys_fence->kfence,
							&(imgsys_fence->cb));
		dma_fence_put(imgsys_fence->kfence);

		if (imgsys_fence_dbg_enable_plat7sp()) {
			dev_info(imgsys_dev->dev,
				"%s: Own:%s MWFrame:#%d MWReq:#%d ReqFd:%d,[wait-#%d/%d]0x%lx,fencefd:%d,gthrd:%d,event:%d,ref_ct:%d\n",
				__func__, (char *)(&(cb_param->frm_info->frm_owner)),
				cb_param->frm_info->frame_no, cb_param->frm_info->request_no,
				cb_param->frm_info->request_fd, frm_idx, f_lop_idx,
				(unsigned long)imgsys_fence->kfence, imgsys_fence->fence_fd,
				imgsys_fence->thd_idx, imgsys_fence->event_id,
				atomic_read(&imgsys_fence->kfence->refcount.refcount.refs));
		}
	}

}

#if CMDQ_CB_KTHREAD
static void imgsys_cmdq_cb_work_plat7sp(struct kthread_work *work)
#else
static void imgsys_cmdq_cb_work_plat7sp(struct work_struct *work)
#endif
{
	struct mtk_imgsys_cb_param *cb_param = NULL;
	struct mtk_imgsys_dev *imgsys_dev = NULL;
	u32 hw_comb = 0;
	u32 cb_frm_cnt = 0;
	u64 tsDvfsQosStart = 0, tsDvfsQosEnd = 0;
	int req_fd = 0, req_no = 0, frm_no = 0, ret_sn = 0;
	u32 tsSwEvent = 0, tsHwEvent = 0, tsHw = 0, tsTaskPending = 0;
	u32 tsHwStr = 0, tsHwEnd = 0;
	bool isLastTaskInReq = 0;
	char *wpestr = NULL;
	u32 wpebw_en = imgsys_wpe_bwlog_enable_plat7sp();
	char logBuf_temp[MTK_IMGSYS_LOG_LENGTH];
	u32 idx = 0;
	u32 real_frm_idx = 0;

    if (imgsys_cmdq_dbg_enable_plat7sp()) {
	pr_debug("%s: +\n", __func__);
    }

	cb_param = container_of(work, struct mtk_imgsys_cb_param, cmdq_cb_work);
	cb_param->cmdqTs.tsCmdqCbWorkStart = ktime_get_boottime_ns()/1000;
	imgsys_dev = cb_param->imgsys_dev;


    if (imgsys_cmdq_dbg_enable_plat7sp()) {
	dev_dbg(imgsys_dev->dev,
		"%s: cb(%p) gid(%d) in block(%d/%d) for frm(%d/%d) lst(%d/%d/%d) task(%d/%d/%d) ofst(%lx/%lx/%lx/%lx/%lx)\n",
		__func__, cb_param, cb_param->group_id,
		cb_param->blk_idx,  cb_param->blk_num,
		cb_param->frm_idx, cb_param->frm_num,
		cb_param->isBlkLast, cb_param->isFrmLast, cb_param->isTaskLast,
		cb_param->task_id, cb_param->task_num, cb_param->task_cnt,
		cb_param->pkt_ofst[0], cb_param->pkt_ofst[1], cb_param->pkt_ofst[2],
		cb_param->pkt_ofst[3], cb_param->pkt_ofst[4]);
    }

	imgsys_cmdq_fence_notify_plat7sp(imgsys_dev, cb_param);
	imgsys_cmdq_fence_rmcb_plat7sp(imgsys_dev, cb_param);

#ifndef CONFIG_FPGA_EARLY_PORTING
	mtk_imgsys_power_ctrl_plat7sp(imgsys_dev, false);
#endif

	if (imgsys_cmdq_ts_enable_plat7sp()) {
		for (idx = 0; idx < cb_param->task_cnt; idx++) {
			/* Calculating task timestamp */
			tsSwEvent = cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+1]
					- cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+0];
			tsHwEvent = cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+2]
					- cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+1];
			tsHwStr = cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+2];
			tsHwEnd = cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+3];
			tsHw = cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+3]
				- cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+2];
			tsTaskPending =
			cb_param->taskTs.dma_va[cb_param->taskTs.ofst+cb_param->taskTs.num-1]
			- cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+0];
			CMDQ_TICK_TO_US(tsSwEvent);
			CMDQ_TICK_TO_US(tsHwEvent);
			CMDQ_TICK_TO_US(tsHw);
			CMDQ_TICK_TO_US(tsHwStr);
			CMDQ_TICK_TO_US(tsHwEnd);
			CMDQ_TICK_TO_US(tsTaskPending);
			tsTaskPending =
				(cb_param->cmdqTs.tsCmdqCbStart-cb_param->cmdqTs.tsFlushStart)
				- tsTaskPending;
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			dev_dbg(imgsys_dev->dev,
			"%s: TSus cb(%p) err(%d) frm(%d/%d/%d) hw_comb(0x%x) ts_num(%d) sw_event(%d) hw_event(%d) hw_real(%d) (%d/%d/%d/%d)\n",
				__func__, cb_param, cb_param->err, cb_param->frm_idx,
				cb_param->frm_num, cb_frm_cnt, hw_comb,
				cb_param->taskTs.num, tsSwEvent, tsHwEvent, tsHw,
				cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+0],
				cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+1],
				cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+2],
				cb_param->taskTs.dma_va[cb_param->taskTs.ofst+4*idx+3]
			);
            }
			/* if (imgsys_cmdq_ts_dbg_enable_plat7sp()) { */
				real_frm_idx = cb_param->frm_idx - (cb_param->task_cnt - 1) + idx;
				hw_comb = cb_param->frm_info->user_info[real_frm_idx].hw_comb;
				memset((char *)logBuf_temp, 0x0, MTK_IMGSYS_LOG_LENGTH);
				logBuf_temp[strlen(logBuf_temp)] = '\0';
				ret_sn = snprintf(logBuf_temp, MTK_IMGSYS_LOG_LENGTH,
					"/[%d/%d/%d/%d]hw_comb(0x%x)ts(%d-%d-%d-%d)hw(%d-%d)",
					real_frm_idx, cb_param->frm_num,
					cb_param->blk_idx, cb_param->blk_num,
					hw_comb, tsTaskPending, tsSwEvent, tsHwEvent,
					tsHw, tsHwStr, tsHwEnd);
				if (ret_sn < 0) {
					pr_info("%s: [ERROR] snprintf fail: %d\n",
						__func__, ret_sn);
				}
				strncat(cb_param->frm_info->hw_ts_log, logBuf_temp,
						strlen(logBuf_temp));
			/* } */
		}
	}

	if (cb_param->err != 0)
		pr_info(
			"%s: [ERROR] cb(%p) req fd/no(%d/%d) frame no(%d) error(%d) gid(%d) clt(0x%lx) hw_comb(0x%x) for frm(%d/%d) blk(%d/%d) lst(%d/%d) earlycb(%d) ofst(0x%lx) task(%d/%d/%d) ofst(%lx/%lx/%lx/%lx/%lx)",
			__func__, cb_param, cb_param->req_fd,
			cb_param->req_no, cb_param->frm_no,
			cb_param->err, cb_param->group_id,
			(unsigned long)cb_param->clt, cb_param->hw_comb,
			cb_param->frm_idx, cb_param->frm_num,
			cb_param->blk_idx, cb_param->blk_num,
			cb_param->isBlkLast, cb_param->isFrmLast,
			cb_param->is_earlycb, cb_param->pkt->err_data.offset,
			cb_param->task_id, cb_param->task_num, cb_param->task_cnt,
			cb_param->pkt_ofst[0], cb_param->pkt_ofst[1], cb_param->pkt_ofst[2],
			cb_param->pkt_ofst[3], cb_param->pkt_ofst[4]);
	if (is_stream_off == 1)
		pr_info("%s: [ERROR] cb(%p) pipe already streamoff(%d)!\n",
			__func__, cb_param, is_stream_off);

    if (imgsys_cmdq_dbg_enable_plat7sp()) {
	dev_dbg(imgsys_dev->dev,
		"%s: req fd/no(%d/%d) frame no(%d) cb(%p)frm_info(%p) isBlkLast(%d) isFrmLast(%d) isECB(%d) isGPLast(%d) isGPECB(%d) for frm(%d/%d)\n",
		__func__, cb_param->frm_info->request_fd,
		cb_param->frm_info->request_no, cb_param->frm_info->frame_no,
		cb_param, cb_param->frm_info, cb_param->isBlkLast,
		cb_param->isFrmLast, cb_param->is_earlycb,
		cb_param->frm_info->user_info[cb_param->frm_idx].is_lastingroup,
		cb_param->frm_info->user_info[cb_param->frm_idx].is_earlycb,
		cb_param->frm_idx, cb_param->frm_num);
    }

	req_fd = cb_param->frm_info->request_fd;
	req_no = cb_param->frm_info->request_no;
	frm_no = cb_param->frm_info->frame_no;

	cb_param->frm_info->cb_frmcnt++;
	cb_frm_cnt = cb_param->frm_info->cb_frmcnt;

	if (wpebw_en > 0) {
		for (idx = 0; idx < cb_param->task_cnt; idx++) {
			wpestr = NULL;
			real_frm_idx = cb_param->frm_idx - (cb_param->task_cnt - 1) + idx;
			hw_comb = cb_param->frm_info->user_info[real_frm_idx].hw_comb;
			switch (wpebw_en) {
			case 1:
				if ((hw_comb & WPE_BWLOG_HW_COMB) == WPE_BWLOG_HW_COMB)
					wpestr = "tnr";
				break;
			case 2:
				if (((hw_comb & WPE_BWLOG_HW_COMB_ninA) == WPE_BWLOG_HW_COMB_ninA)
				 || ((hw_comb & WPE_BWLOG_HW_COMB_ninB) == WPE_BWLOG_HW_COMB_ninB))
					wpestr = "eis";
				break;
			case 3:
				if (((hw_comb & WPE_BWLOG_HW_COMB_ninC) == WPE_BWLOG_HW_COMB_ninC)
				 || ((hw_comb & WPE_BWLOG_HW_COMB_ninD) == WPE_BWLOG_HW_COMB_ninD))
					wpestr = "lite";
				break;
			}
			if (wpestr) {
				dev_info(imgsys_dev->dev,
					"%s: wpe_bwlog req fd/no(%d/%d)frameNo(%d)cb(%p)err(%d)frm(%d/%d/%d)hw_comb(0x%x)read_num(%d)-%s(%d/%d/%d/%d)\n",
					__func__, cb_param->frm_info->request_fd,
					cb_param->frm_info->request_no,
					cb_param->frm_info->frame_no,
					cb_param, cb_param->err, real_frm_idx,
					cb_param->frm_num, cb_frm_cnt, hw_comb,
					cb_param->taskTs.num,
					wpestr,
					cb_param->taskTs.dma_va[cb_param->taskTs.ofst+0],
					cb_param->taskTs.dma_va[cb_param->taskTs.ofst+1],
					cb_param->taskTs.dma_va[cb_param->taskTs.ofst+2],
					cb_param->taskTs.dma_va[cb_param->taskTs.ofst+3]
					);
			}
		}
	}

	hw_comb = cb_param->frm_info->user_info[cb_param->frm_idx].hw_comb;

    if (imgsys_cmdq_dbg_enable_plat7sp()) {
	dev_dbg(imgsys_dev->dev,
		"%s: req fd/no(%d/%d) frame no(%d) cb(%p)frm_info(%p) isBlkLast(%d) cb_param->frm_num(%d) cb_frm_cnt(%d)\n",
		__func__, cb_param->frm_info->request_fd,
		cb_param->frm_info->request_no, cb_param->frm_info->frame_no,
		cb_param, cb_param->frm_info, cb_param->isBlkLast, cb_param->frm_num,
		cb_frm_cnt);
    }

	/* For ME hang flow */
#ifdef IMGSYS_ME_CHECK_FUNC_EN
	if ((cb_param->err == -800) && (cb_param->hw_comb == 0x800)) {
		pr_info(
			"%s: [ERROR] ME HW timeout! wfe(%d) event(%d)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event);
		cmdq_pkt_dump_buf(cb_param->pkt, 0);
		mtk_smi_dbg_hang_detect("IMGSYS_ME");
		imgsys_cmdq_cmd_dump_plat7sp(cb_param->frm_info, cb_param->frm_idx);
		if (cb_param->user_cmdq_err_cb) {
			struct cmdq_cb_data user_cb_data;

			user_cb_data.err = cb_param->err;
			user_cb_data.data = (void *)cb_param->frm_info;
			cb_param->user_cmdq_err_cb(
				user_cb_data, cb_param->frm_idx, 1, 0);
		}
	}
#endif

	if (cb_param->isBlkLast && cb_param->user_cmdq_cb &&
		((cb_param->frm_info->total_taskcnt == cb_frm_cnt) || cb_param->is_earlycb)) {
		struct cmdq_cb_data user_cb_data;

		/* PMQOS API */
		tsDvfsQosStart = ktime_get_boottime_ns()/1000;
		IMGSYS_CMDQ_SYSTRACE_BEGIN(
			"%s_%s|Imgsys MWFrame:#%d MWReq:#%d ReqFd:%d Own:%llx",
			__func__, "dvfs_qos", cb_param->frm_info->frame_no,
			cb_param->frm_info->request_no, cb_param->frm_info->request_fd,
			cb_param->frm_info->frm_owner);
		/* Calling PMQOS API if last frame */
		if (cb_param->frm_info->total_taskcnt == cb_frm_cnt) {
			mutex_lock(&(imgsys_dev->dvfs_qos_lock));
			#if DVFS_QOS_READY
			mtk_imgsys_mmdvfs_mmqos_cal_plat7sp(imgsys_dev, cb_param->frm_info, 0);
			mtk_imgsys_mmdvfs_set_plat7sp(imgsys_dev, cb_param->frm_info, 0);
			#endif
			mutex_unlock(&(imgsys_dev->dvfs_qos_lock));
			if (imgsys_cmdq_ts_enable_plat7sp() || imgsys_wpe_bwlog_enable_plat7sp()) {
				IMGSYS_CMDQ_SYSTRACE_BEGIN(
					"%s_%s|%s",
					__func__, "hw_ts_trace", cb_param->frm_info->hw_ts_log);
				cmdq_mbox_buf_free(cb_param->clt,
					cb_param->taskTs.dma_va, cb_param->taskTs.dma_pa);
				if (imgsys_cmdq_ts_dbg_enable_plat7sp())
					dev_info(imgsys_dev->dev, "%s: %s",
						__func__, cb_param->frm_info->hw_ts_log);
				vfree(cb_param->frm_info->hw_ts_log);
				IMGSYS_CMDQ_SYSTRACE_END();
			}
			isLastTaskInReq = 1;
		} else
			isLastTaskInReq = 0;
		IMGSYS_CMDQ_SYSTRACE_END();
		tsDvfsQosEnd = ktime_get_boottime_ns()/1000;

		user_cb_data.err = cb_param->err;
		user_cb_data.data = (void *)cb_param->frm_info;
		cb_param->cmdqTs.tsUserCbStart = ktime_get_boottime_ns()/1000;
		IMGSYS_CMDQ_SYSTRACE_BEGIN(
			"%s_%s|Imgsys MWFrame:#%d MWReq:#%d ReqFd:%d Own:%llx",
			__func__, "user_cb", cb_param->frm_info->frame_no,
			cb_param->frm_info->request_no, cb_param->frm_info->request_fd,
			cb_param->frm_info->frm_owner);
		cb_param->user_cmdq_cb(user_cb_data, cb_param->frm_idx, isLastTaskInReq,
			cb_param->batchnum, cb_param->is_capture);
		IMGSYS_CMDQ_SYSTRACE_END();
		cb_param->cmdqTs.tsUserCbEnd = ktime_get_boottime_ns()/1000;
	}

	IMGSYS_CMDQ_SYSTRACE_BEGIN(
		"%s_%s|Imgsys MWFrame:#%d MWReq:#%d ReqFd:%d fidx:%d hw_comb:0x%x Own:%llx cb:%p thd:%d frm(%d/%d/%d) DvfsSt(%lld) SetCmd(%lld) HW(%lld/%d-%d-%d-%d) Cmdqcb(%lld) WK(%lld) UserCb(%lld) DvfsEnd(%lld)",
		__func__, "wait_pkt", cb_param->frm_info->frame_no,
		cb_param->frm_info->request_no, cb_param->frm_info->request_fd,
		cb_param->frm_info->user_info[cb_param->frm_idx].subfrm_idx, hw_comb,
		cb_param->frm_info->frm_owner, cb_param, cb_param->thd_idx,
		cb_param->frm_idx, cb_param->frm_num, cb_frm_cnt,
		(cb_param->cmdqTs.tsDvfsQosEnd-cb_param->cmdqTs.tsDvfsQosStart),
		(cb_param->cmdqTs.tsFlushStart-cb_param->cmdqTs.tsReqStart),
		(cb_param->cmdqTs.tsCmdqCbStart-cb_param->cmdqTs.tsFlushStart),
		tsTaskPending, tsSwEvent, tsHwEvent, tsHw,
		(cb_param->cmdqTs.tsCmdqCbEnd-cb_param->cmdqTs.tsCmdqCbStart),
		(cb_param->cmdqTs.tsCmdqCbWorkStart-cb_param->cmdqTs.tsCmdqCbEnd),
		(cb_param->cmdqTs.tsUserCbEnd-cb_param->cmdqTs.tsUserCbStart),
		(tsDvfsQosEnd-tsDvfsQosStart));

	cmdq_pkt_wait_complete(cb_param->pkt);
	cmdq_pkt_destroy(cb_param->pkt);
	cb_param->cmdqTs.tsReqEnd = ktime_get_boottime_ns()/1000;
	IMGSYS_CMDQ_SYSTRACE_END();

	if (imgsys_cmdq_ts_dbg_enable_plat7sp())
		dev_dbg(imgsys_dev->dev,
			"%s: TSus req fd/no(%d/%d) frame no(%d) thd(%d) cb(%p) err(%d) frm(%d/%d/%d) hw_comb(0x%x) DvfsSt(%lld) Req(%lld) SetCmd(%lld) HW(%lld/%d-%d-%d-%d) Cmdqcb(%lld) WK(%lld) CmdqCbWk(%lld) UserCb(%lld) DvfsEnd(%lld)\n",
			__func__, req_fd, req_no, frm_no, cb_param->thd_idx,
			cb_param, cb_param->err, cb_param->frm_idx,
			cb_param->frm_num, cb_frm_cnt, hw_comb,
			(cb_param->cmdqTs.tsDvfsQosEnd-cb_param->cmdqTs.tsDvfsQosStart),
			(cb_param->cmdqTs.tsReqEnd-cb_param->cmdqTs.tsReqStart),
			(cb_param->cmdqTs.tsFlushStart-cb_param->cmdqTs.tsReqStart),
			(cb_param->cmdqTs.tsCmdqCbStart-cb_param->cmdqTs.tsFlushStart),
			tsTaskPending, tsSwEvent, tsHwEvent, tsHw,
			(cb_param->cmdqTs.tsCmdqCbEnd-cb_param->cmdqTs.tsCmdqCbStart),
			(cb_param->cmdqTs.tsCmdqCbWorkStart-cb_param->cmdqTs.tsCmdqCbEnd),
			(cb_param->cmdqTs.tsReqEnd-cb_param->cmdqTs.tsCmdqCbWorkStart),
			(cb_param->cmdqTs.tsUserCbEnd-cb_param->cmdqTs.tsUserCbStart),
			(tsDvfsQosEnd-tsDvfsQosStart)
			);
#ifdef IMGSYS_CMDQ_CBPARAM_NUM
	cb_param->isOccupy = false;
#else
	vfree(cb_param);
#endif
}

void imgsys_cmdq_task_cb_plat7sp(struct cmdq_cb_data data)
{
	struct mtk_imgsys_cb_param *cb_param = NULL;
	struct mtk_imgsys_pipe *pipe = NULL;
	struct mtk_imgsys_dev *imgsys_dev = NULL;
	size_t err_ofst;
	u32 idx = 0, err_idx = 0, real_frm_idx = 0;
	u16 event = 0, event_sft = 0;
	u64 event_diff = 0;
	u32 event_val = 0L;
	bool isHWhang = 0;
	bool isQOFhang = 0;
	bool isHwDone = 1;
	bool isGPRtimeout = 0;
#ifdef IMGSYS_ME_CHECK_FUNC_EN
	u32 me_done_reg = 0;
	u32 me_check_reg[4] = {0};
#endif

    if (imgsys_cmdq_dbg_enable_plat7sp()) {
	pr_debug("%s: +\n", __func__);
    }

	if (!data.data) {
		pr_info("%s: [ERROR]no callback data\n", __func__);
		return;
	}

	cb_param = (struct mtk_imgsys_cb_param *)data.data;
	cb_param->err = data.err;
	cb_param->cmdqTs.tsCmdqCbStart = ktime_get_boottime_ns()/1000;
	imgsys_dev = cb_param->imgsys_dev;

    if (imgsys_cmdq_dbg_enable_plat7sp()) {
	pr_debug(
		"%s: Receive cb(%p) with err(%d) for frm(%d/%d)\n",
		__func__, cb_param, data.err, cb_param->frm_idx, cb_param->frm_num);
    }

#ifdef IMGSYS_ME_CHECK_FUNC_EN
	if ((cb_param->err == 0) && (cb_param->hw_comb == 0x800)
		&& (is_me_read_cmd == 1) && (is_stream_off == 0)) {
		me_done_reg = g_pkt_me_va[0];
		for (idx = 0; idx < IMGSYS_ME_CHECK_REG_NUM; idx++)
			me_check_reg[idx] = g_pkt_me_va[idx+1];
		if ((me_done_reg & 0x00000001) == 0) {
			pr_info("%s: [ERROR] ME hang detected! done_reg(0x%x) check_reg(0x%x/0x%x/0x%x/0x%x)\n",
				__func__, me_done_reg, me_check_reg[0], me_check_reg[1],
				me_check_reg[2], me_check_reg[3]);
			if (((me_check_reg[0] & 0x0000000F) == 0x0000000B)
			&& (me_check_reg[1] == 0x00000008)
			&& ((me_check_reg[2] & 0xFF000000) == 0x18000000)
			&& ((me_check_reg[3] & 0xFF000000) == 0x4A000000)) {
				if (imgsys_dev->modules[IMGSYS_MOD_ME].set) {
					imgsys_dev->modules[IMGSYS_MOD_ME].set(imgsys_dev);
					cb_param->frm_info->user_info[cb_param->frm_idx].priv[IMGSYS_ME].need_update_desc = 1;
					pr_info("%s: [WARN] Do ME reset flow\n", __func__);
				} else	/* ME set function is not implement */
					cb_param->err = -800;
			} else	/* ME hang, and can't recover case */ {
				cb_param->err = -800;
				cmdq_thread_dump_spr((struct cmdq_thread *)cb_param->clt->chan->con_priv);
			}
		}
		if (cb_param->err == 0)
			cmdq_clear_event(imgsys_clt[0]->chan,
				imgsys_event[IMGSYS_CMDQ_SYNC_TOKEN_IPESYS_ME].event);
	}
#endif

	if ((cb_param->err != 0) && (cb_param->err != -800)) {
		err_ofst = cb_param->pkt->err_data.offset;
		err_idx = 0;
		for (idx = 0; idx < cb_param->task_cnt; idx++)
			if (err_ofst > cb_param->pkt_ofst[idx])
				err_idx++;
			else
				break;
		if (err_idx >= cb_param->task_cnt) {
			pr_info(
				"%s: [ERROR] can't find task in task list! cb(%p) error(%d)  gid(%d) for frm(%d/%d) blk(%d/%d) ofst(0x%lx) erridx(%d/%d) task(%d/%d/%d) ofst(%lx/%lx/%lx/%lx/%lx)",
				__func__, cb_param, cb_param->err, cb_param->group_id,
				cb_param->frm_idx, cb_param->frm_num,
				cb_param->blk_idx, cb_param->blk_num,
				cb_param->pkt->err_data.offset,
				err_idx, real_frm_idx,
				cb_param->task_id, cb_param->task_num, cb_param->task_cnt,
				cb_param->pkt_ofst[0], cb_param->pkt_ofst[1], cb_param->pkt_ofst[2],
				cb_param->pkt_ofst[3], cb_param->pkt_ofst[4]);
			err_idx = cb_param->task_cnt - 1;
		}
		real_frm_idx = cb_param->frm_idx - (cb_param->task_cnt - 1) + err_idx;
		pr_info(
			"%s: [ERROR] cb(%p) req fd/no(%d/%d) frame no(%d) error(%d) gid(%d) clt(0x%lx) hw_comb(0x%x) for frm(%d/%d) blk(%d/%d) lst(%d/%d/%d) earlycb(%d) ofst(0x%lx) erridx(%d/%d) task(%d/%d/%d) ofst(%lx/%lx/%lx/%lx/%lx)",
			__func__, cb_param, cb_param->req_fd,
			cb_param->req_no, cb_param->frm_no,
			cb_param->err, cb_param->group_id,
			(unsigned long)cb_param->clt, cb_param->hw_comb,
			cb_param->frm_idx, cb_param->frm_num,
			cb_param->blk_idx, cb_param->blk_num,
			cb_param->isBlkLast, cb_param->isFrmLast, cb_param->isTaskLast,
			cb_param->is_earlycb, cb_param->pkt->err_data.offset,
			err_idx, real_frm_idx,
			cb_param->task_id, cb_param->task_num, cb_param->task_cnt,
			cb_param->pkt_ofst[0], cb_param->pkt_ofst[1], cb_param->pkt_ofst[2],
			cb_param->pkt_ofst[3], cb_param->pkt_ofst[4]);
		if (is_stream_off == 1)
			pr_info("%s: [ERROR] cb(%p) pipe had been turned off(%d)!\n",
				__func__, cb_param, is_stream_off);
		pipe = (struct mtk_imgsys_pipe *)cb_param->frm_info->pipe;
		if (!pipe->streaming) {
			/* is_stream_off = 1; */
			pr_info("%s: [ERROR] cb(%p) pipe already streamoff(%d)\n",
				__func__, cb_param, is_stream_off);
		}

		event = cb_param->pkt->err_data.event;
		if ((event >= IMGSYS_CMDQ_HW_TOKEN_BEGIN) &&
			(event <= IMGSYS_CMDQ_HW_TOKEN_END)) {
			pr_info(
				"%s: [ERROR] HW token timeout! wfe(%d) event(%d) isHW(%d)",
				__func__,
				cb_param->pkt->err_data.wfe_timeout,
				cb_param->pkt->err_data.event, isHWhang);
		} else if ((event >= IMGSYS_CMDQ_SW_EVENT1_BEGIN) &&
			(event <= IMGSYS_CMDQ_SW_EVENT1_END)) {
			event_sft = event - IMGSYS_CMDQ_SW_EVENT1_BEGIN;
			event_diff = event_hist[event_sft].set.ts >
						event_hist[event_sft].wait.ts ?
						(event_hist[event_sft].set.ts -
						event_hist[event_sft].wait.ts) :
						(event_hist[event_sft].wait.ts -
						event_hist[event_sft].set.ts);
			pr_info(
				"%s: [ERROR] SW event1 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
				__func__,
				cb_param->pkt->err_data.wfe_timeout,
				cb_param->pkt->err_data.event, isHWhang,
				event_hist[event_sft].st, event_diff,
				event_hist[event_sft].set.req_fd,
				event_hist[event_sft].set.req_no,
				event_hist[event_sft].set.frm_no,
				event_hist[event_sft].set.ts,
				event_hist[event_sft].wait.req_fd,
				event_hist[event_sft].wait.req_no,
				event_hist[event_sft].wait.frm_no,
				event_hist[event_sft].wait.ts);
		} else if ((event >= IMGSYS_CMDQ_SW_EVENT2_BEGIN) &&
			(event <= IMGSYS_CMDQ_SW_EVENT2_END)) {
			event_sft = event - IMGSYS_CMDQ_SW_EVENT2_BEGIN +
				(IMGSYS_CMDQ_SW_EVENT1_END - IMGSYS_CMDQ_SW_EVENT1_BEGIN + 1);
			event_diff = event_hist[event_sft].set.ts >
						event_hist[event_sft].wait.ts ?
						(event_hist[event_sft].set.ts -
						event_hist[event_sft].wait.ts) :
						(event_hist[event_sft].wait.ts -
						event_hist[event_sft].set.ts);
			pr_info(
				"%s: [ERROR] SW event2 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
				__func__,
				cb_param->pkt->err_data.wfe_timeout,
				cb_param->pkt->err_data.event, isHWhang,
				event_hist[event_sft].st, event_diff,
				event_hist[event_sft].set.req_fd,
				event_hist[event_sft].set.req_no,
				event_hist[event_sft].set.frm_no,
				event_hist[event_sft].set.ts,
				event_hist[event_sft].wait.req_fd,
				event_hist[event_sft].wait.req_no,
				event_hist[event_sft].wait.frm_no,
				event_hist[event_sft].wait.ts);
		} else if ((event >= IMGSYS_CMDQ_SW_EVENT3_BEGIN) &&
			(event <= IMGSYS_CMDQ_SW_EVENT3_END)) {
			event_sft = event - IMGSYS_CMDQ_SW_EVENT3_BEGIN +
				(IMGSYS_CMDQ_SW_EVENT2_END - IMGSYS_CMDQ_SW_EVENT2_BEGIN + 1) +
				(IMGSYS_CMDQ_SW_EVENT1_END - IMGSYS_CMDQ_SW_EVENT1_BEGIN + 1);
			event_diff = event_hist[event_sft].set.ts >
						event_hist[event_sft].wait.ts ?
						(event_hist[event_sft].set.ts -
						event_hist[event_sft].wait.ts) :
						(event_hist[event_sft].wait.ts -
						event_hist[event_sft].set.ts);
			pr_info(
				"%s: [ERROR] SW event3 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
				__func__,
				cb_param->pkt->err_data.wfe_timeout,
				cb_param->pkt->err_data.event, isHWhang,
				event_hist[event_sft].st, event_diff,
				event_hist[event_sft].set.req_fd,
				event_hist[event_sft].set.req_no,
				event_hist[event_sft].set.frm_no,
				event_hist[event_sft].set.ts,
				event_hist[event_sft].wait.req_fd,
				event_hist[event_sft].wait.req_no,
				event_hist[event_sft].wait.frm_no,
				event_hist[event_sft].wait.ts);
		} else if ((event >= IMGSYS_CMDQ_SW_EVENT4_BEGIN) &&
			(event <= IMGSYS_CMDQ_SW_EVENT4_END)) {
			event_sft = event - IMGSYS_CMDQ_SW_EVENT4_BEGIN +
				(IMGSYS_CMDQ_SW_EVENT3_END - IMGSYS_CMDQ_SW_EVENT3_BEGIN + 1) +
				(IMGSYS_CMDQ_SW_EVENT2_END - IMGSYS_CMDQ_SW_EVENT2_BEGIN + 1) +
				(IMGSYS_CMDQ_SW_EVENT1_END - IMGSYS_CMDQ_SW_EVENT1_BEGIN + 1);
			event_diff = event_hist[event_sft].set.ts >
						event_hist[event_sft].wait.ts ?
						(event_hist[event_sft].set.ts -
						event_hist[event_sft].wait.ts) :
						(event_hist[event_sft].wait.ts -
						event_hist[event_sft].set.ts);
			pr_info(
				"%s: [ERROR] SW event4 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
				__func__,
				cb_param->pkt->err_data.wfe_timeout,
				cb_param->pkt->err_data.event, isHWhang,
				event_hist[event_sft].st, event_diff,
				event_hist[event_sft].set.req_fd,
				event_hist[event_sft].set.req_no,
				event_hist[event_sft].set.frm_no,
				event_hist[event_sft].set.ts,
				event_hist[event_sft].wait.req_fd,
				event_hist[event_sft].wait.req_no,
				event_hist[event_sft].wait.frm_no,
				event_hist[event_sft].wait.ts);
		} else if ((event >= IMGSYS_CMDQ_SW_EVENT5_BEGIN) &&
			(event <= IMGSYS_CMDQ_SW_EVENT5_END)) {
			event_sft = event - IMGSYS_CMDQ_SW_EVENT5_BEGIN +
				(IMGSYS_CMDQ_SW_EVENT4_END - IMGSYS_CMDQ_SW_EVENT4_BEGIN + 1) +
				(IMGSYS_CMDQ_SW_EVENT3_END - IMGSYS_CMDQ_SW_EVENT3_BEGIN + 1) +
				(IMGSYS_CMDQ_SW_EVENT2_END - IMGSYS_CMDQ_SW_EVENT2_BEGIN + 1) +
				(IMGSYS_CMDQ_SW_EVENT1_END - IMGSYS_CMDQ_SW_EVENT1_BEGIN + 1);
			event_diff = event_hist[event_sft].set.ts >
						event_hist[event_sft].wait.ts ?
						(event_hist[event_sft].set.ts -
						event_hist[event_sft].wait.ts) :
						(event_hist[event_sft].wait.ts -
						event_hist[event_sft].set.ts);
			pr_info(
				"%s: [ERROR] SW event5 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
				__func__,
				cb_param->pkt->err_data.wfe_timeout,
				cb_param->pkt->err_data.event, isHWhang,
				event_hist[event_sft].st, event_diff,
				event_hist[event_sft].set.req_fd,
				event_hist[event_sft].set.req_no,
				event_hist[event_sft].set.frm_no,
				event_hist[event_sft].set.ts,
				event_hist[event_sft].wait.req_fd,
				event_hist[event_sft].wait.req_no,
				event_hist[event_sft].wait.frm_no,
				event_hist[event_sft].wait.ts);
		} else if ((event >= IMGSYS_CMDQ_SW_EVENT6_BEGIN) &&
				(event <= IMGSYS_CMDQ_SW_EVENT6_END)) {
				event_sft = event - IMGSYS_CMDQ_SW_EVENT6_BEGIN +
					(IMGSYS_CMDQ_SW_EVENT5_END - IMGSYS_CMDQ_SW_EVENT5_BEGIN + 1) +
					(IMGSYS_CMDQ_SW_EVENT4_END - IMGSYS_CMDQ_SW_EVENT4_BEGIN + 1) +
					(IMGSYS_CMDQ_SW_EVENT3_END - IMGSYS_CMDQ_SW_EVENT3_BEGIN + 1) +
					(IMGSYS_CMDQ_SW_EVENT2_END - IMGSYS_CMDQ_SW_EVENT2_BEGIN + 1) +
					(IMGSYS_CMDQ_SW_EVENT1_END - IMGSYS_CMDQ_SW_EVENT1_BEGIN + 1);
				event_diff = event_hist[event_sft].set.ts >
							event_hist[event_sft].wait.ts ?
							(event_hist[event_sft].set.ts -
							event_hist[event_sft].wait.ts) :
							(event_hist[event_sft].wait.ts -
							event_hist[event_sft].set.ts);
				pr_info(
					"%s: [ERROR] SW event6 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
					__func__,
					cb_param->pkt->err_data.wfe_timeout,
					cb_param->pkt->err_data.event, isHWhang,
					event_hist[event_sft].st, event_diff,
					event_hist[event_sft].set.req_fd,
					event_hist[event_sft].set.req_no,
					event_hist[event_sft].set.frm_no,
					event_hist[event_sft].set.ts,
					event_hist[event_sft].wait.req_fd,
					event_hist[event_sft].wait.req_no,
					event_hist[event_sft].wait.frm_no,
					event_hist[event_sft].wait.ts);

		} else if ((event >= IMGSYS_CMDQ_QOF_EVENT_BEGIN) &&
			(event <= IMGSYS_CMDQ_QOF_EVENT_END)) {
			dma_addr_t err_pc;
			isQOFhang = 1;
			pr_info(
				"%s: [ERROR] QOF event timeout! wfe(%d) event(%d) isQOF(%d)",
				__func__,
				cb_param->pkt->err_data.wfe_timeout,
				cb_param->pkt->err_data.event, isQOFhang);
			err_pc = cmdq_pkt_get_pa_by_offset(cb_param->pkt, cb_param->pkt->err_data.offset);
			cmdq_pkt_dump_buf(cb_param->pkt, err_pc);
		} else if ((event >= IMGSYS_CMDQ_GPR_EVENT_BEGIN) &&
			(event <= IMGSYS_CMDQ_GPR_EVENT_END)) {
			isHWhang = 1;
			isGPRtimeout = 1;
			pr_info(
				"%s: [ERROR] GPR event timeout! wfe(%d) event(%d) isHW(%d)",
				__func__,
				cb_param->pkt->err_data.wfe_timeout,
				cb_param->pkt->err_data.event, isHWhang);
		} else if ((is_stream_off == 1) && (event == 0)) {
			pr_info(
				"%s: [ERROR] pipe had been turned off(%d)! wfe(%d) event(%d) isHW(%d)",
				__func__, is_stream_off,
				cb_param->pkt->err_data.wfe_timeout,
				cb_param->pkt->err_data.event, isHWhang);
		} else {
			isHWhang = 1;
			event_val = cmdq_get_event(imgsys_clt[0]->chan, event);
			pr_info(
				"%s: [ERROR] HW event timeout! wfe(%d) event(%d/%d) isHW(%d)",
				__func__,
				cb_param->pkt->err_data.wfe_timeout,
				cb_param->pkt->err_data.event, event_val, isHWhang);
		}

		imgsys_cmdq_cmd_dump_plat7sp(cb_param->frm_info, real_frm_idx);

		if (cb_param->user_cmdq_err_cb) {
			struct cmdq_cb_data user_cb_data;

			user_cb_data.err = cb_param->err;
			user_cb_data.data = (void *)cb_param->frm_info;
			cb_param->user_cmdq_err_cb(
				user_cb_data, real_frm_idx, isHWhang,
				event_sft + IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_POOL_START);
			if (isHWhang) {
				event_val = cmdq_get_event(imgsys_clt[0]->chan, event);
				pr_info(
					"%s: [ERROR] HW event after reg dump is (%d/%d)",
					__func__,
					cb_param->pkt->err_data.event, event_val);
			}
		}

		if (isGPRtimeout) {
			if (cb_param->hw_comb & (IMGSYS_ENG_WPE_EIS|IMGSYS_ENG_WPE_TNR|IMGSYS_ENG_WPE_LITE)) {
				idx = IMGSYS_MOD_WPE;
				if (imgsys_dev->modules[idx].done_chk) {
					isHwDone = imgsys_dev->modules[idx].done_chk(imgsys_dev,
						cb_param->hw_comb);
					if(!isHwDone) {
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_WPE",
						"DISPATCH:IMGSYS_WPE poll done fail, hwcomb:0x%x",
						cb_param->hw_comb);
					}
				}
			}
			if (isHwDone && (cb_param->hw_comb & (IMGSYS_ENG_TRAW|IMGSYS_ENG_LTR))) {
				idx = IMGSYS_MOD_TRAW;
				if (imgsys_dev->modules[idx].done_chk) {
					isHwDone = imgsys_dev->modules[idx].done_chk(imgsys_dev,
						cb_param->hw_comb);
					if(!isHwDone) {
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_TRAW",
						"DISPATCH:IMGSYS_TRAW poll done fail, hwcomb:0x%x",
						cb_param->hw_comb);
					}
				}
			}
			if (isHwDone && (cb_param->hw_comb & IMGSYS_ENG_DIP)) {
				idx = IMGSYS_MOD_DIP;
				if (imgsys_dev->modules[idx].done_chk) {
					isHwDone = imgsys_dev->modules[idx].done_chk(imgsys_dev,
						cb_param->hw_comb);
					if(!isHwDone) {
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_DIP",
						"DISPATCH:IMGSYS_DIP poll done fail, hwcomb:0x%x",
						cb_param->hw_comb);
				}
				}
			}
			if (isHwDone && (cb_param->hw_comb & (IMGSYS_ENG_PQDIP_A|IMGSYS_ENG_PQDIP_B))) {
				idx = IMGSYS_MOD_PQDIP;
				if (imgsys_dev->modules[idx].done_chk) {
					isHwDone = imgsys_dev->modules[idx].done_chk(imgsys_dev,
						cb_param->hw_comb);
					if(!isHwDone) {
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_PQDIP",
						"DISPATCH:IMGSYS_PQDIP poll done fail, hwcomb:0x%x",
						cb_param->hw_comb);
					}
				}
			}
			if (isHwDone && (cb_param->hw_comb & IMGSYS_ENG_ME)) {
				idx = IMGSYS_MOD_ME;
				if (imgsys_dev->modules[idx].done_chk) {
					isHwDone = imgsys_dev->modules[idx].done_chk(imgsys_dev,
						cb_param->hw_comb);
					if(!isHwDone) {
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_ME",
							"DISPATCH:IMGSYS_ME poll done fail, hwcomb:0x%x",
							cb_param->hw_comb);
					}
				}
			}
			/* Polling timeout but all modules are done */
			if (isHwDone) {
				aee_kernel_exception("CRDISPATCH_KEY:IMGSYS",
					"DISPATCH:IMGSYS poll done fail, hwcomb:0x%x",
					cb_param->hw_comb);
			}
		}

		if (isHWhang | isQOFhang) {
			if (mtk_imgsys_cmdq_qof_get_pwr_status(ISP7SP_ISP_TRAW))
			mtk_smi_dbg_dump_for_isp_fast(IMGSYS_SMIDUMP_QOF_TRAW);
			if (mtk_imgsys_cmdq_qof_get_pwr_status(ISP7SP_ISP_DIP))
			mtk_smi_dbg_dump_for_isp_fast(IMGSYS_SMIDUMP_QOF_DIP);

			mtk_imgsys_cmdq_qof_dump(cb_param->hw_comb, true);
		}
	}
	cb_param->cmdqTs.tsCmdqCbEnd = ktime_get_boottime_ns()/1000;

#if CMDQ_CB_KTHREAD
	kthread_init_work(&cb_param->cmdq_cb_work, imgsys_cmdq_cb_work_plat7sp);
	kthread_queue_work(&imgsys_cmdq_worker, &cb_param->cmdq_cb_work);
#else
	INIT_WORK(&cb_param->cmdq_cb_work, imgsys_cmdq_cb_work_plat7sp);
	queue_work(imgsys_cmdq_wq, &cb_param->cmdq_cb_work);
#endif
}

int imgsys_cmdq_task_aee_cb_plat7sp(struct cmdq_cb_data data)
{
	struct cmdq_pkt *pkt = NULL;
	struct mtk_imgsys_cb_param *cb_param = NULL;
	struct mtk_imgsys_pipe *pipe = NULL;
	size_t err_ofst;
	u32 idx = 0, err_idx = 0, real_frm_idx = 0;
	u16 event = 0, event_sft = 0;
	u64 event_diff = 0;
	bool isHWhang = 0;
	enum cmdq_aee_type ret = CMDQ_NO_AEE;
	bool isGPRtimeout = 0;

	pkt = (struct cmdq_pkt *)data.data;
	cb_param = pkt->user_priv;

	err_ofst = cb_param->pkt->err_data.offset;
	err_idx = 0;
	for (idx = 0; idx < cb_param->task_cnt; idx++)
		if (err_ofst > cb_param->pkt_ofst[idx])
			err_idx++;
		else
			break;
	if (err_idx >= cb_param->task_cnt) {
		pr_info(
			"%s: [ERROR] can't find task in task list! cb(%p) error(%d)  gid(%d) for frm(%d/%d) blk(%d/%d) ofst(0x%lx) erridx(%d/%d) task(%d/%d/%d) ofst(%lx/%lx/%lx/%lx/%lx)",
			__func__, cb_param, cb_param->err, cb_param->group_id,
			cb_param->frm_idx, cb_param->frm_num,
			cb_param->blk_idx, cb_param->blk_num,
			cb_param->pkt->err_data.offset,
			err_idx, real_frm_idx,
			cb_param->task_id, cb_param->task_num, cb_param->task_cnt,
			cb_param->pkt_ofst[0], cb_param->pkt_ofst[1], cb_param->pkt_ofst[2],
			cb_param->pkt_ofst[3], cb_param->pkt_ofst[4]);
		err_idx = cb_param->task_cnt - 1;
	}
	real_frm_idx = cb_param->frm_idx - (cb_param->task_cnt - 1) + err_idx;
	pr_info(
		"%s: [ERROR] cb(%p) req fd/no(%d/%d) frame no(%d) error(%d) gid(%d) clt(0x%lx) hw_comb(0x%x) for frm(%d/%d) blk(%d/%d) lst(%d/%d/%d) earlycb(%d) ofst(0x%lx) erridx(%d/%d) task(%d/%d/%d) ofst(%lx/%lx/%lx/%lx/%lx)",
		__func__, cb_param, cb_param->req_fd,
		cb_param->req_no, cb_param->frm_no,
		cb_param->err, cb_param->group_id,
		(unsigned long)cb_param->clt, cb_param->hw_comb,
		cb_param->frm_idx, cb_param->frm_num,
		cb_param->blk_idx, cb_param->blk_num,
		cb_param->isBlkLast, cb_param->isFrmLast, cb_param->isTaskLast,
		cb_param->is_earlycb, cb_param->pkt->err_data.offset,
		err_idx, real_frm_idx,
		cb_param->task_id, cb_param->task_num, cb_param->task_cnt,
		cb_param->pkt_ofst[0], cb_param->pkt_ofst[1], cb_param->pkt_ofst[2],
		cb_param->pkt_ofst[3], cb_param->pkt_ofst[4]);
	if (is_stream_off == 1)
		pr_info("%s: [ERROR] cb(%p) pipe had been turned off(%d)!\n",
			__func__, cb_param, is_stream_off);
	pipe = (struct mtk_imgsys_pipe *)cb_param->frm_info->pipe;
	if (!pipe->streaming) {
		/* is_stream_off = 1; */
		pr_info("%s: [ERROR] cb(%p) pipe already streamoff(%d)\n",
			__func__, cb_param, is_stream_off);
	}

	event = cb_param->pkt->err_data.event;
	if ((event >= IMGSYS_CMDQ_HW_TOKEN_BEGIN) &&
		(event <= IMGSYS_CMDQ_HW_TOKEN_END)) {
		ret = CMDQ_NO_AEE;
		pr_info(
			"%s: [ERROR] HW token timeout! wfe(%d) event(%d) isHW(%d)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event, isHWhang);
	} else if ((event >= IMGSYS_CMDQ_SW_EVENT1_BEGIN) &&
		(event <= IMGSYS_CMDQ_SW_EVENT1_END)) {
		event_sft = event - IMGSYS_CMDQ_SW_EVENT1_BEGIN;
		event_diff = event_hist[event_sft].set.ts >
					event_hist[event_sft].wait.ts ?
					(event_hist[event_sft].set.ts -
					event_hist[event_sft].wait.ts) :
					(event_hist[event_sft].wait.ts -
					event_hist[event_sft].set.ts);
		ret = CMDQ_NO_AEE;
		pr_info(
			"%s: [ERROR] SW event1 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event, isHWhang,
			event_hist[event_sft].st, event_diff,
			event_hist[event_sft].set.req_fd,
			event_hist[event_sft].set.req_no,
			event_hist[event_sft].set.frm_no,
			event_hist[event_sft].set.ts,
			event_hist[event_sft].wait.req_fd,
			event_hist[event_sft].wait.req_no,
			event_hist[event_sft].wait.frm_no,
			event_hist[event_sft].wait.ts);
	} else if ((event >= IMGSYS_CMDQ_SW_EVENT2_BEGIN) &&
		(event <= IMGSYS_CMDQ_SW_EVENT2_END)) {
		event_sft = event - IMGSYS_CMDQ_SW_EVENT2_BEGIN +
			(IMGSYS_CMDQ_SW_EVENT1_END - IMGSYS_CMDQ_SW_EVENT1_BEGIN + 1);
		event_diff = event_hist[event_sft].set.ts >
					event_hist[event_sft].wait.ts ?
					(event_hist[event_sft].set.ts -
					event_hist[event_sft].wait.ts) :
					(event_hist[event_sft].wait.ts -
					event_hist[event_sft].set.ts);
		ret = CMDQ_NO_AEE;
		pr_info(
			"%s: [ERROR] SW event2 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event, isHWhang,
			event_hist[event_sft].st, event_diff,
			event_hist[event_sft].set.req_fd,
			event_hist[event_sft].set.req_no,
			event_hist[event_sft].set.frm_no,
			event_hist[event_sft].set.ts,
			event_hist[event_sft].wait.req_fd,
			event_hist[event_sft].wait.req_no,
			event_hist[event_sft].wait.frm_no,
			event_hist[event_sft].wait.ts);
	} else if ((event >= IMGSYS_CMDQ_SW_EVENT3_BEGIN) &&
		(event <= IMGSYS_CMDQ_SW_EVENT3_END)) {
		event_sft = event - IMGSYS_CMDQ_SW_EVENT3_BEGIN +
			(IMGSYS_CMDQ_SW_EVENT2_END - IMGSYS_CMDQ_SW_EVENT2_BEGIN + 1) +
			(IMGSYS_CMDQ_SW_EVENT1_END - IMGSYS_CMDQ_SW_EVENT1_BEGIN + 1);
		event_diff = event_hist[event_sft].set.ts >
					event_hist[event_sft].wait.ts ?
					(event_hist[event_sft].set.ts -
					event_hist[event_sft].wait.ts) :
					(event_hist[event_sft].wait.ts -
					event_hist[event_sft].set.ts);
		ret = CMDQ_NO_AEE;
		pr_info(
			"%s: [ERROR] SW event3 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event, isHWhang,
			event_hist[event_sft].st, event_diff,
			event_hist[event_sft].set.req_fd,
			event_hist[event_sft].set.req_no,
			event_hist[event_sft].set.frm_no,
			event_hist[event_sft].set.ts,
			event_hist[event_sft].wait.req_fd,
			event_hist[event_sft].wait.req_no,
			event_hist[event_sft].wait.frm_no,
			event_hist[event_sft].wait.ts);
	} else if ((event >= IMGSYS_CMDQ_SW_EVENT4_BEGIN) &&
		(event <= IMGSYS_CMDQ_SW_EVENT4_END)) {
		event_sft = event - IMGSYS_CMDQ_SW_EVENT4_BEGIN +
			(IMGSYS_CMDQ_SW_EVENT3_END - IMGSYS_CMDQ_SW_EVENT3_BEGIN + 1) +
			(IMGSYS_CMDQ_SW_EVENT2_END - IMGSYS_CMDQ_SW_EVENT2_BEGIN + 1) +
			(IMGSYS_CMDQ_SW_EVENT1_END - IMGSYS_CMDQ_SW_EVENT1_BEGIN + 1);
		event_diff = event_hist[event_sft].set.ts >
					event_hist[event_sft].wait.ts ?
					(event_hist[event_sft].set.ts -
					event_hist[event_sft].wait.ts) :
					(event_hist[event_sft].wait.ts -
					event_hist[event_sft].set.ts);
		ret = CMDQ_NO_AEE;
		pr_info(
			"%s: [ERROR] SW event4 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event, isHWhang,
			event_hist[event_sft].st, event_diff,
			event_hist[event_sft].set.req_fd,
			event_hist[event_sft].set.req_no,
			event_hist[event_sft].set.frm_no,
			event_hist[event_sft].set.ts,
			event_hist[event_sft].wait.req_fd,
			event_hist[event_sft].wait.req_no,
			event_hist[event_sft].wait.frm_no,
			event_hist[event_sft].wait.ts);
	} else if ((event >= IMGSYS_CMDQ_SW_EVENT5_BEGIN) &&
		(event <= IMGSYS_CMDQ_SW_EVENT5_END)) {
		event_sft = event - IMGSYS_CMDQ_SW_EVENT5_BEGIN +
			(IMGSYS_CMDQ_SW_EVENT4_END - IMGSYS_CMDQ_SW_EVENT4_BEGIN + 1) +
			(IMGSYS_CMDQ_SW_EVENT3_END - IMGSYS_CMDQ_SW_EVENT3_BEGIN + 1) +
			(IMGSYS_CMDQ_SW_EVENT2_END - IMGSYS_CMDQ_SW_EVENT2_BEGIN + 1) +
			(IMGSYS_CMDQ_SW_EVENT1_END - IMGSYS_CMDQ_SW_EVENT1_BEGIN + 1);
		event_diff = event_hist[event_sft].set.ts >
					event_hist[event_sft].wait.ts ?
					(event_hist[event_sft].set.ts -
					event_hist[event_sft].wait.ts) :
					(event_hist[event_sft].wait.ts -
					event_hist[event_sft].set.ts);
		ret = CMDQ_NO_AEE;
		pr_info(
			"%s: [ERROR] SW event5 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event, isHWhang,
			event_hist[event_sft].st, event_diff,
			event_hist[event_sft].set.req_fd,
			event_hist[event_sft].set.req_no,
			event_hist[event_sft].set.frm_no,
			event_hist[event_sft].set.ts,
			event_hist[event_sft].wait.req_fd,
			event_hist[event_sft].wait.req_no,
			event_hist[event_sft].wait.frm_no,
			event_hist[event_sft].wait.ts);
	} else if ((event >= IMGSYS_CMDQ_SW_EVENT6_BEGIN) &&
		(event <= IMGSYS_CMDQ_SW_EVENT6_END)) {
		event_sft = event - IMGSYS_CMDQ_SW_EVENT6_BEGIN +
			(IMGSYS_CMDQ_SW_EVENT5_END - IMGSYS_CMDQ_SW_EVENT5_BEGIN + 1) +
			(IMGSYS_CMDQ_SW_EVENT4_END - IMGSYS_CMDQ_SW_EVENT4_BEGIN + 1) +
			(IMGSYS_CMDQ_SW_EVENT3_END - IMGSYS_CMDQ_SW_EVENT3_BEGIN + 1) +
			(IMGSYS_CMDQ_SW_EVENT2_END - IMGSYS_CMDQ_SW_EVENT2_BEGIN + 1) +
			(IMGSYS_CMDQ_SW_EVENT1_END - IMGSYS_CMDQ_SW_EVENT1_BEGIN + 1);
		event_diff = event_hist[event_sft].set.ts >
					event_hist[event_sft].wait.ts ?
					(event_hist[event_sft].set.ts -
					event_hist[event_sft].wait.ts) :
					(event_hist[event_sft].wait.ts -
					event_hist[event_sft].set.ts);
		ret = CMDQ_NO_AEE;
		pr_info(
			"%s: [ERROR] SW event6 timeout! wfe(%d) event(%d) isHW(%d); event st(%d)_ts(%lld)_set(%d/%d/%d/%lld)_wait(%d/%d/%d/%lld)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event, isHWhang,
			event_hist[event_sft].st, event_diff,
			event_hist[event_sft].set.req_fd,
			event_hist[event_sft].set.req_no,
			event_hist[event_sft].set.frm_no,
			event_hist[event_sft].set.ts,
			event_hist[event_sft].wait.req_fd,
			event_hist[event_sft].wait.req_no,
			event_hist[event_sft].wait.frm_no,
			event_hist[event_sft].wait.ts);
	} else if ((event >= IMGSYS_CMDQ_QOF_EVENT_BEGIN) &&
		(event <= IMGSYS_CMDQ_QOF_EVENT_END)) {
		ret = CMDQ_NO_AEE;
		pr_info(
			"%s: [ERROR] QOF event timeout! wfe(%d) event(%d)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event);
	} else if ((event >= IMGSYS_CMDQ_GPR_EVENT_BEGIN) &&
		(event <= IMGSYS_CMDQ_GPR_EVENT_END)) {
		isHWhang = 1;
		isGPRtimeout = 1;
		pkt->timeout_dump_hw_trace = 1;
		ret = CMDQ_NO_AEE_DUMP;
		pr_info(
			"%s: [ERROR] GPR event timeout! wfe(%d) event(%d) isHW(%d)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event, isHWhang);
	} else if ((is_stream_off == 1) && (event == 0)) {
		ret = CMDQ_NO_AEE;
		pr_info(
			"%s: [ERROR] pipe had been turned off(%d)! wfe(%d) event(%d) isHW(%d)",
			__func__, is_stream_off,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event, isHWhang);
	} else {
		isHWhang = 1;
		pkt->timeout_dump_hw_trace = 1;
		ret = CMDQ_AEE_EXCEPTION;
		pr_info(
			"%s: [ERROR] HW event timeout! wfe(%d) event(%d) isHW(%d)",
			__func__,
			cb_param->pkt->err_data.wfe_timeout,
			cb_param->pkt->err_data.event, isHWhang);
	}

	return ret;
}

int imgsys_cmdq_sendtask_plat7sp(struct mtk_imgsys_dev *imgsys_dev,
				struct swfrm_info_t *frm_info,
				void (*cmdq_cb)(struct cmdq_cb_data data,
					uint32_t subfidx, bool isLastTaskInReq,
					uint32_t batchnum, uint32_t is_capture),
				void (*cmdq_err_cb)(struct cmdq_cb_data data,
					uint32_t fail_subfidx, bool isHWhang, uint32_t hangEvent),
				u64 (*imgsys_get_iova)(struct dma_buf *dma_buf, s32 ionFd,
					struct mtk_imgsys_dev *imgsys_dev,
					struct mtk_imgsys_dev_buffer *dev_buf),
				int (*is_singledev_mode)(struct mtk_imgsys_request *req))
{
	struct cmdq_client *clt = NULL;
	struct cmdq_pkt *pkt = NULL;
	struct GCERecoder *cmd_buf = NULL;
	struct Command *cmd = NULL;
	struct mtk_imgsys_cb_param *cb_param = NULL;
	struct mtk_imgsys_dvfs *dvfs_info = NULL;
	dma_addr_t pkt_ts_pa = 0;
	u32 *pkt_ts_va = NULL;
	u32 pkt_ts_num = 0;
	u32 pkt_ts_ofst = 0;
	u32 cmd_num = 0;
	u32 cmd_idx = 0;
	u32 blk_idx = 0; /* For Vss block cnt */
	u32 blk_num = 0;
	u32 thd_idx = 0;
	u32 hw_comb = 0;
	int ret = 0, ret_flush = 0, ret_sn = 0;
	u64 tsReqStart = 0;
	u64 tsDvfsQosStart = 0, tsDvfsQosEnd = 0;
	u32 frm_num = 0, frm_idx = 0;
	u32 cmd_ofst = 0;
	bool isPack = 0;
	u32 task_idx = 0;
	u32 task_id = 0;
	u32 task_num = 0;
	u32 task_cnt = 0;
	bool qof_need_sub[ISP7SP_PWR_NUM] = {0};
	size_t pkt_ofst[MAX_FRAME_IN_TASK] = {0};
	char logBuf_temp[MTK_IMGSYS_LOG_LENGTH];
	u64 tsflushStart = 0, tsFlushEnd = 0;
	bool isTimeShared = 0;
	u32 log_sz = 0;

	/* PMQOS API */
	tsDvfsQosStart = ktime_get_boottime_ns()/1000;
	IMGSYS_CMDQ_SYSTRACE_BEGIN("%s_%s|Imgsys MWFrame:#%d MWReq:#%d ReqFd:%d Own:%llx",
		__func__, "dvfs_qos", frm_info->frame_no, frm_info->request_no,
		frm_info->request_fd, frm_info->frm_owner);
	mutex_lock(&(imgsys_dev->dvfs_qos_lock));
	#if DVFS_QOS_READY
	mtk_imgsys_mmdvfs_mmqos_cal_plat7sp(imgsys_dev, frm_info, 1);
	mtk_imgsys_mmdvfs_set_plat7sp(imgsys_dev, frm_info, 1);
	mtk_imgsys_mmqos_set_by_scen_plat7sp(imgsys_dev, frm_info, 1);
	#endif
	mutex_unlock(&(imgsys_dev->dvfs_qos_lock));
	IMGSYS_CMDQ_SYSTRACE_END();
	tsDvfsQosEnd = ktime_get_boottime_ns()/1000;

	/* is_stream_off = 0; */
	frm_num = frm_info->total_frmnum;
	frm_info->cb_frmcnt = 0;
	frm_info->total_taskcnt = 0;
	cmd_ofst = sizeof(struct GCERecoder);
	dvfs_info = &imgsys_dev->dvfs_info;

	#if IMGSYS_SECURE_ENABLE
	mutex_lock(&(imgsys_dev->sec_task_lock));
	if (frm_info->is_secReq && (is_sec_task_create == 0)) {
		imgsys_cmdq_sec_sendtask_plat7sp(imgsys_dev);
		is_sec_task_create = 1;
		pr_info(
			"%s: create imgsys secure task is_secReq(%d)\n",
			__func__, frm_info->is_secReq);
	}
	mutex_unlock(&(imgsys_dev->sec_task_lock));
	#endif

	/* Allocate cmdq buffer for task timestamp */
	if (imgsys_cmdq_ts_enable_plat7sp() || imgsys_wpe_bwlog_enable_plat7sp()) {
		pkt_ts_va = cmdq_mbox_buf_alloc(imgsys_clt[0], &pkt_ts_pa);
		/* if (imgsys_cmdq_ts_dbg_enable_plat7sp()) { */
			log_sz = ((frm_num / 10) + 1) * 4;
			frm_info->hw_ts_log = vzalloc(sizeof(char)*MTK_IMGSYS_LOG_LENGTH*log_sz);
			if (frm_info->hw_ts_log == NULL)
				return -ENOMEM;
			memset((char *)frm_info->hw_ts_log, 0x0, MTK_IMGSYS_LOG_LENGTH*log_sz);
			frm_info->hw_ts_log[strlen(frm_info->hw_ts_log)] = '\0';
			memset((char *)logBuf_temp, 0x0, MTK_IMGSYS_LOG_LENGTH);
			logBuf_temp[strlen(logBuf_temp)] = '\0';
			ret_sn = snprintf(logBuf_temp, MTK_IMGSYS_LOG_LENGTH,
				"Imgsys MWFrame:#%d MWReq:#%d ReqFd:%d Own:%s gid(%d) fps(%d) dvfs_v/f(%d/%ld) freq(%ld/%ld/%ld/%ld)",
				frm_info->frame_no, frm_info->request_no,
				frm_info->request_fd, (char *)(&(frm_info->frm_owner)),
				frm_info->group_id, frm_info->fps,
				dvfs_info->cur_volt, dvfs_info->cur_freq,
				dvfs_info->freq, dvfs_info->pixel_size[0],
				dvfs_info->pixel_size[1], dvfs_info->pixel_size[2]);
			if (ret_sn < 0)
				pr_info("%s: [ERROR] snprintf fail: %d\n", __func__, ret_sn);
			strncat(frm_info->hw_ts_log, logBuf_temp, strlen(logBuf_temp));
		/* } */
	}

	for (frm_idx = 0; frm_idx < frm_num; frm_idx++) {
		cmd_buf = (struct GCERecoder *)frm_info->user_info[frm_idx].g_swbuf;

		if ((cmd_buf->header_code != 0x5A5A5A5A) ||
				(cmd_buf->check_pre != 0x55AA55AA) ||
				(cmd_buf->check_post != 0xAA55AA55) ||
				(cmd_buf->footer_code != 0xA5A5A5A5)) {
			pr_info("%s: Incorrect guard word: %08x/%08x/%08x/%08x", __func__,
				cmd_buf->header_code, cmd_buf->check_pre, cmd_buf->check_post,
				cmd_buf->footer_code);
			return -1;
		}

		cmd_num = cmd_buf->curr_length / sizeof(struct Command);
		cmd = (struct Command *)((unsigned long)(frm_info->user_info[frm_idx].g_swbuf) +
			(unsigned long)(cmd_buf->cmd_offset));
		blk_num = cmd_buf->frame_block;
		hw_comb = frm_info->user_info[frm_idx].hw_comb;
		isTimeShared = frm_info->user_info[frm_idx].is_time_shared;

		if (isPack == 0) {
			if (frm_info->group_id == -1) {
				/* Choose cmdq_client base on hw scenario */
				for (thd_idx = 0; thd_idx < IMGSYS_NOR_THD; thd_idx++) {
					if (hw_comb & 0x1) {
						clt = imgsys_clt[thd_idx];
                        if (imgsys_cmdq_dbg_enable_plat7sp()) {
						pr_debug(
						"%s: chosen mbox thread (%d, 0x%lx) for frm(%d/%d)\n",
						__func__, thd_idx, (unsigned long)clt, frm_idx, frm_num);
                        }
						break;
					}
					hw_comb = hw_comb>>1;
				}
				/* This segment can be removed since user had set dependency */
				if (frm_info->user_info[frm_idx].hw_comb & IMGSYS_ENG_DIP) {
					thd_idx = 4;
					clt = imgsys_clt[thd_idx];
				}
			} else {
				if (frm_info->group_id < IMGSYS_NOR_THD) {
					thd_idx = frm_info->group_id;
					clt = imgsys_clt[thd_idx];
				} else {
					pr_info(
						"%s: [ERROR] group_id(%d) is over max hw num(%d),hw_comb(0x%x) for frm(%d/%d)!\n",
						__func__, frm_info->group_id, IMGSYS_NOR_THD,
						frm_info->user_info[frm_idx].hw_comb,
						frm_idx, frm_num);
					return -1;
				}
			}

			/* This is work around for low latency flow.		*/
			/* If we change to request base,			*/
			/* we don't have to take this condition into account.	*/
			if (frm_info->sync_id != -1) {
				thd_idx = 0;
				clt = imgsys_clt[thd_idx];
			}

			if (clt == NULL) {
				pr_info("%s: [ERROR] No HW Found (0x%x) for frm(%d/%d)!\n",
					__func__, frm_info->user_info[frm_idx].hw_comb,
					frm_idx, frm_num);
				return -1;
			}
		}

        if (imgsys_cmdq_dbg_enable_plat7sp()) {
		dev_dbg(imgsys_dev->dev,
		"%s: req fd/no(%d/%d) frame no(%d) frm(%d/%d) cmd_oft(0x%x/0x%x), cmd_len(%d), num(%d), sz_per_cmd(%lu), frm_blk(%d), hw_comb(0x%x), sync_id(%d), gce_thd(%d), gce_clt(0x%lx)\n",
			__func__, frm_info->request_fd, frm_info->request_no, frm_info->frame_no,
			frm_idx, frm_num, cmd_buf->cmd_offset, cmd_ofst, cmd_buf->curr_length,
			cmd_num, sizeof(struct Command), cmd_buf->frame_block,
			frm_info->user_info[frm_idx].hw_comb, frm_info->sync_id, thd_idx, (unsigned long)clt);
        }

		cmd_idx = 0;
		if (isTimeShared)
			mutex_lock(&(imgsys_dev->vss_blk_lock));
		for (blk_idx = 0; blk_idx < blk_num; blk_idx++) {
			tsReqStart = ktime_get_boottime_ns()/1000;
			if (isPack == 0) {
				/* create pkt and hook clt as pkt's private data */
				pkt = cmdq_pkt_create(clt);
				if (pkt == NULL) {
					pr_info(
						"%s: [ERROR] cmdq_pkt_create fail in block(%d)!\n",
						__func__, blk_idx);
					if (isTimeShared)
						mutex_unlock(&(imgsys_dev->vss_blk_lock));
					return -1;
				}
                if (imgsys_cmdq_dbg_enable_plat7sp()) {
				pr_debug(
					"%s: cmdq_pkt_create success(0x%lx) in block(%d) for frm(%d/%d)\n",
					__func__, (unsigned long)pkt, blk_idx, frm_idx, frm_num);
                }
				/* Reset pkt timestamp num */
				pkt_ts_num = 0;
			}

			MTK_IMGSYS_QOF_NEED_RUN(imgsys_dev->qof_ver,
					mtk_imgsys_cmdq_qof_add(pkt, frm_info->user_info[frm_idx].hw_comb, qof_need_sub));

			IMGSYS_CMDQ_SYSTRACE_BEGIN(
				"%s_%s|Imgsys MWFrame:#%d MWReq:#%d ReqFd:%d fidx:%d hw_comb:0x%x Own:%llx frm(%d/%d) blk(%d)",
				__func__, "cmd_parser", frm_info->frame_no, frm_info->request_no,
				frm_info->request_fd, frm_info->user_info[frm_idx].subfrm_idx,
				frm_info->user_info[frm_idx].hw_comb, frm_info->frm_owner,
				frm_idx, frm_num, blk_idx);
			// Add secure token begin
			#if IMGSYS_SECURE_ENABLE
			if (frm_info->user_info[frm_idx].is_secFrm)
				imgsys_cmdq_sec_cmd_plat7sp(pkt);
			#endif

			ret = imgsys_cmdq_parser_plat7sp(imgsys_dev, frm_info, pkt,
				&cmd[cmd_idx], hw_comb,
				(pkt_ts_pa + 4 * pkt_ts_ofst), &pkt_ts_num, thd_idx,
				imgsys_get_iova, is_singledev_mode);
			if (ret < 0) {
				pr_info(
					"%s: [ERROR] parsing idx(%d) with cmd(%d) in block(%d) for frm(%d/%d) fail\n",
					__func__, cmd_idx, cmd[cmd_idx].opcode,
					blk_idx, frm_idx, frm_num);
				cmdq_pkt_destroy(pkt);
				if (isTimeShared)
					mutex_unlock(&(imgsys_dev->vss_blk_lock));
				goto sendtask_done;
			}
			cmd_idx += ret;

			// Add secure token end
			#if IMGSYS_SECURE_ENABLE
			if (frm_info->user_info[frm_idx].is_secFrm)
				imgsys_cmdq_sec_cmd_plat7sp(pkt);
			#endif

			IMGSYS_CMDQ_SYSTRACE_END();

			/* Check for packing gce task */
			pkt_ofst[task_cnt] = pkt->cmd_buf_size - CMDQ_INST_SIZE;
			task_cnt++;
			if ((frm_info->user_info[frm_idx].is_time_shared)
				|| (frm_info->user_info[frm_idx].is_secFrm)
				|| (frm_info->user_info[frm_idx].is_earlycb)
				|| ((frm_idx + 1) == frm_num)
				|| (frm_info->user_info[frm_idx].wait_fence_num > 0)
				|| (frm_info->user_info[frm_idx].notify_fence_num > 0)) {
#ifndef CONFIG_FPGA_EARLY_PORTING
				mtk_imgsys_power_ctrl_plat7sp(imgsys_dev, true);
#endif
				/* Prepare cb param */
#ifdef IMGSYS_CMDQ_CBPARAM_NUM
				mutex_lock(&g_cb_param_lock);
				g_cb_param_idx = (g_cb_param_idx+1)%IMGSYS_CMDQ_CBPARAM_NUM;
				if ((g_cb_param_idx < 0)
					|| g_cb_param_idx >= IMGSYS_CMDQ_CBPARAM_NUM) {
					dev_info(imgsys_dev->dev,
						"%s: force set g_cb_param_idx(%d) to 0! in block(%d) for frm(%d/%d)\n",
						__func__, g_cb_param_idx, blk_idx,
						frm_idx, frm_num);
					g_cb_param_idx = 0;
				}
				cb_param = &g_cb_param[g_cb_param_idx];
				if (cb_param->isOccupy) {
					dev_info(imgsys_dev->dev,
						"%s: g_cb_param[%d] is occypied!!! in block(%d) for frm(%d/%d)\n",
						__func__, g_cb_param_idx, blk_idx,
						frm_idx, frm_num);
					mutex_unlock(&g_cb_param_lock);
					if (isTimeShared)
						mutex_unlock(&(imgsys_dev->vss_blk_lock));
					return -1;
				}
				cb_param->isOccupy = true;
				mutex_unlock(&g_cb_param_lock);
                if (imgsys_cmdq_dbg_enable_plat7sp()) {
				dev_dbg(imgsys_dev->dev,
						"%s: set cb_param to g_cb_param[%d] in block(%d) for frm(%d/%d)\n",
						__func__, g_cb_param_idx, blk_idx,
						frm_idx, frm_num);
                }
#else
				cb_param =
					vzalloc(sizeof(struct mtk_imgsys_cb_param));
#endif
				if (cb_param == NULL) {
					cmdq_pkt_destroy(pkt);
					dev_info(imgsys_dev->dev,
						"%s: cb_param is NULL! in block(%d) for frm(%d/%d)!\n",
						__func__, blk_idx, frm_idx, frm_num);
					if (isTimeShared)
						mutex_unlock(&(imgsys_dev->vss_blk_lock));
					return -1;
				}
                if (imgsys_cmdq_dbg_enable_plat7sp()) {
				dev_dbg(imgsys_dev->dev,
				"%s: cb_param kzalloc success cb(%p) in block(%d) for frm(%d/%d)!\n",
					__func__, cb_param, blk_idx, frm_idx, frm_num);
                }

				task_num++;
				cb_param->pkt = pkt;
				cb_param->frm_info = frm_info;
				cb_param->req_fd = frm_info->request_fd;
				cb_param->req_no = frm_info->request_no;
				cb_param->frm_no = frm_info->frame_no;
				cb_param->hw_comb = hw_comb;
				cb_param->frm_idx = frm_idx;
				cb_param->frm_num = frm_num;
				cb_param->user_cmdq_cb = cmdq_cb;
				cb_param->user_cmdq_err_cb = cmdq_err_cb;
				if ((blk_idx + 1) == blk_num)
					cb_param->isBlkLast = 1;
				else
					cb_param->isBlkLast = 0;
				if ((frm_idx + 1) == frm_num)
					cb_param->isFrmLast = 1;
				else
					cb_param->isFrmLast = 0;
				cb_param->blk_idx = blk_idx;
				cb_param->blk_num = blk_num;
				cb_param->is_earlycb = frm_info->user_info[frm_idx].is_earlycb;
				cb_param->group_id = frm_info->group_id;
				cb_param->cmdqTs.tsReqStart = tsReqStart;
				cb_param->cmdqTs.tsDvfsQosStart = tsDvfsQosStart;
				cb_param->cmdqTs.tsDvfsQosEnd = tsDvfsQosEnd;
				cb_param->imgsys_dev = imgsys_dev;
				cb_param->thd_idx = thd_idx;
				cb_param->clt = clt;
				cb_param->task_cnt = task_cnt;
				for (task_idx = 0; task_idx < task_cnt; task_idx++)
					cb_param->pkt_ofst[task_idx] = pkt_ofst[task_idx];
				task_cnt = 0;
				cb_param->task_id = task_id;
				task_id++;
				if ((cb_param->isBlkLast) && (cb_param->isFrmLast)) {
					cb_param->isTaskLast = 1;
					cb_param->task_num = task_num;
					frm_info->total_taskcnt = task_num;
				} else {
					cb_param->isTaskLast = 0;
					cb_param->task_num = 0;
				}
				cb_param->batchnum = frm_info->batchnum;
				cb_param->is_capture = frm_info->is_capture;

				imgsys_cmdq_fence_set_plat7sp(imgsys_dev, frm_info, cb_param,
					frm_idx, thd_idx);

                if (imgsys_cmdq_dbg_enable_plat7sp()) {
				dev_dbg(imgsys_dev->dev,
					"%s: cb(%p) gid(%d) in block(%d/%d) for frm(%d/%d) lst(%d/%d/%d) task(%d/%d/%d) ofst(%lx/%lx/%lx/%lx/%lx)\n",
					__func__, cb_param, cb_param->group_id,
					cb_param->blk_idx,  cb_param->blk_num,
					cb_param->frm_idx, cb_param->frm_num,
					cb_param->isBlkLast, cb_param->isFrmLast,
					cb_param->isTaskLast, cb_param->task_id,
					cb_param->task_num, cb_param->task_cnt,
					cb_param->pkt_ofst[0], cb_param->pkt_ofst[1],
					cb_param->pkt_ofst[2], cb_param->pkt_ofst[3],
					cb_param->pkt_ofst[4]);
                }

				if (imgsys_cmdq_ts_enable_plat7sp()
					|| imgsys_wpe_bwlog_enable_plat7sp()) {
					cb_param->taskTs.dma_pa = pkt_ts_pa;
					cb_param->taskTs.dma_va = pkt_ts_va;
					cb_param->taskTs.num = pkt_ts_num;
					cb_param->taskTs.ofst = pkt_ts_ofst;
					pkt_ts_ofst += pkt_ts_num;
				}

				/* flush synchronized, block API */
				cb_param->cmdqTs.tsFlushStart = ktime_get_boottime_ns()/1000;
				tsflushStart = cb_param->cmdqTs.tsFlushStart;

				pkt->aee_cb = imgsys_cmdq_task_aee_cb_plat7sp;
				pkt->user_priv = (void *)cb_param;

				IMGSYS_CMDQ_SYSTRACE_BEGIN(
					"%s_%s|Imgsys MWFrame:#%d MWReq:#%d ReqFd:%d fidx:%d hw_comb:0x%x Own:%llx cb(%p) frm(%d/%d) blk(%d/%d)",
					__func__, "pkt_flush", frm_info->frame_no,
					frm_info->request_no, frm_info->request_fd,
					frm_info->user_info[frm_idx].subfrm_idx,
					frm_info->user_info[frm_idx].hw_comb,
					frm_info->frm_owner, cb_param, frm_idx, frm_num,
					blk_idx, blk_num);

				MTK_IMGSYS_QOF_NEED_RUN(imgsys_dev->qof_ver, mtk_imgsys_cmdq_qof_sub(pkt, qof_need_sub));

				ret_flush = cmdq_pkt_flush_async(pkt, imgsys_cmdq_task_cb_plat7sp,
								(void *)cb_param);
				IMGSYS_CMDQ_SYSTRACE_END();

				tsFlushEnd = ktime_get_boottime_ns()/1000;
				if (ret_flush < 0)
					pr_info(
					"%s: cmdq_pkt_flush_async ret(%d) for frm(%d/%d) ts(%lld)!\n",
						__func__, ret_flush, frm_idx, frm_num,
						tsFlushEnd - tsflushStart);
				else {
                    if (imgsys_cmdq_dbg_enable_plat7sp()) {
					pr_debug(
					"%s: cmdq_pkt_flush_async success(%d), blk(%d), frm(%d/%d), ts(%lld)!\n",
						__func__, ret_flush, blk_idx, frm_idx, frm_num,
						tsFlushEnd - tsflushStart);
                    }
				}
				isPack = 0;
			} else {
				isPack = 1;
			}
		}
		if (isTimeShared)
			mutex_unlock(&(imgsys_dev->vss_blk_lock));
	}

sendtask_done:
	return ret;
}

int imgsys_cmdq_parser_plat7sp(struct mtk_imgsys_dev *imgsys_dev,
					struct swfrm_info_t *frm_info, struct cmdq_pkt *pkt,
					struct Command *cmd, u32 hw_comb,
					dma_addr_t dma_pa, uint32_t *num, u32 thd_idx,
					u64 (*imgsys_get_iova)(struct dma_buf *dma_buf, s32 ionFd,
						struct mtk_imgsys_dev *imgsys_dev,
						struct mtk_imgsys_dev_buffer *dev_buf),
					int (*is_singledev_mode)(struct mtk_imgsys_request *req))
{
	bool stop = 0;
	int count = 0;
	int req_fd = 0, req_no = 0, frm_no = 0;
	u32 event = 0;
#ifdef MTK_IOVA_SINK2KERNEL
	u64 iova_addr = 0, cur_iova_addr = 0;
	struct mtk_imgsys_req_fd_info *fd_info = NULL;
	struct dma_buf *dbuf = NULL;
	struct mtk_imgsys_request *req = NULL;
	struct mtk_imgsys_dev_buffer *dev_b = 0;
	bool iova_dbg = false;
	u16 pre_fd = 0;
#endif
#ifdef IMGSYS_ME_CHECK_FUNC_EN
	u32 me_read_num = 0;
#endif
	req_fd = frm_info->request_fd;
	req_no = frm_info->request_no;
	frm_no = frm_info->frame_no;

    if (imgsys_cmdq_dbg_enable_plat7sp()) {
	pr_debug("%s: +, cmd(%d)\n", __func__, cmd->opcode);
    }

	do {
		switch (cmd->opcode) {
		case IMGSYS_CMD_READ:
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			pr_debug(
				"%s: READ with source(0x%08x) target(0x%08x) mask(0x%08x)\n",
				__func__, cmd->u.source, cmd->u.target, cmd->u.mask);
            }
			if (imgsys_wpe_bwlog_enable_plat7sp() && (hw_comb != 0x800)) {
				cmdq_pkt_mem_move(pkt, NULL, (dma_addr_t)cmd->u.source,
					dma_pa + (4*(*num)), CMDQ_THR_SPR_IDX3);
				(*num)++;
#ifdef IMGSYS_ME_CHECK_FUNC_EN
			} else if (hw_comb == 0x800) {
				cmdq_pkt_mem_move(pkt, NULL, (dma_addr_t)cmd->u.source,
					g_pkt_me_pa + (4 * me_read_num), CMDQ_THR_SPR_IDX2);
				me_read_num++;
				is_me_read_cmd = 1;
#endif
			} else
				pr_info(
					"%s: [ERROR]Not enable imgsys read cmd!!\n",
					__func__);
			break;
		case IMGSYS_CMD_WRITE:
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			pr_debug(
				"%s: WRITE with addr(0x%08x) value(0x%08x) mask(0x%08x)\n",
				__func__, cmd->u.address, cmd->u.value, cmd->u.mask);
            }
			cmdq_pkt_write(pkt, NULL, (dma_addr_t)cmd->u.address,
					cmd->u.value, cmd->u.mask);
			break;
#ifdef MTK_IOVA_SINK2KERNEL
		case IMGSYS_CMD_WRITE_FD:
			iova_dbg = (imgsys_iova_dbg_port_plat7sp() == cmd->u.dma_addr);
			if (imgsys_iova_dbg_enable_plat7sp() || iova_dbg) {
				pr_info(
					"%s: WRITE_FD with req_fd/no(%d/%d) frame_no(%d) addr(0x%08lx) msb_ofst(0x%08x) fd(0x%08x) ofst(0x%08x) rshift(%d)\n",
					__func__, req_fd, req_no, frm_no,
				(unsigned long)cmd->u.dma_addr, cmd->u.dma_addr_msb_ofst,
				cmd->u.fd, cmd->u.ofst, cmd->u.right_shift);
			}
			if (cmd->u.fd <= 0) {
				pr_info("%s: [ERROR] WRITE_FD with FD(%d)! req_fd/no(%d/%d) frame_no(%d)\n",
					__func__, cmd->u.fd, req_fd, req_no, frm_no);
				return -1;
			}
			//
			if (cmd->u.fd != pre_fd) {
				dbuf = dma_buf_get(cmd->u.fd);
				fd_info = &imgsys_dev->req_fd_cache.info_array[req_fd];
				req = (struct mtk_imgsys_request *) fd_info->req_addr_va;
				dev_b = req->buf_map[is_singledev_mode(req)];
				iova_addr = imgsys_get_iova(dbuf, cmd->u.fd, imgsys_dev, dev_b);
				pre_fd = cmd->u.fd;

				if (iova_addr <= 0) {
					u32 dma_addr_msb = (cmd->u.dma_addr >> 16);

					pr_info(
						"%s: [ERROR] WRITE_FD map iova fail (%llu)! with req_fd/no(%d/%d) frame_no(%d) fd(%d) addr(0x%08lx)\n",
						__func__, iova_addr, req_fd, req_no, frm_no, cmd->u.fd,
						(unsigned long)cmd->u.dma_addr);

					switch (dma_addr_msb) {
					case 0x1510:
					case 0x1515:
					case 0x1516:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_DIP",
							"DISPATCH:IMGSYS_DIP map iova fail, addr:0x%08llx",
							(unsigned long)cmd->u.dma_addr);
						break;
					case 0x1570:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_TRAW",
							"DISPATCH:IMGSYS_TRAW map iova fail, addr:0x%08llx",
							(unsigned long)cmd->u.dma_addr);
						break;
					case 0x1504:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_LTRAW",
							"DISPATCH:IMGSYS_LTRAW map iova fail, addr:0x%08llx",
							(unsigned long)cmd->u.dma_addr);
						break;
					case 0x1520:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_WPE_EIS",
							"DISPATCH:IMGSYS_WPE_EIS map iova fail, addr:0x%08llx",
							(unsigned long)cmd->u.dma_addr);
						break;
					case 0x1550:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_WPE_TNR",
							"DISPATCH:IMGSYS_WPE_TNR map iova fail, addr:0x%08llx",
							(unsigned long)cmd->u.dma_addr);
						break;
					case 0x1560:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_WPE_LITE",
							"DISPATCH:IMGSYS_WPE_LITE map iova fail, addr:0x%08llx",
							(unsigned long)cmd->u.dma_addr);
						break;
					case 0x1521:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_PQDIP_A",
							"DISPATCH:IMGSYS_PQDIP_A map iova fail, addr:0x%08llx",
							 (unsigned long)cmd->u.dma_addr);
						break;
					case 0x1551:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_PQDIP_B",
							"DISPATCH:IMGSYS_PQDIP_B map iova fail, addr:0x%08llx",
							(unsigned long)cmd->u.dma_addr);
						break;
					case 0x1532:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_ME",
							"DISPATCH:IMGSYS_ME map iova fail, addr:0x%08llx",
							(unsigned long)cmd->u.dma_addr);
						break;
					case 0x1533:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS_MMG",
							"DISPATCH:IMGSYS_MMG map iova fail, addr:0x%08llx",
							(unsigned long)cmd->u.dma_addr);
						break;
					default:
						aee_kernel_exception("CRDISPATCH_KEY:IMGSYS",
							"DISPATCH:IMGSYS map iova fail, addr:0x%08llx",
							(unsigned long)cmd->u.dma_addr);
						break;
					}

					return -1;
				}
			} else {
				if (imgsys_cmdq_dbg_enable_plat7sp()) {
					pr_info(
						"%s: Current fd(0x%08x) is the same with previous fd(0x%08x) with iova(0x%08llx), bypass map iova operation\n",
						__func__, cmd->u.fd, pre_fd, iova_addr);
				}
			}
			cur_iova_addr = iova_addr + cmd->u.ofst;
			//
			if (imgsys_iova_dbg_enable_plat7sp() || iova_dbg) {
				pr_info(
					"%s: WRITE_FD with req_fd/no(%d/%d) frame_no(%d) addr(0x%08lx) value(0x%08llx)\n",
					__func__, req_fd, req_no, frm_no,
					(unsigned long)cmd->u.dma_addr,
					(cur_iova_addr >> cmd->u.right_shift));
			}
			cmdq_pkt_write(pkt, NULL, cmd->u.dma_addr,
				(cur_iova_addr >> cmd->u.right_shift), 0xFFFFFFFF);

			if (cmd->u.dma_addr_msb_ofst) {
				if (imgsys_iova_dbg_enable_plat7sp() || iova_dbg) {
					pr_info(
						"%s: WRITE_FD with req_fd/no(%d/%d) frame_no(%d) addr(0x%08lx) value(0x%08llx)\n",
						__func__, req_fd, req_no, frm_no,
						(unsigned long)cmd->u.dma_addr,
						(cur_iova_addr>>32));
				}
				cmdq_pkt_write(pkt, NULL,
					(cmd->u.dma_addr + cmd->u.dma_addr_msb_ofst),
					(cur_iova_addr>>32), 0xFFFFFFFF);
			}

			break;
#endif
		case IMGSYS_CMD_POLL:
			/* cmdq_pkt_poll(pkt, NULL, cmd->u.value, cmd->u.address, */
			/* cmd->u.mask, CMDQ_GPR_R15); */
		{
			u32 addr_msb = (cmd->u.address >> 16);
			u32 gpr_idx = 0;
			switch (addr_msb) {
				case 0x1510:
				case 0x1515:
				case 0x1516:
					/* DIP */
					gpr_idx = 0;
					break;
				case 0x1570:
					/* TRAW */
					gpr_idx = 1;
					break;
				case 0x1504:
					/* LTRAW */
					gpr_idx = 2;
					break;
				case 0x1520:
					/* WPE_EIS */
					gpr_idx = 3;
					break;
				case 0x1550:
					/* WPE_TNR */
					gpr_idx = 4;
					break;
				case 0x1560:
					/* WPE_LITE */
					gpr_idx = 5;
					break;
				case 0x1521:
					/* PQDIP_A */
					gpr_idx = 6;
					break;
				case 0x1551:
					/* PQDIP_B */
					gpr_idx = 7;
					break;
				case 0x1532:
				case 0x1533:
					/* ME/MMG */
					gpr_idx = 8;
					break;
				default:
					gpr_idx = thd_idx;
					break;
			}
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			pr_debug(
					"%s: POLL with addr(0x%08x) value(0x%08x) mask(0x%08x) addr_msb(0x%08x) thd(%d/%d)\n",
					__func__, cmd->u.address, cmd->u.value, cmd->u.mask, addr_msb, gpr_idx, thd_idx);
            }
			#ifdef IMGSYS_ME_CHECK_FUNC_EN
			if ((gpr_idx == 8) && (hw_comb == 0x800))
			cmdq_pkt_poll_timeout(pkt, cmd->u.value, SUBSYS_NO_SUPPORT,
				cmd->u.address, cmd->u.mask, IMGSYS_POLL_TIME_30MS,
					CMDQ_GPR_R03+gpr_idx);
			else
				cmdq_pkt_poll_timeout(pkt, cmd->u.value, SUBSYS_NO_SUPPORT,
					cmd->u.address, cmd->u.mask, IMGSYS_POLL_TIME_INFINI,
					CMDQ_GPR_R03+gpr_idx);
			#else
			cmdq_pkt_poll_timeout(pkt, cmd->u.value, SUBSYS_NO_SUPPORT,
				cmd->u.address, cmd->u.mask, IMGSYS_POLL_TIME_INFINI,
				CMDQ_GPR_R03+gpr_idx);
			#endif
		}
			break;
		case IMGSYS_CMD_WAIT:
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			pr_debug(
				"%s: WAIT event(%d/%d) action(%d)\n",
				__func__, cmd->u.event, imgsys_event[cmd->u.event].event,
				cmd->u.action);
            }
			if (cmd->u.action == 1) {
				cmdq_pkt_wfe(pkt, imgsys_event[cmd->u.event].event);
				if ((cmd->u.event >= IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_POOL_START) &&
					(cmd->u.event <= IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_END)) {
					event = cmd->u.event -
						IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_POOL_START;
					event_hist[event].st++;
					event_hist[event].wait.req_fd = req_fd;
					event_hist[event].wait.req_no = req_no;
					event_hist[event].wait.frm_no = frm_no;
					event_hist[event].wait.ts = ktime_get_boottime_ns()/1000;
					event_hist[event].wait.frm_info = frm_info;
					event_hist[event].wait.pkt = pkt;
				}
			} else if (cmd->u.action == 0)
				cmdq_pkt_wait_no_clear(pkt, imgsys_event[cmd->u.event].event);
			else
				pr_info("%s: [ERROR]Not Support wait action(%d)!\n",
					__func__, cmd->u.action);
			break;
		case IMGSYS_CMD_UPDATE:
			if (imgsys_cmdq_dbg_enable_plat7sp()) {
				pr_debug(
					"%s: UPDATE event(%d/%d) action(%d)\n",
					__func__, cmd->u.event, imgsys_event[cmd->u.event].event,
					cmd->u.action);
			}
			if (cmd->u.action == 1) {
				cmdq_pkt_set_event(pkt, imgsys_event[cmd->u.event].event);
				if ((cmd->u.event >= IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_POOL_START) &&
					(cmd->u.event <= IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_END)) {
					event = cmd->u.event -
						IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_POOL_START;
					event_hist[event].st--;
					event_hist[event].set.req_fd = req_fd;
					event_hist[event].set.req_no = req_no;
					event_hist[event].set.frm_no = frm_no;
					event_hist[event].set.ts = ktime_get_boottime_ns()/1000;
					event_hist[event].set.frm_info = frm_info;
					event_hist[event].set.pkt = pkt;
				}
			} else if (cmd->u.action == 0)
				cmdq_pkt_clear_event(pkt, imgsys_event[cmd->u.event].event);
			else
				pr_info("%s: [ERROR]Not Support update action(%d)!\n",
					__func__, cmd->u.action);
			break;
		case IMGSYS_CMD_ACQUIRE:
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			pr_debug(
				"%s: ACQUIRE event(%d/%d) action(%d)\n", __func__,
				cmd->u.event, imgsys_event[cmd->u.event].event, cmd->u.action);
            }
            cmdq_pkt_acquire_event(pkt, imgsys_event[cmd->u.event].event);
			break;
		case IMGSYS_CMD_TIME:
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			pr_debug(
				"%s: TIME with addr(0x%08lx) num(0x%08x)\n",
				__func__, (unsigned long)dma_pa, *num);
            }
			if (imgsys_cmdq_ts_enable_plat7sp()) {
				cmdq_pkt_write_indriect(pkt, NULL, dma_pa + (4*(*num)),
					CMDQ_TPR_ID, ~0);
				(*num)++;
			} else
				pr_info(
					"%s: [ERROR]Not enable imgsys cmdq ts function!!\n",
					__func__);
			break;
		case IMGSYS_CMD_STOP:
            if (imgsys_cmdq_dbg_enable_plat7sp()) {
			pr_debug("%s: End Of Cmd!\n", __func__);
            }
			stop = 1;
			break;
		default:
			pr_info("%s: [ERROR]Not Support Cmd(%d)!\n", __func__, cmd->opcode);
			return -1;
		}
		cmd++;
		count++;
	} while (stop == 0);

	return count;
}

void imgsys_cmdq_sec_task_cb_plat7sp(struct cmdq_cb_data data)
{
	struct cmdq_pkt *pkt_sec = (struct cmdq_pkt *)data.data;

	cmdq_pkt_destroy(pkt_sec);
}

int imgsys_cmdq_sec_sendtask_plat7sp(struct mtk_imgsys_dev *imgsys_dev)
{
	struct cmdq_client *clt_sec = NULL;
	#if IMGSYS_SECURE_ENABLE
	struct cmdq_pkt *pkt_sec = NULL;
	#endif
	int ret = 0;

	clt_sec = imgsys_sec_clt[0];
	#if IMGSYS_SECURE_ENABLE
	pkt_sec = cmdq_pkt_create(clt_sec);
	cmdq_sec_pkt_set_data(pkt_sec, 0, 0, CMDQ_SEC_DEBUG, CMDQ_METAEX_TZMP);
	cmdq_sec_pkt_set_mtee(pkt_sec, true);
	cmdq_pkt_finalize_loop(pkt_sec);
	cmdq_pkt_flush_threaded(pkt_sec, imgsys_cmdq_sec_task_cb_plat7sp, (void *)pkt_sec);
	#endif
	return ret;
}

void imgsys_cmdq_sec_cmd_plat7sp(struct cmdq_pkt *pkt)
{
	cmdq_pkt_set_event(pkt, imgsys_event[IMGSYS_CMDQ_SYNC_TOKEN_TZMP_ISP_WAIT].event);
	cmdq_pkt_wfe(pkt, imgsys_event[IMGSYS_CMDQ_SYNC_TOKEN_TZMP_ISP_SET].event);
}

void imgsys_cmdq_setevent_plat7sp(u64 u_id)
{
	u32 event_id = 0L, event_val = 0L;

	event_id = IMGSYS_CMDQ_SYNC_TOKEN_CAMSYS_POOL_1 + (u_id % 10);
	event_val = cmdq_get_event(imgsys_clt[0]->chan, imgsys_event[event_id].event);

	if (event_val == 0) {
		cmdq_set_event(imgsys_clt[0]->chan, imgsys_event[event_id].event);
        if (imgsys_cmdq_dbg_enable_plat7sp()) {
		pr_debug("%s: SetEvent success with (u_id/event_id/event_val)=(%llu/%d/%d)!\n",
			__func__, u_id, event_id, event_val);
        }
	} else {
		pr_info("%s: [ERROR]SetEvent fail with (u_id/event_id/event_val)=(%llu/%d/%d)!\n",
			__func__, u_id, event_id, event_val);
	}
}

void imgsys_cmdq_clearevent_plat7sp(int event_id)
{
	u32 event = 0;

	if ((event_id >= IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_POOL_START) &&
		(event_id <= IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_END)) {
		cmdq_mbox_enable(imgsys_clt[0]->chan);
		cmdq_clear_event(imgsys_clt[0]->chan, imgsys_event[event_id].event);
        if (imgsys_cmdq_dbg_enable_plat7sp()) {
		pr_debug("%s: cmdq_clear_event with (%d/%d)!\n",
			__func__, event_id, imgsys_event[event_id].event);
        }
		event = event_id - IMGSYS_CMDQ_SYNC_TOKEN_IMGSYS_POOL_START;
		memset((void *)&event_hist[event], 0x0,
			sizeof(struct imgsys_event_history));
		cmdq_mbox_disable(imgsys_clt[0]->chan);
	} else {
		pr_info("%s: [ERROR]unexpected event_id=(%d)!\n",
			__func__, event_id);
	}
}

#if DVFS_QOS_READY
void mtk_imgsys_power_ctrl_plat7sp(struct mtk_imgsys_dev *imgsys_dev, bool isPowerOn)
{
	struct mtk_imgsys_dvfs *dvfs_info = &imgsys_dev->dvfs_info;
	u32 user_cnt = 0;
	int i;
	u32 img_main_modules = 0xFFFF;

	if (imgsys_dev->qof_ver != MTK_IMGSYS_QOF_FUNCTION_OFF) {
		img_main_modules = BIT(IMGSYS_MOD_ADL) |
			BIT(IMGSYS_MOD_ME) |
			BIT(IMGSYS_MOD_IMGMAIN);
	}

	if (isPowerOn) {
		user_cnt = atomic_inc_return(&imgsys_dev->imgsys_user_cnt);
		if (user_cnt == 1) {
			if (!imgsys_quick_onoff_enable_plat7sp())
				dev_info(dvfs_info->dev,
					"[%s] isPowerOn(%d) user(%d)\n",
					__func__, isPowerOn, user_cnt);

			mutex_lock(&(imgsys_dev->power_ctrl_lock));

			cmdq_mbox_enable(imgsys_clt[0]->chan);

			pm_runtime_get_sync(imgsys_dev->dev);

			/*set default value for hw module*/
			mtk_imgsys_mod_get(imgsys_dev);

			for (i = 0; i < imgsys_dev->modules_num; i++)
				if ((BIT(i) & img_main_modules) && imgsys_dev->modules[i].set)
					imgsys_dev->modules[i].set(imgsys_dev);
			mutex_unlock(&(imgsys_dev->power_ctrl_lock));
		}
	} else {
		user_cnt = atomic_dec_return(&imgsys_dev->imgsys_user_cnt);
		if (user_cnt == 0) {
			if (!imgsys_quick_onoff_enable_plat7sp())
				dev_info(dvfs_info->dev,
					"[%s] isPowerOn(%d) user(%d)\n",
					__func__, isPowerOn, user_cnt);

			mutex_lock(&(imgsys_dev->power_ctrl_lock));

			mtk_imgsys_mod_put(imgsys_dev);
			pm_runtime_put_sync(imgsys_dev->dev);
			//pm_runtime_mark_last_busy(imgsys_dev->dev);
			//pm_runtime_put_autosuspend(imgsys_dev->dev);

			cmdq_mbox_disable(imgsys_clt[0]->chan);

			mutex_unlock(&(imgsys_dev->power_ctrl_lock));
		}
	}
}

void mtk_imgsys_main_power_ctrl_plat7sp(struct mtk_imgsys_dev *imgsys_dev, bool isPowerOn)
{
	int i;
	const u32 img_main_modules
			= BIT(IMGSYS_MOD_ADL) |
			BIT(IMGSYS_MOD_ME) |
			BIT(IMGSYS_MOD_IMGMAIN);

	if (isPowerOn) {
		for (i = 0; i < imgsys_dev->modules_num; i++) {
			if ((BIT(i) & img_main_modules) && imgsys_dev->modules[i].set)
				imgsys_dev->modules[i].set(imgsys_dev);
		}
	}
}

#endif

bool imgsys_cmdq_ts_enable_plat7sp(void)
{
	return imgsys_cmdq_ts_en;
}

u32 imgsys_wpe_bwlog_enable_plat7sp(void)
{
	return imgsys_wpe_bwlog_en;
}

bool imgsys_cmdq_ts_dbg_enable_plat7sp(void)
{
	return imgsys_cmdq_ts_dbg_en;
}

bool imgsys_cmdq_dbg_enable_plat7sp(void)
{
	return imgsys_cmdq_dbg_en;
}


bool imgsys_dvfs_dbg_enable_plat7sp(void)
{
	return imgsys_dvfs_dbg_en;
}

bool imgsys_qos_dbg_enable_plat7sp(void)
{
	return imgsys_qos_dbg_en;
}

bool imgsys_quick_onoff_enable_plat7sp(void)
{
	return imgsys_quick_onoff_en;
}

bool imgsys_fence_dbg_enable_plat7sp(void)
{
	return imgsys_fence_dbg_en;
}

bool imgsys_fine_grain_dvfs_enable_plat7sp(void)
{
	return imgsys_fine_grain_dvfs_en;
}

bool imgsys_iova_dbg_enable_plat7sp(void)
{
	return imgsys_iova_dbg_en;
}

u32 imgsys_iova_dbg_port_plat7sp(void)
{
	return imgsys_iova_dbg_port_en;
}

struct imgsys_cmdq_cust_data imgsys_cmdq_data_7sp = {
	.cmdq_init = imgsys_cmdq_init_plat7sp,
	.cmdq_release = imgsys_cmdq_release_plat7sp,
	.cmdq_streamon = imgsys_cmdq_streamon_plat7sp,
	.cmdq_streamoff = imgsys_cmdq_streamoff_plat7sp,
	.cmdq_sendtask = imgsys_cmdq_sendtask_plat7sp,
	.cmdq_sec_sendtask = imgsys_cmdq_sec_sendtask_plat7sp,
	.cmdq_sec_cmd = imgsys_cmdq_sec_cmd_plat7sp,
	.cmdq_clearevent = imgsys_cmdq_clearevent_plat7sp,
#if DVFS_QOS_READY
	.mmdvfs_init = mtk_imgsys_mmdvfs_init_plat7sp,
	.mmdvfs_uninit = mtk_imgsys_mmdvfs_uninit_plat7sp,
	.mmdvfs_set = mtk_imgsys_mmdvfs_set_plat7sp,
	.mmqos_init = mtk_imgsys_mmqos_init_plat7sp,
	.mmqos_uninit = mtk_imgsys_mmqos_init_plat7sp,
	.mmqos_set_by_scen = mtk_imgsys_mmqos_set_by_scen_plat7sp,
	.mmqos_reset = mtk_imgsys_mmqos_reset_plat7sp,
	.mmdvfs_mmqos_cal = mtk_imgsys_mmdvfs_mmqos_cal_plat7sp,
	.mmqos_bw_cal = mtk_imgsys_mmqos_bw_cal_plat7sp,
	.mmqos_ts_cal = mtk_imgsys_mmqos_ts_cal_plat7sp,
	.power_ctrl = mtk_imgsys_power_ctrl_plat7sp,
    .main_power_ctrl = mtk_imgsys_main_power_ctrl_plat7sp,
	.mmqos_monitor = mtk_imgsys_mmqos_monitor_plat7sp,
#endif
	.cmdq_ts_en = imgsys_cmdq_ts_enable_plat7sp,
	.wpe_bwlog_en = imgsys_wpe_bwlog_enable_plat7sp,
	.cmdq_ts_dbg_en = imgsys_cmdq_ts_dbg_enable_plat7sp,
	.dvfs_dbg_en = imgsys_dvfs_dbg_enable_plat7sp,
	.quick_onoff_en = imgsys_quick_onoff_enable_plat7sp,
};
MODULE_IMPORT_NS(DMA_BUF);
