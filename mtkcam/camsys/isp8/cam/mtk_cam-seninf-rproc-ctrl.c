// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2023 MediaTek Inc.

#include <linux/remoteproc.h>
#ifndef RM_CCU_DEPENDENT
#include <linux/remoteproc/mtk_ccu.h>
#endif

#include "mtk_cam-seninf-rproc-ctrl.h"
#include "mtk_cam-seninf-tsrec.h"
#include "mtk_cam-seninf-hw.h"


static struct device_node *seninf_rproc_node;


/******************************************************************************/
// private / static function
/******************************************************************************/
static inline int is_seninf_rproc_node_valid(struct device *dev,
	const char *caller)
{
	if (unlikely(seninf_rproc_node == NULL)) {
		dev_info(dev,
			"[%s][%s] ERROR: seninf rproc node is null because the DTS compatiable node:'%s' can't be found\n",
			__func__, caller, SENINF_RPROC_COMP_NAME);
		return -1;
	}
	return 0;
}

static void get_seninf_rproc_node(struct device *dev, const char *caller)
{
	struct device_node *node = NULL;

	node = of_find_compatible_node(dev->of_node, NULL, SENINF_RPROC_COMP_NAME);
	if (unlikely(node == NULL)) {
		seninf_rproc_node = NULL;
		dev_info(dev,
			"[%s][%s] ERROR: can't find DTS compatiable node:'%s'\n",
			__func__, caller, SENINF_RPROC_COMP_NAME);
		return;
	}
	seninf_rproc_node = node;
}

static int get_rproc_phandle_by_prop_name(struct device *dev,
	const char *prop_name, phandle *phandle_ptr, const char *caller)
{
	int ret = 0;

	if (unlikely(is_seninf_rproc_node_valid(dev, __func__) != 0))
		return 1;
	/* return: 0 on success, */
	/*         -EINVAL    : if the property does not exist, */
	/*         -ENODATA   : if property does not have a value, and */
	/*         -EOVERFLOW : if the property data isn't large enough */
	ret = of_property_read_u32(seninf_rproc_node, prop_name, phandle_ptr);
	if (unlikely(ret < 0)) {
		dev_info(dev,
			"[%s][%s]: ERROR: can't find property:'%s', ret:%d\n",
			__func__, caller, prop_name, ret);
	}
	return ret;
}

static struct device_node *get_rproc_node_by_phandle(struct device *dev,
	phandle handle, const char *caller)
{
	struct device_node *rproc_np = NULL;

	rproc_np = of_find_node_by_phandle(handle);
	if (unlikely(rproc_np == NULL)) {
		dev_info(dev,
			"[%s][%s]: ERROR: can't find rproc node by phandle\n",
			__func__, caller);
	}
	return rproc_np;
}

static struct platform_device *get_rproc_dev_by_node(struct device *dev,
	struct device_node *node, const char *caller)
{
	struct platform_device *pdev = NULL;

	pdev = of_find_device_by_node(node);
	if (unlikely(pdev == NULL)) {
		dev_info(dev,
			"[%s][%s]: ERROR: can't find rproc platform device by node\n",
			__func__, caller);
	}
	return pdev;
}

/******************************************************************************/
// public / basic function
/******************************************************************************/

/*----------------------------------------------------------------------------*/
// ccu rproc
/*----------------------------------------------------------------------------*/
int mtk_cam_seninf_rproc_ccu_pwr_en(struct device *dev,
	struct seninf_rproc_ccu_ctrl *p_ccu_ctrl, const unsigned int flag,
	const char *caller)
{
	int ret = 0;

	/* error handle */
	if (unlikely(p_ccu_ctrl->rproc_handle == NULL)) {
		dev_info(dev,
			"[%s][%s] ERROR: rproc handle is null\n",
			__func__, caller);
		return SENINF_RPROC_ERROR;
	}

	if (flag > 0) {
		/* bootup ccu */
#ifndef RM_CCU_DEPENDENT
		ret = rproc_bootx(p_ccu_ctrl->rproc_handle, SENINF_RPROC_CCU_UID);
		// ret = rproc_boot(p_ccu_ctrl->rproc_handle);
		if (unlikely(ret != 0)) {
			dev_info(dev,
				"[%s][%s] ERROR: pwn_cnt:%d, rproc boot failed, ret:%d\n",
				__func__, caller,
				atomic_read(&p_ccu_ctrl->pwn_cnt), ret);
			return ret;
		}
#endif
		atomic_inc(&p_ccu_ctrl->pwn_cnt);
	} else {
		/* shutdown ccu */
#ifndef RM_CCU_DEPENDENT
		ret = rproc_shutdownx(p_ccu_ctrl->rproc_handle, SENINF_RPROC_CCU_UID);
		// ret = rproc_shutdown(p_ccu_ctrl->rproc_handle);
		if (unlikely(ret != 0)) {
			dev_info(dev,
				"[%s][%s] ERROR: pwn_cnt:%d, rproc shutdown failed, ret:%d\n",
				__func__, caller,
				atomic_read(&p_ccu_ctrl->pwn_cnt), ret);
			return ret;
		}
#endif
		atomic_dec(&p_ccu_ctrl->pwn_cnt);
	}

	dev_dbg(dev,
		"[%s] pwr_cnt:%d, flag:%u(bootup(1)/shutdown(0))\n",
		__func__, atomic_read(&p_ccu_ctrl->pwn_cnt), flag);

	return ret;
}

