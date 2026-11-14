// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/******************************************************************************
*[File]             usb.c
*[Version]          v1.0
*[Revision Date]    2010-03-01
*[Author]
*[Description]
*    The program provides USB HIF driver
*[Copyright]
*    Copyright (C) 2010 MediaTek Incorporation. All Rights Reserved.
******************************************************************************/


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include "precomp.h"

#include <linux/usb.h>
#include <linux/mutex.h>

#if CFG_CHIP_RESET_USE_DTS_GPIO_NUM
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#endif
#if CFG_ENABLE_GKI_SUPPORT
#include <linux/gpio.h>
#endif

#include <linux/mm.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif

#include "mt66xx_reg.h"
#include "cust_usb_id.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

#define HIF_USB_ERR_TITLE_STR           "["CHIP_NAME"] USB Access Error!"
#define HIF_USB_ERR_DESC_STR            "**USB Access Error**\n"

#define HIF_USB_ACCESS_RETRY_LIMIT      1

#define MT_MAC_BASE                     0x2

#define MTK_USB_PORT_MASK               0x0F
#define MTK_USB_BULK_IN_MIN_EP          4
#define MTK_USB_BULK_IN_MAX_EP          5
#define MTK_USB_BULK_OUT_MIN_EP         4
#define MTK_USB_BULK_OUT_MAX_EP         9
#if CFG_DC_USB_WOW_CALLBACK
#ifdef MT6639
#define WIFI_POWER_OFF_DONE     (0x7C05B120)
#endif
#ifdef MT7925
#define WIFI_POWER_OFF_DONE     (0x7C059400)
#endif
#endif

static const struct usb_device_id mtk_usb_ids[] = {
	/* {USB_DEVICE(0x0E8D,0x6632), .driver_info = MT_MAC_BASE}, */
#ifdef MT6632
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x6632, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt6632},
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x7666, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt6632},
#endif /* MT6632 */
#ifdef MT7668
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x7668, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt7668},
#endif /* MT7668 */
#ifdef MT7663
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x7663, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt7663},
#endif /* MT7663 */
#ifdef MT7915
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x7915, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt7915},
#endif /* MT7915 */
#ifdef MT7961
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x7961, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt7961},
#endif /* MT7961 */
	/* If customer usb id is presented, add to the table. */
#ifdef MT6639
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x6639, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt6639},
#endif /* MT6639 */
#ifdef MT7925
	{	USB_DEVICE_AND_INTERFACE_INFO(0x0E8D, 0x7925, 0xff, 0xff, 0xff),
		.driver_info = (kernel_ulong_t)&mt66xx_driver_data_mt7925},
#endif /* MT7925 */
	CUST_USB_ID_TABLES
	{ /* end: all zeroes */ },
};

MODULE_DEVICE_TABLE(usb, mtk_usb_ids);

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static probe_card pfWlanProbe;
static remove_card pfWlanRemove;

static u_int8_t g_fgDriverProbed = FALSE;

static struct usb_driver mtk_usb_driver = {
	.name = "wlan",		/* "MTK USB WLAN Driver" */
	.id_table = mtk_usb_ids,
	.probe = NULL,
	.disconnect = NULL,
	.suspend = NULL,
	.resume = NULL,
	.reset_resume = NULL,
#if CFG_USB_AUTO_SUSPEND
	.supports_autosuspend = 1,
#else
	.supports_autosuspend = 0,
#endif
};

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static int mtk_usb_probe(struct usb_interface *intf,
				const struct usb_device_id *id);
static void mtk_usb_disconnect(struct usb_interface *intf);
static int mtk_usb_suspend(struct usb_interface *intf, pm_message_t message);
static int mtk_usb_resume(struct usb_interface *intf);
static int mtk_usb_reset_resume(struct usb_interface *intf);
static int mtk_usb_bulk_in_msg(struct GL_HIF_INFO *prHifInfo, uint32_t len,
				uint8_t *buffer, int InEp);
static int mtk_usb_intr_in_msg(struct GL_HIF_INFO *prHifInfo, uint32_t len,
				uint8_t *buffer, int InEp);
static int mtk_usb_bulk_out_msg(struct GL_HIF_INFO *prHifInfo, uint32_t len,
				uint8_t *buffer, int OutEp);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

struct mt66xx_hif_driver_data *get_platform_driver_data_by_dev(void *ctx)
{
	return get_platform_driver_data();
}

struct mt66xx_hif_driver_data *get_platform_driver_data(void)
{
	return (struct mt66xx_hif_driver_data *) mtk_usb_ids[0].driver_info;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is a USB probe function
*
* \param[in] intf   USB interface
* \param[in] id     USB device id
*
* \return void
*/
/*----------------------------------------------------------------------------*/
static int mtk_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	int ret = 0;
	struct usb_device *dev;

	DBGLOG(HAL, EVENT, "mtk_usb_probe()\n");

	ASSERT(intf);
	ASSERT(id);

	dev = interface_to_usbdev(intf);
	dev = usb_get_dev(dev);

	/* Prevent un-expected usb operation  */
	if (g_fgDriverProbed) {
		DBGLOG(HAL, ERROR, "wlan_probe(): Device already probed!!\n");
		return -EBUSY;
	}

	DBGLOG(HAL, EVENT, "wlan_probe()\n");
	if (pfWlanProbe((void *) intf, (void *) id->driver_info) != WLAN_STATUS_SUCCESS) {
		pfWlanRemove();
		DBGLOG(HAL, ERROR, "wlan_probe() failed\n");
		ret = -1;
	} else {
		g_fgDriverProbed = TRUE;
	}

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is a USB remove function
*
* \param[in] intf   USB interface
*
* \return void
*/
/*----------------------------------------------------------------------------*/
static void mtk_usb_disconnect(struct usb_interface *intf)
{
	struct GLUE_INFO *prGlueInfo;

	DBGLOG(HAL, STATE, "mtk_usb_disconnect()\n");

	ASSERT(intf);
	prGlueInfo  = (struct GLUE_INFO *)usb_get_intfdata(intf);

	glUsbSetState(&prGlueInfo->rHifInfo, USB_STATE_LINK_DOWN);
#if CFG_DC_USB_WOW_CALLBACK
	if (prGlueInfo->rHifInfo.fgUsbShutdown) {
		DBGLOG(HAL, WARN, "usb shutdown\n");
	} else
#endif
	{
	if (g_fgDriverProbed)
		pfWlanRemove();
	}
	usb_set_intfdata(intf, NULL);
	usb_put_dev(interface_to_usbdev(intf));

	g_fgDriverProbed = FALSE;

	DBGLOG(HAL, STATE, "mtk_usb_disconnect() done\n");
}

static int mtk_usb_resume(struct usb_interface *intf)
{
	int ret = 0;
	struct GLUE_INFO *prGlueInfo =
		(struct GLUE_INFO *)usb_get_intfdata(intf);
	struct BUS_INFO *prBusInfo = NULL;

	DBGLOG(HAL, STATE, "mtk_usb_resume()\n");
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	if (prBusInfo->asicUsbResume) {
		/* callback func registered for each chip */
		if (prBusInfo->asicUsbResume(prGlueInfo->prAdapter, prGlueInfo))
			ret = 0;
		else
			ret = -1;
	} else {
		/* Do general method if no callback func registered */
		/* NOTE: USB bus may not really do suspend and resume*/
		ret = usb_control_msg(prGlueInfo->rHifInfo.udev,
			      usb_sndctrlpipe(prGlueInfo->rHifInfo.udev, 0),
			      VND_REQ_FEATURE_SET,
			      prBusInfo->u4device_vender_request_out,
			      FEATURE_SET_WVALUE_RESUME, 0, NULL, 0,
			      VENDOR_TIMEOUT_MS);
		if (ret)
			DBGLOG(HAL, ERROR,
				"VendorRequest FeatureSetResume ERROR: %x\n",
				(unsigned int)ret);

		glUsbSetState(&prGlueInfo->rHifInfo, USB_STATE_PRE_RESUME);
		/* To trigger CR4 path */
		wlanSendDummyCmd(prGlueInfo->prAdapter, FALSE);

		glUsbSetState(&prGlueInfo->rHifInfo, USB_STATE_LINK_UP);
		halEnableInterrupt(prGlueInfo->prAdapter);

#if CFG_WOW_SUPPORT
		if (prGlueInfo->prAdapter->rWifiVar.ucWow) {
			DBGLOG(HAL, EVENT, "leave WOW flow\n");
			kalWowProcess(prGlueInfo, FALSE);
		}
#endif
	}

	prGlueInfo->fgIsInSuspendMode = FALSE;

	/* Allow upper layers to call the device hard_start_xmit routine. */
	netif_tx_wake_all_queues(prGlueInfo->prDevHandler);

#if CFG_CHIP_RESET_SUPPORT
	if (prGlueInfo->prAdapter->chip_info->fgIsSupportL0p5Reset) {
		cancel_work_sync(&prGlueInfo->rWfsysResetWork);

		if (glReSchWfsysReset(prGlueInfo->prAdapter))
			DBGLOG(REQ, WARN, "reschedule L0.5 reset procedure\n");
	}
#endif

	DBGLOG(HAL, STATE, "mtk_usb_resume() done ret=%d!\n", ret);

	/* TODO */
	return ret;
}

static int mtk_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)usb_get_intfdata(intf);
	uint32_t count = 0;
	struct BUS_INFO *prBusInfo = NULL;
	int ret = 0;

	DBGLOG(HAL, STATE, "mtk_usb_suspend()\n");

	if (!prGlueInfo || !prGlueInfo->prDevHandler)
		return -EPERM;

	/* TODO : support auto-suspend in stopped dev?
	* ref : history of __dev_open()
	*/
