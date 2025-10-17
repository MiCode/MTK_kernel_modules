// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 */
/*include linux common header*/
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
/*include imgsys header*/
#include "mtk_imgsys-dev.h"
#include "mtk_imgsys_frm_sync.h"
#include "mtk_imgsys_frm_sync_isp8.h"

#define FRM_SYNC_DEVNAME             "mtk_img_frm_sync"
//static struct mtk_img_frm_sync *frm_sync_dev;

static int mtk_imgsys_frm_sync_open(struct inode *inode, struct file *file)
{
	struct mtk_img_frm_sync *mtk_img_frm_sync_dev = NULL;

	mtk_img_frm_sync_dev = container_of(inode->i_cdev, struct mtk_img_frm_sync, frm_sync_cdev);
	mtk_img_frm_sync_dev->is_open = true;
	mtk_img_frm_sync_dev->data->open();
	return 0;
}

static int mtk_imgsys_frm_sync_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations mtk_imgsys_frm_sync_fops = {
	.owner          = THIS_MODULE,
	//.unlocked_ioctl = mtk_imgsys_frm_sync_ioctl,
	.open           = mtk_imgsys_frm_sync_open,
	.release        = mtk_imgsys_frm_sync_release,
};

int mtk_imgsys_frm_sync_init(struct platform_device *pdev, struct group group)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if ((frm_sync_dev == NULL)
		|| (frm_sync_dev->data == NULL)
		|| (frm_sync_dev->data->init == NULL)) {
		dev_info(&pdev->dev, "%s init not support", __func__);
		return -ENOENT;
	}

	ret = frm_sync_dev->data->init(frm_sync_dev, group);
	dev_info(&pdev->dev, "%s-group num(%d)", __func__, group.hw_group_id);
	return 0;
}
EXPORT_SYMBOL(mtk_imgsys_frm_sync_init);

int mtk_imgsys_frm_sync_uninit(struct platform_device *pdev, struct group group)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if ((frm_sync_dev == NULL)
		|| (frm_sync_dev->data == NULL)
		|| (frm_sync_dev->data->uninit == NULL)) {
		dev_info(&pdev->dev, "%s uninit not support", __func__);
		return -ENOENT;
	}

	ret = frm_sync_dev->data->uninit(frm_sync_dev, group);
	return 0;
}
EXPORT_SYMBOL(mtk_imgsys_frm_sync_uninit);

int Handler_frame_token_sync_imgsys(struct platform_device *pdev, struct imgsys_in_data *in_data,
	struct imgsys_out_data *out_data)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if ((frm_sync_dev == NULL)
		|| (frm_sync_dev->data == NULL)
		|| (frm_sync_dev->data->Handler_frame_token_sync_imgsys == NULL)) {
		dev_info(&pdev->dev, "%s frm token imgsys not support", __func__);
		return -ENOENT;
	}

	ret = frm_sync_dev->data->Handler_frame_token_sync_imgsys(frm_sync_dev, in_data, out_data);
	return 0;
}
EXPORT_SYMBOL(Handler_frame_token_sync_imgsys);

int Handler_frame_token_sync_DPE(struct platform_device *pdev, struct dpe_in_data *in_data,
	struct dpe_out_data *out_data)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if ((frm_sync_dev == NULL)
		|| (frm_sync_dev->data == NULL)
		|| (frm_sync_dev->data->Handler_frame_token_sync_DPE == NULL)) {
		dev_info(&pdev->dev, "%s frm token DPE not support", __func__);
		return -ENOENT;
	}

	ret = frm_sync_dev->data->Handler_frame_token_sync_DPE(frm_sync_dev, in_data, out_data);
	return 0;
}
EXPORT_SYMBOL(Handler_frame_token_sync_DPE);

