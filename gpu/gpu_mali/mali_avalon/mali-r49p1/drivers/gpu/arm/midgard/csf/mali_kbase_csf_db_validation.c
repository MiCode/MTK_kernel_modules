#if IS_ENABLED(CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL)
#include <mali_kbase.h>
#include "mali_kbase_csf_db_validation.h"
#include "mali_kbase_csf_db_irq_valid_ring_queue.h"
#include "mali_kbase_csf_firmware.h"

#define MAX_PENDING_COUNT 16

struct db_validation_context {
	struct kbase_device *kbdev;
	struct kbase_csf_global_iface *global_iface;
	u32 ring_queue_ptr;
	u32 pending_events[MAX_PENDING_COUNT];
	u32 pending_event_count;
	u32 result_buffer[DBVALID_RESULT_BUFFER_SIZE];
};

DEFINE_SPINLOCK(queue_lock);
static struct db_validation_context dbvld_ctx;


static void clear_ring_queue(u32 begin, u32 length)
{
	int i;

	for (i = 0 ; i < length ; ++i) {
		kbase_csf_firmware_global_input(dbvld_ctx.global_iface, RING_QUEUE_GLB_INPUT(begin + i), 0);
	}

	dmb(osh);
}

static void write_ring_queue(u32 event)
{
	if (dbvld_ctx.ring_queue_ptr < RING_QUEUE_HSIZE) {
		if (dbvld_ctx.ring_queue_ptr == RING_QUEUE_HSIZE - 1) {
			clear_ring_queue(RING_QUEUE_HSIZE, RING_QUEUE_HSIZE);
		}
	}
	else {
		if (dbvld_ctx.ring_queue_ptr == RING_QUEUE_SIZE - 1) {
			clear_ring_queue(0, RING_QUEUE_HSIZE);
		}
	}

	kbase_csf_firmware_global_input(dbvld_ctx.global_iface, RING_QUEUE_GLB_INPUT(dbvld_ctx.ring_queue_ptr), event);

	++dbvld_ctx.ring_queue_ptr;
	if (dbvld_ctx.ring_queue_ptr >= RING_QUEUE_SIZE) {
		dbvld_ctx.ring_queue_ptr = 0;
	}
}


int kbase_csf_db_valid_init(struct kbase_device *kbdev)
{
	dbvld_ctx.kbdev = kbdev;
	dbvld_ctx.ring_queue_ptr = 0;
	dbvld_ctx.pending_event_count = 0;

	return (dbvld_ctx.kbdev != NULL) ? 0 : -1;
}

int kbase_csf_db_valid_reset(struct kbase_device *kbdev)
{
	unsigned long flags;

	dbvld_ctx.global_iface = &kbdev->csf.global_iface;

	spin_lock_irqsave(&queue_lock, flags);

	// clear ring buffers
	if (dbvld_ctx.global_iface) {
		clear_ring_queue(0, RING_QUEUE_HSIZE);
	}

	dbvld_ctx.ring_queue_ptr = 0;
	dbvld_ctx.pending_event_count = 0;

	spin_unlock_irqrestore(&queue_lock, flags);

	return 0;
}

int kbase_csf_db_valid_push_event(u32 event)
{
	if (dbvld_ctx.global_iface) {
		unsigned long flags;

		spin_lock_irqsave(&queue_lock, flags);
		write_ring_queue(event);
		spin_unlock_irqrestore(&queue_lock, flags);

		dmb(osh);

		return 0;
	}

	return -1;
}

int kbase_csf_db_valid_pend_event(u32 event)
{
	unsigned long flags;

	if (dbvld_ctx.pending_event_count >= MAX_PENDING_COUNT) {
		dev_err(dbvld_ctx.kbdev->dev, "too many pending event, ignore event %08x\n", event);
		return -1;
	}

	spin_lock_irqsave(&queue_lock, flags);
	dbvld_ctx.pending_events[dbvld_ctx.pending_event_count++] = event;
	spin_unlock_irqrestore(&queue_lock, flags);

	return 0;
}

int kbase_csf_db_valid_flush_pending_events(void)
{
	int i;
	unsigned long flags;

	if (dbvld_ctx.pending_event_count==0)
		return 0;

	spin_lock_irqsave(&queue_lock, flags);

	for (i = 0 ; i < dbvld_ctx.pending_event_count ; ++i) {
		write_ring_queue(dbvld_ctx.pending_events[i]);
	}

	dbvld_ctx.pending_event_count = 0;

	spin_unlock_irqrestore(&queue_lock, flags);

	dmb(osh);

	return 0;
}

