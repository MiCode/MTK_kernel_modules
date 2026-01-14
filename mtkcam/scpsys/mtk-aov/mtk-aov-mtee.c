// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 *
 * Author: ChenHung Yang <chenhung.yang@mediatek.com>
 */

#include <tz_cross/trustzone.h>
#include <tz_cross/ta_test.h>
#include <tz_cross/ta_system.h>
#include <kree/system.h>
#include <linux/ktime.h>

#include "mtk-aov-mtee.h"
#include "mtk-aov-data.h"

/* Emulate a user and the user_id is 10. */
static const uint32_t aov_fr_user_id = 10;
static KREE_SESSION_HANDLE session = -1;

static int aov_fr_init(KREE_SESSION_HANDLE session)
{
	int ret = 0;
	union MTEEC_PARAM param[4] = { 0 };
	uint32_t types;

	types = TZ_ParamTypes1(TZPT_VALUE_INOUT);
	ret = KREE_TeeServiceCall(session, AOV_FR_CMD_INITIALIZE, types, param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_info("%s failed to invoke service call, ret %d\n", __func__, ret);
		return ret;
	}

	return param[0].value.a;
}

static int aov_fr_release(KREE_SESSION_HANDLE session)
{
	int ret = 0;
	union MTEEC_PARAM param[4] = { 0 };
	uint32_t types;

	types = TZ_ParamTypes1(TZPT_VALUE_INOUT);
	ret = KREE_TeeServiceCall(session, AOV_FR_CMD_RELEASE, types, param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_info("%s failed to invoke service call, ret %d\n", __func__, ret);
		return ret;
	}

	return param[0].value.a;
}

static int aov_fr_set_active_user(KREE_SESSION_HANDLE session, uint32_t user_id)
{
	int ret = 0;
	union MTEEC_PARAM param[4] = { 0 };
	uint32_t types;

	param[0].value.a = user_id;
	types = TZ_ParamTypes1(TZPT_VALUE_INOUT);
	ret = KREE_TeeServiceCall(session, AOV_FR_CMD_SET_ACTIVE_USER, types, param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_info("%s failed to invoke service call, ret %d\n", __func__, ret);
		return ret;
	}

	return param[0].value.a;
}

static int aov_fr_clear_active_user(KREE_SESSION_HANDLE session)
{
	int ret = 0;
	union MTEEC_PARAM param[4] = { 0 };
	uint32_t types;

	types = TZ_ParamTypes1(TZPT_VALUE_INOUT);
	ret = KREE_TeeServiceCall(session, AOV_FR_CMD_CLEAR_ACTIVE_USER, types, param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_info("%s failed to invoke service call, ret %d\n", __func__, ret);
		return ret;
	}

	return param[0].value.a;
}

static int aov_fr_authenticate(KREE_SESSION_HANDLE session, uint32_t user_id,
			       struct aov_fr_offset *fr_offset, int *recognized)
{
	int ret = 0;
	union MTEEC_PARAM param[4] = { 0 };
	uint32_t types;

	param[0].value.a = user_id;
	param[1].mem.buffer = fr_offset;
	param[1].mem.size = sizeof(struct aov_fr_offset);
	types = TZ_ParamTypes2(TZPT_VALUE_INOUT, TZPT_MEM_INPUT);
	ret = KREE_TeeServiceCall(session, AOV_FR_CMD_AUTHENTICATE, types, param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_info("%s failed to invoke service call, ret %d\n", __func__, ret);
		return ret;
	}

	*recognized = param[0].value.b;

	return param[0].value.a;
}

int aov_mtee_init(struct mtk_aov *device)
{
	int ret = 0;

	pr_info("%s ++\n", __func__);

	if (session == -1) {
		ret = KREE_CreateSession("com.mediatek.geniezone.aov_fr_sample", &session);
		if (ret != TZ_RESULT_SUCCESS)
			pr_info("%s failed to create session, ret %d\n", __func__, ret);
	} else
		pr_info("%s session already created\n", __func__);

	ret = aov_fr_init(session);
	if (ret != 0) {
		pr_info("%s failed to aov_fr_init, ret %d\n", __func__, ret);
		return ret;
	}

	ret = aov_fr_set_active_user(session, aov_fr_user_id);
	if (ret != 0) {
		pr_info("%s failed to aov_fr_set_active_user, ret %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

int aov_mtee_notify(struct mtk_aov *device, void *buffer)
{
	int ret = 0;
	struct aov_fr_offset fr_offset;
	struct fr_info_t *fr_info = (struct fr_info_t *)buffer;
	ktime_t start, end;
	int recognized = 0;

	if (!fr_info) {
		pr_info("%s NULL input\n", __func__);
		return -EINVAL;
	}

	if (fr_info->count == 0) {
		pr_info("%s fr_info_t records are not available\n", __func__);
		return -EINVAL;
	}

	pr_debug("%s fr_info_t count %d\n", __func__, fr_info->count);
	pr_debug("%s fr_info_t 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", __func__,
		 fr_info->offset[0], fr_info->offset[1], fr_info->offset[2],
		 fr_info->offset[3], fr_info->offset[4]);

	fr_offset.token = aov_fr_user_id;
	fr_offset.result_offset = (uint64_t)fr_info->offset[0];

	pr_debug("%s aov_fr_offset token %d\n", __func__, fr_offset.token);
	pr_debug("%s aov_fr_offset result_offset 0x%llx\n", __func__, fr_offset.result_offset);

	start = ktime_get();
	ret = aov_fr_authenticate(session, aov_fr_user_id, &fr_offset, &recognized);
	end = ktime_get();
	pr_info("%s Authenticate Time: %lld (ns)\n", __func__, ktime_to_ns(ktime_sub(end, start)));

	if (ret != 0) {
		pr_info("%s failed to aov_fr_authenticate, ret %d\n", __func__, ret);
		return ret;
	}

	return recognized;
}

int aov_mtee_uninit(struct mtk_aov *aov_dev)
{
	int ret = 0;

	pr_info("%s ++\n", __func__);

	if (session == -1) {
		pr_info("%s session is not available\n", __func__);
		return -EINVAL;
	}

	ret = aov_fr_clear_active_user(session);
	if (ret != 0) {
		pr_info("%s failed to aov_fr_clear_active_user, ret %d\n", __func__, ret);
		return ret;
	}

	ret = aov_fr_release(session);
	if (ret != 0) {
		pr_info("%s failed to aov_fr_release, ret %d\n", __func__, ret);
		return ret;
	}

	ret = KREE_CloseSession(session);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_info("%s failed to close session, ret %d\n", __func__, ret);
		return ret;
	}

	session = -1;

	return 0;
}