#if !CFG_USB_AUTO_SUSPEND
	if (PMSG_IS_AUTO(message) &&
		!netif_running(prGlueInfo->prDevHandler)) {
		DBGLOG(HAL, WARN, "unable suspend w/o ruuning dev\n");
		return -EPERM;
	}
#endif
	prGlueInfo->fgIsInSuspendMode = TRUE;

	/* Stop upper layers calling the device hard_start_xmit routine. */
	netif_tx_stop_all_queues(prGlueInfo->prDevHandler);

#if CFG_WOW_SUPPORT
	/* wait wiphy device do cfg80211 suspend done, then start hif suspend */
	if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWifiVar.ucWow))
		wlanWaitCfg80211SuspendDone(prGlueInfo);
#endif
	/* change to pre-Suspend state & block cfg80211 ops */
	glUsbSetState(&prGlueInfo->rHifInfo, USB_STATE_PRE_SUSPEND);

#if CFG_CHIP_RESET_SUPPORT
	if (prGlueInfo->prAdapter->chip_info->fgIsSupportL0p5Reset)
		cancel_work_sync(&prGlueInfo->rWfsysResetWork);
#endif

	wlanSuspendPmHandle(prGlueInfo);

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	if (prBusInfo->asicUsbSuspend) {
		if (prBusInfo->asicUsbSuspend(prGlueInfo->prAdapter, prGlueInfo))
			return 0;
		else
			return -1;
	}

	halUSBPreSuspendCmd(prGlueInfo->prAdapter);

	while (prGlueInfo->rHifInfo.state != USB_STATE_SUSPEND) {
		if (count > 250) {
			DBGLOG(HAL, ERROR, "pre_suspend timeout\n");
			ret = -EFAULT;
			break;
		}
		mdelay(2);
		count++;
	}

	halDisableInterrupt(prGlueInfo->prAdapter);
	halTxCancelAllSending(prGlueInfo->prAdapter);

#if CFG_WOW_SUPPORT
	/* pending cmd will be kept in queue and no one to handle it after HIF resume.
	 * In STR, it will result in cmd buf full and then cmd buf alloc fail .
	 */
	if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWifiVar.ucWow))
		wlanReleaseAllTxCmdQueue(prGlueInfo->prAdapter);
#endif

	DBGLOG(HAL, STATE, "mtk_usb_suspend() done!\n");

	if (ret && PMSG_IS_AUTO(message))
		mtk_usb_resume(intf);

	return ret;
}

static int mtk_usb_reset_resume(struct usb_interface *intf)
{
	DBGLOG(HAL, STATE, "mtk_usb_reset_resume()\n");

	mtk_usb_resume(intf);

	DBGLOG(HAL, STATE, "mtk_usb_reset_resume done!()\n");

	/* TODO */
	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief USB EP0 vendor request
*
* \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
* \param[in] uEndpointAddress
* \param[in] RequestType
* \param[in] Request
* \param[in] Value
* \param[in] Index
* \param[in] TransferBuffer
* \param[in] TransferBufferLength
*
* \retval 0          if success
*         non-zero   if fail, the return value of usb_control_msg()
*/
/*----------------------------------------------------------------------------*/
int32_t mtk_usb_vendor_request(struct GLUE_INFO *prGlueInfo,
		uint8_t uEndpointAddress, uint8_t RequestType,
	    uint8_t Request, uint16_t Value, uint16_t Index,
	    void *TransferBuffer, uint32_t TransferBufferLength)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct BUS_INFO *prBusInfo = NULL;
	void *xfer_buf;

	/* refer to RTUSB_VendorRequest */
	int ret = 0;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	/* TODO: semaphore */

	if (in_interrupt()) {
		DBGLOG(REQ, ERROR, "BUG: mtk_usb_vendor_request is called from invalid context\n");
		return -EFAULT;
	}

	if (prHifInfo == NULL) {
		DBGLOG(REQ, WARN, "prHifInfo = NULL\n");
		return -EINVAL;
	}

	if (unlikely(TransferBufferLength > prHifInfo->vendor_req_buf_sz)) {
		DBGLOG(REQ, ERROR, "len %u exceeds limit %u\n",
			TransferBufferLength,
			prHifInfo->vendor_req_buf_sz);
		return -E2BIG;
	}

	if (unlikely(TransferBuffer && !prHifInfo->vendor_req_buf)) {
		DBGLOG(REQ, ERROR, "NULL vendor_req_buf\n");
		return -EFAULT;
	}

	/* use heap instead of old stack memory */
	xfer_buf = (TransferBuffer) ? prHifInfo->vendor_req_buf : NULL;

	mutex_lock(&prHifInfo->vendor_req_sem);

	if (prHifInfo->stateSyncCtrl != USB_STATE_LINK_UP &&
	    RequestType != DEVICE_VENDOR_REQUEST_UHW_IN &&
	    RequestType != DEVICE_VENDOR_REQUEST_UHW_OUT) {
		mutex_unlock(&prHifInfo->vendor_req_sem);
		DBGLOG(HAL, WARN, "forbid usb vendor request\n");
		return -EPERM;
	}

	if (RequestType == prBusInfo->u4device_vender_request_out ||
	    RequestType == DEVICE_VENDOR_REQUEST_UHW_OUT) {
		if (xfer_buf)
			memcpy(xfer_buf, TransferBuffer, TransferBufferLength);
		ret = usb_control_msg(prHifInfo->udev,
				      usb_sndctrlpipe(prHifInfo->udev,
				      uEndpointAddress),
				      Request, RequestType, Value, Index,
				      xfer_buf, TransferBufferLength,
				      VENDOR_TIMEOUT_MS);
	} else if (RequestType == prBusInfo->u4device_vender_request_in ||
		   RequestType == DEVICE_VENDOR_REQUEST_UHW_IN) {
		ret = usb_control_msg(prHifInfo->udev,
				      usb_rcvctrlpipe(prHifInfo->udev,
				      uEndpointAddress),
				      Request, RequestType, Value, Index,
				      xfer_buf, TransferBufferLength,
				      VENDOR_TIMEOUT_MS);
		if (xfer_buf && (ret > 0))
			memcpy(TransferBuffer, xfer_buf, TransferBufferLength);
	}
	mutex_unlock(&prHifInfo->vendor_req_sem);

	return (ret == TransferBufferLength) ? 0 : ret;
}

#if CFG_DC_USB_WOW_CALLBACK
void mtk_usb_shutdown_vnd_cmd(struct GLUE_INFO *prGlueInfo)
{
	int ret = 0;
	uint32_t u4Data = 0;

	DBGLOG(INIT, ERROR, "mtk_usb_vnd_cmd\n");

	if (prGlueInfo && prGlueInfo->prAdapter) {

		HAL_MCR_RD(prGlueInfo->prAdapter, WIFI_POWER_OFF_DONE, &u4Data);
		DBGLOG(REQ, STATE, "Read data is: %x\n", u4Data);
#ifdef MT6639
		u4Data |= BIT(1);
#endif
#ifdef MT7925
		u4Data |= BIT(15);
#endif
		DBGLOG(REQ, STATE, "Write data is: %x\n", u4Data);
		HAL_MCR_WR(prGlueInfo->prAdapter, WIFI_POWER_OFF_DONE, u4Data);

		ret = usb_control_msg(prGlueInfo->rHifInfo.udev,
				usb_sndctrlpipe(prGlueInfo->rHifInfo.udev, 0),
				VND_REQ_USB_SHUTDOWN,
				DEVICE_VENDOR_REQUEST_OUT,
				0, 0, NULL, 0, 100);

		if (ret)
			DBGLOG(INIT, ERROR,
			"mtk_usb_vnd_cmd ERROR: %d\n", ret);
	}
}
#endif

/*----------------------------------------------------------------------------*/
/*!
* \brief USB Bulk IN msg
*
* \param[in] prHifInfo  Pointer to the struct GL_HIF_INFO structure
* \param[in] len
* \param[in] buffer
* \param[in] InEp
*
* \retval
*/
/*----------------------------------------------------------------------------*/
static int mtk_usb_bulk_in_msg(struct GL_HIF_INFO *prHifInfo, uint32_t len,
		uint8_t *buffer, int InEp)
{
	int ret = 0;
	uint32_t count;

	if (in_interrupt()) {
		DBGLOG(REQ, ERROR, "BUG: mtk_usb_bulk_in_msg is called from invalid context\n");
		return FALSE;
	}

	mutex_lock(&prHifInfo->vendor_req_sem);

	if (prHifInfo->stateSyncCtrl != USB_STATE_LINK_UP) {
		mutex_unlock(&prHifInfo->vendor_req_sem);
		DBGLOG(HAL, WARN, "forbid usb bulk in\n");
		return -1;
	}

	/* do a blocking bulk read to get data from the device */
	ret = usb_bulk_msg(prHifInfo->udev,
			   usb_rcvbulkpipe(prHifInfo->udev, InEp), buffer, len, &count, BULK_TIMEOUT_MS);

	mutex_unlock(&prHifInfo->vendor_req_sem);

	if (ret >= 0) {
#if 0 /* maximize buff len for usb in */
		if (count != len) {
			DBGLOG(HAL, WARN, "usb_bulk_msg(IN=%d) Warning. Data is not completed. (receive %d/%u)\n",
			       InEp, count, len);
		}
#endif
		return count;
	}

	DBGLOG(HAL, ERROR, "usb_bulk_msg(IN=%d) Fail. Error code = %d.\n", InEp, ret);
	return ret;
}