int Handler_frame_token_sync_MAE(struct platform_device *pdev, struct mae_in_data *in_data,
	struct mae_out_data *out_data)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if ((frm_sync_dev == NULL)
		|| (frm_sync_dev->data == NULL)
		|| (frm_sync_dev->data->Handler_frame_token_sync_MAE == NULL)) {
		dev_info(&pdev->dev, "%s frm token MAE not support", __func__);
		return -ENOENT;
	}

	ret = frm_sync_dev->data->Handler_frame_token_sync_MAE(frm_sync_dev, in_data, out_data);
	return 0;
}
EXPORT_SYMBOL(Handler_frame_token_sync_MAE);

int release_frame_token_imgsys(struct platform_device *pdev, struct imgsys_deque_done_in_data *in_data)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if ((frm_sync_dev == NULL)
		|| (frm_sync_dev->data == NULL)
		|| (frm_sync_dev->data->release_frame_token_imgsys == NULL)) {
		dev_info(&pdev->dev, "%s frm token imgsys not support", __func__);
		return -ENOENT;
	}

	ret = frm_sync_dev->data->release_frame_token_imgsys(frm_sync_dev, in_data);
	return 0;
}
EXPORT_SYMBOL(release_frame_token_imgsys);

int release_frame_token_DPE(struct platform_device *pdev, struct dpe_deque_done_in_data *in_data)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if ((frm_sync_dev == NULL)
		|| (frm_sync_dev->data == NULL)
		|| (frm_sync_dev->data->release_frame_token_DPE == NULL)) {
		dev_info(&pdev->dev, "%s frm token DPE not support", __func__);
		return -ENOENT;
	}

	ret = frm_sync_dev->data->release_frame_token_DPE(frm_sync_dev, in_data);
	return 0;
}
EXPORT_SYMBOL(release_frame_token_DPE);

int release_frame_token_MAE(struct platform_device *pdev, struct mae_deque_done_in_data *in_data)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if ((frm_sync_dev == NULL)
		|| (frm_sync_dev->data == NULL)
		|| (frm_sync_dev->data->release_frame_token_MAE == NULL)) {
		dev_info(&pdev->dev, "%s frm token MAE not support", __func__);
		return -ENOENT;
	}

	ret = frm_sync_dev->data->release_frame_token_MAE(frm_sync_dev, in_data);
	return 0;
}
EXPORT_SYMBOL(release_frame_token_MAE);

int clear_token_user(struct platform_device *pdev, unsigned long frm_owner, unsigned long imgstm_inst)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);
	int ret = 0;

	dev_info(frm_sync_dev->dev, "clear token user %s", (char *)(&frm_owner));

	if ((frm_sync_dev == NULL)
		|| (frm_sync_dev->data == NULL)
		|| (frm_sync_dev->data->clear_token_user == NULL)) {
		dev_info(&pdev->dev, "%s frm token imgsys not support", __func__);
		return -ENOENT;
	}

	ret = frm_sync_dev->data->clear_token_user(frm_sync_dev, frm_owner, imgstm_inst);
	return 0;
}
EXPORT_SYMBOL(clear_token_user);

int mtk_imgsys_frm_sync_timeout(struct platform_device *pdev, int gce_event_id)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if ((frm_sync_dev == NULL)
		|| (frm_sync_dev->data == NULL)
		|| (frm_sync_dev->data->frm_sync_timeout == NULL)) {
		dev_info(&pdev->dev, "%s frm sync timeout not support", __func__);
		return -ENOENT;
	}

	ret = frm_sync_dev->data->frm_sync_timeout(frm_sync_dev, gce_event_id);
	return ret;
}
EXPORT_SYMBOL(mtk_imgsys_frm_sync_timeout);

