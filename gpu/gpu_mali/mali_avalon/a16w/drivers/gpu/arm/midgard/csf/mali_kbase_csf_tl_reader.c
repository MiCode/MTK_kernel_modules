// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2019-2024 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#include "mali_kbase_csf_tl_reader.h"

#include "mali_kbase_csf_trace_buffer.h"
#include "mali_kbase_reset_gpu.h"

#include "tl/mali_kbase_tlstream.h"
#include "tl/mali_kbase_tl_serialize.h"
#include "tl/mali_kbase_tracepoints.h"

#include "mali_kbase_pm.h"
#include "mali_kbase_hwaccess_time.h"

#include <linux/math64.h>

#if IS_ENABLED(CONFIG_DEBUG_FS)
#include "tl/mali_kbase_timeline_priv.h"
#include <linux/debugfs.h>
#include <linux/version_compat_defs.h>
#endif

/* Name of the timeline header metatadata */
#define KBASE_CSFFW_TIMELINE_HEADER_NAME "timeline_header"

/**
 * struct kbase_csffw_tl_message - CSFFW timeline message.
 *
 * @msg_id: Message ID.
 * @timestamp: Timestamp of the event.
 * @cycle_counter: Cycle number of the event.
 *
 * Contain fields that are common for all CSFFW timeline messages.
 */
struct kbase_csffw_tl_message {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
} __packed __aligned(4);

#if IS_ENABLED(CONFIG_MALI_MTK_TIMELINE_TRACE_DEBUG)
enum kbase_csffw_tl_msg_id {
	CSFFW_TL_ENUM,
	CSFFW_TL_EVENT_ITER_INITIAL_CONNECTIONS,
	CSFFW_TL_EVENT_FW_STATE_CHANGE,
	CSFFW_TL_EVENT_CS_USER_DB,
	CSFFW_TL_EVENT_CS_STATE_CHANGE,
	CSFFW_TL_EVENT_CSG_STATE_CHANGE,
	CSFFW_TL_EVENT_CSHWIF_STATE_CHANGE,
	CSFFW_TL_EVENT_ITER_STATE_CHANGE,
	CSFFW_TL_EVENT_CS_ACQUIRE_ITER,
	CSFFW_TL_EVENT_CS_RELEASE_ITER,
	CSFFW_TL_EVENT_BOOT_START,
	CSFFW_TL_EVENT_GEOMETRY_DESIRED_READY_CHANGE,
	CSFFW_TL_EVENT_SHADER_DESIRED_READY_CHANGE,
	CSFFW_TL_EVENT_NEURAL_DESIRED_READY_CHANGE,
	CSFFW_TRACEPOINT_COUNT,
};

enum kbase_csffw_tl_fw_state {
	CSFFW_TL_FW_NORMAL_MODE,    /**< FW has entered normal mode */
	CSFFW_TL_FW_PROTECTED_MODE, /**< FW has entered protected mode */
	CSFFW_TL_FW_SLEEPING,	    /**< FW has started sleeping */
};

static const char *csffw_tl_fw_state_strings[] = {
	"NORMAL_MODE",    /**< FW has entered normal mode */
	"PROTECTED_MODE", /**< FW has entered protected mode */
	"SLEEPING",	    /**< FW has started sleeping */
};

enum kbase_tl_csg_internal_state {
	CSFFW_TL_CSG_DISABLING, /**< Disabling as a result of suspension or termination */
	CSFFW_TL_CSG_ENABLING, /**< Enabling, with one or more CSIs enabled and/or iterators binded */
	CSFFW_TL_CSG_SUSPENDING, /**< Transition from ENABLED to SUSPEND. Can be escalated to TERMINATE */
	CSFFW_TL_CSG_TERMINATING, /**< Transition from ENABLED/SUSPENDING to TERMINATE */
	CSFFW_TL_CSG_PROTM_SUSPENDING, /**< Transition from ENABLED to SUSPEND by protected mode manager request */
	CSFFW_TL_CSG_ENABLED,  /**< End of handling ENABLED state */
	CSFFW_TL_CSG_DISABLED, /**< End of handling DISABLED state */
};

