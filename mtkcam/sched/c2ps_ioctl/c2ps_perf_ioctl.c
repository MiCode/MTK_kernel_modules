// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#include "c2ps_perf_ioctl.h"

#define TAG "C2PS_PERF_IOCTL"

int (*c2ps_notify_init_fp)(
	int cfg_camfps, int max_uclamp_cluster0,
	int max_uclamp_cluster1, int max_uclamp_cluster2);
EXPORT_SYMBOL_GPL(c2ps_notify_init_fp);
int (*c2ps_notify_uninit_fp)(void);
EXPORT_SYMBOL_GPL(c2ps_notify_uninit_fp);
int (*c2ps_notify_add_task_fp)(
    u32 task_id, u32 task_target_time, u32 default_uclamp,
	int group_head, u32 task_group_target_time,
	bool is_vip_task, bool is_dynamic_tid,
	const char *task_name);
EXPORT_SYMBOL_GPL(c2ps_notify_add_task_fp);
int (*c2ps_notify_task_start_fp)(int pid, int task_id);
EXPORT_SYMBOL_GPL(c2ps_notify_task_start_fp);
int (*c2ps_notify_task_end_fp)(int pid, int task_id);
EXPORT_SYMBOL_GPL(c2ps_notify_task_end_fp);
int (*c2ps_notify_vsync_fp)(void);
EXPORT_SYMBOL_GPL(c2ps_notify_vsync_fp);
int (*c2ps_notify_camfps_fp)(int camfps);
EXPORT_SYMBOL_GPL(c2ps_notify_camfps_fp);
int (*c2ps_notify_task_scene_change_fp)(int task_id, int scene_mode);
EXPORT_SYMBOL_GPL(c2ps_notify_task_scene_change_fp);
int (*c2ps_notify_task_single_shot_fp)(
	int *uclamp_max, int idle_rate_alert, int timeout,
	int *uclamp_max_placeholder1, int *uclamp_max_placeholder2,
	int *uclamp_max_placeholder3, bool reset_param);
EXPORT_SYMBOL_GPL(c2ps_notify_task_single_shot_fp);

struct proc_dir_entry *c2ps_ioctl_root;
EXPORT_SYMBOL(c2ps_ioctl_root);

int debug_log_on = 0;
module_param(debug_log_on, int, 0644);

static u64 perfctl_copy_from_user(void *pvTo,
    const void __user *pvFrom, u64 ulBytes)
{
	if (access_ok(pvFrom, ulBytes))
		return __copy_from_user(pvTo, pvFrom, ulBytes);

	return ulBytes;
}

static int device_show(struct seq_file *m, void *v)
{
	return 0;
}

static int device_open(struct inode *inode, struct file *file)
{
	return single_open(file, device_show, inode->i_private);
}

static long device_ioctl(
    struct file *filp, unsigned int cmd, unsigned long arg)
{
	ssize_t ret = 0;
	void __user *argp = (void __user *)arg;
	C2PS_PACKAGE c2ps_pkg;
	C2PS_INIT_PARAM c2ps_init_param;
	C2PS_UNINIT_PARAM c2ps_uninit_param;
	C2PS_TASK_INIT_PARAMS c2ps_tsk_init_param;
	C2PS_INFO_NOTIFY c2ps_info;
	C2PS_SINGLE_SHOT_PARAM c2ps_single_shot;

