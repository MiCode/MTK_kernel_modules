// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 *
 * Author: ChenHung Yang <chenhung.yang@mediatek.com>
 */

#include <linux/wait.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <uapi/linux/dma-heap.h>

#include "mtk_heap.h"

#include "mtk-aov-config.h"
#include "mtk-aov-drv.h"
#include "mtk-aov-core.h"
#include "mtk-aov-mtee.h"
#include "mtk-aov-aee.h"
#include "mtk-aov-data.h"
#include "mtk-aov-trace.h"
#include "mtk-aov-log.h"
#include "mtk-aov-ulposc.h"
#include "mtk-aov-regs.h"

#include "mtk-vmm-notifier.h"
#include "mtk_mmdvfs.h"

#include "slbc_ops.h"
#include "scp.h"
#include <soc/mediatek/smi.h>
#include <soc/mediatek/emi.h>

#if AOV_EVENT_IN_PLACE
#define ALIGN16(x) ((void *)(((uint64_t)x + 0x0F) & ~0x0F))
#else
#define ALIGN16(x) (x)
#endif  // AOV_EVENT_IN_PLACE

static struct mtk_aov *curr_dev;

struct mtk_aov *aov_core_get_device(void)
{
	return curr_dev;
}

static int send_cmd_internal(struct aov_core *core_info,
	uint32_t cmd_code, uint32_t buffer, uint32_t length, bool wait, bool ack)
{
	struct mtk_aov *aov_dev = aov_core_get_device();
	struct packet packet;
	int count;
	int retry;
	unsigned long timeout;
	int scp_ready;
	int cmd_seq;
	int ret;

	mutex_lock(&core_info->sned_ipi_mutex);

	cmd_seq = atomic_add_return(1, &(core_info->cmd_seq));

	dev_info(aov_dev->dev, "%s: send seq(%d), cmd(%d)+\n",
		__func__, cmd_seq, cmd_code);

	(void)aov_aee_record(aov_dev, cmd_seq, cmd_code);

	packet.sequence = cmd_seq;
	packet.command  = cmd_code;
	packet.buffer   = buffer;
	packet.length   = length;
	packet.auth     = AOV_SCP_CMD_ACK - cmd_code;

	AOV_TRACE_BEGIN("AOV Send Cmd");

	retry = 0;
	do {
		if (wait) {
			timeout = msecs_to_jiffies(AOV_TIMEOUT_MS);

			count = 0;
			do {
				ret = wait_event_interruptible_timeout(core_info->scp_queue,
					((scp_ready = atomic_read(&(core_info->scp_ready))) == 2),
					timeout);
				if (ret == 0) {
					AOV_TRACE_END();
					dev_info(aov_dev->dev, "%s: send cmd(%d/%d) timeout!\n",
						__func__, cmd_code, scp_ready);
					mutex_unlock(&core_info->sned_ipi_mutex);
					return -EIO;
				} else if (-ERESTARTSYS == ret) {
					if (count++ >= 100) {
						AOV_TRACE_END();
						dev_info(aov_dev->dev, "%s: send cmd(%d/%d/%d) failed\n",
							__func__, cmd_code, scp_ready, count);
						mutex_unlock(&core_info->sned_ipi_mutex);
						return -ERESTARTSYS;
					}

					AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
						"%s: send cmd(%d/%d/%d) interrupted !\n",
						__func__, cmd_code, scp_ready, count);

					/* retry again after 1ms */
					udelay(1000);
					continue;
				} else {
					AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
						"%s: send cmd(%d/%d) done\n",
						__func__, cmd_code, scp_ready);
					break;
				}
			} while (1);
		}

		packet.session = atomic_read(&(core_info->scp_session));

		if (ack) {
			atomic_set(&(core_info->ack_cmd[cmd_code]), 0);
			packet.command |= AOV_SCP_CMD_ACK;
		}

		ret = mtk_ipi_send(&scp_ipidev, IPI_OUT_AOV_SCP,
			IPI_SEND_WAIT, &packet, 4, AOV_TIMEOUT_MS);
		if (ret < 0 && ret != IPI_PIN_BUSY) {
			dev_info(aov_dev->dev, "%s: failed to send packet: %d", __func__, ret);
			break;
		}
		if (ret == IPI_PIN_BUSY) {
			if (retry++ >= 100) {
				AOV_TRACE_END();
				dev_info(aov_dev->dev, "%s: failed to send cmd(%d): %d\n",
					__func__, cmd_code, ret);
				mutex_unlock(&core_info->sned_ipi_mutex);
				return -EBUSY;
			}
			if (retry % 100 == 0)
				usleep_range(1000, 2000);
		} else if (ack) {
			timeout = msecs_to_jiffies(AOV_TIMEOUT_MS);

			count = 0;
			do {
				ret = wait_event_interruptible_timeout(core_info->ack_wq[cmd_code],
					atomic_cmpxchg(&(core_info->ack_cmd[cmd_code]), 1, 0),
					timeout);
				if (ret == 0) {
					AOV_TRACE_END();
					dev_info(aov_dev->dev, "%s: wait ack cmd(%d) timeout\n",
						__func__, cmd_code);
					mutex_unlock(&core_info->sned_ipi_mutex);
					return -EIO;
				} else if (-ERESTARTSYS == ret) {
					if (count++ >= 100) {
						AOV_TRACE_END();
						dev_info(aov_dev->dev, "%s: wait cmd(%d/%d) ack failed\n",
							__func__, cmd_code, count);
						mutex_unlock(&core_info->sned_ipi_mutex);
						return -ERESTARTSYS;
					}

					AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
						"%s: wait cmd(%d/%d) ack interrupted\n",
						__func__, cmd_code, count);

					/* retry again after 1ms */
					udelay(1000);
					continue;
				} else {
					AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
						"%s: wait cmd(%d) ack done\n",
						__func__, cmd_code);
					ret = 0;
					break;
				}
			} while (1);
		}
	} while (ret == IPI_PIN_BUSY);

	AOV_TRACE_END();

	dev_info(aov_dev->dev, "%s: send seq(%d), cmd(%d)-\n",
		__func__, cmd_seq, cmd_code);

	mutex_unlock(&core_info->sned_ipi_mutex);

	return 0;
}

static void *buffer_acquire(struct aov_core *core_info)
{
	unsigned long flag, empty;
	struct buffer *buffer = NULL;

	spin_lock_irqsave(&core_info->event_lock, flag);
	empty = list_empty(&core_info->event_list);
	if (!empty) {
		buffer = list_first_entry(&core_info->event_list, struct buffer, entry);
		list_del(&buffer->entry);
	}
	spin_unlock_irqrestore(&core_info->event_lock, flag);

	return buffer ? &(buffer->data) : NULL;
}

static void buffer_release(struct aov_core *core_info, void *data)
{
	unsigned long flag;
	struct buffer *buffer =
		(struct buffer *)((uintptr_t)data - offsetof(struct buffer, data));

	spin_lock_irqsave(&core_info->event_lock, flag);
	list_add_tail(&buffer->entry, &core_info->event_list);
	spin_unlock_irqrestore(&core_info->event_lock, flag);
}

static int copy_event_data(struct mtk_aov *aov_dev,
		struct base_event *event)
{
	struct aov_core *core_info = &aov_dev->core_info;
	struct aov_notify info;
	uint32_t frame_mode;
	uint32_t debug_mode;
	uint32_t power_mode;
	void *buffer;
	int ret;

	AOV_TRACE_BEGIN("AOV Copy Event");

	if (event == NULL) {
		AOV_TRACE_END();
		return -EINVAL;
	}

	// Is FR mode
	frame_mode = atomic_read(&(core_info->frame_mode));
	if (frame_mode & 0x20) {
		ret = aov_mtee_notify(aov_dev, &(event->fr_info));
		if ((event->detect_mode == 0) && (ret <= 0)) {
			AOV_TRACE_END();
			dev_info(aov_dev->dev, "%s: don't have face in event: ret(%d)\n",
					__func__, ret);
			return ret;
		}
	}

	buffer = buffer_acquire(core_info);
	if (buffer == NULL) {
#if AOV_FORCE_SKIP_MODE
		buffer = queue_pop(&(core_info->queue));
		if (buffer == NULL) {
			AOV_TRACE_END();
			dev_info(aov_dev->dev, "%s: failed to discard event\n", __func__);
			return -ENOMEM;
		}
#else
		AOV_TRACE_END();
		dev_info(aov_dev->dev, "%s: failed to acquire message\n", __func__);
		return -ENOMEM;
#endif  // AOV_FORCE_SKIP_MODE
	}

