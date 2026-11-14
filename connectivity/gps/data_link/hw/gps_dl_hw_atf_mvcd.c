/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_hw_atf.h"
#include "gps_dl_hw_api.h"

#define GNSS_IMAGE_MAX_FRAGEMENT_NUM 256
#define N1_DSP_SV_MAX_NUM 64

bool gps_dl_hw_gps_get_bootup_info(enum gps_dl_link_id_enum link_id, bool is_cw_dsp,
		struct gps_dl_hw_mvcd_gps_bootup_info *bootup_info)
{
	struct arm_smccc_res res;
	enum gps_dl_hw_mvcd_dsp_type dsp_type;

	if (bootup_info == NULL)
		return false;

	if (link_id == GPS_DATA_LINK_ID0) {
		if (!is_cw_dsp)
			dsp_type = GPS_DL_MVCD_DSP_L1;
		else
			dsp_type = GPS_DL_MVCD_DSP_L1_CW;
	} else if (link_id == GPS_DATA_LINK_ID1) {
		if (!is_cw_dsp)
			dsp_type = GPS_DL_MVCD_DSP_L5;
		else
			dsp_type = GPS_DL_MVCD_DSP_L5_CW;
	} else {
		GDL_LOGW("Error link_id: %d, is_cw_dsp:%d", link_id, is_cw_dsp);
		return false;
	}

	memset(&res, 0, sizeof(res));

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MVCD_GET_DSP_BOOT_UP_INFO,
			dsp_type, 0, 0, 0, 0, 0, &res);

	if (res.a1 == 0) {
		GDL_LOGE("get bootup info failed from ATF link_id:%d, is_cw_dsp:%d", link_id, is_cw_dsp);
		return false;
	}

	bootup_info->frag_num = (unsigned int)(res.a1 & 0x000000000000FFFF);
	bootup_info->code_size = (unsigned int)((res.a1 & 0x00000000FFFF0000) >> 16);
	bootup_info->start_addr = (unsigned int)((res.a1 & 0x0000FFFF00000000) >> 32);
	bootup_info->exec_addr = (unsigned int)((res.a1 & 0xFFFF000000000000) >> 48);
	bootup_info->cipher_key = (unsigned int)(res.a2 & 0x000000000000FFFF);

	return true;
}

bool gps_dl_hw_gps_send_dsp_fragement_num(enum gps_dl_link_id_enum link_id,
		bool is_cw_dsp, unsigned int fragement_num)
{
	struct arm_smccc_res res;
	enum gps_dl_hw_mvcd_dsp_type dsp_type;

	if (link_id == GPS_DATA_LINK_ID0) {
		if (!is_cw_dsp)
			dsp_type = GPS_DL_MVCD_DSP_L1;
		else
			dsp_type = GPS_DL_MVCD_DSP_L1_CW;
	} else if (link_id == GPS_DATA_LINK_ID1) {
		if (!is_cw_dsp)
			dsp_type = GPS_DL_MVCD_DSP_L5;
		else
			dsp_type = GPS_DL_MVCD_DSP_L5_CW;
	} else {
		GDL_LOGW("Error link_id: %d, is_cw_dsp:%d", link_id, is_cw_dsp);
		return false;
	}

	if (fragement_num > GNSS_IMAGE_MAX_FRAGEMENT_NUM) {
		GDL_LOGE("dsp fragement number error link_id:%d, is_cw_dsp:%d, fragment_num:%d",
			link_id, is_cw_dsp, fragement_num);
		return false;
	}

	memset(&res, 0, sizeof(res));
	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MVCD_SEND_DSP_FRAGEMENT,
			dsp_type, fragement_num, 0, 0, 0, 0, &res);

	if (res.a1 == 1) {
		GDL_LOGE("Send dsp fragement failed link_id:%d, is_cw_dsp:%d, fragment_num:%d",
			link_id, is_cw_dsp, fragement_num);
		return false;
	} else {
		return true;
	}
}

bool gps_dl_hw_gps_n1_get_bootup_info(struct gps_dl_hw_mvcd_n1_dsp_bootup_info *bootup_info)
{
	struct arm_smccc_res res;

	if (bootup_info == NULL)
		return false;

	memset(&res, 0, sizeof(res));

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MVCD_N1_GET_DSP_BOOT_UP_INFO,
			0, 0, 0, 0, 0, 0, &res);

	if (res.a2 == 1) {
		GDL_LOGE("get N1 dsp bootup info failed from ATF");
		return false;
	}

	bootup_info->ldpc_count = (unsigned int)res.a1;

	return true;
}

bool gps_dl_hw_n1_gps_send_dsp_fragement_num(enum gps_dl_hw_mvcd_n1_dsp_segment_type segment_type,
		unsigned int sv_id)
{
	struct arm_smccc_res res;

	if (!((segment_type >= N1_MVCD_PILOT) && (segment_type <= N1_MVCD_2ND_CODE))) {
		GDL_LOGE("n1 dsp segment_type error");
		return false;
	}

	if (sv_id == 0 || sv_id > N1_DSP_SV_MAX_NUM) {
		GDL_LOGE("sv id error %d", sv_id);
		return false;
	}

	memset(&res, 0, sizeof(res));
	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_MVCD_N1_SEND_DSP_FRAGEMENT,
			segment_type, sv_id, 0, 0, 0, 0, &res);

	if (res.a1 == 1) {
		GDL_LOGE("Send n1 dsp fragement failed segment_type:%d, sv_id:%d",
			segment_type, sv_id);
		return false;
	} else {
		return true;
	}
}