static const char *tl_csg_internal_state_strings[] = {
	"DISABLING", /**< Disabling as a result of suspension or termination */
	"ENABLING", /**< Enabling, with one or more CSIs enabled and/or iterators binded */
	"SUSPENDING", /**< Transition from ENABLED to SUSPEND. Can be escalated to TERMINATE */
	"TERMINATING", /**< Transition from ENABLED/SUSPENDING to TERMINATE */
	"PROTM_SUSPENDING", /**< Transition from ENABLED to SUSPEND by protected mode manager request */
	"ENABLED",  /**< End of handling ENABLED state */
	"DISABLED", /**< End of handling DISABLED state */
};

enum kbase_tl_iter_type {
	CSFFW_ITER_TILER,
	CSFFW_ITER_COMPUTE,
	CSFFW_ITER_FRAGMENT,
	CSFFW_ITER_NEURAL,
};

static const char *tl_iter_type_strings[] = {
	"Tiler",
	"Compute",
	"Fragment",
	"Neural",
};

enum kbase_csffw_tl_csi_state {
	CSFFW_CSI_STOPPED = 0,		 /**< Stopped by host request, must be 0 */
	CSFFW_CSI_STOPPING,		 /**< Stopping by host request */
	CSFFW_CSI_SUSPENDING,		 /**< Suspending by host request */
	CSFFW_CSI_TERMINATING,		 /**< Terminating by host request */
	CSFFW_CSI_BLOCKED_PROGRESS,	 /**< Blocked in PROGRESS_WAIT */
	CSFFW_CSI_BLOCKED_DEFERRED,	 /**< Blocked waiting for deferred slots */
	CSFFW_CSI_BLOCKED_RESOURCE,	 /**< Blocked in resource request */
	CSFFW_CSI_BLOCKED_PROTM_PEND,	 /**< Blocked in protected memory request */
	CSFFW_CSI_BLOCKED_SHARED_SB_DEC, /**< Blocked as result of stalled SHARED_SB_DEC */
	CSFFW_CSI_FAULT,		 /**< Fault has occurred */
	CSFFW_CSI_SUSPENDING_FAULT, /**< Fault has occurred during suspension, or suspend requested during fault. */
	CSFFW_CSI_RUNNABLE,	     /**< Runnable (subject to scheduler decisions) */
	CSFFW_CSI_BLOCKED_SYNC_WAIT, /**< Blocked in SYNC_WAIT */
	CSFFW_CSI_EMPTY,	     /**< Command buffer is empty */
};

static const char *tl_csi_state_strings[] = {
	"STOPPED", /**< Disabling as a result of suspension or termination */
	"STOPING", /**< Enabling, with one or more CSIs enabled and/or iterators binded */
	"SUSPENDING", /**< Transition from ENABLED to SUSPEND. Can be escalated to TERMINATE */
	"TERMINATING", /**< Transition from ENABLED/SUSPENDING to TERMINATE */
	"BLOCKED_PROGRESS",	 /**< Blocked in PROGRESS_WAIT */
	"BLOCKED_DEFERRED",	 /**< Blocked waiting for deferred slots */
	"BLOCKED_RESOURCE",	 /**< Blocked in resource request */
	"BLOCKED_PROTM_PEND",	 /**< Blocked in protected memory request */
	"BLOCKED_SHARED_SB_DEC", /**< Blocked as result of stalled SHARED_SB_DEC */
	"FAULT",		 /**< Fault has occurred */
	"SUSPENDING_FAULT", /**< Fault has occurred during suspension, or suspend requested during fault. */
	"RUNNABLE",	     /**< Runnable (subject to scheduler decisions) */
	"SYNC_WAIT", /**< Blocked in SYNC_WAIT */
	"EMPTY",	     /**< Command buffer is empty */
};