static int mtk_usb_intr_in_msg(struct GL_HIF_INFO *prHifInfo, uint32_t len,
		uint8_t *buffer, int InEp)
{
	int ret = 0;
	uint32_t count;

	if (in_interrupt()) {
		DBGLOG(REQ, ERROR, "BUG: mtk_usb_intr_in_msg is called from invalid context\n");
		return FALSE;
	}

	mutex_lock(&prHifInfo->vendor_req_sem);

	if (prHifInfo->stateSyncCtrl != USB_STATE_LINK_UP) {
		mutex_unlock(&prHifInfo->vendor_req_sem);
		DBGLOG(HAL, WARN, "forbid usb interrupt in\n");
		return -1;
	}

	/* do a blocking interrupt read to get data from the device */
	ret = usb_interrupt_msg(prHifInfo->udev,
			   usb_rcvintpipe(prHifInfo->udev, InEp), buffer, len, &count, INTERRUPT_TIMEOUT_MS);

	mutex_unlock(&prHifInfo->vendor_req_sem);

	if (ret >= 0) {
#if 0 /* maximize buff len for usb in */
		if (count != len) {
			DBGLOG(HAL, WARN, "usb_interrupt_msg(IN=%d) Warning. Data is not completed. (receive %d/%u)\n",
			       InEp, count, len);
		}
#endif
		return count;
	}

	DBGLOG(HAL, ERROR, "usb_interrupt_msg(IN=%d) Fail. Error code = %d.\n", InEp, ret);
	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief USB Bulk OUT msg
*
* \param[in] prHifInfo  Pointer to the struct GL_HIF_INFO structure
* \param[in] len
* \param[in] buffer
* \param[in] OutEp
*
* \retval
*/
/*----------------------------------------------------------------------------*/
static int mtk_usb_bulk_out_msg(struct GL_HIF_INFO *prHifInfo, uint32_t len,
		uint8_t *buffer, int OutEp)
{
	int ret = 0;
	uint32_t count;

	if (in_interrupt()) {
		DBGLOG(REQ, ERROR, "BUG: mtk_usb_bulk_out_msg is called from invalid context\n");
		return FALSE;
	}

	mutex_lock(&prHifInfo->vendor_req_sem);

	if (prHifInfo->stateSyncCtrl != USB_STATE_LINK_UP) {
		mutex_unlock(&prHifInfo->vendor_req_sem);
		DBGLOG(HAL, WARN, "forbid usb bulk out\n");
		return -1;
	}

	/* do a blocking bulk read to get data from the device */
	ret = usb_bulk_msg(prHifInfo->udev,
			   usb_sndbulkpipe(prHifInfo->udev, OutEp), buffer, len, &count, BULK_TIMEOUT_MS);

	mutex_unlock(&prHifInfo->vendor_req_sem);

	if (ret >= 0) {
#if 0
		if (count != len) {
			DBGLOG(HAL, ERROR, "usb_bulk_msg(OUT=%d) Warning. Data is not completed. (send %d/%u)\n", OutEp,
			       count, len);
		}
#endif
		return count;
	}

	DBGLOG(HAL, ERROR, "usb_bulk_msg(OUT=%d) Fail. Error code = %d.\n", OutEp, ret);
	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will register USB bus to the os
*
* \param[in] pfProbe    Function pointer to detect card
* \param[in] pfRemove   Function pointer to remove card
*
* \return The result of registering USB bus
*/
/*----------------------------------------------------------------------------*/
uint32_t glRegisterBus(probe_card pfProbe, remove_card pfRemove)
{
	uint32_t ret = 0;

	ASSERT(pfProbe);
	ASSERT(pfRemove);

	pfWlanProbe = pfProbe;
	pfWlanRemove = pfRemove;

	mtk_usb_driver.probe = mtk_usb_probe;
	mtk_usb_driver.disconnect = mtk_usb_disconnect;
	mtk_usb_driver.suspend = mtk_usb_suspend;
	mtk_usb_driver.resume = mtk_usb_resume;
	mtk_usb_driver.reset_resume = mtk_usb_reset_resume;

	ret = (usb_register(&mtk_usb_driver) == 0) ? WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE;

	return ret;
}				/* end of glRegisterBus() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function will unregister USB bus to the os
*
* \param[in] pfRemove   Function pointer to remove card
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
void glUnregisterBus(remove_card pfRemove)
{
	usb_deregister(&mtk_usb_driver);
}				/* end of glUnregisterBus() */

void glUdmaTxRxEnable(struct GLUE_INFO *prGlueInfo, u_int8_t enable)
{
	uint32_t u4Value = 0;
	struct BUS_INFO *prBusInfo;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	kalDevRegRead(prGlueInfo, prBusInfo->u4UdmaWlCfg_0_Addr, &u4Value);

	/* enable UDMA TX & RX */
	if (enable)
		u4Value |= prBusInfo->u4UdmaWlCfg_0;
	else
		u4Value &= ~prBusInfo->u4UdmaWlCfg_0;

	kalDevRegWrite(prGlueInfo, prBusInfo->u4UdmaWlCfg_0_Addr, u4Value);
}

void glUdmaRxAggEnable(struct GLUE_INFO *prGlueInfo, u_int8_t enable)
{
	uint32_t u4Value = 0;
	struct BUS_INFO *prBusInfo;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	if (enable) {
		kalDevRegRead(prGlueInfo, prBusInfo->u4UdmaWlCfg_0_Addr, &u4Value);
		/* enable UDMA TX & RX */
		u4Value &= ~(UDMA_WLCFG_0_RX_AGG_EN_MASK |
		    UDMA_WLCFG_0_RX_AGG_LMT_MASK |
		    UDMA_WLCFG_0_RX_AGG_TO_MASK);
		u4Value |= UDMA_WLCFG_0_RX_AGG_EN(1) |
		    UDMA_WLCFG_0_RX_AGG_LMT(USB_RX_AGGREGTAION_LIMIT) |
		    UDMA_WLCFG_0_RX_AGG_TO(USB_RX_AGGREGTAION_TIMEOUT);
		kalDevRegWrite(prGlueInfo, prBusInfo->u4UdmaWlCfg_0_Addr, u4Value);

		kalDevRegRead(prGlueInfo, prBusInfo->u4UdmaWlCfg_1_Addr, &u4Value);
		u4Value &= ~UDMA_WLCFG_1_RX_AGG_PKT_LMT_MASK;
		u4Value |= UDMA_WLCFG_1_RX_AGG_PKT_LMT(USB_RX_AGGREGTAION_PKT_LIMIT);
		kalDevRegWrite(prGlueInfo, prBusInfo->u4UdmaWlCfg_1_Addr, u4Value);
	} else {
		kalDevRegRead(prGlueInfo, prBusInfo->u4UdmaWlCfg_0_Addr, &u4Value);
		/* enable UDMA TX & RX */
		u4Value &= ~(UDMA_WLCFG_0_RX_AGG_LMT_MASK |
			UDMA_WLCFG_0_RX_AGG_TO_MASK);
		kalDevRegWrite(prGlueInfo,
			prBusInfo->u4UdmaWlCfg_0_Addr, u4Value);

		kalDevRegRead(prGlueInfo,
			prBusInfo->u4UdmaWlCfg_1_Addr, &u4Value);
		u4Value &= ~UDMA_WLCFG_1_RX_AGG_PKT_LMT_MASK;
		kalDevRegWrite(prGlueInfo,
			prBusInfo->u4UdmaWlCfg_1_Addr, u4Value);
	}
}

void *glUsbInitQ(struct GL_HIF_INFO *prHifInfo, struct list_head *prHead, uint32_t u4Cnt)
{
	uint32_t i;
	struct USB_REQ *prUsbReqs, *prUsbReq;

	INIT_LIST_HEAD(prHead);

	prUsbReqs = kcalloc(u4Cnt, sizeof(struct USB_REQ), GFP_ATOMIC);
	if (prUsbReqs == NULL) {
		DBGLOG(HAL, ERROR, "glUsbInitQ() kcalloc error\n");
		goto error;
	}

	prUsbReq = prUsbReqs;

	for (i = 0; i < u4Cnt; ++i) {
		prUsbReq->prHifInfo = prHifInfo;
		prUsbReq->prUrb = usb_alloc_urb(0, GFP_ATOMIC);

		if (prUsbReq->prUrb == NULL)
			DBGLOG(HAL, ERROR, "usb_alloc_urb() reports error\n");

		prUsbReq->prBufCtrl = NULL;

		INIT_LIST_HEAD(&prUsbReq->list);
		list_add_tail(&prUsbReq->list, prHead);

		prUsbReq++;
	}

error:
	return (void *) prUsbReqs;
}

void glUsbUnInitQ(struct list_head *prHead)
{
	struct USB_REQ *prUsbReq, *prUsbReqNext;

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, prHead, list) {
		usb_free_urb(prUsbReq->prUrb);
		list_del_init(&prUsbReq->list);
	}
}

void glUsbEnqueueReq(struct GL_HIF_INFO *prHifInfo, struct list_head *prHead, struct USB_REQ *prUsbReq, spinlock_t *prLock,
		     u_int8_t fgHead)
{
	unsigned long flags;

	spin_lock_irqsave(prLock, flags);
	if (fgHead)
		list_add(&prUsbReq->list, prHead);
	else
		list_add_tail(&prUsbReq->list, prHead);
	spin_unlock_irqrestore(prLock, flags);
}

struct USB_REQ *glUsbDequeueReq(struct GL_HIF_INFO *prHifInfo, struct list_head *prHead, spinlock_t *prLock)
{
	struct USB_REQ *prUsbReq;
	unsigned long flags;

	spin_lock_irqsave(prLock, flags);
	if (list_empty(prHead)) {
		spin_unlock_irqrestore(prLock, flags);
		return NULL;
	}
	prUsbReq = list_entry(prHead->next, struct USB_REQ, list);
	list_del_init(prHead->next);
	spin_unlock_irqrestore(prLock, flags);

	return prUsbReq;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function borrow UsbReq from Tx data FFA queue to the spcified TC Tx data free queue
*
* \param[in] prHifInfo  Pointer to the struct GL_HIF_INFO structure
* \param[in] ucTc       Specify TC index
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t glUsbBorrowFfaReq(struct GL_HIF_INFO *prHifInfo, uint8_t ucTc)
{
	struct USB_REQ *prUsbReq;

	if (list_empty(&prHifInfo->rTxDataFfaQ))
		return FALSE;
	prUsbReq = list_entry(prHifInfo->rTxDataFfaQ.next, struct USB_REQ, list);
	list_del_init(prHifInfo->rTxDataFfaQ.next);

	*((uint8_t *)&prUsbReq->prPriv) = FFA_MASK | ucTc;
	list_add_tail(&prUsbReq->list, &prHifInfo->rTxDataFreeQ[ucTc]);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function set USB state
*
* \param[in] prHifInfo  Pointer to the struct GL_HIF_INFO structure
* \param[in] state      Specify TC index
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
void glUsbSetState(struct GL_HIF_INFO *prHifInfo, enum usb_state state)
{
	unsigned long flags;

	spin_lock_irqsave(&prHifInfo->rStateLock, flags);
	prHifInfo->state = state;
	spin_unlock_irqrestore(&prHifInfo->rStateLock, flags);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function set USB stateSyncCtrl which is used to determine if it's
*        allowed to send synchronous usb control.
*
* \param[in] prHifInfo  Pointer to the struct GL_HIF_INFO structure
* \param[in] state      Specify new usb state
*
* \retval none
*/
/*----------------------------------------------------------------------------*/
void glUsbSetStateSyncCtrl(struct GL_HIF_INFO *prHifInfo, enum usb_state state)
{
	mutex_lock(&prHifInfo->vendor_req_sem);
	prHifInfo->stateSyncCtrl = state;
	mutex_unlock(&prHifInfo->vendor_req_sem);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function is a wrapper of submit urb to ensure driver can transmit
*        WiFi packet when WiFi path of device is allowed.
*
* \param[in] prHifInfo  Pointer to the struct GL_HIF_INFO structure
* \param[in] type       Specify submit type
*
* \retval 0             Successful submissions.
* \retval negative      Error number.
*/
/*----------------------------------------------------------------------------*/
int glUsbSubmitUrb(struct GL_HIF_INFO *prHifInfo, struct urb *urb,
			enum usb_submit_type type)
{
	unsigned long flags;
	uint32_t ret = 0;

	spin_lock_irqsave(&prHifInfo->rStateLock, flags);
	if (type == SUBMIT_TYPE_TX_CMD) {
		if (!(prHifInfo->state == USB_STATE_LINK_UP ||
			prHifInfo->state == USB_STATE_PRE_RESUME ||
			prHifInfo->state == USB_STATE_PRE_SUSPEND)) {
			spin_unlock_irqrestore(&prHifInfo->rStateLock, flags);
			DBGLOG(HAL, DEBUG,
				"not allowed to transmit CMD packet. (%d)\n",
				prHifInfo->state);
			return -ESHUTDOWN;
		}
	} else if (type == SUBMIT_TYPE_TX_DATA) {
		if (!(prHifInfo->state == USB_STATE_LINK_UP)) {
			spin_unlock_irqrestore(&prHifInfo->rStateLock, flags);
			DBGLOG(HAL, DEBUG,
				"not allowed to transmit DATA packet. (%d)\n",
				prHifInfo->state);
			return -ESHUTDOWN;
		}
	}

	if (nicSerIsTxStop(prHifInfo->prGlueInfo->prAdapter)) {
		DBGLOG(HAL, ERROR, "[SER] BYPASS USB send packet\n");
		spin_unlock_irqrestore(&prHifInfo->rStateLock, flags);
		return -EBUSY;
	}

	ret = usb_submit_urb(urb, GFP_ATOMIC);
	spin_unlock_irqrestore(&prHifInfo->rStateLock, flags);

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function stores hif related info, which is initialized before.
*
* \param[in] prGlueInfo Pointer to glue info structure
* \param[in] u4Cookie   Pointer to uint32_t memory base variable for _HIF_HPI
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
void glSetHifInfo(struct GLUE_INFO *prGlueInfo, unsigned long ulCookie)
{
	struct usb_host_interface *alts;
	struct usb_host_endpoint *ep;
	struct usb_endpoint_descriptor *ep_desc;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct USB_REQ *prUsbReq, *prUsbReqNext;
	uint32_t i;
	uint8_t ucTc;
#if CFG_CHIP_RESET_SUPPORT
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
#endif

	prHifInfo->eEventEpType = USB_EVENT_TYPE;
	prHifInfo->fgEventEpDetected = FALSE;
#if CFG_DC_USB_WOW_CALLBACK
	prHifInfo->fgUsbShutdown = FALSE;
#endif
	prHifInfo->intf = (struct usb_interface *)ulCookie;
	prHifInfo->udev = interface_to_usbdev(prHifInfo->intf);

	alts = prHifInfo->intf->cur_altsetting;
	DBGLOG(HAL, STATE, "USB Device speed: %x [%u]\n",
		prHifInfo->udev->speed, alts->endpoint[0].desc.wMaxPacketSize);

	if (prHifInfo->eEventEpType == EVENT_EP_TYPE_UNKONW) {
		for (i = 0; i < alts->desc.bNumEndpoints; ++i) {
			ep = &alts->endpoint[i];
			if (ep->desc.bEndpointAddress == USB_EVENT_EP_IN) {
				ep_desc = &alts->endpoint[i].desc;
				switch (ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
				case USB_ENDPOINT_XFER_INT:
					prHifInfo->eEventEpType = EVENT_EP_TYPE_INTR;
					break;
				case USB_ENDPOINT_XFER_BULK:
				default:
					prHifInfo->eEventEpType = EVENT_EP_TYPE_BULK;
					break;
				}
			}
		}
	}
	ASSERT(prHifInfo->eEventEpType != EVENT_EP_TYPE_UNKONW);
	DBGLOG(HAL, DEBUG, "Event EP Type: %x\n", prHifInfo->eEventEpType);

	prHifInfo->prGlueInfo = prGlueInfo;
	usb_set_intfdata(prHifInfo->intf, prGlueInfo);

	SET_NETDEV_DEV(prGlueInfo->prDevHandler, &prHifInfo->udev->dev);

	spin_lock_init(&prHifInfo->rTxCmdQLock);
	spin_lock_init(&prHifInfo->rTxDataQLock);
	spin_lock_init(&prHifInfo->rRxEventQLock);
	spin_lock_init(&prHifInfo->rRxDataQLock);
#if CFG_CHIP_RESET_SUPPORT
	if (prChipInfo->fgIsSupportL0p5Reset && prBusInfo->fgIsSupportWdtEp)
		spin_lock_init(&prHifInfo->rRxWdtQLock);
#endif
	spin_lock_init(&prHifInfo->rStateLock);

	mutex_init(&prHifInfo->vendor_req_sem);
	prHifInfo->vendor_req_buf = kzalloc(VND_REQ_BUF_SIZE, GFP_KERNEL);
	if (!prHifInfo->vendor_req_buf) {
		DBGLOG(HAL, ERROR, "kzalloc vendor_req_buf %u error\n",
			VND_REQ_BUF_SIZE);
		goto error;
	}
	prHifInfo->vendor_req_buf_sz = VND_REQ_BUF_SIZE;

	for (ucTc = 0; ucTc < USB_TC_NUM; ++ucTc) {
		prHifInfo->u4AggRsvSize[ucTc] = 0;
		init_usb_anchor(&prHifInfo->rTxDataAnchor[ucTc]);
	}
	init_usb_anchor(&prHifInfo->rRxDataAnchor);
	init_usb_anchor(&prHifInfo->rRxEventAnchor);
#if CFG_CHIP_RESET_SUPPORT
	if (prChipInfo->fgIsSupportL0p5Reset && prBusInfo->fgIsSupportWdtEp)
		init_usb_anchor(&prHifInfo->rRxWdtAnchor);
#endif

	/* TX CMD */
	prHifInfo->prTxCmdReqHead = glUsbInitQ(prHifInfo, &prHifInfo->rTxCmdFreeQ, USB_REQ_TX_CMD_CNT);
	i = 0;
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxCmdFreeQ, list) {
		prUsbReq->prBufCtrl = &prHifInfo->rTxCmdBufCtrl[i];
#ifdef CFG_PREALLOC_MEMORY
		prUsbReq->prBufCtrl->pucBuf = preallocGetMem(MEM_ID_TX_CMD);
#else
		prUsbReq->prBufCtrl->pucBuf = kmalloc(USB_TX_CMD_BUF_SIZE, GFP_ATOMIC);
#endif
		if (prUsbReq->prBufCtrl->pucBuf == NULL) {
			DBGLOG(HAL, ERROR, "kmalloc() reports error\n");
			goto error;
		}
		prUsbReq->prBufCtrl->u4BufSize = USB_TX_CMD_BUF_SIZE;
		++i;
	}

	/* TX Data FFA */
	prHifInfo->arTxDataFfaReqHead = glUsbInitQ(prHifInfo,
							&prHifInfo->rTxDataFfaQ, USB_REQ_TX_DATA_FFA_CNT);
	i = 0;
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxDataFfaQ, list) {
		QUEUE_INITIALIZE(&prUsbReq->rSendingDataMsduInfoList);
		*((uint8_t *)&prUsbReq->prPriv) = FFA_MASK;
		prUsbReq->prBufCtrl = &prHifInfo->rTxDataFfaBufCtrl[i];
#ifdef CFG_PREALLOC_MEMORY
		prUsbReq->prBufCtrl->pucBuf =
			preallocGetMem(MEM_ID_TX_DATA_FFA);
#else
		prUsbReq->prBufCtrl->pucBuf = kmalloc(USB_TX_DATA_BUFF_SIZE, GFP_ATOMIC);
#endif
		if (prUsbReq->prBufCtrl->pucBuf == NULL) {
			DBGLOG(HAL, ERROR, "kmalloc() reports error\n");
			goto error;
		}
		prUsbReq->prBufCtrl->u4BufSize = USB_TX_DATA_BUFF_SIZE;
		prUsbReq->prBufCtrl->u4WrIdx = 0;
		++i;
	}

	/* TX Data */
	for (ucTc = 0; ucTc < USB_TC_NUM; ++ucTc) {
		/* Only for TC0 ~ TC3 and DBDC1_TC */
		if (ucTc >= TC4_INDEX && ucTc < USB_DBDC1_TC)
			continue;
		prHifInfo->arTxDataReqHead[ucTc] = glUsbInitQ(prHifInfo,
								&prHifInfo->rTxDataFreeQ[ucTc], USB_REQ_TX_DATA_CNT);
		i = 0;
		list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxDataFreeQ[ucTc], list) {
			QUEUE_INITIALIZE(&prUsbReq->rSendingDataMsduInfoList);
			/* TODO: every endpoint should has an unique and only TC */
			*((uint8_t *)&prUsbReq->prPriv) = ucTc;
			prUsbReq->prBufCtrl = &prHifInfo->rTxDataBufCtrl[ucTc][i];
#ifdef CFG_PREALLOC_MEMORY
			prUsbReq->prBufCtrl->pucBuf =
				preallocGetMem(MEM_ID_TX_DATA);
#else
			prUsbReq->prBufCtrl->pucBuf = kmalloc(USB_TX_DATA_BUFF_SIZE, GFP_ATOMIC);
#endif
			if (prUsbReq->prBufCtrl->pucBuf == NULL) {
				DBGLOG(HAL, ERROR, "kmalloc() reports error\n");
				goto error;
			}
			prUsbReq->prBufCtrl->u4BufSize = USB_TX_DATA_BUFF_SIZE;
			prUsbReq->prBufCtrl->u4WrIdx = 0;
			++i;
		}

		DBGLOG(INIT, DEBUG,
		       "USB Tx URB INIT Tc[%u] cnt[%u] len[%u]\n", ucTc, i,
		       prHifInfo->rTxDataBufCtrl[ucTc][0].u4BufSize);
	}

	/* RX EVENT */
	prHifInfo->prRxEventReqHead = glUsbInitQ(prHifInfo, &prHifInfo->rRxEventFreeQ, USB_REQ_RX_EVENT_CNT);
	i = 0;
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxEventFreeQ, list) {
		prUsbReq->prBufCtrl = &prHifInfo->rRxEventBufCtrl[i];
#ifdef CFG_PREALLOC_MEMORY
		prUsbReq->prBufCtrl->pucBuf = preallocGetMem(MEM_ID_RX_EVENT);
#else
		prUsbReq->prBufCtrl->pucBuf = kmalloc(USB_RX_EVENT_BUF_SIZE, GFP_ATOMIC);
#endif
		if (prUsbReq->prBufCtrl->pucBuf == NULL) {
			DBGLOG(HAL, ERROR, "kmalloc() reports error\n");
			goto error;
		}
		prUsbReq->prBufCtrl->u4BufSize = USB_RX_EVENT_BUF_SIZE;
		prUsbReq->prBufCtrl->u4ReadSize = 0;
		++i;
	}

	/* RX Data */
	prHifInfo->prRxDataReqHead = glUsbInitQ(prHifInfo, &prHifInfo->rRxDataFreeQ, USB_REQ_RX_DATA_CNT);
	i = 0;
	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxDataFreeQ, list) {
		prUsbReq->prBufCtrl = &prHifInfo->rRxDataBufCtrl[i];
#ifdef CFG_PREALLOC_MEMORY
		prUsbReq->prBufCtrl->pucBuf = preallocGetMem(MEM_ID_RX_DATA);
#else
		prUsbReq->prBufCtrl->pucBuf = kmalloc(USB_RX_DATA_BUF_SIZE, GFP_ATOMIC);
#endif
		if (prUsbReq->prBufCtrl->pucBuf == NULL) {
			DBGLOG(HAL, ERROR, "kmalloc() reports error\n");
			goto error;
		}
		prUsbReq->prBufCtrl->u4BufSize = USB_RX_DATA_BUF_SIZE;
		prUsbReq->prBufCtrl->u4ReadSize = 0;
		++i;
	}

#if CFG_CHIP_RESET_SUPPORT
	/* RX WDT interrupt */
	if (prChipInfo->fgIsSupportL0p5Reset && prBusInfo->fgIsSupportWdtEp) {
		prHifInfo->prRxWdtReqHead = glUsbInitQ(prHifInfo,
						       &prHifInfo->rRxWdtFreeQ,
						       USB_REQ_RX_WDT_CNT);
		i = 0;
		list_for_each_entry_safe(prUsbReq, prUsbReqNext,
					 &prHifInfo->rRxWdtFreeQ, list) {
			prUsbReq->prBufCtrl = &prHifInfo->rRxWdtBufCtrl[i];
			prUsbReq->prBufCtrl->pucBuf = kmalloc(
				USB_RX_WDT_BUF_SIZE, GFP_ATOMIC);
			if (prUsbReq->prBufCtrl->pucBuf == NULL) {
				DBGLOG(HAL, ERROR, "kmalloc() reports error\n");
				goto error;
			}
			prUsbReq->prBufCtrl->u4BufSize = USB_RX_WDT_BUF_SIZE;
			prUsbReq->prBufCtrl->u4ReadSize = 0;
			++i;
		}
	}
#endif

	INIT_LIST_HEAD(&prHifInfo->rTxCmdSendingQ);
	INIT_LIST_HEAD(&prHifInfo->rTxCmdCompleteQ);
	INIT_LIST_HEAD(&prHifInfo->rTxDataCompleteQ);
	INIT_LIST_HEAD(&prHifInfo->rRxEventCompleteQ);
	INIT_LIST_HEAD(&prHifInfo->rRxDataCompleteQ);

#if CFG_CHIP_RESET_SUPPORT
	if (prChipInfo->fgIsSupportL0p5Reset && prBusInfo->fgIsSupportWdtEp)
		INIT_LIST_HEAD(&prHifInfo->rRxWdtCompleteQ);
#endif

	glUsbSetState(prHifInfo, USB_STATE_LINK_UP);
	glUsbSetStateSyncCtrl(prHifInfo, USB_STATE_LINK_UP);
	prGlueInfo->u4InfType = MT_DEV_INF_USB;

	prBusInfo->ucVndReqToMcuFailCnt = 0;

	return;

error:
	/* TODO */
	;
}				/* end of glSetHifInfo() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function clears hif related info.
*
* \param[in] prGlueInfo Pointer to glue info structure
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
void glClearHifInfo(struct GLUE_INFO *prGlueInfo)
{
	/* struct GL_HIF_INFO *prHifInfo = NULL; */
	/* ASSERT(prGlueInfo); */
	/* prHifInfo = &prGlueInfo->rHifInfo; */
	uint8_t ucTc;
	struct USB_REQ *prUsbReq, *prUsbReqNext;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
#if CFG_CHIP_RESET_SUPPORT
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;

	if (prGlueInfo->prAdapter == NULL) {
		DBGLOG(INIT, WARN, "Adapter is null\n");
		return;
	}
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
#endif

	for (ucTc = 0; ucTc < USB_TC_NUM; ++ucTc) {
		if (ucTc >= TC4_INDEX && ucTc < USB_DBDC1_TC)
			continue;
		list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxDataFreeQ[ucTc], list) {
#ifndef CFG_PREALLOC_MEMORY
			kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
			usb_free_urb(prUsbReq->prUrb);
		}
	}

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxDataFfaQ, list) {
#ifndef CFG_PREALLOC_MEMORY
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxCmdFreeQ, list) {
#ifndef CFG_PREALLOC_MEMORY
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxCmdCompleteQ, list) {
#ifndef CFG_PREALLOC_MEMORY
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxDataCompleteQ, list) {
#ifndef CFG_PREALLOC_MEMORY
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxDataFreeQ, list) {
#ifndef CFG_PREALLOC_MEMORY
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxEventFreeQ, list) {
#ifndef CFG_PREALLOC_MEMORY
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}