	debug_mode = atomic_read(&(core_info->debug_mode));
	power_mode = atomic_read(&(core_info->power_mode));
	if (debug_mode == AOV_DEBUG_MODE_NDD) {
		// Copy yuvo1/yuvo2/imgo and etc.
		memcpy(buffer, (void *)event, sizeof(struct ndd_event));
	} else {
		if (!power_mode) {
			// Only copy yuvo1/yuvo2/aie/fld/apu out
			memcpy(buffer, (void *)event, sizeof(struct base_event));
		} else {
			// Only copy aie/fld/apu output
			memcpy(buffer, (void *)event, offsetof(struct base_event, yuvo1_width));
		}
	}

	if (atomic_read(&(core_info->aov_ready))) {
		dev_info(aov_dev->dev, "%s: release aov event id(%d)\n", __func__, event->event_id);

		info.notify = AOV_NOTIFY_EVT_AVAIL;
		info.status = event->event_id;

		(void)aov_core_send_cmd(aov_dev, AOV_SCP_CMD_NOTIFY,
			(void *)&info, sizeof(struct aov_notify), true);
	}

	(void)queue_push(&(core_info->queue), buffer);

	AOV_TRACE_END();

	return 0;
}

static int ipi_receive(unsigned int id, void *unused,
		void *data, unsigned int len)
{
	struct mtk_aov *aov_dev = aov_core_get_device();
	struct aov_core *core_info = &aov_dev->core_info;
	uint32_t aov_session;
	struct packet *packet;
	struct base_event *event;

	AOV_TRACE_BEGIN("AOV Recv IPI");

	if (aov_dev->op_mode == 0) {
		AOV_TRACE_END();
		dev_info(aov_dev->dev, "%s: bypass ipi receive operation", __func__);
		return 0;
	}

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
		"%s: receive id(%d), len(%d)\n",
		__func__, id, len);

	packet = (struct packet *)data;
	if (packet->command & AOV_SCP_CMD_ACK) {
		uint32_t cmd = packet->command & ~AOV_SCP_CMD_ACK;

		if ((cmd > 0) && (cmd < AOV_SCP_CMD_MAX)) {
			atomic_set(&(core_info->ack_cmd[cmd]), 1);
			wake_up_interruptible(&core_info->ack_wq[cmd]);
		}
	} else if (packet->command == AOV_SCP_CMD_SMI_DUMP) {
		dev_info(aov_dev->dev, "%s: receive SMI DUMP signal from SCP (%u)\n", __func__, packet->buffer);
		core_info->smi_dump_id = packet->buffer;
		atomic_set(&(core_info->do_smi_dump), 1);
		wake_up_interruptible(&core_info->smi_dump_wq);
	} else if (packet->command == AOV_SCP_CMD_RESET_SENSOR) {
		dev_info(aov_dev->dev, "%s: receive reset sensor signal from SCP\n", __func__);
		atomic_set(&(core_info->do_reset_sensor), 1);
		wake_up_interruptible(&core_info->reset_sensor_wq);
	} else {
		event = (struct base_event *)(core_info->buf_va +
			(packet->buffer - core_info->buf_pa));

		aov_session = atomic_read(&(core_info->aov_session));

		if (event->session != aov_session) {
			AOV_TRACE_END();
			dev_info(aov_dev->dev, "%s: invalid aov session mismatch(%d/%d)\n",
				__func__, event->session, aov_session);
			return 0;
		}

		(void)queue_push(&(core_info->event), event);
		wake_up_interruptible(&core_info->poll_wq);
	}

	AOV_TRACE_END();

	return 0;
}

int aov_core_send_cmd(struct mtk_aov *aov_dev, uint32_t cmd,
	void *data, int len, bool ack)
{
	struct aov_core *core_info = &aov_dev->core_info;
	unsigned long flag;
	uint8_t *buf;
	uint32_t buffer;
	uint32_t length;
	int ret;
	struct aov_start *start = NULL;
	struct aov_start_v2 *start_v2 = NULL;

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "%s+\n", __func__);

	if (aov_dev->op_mode == 0) {
		dev_info(aov_dev->dev, "%s: bypass send command(%d)", __func__, cmd);
		return 0;
	}

	if (data && len) {
		if (cmd == AOV_SCP_CMD_START) {
			if (atomic_read(&(core_info->aov_ready))) {
				dev_info(aov_dev->dev, "%s: invalid aov ready state\n", __func__);
				return -EIO;
			}

			AOV_TRACE_BEGIN("AOV Alloc Init");
			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "aov malloc init+\n");
			spin_lock_irqsave(&core_info->buf_lock, flag);
			if (aov_dev->fd_version == 2)
				buf = tlsf_malloc(&(core_info->alloc), sizeof(struct aov_start_v2));
			else
				buf = tlsf_malloc(&(core_info->alloc), sizeof(struct aov_start));
			spin_unlock_irqrestore(&core_info->buf_lock, flag);
			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "aov malloc init-\n");
			AOV_TRACE_END();
		} else {
			AOV_TRACE_BEGIN("AOV Alloc Buffer");
			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "aov malloc buffer+\n");
			spin_lock_irqsave(&core_info->buf_lock, flag);
			buf = tlsf_malloc(&(core_info->alloc), len);
			spin_unlock_irqrestore(&core_info->buf_lock, flag);
			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "aov malloc buffer-\n");
			AOV_TRACE_END();
		}
		if (buf) {
			if (cmd == AOV_SCP_CMD_START) {
				struct aov_user user;
				if (aov_dev->fd_version == 2) {
					start_v2 = (struct aov_start_v2 *)buf;

					AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
						"%s: init buffer %p, size (%ld/%ld)\n",
						__func__, buf, sizeof(struct aov_start_v2),
						sizeof(struct base_event));
				} else {
					start = (struct aov_start *)buf;

					AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
						"%s: init buffer %p, size (%ld/%ld)\n",
						__func__, buf, sizeof(struct aov_start),
						sizeof(struct base_event));
				}

				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"%s: copy aov user data %p, %ld+\n",
					__func__, data, sizeof(struct aov_user));

				ret = copy_from_user((void *)&user,
					(void *)data, sizeof(struct aov_user));
				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"%s: copy aov user data %p, %ld-\n",
					__func__, data, sizeof(struct aov_user));
				if (ret) {
					dev_info(aov_dev->dev, "%s: failed to copy aov user data: %d\n",
						__func__, ret);
					return -EFAULT;
				}

				if (aov_dev->fd_version == 2) {
					memcpy(start_v2, &user, AOV_MAX_USER_SIZE);
				} else {
					memcpy(start, &user, AOV_MAX_USER_SIZE);
				}

				if ((user.aaa_size > 0) &&
					(user.aaa_size <= AOV_MAX_AAA_SIZE)) {
					AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
						"%s: copy aaa info %p, %d+\n",
						__func__, user.aaa_info, user.aaa_size);
					if (aov_dev->fd_version == 2) {
						(void)copy_from_user(&(start_v2->aaa_info),
							user.aaa_info, user.aaa_size);
					} else {
						(void)copy_from_user(&(start->aaa_info),
							user.aaa_info, user.aaa_size);
					}
					AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
						"%s: copy aaa info %p, %d-\n",
						__func__, user.aaa_info, user.aaa_size);
				} else if (user.aaa_size > AOV_MAX_AAA_SIZE) {
					dev_info(aov_dev->dev, "%s: aaa info size(%d) overflow\n",
						__func__, user.aaa_size);
					return -ENOMEM;
				}

				if ((user.tuning_size > 0) &&
					(user.tuning_size <= AOV_MAX_TUNING_SIZE)) {
					AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
						"%s: copy tuning info %p, %d+\n",
						__func__, user.tuning_info,
						user.tuning_size);
					if (aov_dev->fd_version == 2) {
						(void)copy_from_user(&(start_v2->tuning_info),
							user.tuning_info, user.tuning_size);
					} else {
						(void)copy_from_user(&(start->tuning_info),
							user.tuning_info, user.tuning_size);
					}
					AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
						"%s: copy tuning info %p, %d-\n",
						__func__, user.tuning_info,
						user.tuning_size);
				} else if (user.tuning_size > AOV_MAX_TUNING_SIZE) {
					dev_info(aov_dev->dev, "%s: tuning info size(%d) overflow\n",
						__func__, user.tuning_size);
					return -ENOMEM;
				}

				core_info->sensor_id = user.pipe_id;

				/* set seninf aov parameters for scp use and
				 * switch i2c bus aux function here on scp side.
				 */
				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"mtk_cam_seninf_s_aov_param(%d/%d)+\n",
					core_info->sensor_id, INIT_NORMAL);
				if (aov_dev->fd_version == 2) {
					ret = mtk_cam_seninf_s_aov_param(core_info->sensor_id,
						(void *)&(start_v2->senif_info), INIT_NORMAL);
				} else {
					ret = mtk_cam_seninf_s_aov_param(core_info->sensor_id,
						(void *)&(start->senif_info), INIT_NORMAL);
				}
				if (ret < 0)
					dev_info(aov_dev->dev,
						"mtk_cam_seninf_s_aov_param(%d/%d) fail, ret: %d\n",
						core_info->sensor_id, INIT_NORMAL, ret);
				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"mtk_cam_seninf_s_aov_param(%d/%d)-\n",
					core_info->sensor_id, INIT_NORMAL);

				/* suspend and set clk parent here to prevent enque
				 * racing issue when power on/off on scp side.
				 */
				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"mtk_cam_seninf_aov_runtime_suspend(%d)+\n",
					core_info->sensor_id);
				AOV_TRACE_BEGIN("AOV Seninf Suspend");
				ret = mtk_cam_seninf_aov_runtime_suspend(core_info->sensor_id);
				AOV_TRACE_END();
				if (ret < 0)
					dev_info(aov_dev->dev,
					"mtk_cam_seninf_aov_runtime_suspend(%d) fail, ret: %d\n",
					core_info->sensor_id, ret);
				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"mtk_cam_seninf_aov_runtime_suspend(%d)-\n",
					core_info->sensor_id);

				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"MTK FD COPY, version(%u)+\n", aov_dev->fd_version);
				AOV_TRACE_BEGIN("AOV Copy FD");
				if (aov_dev->fd_version == 1)
					mtk_aie_aov_memcpy((void *)&(start->aie_info));
				else if (aov_dev->fd_version == 2)
					mtk_aie_aov_memcpy((void *)&(start_v2->aie_info));
				AOV_TRACE_END();
				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"MTK FD COPY, version(%u)-\n", aov_dev->fd_version);

				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"MTK FLD COPY, version(%u)+\n", aov_dev->fd_version);
				AOV_TRACE_BEGIN("AOV Copy FLD");
				if (aov_dev->fd_version == 1)
					mtk_fld_aov_memcpy((void *)&(start->fld_info));
				else if (aov_dev->fd_version == 2)
					mtk_fld_aov_memcpy((void *)&(start_v2->fld_info));
				AOV_TRACE_END();
				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"MTK FLD COPY, version(%u)-\n", aov_dev->fd_version);

				/* debug use
				 * dev_info(aov_dev->dev, "out_pad %d\n",
				 *  start->aov_seninf_param.vc.out_pad);
				 * dev_info(aov_dev->dev, "start->aov_seninf_param.sensor_idx%d\n",
				 *  start->aov_seninf_param.sensor_idx);
				 * dev_info(aov_dev->dev, "width %d\n",
				 *  start->aov_seninf_param.width);
				 * dev_info(aov_dev->dev, "height %d\n",
				 *  start->aov_seninf_param.height);
				 * dev_info(aov_dev->dev, "isp_freq %d\n",
				 *  start->aov_seninf_param.isp_freq);
				 * dev_info(aov_dev->dev, "camtg %d\n",
				 *  start->aov_seninf_param.camtg);
				 */
				if (aov_dev->fd_version == 2)
					start_v2->session = user.session;
				else
					start->session = user.session;

				atomic_set(&(core_info->aov_session), user.session);

				len = (aov_dev->fd_version == 2) ? sizeof(struct aov_start_v2)
					: sizeof(struct aov_start);
			} else if (cmd == AOV_SCP_CMD_NOTIFY) {
				memcpy(buf, (void *)data, sizeof(struct aov_notify));
			} else {
				AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
					"%s: data buffer %p, size %d\n",
					__func__, buf, len);
				(void)copy_from_user(buf, (void *)data, len);
			}
		} else {
			dev_info(aov_dev->dev, "%s: failed to alloc buffer size(%d)",
				__func__, len);
			return -ENOMEM;
		}