int kbase_csf_db_valid_push_user_event(struct kbase_queue *queue)
{
	if (queue && queue->user_io_addr) {
		u32 *volatile input_addr = (u32 *volatile)queue->user_io_addr;
		u32 seqno;

		dmb(osh);

		seqno = input_addr[USER_DB_KBASE_SEQ_OFFSET / 4];
		input_addr[USER_DB_KBASE_SEQ_OFFSET / 4] = seqno + 1;

		return 0;
	}

	return -1;
}

static inline bool kbasep_csf_db_valid_request_done(
					struct kbase_device *kbdev,
					u32 req_mask)
{
	struct kbase_csf_global_iface *global_iface =
				&kbdev->csf.global_iface;
	bool complete = false;
	unsigned long flags;

	kbase_csf_scheduler_spin_lock(kbdev, &flags);

	if ((kbase_csf_firmware_global_output(global_iface, DBVALID_ACK) & req_mask) ==
		(kbase_csf_firmware_global_input_read(global_iface, DBVALID_REQ) & req_mask))
		complete = true;

	kbase_csf_scheduler_spin_unlock(kbdev, flags);

	return complete;
}

static void kbasep_csf_db_valid_send_request(struct kbase_device *kbdev, u32 req_mask)
{
	u32 glb_req;
	u32 dbvalid_req;
	unsigned long flags;

	kbase_csf_scheduler_spin_lock(kbdev, &flags);

	dbvalid_req = kbase_csf_firmware_global_output(dbvld_ctx.global_iface, DBVALID_ACK);
	dbvalid_req ^= req_mask;
	kbase_csf_firmware_global_input_mask(dbvld_ctx.global_iface, DBVALID_REQ, dbvalid_req,
						 req_mask);

	dmb(osh);

	glb_req = kbase_csf_firmware_global_output(dbvld_ctx.global_iface, GLB_ACK);
	glb_req ^= GLB_REQ_DBVALID_EVENT_MASK;
	kbase_csf_firmware_global_input_mask(dbvld_ctx.global_iface, GLB_REQ, glb_req,
						 GLB_REQ_DBVALID_EVENT_MASK);

	kbase_csf_ring_doorbell(kbdev, CSF_KERNEL_DOORBELL_NR);

	kbase_csf_scheduler_spin_unlock(kbdev, flags);
}

static bool kbasep_csf_db_valid_wait_ack(struct kbase_device *kbdev, u32 req_mask)
{
	const unsigned int fw_timeout_ms = kbase_get_timeout_ms(kbdev, CSF_FIRMWARE_TIMEOUT);
	long wt = kbase_csf_timeout_in_jiffies(fw_timeout_ms);
	long remaining;

	remaining = wait_event_timeout(kbdev->csf.event_wait,
			kbasep_csf_db_valid_request_done(kbdev, req_mask), wt);

	return remaining > 0;
}

int kbasep_csf_db_valid_update_result(struct kbase_device *kbdev)
{
	kbasep_csf_db_valid_send_request(kbdev, DBVALID_REQ_RESULT_UPDATE_MASK);

	if (!kbasep_csf_db_valid_wait_ack(kbdev, DBVALID_REQ_RESULT_UPDATE_MASK)) {
		dev_warn(kbdev->dev, "DB validation may not able to get latest result!\n");
	}

	if (dbvld_ctx.global_iface) {
		int i;

		for (i = 0 ; i < DBVALID_RESULT_BUFFER_SIZE ; ++i) {
			dbvld_ctx.result_buffer[i] += kbase_csf_firmware_global_output(dbvld_ctx.global_iface, DBVALID_IFACE_IO_PAGE_RESULT + (i << 2));
		}

		dev_dbg(kbdev->dev, "DB validation update result\n");
	}

	return 0;
}

static const char *get_event_name(uint32_t event)
{
	const char *ev_name = "";

	switch (DOORBELL_GET_EVENT_GROUP(event)) {
		case DOORBELL_GROUP_A:
			{
				switch (DOORBELL_GET_EVENT_ID(event)) {
					case 1:  ev_name = "HALT";				   break;
					case 2:  ev_name = "CFG_PROGRESS_TIMER";	 break;
					case 3:  ev_name = "ALLOC_EN";			   break;
					case 4:  ev_name = "CFG_PWROFF_TIMER";	   break;
					case 5:  ev_name = "PROTM_ENTER";			break;
					case 6:  ev_name = "PRFCNT_ENABLE";		  break;
					case 7:  ev_name = "PRFCNT_SAMPLE";		  break;
					case 8:  ev_name = "COUNTER_ENABLE";		 break;
					case 9:  ev_name = "PING";				   break;
					case 10: ev_name = "FIRMWARE_CONFIG_UPDATE"; break;
					case 11: ev_name = "IDLE_ENABLE";			break;
					case 12: ev_name = "ITER_TRACE_ENABLE";	  break;
					case 13: ev_name = "SLEEP";				  break;
					case 14: ev_name = "DEBUG_CSF_REQ";		  break;
					case 23: ev_name = "SYNC_NOTIFY";			break;
					default:									 break;
				}
			}
			break;
		case DOORBELL_GROUP_B:
			{
				switch (DOORBELL_GET_EVENT_ID(event)) {
					case 1:  ev_name = "STATE";				  break;
					case 2:  ev_name = "EP_CFG";				 break;
					case 3:  ev_name = "STATUS_UPDATE";		  break;
					default:									 break;
				}
			}
			break;
		case DOORBELL_GROUP_C:
			{
				switch (DOORBELL_GET_EVENT_ID(event)) {
					case 1:  ev_name = "STATE";				  break;
					case 2:  ev_name = "EXTRACT_EVENT";		  break;
					case 3:  ev_name = "TILER_OOM";			  break;
					case 4:  ev_name = "FAULT";				  break;
					default:									 break;
				}
			}
			break;
		case DOORBELL_GROUP_D:
			break;
		default:
			break;
	}

	return ev_name;
}