#if CFG_CHIP_RESET_SUPPORT
	if (prChipInfo->fgIsSupportL0p5Reset && prBusInfo->fgIsSupportWdtEp) {
		list_for_each_entry_safe(prUsbReq, prUsbReqNext,
					 &prHifInfo->rRxWdtFreeQ, list) {
			kfree(prUsbReq->prBufCtrl->pucBuf);
			usb_free_urb(prUsbReq->prUrb);
		}
	}
#endif

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxDataCompleteQ, list) {
#ifndef CFG_PREALLOC_MEMORY
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rRxEventCompleteQ, list) {
#ifndef CFG_PREALLOC_MEMORY
		kfree(prUsbReq->prBufCtrl->pucBuf);
#endif
		usb_free_urb(prUsbReq->prUrb);
	}

#if CFG_CHIP_RESET_SUPPORT
	if (prChipInfo->fgIsSupportL0p5Reset && prBusInfo->fgIsSupportWdtEp) {
		list_for_each_entry_safe(prUsbReq, prUsbReqNext,
					 &prHifInfo->rRxWdtCompleteQ, list) {
			kfree(prUsbReq->prBufCtrl->pucBuf);
			usb_free_urb(prUsbReq->prUrb);
		}
	}
#endif

	kfree(prHifInfo->prTxCmdReqHead);
	kfree(prHifInfo->arTxDataFfaReqHead);
	for (ucTc = 0; ucTc < USB_TC_NUM; ++ucTc)
		kfree(prHifInfo->arTxDataReqHead[ucTc]);
	kfree(prHifInfo->prRxEventReqHead);
	kfree(prHifInfo->prRxDataReqHead);