#if AOV_SLB_ALLOC_FREE
	} else if (cmd == AOV_SCP_CMD_PWR_OFF) {
		struct slbc_data slb;

		slb.uid = UID_AOV_DC;
		slb.type = TP_BUFFER;
		AOV_TRACE_BEGIN("AOV Alloc SLB");
		ret = slbc_request(&slb);
		AOV_TRACE_END();
		if (ret < 0) {
			dev_info(aov_dev->dev, "%s: failed to allocate slb buffer", __func__);
			return ret;
		}

		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
			"%s: slb buffer base(0x%x), size(%ld): %x\n",
			__func__, (uintptr_t)slb.paddr, slb.size);

		buf = slb.paddr;
		len = slb.size;
#endif  // AOV_SLB_ALLOC_FREE
	} else {
		buf = NULL;
	}

	if (cmd == AOV_SCP_CMD_START) {
		if (aov_dev->fd_version == 2) {
			start_v2 = (struct aov_start_v2 *)buf;

			if (start_v2 == NULL) {
				dev_info(aov_dev->dev, "%s: invalid null aov init info", __func__);
				return -ENOMEM;
			}
		} else {
			start = (struct aov_start *)buf;

			if (start == NULL) {
				dev_info(aov_dev->dev, "%s: invalid null aov init info", __func__);
				return -ENOMEM;
			}
		}

		if (aov_dev->fd_version == 2) {
			// Setup frame mode
			atomic_set(&(core_info->frame_mode), start_v2->frame_mode);

			// Setup debug mode
			atomic_set(&(core_info->debug_mode), start_v2->debug_mode);

			// Setup display mode
			start_v2->disp_mode = atomic_read(&(core_info->disp_mode));

			// Setup aie available
			start_v2->aie_avail = atomic_read(&(core_info->aie_avail));

			// Setup power mode
			atomic_set(&(core_info->power_mode), start_v2->power_mode);

			// Record aov_start buffer
			core_info->aov_start = start_v2;
		} else {
			// Setup frame mode
			atomic_set(&(core_info->frame_mode), start->frame_mode);

			// Setup debug mode
			atomic_set(&(core_info->debug_mode), start->debug_mode);

			// Setup display mode
			start->disp_mode = atomic_read(&(core_info->disp_mode));

			// Setup aie available
			start->aie_avail = atomic_read(&(core_info->aie_avail));

			// Setup power mode
			atomic_set(&(core_info->power_mode), start->power_mode);

			// Record aov_start buffer
			core_info->aov_start = start;
		}

		atomic_set(&(core_info->aov_ready), 1);
	} else if (cmd == AOV_SCP_CMD_PWR_OFF) {
		atomic_set(&(core_info->disp_mode), AOV_DISP_MODE_OFF);
	} else if (cmd == AOV_SCP_CMD_PWR_ON) {
		atomic_set(&(core_info->disp_mode), AOV_DiSP_MODE_ON);
	} else if (cmd == AOV_SCP_CMD_NOTIFY) {
		struct aov_notify *notify = (struct aov_notify *)buf;

		if (notify == NULL) {
			dev_info(aov_dev->dev, "%s: invalid null aov notify info", __func__);
			return -ENOMEM;
		}

		if (notify->notify == AOV_NOTIFY_AIE_AVAIL)
			atomic_set(&(core_info->aie_avail), notify->status);
	} else if (cmd == AOV_SCP_CMD_QEA) {
		vmm_isp_ctrl_notify(1);
		mtk_mmdvfs_aov_enable(1);
		atomic_set(&(core_info->qea_ready), 1);
		send_cmd_internal(core_info, cmd, 0, 0, false, true);
		atomic_set(&(core_info->qea_ready), 0);
	} else if (cmd == AOV_SCP_CMD_PWR_UT) {
		vmm_isp_ctrl_notify(1);
		mtk_mmdvfs_aov_enable(1);
		send_cmd_internal(core_info, cmd, 0, 0, false, false);
	} else if (cmd == AOV_SCP_CMD_ON_UT) {
		vmm_isp_ctrl_notify(1);
		mtk_mmdvfs_aov_enable(1);
		send_cmd_internal(core_info, cmd, 0, 0, false, false);
	} else if (cmd == AOV_SCP_CMD_OFF_UT) {
		vmm_isp_ctrl_notify(1);
		mtk_mmdvfs_aov_enable(1);
		send_cmd_internal(core_info, cmd, 0, 0, false, false);
	} else if (cmd == AOV_SCP_CMD_TURN_ON_ULPOSC) {
		send_cmd_internal(core_info, cmd, 0, 0, false, true);
	} else if (cmd == AOV_SCP_CMD_TURN_OFF_ULPOSC) {
		send_cmd_internal(core_info, cmd, 0, 0, false, true);
	}

	if (atomic_read(&(core_info->aov_ready))) {
		if (buf) {
			buffer = core_info->buf_pa + (buf - core_info->buf_va);
			length = len;
		} else {
			buffer = 0;
			length = 0;
		}
		if (*(aov_dev->bypass_aov_scp_flag)) {
			dev_info(aov_dev->dev, "skip flow below AOV SCP!\n");
		} else {
			(void)send_cmd_internal(core_info, cmd, buffer, length, true, ack);
		}
	} else {
		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
			"%s: aov is not started(%d)\n",
			__func__, cmd);
	}

	if (cmd == AOV_SCP_CMD_PWR_ON) {
#if AOV_SLB_ALLOC_FREE
		struct slbc_data slb;

		slb.uid = UID_AOV_DC;
		slb.type = TP_BUFFER;
		AOV_TRACE_BEGIN("AOV Release SLB");
		ret = slbc_release(&slb);
		AOV_TRACE_END();
		if (ret < 0) {
			dev_info(aov_dev->dev, "%s: failed to release slb buffer: %d",
				__func__, ret);
			return ret;
		}
#else
		struct slbc_data slb;

		slb.uid = UID_AOV_DC;
		slb.type = TP_BUFFER;
		ret = slbc_status(&slb);
		if (ret > 0) {
			dev_info(aov_dev->dev,
				"%s: still have slb user at resume. ref count: %d\n",
				__func__, ret);
		}
#endif  // AOV_SLB_ALLOC_FREE
	} else if (cmd == AOV_SCP_CMD_NOTIFY) {
		// Free aov_notify buffer
		AOV_TRACE_BEGIN("AOV Free Buffer");
		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "aov free buffer+\n");
		spin_lock_irqsave(&core_info->buf_lock, flag);
		if (buf != NULL)
			tlsf_free(&(core_info->alloc), buf);
		else
			dev_info(aov_dev->dev, "aov free buffer is NULL");
		spin_unlock_irqrestore(&core_info->buf_lock, flag);
		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "aov free buffer-\n");
		AOV_TRACE_END();
	} else if (cmd == AOV_SCP_CMD_STOP) {
		atomic_set(&(core_info->aov_ready), 0);

		// Free aov_start buffer
		AOV_TRACE_BEGIN("AOV Free Start");
		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "aov free start+\n");
		spin_lock_irqsave(&core_info->buf_lock, flag);
		tlsf_free(&(core_info->alloc), core_info->aov_start);
		spin_unlock_irqrestore(&core_info->buf_lock, flag);
		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "aov free start-\n");
		AOV_TRACE_END();

		// Reset queue to empty
		while (!queue_empty(&(core_info->event)))
			(void)queue_pop(&(core_info->event));
		queue_deinit(&(core_info->event));
		queue_init(&(core_info->event));

		// Reset queue to empty
		while (!queue_empty(&(core_info->queue))) {
			buf = queue_pop(&(core_info->queue));
			if (buf)
				buffer_release(core_info, buf);
		}
		queue_deinit(&(core_info->queue));
		queue_init(&(core_info->queue));

		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
			"mtk_cam_seninf_aov_runtime_resume(%d/%d)+\n",
			core_info->sensor_id, DEINIT_NORMAL);
		mtk_cam_seninf_aov_runtime_resume(core_info->sensor_id,
			DEINIT_NORMAL);
		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
			"mtk_cam_seninf_aov_runtime_resume(%d/%d)-\n",
			core_info->sensor_id, DEINIT_NORMAL);
	} else if (cmd == AOV_SCP_CMD_START) {
		dev_info(aov_dev->dev, "%s: notify seninf to close mclk", __func__);
		mtk_cam_seninf_aov_sensor_set_mclk(core_info->sensor_id, 0);
	}

		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "%s-\n", __func__);

	return 0;
}