int mtk_cam_seninf_rproc_ccu_ipc_send(struct device *dev,
	struct seninf_rproc_ccu_ctrl *p_ccu_ctrl, const unsigned int ccu_msg_id,
	void *p_data, const unsigned int data_size,
	const char *caller)
{
	int ret = 0;

	/* error handle */
	if (unlikely(p_ccu_ctrl->pdev == NULL)) {
		dev_info(dev,
			"[%s][%s] ERROR: rproc platform dev is null\n",
			__func__, caller);
		return SENINF_RPROC_ERROR;
	}

#ifndef RM_CCU_DEPENDENT
	ret = mtk_ccu_rproc_ipc_send(p_ccu_ctrl->pdev,
		SENINF_RPROC_CCU_FEATURE_TYPE, ccu_msg_id, p_data, data_size);
	if (unlikely(ret != 0)) {
		dev_info(dev,
			"[%s][%s] ERROR: msg_id:%u, p_data:%p, data_size:%u, ipc senc failed, ret:%d\n",
			__func__, caller, ccu_msg_id, p_data, data_size, ret);
	}
#endif

	return ret;
}

void mtk_cam_seninf_rproc_init_ccu_ctrl(struct device *dev,
	struct seninf_rproc_ccu_ctrl *p_ccu_ctrl)
{
	struct platform_device *pdev;
	struct device_node *rproc_np;
	phandle handle;
	int ret;

	/* reset/init */
	memset(p_ccu_ctrl, 0, sizeof(*p_ccu_ctrl));
	atomic_set(&p_ccu_ctrl->pwn_cnt, 0);

	if (seninf_rproc_node == NULL)
		get_seninf_rproc_node(dev, __func__);

	/* use seninf rproc node to find ccu phandle by node's property */
	ret = get_rproc_phandle_by_prop_name(dev,
		SENINF_RPROC_CCU_PROP_NAME, &handle, __func__);
	if (unlikely(ret != 0))
		return;
	/* use phandle to find rproc node */
	rproc_np = get_rproc_node_by_phandle(dev, handle, __func__);
	if (unlikely(rproc_np == NULL))
		return;
	/* use rproc node to find rproc platform dev */
	pdev = get_rproc_dev_by_node(dev, rproc_np, __func__);
	if (unlikely(pdev == NULL))
		return;

	/* init/setup rproc ccu ctx */
	p_ccu_ctrl->pdev = pdev;
	p_ccu_ctrl->rproc_handle = rproc_get_by_phandle(handle);
	if (unlikely(p_ccu_ctrl->rproc_handle == NULL)) {
		dev_info(dev,
			"[%s] ERROR: get ccu rproc_handle:%p failed, handle:%u, pdev:'%s'\n",
			__func__, p_ccu_ctrl->rproc_handle, handle, pdev->name);
		return;
	}

	dev_info(dev,
		"[%s] ccu_ctrl:(pdev:'%s', rproc_handle:%p, pwn_cnt:%d)\n",
		__func__, p_ccu_ctrl->pdev->name, p_ccu_ctrl->rproc_handle,
		atomic_read(&p_ccu_ctrl->pwn_cnt));
}
/*----------------------------------------------------------------------------*/