enum kbase_tl_cshwif_state {
	CSFFW_TL_CSHWIF_DISABLED,
	CSFFW_TL_CSHWIF_ENABLED,
	CSFFW_TL_CSHWIF_HALTED,
	CSFFW_TL_CSHWIF_DISABLING,
	CSFFW_TL_CSHWIF_PAUSING,
	CSFFW_TL_CSHWIF_PAUSED,
};

static const char *tl_cshwif_state_strings[] = {
	"DISABLED",
	"ENABLED",
	"HALTED",
	"DISABLING",
	"PAUSING",
	"PAUSED",
};

enum kbase_tl_iter_status {
	/** Iterator is disabled */
	CSFFW_ITER_DISABLED,
	/** Iterator is enabled */
	CSFFW_ITER_ENABLED,
	/** Iterator is being enabled during start (DISABLED->ENABLED) or
	 * resumption step 2 (PAUSED->ENABLED)
	 */
	CSFFW_ITER_ENABLING,
	/** Iterator is being paused during resumption step 1 (DISABLED->PAUSED) */
	CSFFW_ITER_PAUSING,
	/** Iterator is PAUSED. */
	CSFFW_ITER_PAUSED,
	/* TODO GPUFW-2002: Update usage. */
	/* Currently unused. */
	CSFFW_ITER_HALTING,
	/* TODO GPUFW-2002: Update usage, or remove. */
	/** Iterator is HALTED. Currently unused. */
	CSFFW_ITER_HALTED,
	/** Iterator is being disabled during suspension step 2
	 * (PAUSED->DISABLED) or termination (ENABLED->DISABLED) or
	 * preemption step 2 (PAUSED->DISABLED)
	 */
	CSFFW_ITER_DISABLING,
};

static const char *tl_iter_status_strings[] = {
	"DISABLED",
	"ENABLED",
	"ENABLING",
	"PAUSING",
	"PAUSED",
	"HALTING",
	"HALTED",
	"DISABLING",
};

struct kbase_csffw_tl_event_iter_initial_connections_msg {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
	u32 iter;
	u32 iterator_type;
	u32 csg;
	u32 csis;
} __pack4 __aligned(4);

struct kbase_csffw_fw_state_change_tl_message {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
	u32 fw_state;
} __packed __aligned(4);

struct kbase_csffw_fw_cs_user_db_msg {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
	u32 csg;
	u32 cs;
} __packed __aligned(4);

struct kbase_csffw_tl_event_cs_state_change_msg {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
	u32 csg;
	u32 cs;
	u32 cs_state;
} __packed __aligned(4);

struct kbase_csffw_tl_event_csg_state_change_msg {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
	u32 csg;
	u32 csg_state;
} __packed __aligned(4);

struct kbase_csffw_tl_event_cshwif_state_change_msg {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
	u32 cshwif;
	u32 tl_cshwif_state;
	u32 csg;
	u32 cs;
}  __packed __aligned(4);

struct kbase_csffw_tl_event_iter_state_change_msg {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
	u32 iter;
	u32 iterator_type;
	u32 iterator_state;
	u32 csg;
} __packed __aligned(4);

struct kbase_csffw_tl_event_cs_acquire_iter_msg {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
	u32 csg;
	u32 cs;
	u32 iter;
	u32 iterator_type;
} __packed __aligned(4);

struct kbase_csffw_tl_event_cs_release_iter_msg {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
	u32 csg;
	u32 cs;
	u32 iter;
	u32 iterator_type;
} __packed __aligned(4);

struct kbase_csffw_tl_event_shader_desired_ready_change_msg {
	u32 msg_id;
	u64 timestamp;
	u64 cycle_counter;
	u64 shader_desired_ready;
} __packed __aligned(4);

#endif /* CONFIG_MALI_MTK_TIMELINE_TRACE_DEBUG */