int aov_core_notify(struct mtk_aov *aov_dev,
	void *data, bool enable)
{
	int ret;
	struct sensor_notify notify;
	// int index;

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "%s+\n", __func__);

	if (aov_dev->op_mode == 0) {
		dev_info(aov_dev->dev, "%s: bypass notify operation", __func__);
		return 0;
	}

	ret = copy_from_user((void *)&notify,
		(void *)data, sizeof(struct sensor_notify));
	if (ret) {
		dev_info(aov_dev->dev, "%s: failed to copy senor notification: %d\n",
			__func__, ret);
		return -EFAULT;
	}

	if (notify.count >= AOV_MAX_SENSOR_COUNT) {
		dev_info(aov_dev->dev, "%s: invalid sensor notify count(%d)\n",
			__func__, notify.count);
		return -EINVAL;
	}

	//for (index = 0; index < notify.count; index++) {
	//  dev_dbg(aov_dev->dev, "%s: notify sensor(%d), status(%d)\n",
	//    __func__, notify.sensor[index], enable);
	//}

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "%s-\n", __func__);

	return 0;
}

static int aov_core_recover(struct mtk_aov *aov_dev)
{
	struct aov_core *core_info = &aov_dev->core_info;
	uint32_t buffer;
	uint32_t length;
	int ret;
	struct aov_start *start = NULL;
	struct aov_start_v2 *start_v2 = NULL;

	dev_info(aov_dev->dev, "%s+\n", __func__);

	pm_stay_awake(aov_dev->dev);

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
		"mtk_cam_seninf_aov_runtime_resume(%d/%d)+\n",
		core_info->sensor_id, DEINIT_ABNORMAL_SCP_STOP);
	mtk_cam_seninf_aov_runtime_resume(core_info->sensor_id,
		DEINIT_ABNORMAL_SCP_STOP);
	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
		"mtk_cam_seninf_aov_runtime_resume(%d/%d)-\n",
		core_info->sensor_id, DEINIT_ABNORMAL_SCP_STOP);

	if (aov_dev->fd_version == 2) {
		start_v2 = core_info->aov_start;
		if (start_v2) {
			buffer =
				core_info->buf_pa + (((uint8_t *)start_v2) - core_info->buf_va);
			length = (aov_dev->fd_version == 2) ? sizeof(struct aov_start_v2) : sizeof(struct aov_start);
		} else {
			pm_relax(aov_dev->dev);
			dev_info(aov_dev->dev, "%s: invalid null aov start_v2 parameter\n",
				__func__);
			return -1;
		}
	} else {
		start = core_info->aov_start;
		if (start) {
			buffer =
				core_info->buf_pa + (((uint8_t *)start) - core_info->buf_va);
			length = (aov_dev->fd_version == 2) ? sizeof(struct aov_start_v2) : sizeof(struct aov_start);
		} else {
			pm_relax(aov_dev->dev);
			dev_info(aov_dev->dev, "%s: invalid null aov start parameter\n",
				__func__);
			return -1;
		}
	}

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
		"mtk_cam_seninf_s_aov_param(%d/%d)+\n",
		core_info->sensor_id, INIT_ABNORMAL_SCP_READY);
	if (aov_dev->fd_version == 2) {
		ret = mtk_cam_seninf_s_aov_param(core_info->sensor_id,
			(void *)&(start_v2->senif_info), INIT_ABNORMAL_SCP_READY);
	} else {
		ret = mtk_cam_seninf_s_aov_param(core_info->sensor_id,
			(void *)&(start->senif_info), INIT_ABNORMAL_SCP_READY);
	}
	if (ret < 0)
		dev_info(aov_dev->dev,
			"mtk_cam_seninf_s_aov_param(%d/%d) fail, ret: %d\n",
			core_info->sensor_id, INIT_ABNORMAL_SCP_READY, ret);
	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
		"mtk_cam_seninf_s_aov_param(%d/%d)-\n",
		core_info->sensor_id, INIT_ABNORMAL_SCP_READY);

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
		"mtk_cam_seninf_aov_runtime_suspend(%d)+\n",
		core_info->sensor_id);
	ret = mtk_cam_seninf_aov_runtime_suspend(core_info->sensor_id);
	if (ret < 0)
		dev_info(aov_dev->dev,
			"mtk_cam_seninf_aov_runtime_suspend(%d) fail, ret: %d\n",
			core_info->sensor_id, ret);
	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
		"mtk_cam_seninf_aov_runtime_suspend(%d)-\n",
		core_info->sensor_id);

	if (aov_dev->fd_version == 2) {
		// Setup display mode
		start_v2->disp_mode = atomic_read(&(core_info->disp_mode));

		// Setup aie available
		start_v2->aie_avail = atomic_read(&(core_info->aie_avail));
	} else {
		// Setup display mode
		start->disp_mode = atomic_read(&(core_info->disp_mode));

		// Setup aie available
		start->aie_avail = atomic_read(&(core_info->aie_avail));
	}

	ret = send_cmd_internal(core_info, AOV_SCP_CMD_START, buffer,
		length, false, true);
	if (ret < 0)
		dev_info(aov_dev->dev, "%s: failed to do aov recovery: %d\n",
			__func__, ret);

	pm_relax(aov_dev->dev);

	dev_info(aov_dev->dev, "%s-\n", __func__);

	return 0;
}

