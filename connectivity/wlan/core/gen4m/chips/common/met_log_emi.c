// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "met_log_emi.h"
#include "gl_met_log.h"

#if CFG_SUPPORT_MET_LOG

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define MET_LOG_EMI_RESERVED 0x10 /* for rp, wp*/
#define ADDR_SYNC_FOR_IRP 0x00401800
#if (CFG_SUPPORT_WIFI_MET_CONNAC3_V2 == 1)
#define MET_LOG_STATS_UPDATE_PERIOD (1000) /* ms */
#define MET_LOG_EMI_SIZE 0xC8000
#define MET_LOG_EMI_WIFI_LONG_SIZE 0x32000
#define MET_LOG_EMI_COMMON_SIZE 0x40010
#else
#define MET_LOG_STATS_UPDATE_PERIOD (1000) /* ms */
#define MET_LOG_EMI_SIZE 0x19000
#endif

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
static uint32_t WIFI_MODULE_ID = 3;
#if (CFG_SUPPORT_WIFI_MET_CONNAC3_V2 == 1)
static uint32_t WIFI_MODULE_LONG_ID = 4;
static uint32_t COMMON_MODULE_ID = 5;
#endif

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
struct MET_LOG_EMI_CTRL g_met_log_emi_ctx;

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check if EMI is empty.
 *
 */
/*----------------------------------------------------------------------------*/
static u_int8_t met_log_emi_is_empty(uint32_t *wp, uint32_t *irp)
{
	if (*irp == *wp)
		return TRUE;
	else
		return FALSE;
}

static void met_log_emi_refresh_header(struct ADAPTER *ad,
	struct MET_LOG_EMI_CTRL *ctrl)
{
	struct mt66xx_chip_info *chip_info = ad->chip_info;
	struct MET_LOG_HEADER common = {0};

	emi_mem_read(chip_info,
		     ctrl->offset,
		     &common,
		     sizeof(common));

	ctrl->rp = common.rp;
	ctrl->wp = common.wp;
	ctrl->checksum = common.checksum;

	DBGLOG(MET, TRACE,
			"irp: 0x%x, rp: 0x%x, wp: 0x%x, checksum: 0x%x\n",
			ctrl->irp,
			ctrl->rp,
			ctrl->wp,
			ctrl->checksum);

#if (CFG_SUPPORT_WIFI_MET_CONNAC3_V2 == 1)
	emi_mem_read(chip_info,
		     ctrl->offset_wifi_long,
		     &common,
		     sizeof(common));

	ctrl->rp_wifi_long = common.rp;
	ctrl->wp_wifi_long = common.wp;
	ctrl->checksum_wifi_long = common.checksum;

	DBGLOG(MET, TRACE,
			"[Wifi Long] irp: 0x%x, rp: 0x%x, wp: 0x%x, checksum: 0x%x\n",
			ctrl->irp_wifi_long,
			ctrl->rp_wifi_long,
			ctrl->wp_wifi_long,
			ctrl->checksum_wifi_long);

	emi_mem_read(chip_info,
		     ctrl->offset_common,
		     &common,
		     sizeof(common));

	ctrl->rp_common = ((common.rp & 0x00FFFFFF) | 0x78000000);
	ctrl->wp_common = ((common.wp & 0x00FFFFFF) | 0x78000000);
	ctrl->checksum_common = common.checksum;

	DBGLOG(MET, TRACE,
			"[Common] irp: 0x%x, rp: 0x%x, wp: 0x%x, checksum: 0x%x\n",
			ctrl->irp_common,
			ctrl->rp_common,
			ctrl->wp_common,
			ctrl->checksum_common);
#endif
}