static int kbasep_csf_db_valid_finish_test(struct seq_file *file,
		void *data)
{
	struct kbase_device *kbdev = file->private;
	bool gpu_is_on = kbdev->pm.backend.gpu_powered;
	const unsigned int fw_timeout_ms = kbase_get_timeout_ms(kbdev, CSF_FIRMWARE_TIMEOUT);

	if (!gpu_is_on) {
		int err;

		kbase_csf_scheduler_pm_active(kbdev);
		err = kbase_csf_scheduler_wait_mcu_active(kbdev);

		if (err) {
			seq_printf(file, "ERROR: MCU power-on failed\n");
			return 0;
		}
	}

	if (kbasep_csf_db_valid_update_result(kbdev) == 0) {
		const int _statistic_group_base[] = { 0, 24, 32, 40 };
		const u32 *result_total = dbvld_ctx.result_buffer;
		const u32 *result_miss = dbvld_ctx.result_buffer + DBVALID_RESULT_SIZE;
		int i, g = 0, missing = 0;

		seq_printf(file, "Statistic Report:\n");

		for (i = 0 ; i < DBVALID_RESULT_SIZE ; ++i) {
			const char *ev_name;
			uint32_t event;
			int ev_id;

			if (g < 4 && i >= _statistic_group_base[g]) {
				++g;
			}

			ev_id = i - _statistic_group_base[g - 1];
			event = DOORBELL_EVENT(g, ev_id, 0, 0);

			ev_name = get_event_name(event);

			switch (g)
			{
			case DOORBELL_GROUP_A:
				if (ev_name[0]) {
					seq_printf(file, "  GLB %-24s: %3d / %10d\n", ev_name, result_miss[i], result_total[i]);
				}
				break;
			case DOORBELL_GROUP_B:
				if (ev_name[0]) {
					seq_printf(file, "  CSG %-24s: %3d / %10d\n", ev_name, result_miss[i], result_total[i]);
				}
				break;
			case DOORBELL_GROUP_C:
				if (ev_name[0]) {
					seq_printf(file, "  CSI %-24s: %3d / %10d\n", ev_name, result_miss[i], result_total[i]);
				}
				break;
			case DOORBELL_GROUP_D:
				if (result_total[i] > 0) {
					seq_printf(file, "  USER Doorbell %-14d: %3d / %10d\n", ev_id, result_miss[i], result_total[i]);
				}
				break;
			default:
				break;
			}

			missing += result_miss[i];
		}

		if (missing > 0) {
			seq_printf(file, "\nTotal missing %d doorbell(s)\nFAILURES!!!\n", missing);
		}
		else {
			seq_printf(file, "\nNo missing doorbell\nOK (1 tests)\n");
		}

		memset(dbvld_ctx.result_buffer, 0, sizeof(u32) * DBVALID_RESULT_BUFFER_SIZE);
	}
	else {
		seq_printf(file, "DB validation finish timeout (%d ms)\nFAILURES!!!\n",
			fw_timeout_ms);
	}

	if (!gpu_is_on) {
		kbase_csf_scheduler_pm_idle(kbdev);
	}

	return 0;
}

static int kbasep_csf_db_valid_debugfs_open(struct inode *in,
		struct file *file)
{
	return single_open(file, kbasep_csf_db_valid_finish_test,
			in->i_private);
}

static const struct file_operations
	kbasep_csf_db_valid_debugfs_fops = {
	.open = kbasep_csf_db_valid_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void kbase_csf_db_valid_debugfs_init(struct kbase_device *kbdev)
{
	debugfs_create_file("dbvalid_finish", 0444,
		kbdev->mali_debugfs_directory, kbdev,
		&kbasep_csf_db_valid_debugfs_fops);
}
#endif /* CONFIG_MALI_MTK_WHITEBOX_MISSING_DOORBELL */