static int scp_state_notify(struct notifier_block *this,
	unsigned long event, void *ptr)
{
	struct mtk_aov *aov_dev = aov_core_get_device();
	struct aov_core *core_info = &aov_dev->core_info;
	struct slbc_data slb;
	uint32_t session;
	int ret;

	if (event == SCP_EVENT_STOP) {
		mutex_lock(&core_info->start_stop_mutex);
		(void)aov_aee_record(aov_dev, 0, SCP_STOP);
		(void)aov_aee_flush(aov_dev);

		dev_info(aov_dev->dev, "%s: receive scp stop event(%lu)\n", __func__, event);

		atomic_set(&(core_info->scp_ready), 1);

		if (atomic_read(&(core_info->aov_ready))) {
			slb.uid = UID_AOV_DC;
			slb.type = TP_BUFFER;
			ret = slbc_status(&slb);
			if (ret > 0) {
				dev_info(aov_dev->dev, "%s: force release slb(%d)\n", __func__,
					ret);

				slb.uid = UID_AOV_DC;
				slb.type = TP_BUFFER;
				ret = slbc_release(&slb);
				if (ret < 0)
					dev_info(aov_dev->dev, "%s: failed to release slb buffer\n",
						__func__);
			}
		}
	} else if (event == SCP_EVENT_READY) {
		session = atomic_fetch_add(1, &(core_info->scp_session)) + 1;

		dev_info(aov_dev->dev, "%s: receive scp start event(%lu), session(%d)\n",
			__func__, event, session);

		ret = send_cmd_internal(core_info, AOV_SCP_CMD_READY, 0, 0, false, false);
		if (ret < 0) {
			dev_info(aov_dev->dev,
				"%s: failed to init scp session(%d): %d\n",
				__func__, session, ret);
				mutex_unlock(&core_info->start_stop_mutex);
			return NOTIFY_DONE;
		}

		/* Recover emiisu when scp bootup */
#if IS_ENABLED(CONFIG_MTK_LOAD_TRACKER_DEBUG)
#if !IS_ENABLED(CONFIG_MTK_EMI_LEGACY)
		mtk_emiisu_record_on();
#endif
#endif

		// Recovery the interruped session
		if (atomic_read(&(core_info->aov_ready))) {
			(void)aov_core_recover(aov_dev);
		}

		// Recovery the interruped session - qea
		if (atomic_read(&(core_info->qea_ready))) {
			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "AOV QEA restart\n");
			ret = aov_core_send_cmd(aov_dev, AOV_SCP_CMD_QEA, NULL, 0, false);
			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "AOV QEA restart done, ret(%d)\n", ret);
		}

		atomic_set(&(core_info->scp_ready), 2);
		aov_ulposc_cali(aov_dev);
		mutex_unlock(&core_info->start_stop_mutex);
	}

	return NOTIFY_DONE;
}

static struct notifier_block scp_state_notifier = {
	.notifier_call = scp_state_notify
};

int aov_core_init(struct mtk_aov *aov_dev)
{
	struct aov_core *core_info = &aov_dev->core_info;
	int index;
	int ret;
	struct dma_heap *dma_heap;

	curr_dev = aov_dev;

	init_waitqueue_head(&(core_info->scp_queue));
	atomic_set(&(core_info->scp_session), 0);
	atomic_set(&(core_info->scp_ready), 0);
	atomic_set(&(core_info->aov_session), 0);
	atomic_set(&(core_info->aov_ready), 0);
	atomic_set(&(core_info->cmd_seq), 0);
	atomic_set(&(core_info->qea_ready), 0);
	mutex_init(&core_info->sned_ipi_mutex);
	mutex_init(&core_info->start_stop_mutex);
	mutex_lock(&core_info->start_stop_mutex);

	if (curr_dev->op_mode == 0) {
		dev_info(aov_dev->dev, "%s: bypass init operation", __func__);
		return 0;
	}

	for (index = 0; index < AOV_SCP_CMD_MAX; index++)
		init_waitqueue_head(&core_info->ack_wq[index]);

	ret = mtk_ipi_register(&scp_ipidev, IPI_IN_SCP_AOV,
		ipi_receive, NULL, &(core_info->packet));
	if (ret < 0) {
		dev_info(aov_dev->dev, "%s: failed to register ipi callback: %d\n",
			__func__, ret);
		return ret;
	}

	/* this call back can get scp power down status */
	scp_A_register_notify(&scp_state_notifier);

	core_info->buf_pa = scp_get_reserve_mem_phys(SCP_AOV_MEM_ID);
	core_info->buf_va = (void *)scp_get_reserve_mem_virt(SCP_AOV_MEM_ID);
	core_info->buf_size = scp_get_reserve_mem_size(SCP_AOV_MEM_ID);

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
		"%s: scp buffer pa(%p), va(%p), size(0x%lx)\n", __func__,
		(void *)core_info->buf_pa, core_info->buf_va, core_info->buf_size);

	tlsf_init(&(core_info->alloc), core_info->buf_va, core_info->buf_size);

	spin_lock_init(&core_info->buf_lock);

	core_info->aov_start = NULL;

	// core_info->event_data = devm_kzalloc(
	//  aov_dev->dev, sizeof(struct buffer) * 3, GFP_KERNEL);
	// if (core_info->event_data == NULL) {
	//  dev_info(aov_dev->dev, "%s: failed to alloc event buffer\n", __func__);
	//  return -ENOMEM;
	// }

	dma_heap = dma_heap_find("mtk_mm");
	if (!dma_heap) {
		dev_info(aov_dev->dev, "%s: failed to find dma heap\n", __func__);
		return -ENOMEM;
	}

	core_info->dma_buf = dma_heap_buffer_alloc(
		dma_heap, sizeof(struct buffer) * 5, O_RDWR | O_CLOEXEC,
		DMA_HEAP_VALID_HEAP_FLAGS);
	if (IS_ERR(core_info->dma_buf)) {
		core_info->dma_buf = NULL;
		dev_info(aov_dev->dev, "%s: failed to alloc dma buffer\n", __func__);
		return -ENOMEM;
	}

	mtk_dma_buf_set_name(core_info->dma_buf, "AOV Event");

#ifdef NEW_DMA_BUF_API
	ret = dma_buf_vmap_unlocked(core_info->dma_buf, &(core_info->dma_map));
#else
	ret = dma_buf_vmap(core_info->dma_buf, &(core_info->dma_map));
#endif
	if (ret) {
		dev_info(aov_dev->dev, "%s: failed to map dmap buffer(%d)\n", __func__, ret);
		return -ENOMEM;
	}

	core_info->event_data = (void *)core_info->dma_map.vaddr;

	INIT_LIST_HEAD(&core_info->event_list);
	for (index = 0; index < 5; index++)
		list_add_tail(&core_info->event_data[index].entry, &core_info->event_list);

	spin_lock_init(&core_info->event_lock);

	init_waitqueue_head(&core_info->poll_wq);

	atomic_set(&(core_info->debug_mode), 0);
	atomic_set(&(core_info->disp_mode), AOV_DiSP_MODE_ON);
	atomic_set(&(core_info->aie_avail), 1);

	// Init event to receive event
	queue_init(&(core_info->event));
	// Init queue to receive event
	queue_init(&(core_info->queue));

	// create smi dump thread
	atomic_set(&(core_info->do_smi_dump), 0);
	init_waitqueue_head(&core_info->smi_dump_wq);
	core_info->smi_dump_thread = kthread_create(aov_smi_kernel_dump, NULL,
		"aov_smi_dump_thread");
	if (IS_ERR(core_info->smi_dump_thread)) {
		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
			"%s kthread_create error ret:%ld\n", __func__,
			PTR_ERR(core_info->smi_dump_thread));
	}
	wake_up_process(core_info->smi_dump_thread);

	// create reset sensor thread
	atomic_set(&(core_info->do_reset_sensor), 0);
	init_waitqueue_head(&core_info->reset_sensor_wq);
	core_info->reset_sensor_thread = kthread_create(reset_sensor_flow, NULL,
		"aov_reset_sensor_thread");
	if (IS_ERR(core_info->reset_sensor_thread)) {
		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
			"%s kthread_create error ret:%ld\n", __func__,
			PTR_ERR(core_info->reset_sensor_thread));
	}
	wake_up_process(core_info->reset_sensor_thread);

	return 0;
}