static int met_log_emi_read(void)
{
	struct MET_LOG_EMI_CTRL *ctrl = &g_met_log_emi_ctx;
	uint32_t rpTemp = 0, recv = 0, handled = 0;
	struct ADAPTER *ad = NULL;

	ad = (struct ADAPTER *) ctrl->priv;
	if (!ad)
		return 0;

	if (ctrl->wp > ctrl->irp)
		recv = ctrl->wp - ctrl->irp;
	else
		recv = ctrl->emi_size - ctrl->irp + ctrl->wp;
	if (recv > ctrl->emi_size) {
		DBGLOG(MET, ERROR,
			"Invalid recv emi_size (%u %u)\n",
			recv, ctrl->emi_size);
		return 0;
	}
	if (recv % 8 != 0) {
		DBGLOG(MET, DEBUG,
			"align 8, change recv size from %d to %d\n",
			recv, recv - (recv % 8));
		recv -= (recv % 8);
	}

	rpTemp = ctrl->irp;

	while (recv) {
		uint32_t size = 0;

		if (rpTemp + recv > ctrl->end_addr)
			size = ctrl->end_addr - rpTemp;
		else
			size = recv;

		DBGLOG(MET, DEBUG,
			"Read data from: 0x%x, size: 0x%x\n",
			rpTemp,
			size);

		if (emi_mem_read(ad->chip_info,
				 rpTemp,
				 ctrl->buffer + handled,
				 size)) {
			DBGLOG(MET, ERROR,
				"Read EMI fail!\n");
			continue;
		}

		handled += size;
		rpTemp += size;
		if (rpTemp >= ctrl->end_addr)
			rpTemp = (rpTemp % ctrl->end_addr) + ctrl->start_addr;

		recv -= size;
	}

	ctrl->irp += handled;
	if (ctrl->irp >= ctrl->end_addr)
		ctrl->irp = (ctrl->irp % ctrl->end_addr)
				+ ctrl->start_addr;

	return handled;
}

#if (CFG_SUPPORT_WIFI_MET_CONNAC3_V2 == 1)
static int met_log_emi_read_wifi_long(void)
{
	struct MET_LOG_EMI_CTRL *ctrl = &g_met_log_emi_ctx;
	uint32_t rpTemp = 0, recv = 0, handled = 0;
	struct ADAPTER *ad = NULL;

	ad = (struct ADAPTER *) ctrl->priv;
	if (!ad)
		return 0;

	if (ctrl->wp_wifi_long > ctrl->irp_wifi_long)
		recv = ctrl->wp_wifi_long - ctrl->irp_wifi_long;
	else
		recv = ctrl->emi_size_wifi_long - ctrl->irp_wifi_long
				+ ctrl->wp_wifi_long;

	if (recv > ctrl->emi_size_wifi_long) {
		DBGLOG(MET, ERROR,
			"[Wifi Long]Invalid recv emi_size (%u %u)\n",
			recv, ctrl->emi_size_wifi_long);
		return 0;
	}
	if (recv % 16 != 0) {
		DBGLOG(MET, DEBUG,
			"align 16, change recv size from %d to %d\n",
			recv, recv - (recv % 16));
		recv -= (recv % 16);
	}

	rpTemp = ctrl->irp_wifi_long;

	while (recv) {
		uint32_t size = 0;

		if (rpTemp + recv > ctrl->end_addr_wifi_long)
			size = ctrl->end_addr_wifi_long - rpTemp;
		else
			size = recv;

		DBGLOG(MET, DEBUG,
			"Read data from: 0x%x, size: 0x%x\n",
			rpTemp,
			size);

		if (emi_mem_read(ad->chip_info,
				 rpTemp,
				 ctrl->buffer_wifi_long + handled,
				 size)) {
			DBGLOG(MET, ERROR,
				"Read EMI fail!\n");
			continue;
		}

		handled += size;
		rpTemp += size;
		if (rpTemp >= ctrl->end_addr_wifi_long)
			rpTemp = (rpTemp % ctrl->end_addr_wifi_long)
					+ ctrl->start_addr_wifi_long;

		recv -= size;
	}

	ctrl->irp_wifi_long += handled;
	if (ctrl->irp_wifi_long >= ctrl->end_addr_wifi_long)
		ctrl->irp_wifi_long =
				(ctrl->irp_wifi_long % ctrl->end_addr_wifi_long)
				+ ctrl->start_addr_wifi_long;

	return handled;
}