#if CFG_CHIP_RESET_SUPPORT
	if (prChipInfo->fgIsSupportL0p5Reset && prBusInfo->fgIsSupportWdtEp)
		kfree(prHifInfo->prRxWdtReqHead);
#endif
	mutex_destroy(&prHifInfo->vendor_req_sem);
	kfree(prHifInfo->vendor_req_buf);
	prHifInfo->vendor_req_buf = NULL;
	prHifInfo->vendor_req_buf_sz = 0;
} /* end of glClearHifInfo() */

/*----------------------------------------------------------------------------*/
/*!
* \brief This function reset necessary hif related info when chip reset.
*
* \param[in] prGlueInfo Pointer to glue info structure
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
void glResetHifInfo(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct BUS_INFO *prBusInfo;

	ASSERT(prGlueInfo);

	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	/* If chip supports event endpoint detection, ex: MT7961, then we shall
	 * do the procedure again after chip reset. Otherwise, chip will be fail
	 * to receive event in the re-initialization stage after chip reset.
	 */
	prHifInfo->fgEventEpDetected = FALSE;

	glUsbSetState(prHifInfo, USB_STATE_LINK_UP);
	glUsbSetStateSyncCtrl(prHifInfo, USB_STATE_LINK_UP);

	prBusInfo->ucVndReqToMcuFailCnt = 0;
} /* end of glResetHifInfo() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Initialize bus operation and hif related information, request resources.
*
* \param[out] pvData    A pointer to HIF-specific data type buffer.
*                       For eHPI, pvData is a pointer to uint32_t type and
*                       stores a mapped base address.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
u_int8_t glBusInit(void *pvData)
{
	return TRUE;
}				/* end of glBusInit() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Stop bus operation and release resources.
*
* \param[in] pvData A pointer to struct net_device.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
void glBusRelease(void *pvData)
{
}				/* end of glBusRelease() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Setup bus interrupt operation and interrupt handler for os.
*
* \param[in] pvData     A pointer to struct net_device.
* \param[in] pfnIsr     A pointer to interrupt handler function.
* \param[in] pvCookie   Private data for pfnIsr function.
*
* \retval WLAN_STATUS_SUCCESS   if success
*         NEGATIVE_VALUE   if fail
*/
/*----------------------------------------------------------------------------*/
int32_t glBusSetIrq(void *pvData, void *pfnIsr, void *pvCookie)
{
	int ret = 0;

	struct net_device *prNetDevice = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;

	ASSERT(pvData);
	if (!pvData)
		return -1;

	prNetDevice = (struct net_device *)pvData;
	prGlueInfo = (struct GLUE_INFO *) pvCookie;
	ASSERT(prGlueInfo);
	if (!prGlueInfo)
		return -1;

	prHifInfo = &prGlueInfo->rHifInfo;

	return ret;
}				/* end of glBusSetIrq() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Stop bus interrupt operation and disable interrupt handling for os.
*
* \param[in] pvData     A pointer to struct net_device.
* \param[in] pvCookie   Private data for pfnIsr function.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
void glBusFreeIrq(void *pvData, void *pvCookie)
{
	struct net_device *prNetDevice = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;

	ASSERT(pvData);
	if (!pvData) {
		return;
	}
	prNetDevice = (struct net_device *)pvData;
	prGlueInfo = (struct GLUE_INFO *) pvCookie;
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		return;
	}

	prHifInfo = &prGlueInfo->rHifInfo;
}				/* end of glBusreeIrq() */