int aov_core_copy(struct mtk_aov *aov_dev, struct aov_dqevent *dequeue)
{
	struct aov_core *core_info = &aov_dev->core_info;
	struct base_event *event;
	void *buffer;
	uint32_t debug_mode;
	uint32_t power_mode;
	int ret = 0;

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "%s: copy aov event+\n", __func__);

	if (aov_dev->op_mode == 0) {
		dev_info(aov_dev->dev, "%s: bypass copy operation", __func__);
		return 0;
	}

	if (atomic_read(&(core_info->aov_ready)) == 0) {
		dev_info(aov_dev->dev, "%s: aov is not started\n", __func__);
		return -EIO;
	}

	event = queue_pop(&(core_info->queue));
	if (event == NULL) {
		dev_info(aov_dev->dev, "%s: event is null\n", __func__);
		return -EIO;
	}

	put_user(event->session,
		(uint32_t *)((uintptr_t)dequeue + offsetof(struct aov_dqevent, session)));
	put_user(event->frame_id,
		(uint32_t *)((uintptr_t)dequeue + offsetof(struct aov_dqevent, frame_id)));
	put_user(event->frame_width,
		(uint32_t *)((uintptr_t)dequeue + offsetof(struct aov_dqevent, frame_width)));
	put_user(event->frame_height,
		(uint32_t *)((uintptr_t)dequeue + offsetof(struct aov_dqevent, frame_height)));
	put_user(event->frame_mode,
		(uint32_t *)((uintptr_t)dequeue + offsetof(struct aov_dqevent, frame_mode)));
	put_user(event->detect_mode,
		(uint32_t *)((uintptr_t)dequeue + offsetof(struct aov_dqevent, detect_mode)));

	debug_mode = atomic_read(&(core_info->debug_mode));
	if ((event->detect_mode) || (debug_mode == AOV_DEBUG_MODE_DUMP) ||
		(debug_mode == AOV_DEBUG_MODE_NDD)) {
		// Setup aie output size
		put_user(event->aie_size, (uint32_t *)((uintptr_t)dequeue +
			offsetof(struct aov_dqevent, aie_size)));

		if (event->aie_size) {
			if (event->aie_size > AOV_MAX_AIE_OUTPUT) {
				buffer_release(core_info, event);
				dev_info(aov_dev->dev, "%s: invalid aie output overflow(%d/%d)\n",
					__func__, event->aie_size, AOV_MAX_AIE_OUTPUT);
				return -ENOMEM;
			}

			// Copy aie buffer output
			get_user(buffer, (void **)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, aie_output)));

			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
				"%s: copy aie output from(%p) to(%p) size(%d)\n",
				__func__, ALIGN16(&(event->aie_output[0])),
				buffer, event->aie_size);

			ret = copy_to_user((void *)buffer,
				ALIGN16(&(event->aie_output[0])), event->aie_size);
			if (ret) {
				buffer_release(core_info, event);
				dev_info(aov_dev->dev,
					"%s: failed to copy aie output(%d)\n", __func__, ret);
				return -EFAULT;
			}
		}

		// Setup fld output size
		put_user(event->fld_size, (uint32_t *)((uintptr_t)dequeue +
			offsetof(struct aov_dqevent, fld_size)));

		if (event->fld_size) {
			if (event->fld_size > AOV_MAX_FLD_OUTPUT) {
				buffer_release(core_info, event);
				dev_info(aov_dev->dev, "%s: invalid fld output overflow(%d/%d)\n",
					__func__, event->fld_size, AOV_MAX_FLD_OUTPUT);
				return -ENOMEM;
			}

			// Copy fld buffer output
			get_user(buffer, (void **)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, fld_output)));

			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
				"%s: copy fld output from(%p) to(%p) size(%d)\n",
				__func__, ALIGN16(&(event->fld_output[0])),
				buffer, event->fld_size);

			ret = copy_to_user((void *)buffer,
				ALIGN16(&(event->fld_output[0])), event->fld_size);
			if (ret) {
				buffer_release(core_info, event);
				dev_info(aov_dev->dev,
					"%s: failed to copy fld output(%d)\n", __func__, ret);
				return -EFAULT;
			}
		}

		// Setup apu output size
		put_user(event->apu_size, (uint32_t *)((uintptr_t)dequeue +
			offsetof(struct aov_dqevent, apu_size)));

		if (event->apu_size) {
			if (event->apu_size > AOV_MAX_APU_OUTPUT) {
				buffer_release(core_info, event);
				dev_info(aov_dev->dev, "%s: invalid apu output overflow(%d/%d)\n",
					__func__, event->apu_size, AOV_MAX_APU_OUTPUT);
				return -ENOMEM;
			}

			// Copy apu buffer output
			get_user(buffer, (void **)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, apu_output)));

			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
				"%s: copy apu output from(%p) to(%p) size(%d)\n",
				__func__, ALIGN16(&(event->apu_output[0])),
				buffer, event->apu_size);

			ret = copy_to_user((void *)buffer,
				ALIGN16(&(event->apu_output[0])), event->apu_size);
			if (ret) {
				buffer_release(core_info, event);
				dev_info(aov_dev->dev,
					"%s: failed to copy apu output(%d)", __func__, ret);
				return -EFAULT;
			}
		}

		power_mode = atomic_read(&(core_info->power_mode));
		if ((debug_mode == AOV_DEBUG_MODE_DUMP) ||
			(debug_mode == AOV_DEBUG_MODE_NDD) || (!power_mode)) {
			// Setup yuvo1 stride
			put_user(event->yuvo1_width, (uint32_t *)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, yuvo1_width)));

			put_user(event->yuvo1_height, (uint32_t *)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, yuvo1_height)));

			put_user(event->yuvo1_format, (uint32_t *)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, yuvo1_format)));

			put_user(event->yuvo1_stride, (uint32_t *)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, yuvo1_stride)));

			// Copy yuvo1 buffer output
			get_user(buffer, (void **)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, yuvo1_output)));

			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
				"%s: copy yuvo1 output from(%p) to(%p) size(%d)\n",
				__func__, ALIGN16(&(event->yuvo1_output[0])),
				buffer, AOV_MAX_YUVO1_OUTPUT);

			ret = copy_to_user((void *)buffer,
				ALIGN16(&(event->yuvo1_output[0])), AOV_MAX_YUVO1_OUTPUT);
			if (ret) {
				buffer_release(core_info, event);
				dev_info(aov_dev->dev,
					"%s: failed to copy yuvo1 output(%d)\n", __func__, ret);
				return -EFAULT;
			}

			// Setup yuvo2 stride
			put_user(event->yuvo2_width, (uint32_t *)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, yuvo2_width)));

			put_user(event->yuvo2_height, (uint32_t *)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, yuvo2_height)));

			put_user(event->yuvo2_format, (uint32_t *)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, yuvo2_format)));

			put_user(event->yuvo2_stride, (uint32_t *)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, yuvo2_stride)));

			// Copy yuvo2 buffer output
			get_user(buffer, (void **)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, yuvo2_output)));

			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
				"%s: copy yuvo1 output from(%p) to(%p) size(%d)\n",
				__func__, ALIGN16(&(event->yuvo2_output[0])),
				buffer, AOV_MAX_YUVO2_OUTPUT);

			ret = copy_to_user((void *)buffer,
				ALIGN16(&(event->yuvo2_output[0])), AOV_MAX_YUVO2_OUTPUT);
			if (ret) {
				buffer_release(core_info, event);
				dev_info(aov_dev->dev,
					"%s: failed to copy yuvo2 output(%d)\n", __func__, ret);
				return -EFAULT;
			}
		}
	}

	if (debug_mode == AOV_DEBUG_MODE_NDD) {
		struct ndd_event *ndd_data = (struct ndd_event *)event;

		// Setup imgo stride
		put_user(ndd_data->imgo_width, (uint32_t *)((uintptr_t)dequeue +
			offsetof(struct aov_dqevent, imgo_width)));

		put_user(ndd_data->imgo_height, (uint32_t *)((uintptr_t)dequeue +
			offsetof(struct aov_dqevent, imgo_height)));

		put_user(ndd_data->imgo_format, (uint32_t *)((uintptr_t)dequeue +
			offsetof(struct aov_dqevent, imgo_format)));

		put_user(ndd_data->imgo_order, (uint32_t *)((uintptr_t)dequeue +
			offsetof(struct aov_dqevent, imgo_order)));

		put_user(ndd_data->imgo_stride, (uint32_t *)((uintptr_t)dequeue +
			offsetof(struct aov_dqevent, imgo_stride)));

		// Copy imgo buffer output
		get_user(buffer, (void **)((uintptr_t)dequeue +
			offsetof(struct aov_dqevent, imgo_output)));

		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
			"%s: copy imgo output from(%p) to(%p) size(%d)\n",
			__func__, ALIGN16(&(ndd_data->imgo_output[0])),
			buffer, AOV_MAX_IMGO_OUTPUT);

		ret = copy_to_user((void *)buffer,
			ALIGN16(&(ndd_data->imgo_output[0])), AOV_MAX_IMGO_OUTPUT);
		if (ret) {
			buffer_release(core_info, ndd_data);
			dev_info(aov_dev->dev,
				"%s: failed to copy imgo output(%d)\n", __func__, ret);
			return -EFAULT;
		}

		// Setup aao output size
		put_user(ndd_data->aao_size,
			(uint32_t *)((uintptr_t)dequeue + offsetof(struct aov_dqevent, aao_size)));

		if (ndd_data->aao_size) {
			if (ndd_data->aao_size > AOV_MAX_AAO_OUTPUT) {
				buffer_release(core_info, ndd_data);
				dev_info(aov_dev->dev, "%s: invalid aao output overflow(%d/%d)\n",
					__func__, ndd_data->aao_size, AOV_MAX_AAO_OUTPUT);
				return -ENOMEM;
			}

			// Copy apu buffer output
			get_user(buffer, (void **)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, aao_output)));

			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
				"%s: copy aao output from(%p) to(%p) size(%d)\n",
				__func__, ALIGN16(&(ndd_data->aao_output[0])),
				buffer, ndd_data->aao_size);

			ret = copy_to_user((void *)buffer,
				ALIGN16(&(ndd_data->aao_output[0])), ndd_data->aao_size);
			if (ret) {
				buffer_release(core_info, ndd_data);
				dev_info(aov_dev->dev,
					"%s: failed to copy aao output(%d)\n", __func__, ret);
				return -EFAULT;
			}
		}

		// Setup aaho output size
		put_user(ndd_data->aaho_size, (uint32_t *)((uintptr_t)dequeue +
			offsetof(struct aov_dqevent, aaho_size)));

		if (ndd_data->aaho_size) {
			if (ndd_data->aaho_size > AOV_MAX_AAHO_OUTPUT) {
				buffer_release(core_info, ndd_data);
				dev_info(aov_dev->dev, "%s: invalid aaho output overflow(%d/%d)\n",
					__func__, ndd_data->aaho_size, AOV_MAX_AAHO_OUTPUT);
				return -ENOMEM;
			}

			// Copy apu buffer output
			get_user(buffer, (void **)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, aaho_output)));

			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
				"%s: copy aaho output from(%p) to(%p) size(%d)\n",
				__func__, ALIGN16(&(ndd_data->aaho_output[0])),
				buffer, ndd_data->aaho_size);

			ret = copy_to_user((void *)buffer,
				ALIGN16(&(ndd_data->aaho_output[0])), ndd_data->aaho_size);
			if (ret) {
				buffer_release(core_info, ndd_data);
				dev_info(aov_dev->dev,
					"%s: failed to copy aaho output(%d)\n", __func__, ret);
				return -EFAULT;
			}
		}

		// Setup meta output size
		put_user(ndd_data->meta_size,
			(uint32_t *)((uintptr_t)dequeue + offsetof(struct aov_dqevent, meta_size)));

		if (ndd_data->meta_size) {
			if (ndd_data->meta_size > AOV_MAX_META_OUTPUT) {
				buffer_release(core_info, ndd_data);
				dev_info(aov_dev->dev, "%s: invalid meta output overflow(%d/%d)\n",
					__func__, ndd_data->meta_size, AOV_MAX_META_OUTPUT);
				return -ENOMEM;
			}

			// Copy apu buffer output
			get_user(buffer, (void **)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, meta_output)));

			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
				"%s: copy meta output from(%p) to(%p) size(%d)\n",
				__func__, ALIGN16(&(ndd_data->meta_output[0])),
				buffer, ndd_data->meta_size);

			ret = copy_to_user((void *)buffer,
				ALIGN16(&(ndd_data->meta_output[0])), ndd_data->meta_size);
			if (ret) {
				buffer_release(core_info, ndd_data);
				dev_info(aov_dev->dev,
					"%s: failed to copy meta output(%d)\n", __func__, ret);
				return -EFAULT;
			}
		}

		// Setup awb output size
		put_user(ndd_data->awb_size,
			(uint32_t *)((uintptr_t)dequeue + offsetof(struct aov_dqevent, awb_size)));

		if (ndd_data->awb_size) {
			if (ndd_data->awb_size > AOV_MAX_AWB_OUTPUT) {
				buffer_release(core_info, ndd_data);
				dev_info(aov_dev->dev, "%s: invalid awb output overflow(%d/%d)\n",
					__func__, ndd_data->awb_size, AOV_MAX_AWB_OUTPUT);
				return -ENOMEM;
			}

			// Copy apu buffer output
			get_user(buffer, (void **)((uintptr_t)dequeue +
				offsetof(struct aov_dqevent, awb_output)));

			AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
				"%s: copy tuning output from(%p) to(%p) size(%d)\n",
				__func__, ALIGN16(&(ndd_data->awb_output[0])),
				buffer, ndd_data->awb_size);

			ret = copy_to_user((void *)buffer,
				ALIGN16(&(ndd_data->awb_output[0])), ndd_data->awb_size);
			if (ret) {
				buffer_release(core_info, ndd_data);
				dev_info(aov_dev->dev,
					"%s: failed to copy awb output(%d)\n", __func__, ret);
				return -EFAULT;
			}
		}
	}

	buffer_release(core_info, event);

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "%s: copy aov event-\n", __func__);

	return ret;
}

