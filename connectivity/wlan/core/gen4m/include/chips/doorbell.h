/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  doorbell.h
 */

#ifndef _DOORBELL_H
#define _DOORBELL_H

static inline void triggerWfFwdlDoorbell(struct ADAPTER *ad)
{
	if (!ad || !ad->chip_info ||
	    !ad->chip_info->uni_fwdl_info ||
	    !ad->chip_info->uni_fwdl_info->trigger_wf_fwdl_doorbell)
		return;

	ad->chip_info->uni_fwdl_info->trigger_wf_fwdl_doorbell(ad);
}
#endif /* _DOORBELL_H */