u_int8_t glIsReadClearReg(uint32_t u4Address)
{
	switch (u4Address) {
	case MCR_WHISR:
	case MCR_WASR:
	case MCR_D2HRM0R:
	case MCR_D2HRM1R:
	case MCR_WTQCR0:
	case MCR_WTQCR1:
	case MCR_WTQCR2:
	case MCR_WTQCR3:
	case MCR_WTQCR4:
	case MCR_WTQCR5:
	case MCR_WTQCR6:
	case MCR_WTQCR7:
		return TRUE;

	default:
		return FALSE;
	}
}

uint16_t glGetUsbDeviceVendorId(struct usb_device *dev)
{
	return dev->descriptor.idVendor;
}				/* end of glGetUsbDeviceVendorId() */

uint16_t glGetUsbDeviceProductId(struct usb_device *dev)
{
	return dev->descriptor.idProduct;
}				/* end of glGetUsbDeviceProductId() */

int32_t glGetUsbDeviceManufacturerName(struct usb_device *dev, uint8_t *buffer, uint32_t bufLen)
{
	return usb_string(dev, dev->descriptor.iManufacturer, buffer, bufLen);
}				/* end of glGetUsbDeviceManufacturerName() */

int32_t glGetUsbDeviceProductName(struct usb_device *dev, uint8_t *buffer, uint32_t bufLen)
{
	return usb_string(dev, dev->descriptor.iProduct, buffer, bufLen);
}				/* end of glGetUsbDeviceManufacturerName() */

int32_t glGetUsbDeviceSerialNumber(struct usb_device *dev, uint8_t *buffer, uint32_t bufLen)
{
	return usb_string(dev, dev->descriptor.iSerialNumber, buffer, bufLen);
}				/* end of glGetUsbDeviceSerialNumber() */