int aov_core_poll(struct mtk_aov *aov_dev, struct file *file,
	poll_table *wait)
{
	struct aov_core *core_info = &aov_dev->core_info;
	struct base_event *event;
	struct aov_notify info;
	int ret;

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "%s: poll start+\n", __func__);

	if (aov_dev->op_mode == 0) {
		dev_info(aov_dev->dev, "%s: bypass poll operation", __func__);
		return 0;
	}

	// only copy latest event
	event = queue_pop(&(core_info->event));
	while (!queue_empty(&(core_info->event))) {
		if (event != NULL) {
			dev_info(aov_dev->dev, "%s: release old aov event id(%d)\n", __func__, event->event_id);
			info.notify = AOV_NOTIFY_EVT_AVAIL;
			info.status = event->event_id;
			(void)aov_core_send_cmd(aov_dev, AOV_SCP_CMD_NOTIFY,
				(void *)&info, sizeof(struct aov_notify), true);
		}
		event = queue_pop(&(core_info->event));
	}
	if (event != NULL) {
		ret = copy_event_data(aov_dev, event);
		if (ret >= 0)
			return POLLPRI;
	}

	poll_wait(file, &core_info->poll_wq, wait);

	// only copy latest event
	event = queue_pop(&(core_info->event));
	while (!queue_empty(&(core_info->event))) {
		if (event != NULL) {
			dev_info(aov_dev->dev, "%s: release old aov event id(%d)\n", __func__, event->event_id);
			info.notify = AOV_NOTIFY_EVT_AVAIL;
			info.status = event->event_id;
			(void)aov_core_send_cmd(aov_dev, AOV_SCP_CMD_NOTIFY,
				(void *)&info, sizeof(struct aov_notify), true);
		}
		event = queue_pop(&(core_info->event));
	}
	if (event != NULL) {
		ret = copy_event_data(aov_dev, event);
		if (ret >= 0)
			return POLLPRI;
	}

	AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag), "%s: poll start-: 0\n", __func__);

	return 0;
}