static int met_log_emi_read_common(void)
{
	struct MET_LOG_EMI_CTRL *ctrl = &g_met_log_emi_ctx;
	uint32_t rpTemp = 0, recv = 0, handled = 0;
	struct ADAPTER *ad = NULL;

	ad = (struct ADAPTER *) ctrl->priv;
	if (!ad)
		return 0;

	if (ctrl->wp_common > ctrl->irp_common)
		recv = ctrl->wp_common - ctrl->irp_common;
	else
		recv = ctrl->emi_size_common - ctrl->irp_common
				+ ctrl->wp_common;

	if (recv > ctrl->emi_size_common) {
		DBGLOG(MET, ERROR,
			"[Common]Invalid recv emi_size (%u %u)\n",
			recv, ctrl->emi_size_common);
		return 0;
	}
	if (recv % 16 != 0) {
		DBGLOG(MET, DEBUG,
			"align 16, change recv size from %d to %d\n",
			recv, recv - (recv % 16));
		recv -= (recv % 16);
	}

	rpTemp = ctrl->irp_common;

	while (recv) {
		uint32_t size = 0;

		if (rpTemp + recv > ctrl->end_addr_common)
			size = ctrl->end_addr_common - rpTemp;
		else
			size = recv;

		DBGLOG(MET, DEBUG,
			"Read data from: 0x%x, size: 0x%x\n",
			rpTemp,
			size);

		if (emi_mem_read(ad->chip_info,
				 rpTemp,
				 ctrl->buffer_common + handled,
				 size)) {
			DBGLOG(MET, ERROR,
				"Read EMI fail!\n");
			continue;
		}

		handled += size;
		rpTemp += size;
		if (rpTemp >= ctrl->end_addr_common)
			rpTemp = (rpTemp % ctrl->end_addr_common)
				+ ctrl->start_addr_common;

		recv -= size;
	}

	ctrl->irp_common += handled;
	if (ctrl->irp_common >= ctrl->end_addr_common)
		ctrl->irp_common = (ctrl->irp_common % ctrl->end_addr_common)
				+ ctrl->start_addr_common;

	return handled;
}
#endif