/*----------------------------------------------------------------------------*/
/*!
* \brief Read a 32-bit device register
*
* \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
* \param[in] u4Register Register offset
* \param[in] pu4Value   Pointer to variable used to store read value
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t kalDevRegRead(struct GLUE_INFO *prGlueInfo, uint32_t u4Register,
		uint32_t *pu4Value)
{
	struct BUS_INFO *prBusInfo = NULL;
	int ret = 0;
	uint8_t ucRetryCount = 0;
	uint8_t ucTotalFailCnt;

	ASSERT(prGlueInfo);
	ASSERT(pu4Value);

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	*pu4Value = 0xFFFFFFFF;

	ucTotalFailCnt = prBusInfo->ucVndReqToMcuFailCnt;

	if (ucTotalFailCnt >= VND_REQ_FAIL_TH) {
		DBGLOG(HAL, TRACE, "vendor reqs keep failure over %d times\n",
		       VND_REQ_FAIL_TH);
		return FALSE;
	}

	do {
		ret = mtk_usb_vendor_request(prGlueInfo,
			0,
			prBusInfo->u4device_vender_request_in,
			VND_REQ_REG_READ,
			(u4Register & 0xffff0000) >> 16,
			(u4Register & 0x0000ffff), pu4Value,
				       sizeof(*pu4Value));

		if (ret || ucRetryCount)
			DBGLOG(HAL, ERROR,
				"usb_control_msg() status: %d retry: %u\n",
				ret, ucRetryCount);

		if (ret) {
			if (ucTotalFailCnt < 0xff)
				ucTotalFailCnt++;
		} else
			ucTotalFailCnt = 0;

		ucRetryCount++;
		if (ucRetryCount > HIF_USB_ACCESS_RETRY_LIMIT)
			break;
	} while (ret);

	if (ret) {
		kalSendAeeWarning(HIF_USB_ERR_TITLE_STR,
				  HIF_USB_ERR_DESC_STR "USB() reports error: %x retry: %u", ret, ucRetryCount);
		DBGLOG(HAL, ERROR, "usb_readl() reports error: %x retry: %u\n", ret, ucRetryCount);
	} else {
		DBGLOG(HAL, TRACE, "Get CR[0x%08x] value[0x%08x]\n",
			u4Register, *pu4Value);
	}

	if (kalIsResetting() == FALSE) {
		if (ret == -ENODEV) {
			/* No such device error, L0.5 reset always fail */
			ucTotalFailCnt = VND_REQ_FAIL_TH;
			GL_DEFAULT_RESET_TRIGGER(
				prGlueInfo->prAdapter,
				RST_SER_L0P5_FAIL);
		} else if (ucTotalFailCnt >= VND_REQ_FAIL_TH) {
			GL_DEFAULT_RESET_TRIGGER(
				prGlueInfo->prAdapter,
				RST_CMD_EVT_FAIL);
		}
	}

	prBusInfo->ucVndReqToMcuFailCnt = ucTotalFailCnt;

	return (ret) ? FALSE : TRUE;
}				/* end of kalDevRegRead() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Write a 32-bit device register
*
* \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
* \param[in] u4Register Register offset
* \param[in] u4Value    Value to be written
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t kalDevRegWrite(struct GLUE_INFO *prGlueInfo, uint32_t u4Register,
		uint32_t u4Value)
{
	int ret = 0;
	uint8_t ucRetryCount = 0;
	struct BUS_INFO *prBusInfo = NULL;
	uint8_t ucTotalFailCnt;

	ASSERT(prGlueInfo);
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	ucTotalFailCnt = prBusInfo->ucVndReqToMcuFailCnt;

	if (ucTotalFailCnt >= VND_REQ_FAIL_TH) {
		DBGLOG(HAL, TRACE, "vendor reqs keep failure over %d times\n",
		       VND_REQ_FAIL_TH);
		return FALSE;
	}

	do {
		ret = mtk_usb_vendor_request(prGlueInfo,
			0,
			prBusInfo->u4device_vender_request_out,
			VND_REQ_REG_WRITE,
			(u4Register & 0xffff0000) >> 16,
			(u4Register & 0x0000ffff),
			&u4Value,
				       sizeof(u4Value));

		if (ret || ucRetryCount)
			DBGLOG(HAL, ERROR,
				"usb_control_msg() status: %d retry: %u\n",
				ret, ucRetryCount);

		if (ret) {
			if (ucTotalFailCnt < 0xff)
				ucTotalFailCnt++;
		} else
			ucTotalFailCnt = 0;

		ucRetryCount++;
		if (ucRetryCount > HIF_USB_ACCESS_RETRY_LIMIT)
			break;

	} while (ret);

	if (ret) {
		kalSendAeeWarning(HIF_USB_ERR_TITLE_STR,
				  HIF_USB_ERR_DESC_STR "usb_writel() reports error: %x retry: %u", ret, ucRetryCount);
		DBGLOG(HAL, ERROR, "usb_writel() reports error: %x retry: %u\n", ret, ucRetryCount);
	} else {
		DBGLOG(HAL, DEBUG,
		       "Set CR[0x%08x] value[0x%08x]\n", u4Register, u4Value);
	}

	if (kalIsResetting() == FALSE) {
		if (ret == -ENODEV) {
			/* No such device error, L0.5 reset always fail */
			ucTotalFailCnt = VND_REQ_FAIL_TH;
			GL_DEFAULT_RESET_TRIGGER(
				prGlueInfo->prAdapter,
				RST_SER_L0P5_FAIL);
		} else if (ucTotalFailCnt >= VND_REQ_FAIL_TH) {
			GL_DEFAULT_RESET_TRIGGER(
				prGlueInfo->prAdapter,
				RST_CMD_EVT_FAIL);
		}
	}

	prBusInfo->ucVndReqToMcuFailCnt = ucTotalFailCnt;

	return (ret) ? FALSE : TRUE;
}				/* end of kalDevRegWrite() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Read a 32-bit device register through UHW
*
* \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
* \param[in] u4Register Register offset
* \param[in] pu4Value   Pointer to variable used to store read value
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t kalDevUhwRegRead(struct GLUE_INFO *prGlueInfo,
			  uint32_t u4Register, uint32_t *pu4Value)
{
	struct BUS_INFO *prBusInfo = NULL;
	int ret = 0;
	uint8_t ucRetryCount = 0;
	uint8_t ucTotalFailCnt;

	ASSERT(prGlueInfo);
	ASSERT(pu4Value);

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	*pu4Value = 0xFFFFFFFF;

	ucTotalFailCnt = prBusInfo->ucVndReqToMcuFailCnt;

	do {
		ret = mtk_usb_vendor_request(prGlueInfo,
			0,
			DEVICE_VENDOR_REQUEST_UHW_IN,
			VND_REQ_UHW_READ,
			(u4Register & 0xffff0000) >> 16,
			(u4Register & 0x0000ffff), pu4Value,
				       sizeof(*pu4Value));

		if (ret || ucRetryCount)
			DBGLOG(HAL, ERROR,
				"usb_control_msg() status: %d retry: %u\n",
				ret, ucRetryCount);

		if (ret) {
			if (ucTotalFailCnt < 0xff)
				ucTotalFailCnt++;
		} else
			ucTotalFailCnt = 0;

		ucRetryCount++;
		if (ucRetryCount > HIF_USB_ACCESS_RETRY_LIMIT)
			break;
	} while (ret);

	if (ret) {
		kalSendAeeWarning(HIF_USB_ERR_TITLE_STR, HIF_USB_ERR_DESC_STR
				  "USB() reports error: %x retry: %u", ret,
				  ucRetryCount);
		DBGLOG(HAL, ERROR, "usb_readl() reports error: %x retry: %u\n",
		       ret, ucRetryCount);
	} else {
		DBGLOG(HAL, TRACE, "Get CR[0x%08x] value[0x%08x]\n",
			u4Register, *pu4Value);
	}

	if (kalIsResetting() == FALSE) {
		if (ret == -ENODEV) {
			/* No such device error, L0.5 reset always fail */
			ucTotalFailCnt = VND_REQ_FAIL_TH;
			GL_DEFAULT_RESET_TRIGGER(
				prGlueInfo->prAdapter,
				RST_SER_L0P5_FAIL);
		} else if (ucTotalFailCnt >= VND_REQ_FAIL_TH) {
			GL_DEFAULT_RESET_TRIGGER(
				prGlueInfo->prAdapter,
				RST_CMD_EVT_FAIL);
		}
	}

	prBusInfo->ucVndReqToMcuFailCnt = ucTotalFailCnt;

	return (ret) ? FALSE : TRUE;
}				/* end of kalDevUhwRegRead() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Write a 32-bit device register through UHW
*
* \param[in] prGlueInfo Pointer to the GLUE_INFO_T structure.
* \param[in] u4Register Register offset
* \param[in] u4Value    Value to be written
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t kalDevUhwRegWrite(struct GLUE_INFO *prGlueInfo,
			   uint32_t u4Register, uint32_t u4Value)
{
	int ret = 0;
	uint8_t ucRetryCount = 0;
	struct BUS_INFO *prBusInfo = NULL;
	uint8_t ucTotalFailCnt;

	ASSERT(prGlueInfo);
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	ucTotalFailCnt = prBusInfo->ucVndReqToMcuFailCnt;

	do {
		ret = mtk_usb_vendor_request(prGlueInfo,
			0,
			DEVICE_VENDOR_REQUEST_UHW_OUT,
			VND_REQ_UHW_WRITE,
			(u4Register & 0xffff0000) >> 16,
			(u4Register & 0x0000ffff),
			&u4Value,
				       sizeof(u4Value));

		if (ret || ucRetryCount)
			DBGLOG(HAL, ERROR,
				"usb_control_msg() status: %d retry: %u\n",
				ret, ucRetryCount);

		if (ret) {
			if (ucTotalFailCnt < 0xff)
				ucTotalFailCnt++;
		} else
			ucTotalFailCnt = 0;

		ucRetryCount++;
		if (ucRetryCount > HIF_USB_ACCESS_RETRY_LIMIT)
			break;

	} while (ret);

	if (ret) {
		kalSendAeeWarning(HIF_USB_ERR_TITLE_STR, HIF_USB_ERR_DESC_STR
				  "uhw_writel() reports error: %x retry: %u",
				  ret, ucRetryCount);
		DBGLOG(HAL, ERROR, "uhw_writel() reports error: %x retry: %u\n",
		       ret, ucRetryCount);
	} else {
		DBGLOG(HAL, TRACE, "Set CR[0x%08x] value[0x%08x]\n", u4Register,
		       u4Value);
	}

	if (kalIsResetting() == FALSE) {
		if (ret == -ENODEV) {
			/* No such device error, L0.5 reset always fail */
			ucTotalFailCnt = VND_REQ_FAIL_TH;
			GL_DEFAULT_RESET_TRIGGER(
				prGlueInfo->prAdapter,
				RST_SER_L0P5_FAIL);
		} else if (ucTotalFailCnt >= VND_REQ_FAIL_TH) {
			GL_DEFAULT_RESET_TRIGGER(
				prGlueInfo->prAdapter,
				RST_CMD_EVT_FAIL);
		}
	}

	prBusInfo->ucVndReqToMcuFailCnt = ucTotalFailCnt;

	return (ret) ? FALSE : TRUE;
}				/* end of kalDevUhwRegWrite() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Read device I/O port
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] u2Port             I/O port offset
* \param[in] u2Len              Length to be read
* \param[out] pucBuf            Pointer to read buffer
* \param[in] u2ValidOutBufSize  Length of the buffer valid to be accessed
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t
kalDevPortRead(struct GLUE_INFO *prGlueInfo, uint16_t u2Port, uint32_t u4Len,
	       uint8_t *pucBuf, uint32_t u4ValidOutBufSize, u_int8_t isPollMode)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	uint8_t *pucDst = NULL;
	/* int count = u4Len; */
	int ret = 0;
	/* int bNum = 0; */

#if DBG
	DBGLOG(HAL, DEBUG,
	       "++%s++ buf:0x%p, port:0x%x, length:%d\n",
	       __func__, pucBuf, u2Port, u4Len);
#endif

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;

	ASSERT(pucBuf);
	pucDst = pucBuf;

	ASSERT(u4Len <= u4ValidOutBufSize);

	u2Port &= MTK_USB_PORT_MASK;
	if (prGlueInfo->rHifInfo.eEventEpType == EVENT_EP_TYPE_INTR &&
		u2Port == (USB_EVENT_EP_IN & USB_ENDPOINT_NUMBER_MASK)) {
		/* maximize buff len for usb in */
		ret = mtk_usb_intr_in_msg(&prGlueInfo->rHifInfo, u4ValidOutBufSize, pucDst, u2Port);
		if (ret != u4Len) {
			DBGLOG(HAL, WARN, "usb_interrupt_msg(IN=%d) Warning. Data is not completed. (receive %d/%u)\n",
			       u2Port, ret, u4Len);
		}
		ret = ret >= 0 ? 0 : ret;
	} else if (u2Port >= MTK_USB_BULK_IN_MIN_EP && u2Port <= MTK_USB_BULK_IN_MAX_EP) {
		/* maximize buff len for usb in */
		ret = mtk_usb_bulk_in_msg(&prGlueInfo->rHifInfo, u4ValidOutBufSize, pucDst, u2Port);
		if (ret != u4Len) {
			DBGLOG(HAL, WARN, "usb_bulk_msg(IN=%d) Warning. Data is not completed. (receive %d/%u)\n",
			       u2Port, ret, u4Len);
		}
		ret = ret >= 0 ? 0 : ret;
	} else {
		DBGLOG(HAL, ERROR, "kalDevPortRead reports error: invalid port %x\n", u2Port);
		ret = -EINVAL;
	}

	if (ret) {
		kalSendAeeWarning(HIF_USB_ERR_TITLE_STR, HIF_USB_ERR_DESC_STR "usb_readsb() reports error: %x", ret);
		DBGLOG(HAL, ERROR, "usb_readsb() reports error: %x\n", ret);
	}

	return (ret) ? FALSE : TRUE;
}				/* end of kalDevPortRead() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Write device I/O port
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] u2Port             I/O port offset
* \param[in] u2Len              Length to be write
* \param[in] pucBuf             Pointer to write buffer
* \param[in] u2ValidInBufSize   Length of the buffer valid to be accessed
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t
kalDevPortWrite(struct GLUE_INFO *prGlueInfo, uint16_t u2Port, uint32_t u4Len,
		uint8_t *pucBuf, uint32_t u4ValidInBufSize)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	uint8_t *pucSrc = NULL;
	/* int count = u4Len; */
	int ret = 0;
	/* int bNum = 0; */

