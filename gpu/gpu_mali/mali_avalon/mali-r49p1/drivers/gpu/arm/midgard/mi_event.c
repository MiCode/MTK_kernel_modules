/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 */
#if IS_ENABLED(CONFIG_MIEV)
#include <linux/printk.h>
#include <miev/mievent.h>
#include "mi_event.h"

const char *get_mievent_type_name(int event_type)
{
	switch (event_type) {
	case MI_EVENT_GPU_PAGE_FAULT:
		return "gpu_page_fault";
	case MI_EVENT_GPU_FAULT:
		return "gpu_fault";
	case MI_EVENT_GPU_HARD_RESET_FAULT:
		return "gpu_hard_reset_fault";
	case MI_EVENT_GPU_CS_FAULT:
		return "gpu_cs_fault";
	case MI_EVENT_GPU_REQUEST_TIMEOUT:
		return "gpu_request_timeout";
	case MI_EVENT_GPU_SOFT_RESET_FAULT:
		return "gpu_soft_reset_fault";
	default:
		return "unknown";
	}
}

void mi_disp_mievent_report(unsigned int event_type)
{
	struct misight_mievent *event;
	const char *event_describe;
	const char *event_name = NULL;
	static atomic_t err_count[6] = {0};
	static bool err_count_init = false;
	unsigned long wait_timeout = msecs_to_jiffies(10*60*1000); // 10 minutes
	static atomic64_t last_time_gpu_page_fault = ATOMIC64_INIT(0);
	static atomic64_t last_time_gpu_fault = ATOMIC64_INIT(0);
	static atomic64_t last_time_gpu_hard_reset_fault = ATOMIC64_INIT(0);
	static atomic64_t last_time_gpu_cs_fault = ATOMIC64_INIT(0);
	static atomic64_t last_time_gpu_request_timeout = ATOMIC64_INIT(0);
	static atomic64_t last_time_gpu_soft_reset_fault = ATOMIC64_INIT(0);

	if (!err_count_init) {
		for (int i = 0; i < 6; i++) {
			atomic_set(&err_count[i], 0);
		}
		err_count_init = true;
	}

	event_name = get_mievent_type_name(event_type);
	pr_info("%s: event_type[%d], event_name[%s]\n", __func__, event_type, event_name);

	if (!strcmp(event_name, "unknown")) {
		pr_err("unknown event type %d\n", event_type);
		return;
	}

	event  = cdev_tevent_alloc(event_type);

	if (!event) {
		pr_err("cdev_tevent_alloc failed");
		return;
	}

	u64 current_time = get_jiffies_64();

	switch(event_type){
	case MI_EVENT_GPU_PAGE_FAULT:
		atomic_inc(&err_count[0]);
		if (atomic_read(&err_count[0]) > 5 && !time_after64(current_time , atomic64_read(&last_time_gpu_page_fault) + wait_timeout)) {
			pr_info("%s: gpu_page_fault error count %d, short time interval, not report\n", __func__, atomic_read(&err_count[0]));
			cdev_tevent_destroy(event);
			return;
		}
		atomic64_set(&last_time_gpu_page_fault, current_time);
		cdev_tevent_add_int(event, event_name, atomic_read(&err_count[0]));

		if (atomic_read(&err_count[0]) > MI_EVENT_COUNT_MAX) {
			atomic_set(&err_count[0], 0);
		}
	break;
	case MI_EVENT_GPU_FAULT:
		atomic_inc(&err_count[1]);
		if (atomic_read(&err_count[1]) > 5 && !time_after64(current_time , atomic64_read(&last_time_gpu_fault) + wait_timeout)) {
			pr_info("%s: gpu_fault error count %d, short time interval, not report\n", __func__, atomic_read(&err_count[1]));
			cdev_tevent_destroy(event);
			return;
		}
		atomic64_set(&last_time_gpu_fault, current_time);
		cdev_tevent_add_int(event, event_name, atomic_read(&err_count[1]));

		if (atomic_read(&err_count[1]) > MI_EVENT_COUNT_MAX) {
			atomic_set(&err_count[1], 0);
		}
	break;
	case MI_EVENT_GPU_HARD_RESET_FAULT:
		atomic_inc(&err_count[2]);
		if (atomic_read(&err_count[2]) > 5 && !time_after64(current_time , atomic64_read(&last_time_gpu_hard_reset_fault) + wait_timeout)) {
			pr_info("%s: gpu_hard_reset_fault error count %d, short time interval, not report\n", __func__, atomic_read(&err_count[2]));
			cdev_tevent_destroy(event);
			return;
		}
		atomic64_set(&last_time_gpu_hard_reset_fault, current_time);
		cdev_tevent_add_int(event, event_name, atomic_read(&err_count[2]));

		if (atomic_read(&err_count[2]) > MI_EVENT_COUNT_MAX) {
			atomic_set(&err_count[2], 0);
		}
	break;
	case MI_EVENT_GPU_CS_FAULT:
		atomic_inc(&err_count[3]);
		if (atomic_read(&err_count[3]) > 5 && !time_after64(current_time , atomic64_read(&last_time_gpu_cs_fault) + wait_timeout)) {
			pr_info("%s: gpu_cs_fault error count %d, short time interval, not report\n", __func__, atomic_read(&err_count[3]));
			cdev_tevent_destroy(event);
			return;
		}
		atomic64_set(&last_time_gpu_cs_fault, current_time);
		cdev_tevent_add_int(event, event_name, atomic_read(&err_count[3]));

		if (atomic_read(&err_count[3]) > MI_EVENT_COUNT_MAX) {
			atomic_set(&err_count[3], 0);
		}
	break;
	case MI_EVENT_GPU_REQUEST_TIMEOUT:
		atomic_inc(&err_count[4]);
		if (atomic_read(&err_count[4]) > 5 && !time_after64(current_time , atomic64_read(&last_time_gpu_request_timeout) + wait_timeout)) {
			pr_info("%s: gpu_request_timeout error count %d, short time interval, not report\n", __func__, atomic_read(&err_count[4]));
			cdev_tevent_destroy(event);
			return;
		}
		atomic64_set(&last_time_gpu_request_timeout, current_time);
		cdev_tevent_add_int(event, event_name, atomic_read(&err_count[4]));

		if (atomic_read(&err_count[4]) > MI_EVENT_COUNT_MAX) {
			atomic_set(&err_count[4], 0);
		}
	break;
	case MI_EVENT_GPU_SOFT_RESET_FAULT:
		atomic_inc(&err_count[5]);
		if (atomic_read(&err_count[5]) > 5 && !time_after64(current_time , atomic64_read(&last_time_gpu_soft_reset_fault) + wait_timeout)) {
			pr_info("%s: gpu_soft_reset_fault error count %d, short time interval, not report\n", __func__, atomic_read(&err_count[5]));
			cdev_tevent_destroy(event);
			return;
		}
		atomic64_set(&last_time_gpu_soft_reset_fault, current_time);
		cdev_tevent_add_int(event, event_name, atomic_read(&err_count[5]));

		if (atomic_read(&err_count[5]) > MI_EVENT_COUNT_MAX) {
			atomic_set(&err_count[5], 0);
		}
	break;
	default:
		pr_err("It's a invalid event_type");
		cdev_tevent_destroy(event);
		return;
	}

	cdev_tevent_write(event);
	cdev_tevent_destroy(event);
}

#endif
