// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Daniel Huang <daniel.huang@mediatek.com>
 *
 */
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include "mtk_imgsys_cmdq_token.h"
/* TODO */
#include "mtk_imgsys_frm_sync.h"

struct platform_device *frm_sync_pdev;

unsigned int imgsys_cmdq_is_vsdof_event(uint32_t event)
{
	unsigned int ret = 0;

	if (TOKEN_GROUP(event) == mtk_imgsys_frm_sync_event_group_vsdof)
		ret = 1;

	if (ret)
		pr_info("%s: ret(%d) event(0x%x)\n", __func__, ret, event);

	return ret;
}

unsigned int imgsys_cmdq_try_vsdof_wfe(struct token_data *data, struct cmdq_pkt *pkt,
										uint32_t event)
{
	unsigned int ret = 1;
	struct imgsys_in_data in = {0};
	struct imgsys_out_data out = {0};

	if(!frm_sync_pdev)
		return 1;

	if (imgsys_cmdq_is_vsdof_event(event)) {
		in.frm_owner = data->frm_owner;
		in.req_fd = data->req_fd;
		in.req_no = data->req_no;
		in.r_idx = data->sw_ridx;
		in.token_list.mSyncTokenNum = 1;
		in.token_list.mSyncTokenList[0].type = imgsys_token_wait;
		in.token_list.mSyncTokenList[0].token_value = event;
		Handler_frame_token_sync_imgsys(frm_sync_pdev, &in, &out);
		cmdq_pkt_wfe(pkt, out.event_id);
		ret = 0;
	}

	return ret;
}

unsigned int imgsys_cmdq_try_vsdof_wfe_no_clear(struct token_data *data, struct cmdq_pkt *pkt,
										uint32_t event)
{
	unsigned int ret = 1;
	struct imgsys_in_data in = {0};
	struct imgsys_out_data out = {0};


	if(!frm_sync_pdev)
		return 1;

	if (imgsys_cmdq_is_vsdof_event(event)) {
		in.frm_owner = data->frm_owner;
		in.req_fd = data->req_fd;
		in.req_no = data->req_no;
		in.r_idx = data->sw_ridx;
		in.token_list.mSyncTokenNum = 1;
		in.token_list.mSyncTokenList[0].type = imgsys_token_wait;
		in.token_list.mSyncTokenList[0].token_value = event;

		Handler_frame_token_sync_imgsys(frm_sync_pdev, &in, &out);
		cmdq_pkt_wait_no_clear(pkt, out.event_id);
		ret = 0;
	}


	return ret;
}

unsigned int imgsys_cmdq_try_vsdof_set_event(struct token_data *data, struct cmdq_pkt *pkt,
										uint32_t event)

{
	unsigned int ret = 1;
	struct imgsys_in_data in = {0};
	struct imgsys_out_data out = {0};

	if(!frm_sync_pdev)
		return 1;

	if (imgsys_cmdq_is_vsdof_event(event)) {

		in.frm_owner = data->frm_owner;
		in.req_fd = data->req_fd;
		in.req_no = data->req_no;
		in.r_idx = data->sw_ridx;
		in.token_list.mSyncTokenNum = 1;
		//n.token_list.mSyncTokenNotifyList[0] = event;
		in.token_list.mSyncTokenList[0].type = imgsys_token_set;
		in.token_list.mSyncTokenList[0].token_value = event;

		Handler_frame_token_sync_imgsys(frm_sync_pdev, &in, &out);
		cmdq_pkt_set_event(pkt, out.event_id);
		ret = 0;
	}

	return ret;
}

unsigned int imgsys_cmdq_try_vsdof_clear_event(struct token_data *data, struct cmdq_pkt *pkt,
										uint32_t event)

{
	unsigned int ret = 1;
	struct imgsys_in_data in = {0};
	struct imgsys_out_data out = {0};

	if(!frm_sync_pdev)
		return 1;

	if (imgsys_cmdq_is_vsdof_event(event)) {

		in.frm_owner = data->frm_owner;
		in.req_fd = data->req_fd;
		in.req_no = data->req_no;
		in.r_idx = data->sw_ridx;
		//in.token_list.mSyncTokenNotifyList[0] = event;
		in.token_list.mSyncTokenList[0].type = imgsys_token_set;
		in.token_list.mSyncTokenList[0].token_value = event;

		Handler_frame_token_sync_imgsys(frm_sync_pdev, &in, &out);
		cmdq_pkt_set_event(pkt, out.event_id);
		ret = 0;
	}

	return ret;
}


unsigned int imgsys_cmdq_release_token_vsdof(int sw_ridx)
{
	unsigned int ret = 0;
	struct imgsys_deque_done_in_data data = {0};

	if(!frm_sync_pdev)
		return 1;

	data.sw_ridx = sw_ridx;
	release_frame_token_imgsys(frm_sync_pdev, &data);

	return ret;
}

struct platform_device *imgsys_cmdq_set_frm_sync_pdev(struct device *dev)
{
	struct device_node *node;
	struct platform_device *img_frm_sync_pdev = NULL;

	node = of_parse_phandle(dev->of_node, "mediatek,img-frm-sync", 0);
	if (node == NULL) {
		dev_info(dev, "%s can't get img_frm_sync node.\n", __func__);
		return NULL;
	}

	img_frm_sync_pdev = of_find_device_by_node(node);
	if (WARN_ON(img_frm_sync_pdev == NULL) == true) {
		dev_info(dev, "%s img_frm_sync pdev failed.\n", __func__);
		of_node_put(node);
		return NULL;
	}
	frm_sync_pdev = img_frm_sync_pdev;
	pr_info("%s: pdev(0x%lx)\n", __func__, (unsigned long)frm_sync_pdev);

	return img_frm_sync_pdev;
}
EXPORT_SYMBOL(imgsys_cmdq_set_frm_sync_pdev);

unsigned int imgsys_cmdq_frm_sync_init(void)
{
	struct group token_group;
	int ret;

	token_group.hw_group_id = imgsys_engine;
	token_group.algo_group_id = algo_all;
	ret = mtk_imgsys_frm_sync_init(frm_sync_pdev, token_group);

	return ret;
}
EXPORT_SYMBOL(imgsys_cmdq_frm_sync_init);

unsigned int imgsys_cmdq_frm_sync_dump_event_info(int event)
{
	return mtk_imgsys_frm_sync_timeout(frm_sync_pdev, event);
}
EXPORT_SYMBOL(imgsys_cmdq_frm_sync_dump_event_info);

unsigned int imgsys_cmdq_frm_sync_uninit(void)
{
	struct group token_group;
	int ret;

	token_group.hw_group_id = imgsys_engine;
	token_group.algo_group_id = algo_all;
	ret = mtk_imgsys_frm_sync_uninit(frm_sync_pdev, token_group);

	return ret;
}
EXPORT_SYMBOL(imgsys_cmdq_frm_sync_uninit);