#if IS_ENABLED(CONFIG_DEBUG_FS)
static int kbase_csf_tl_debugfs_poll_interval_read(void *data, u64 *val)
{
	struct kbase_device *kbdev = (struct kbase_device *)data;
	struct kbase_csf_tl_reader *self = &kbdev->timeline->csf_tl_reader;

	*val = self->timer_interval;

	return 0;
}

static int kbase_csf_tl_debugfs_poll_interval_write(void *data, u64 val)
{
	struct kbase_device *kbdev = (struct kbase_device *)data;
	struct kbase_csf_tl_reader *self = &kbdev->timeline->csf_tl_reader;

	if (val > KBASE_CSF_TL_READ_INTERVAL_MAX || val < KBASE_CSF_TL_READ_INTERVAL_MIN)
		return -EINVAL;

	self->timer_interval = (u32)val;

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(kbase_csf_tl_poll_interval_fops, kbase_csf_tl_debugfs_poll_interval_read,
			 kbase_csf_tl_debugfs_poll_interval_write, "%llu\n");

void kbase_csf_tl_reader_debugfs_init(struct kbase_device *kbdev)
{
	debugfs_create_file("csf_tl_poll_interval_in_ms", 0644, kbdev->debugfs_instr_directory,
			    kbdev, &kbase_csf_tl_poll_interval_fops);
}
#endif

/**
 * tl_reader_overflow_notify() - Emit stream overflow tracepoint.
 *
 * @self:		CSFFW TL Reader instance.
 * @msg_buf_start:	Start of the message.
 * @msg_buf_end:	End of the message buffer.
 */
static void tl_reader_overflow_notify(const struct kbase_csf_tl_reader *self,
				      u8 *const msg_buf_start, u8 *const msg_buf_end)
{
	struct kbase_device *kbdev = self->kbdev;
	struct kbase_csffw_tl_message message = { 0 };

	/* Reuse the timestamp and cycle count from current event if possible */
	if (msg_buf_start + sizeof(message) <= msg_buf_end)
		memcpy(&message, msg_buf_start, sizeof(message));

	KBASE_TLSTREAM_TL_KBASE_CSFFW_TLSTREAM_OVERFLOW(kbdev, message.timestamp,
							message.cycle_counter);
}

/**
 * tl_reader_overflow_check() - Check if an overflow has happened
 *
 * @self:	CSFFW TL Reader instance.
 * @event_id:	Incoming event id.
 *
 * Return: True, if an overflow has happened, False otherwise.
 */
static bool tl_reader_overflow_check(struct kbase_csf_tl_reader *self, u16 event_id)
{
	struct kbase_device *kbdev = self->kbdev;
	bool has_overflow = false;

	/* 0 is a special event_id and reserved for the very first tracepoint
	 * after reset, we should skip overflow check when reset happened.
	 */
	if (event_id != 0) {
		has_overflow = self->got_first_event && self->expected_event_id != event_id;

		if (has_overflow)
			dev_warn(kbdev->dev, "CSFFW overflow, event_id: %u, expected: %u.",
				 event_id, self->expected_event_id);
	}

	self->got_first_event = true;
	self->expected_event_id = event_id + 1;
	/* When event_id reaches its max value, it skips 0 and wraps to 1. */
	if (self->expected_event_id == 0)
		self->expected_event_id++;

	return has_overflow;
}

/**
 * tl_reader_reset() - Reset timeline tracebuffer reader state machine.
 *
 * @self:	CSFFW TL Reader instance.
 *
 * Reset the reader to the default state, i.e. set all the
 * mutable fields to zero.
 *
 * NOTE: this function expects the irq spinlock to be held.
 */
static void tl_reader_reset(struct kbase_csf_tl_reader *self)
{
	lockdep_assert_held(&self->read_lock);

	self->got_first_event = false;
	self->is_active = false;
	self->expected_event_id = 0;
	self->tl_header.btc = 0;

	/* There might be data left in the trace buffer from the previous
	 * tracing session. We don't want it to leak into this session.
	 */
	kbase_csf_firmware_trace_buffer_discard_all(self->trace_buffer);
}

int kbase_csf_tl_reader_flush_buffer(struct kbase_csf_tl_reader *self)
{
	int ret = 0;
	struct kbase_device *kbdev = self->kbdev;
	struct kbase_tlstream *stream = self->stream;

	u8 *read_buffer = self->read_buffer;
	const size_t read_buffer_size = sizeof(self->read_buffer);

	u32 bytes_read;
	u8 *csffw_data_begin;
	u8 *csffw_data_end;
	u8 *csffw_data_it;

	unsigned long flags;

	spin_lock_irqsave(&self->read_lock, flags);

	/* If not running, early exit. */
	if (!self->is_active) {
		spin_unlock_irqrestore(&self->read_lock, flags);
		return -EBUSY;
	}

	/* Copying the whole buffer in a single shot. We assume
	 * that the buffer will not contain partially written messages.
	 */
	bytes_read = kbase_csf_firmware_trace_buffer_read_data(self->trace_buffer, read_buffer,
							       read_buffer_size);
	csffw_data_begin = read_buffer;
	csffw_data_end = read_buffer + bytes_read;

	for (csffw_data_it = csffw_data_begin; csffw_data_it < csffw_data_end;) {
		u32 event_header;
		u16 event_id;
		u16 event_size;
		unsigned long acq_flags;
		char *buffer;

		/* Can we safely read event_id? */
		if (csffw_data_it + sizeof(event_header) > csffw_data_end) {
			dev_warn(kbdev->dev, "Unable to parse CSFFW tracebuffer event header.");
			ret = -EBUSY;
			break;
		}

		/* Read and parse the event header. */
		memcpy(&event_header, csffw_data_it, sizeof(event_header));
		event_id = (event_header >> 0) & 0xFFFF;
		event_size = (event_header >> 16) & 0xFFFF;
		csffw_data_it += sizeof(event_header);

		/* Detect if an overflow has happened. */
		if (tl_reader_overflow_check(self, event_id))
			tl_reader_overflow_notify(self, csffw_data_it, csffw_data_end);

		/* Can we safely read the message body? */
		if (csffw_data_it + event_size > csffw_data_end) {
			dev_warn(kbdev->dev, "event_id: %u, can't read with event_size: %u.",
				 event_id, event_size);
			ret = -EBUSY;
			break;
		}

		/* Convert GPU timestamp to CPU timestamp. */
		{
			struct kbase_csffw_tl_message *msg =
				(struct kbase_csffw_tl_message *)csffw_data_it;
			msg->timestamp =
				kbase_backend_time_convert_gpu_to_cpu(kbdev, msg->timestamp);
#if IS_ENABLED(CONFIG_MALI_MTK_TIMELINE_TRACE_DEBUG)
			if (msg->msg_id == CSFFW_TL_EVENT_ITER_INITIAL_CONNECTIONS) {
				struct kbase_csffw_tl_event_iter_initial_connections_msg *msg2 =
					(struct kbase_csffw_tl_event_iter_initial_connections_msg *)csffw_data_it;
				//trace_tracing_mark_write_tl(7788,"iter_init_connection", msg2->iterator_type, msg2->timestamp);
				//kbase_tl_systrace("C|7788|init:csgs%d:iter%d:csis%d|%d|%lld", msg2->csg, msg2->iter, msg2->csis, msg2->iterator_type, msg2->timestamp);
			} else if (msg->msg_id == CSFFW_TL_EVENT_FW_STATE_CHANGE) {
				struct kbase_csffw_fw_state_change_tl_message *msg2 =
					(struct kbase_csffw_fw_state_change_tl_message *)csffw_data_it;
				kbase_tl_systrace("E|7788|CSFFW State-300|%lld", msg2->timestamp);
				kbase_tl_systrace("B|7788|CSFFW State-300|%s|%lld", csffw_tl_fw_state_strings[msg2->fw_state], msg2->timestamp);
			} else if (msg->msg_id == CSFFW_TL_EVENT_CS_USER_DB) {
				struct  kbase_csffw_fw_cs_user_db_msg *msg2 =
					(struct kbase_csffw_fw_cs_user_db_msg *)csffw_data_it;
				u32 pid= msg2->cs + 1;
				kbase_tl_systrace("B|7788|User DB CSG%d-CSI%dState-8%d%d|Ring|%lld", msg2->csg, msg2->cs, msg2->csg, pid, msg2->timestamp);
				kbase_tl_systrace("E|7788|User DB CSG%d-CSI%dState-8%d%d|%lld", msg2->csg, msg2->cs, msg2->csg, pid, msg2->timestamp);
			} else if (msg->msg_id == CSFFW_TL_EVENT_CS_STATE_CHANGE) {
				struct  kbase_csffw_tl_event_cs_state_change_msg *msg2 =
					(struct kbase_csffw_tl_event_cs_state_change_msg *)csffw_data_it;
				u32 pid= msg2->cs + 1;
				kbase_tl_systrace("E|7788|CSG%d-CSI%dState-4%d%d|%lld", msg2->csg, msg2->cs, msg2->csg, pid, msg2->timestamp);
				kbase_tl_systrace("B|7788|CSG%d-CSI%dState-4%d%d|%s|%lld", msg2->csg, msg2->cs, msg2->csg, pid, tl_csi_state_strings[msg2->cs_state], msg2->timestamp);
			} else if (msg->msg_id == CSFFW_TL_EVENT_CSG_STATE_CHANGE) {
				struct  kbase_csffw_tl_event_csg_state_change_msg *msg2 =
					(struct kbase_csffw_tl_event_csg_state_change_msg *)csffw_data_it;
				kbase_tl_systrace("E|7788|CSG%d State-4%d0|%lld", msg2->csg, msg2->csg, msg2->timestamp);
				kbase_tl_systrace("B|7788|CSG%d State-4%d0|%s|%lld", msg2->csg, msg2->csg, tl_csg_internal_state_strings[msg2->csg_state], msg2->timestamp);
			} else if (msg->msg_id == CSFFW_TL_EVENT_CSHWIF_STATE_CHANGE) {
				struct  kbase_csffw_tl_event_cshwif_state_change_msg *msg2 =
					(struct kbase_csffw_tl_event_cshwif_state_change_msg *)csffw_data_it;
				kbase_tl_systrace("E|7788|CSHWIF %d-50%d|%lld", msg2->cshwif, msg2->cshwif, msg2->timestamp);
				kbase_tl_systrace("B|7788|CSHWIF %d-50%d|CSI:%d CSG:%d-%s|%lld", msg2->cshwif, msg2->cshwif, msg2->cs,msg2->csg, tl_cshwif_state_strings[msg2->tl_cshwif_state], msg2->timestamp);
			} else if(msg->msg_id == CSFFW_TL_EVENT_ITER_STATE_CHANGE) {
				struct kbase_csffw_tl_event_iter_state_change_msg *msg2 =
					(struct kbase_csffw_tl_event_iter_state_change_msg *)csffw_data_it;
				kbase_tl_systrace("E|7788|%s iterator%d state-6%d0|%lld", tl_iter_type_strings[msg2->iterator_type], msg2->iter,msg2->iterator_type, msg2->timestamp);
				kbase_tl_systrace("B|7788|%s iterator%d state-6%d0|CSG:%d-%s|%lld", tl_iter_type_strings[msg2->iterator_type], msg2->iter, msg2->iterator_type,msg2->csg, tl_iter_status_strings[msg2->iterator_state], msg2->timestamp);
			} else if (msg->msg_id == CSFFW_TL_EVENT_CS_ACQUIRE_ITER) {
				struct kbase_csffw_tl_event_cs_acquire_iter_msg *msg2 =
					(struct kbase_csffw_tl_event_cs_acquire_iter_msg *)csffw_data_it;
				kbase_tl_systrace("B|7788|%s connections-6%d1|CSIs:[%d ]CSG:%d|%lld", tl_iter_type_strings[msg2->iterator_type], msg2->iterator_type, msg2->cs,msg2->csg, msg2->timestamp);
			} else if (msg->msg_id == CSFFW_TL_EVENT_CS_RELEASE_ITER) {
				struct kbase_csffw_tl_event_cs_release_iter_msg *msg2 =
					(struct kbase_csffw_tl_event_cs_release_iter_msg *)csffw_data_it;
				kbase_tl_systrace("E|7788|%s connection-6%d1|%lld", tl_iter_type_strings[msg2->iterator_type], msg2->iterator_type, msg2->timestamp);
			} else if (msg->msg_id == CSFFW_TL_EVENT_SHADER_DESIRED_READY_CHANGE) {
				struct kbase_csffw_tl_event_shader_desired_ready_change_msg *msg2 =
					(struct kbase_csffw_tl_event_shader_desired_ready_change_msg *)csffw_data_it;
				kbase_tl_systrace("E|7788|Shader Ready-900|%lld",  msg2->timestamp);
				kbase_tl_systrace("B|7788|Shader Ready-900|%lld|%lld",  msg2->shader_desired_ready, msg2->timestamp);
			}
#endif /* CONFIG_MALI_MTK_TIMELINE_TRACE_DEBUG */
		}

		/* Copy the message out to the tl_stream. */
		buffer = kbase_tlstream_msgbuf_acquire(stream, event_size, &acq_flags);
		kbasep_serialize_bytes(buffer, 0, csffw_data_it, event_size);
		kbase_tlstream_msgbuf_release(stream, acq_flags);
		csffw_data_it += event_size;
	}

	spin_unlock_irqrestore(&self->read_lock, flags);
	return ret;
}

static void kbasep_csf_tl_reader_read_callback(struct timer_list *timer)
{
	struct kbase_csf_tl_reader *self =
		container_of(timer, struct kbase_csf_tl_reader, read_timer);

	int rcode;

	kbase_csf_tl_reader_flush_buffer(self);

	rcode = mod_timer(&self->read_timer, jiffies + msecs_to_jiffies(self->timer_interval));

	CSTD_UNUSED(rcode);
}

/**
 * tl_reader_init_late() - Late CSFFW TL Reader initialization.
 *
 * @self:	CSFFW TL Reader instance.
 * @kbdev:	Kbase device.
 *
 * Late initialization is done once at kbase_csf_tl_reader_start() time.
 * This is because the firmware image is not parsed
 * by the kbase_csf_tl_reader_init() time.
 *
 * Return: Zero on success, -1 otherwise.
 */
static int tl_reader_init_late(struct kbase_csf_tl_reader *self, struct kbase_device *kbdev)
{
	struct firmware_trace_buffer *tb;
	size_t hdr_size = 0;
	const char *hdr = NULL;

	if (self->kbdev)
		return 0;

	tb = kbase_csf_firmware_get_trace_buffer(kbdev, KBASE_CSFFW_TIMELINE_BUF_NAME);
	hdr = kbase_csf_firmware_get_timeline_metadata(kbdev, KBASE_CSFFW_TIMELINE_HEADER_NAME,
						       &hdr_size);

	if (!tb) {
		dev_warn(kbdev->dev, "'%s' tracebuffer is not present in the firmware image.",
			 KBASE_CSFFW_TIMELINE_BUF_NAME);
		return -1;
	}

	if (!hdr) {
		dev_warn(kbdev->dev, "'%s' timeline metadata is not present in the firmware image.",
			 KBASE_CSFFW_TIMELINE_HEADER_NAME);
		return -1;
	}

	self->kbdev = kbdev;
	self->trace_buffer = tb;
	self->tl_header.data = hdr;
	self->tl_header.size = hdr_size;

	return 0;
}

/**
 * tl_reader_update_enable_bit() - Update the first bit of a CSFFW tracebuffer.
 *
 * @self:	CSFFW TL Reader instance.
 * @value:	The value to set.
 *
 * Update the first bit of a CSFFW tracebufer and then reset the GPU.
 * This is to make these changes visible to the MCU.
 *
 * Return: 0 on success, or negative error code for failure.
 */
static int tl_reader_update_enable_bit(struct kbase_csf_tl_reader *self, bool value)
{
	int err = 0;

	err = kbase_csf_firmware_trace_buffer_update_trace_enable_bit(self->trace_buffer, 0, value);

	return err;
}

void kbase_csf_tl_reader_init(struct kbase_csf_tl_reader *self, struct kbase_tlstream *stream)
{
	*self = (struct kbase_csf_tl_reader){
		.timer_interval = KBASE_CSF_TL_READ_INTERVAL_DEFAULT,
		.stream = stream,
		.kbdev = NULL, /* This will be initialized by tl_reader_init_late() */
		.is_active = false,
	};

	kbase_timer_setup(&self->read_timer, kbasep_csf_tl_reader_read_callback);

	spin_lock_init(&self->read_lock);
}

void kbase_csf_tl_reader_term(struct kbase_csf_tl_reader *self)
{
#if (KERNEL_VERSION(6, 15, 0) > LINUX_VERSION_CODE)
	del_timer_sync(&self->read_timer);
#else
	timer_delete_sync(&self->read_timer);
#endif
}

int kbase_csf_tl_reader_start(struct kbase_csf_tl_reader *self, struct kbase_device *kbdev)
{
	unsigned long flags;
	int rcode;

	spin_lock_irqsave(&self->read_lock, flags);

	/* If already running, early exit. */
	if (self->is_active) {
		spin_unlock_irqrestore(&self->read_lock, flags);
		return 0;
	}

	if (tl_reader_init_late(self, kbdev)) {
		spin_unlock_irqrestore(&self->read_lock, flags);
#if IS_ENABLED(CONFIG_MALI_NO_MALI)
		dev_warn(kbdev->dev, "CSFFW timeline is not available for MALI_NO_MALI builds!");
		return 0;
#else
		return -EINVAL;
#endif
	}

	tl_reader_reset(self);

	self->is_active = true;

	spin_unlock_irqrestore(&self->read_lock, flags);

	/* Set bytes to copy to the header size. This is to trigger copying
	 * of the header to the user space.
	 */
	self->tl_header.btc = self->tl_header.size;

	/* Enable the tracebuffer on the CSFFW side. */
	rcode = tl_reader_update_enable_bit(self, true);
	if (rcode != 0)
		return rcode;

	rcode = mod_timer(&self->read_timer, jiffies + msecs_to_jiffies(self->timer_interval));

	return 0;
}

void kbase_csf_tl_reader_stop(struct kbase_csf_tl_reader *self)
{
	unsigned long flags;

	/* If is not running, early exit. */
	if (!self->is_active)
		return;

	/* Disable the tracebuffer on the CSFFW side. */
	tl_reader_update_enable_bit(self, false);

#if (KERNEL_VERSION(6, 15, 0) > LINUX_VERSION_CODE)
	del_timer_sync(&self->read_timer);
#else
	timer_delete_sync(&self->read_timer);
#endif

	spin_lock_irqsave(&self->read_lock, flags);

	tl_reader_reset(self);

	spin_unlock_irqrestore(&self->read_lock, flags);
}

void kbase_csf_tl_reader_reset(struct kbase_csf_tl_reader *self)
{
	kbase_csf_tl_reader_flush_buffer(self);
}
