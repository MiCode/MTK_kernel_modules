/*
* Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
*/

#ifndef __MI_ENENT_H
#define __MI_ENENT_H

#if IS_ENABLED(CONFIG_MIEV)
#define MI_EVENT_COUNT_MAX 20000 /* number of max counted */

enum display_mievent_type {
	MI_EVENT_GPU_PAGE_FAULT = 911001017,
	MI_EVENT_GPU_HARD_RESET_FAULT = 911101002,
	MI_EVENT_GPU_FAULT = 911102001,
	MI_EVENT_GPU_CS_FAULT = 911102002,
	MI_EVENT_GPU_REQUEST_TIMEOUT = 911102003,
	MI_EVENT_GPU_SOFT_RESET_FAULT = 911102004,
};

void mi_disp_mievent_report(unsigned int event_type);

#endif

#endif /* __MI_ENENT_H */