static void met_log_emi_handler(void)
{
	struct MET_LOG_EMI_CTRL *ctrl = &g_met_log_emi_ctx;
	uint32_t handled = 0;
	struct ADAPTER *ad = NULL;

	DBGLOG(MET, TRACE, "\n");

	ad = (struct ADAPTER *) ctrl->priv;
	if (!ad)
		return;

	met_log_emi_refresh_header(ad, ctrl);
	ctrl->irp = ctrl->rp;
	ctrl->start_addr = ctrl->rp;
	ctrl->end_addr = ctrl->rp + ctrl->emi_size;
	DBGLOG(MET, DEBUG,
		"irp: 0x%x, start_addr: 0x%x, end_addr: 0x%x\n",
		ctrl->irp,
		ctrl->start_addr,
		ctrl->end_addr);
#if (CFG_SUPPORT_WIFI_MET_CONNAC3_V2 == 1)
	ctrl->irp_wifi_long = ctrl->rp_wifi_long;
	ctrl->start_addr_wifi_long = ctrl->rp_wifi_long;
	ctrl->end_addr_wifi_long =
		ctrl->rp_wifi_long + ctrl->emi_size_wifi_long;
	DBGLOG(MET, DEBUG,
		"[Wifi Long] irp: 0x%x, start_addr: 0x%x, end_addr: 0x%x\n",
		ctrl->irp_wifi_long,
		ctrl->start_addr_wifi_long,
		ctrl->end_addr_wifi_long);

	ctrl->irp_common = ctrl->rp_common;
	ctrl->start_addr_common = ctrl->rp_common;
	ctrl->end_addr_common = ctrl->rp_common + ctrl->emi_size_common;
	DBGLOG(MET, DEBUG,
		"[Common] irp: 0x%x, start_addr: 0x%x, end_addr: 0x%x\n",
		ctrl->irp_common,
		ctrl->start_addr_common,
		ctrl->end_addr_common);
#endif

	while (1) {
		if (!ctrl->initialized)
			break;
		if (test_bit(GLUE_FLAG_HALT_BIT, &ad->prGlueInfo->ulFlag) ||
		    kalIsResetting()) {
			DBGLOG(MET, DEBUG,
				"wifi off, stop Met log.\n");
			break;
		}

		met_log_emi_refresh_header(ad, ctrl);
#if (CFG_SUPPORT_WIFI_MET_CONNAC3_V2 == 1)
		if (met_log_emi_is_empty(&(ctrl->wp), &(ctrl->irp)) &&
			met_log_emi_is_empty(
				&(ctrl->wp_wifi_long),
				&(ctrl->irp_wifi_long)) &&
			met_log_emi_is_empty(
				&(ctrl->wp_common),
				&(ctrl->irp_common)))
#else
		if (met_log_emi_is_empty(&(ctrl->wp), &(ctrl->irp)))
#endif
		{
			DBGLOG(MET, TRACE, "EMI is empty, no more MET log!\n");
			msleep(MET_LOG_STATS_UPDATE_PERIOD);
			continue;
		}

		if (!met_log_emi_is_empty(&(ctrl->wp), &(ctrl->irp))) {
			handled = met_log_emi_read();

			met_log_print_data(ctrl->buffer,
					handled,
					WIFI_MODULE_ID,
					ctrl->project_id,
					ad->chip_info->chip_id);
		}
#if (CFG_SUPPORT_WIFI_MET_CONNAC3_V2 == 1)
		if (!met_log_emi_is_empty(&(ctrl->wp_wifi_long),
				&(ctrl->irp_wifi_long))) {
			handled = met_log_emi_read_wifi_long();

			met_log_print_long_data(ctrl->buffer_wifi_long,
					handled,
					WIFI_MODULE_LONG_ID,
					ctrl->project_id,
					ad->chip_info->chip_id);
		}

		if (!met_log_emi_is_empty(&(ctrl->wp_common),
				&(ctrl->irp_common))) {
			handled = met_log_emi_read_common();

			met_log_print_long_data(ctrl->buffer_common,
					handled,
					COMMON_MODULE_ID,
					ctrl->project_id,
					ad->chip_info->chip_id);
		}

#endif
		msleep(MET_LOG_STATS_UPDATE_PERIOD);
	}

	DBGLOG(MET, TRACE, "\n");
}

static void met_log_emi_work(struct work_struct *work)
{
	met_log_emi_handler();
}