int aov_core_reset(struct mtk_aov *aov_dev)
{
	struct aov_core *core_info = &aov_dev->core_info;
	unsigned long flag;
	void *buf;
	int ret = 0;

	if (aov_dev->op_mode == 0) {
		dev_info(aov_dev->dev, "%s: bypass reset operation", __func__);
		return 0;
	}

	if (atomic_read(&(core_info->aov_ready))) {
		mutex_lock(&core_info->start_stop_mutex);
#if AOV_SLB_ALLOC_FREE
		struct slbc_data slb;
#endif  // AOV_SLB_ALLOC_FREE

		dev_info(aov_dev->dev, "%s: force aov deinit+\n", __func__);

		ret = send_cmd_internal(core_info, AOV_SCP_CMD_STOP, 0, 0, true, true);
		if (ret != 0) {
			dev_info(aov_dev->dev, "%s: send_cmd_internal ret(%d) != 0.\n", __func__, ret);
			// sleep 100 ms to wait scp stop done.
			msleep(100);
		}

		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
			"mtk_cam_seninf_aov_runtime_resume(%d/%d)+\n",
			core_info->sensor_id, DEINIT_ABNORMAL_USR_FD_KILL);
		mtk_cam_seninf_aov_runtime_resume(core_info->sensor_id,
			DEINIT_ABNORMAL_USR_FD_KILL);
		AOV_DEBUG_LOG(*(aov_dev->enable_aov_log_flag),
			"mtk_cam_seninf_aov_runtime_resume(%d/%d)-\n",
			core_info->sensor_id, DEINIT_ABNORMAL_USR_FD_KILL);

#if AOV_SLB_ALLOC_FREE
		slb.uid = UID_AOV_DC;
		slb.type = TP_BUFFER;
		ret = slbc_status(&slb);
		if (ret > 0) {
			dev_info(aov_dev->dev, "%s: force release slb(%d)\n", __func__, ret);

			slb.uid = UID_AOV_DC;
			slb.type = TP_BUFFER;
			AOV_TRACE_BEGIN("AOV Force Release SLB");
			ret = slbc_release(&slb);
			AOV_TRACE_END();
			if (ret < 0)
				dev_info(aov_dev->dev, "%s: failed to release slb buffer\n",
					__func__);
		}
#endif  // AOV_SLB_ALLOC_FREE

		// Free aov_start buffer
		if (core_info->aov_start) {
			dev_info(aov_dev->dev, "%s: aov force free init+", __func__);
			spin_lock_irqsave(&core_info->buf_lock, flag);
			tlsf_free(&(core_info->alloc), core_info->aov_start);
			spin_unlock_irqrestore(&core_info->buf_lock, flag);
			dev_info(aov_dev->dev, "%s: aov force free init-", __func__);
		}

		// Reset event to empty
		while (!queue_empty(&(core_info->event)))
			(void)queue_pop(&(core_info->event));
		queue_deinit(&(core_info->event));
		queue_init(&(core_info->event));

		// Reset queue to empty
		while (!queue_empty(&(core_info->queue))) {
			buf = queue_pop(&(core_info->queue));
			if (buf)
				buffer_release(core_info, buf);
		}
		queue_deinit(&(core_info->queue));
		queue_init(&(core_info->queue));

		dev_info(aov_dev->dev, "%s: force aov deinit-: (%d)", __func__, ret);

		atomic_set(&(core_info->aov_ready), 0);

		ret = 1;
		mutex_unlock(&core_info->start_stop_mutex);
	}

	return ret;
}

int aov_core_uninit(struct mtk_aov *aov_dev)
{
	struct aov_core *core_info = &aov_dev->core_info;

	//devm_kfree(aov_dev->dev, core_info->event_data);
	mutex_destroy(&core_info->start_stop_mutex);
	mutex_destroy(&core_info->sned_ipi_mutex);

	if (aov_dev->op_mode == 0) {
		dev_info(aov_dev->dev, "%s: bypass uninit operation", __func__);
		return 0;
	}

	if (core_info->dma_buf) {
		if (core_info->event_data)
#ifdef NEW_DMA_BUF_API
			dma_buf_vunmap_unlocked(core_info->dma_buf, &(core_info->dma_map));
#else
			dma_buf_vunmap(core_info->dma_buf, &(core_info->dma_map));
#endif

		dma_heap_buffer_free(core_info->dma_buf);
	}

	// stop aie smi dump thread
	if (core_info->smi_dump_thread) {
		kthread_stop(core_info->smi_dump_thread);
		core_info->smi_dump_thread = NULL;
	}

	// stop reset sensor thread
	if (core_info->reset_sensor_thread) {
		kthread_stop(core_info->reset_sensor_thread);
		core_info->reset_sensor_thread = NULL;
	}

	// deinit event
	queue_deinit(&(core_info->event));
	// deinit queue
	queue_deinit(&(core_info->queue));

	curr_dev = NULL;

	return 0;
}

int aov_smi_kernel_dump(void *arg)
{
	struct mtk_aov *aov_dev = aov_core_get_device();
	struct aov_core *core_info = &aov_dev->core_info;
	int ret = 0;
	long wait_ret = 0;

	dev_info(aov_dev->dev, "%s: Enter while loop to wait event", __func__);
	while (!kthread_should_stop()) {
		wait_ret = wait_event_interruptible(core_info->smi_dump_wq,
			atomic_cmpxchg(&(core_info->do_smi_dump), 1, 0));
		if (wait_ret) {
			dev_info(aov_dev->dev, "%s: wake up by signal(%ld)", __func__, wait_ret);
			continue;
		}

		dev_info(aov_dev->dev, "%s: do aov smi kernel dump+ id(%u)", __func__, core_info->smi_dump_id);
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wimplicit-function-declaration"
		mtk_smi_dbg_hang_detect("AOV_SMI_DUMP");
		#pragma clang diagnostic pop
		dev_info(aov_dev->dev, "%s: do aov smi kernel dump- id(%u)", __func__, core_info->smi_dump_id);
		core_info->smi_dump_id = 0;

		/* Stop emiisu to save current emi debug info */
#if IS_ENABLED(CONFIG_MTK_LOAD_TRACKER_DEBUG)
#if !IS_ENABLED(CONFIG_MTK_EMI_LEGACY)
		mtk_emiisu_record_off();
#endif
#endif

		ret = send_cmd_internal(core_info, AOV_SCP_CMD_SMI_DUMP_DONE, 0, 0, false, false);
		if (ret < 0)
			dev_info(aov_dev->dev, "%s: failed to do aov smi dump done: %d\n",
				__func__, ret);
	}
	dev_info(aov_dev->dev, "%s: leave while loop for kthread stop", __func__);
	return 0;
}

int reset_sensor_flow(void *arg)
{
	struct mtk_aov *aov_dev = aov_core_get_device();
	struct aov_core *core_info = &aov_dev->core_info;
	int ret = 0;
	long wait_ret = 0;

	dev_info(aov_dev->dev, "%s: Enter while loop to wait event", __func__);
	while (!kthread_should_stop()) {
		wait_ret = wait_event_interruptible(core_info->reset_sensor_wq,
			atomic_cmpxchg(&(core_info->do_reset_sensor), 1, 0));
		if (wait_ret) {
			dev_info(aov_dev->dev, "%s: wake up by signal(%ld)", __func__, wait_ret);
			continue;
		}

		dev_info(aov_dev->dev, "%s: do reset sensor flow+", __func__);
		ret = mtk_cam_seninf_aov_reset_sensor(core_info->sensor_id);
		if (ret < 0)
			dev_info(aov_dev->dev,
				"mtk_cam_seninf_aov_reset_sensor(%d) fail, ret: %d\n",
				core_info->sensor_id, ret);
		dev_info(aov_dev->dev, "%s: do reset sensor flow-", __func__);

		ret = send_cmd_internal(core_info, AOV_SCP_CMD_RESET_SENSOR_END, 0, 0, false, false);
		if (ret < 0)
			dev_info(aov_dev->dev, "%s: failed to do aov reset sensor end: %d\n",
				__func__, ret);
	}
	dev_info(aov_dev->dev, "%s: leave while loop for kthread stop", __func__);
	return 0;
}

MODULE_IMPORT_NS(DMA_BUF);