/******************************************************************************/
// public / user define function
/******************************************************************************/
void mtk_cam_seninf_rproc_ccu_ctrl(struct device *dev,
	struct seninf_rproc_ccu_ctrl *p_ccu_ctrl, const unsigned int ccu_msg_id[],
	const unsigned int msg_id_cnt, const char *caller)
{
	const int curr_pwn_cnt = atomic_read(&p_ccu_ctrl->pwn_cnt);
	void *p_data = NULL;
	unsigned int i = 0;
	unsigned int data_size = 0;
	int ret;

	/* first, check if seninf rproc exist */
	if (unlikely(is_seninf_rproc_node_valid(dev, __func__) != 0))
		return;

	/* boot ccu */
	ret = mtk_cam_seninf_rproc_ccu_pwr_en(dev, p_ccu_ctrl, 1, __func__);
	if (unlikely(ret != 0))
		return;

	for (i = 0; i < msg_id_cnt; i ++) {
		/* by id choose correct work */
		switch (ccu_msg_id[i]) {
		case MSG_TO_CCU_SENINF_TSREC_IRQ_SEL_CTRL:
			{
				struct tsrec_irq_sel_info *tsrec_irq_sel = NULL;

				tsrec_irq_sel = kmalloc(sizeof(struct tsrec_irq_sel_info), GFP_KERNEL);
				if (unlikely(tsrec_irq_sel == NULL)) {
					dev_info(dev, "[%s] ERROR: alloc resource failed msg_id: %u\n",
						 __func__, ccu_msg_id[i]);
					continue;
				}
				memset(tsrec_irq_sel, 0, sizeof(struct tsrec_irq_sel_info));

				mtk_cam_seninf_tsrec_g_irq_sel_info(tsrec_irq_sel);

				p_data = tsrec_irq_sel;
				data_size = sizeof(struct tsrec_irq_sel_info);
			}
			break;
		case MSG_TO_CCU_SENINF_DEVICE_GRP_SEL_CTRL:
			{
				struct mtk_cam_seninf_dev *seninf_dev_sel = NULL;

				seninf_dev_sel = kmalloc(sizeof(struct mtk_cam_seninf_dev), GFP_KERNEL);
				if (unlikely(seninf_dev_sel == NULL)) {
					dev_info(dev, "[%s] ERROR: alloc resource failed msg_id: %u\n",
						 __func__, ccu_msg_id[i]);
					continue;
				}
				memset(seninf_dev_sel, 0, sizeof(struct mtk_cam_seninf_dev));

				g_seninf_ops->_get_device_sel_setting(dev, seninf_dev_sel);

				p_data = seninf_dev_sel;
				data_size = sizeof(struct mtk_cam_seninf_dev);
			}
			break;
		default:
			dev_info(dev,
				"[%s] ERROR: unknown msg_id:%u, skip ipc send\n",
				__func__, ccu_msg_id[i]);
			break;
		}

		/* send ipc msg to ccu */
		if (likely(p_data)) {
			mtk_cam_seninf_rproc_ccu_ipc_send(dev,
				p_ccu_ctrl, ccu_msg_id[i],
				p_data, data_size, __func__);

			kfree(p_data);
			p_data = NULL;
		}
	}

	/* shutdown ccu */
	mtk_cam_seninf_rproc_ccu_pwr_en(dev, p_ccu_ctrl, 0, __func__);

	// dev_dbg(dev,
	dev_info(dev,
		"[%s] pwn_cnt:(%d => %d), ccu_msg_id_cnt:%u\n",
		__func__, curr_pwn_cnt, atomic_read(&p_ccu_ctrl->pwn_cnt),
		msg_id_cnt);
}

void mtk_cam_seninf_rproc_ccu_ctrl_with_para(struct device *dev,
	struct seninf_rproc_ccu_ctrl *p_ccu_ctrl, const unsigned int ccu_msg_id,
	const void *para, const char *caller)
{
	const int curr_pwn_cnt = atomic_read(&p_ccu_ctrl->pwn_cnt);
	void *p_data = NULL;
	unsigned int data_size = 0;
	int ret;

	/* first, check if seninf rproc exist */
	if (unlikely(is_seninf_rproc_node_valid(dev, __func__) != 0))
		return;

	/* boot ccu */
	ret = mtk_cam_seninf_rproc_ccu_pwr_en(dev, p_ccu_ctrl, 1, __func__);
	if (unlikely(ret != 0))
		return;

	if (para) {
		switch (ccu_msg_id) {
		case MSG_TO_CCU_SENINF_MIPI_SPLIT_CTRL:
			{
				struct mtk_cam_seninf_async_split *split_info = NULL;

				split_info = kmalloc(sizeof(struct mtk_cam_seninf_async_split), GFP_KERNEL);
				if (unlikely(split_info == NULL)) {
					dev_info(dev, "[%s] ERROR: alloc resource failed msg_id: %u\n",
						 __func__, ccu_msg_id);
					break;
				}
				memcpy(split_info, para, sizeof(struct mtk_cam_seninf_async_split));

				p_data = split_info;
				data_size = sizeof(struct mtk_cam_seninf_async_split);
			}
			break;
		default:
			dev_info(dev,
				"[%s] ERROR: unknown msg_id:%u, skip ipc send\n",
				__func__, ccu_msg_id);
			break;
		}

		/* send ipc msg to ccu */
		if (likely(p_data)) {
			mtk_cam_seninf_rproc_ccu_ipc_send(dev,
				p_ccu_ctrl, ccu_msg_id,
				p_data, data_size, __func__);

			kfree(p_data);
			p_data = NULL;
		}
	}

	/* shutdown ccu */
	mtk_cam_seninf_rproc_ccu_pwr_en(dev, p_ccu_ctrl, 0, __func__);

	dev_info(dev,
		"[%s] pwn_cnt:(%d => %d), ccu_msg_id:%u\n",
		__func__, curr_pwn_cnt, atomic_read(&p_ccu_ctrl->pwn_cnt),
		ccu_msg_id);
}