uint32_t met_log_emi_init(struct ADAPTER *ad)
{
	struct MET_LOG_EMI_CTRL *ctrl = &g_met_log_emi_ctx;
	uint32_t u4EmiMetOffset = 0;
	uint32_t u4ProjectId = 0;
	uint32_t status = WLAN_STATUS_SUCCESS;

	if (ctrl->initialized) {
		DBGLOG(MET, WARN,
			"Met log already init!\n");
		return status;
	}

	DBGLOG(INIT, TRACE, "\n");

	u4EmiMetOffset = emi_mem_offset_convert(kalGetEmiMetOffset());
	u4ProjectId = kalGetProjectId();
	kalMemZero(ctrl, sizeof(*ctrl));
	ctrl->priv = ad;
	ctrl->project_id = u4ProjectId;
#if (CFG_SUPPORT_WIFI_MET_CONNAC3_V2 == 1)
	ctrl->offset = u4EmiMetOffset;
	ctrl->emi_size = MET_LOG_EMI_SIZE - MET_LOG_EMI_RESERVED;
	ctrl->offset_wifi_long = u4EmiMetOffset + MET_LOG_EMI_SIZE;
	ctrl->emi_size_wifi_long =
			MET_LOG_EMI_WIFI_LONG_SIZE - MET_LOG_EMI_RESERVED;
	ctrl->offset_common =
		u4EmiMetOffset + MET_LOG_EMI_SIZE + MET_LOG_EMI_WIFI_LONG_SIZE;
	ctrl->emi_size_common = MET_LOG_EMI_COMMON_SIZE - MET_LOG_EMI_RESERVED;

	DBGLOG(MET, DEBUG,
		"offset: 0x%x, emi_size: 0x%x, wifi_long_offset: 0x%x, wifi_long_size: 0x%x, common_offset: 0x%x, common_size: 0x%x\n",
		ctrl->offset,
		ctrl->emi_size,
		ctrl->offset_wifi_long,
		ctrl->emi_size_wifi_long,
		ctrl->offset_common,
		ctrl->emi_size_common);

	ctrl->buffer = kalMemAlloc(ctrl->emi_size, VIR_MEM_TYPE);
	ctrl->buffer_wifi_long =
		kalMemAlloc(ctrl->emi_size_wifi_long, VIR_MEM_TYPE);
	ctrl->buffer_common = kalMemAlloc(ctrl->emi_size_common, VIR_MEM_TYPE);
	if (!ctrl->buffer || !ctrl->buffer_wifi_long || !ctrl->buffer_common) {
		DBGLOG(MET, ERROR,
			"Alloc buffer failed.\n");
		status = WLAN_STATUS_RESOURCES;
		goto exit;
	} else {
		kalMemZero(ctrl->buffer, ctrl->emi_size);
		kalMemZero(ctrl->buffer_wifi_long, ctrl->emi_size_wifi_long);
		kalMemZero(ctrl->buffer_common, ctrl->emi_size_common);
	}
#else
	ctrl->offset = u4EmiMetOffset;
	ctrl->emi_size = MET_LOG_EMI_SIZE - MET_LOG_EMI_RESERVED;

	DBGLOG(MET, DEBUG,
		"offset: 0x%x, emi_size: 0x%x\n",
		ctrl->offset,
		ctrl->emi_size);

	ctrl->buffer = kalMemAlloc(ctrl->emi_size, VIR_MEM_TYPE);
	if (!ctrl->buffer) {
		DBGLOG(MET, ERROR,
			"Alloc buffer failed.\n");
		status = WLAN_STATUS_RESOURCES;
		goto exit;
	} else {
		kalMemZero(ctrl->buffer, ctrl->emi_size);
	}
#endif

	ctrl->wq = create_singlethread_workqueue("met_log_emi");
	if (!ctrl->wq) {
		DBGLOG(MET, ERROR,
			"create_singlethread_workqueue failed.\n");
		status = WLAN_STATUS_RESOURCES;
		goto exit;
	}
	ctrl->initialized = TRUE;
	INIT_WORK(&ctrl->work, met_log_emi_work);
	queue_work(ctrl->wq, &ctrl->work);

exit:
	if (status != WLAN_STATUS_SUCCESS) {
		DBGLOG(MET, ERROR,
			"status: 0x%x\n",
			status);
		met_log_emi_deinit(ad);
	}

	return status;
}

uint32_t met_log_emi_deinit(struct ADAPTER *ad)
{
	struct MET_LOG_EMI_CTRL *ctrl = &g_met_log_emi_ctx;
	uint32_t status = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, TRACE, "\n");

	if (!ctrl->initialized) {
		DBGLOG(MET, WARN,
			"Met log already deinit!\n");
		return status;
	}

	ctrl->initialized = FALSE;
	cancel_work_sync(&ctrl->work);
	if (ctrl->wq)
		destroy_workqueue(ctrl->wq);
	if (ctrl->buffer)
		kalMemFree(ctrl->buffer, VIR_MEM_TYPE, ctrl->emi_size);
#if (CFG_SUPPORT_WIFI_MET_CONNAC3_V2 == 1)
	if (ctrl->buffer_wifi_long)
		kalMemFree(ctrl->buffer_wifi_long,
			VIR_MEM_TYPE,
			ctrl->emi_size_wifi_long);
	if (ctrl->buffer_common)
		kalMemFree(ctrl->buffer_common,
			VIR_MEM_TYPE,
			ctrl->emi_size_common);
#endif
	ctrl->priv = NULL;

	return status;
}
#endif /* CFG_SUPPORT_MET_LOG */
