/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_CAM_SENINF_RPROC_CTRL_H__
#define __MTK_CAM_SENINF_RPROC_CTRL_H__

#include <linux/of_platform.h>
#include <linux/atomic.h>


// #define RM_CCU_DEPENDENT

#define SENINF_RPROC_ERROR                    (2147483647)


/******************************************************************************/
// !!! for talk to rproc, struct / enum / define !!!
/******************************************************************************/

/*----------------------------------------------------------------------------*/
// => for ccu rproc
/*----------------------------------------------------------------------------*/
/* MUST sync to "ccu_seninfctrl_extif.h" */
enum ccu_msg_id_seninfctrl {
	SENINFCTRL_MSG_MIN = 0,

	/* Receive by CCU (start from 1) */
	MSG_TO_CCU_SENINF_TSREC_IRQ_SEL_CTRL, /* 1 */
	MSG_TO_CCU_SENINF_DEVICE_GRP_SEL_CTRL,
	MSG_TO_CCU_SENINF_MIPI_SPLIT_CTRL,

	SENINFCTRL_MSG_MAX
};
/*----------------------------------------------------------------------------*/


/******************************************************************************/
// !!! seninf rproc struct / enum / basic function !!!
/******************************************************************************/

/*----------------------------------------------------------------------------*/
// => for ccu rproc
/*----------------------------------------------------------------------------*/
/* in dts */
#define SENINF_RPROC_COMP_NAME                ("mediatek,seninf-rproc")
#define SENINF_RPROC_CCU_PROP_NAME            ("mediatek,ccu-rproc")

/* in linux/remoteproc.h */
#define SENINF_RPROC_CCU_UID                  (RPROC_UID_SENINF)
/* in linux/remoteproc/mtk_ccu.h */
#define SENINF_RPROC_CCU_FEATURE_TYPE         (MTK_CCU_FEATURE_SENINF)

struct seninf_rproc_ccu_ctrl {
	struct platform_device *pdev;
	struct rproc *rproc_handle;
	atomic_t pwn_cnt;    /* debug info, ccu has itself ref cnt */
};

/* return: 0 on success */
int mtk_cam_seninf_rproc_ccu_pwr_en(struct device *dev,
	struct seninf_rproc_ccu_ctrl *p_ccu_ctrl, const unsigned int flag,
	const char *caller);

/* return: 0 on success */
int mtk_cam_seninf_rproc_ccu_ipc_send(struct device *dev,
	struct seninf_rproc_ccu_ctrl *p_ccu_ctrl, const unsigned int ccu_msg_id,
	void *p_data, const unsigned int data_size,
	const char *caller);

void mtk_cam_seninf_rproc_init_ccu_ctrl(struct device *dev,
	struct seninf_rproc_ccu_ctrl *p_ccu_ctrl);
/*----------------------------------------------------------------------------*/


/******************************************************************************/
// !!! seninf rproc user define function !!!
/******************************************************************************/
void mtk_cam_seninf_rproc_ccu_ctrl(struct device *dev,
	struct seninf_rproc_ccu_ctrl *p_ccu_ctrl, const unsigned int ccu_msg_id[],
	const unsigned int msg_id_cnt, const char *caller);

void mtk_cam_seninf_rproc_ccu_ctrl_with_para(struct device *dev,
	struct seninf_rproc_ccu_ctrl *p_ccu_ctrl, const unsigned int ccu_msg_id,
	const void *para, const char *caller);


#endif /* __MTK_CAM_SENINF_RPROC_CTRL_H__ */
