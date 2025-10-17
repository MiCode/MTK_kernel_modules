/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_mcudl_log.h"
#include "gps_mcudl_plat_api.h"
#include "gps_dl_linux_reserved_mem.h"
#include "gps_dl_linux_reserved_mem_v2.h"
#include <linux/firmware.h>


int gps_mcudl_request_firmware(unsigned char *pPatchName, struct firmware **ppPatch)
{
	int iRet;
	struct firmware *fw;

	iRet = -1;
	fw = NULL;
	if (!ppPatch) {
		MDL_LOGE("invalid ppBufptr!\n");
		return -1;
	}
	*ppPatch = NULL;
	iRet = request_firmware((const struct firmware **)&fw, pPatchName, NULL);
	if (iRet != 0) {
		MDL_LOGE("failed to open or read!(%s)\n", pPatchName);
		release_firmware(fw);
		return -1;
	}
	MDL_LOGW("loader firmware %s ok!!\n", pPatchName);
	iRet = 0;
	*ppPatch = fw;
	return iRet;
}

#define FW_HDR_SIZE (48)
void gps_mcudl_load_single_firmware(unsigned char *p_src_file, void __iomem *p_dst_addr, unsigned int dst_len)
{
	struct firmware *pPatch = NULL;

	if (p_dst_addr == NULL || dst_len == 0)
		return;

	if (p_src_file == NULL)
		return;

	if (gps_mcudl_request_firmware(p_src_file, &pPatch) != 0)
		return;

	if (pPatch == NULL)
		return;

	/*get full name patch success*/
	MDL_LOGW("get full patch name(%s) buf(0x%p) size(0x%zx)",
		p_src_file, (pPatch)->data, (pPatch)->size);
	MDL_LOGW("AF get patch, pPatch(0x%p), dst addr(0x%p) len(0x%x)",
		pPatch, p_dst_addr, dst_len);

	if ((pPatch)->size <= dst_len) {
		MDL_LOGW("Prepare to copy FW to (0x%p)\n", p_dst_addr);
#if GPS_DL_ON_LINUX
		memset_io(p_dst_addr, 0, dst_len);
		memcpy_toio(p_dst_addr,
#else
		memcpy(p_dst_addr,
#endif
			(unsigned char *)((pPatch)->data) + FW_HDR_SIZE,
			(pPatch)->size - FW_HDR_SIZE);
		MDL_LOGW("pGpsEmibaseaddr_1:0x%08x 0x%08x 0x%08x 0x%08x\n",
			*(((unsigned int *)p_dst_addr)+0),
			*(((unsigned int *)p_dst_addr)+1),
			*(((unsigned int *)p_dst_addr)+2),
			*(((unsigned int *)p_dst_addr)+3));
		MDL_LOGW("pGpsEmibaseaddr_2:0x%08x 0x%08x 0x%08x 0x%08x\n",
			*(((unsigned int *)p_dst_addr)+4),
			*(((unsigned int *)p_dst_addr)+5),
			*(((unsigned int *)p_dst_addr)+6),
			*(((unsigned int *)p_dst_addr)+7));
	}
	release_firmware(pPatch);
	pPatch = NULL;
}

bool g_gps_mcudl_fw_loading_done;
bool g_gps_mcudl_need_to_load_fw_in_drv;
void gps_mcudl_may_do_fw_loading(void)
{
	struct gps_mcudl_emi_region_item bin_region;
	void __iomem *p_dst_addr;
	unsigned int dst_len;

	if (!g_gps_mcudl_need_to_load_fw_in_drv) {
		MDL_LOGI("no need to load fw in drv");
		return;
	}

	if (g_gps_mcudl_fw_loading_done)
		return;

	/*MCU.bin*/
	p_dst_addr = (void __iomem *)gps_mcudl_get_emi_region_info(GDL_EMI_REGION_MCU_BIN, &bin_region);
	dst_len = bin_region.length;
	gps_mcudl_load_single_firmware("soc7_1_ram_mcu_1a_1_hdr.bin", p_dst_addr, dst_len);

	/*GPS.bin*/
	p_dst_addr = (void __iomem *)gps_mcudl_get_emi_region_info(GDL_EMI_REGION_GPS_BIN, &bin_region);
	dst_len = bin_region.length;
	gps_mcudl_load_single_firmware("soc7_1_ram_gps_offload_1a_1_hdr.bin", p_dst_addr, dst_len);

	/*MNL.bin*/
	p_dst_addr = (void __iomem *)gps_mcudl_get_emi_region_info(GDL_EMI_REGION_MNL_BIN, &bin_region);
	dst_len = bin_region.length;
	gps_mcudl_load_single_firmware("MNL_hdr.bin", p_dst_addr, dst_len);

	g_gps_mcudl_fw_loading_done = true;
}

void gps_mcudl_clear_fw_loading_done_flag(void)
{
	g_gps_mcudl_fw_loading_done = false;
}

void gps_mcudl_set_need_to_load_fw_in_drv(bool need)
{
	bool old_need;

	old_need = g_gps_mcudl_need_to_load_fw_in_drv;
	g_gps_mcudl_need_to_load_fw_in_drv = need;
	MDL_LOGW("set need %d -> %d", old_need, need);
}

