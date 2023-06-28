/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "met_ext_sym_util.h"

EXTERNAL_SYMBOL_FUNC(void,scmi_tinysys_register_event_notifier,u32 feature_id, f_handler_t hand)
EXTERNAL_SYMBOL_FUNC(int,scmi_tinysys_event_notify,u32 feature_id, u32 notify_enable)
EXTERNAL_SYMBOL_FUNC(struct scmi_tinysys_info_st *,get_scmi_tinysys_info,void)
EXTERNAL_SYMBOL_FUNC(int,scmi_tinysys_common_set,const struct scmi_protocol_handle *ph, u32 feature_id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5)