#if DBG
	DBGLOG(HAL, DEBUG,
	       "++%s++ buf:0x%p, port:0x%x, length:%d\n",
	       __func__, pucBuf, u2Port, u4Len);
#endif

	ASSERT(prGlueInfo);
	prHifInfo = &prGlueInfo->rHifInfo;

	ASSERT(pucBuf);
	pucSrc = pucBuf;

	ASSERT((u4Len + LEN_USB_UDMA_TX_TERMINATOR) <= u4ValidInBufSize);

	kalMemZero(pucSrc + u4Len, LEN_USB_UDMA_TX_TERMINATOR);
	u4Len += LEN_USB_UDMA_TX_TERMINATOR;

	u2Port &= MTK_USB_PORT_MASK;
	if (u2Port >= MTK_USB_BULK_OUT_MIN_EP && u2Port <= MTK_USB_BULK_OUT_MAX_EP) {
		ret = mtk_usb_bulk_out_msg(&prGlueInfo->rHifInfo, u4Len, pucSrc, u2Port/*8*/);
		if (ret != u4Len) {
			DBGLOG(HAL, WARN, "usb_bulk_msg(OUT=%d) Warning. Data is not completed. (receive %d/%u)\n",
			       u2Port, ret, u4Len);
		}
		ret = ret >= 0 ? 0 : ret;
	} else {
		DBGLOG(HAL, ERROR, "kalDevPortWrite reports error: invalid port %x\n", u2Port);
		ret = -EINVAL;
	}

	if (ret) {
		kalSendAeeWarning(HIF_USB_ERR_TITLE_STR, HIF_USB_ERR_DESC_STR "usb_writesb() reports error: %x", ret);
		DBGLOG(HAL, ERROR, "usb_writesb() reports error: %x\n", ret);
	}

	return (ret) ? FALSE : TRUE;
}				/* end of kalDevPortWrite() */

/*----------------------------------------------------------------------------*/
/*!
* \brief Set power state
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] ePowerMode
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
void glSetPowerState(struct GLUE_INFO *prGlueInfo, uint32_t ePowerMode)
{
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Write data to device
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] prMsduInfo         msdu info
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t kalDevWriteData(struct GLUE_INFO *prGlueInfo,
		struct MSDU_INFO *prMsduInfo)
{
	halTxUSBSendData(prGlueInfo, prMsduInfo);
	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Kick Tx data to device
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
u_int8_t kalDevKickData(struct GLUE_INFO *prGlueInfo)
{
#if 0
	halTxUSBKickData(prGlueInfo);
#endif
	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Write command to device
*
* \param[in] prGlueInfo         Pointer to the GLUE_INFO_T structure.
* \param[in] u4Addr             I/O port offset
* \param[in] ucData             Single byte of data to be written
*
* \retval TRUE          operation success
* \retval FALSE         operation fail
*/
/*----------------------------------------------------------------------------*/
enum ENUM_CMD_TX_RESULT kalDevWriteCmd(struct GLUE_INFO *prGlueInfo,
		struct CMD_INFO *prCmdInfo, uint8_t ucTC)
{
	halTxUSBSendCmd(prGlueInfo, ucTC, prCmdInfo);
	return CMD_TX_RESULT_SUCCESS;
}

void glGetDev(void *ctx, void **dev)
{
	struct usb_interface *prUsbIntf = (struct usb_interface *) ctx;
	struct usb_device *prUsbDev = interface_to_usbdev(prUsbIntf);

	*dev = &prUsbDev->dev;
}

void glGetHifDev(struct GL_HIF_INFO *prHif, struct device **dev)
{
	*dev = &(prHif->udev->dev);
}

void glGetChipInfo(void **prChipInfo)
{
	struct mt66xx_hif_driver_data *prDriverData;

	prDriverData = get_platform_driver_data();
	if (!prDriverData)
		return;

	*prChipInfo = (void *)prDriverData->chip_info;
}

void glGetChipInfoByGlue(struct GLUE_INFO *prGlueInfo, void **prChipInfo)
{
	glGetChipInfo(prChipInfo);
}

#if CFG_CHIP_RESET_SUPPORT
void kalRemoveProbe(struct GLUE_INFO *prGlueInfo)
{
	uint32_t gpio_num, default_level, action_level, invert_time;
#if CFG_CHIP_RESET_USE_DTS_GPIO_NUM
	struct device_node *node;
#endif
#if CFG_ENABLE_GKI_SUPPORT
	uint32_t i4Status;
#else
	typedef void (*func_ptr) (unsigned int gpio, int init_value);
	char *func_name = "mtk_gpio_set_value";
	void *pvAddr = NULL;
#endif

#if CFG_CHIP_RESET_USE_DTS_GPIO_NUM
	node = of_find_compatible_node(NULL,
				       NULL,
				       CHIP_RESET_DTS_COMPATIBLE_NAME);
	if (!node) {
		DBGLOG(HAL, ERROR,
		       "[SER][L0]: Failed to find dts node: %s\n",
		       CHIP_RESET_DTS_COMPATIBLE_NAME);
		return;
	}
	if (of_property_read_u32(node, CHIP_RESET_GPIO_PROPERTY_NAME,
				&gpio_num) != 0) {
		DBGLOG(HAL, ERROR,
		       "[SER][L0]: Failed to get gpio_num : %s\n",
		       CHIP_RESET_GPIO_PROPERTY_NAME);
		return;
	}
	if (of_property_read_u32(node, CHIP_RESET_INVERT_PROPERTY_NAME,
				&invert_time) != 0) {
		DBGLOG(HAL, WARN,
		       "[SER][L0]: Failed to get invert_time : %s\n",
		       CHIP_RESET_INVERT_PROPERTY_NAME);
		invert_time = RESET_PIN_SET_LOW_TIME;
	}
	if (of_property_read_u32(node, CHIP_RESET_DEFAULT_VAL_PROPERTY_NAME,
				&default_level) != 0) {
		DBGLOG(HAL, WARN,
		       "[SER][L0]: Failed to get default_level : %s\n",
		       CHIP_RESET_DEFAULT_VAL_PROPERTY_NAME);
		default_level = 1;
	}
	default_level = (default_level == 0) ? 0 : 1;
	action_level = (default_level == 0) ? 1 : 0;
#else
	gpio_num = WIFI_DONGLE_RESET_GPIO_PIN;
	invert_time = RESET_PIN_SET_LOW_TIME;
	default_level = 1;
	action_level = 0;
#endif

	DBGLOG(HAL, DEBUG,
	       "[SER][L0]: wifi reset gpio %d pull %s %dms\n",
	       gpio_num, (action_level == 0) ? "down" : "up", invert_time);

#if CFG_ENABLE_GKI_SUPPORT
	i4Status = gpio_request(gpio_num, "wifi-reset");
	if (i4Status < 0) {
		DBGLOG(HAL, ERROR,
		       "[SER][L0]: gpio_request(%d,%s) %d failed\n",
		       gpio_num, "wifi-reset", i4Status);
		return;
	}
	i4Status = gpio_direction_output(gpio_num, action_level);
	DBGLOG(HAL, WARN,
	       "[SER][L0]: Invoke gpio_direction_output (%d, %d) %d\n",
	       gpio_num, action_level, i4Status);
	mdelay(invert_time);
	i4Status = gpio_direction_output(gpio_num, default_level);
	DBGLOG(HAL, WARN,
	       "[SER][L0]: Invoke gpio_direction_output (%d, %d) %d\n",
	       gpio_num, default_level, i4Status);
	gpio_free(gpio_num);
#else
	pvAddr = GLUE_SYMBOL_GET(func_name);

	if (!pvAddr) {
		DBGLOG(HAL, ERROR, "[SER][L0]%s: No Exported Func Found [%s]\n",
		       __func__, func_name);
	} else {
		func_ptr pFunc = (func_ptr) pvAddr;
		DBGLOG(HAL, WARN, "[SER][L0]%s: Invoke %s(%d,%d)\n",
		       __func__, func_name, gpio_num, action_level);
		pFunc(gpio_num, action_level);
		mdelay(invert_time);
		DBGLOG(HAL, WARN, "[SER][L0]%s: Invoke %s(%d,%d)\n",
		       __func__, func_name, gpio_num, default_level);
		pFunc(gpio_num, default_level);
		GLUE_SYMBOL_PUT(func_name);
	}
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check HIF state to determine if L0.5 reset shall be postponed.
 *
 * \param[in] prGlueInfo
 *
 * \return TRUE  if L0.5 reset shall be postponed.
 *         FALSE  otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalCheckWfsysResetPostpone(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	unsigned long flags;
	u_int8_t fgPostpone = FALSE;

	prHifInfo = &prGlueInfo->rHifInfo;

	spin_lock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_WFSYS_RESET]);

	if (prGlueInfo->prAdapter->fgIsCfgSuspend)
		fgPostpone = TRUE;

	spin_unlock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_WFSYS_RESET]);

	if (fgPostpone)
		goto END;

	spin_lock_irqsave(&prHifInfo->rStateLock, flags);

	if (prHifInfo->state == USB_STATE_PRE_SUSPEND ||
	    prHifInfo->state == USB_STATE_SUSPEND)
		fgPostpone = TRUE;

	spin_unlock_irqrestore(&prHifInfo->rStateLock, flags);

END:
	return fgPostpone;
}
#endif