	switch (cmd) {
	#if IS_ENABLED(CONFIG_MTK_C2PS)
	case C2PS_ACTIVATE:
		C2PS_LOGD("C2PS_ACTIVATE");
		if (perfctl_copy_from_user(&c2ps_init_param, argp,
			sizeof(c2ps_init_param))) {
			ret = -EFAULT;
			goto ret_ioctl;
		}
		if (c2ps_notify_init_fp)
			c2ps_notify_init_fp(
					(&c2ps_init_param)->camfps,
					(&c2ps_init_param)->max_uclamp_cluster0,
					(&c2ps_init_param)->max_uclamp_cluster1,
					(&c2ps_init_param)->max_uclamp_cluster2);
		break;
	case C2PS_DESTROY:
		C2PS_LOGD("C2PS_DESTROY");
		if (perfctl_copy_from_user(&c2ps_uninit_param, argp,
			sizeof(c2ps_uninit_param))) {
			ret = -EFAULT;
			goto ret_ioctl;
		}
		if (c2ps_notify_uninit_fp)
			c2ps_notify_uninit_fp();
		break;
	case C2PS_ADD_TASK:
		C2PS_LOGD("C2PS_ADD_TASK");
		if (perfctl_copy_from_user(&c2ps_tsk_init_param, argp,
			sizeof(c2ps_tsk_init_param))) {
			ret = -EFAULT;
			goto ret_ioctl;
		}
		if (c2ps_notify_add_task_fp)
			c2ps_notify_add_task_fp(
			(&c2ps_tsk_init_param)->task_id,
			(&c2ps_tsk_init_param)->task_target_time,
			(&c2ps_tsk_init_param)->default_uclamp,
			(&c2ps_tsk_init_param)->task_group_head,
			(&c2ps_tsk_init_param)->task_group_target_time,
			(&c2ps_tsk_init_param)->is_vip_task,
			(&c2ps_tsk_init_param)->is_dynamic_tid,
			(&c2ps_tsk_init_param)->task_name);
		break;
	case C2PS_TASK_START:
		C2PS_LOGD("C2PS_TASK_START");
		if (perfctl_copy_from_user((&c2ps_pkg), argp,
			sizeof(c2ps_pkg))) {
			ret = -EFAULT;
			goto ret_ioctl;
		}
		if (c2ps_notify_task_start_fp)
			c2ps_notify_task_start_fp(
				(&c2ps_pkg)->tid, (&c2ps_pkg)->task_id);
		break;
	case C2PS_TASK_END:
		C2PS_LOGD("C2PS_TASK_END");
		if (perfctl_copy_from_user(&c2ps_pkg, argp,
			sizeof(c2ps_pkg))) {
			ret = -EFAULT;
			goto ret_ioctl;
		}
		if (c2ps_notify_task_end_fp)
			c2ps_notify_task_end_fp(
				(&c2ps_pkg)->tid, (&c2ps_pkg)->task_id);
		break;
	case C2PS_TASK_CHANGE:
		C2PS_LOGD("C2PS_TASK_CHANGE");
		if (perfctl_copy_from_user(&c2ps_pkg, argp,
			sizeof(c2ps_pkg))) {
			ret = -EFAULT;
			goto ret_ioctl;
		}
		if (c2ps_notify_task_scene_change_fp)
			c2ps_notify_task_scene_change_fp(
			(&c2ps_pkg)->task_id, (&c2ps_pkg)->mode_change_hint);
		break;
	case C2PS_NOTIFY_VSYNC:
		C2PS_LOGD("C2PS_NOTIFY_VSYNC");
		if (c2ps_notify_vsync_fp)
			c2ps_notify_vsync_fp();
		break;
	case C2PS_NOTIFY_CAMFPS:
		C2PS_LOGD("C2PS_NOTIFY_CAMFPS");
		if (perfctl_copy_from_user(&c2ps_info, argp,
			sizeof(c2ps_info))) {
			ret = -EFAULT;
			goto ret_ioctl;
		}
		if (c2ps_notify_camfps_fp)
			c2ps_notify_camfps_fp((&c2ps_info)->cur_camfps);
		break;
	case C2PS_TASK_SINGLE_SHOT:
		C2PS_LOGD("C2PS_SINGLE_SHOT");
		if (perfctl_copy_from_user(&c2ps_single_shot, argp,
			sizeof(c2ps_single_shot))) {
			ret = -EFAULT;
			goto ret_ioctl;
		}
		if (c2ps_notify_task_single_shot_fp)
			c2ps_notify_task_single_shot_fp(
			(&c2ps_single_shot)->uclamp_max,
			(&c2ps_single_shot)->idle_rate_alert,
			(&c2ps_single_shot)->timeout,
			(&c2ps_single_shot)->uclamp_max_placeholder1,
			(&c2ps_single_shot)->uclamp_max_placeholder2,
			(&c2ps_single_shot)->uclamp_max_placeholder3,
			(&c2ps_single_shot)->reset_param);
		break;
	#else
	case C2PS_ACTIVATE:
		[[fallthrough]];
	case C2PS_DESTROY:
		[[fallthrough]];
	case C2PS_ADD_TASK:
		[[fallthrough]];
	case C2PS_TASK_START:
		[[fallthrough]];
	case C2PS_TASK_END:
		[[fallthrough]];
	case C2PS_TASK_CHANGE:
		[[fallthrough]];
	case C2PS_NOTIFY_VSYNC:
		[[fallthrough]];
	case C2PS_NOTIFY_CAMFPS:
		[[fallthrough]];
	case C2PS_TASK_SINGLE_SHOT:
		[[fallthrough]];
	break;
	#endif

	default:
		C2PS_LOGD(TAG " %s %d: unknown cmd %x\n",
			__FILE__, __LINE__, cmd);
		ret = -EINVAL;
		goto ret_ioctl;
	}

ret_ioctl:
	return ret;
}

static const struct proc_ops Fops = {
	.proc_compat_ioctl = device_ioctl,
	.proc_ioctl = device_ioctl,
	.proc_open = device_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static void __exit exit_c2ps_perf_ioctl(void) {}
static int __init init_c2ps_perf_ioctl(void)
{
	struct proc_dir_entry *pe, *parent;

	C2PS_LOGD(TAG"Start to init c2ps_ioctl driver\n");

	parent = proc_mkdir("c2ps", NULL);
	c2ps_ioctl_root = parent;

	pe = proc_create("c2ps_ioctl", 0660, parent, &Fops);
	if (!pe) {
		C2PS_LOGD(TAG"%s failed with %d\n",
			"Creating file node ",
			ENOMEM);
		return -ENOMEM;
	}

	C2PS_LOGD(TAG"init c2ps_ioctl driver done\n");

	return 0;
}

module_init(init_c2ps_perf_ioctl);
module_exit(exit_c2ps_perf_ioctl);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek C2PS c2ps_perf_ioctl");
MODULE_AUTHOR("MediaTek Inc.");