static int mtk_imgsys_frm_sync_probe(struct platform_device *pdev)
{
	struct mtk_img_frm_sync *frm_sync_dev = NULL;
	int ret = 0;

	dev_info(&pdev->dev, "- E. frm sync driver probe.\n");
	frm_sync_dev = devm_kzalloc(&pdev->dev, sizeof(*frm_sync_dev), GFP_KERNEL);
	if (frm_sync_dev == NULL)
		return -ENOMEM;

	frm_sync_dev->dev = &pdev->dev;
	frm_sync_dev->data = of_device_get_match_data(&pdev->dev);
	platform_set_drvdata(pdev, frm_sync_dev);
	dev_set_drvdata(&pdev->dev, frm_sync_dev);
	frm_sync_dev->is_open = false;

	/* init character device */

	ret = alloc_chrdev_region(&frm_sync_dev->frm_sync_devno, 0, 1, FRM_SYNC_DEVNAME);
	if (ret < 0) {
		dev_info(&pdev->dev, "alloc_chrdev_region failed err= %d", ret);
		goto err_alloc;
	}

	cdev_init(&frm_sync_dev->frm_sync_cdev, &mtk_imgsys_frm_sync_fops);
	frm_sync_dev->frm_sync_cdev.owner = THIS_MODULE;

	ret = cdev_add(&frm_sync_dev->frm_sync_cdev, frm_sync_dev->frm_sync_devno, 1);
	if (ret < 0) {
		dev_info(&pdev->dev, "cdev_add fail  err= %d", ret);
		goto err_add;
	}
#ifdef NEW_KERNEL_API
	frm_sync_dev->frm_sync_class = class_create("mtk_frm_sync_driver");
#else
	frm_sync_dev->frm_sync_class = class_create(THIS_MODULE, "mtk_frm_sync_driver");
#endif
	if (IS_ERR(frm_sync_dev->frm_sync_class) == true) {
		ret = (int)PTR_ERR(frm_sync_dev->frm_sync_class);
		dev_info(&pdev->dev, "class create fail  err= %d", ret);
		goto err_add;
	}

	frm_sync_dev->frm_sync_device = device_create(frm_sync_dev->frm_sync_class, NULL,
					frm_sync_dev->frm_sync_devno, NULL, FRM_SYNC_DEVNAME);
	if (IS_ERR(frm_sync_dev->frm_sync_device) == true) {
		ret = (int)PTR_ERR(frm_sync_dev->frm_sync_device);
		dev_info(&pdev->dev, "device create fail  err= %d", ret);
		goto err_device;
	}

	/* parse hardware event */
	dpe_event_init_isp8(frm_sync_dev);
	mtk_img_frm_sync_init_isp8(frm_sync_dev);

	dev_info(&pdev->dev, "- X. frm sync driver probe success.\n");
	return 0;

err_device:
	class_destroy(frm_sync_dev->frm_sync_class);
err_add:
	cdev_del(&frm_sync_dev->frm_sync_cdev);
err_alloc:
	unregister_chrdev_region(frm_sync_dev->frm_sync_devno, 1);

	devm_kfree(&pdev->dev, frm_sync_dev);

	dev_info(&pdev->dev, "- X. frm sync driver probe fail.\n");

	return ret;
}

static int mtk_imgsys_frm_sync_remove(struct platform_device *pdev)
{
	struct mtk_img_frm_sync *frm_sync_dev = platform_get_drvdata(pdev);

	mtk_img_frm_sync_uninit_isp8(frm_sync_dev);
	devm_kfree(&pdev->dev, frm_sync_dev);
	cdev_del(&frm_sync_dev->frm_sync_cdev);
	unregister_chrdev_region(frm_sync_dev->frm_sync_devno, 1);
	return 0;
}

static const struct of_device_id mtk_imgsys_frm_sync_of_match[] = {
	{ .compatible = "mediatek,imgsys-frm-sync-isp8", .data = (void *)&mtk_img_frm_sync_data_isp8},
	{}
};

MODULE_DEVICE_TABLE(of, mtk_imgsys_frm_sync_of_match);

static struct platform_driver mtk_imgsys_frm_sync_driver = {
	.probe   = mtk_imgsys_frm_sync_probe,
	.remove  = mtk_imgsys_frm_sync_remove,
	.shutdown = NULL,
	.driver  = {
		.name = "imgsys-frm-sync",
		.owner	= THIS_MODULE,
		.pm = NULL,
		.of_match_table = of_match_ptr(mtk_imgsys_frm_sync_of_match),
	}
};

module_platform_driver(mtk_imgsys_frm_sync_driver);

MODULE_AUTHOR("Marvin Lin <Marvin.Lin@mediatek.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek imgsys driver");
