// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/seq_file.h>
#if IS_ENABLED(CONFIG_PROC_FS)
#include <linux/proc_fs.h>
#endif
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_hwaccess_time.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <platform/mtk_platform_common.h>
#include <platform/mtk_platform_common/mtk_platform_logbuffer.h>

static phys_addr_t reserved_mem_phys;
static phys_addr_t reserved_mem_virt;
static phys_addr_t reserved_mem_size;

#if IS_ENABLED(CONFIG_PROC_FS)
static int mtk_logbuffer_regular_show(struct seq_file *m, void *v)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	mtk_logbuffer_dump(&kbdev->logbuf_regular, m);

	return 0;
}
DEFINE_PROC_SHOW_ATTRIBUTE(mtk_logbuffer_regular);

static int mtk_logbuffer_exception_show(struct seq_file *m, void *v)
{
	struct kbase_device *kbdev = (struct kbase_device *)mtk_common_get_kbdev();

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	mtk_logbuffer_dump(&kbdev->logbuf_exception, m);

	return 0;
}
DEFINE_PROC_SHOW_ATTRIBUTE(mtk_logbuffer_exception);

int mtk_logbuffer_procfs_init(struct kbase_device *kbdev, struct proc_dir_entry *parent)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	if (kbdev->logbuf_regular.entries)
		proc_create(kbdev->logbuf_regular.name, 0440, parent, &mtk_logbuffer_regular_proc_ops);

	if (kbdev->logbuf_exception.entries)
		proc_create(kbdev->logbuf_exception.name, 0440, parent, &mtk_logbuffer_exception_proc_ops);

	return 0;
}

int mtk_logbuffer_procfs_term(struct kbase_device *kbdev, struct proc_dir_entry *parent)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	if (kbdev->logbuf_regular.entries)
		remove_proc_entry(kbdev->logbuf_regular.name, parent);

	if (kbdev->logbuf_exception.entries)
		remove_proc_entry(kbdev->logbuf_exception.name, parent);

	return 0;
}
#endif

u64 mtk_logbuffer_get_timestamp(struct kbase_device *kbdev, struct mtk_logbuffer_info *logbuf)
{
	u64 val;
#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
	enum kbase_mcu_state mcu_state;
#endif

	if (IS_ERR_OR_NULL(kbdev) || !logbuf->has_timestamp)
		return 0;

#if IS_ENABLED(CONFIG_MALI_CSF_SUPPORT)
	mcu_state = kbdev->pm.backend.mcu_state;

	/* If MCU is not active, not need to print timestamp for CSFFW logs.
	 */
	if ((mcu_state == KBASE_MCU_OFF) || (mcu_state == KBASE_MCU_IN_SLEEP))
		val = 0;
	else
		val = kbase_backend_get_timestamp(kbdev);
#else
	val = 0;
#endif

	return val;
}

bool mtk_logbuffer_is_empty(struct mtk_logbuffer_info *logbuf)
{
	return logbuf->head == logbuf->tail;
}

bool mtk_logbuffer_is_full(struct mtk_logbuffer_info *logbuf)
{
	return logbuf->head == (logbuf->tail + 1) % logbuf->size;
}

void mtk_logbuffer_clear(struct mtk_logbuffer_info *logbuf)
{
	unsigned long flags;

	if (!logbuf->entries)
		return;

	spin_lock_irqsave(&logbuf->access_lock, flags);
	memset(logbuf->entries, 0x0, logbuf->size);
	logbuf->head = logbuf->tail = 0;
	spin_unlock_irqrestore(&logbuf->access_lock, flags);
}

void mtk_logbuffer_print(struct mtk_logbuffer_info *logbuf, const char *fmt, ...)
{
	va_list args;
	unsigned long flags;
	uint8_t buffer[MTK_LOG_BUFFER_ENTRY_SIZE];
	uint64_t ts_nsec = local_clock();
	uint32_t rem_nsec, entry_len;

	if (!logbuf->entries)
		return;

	spin_lock_irqsave(&logbuf->access_lock, flags);

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (!logbuf->is_circular && mtk_logbuffer_is_full(logbuf))
		goto fail_overflow;

	rem_nsec = do_div(ts_nsec, 1000000000);

	/* Calculate the new entry length */
	if (mtk_logbuffer_is_empty(logbuf)) {
		/*
		 * Since each logbuffer must be null-terminated,
		 * so add a newline at the beginning of this logbuffer
		 */
		entry_len = scnprintf(logbuf->tmp_entry,
		                      MTK_LOG_BUFFER_ENTRY_SIZE, "\n[%5lu.%06lu] %s",
		                      (unsigned long)ts_nsec, (unsigned long)rem_nsec / 1000, buffer);
	} else
		entry_len = scnprintf(logbuf->tmp_entry,
		                      MTK_LOG_BUFFER_ENTRY_SIZE, "[%5lu.%06lu] %s",
		                      (unsigned long)ts_nsec, (unsigned long)rem_nsec / 1000, buffer);

	if (entry_len == 0 || entry_len >= logbuf->size)
		goto fail_invalid_entry;

	if (!logbuf->is_circular) {
		if ((logbuf->tail + entry_len) >= logbuf->size) {
			/* Overflow */
			logbuf->tail = logbuf->size - 1;

			goto fail_overflow;
		} else {
			/* Append the new entry to the tail */
			scnprintf(logbuf->entries + logbuf->tail, entry_len + 1, "%s", logbuf->tmp_entry);

			/* Make sure the tail points to the end of the new entry */
			logbuf->tail += entry_len - 1;

			/* Update the buffer indices */
			logbuf->tail = (logbuf->tail + 1) % logbuf->size;
		}
	} else {
		if ((logbuf->tail + entry_len) >= logbuf->size) {
			/* Clean the unused contents */
			size_t remaining_size = logbuf->size - logbuf->tail;
			memset(logbuf->entries + logbuf->tail, 0, remaining_size);

			/* Update the tail and head to start over from the beginning */
			logbuf->tail = 0;
			logbuf->head = 1;

			/* Append the new entry to the tail */
			scnprintf(logbuf->entries + logbuf->tail, entry_len + 1, "%s", logbuf->tmp_entry);

			/* Make sure the tail points to the end of the new entry */
			logbuf->tail += entry_len - 1;

			/* Update the circular buffer indices */
			logbuf->tail = (logbuf->tail + 1) % logbuf->size;
			logbuf->head = (logbuf->tail + 1) % logbuf->size;
		} else {
			/* Append the new entry to the tail */
			scnprintf(logbuf->entries + logbuf->tail, entry_len + 1, "%s", logbuf->tmp_entry);

			if (mtk_logbuffer_is_full(logbuf)) {
				/* Make sure the tail points to the end of the new entry */
				logbuf->tail += entry_len - 1;

				/* Update the circular buffer indices */
				logbuf->tail = (logbuf->tail + 1) % logbuf->size;
				logbuf->head = (logbuf->tail + 1) % logbuf->size;
			} else {
				/* Make sure the tail points to the end of the new entry */
				logbuf->tail += entry_len - 1;

				/* Update the circular buffer indices */
				logbuf->tail = (logbuf->tail + 1) % logbuf->size;
			}
		}
	}

fail_overflow:
fail_invalid_entry:
	spin_unlock_irqrestore(&logbuf->access_lock, flags);
}

void __mtk_logbuffer_print(struct mtk_logbuffer_info *logbuf, uint8_t* buffer)
{
	unsigned long flags;
	uint64_t ts_nsec = local_clock();
	uint32_t rem_nsec, entry_len;

	if (!logbuf->entries)
		return;

	spin_lock_irqsave(&logbuf->access_lock, flags);

	if (!logbuf->is_circular && mtk_logbuffer_is_full(logbuf))
		goto fail_overflow;

	rem_nsec = do_div(ts_nsec, 1000000000);

	/* Calculate the new entry length */
	if (mtk_logbuffer_is_empty(logbuf)) {
		/*
		 * Since each logbuffer must be null-terminated,
		 * so add a newline at the beginning of this logbuffer
		 */
		entry_len = scnprintf(logbuf->tmp_entry,
		                      MTK_LOG_BUFFER_ENTRY_SIZE, "\n[%5lu.%06lu] %s",
		                      (unsigned long)ts_nsec, (unsigned long)rem_nsec / 1000, buffer);
	} else
		entry_len = scnprintf(logbuf->tmp_entry,
		                      MTK_LOG_BUFFER_ENTRY_SIZE, "[%5lu.%06lu] %s",
		                      (unsigned long)ts_nsec, (unsigned long)rem_nsec / 1000, buffer);

	if (entry_len == 0 || entry_len >= logbuf->size)
		goto fail_invalid_entry;

	if (!logbuf->is_circular) {
		if ((logbuf->tail + entry_len) >= logbuf->size) {
			/* Overflow */
			logbuf->tail = logbuf->size - 1;

			goto fail_overflow;
		} else {
			/* Append the new entry to the tail */
			scnprintf(logbuf->entries + logbuf->tail, entry_len + 1, "%s", logbuf->tmp_entry);

			/* Make sure the tail points to the end of the new entry */
			logbuf->tail += entry_len - 1;

			/* Update the buffer indices */
			logbuf->tail = (logbuf->tail + 1) % logbuf->size;
		}
	} else {
		if ((logbuf->tail + entry_len) >= logbuf->size) {
			/* Clean the unused contents */
			size_t remaining_size = logbuf->size - logbuf->tail;
			memset(logbuf->entries + logbuf->tail, 0, remaining_size);

			/* Update the tail and head to start over from the beginning */
			logbuf->tail = 0;
			logbuf->head = 1;

			/* Append the new entry to the tail */
			scnprintf(logbuf->entries + logbuf->tail, entry_len + 1, "%s", logbuf->tmp_entry);

			/* Make sure the tail points to the end of the new entry */
			logbuf->tail += entry_len - 1;

			/* Update the circular buffer indices */
			logbuf->tail = (logbuf->tail + 1) % logbuf->size;
			logbuf->head = (logbuf->tail + 1) % logbuf->size;
		} else {
			/* Append the new entry to the tail */
			scnprintf(logbuf->entries + logbuf->tail, entry_len + 1, "%s", logbuf->tmp_entry);

			if (mtk_logbuffer_is_full(logbuf)) {
				/* Make sure the tail points to the end of the new entry */
				logbuf->tail += entry_len - 1;

				/* Update the circular buffer indices */
				logbuf->tail = (logbuf->tail + 1) % logbuf->size;
				logbuf->head = (logbuf->tail + 1) % logbuf->size;
			} else {
				/* Make sure the tail points to the end of the new entry */
				logbuf->tail += entry_len - 1;

				/* Update the circular buffer indices */
				logbuf->tail = (logbuf->tail + 1) % logbuf->size;
			}
		}
	}

fail_overflow:
fail_invalid_entry:
	spin_unlock_irqrestore(&logbuf->access_lock, flags);
}

void mtk_logbuffer_type_print(struct kbase_device *const kbdev, enum mtk_logbuffer_type logType, const char *fmt, ...)
{
	va_list args;
	uint8_t buffer[MTK_LOG_BUFFER_ENTRY_SIZE];

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (logType == MTK_LOGBUFFER_TYPE_ALL || logType == MTK_LOGBUFFER_TYPE_REGULAR)
		__mtk_logbuffer_print(&kbdev->logbuf_regular, buffer);

	if (logType == MTK_LOGBUFFER_TYPE_ALL || logType == MTK_LOGBUFFER_TYPE_EXCEPTION)
		__mtk_logbuffer_print(&kbdev->logbuf_exception, buffer);
}

void mtk_logbuffer_dump(struct mtk_logbuffer_info *logbuf, struct seq_file *seq)
{
	uint32_t start, end, used_entry_num;

	if (!logbuf->entries)
		return;

	start = logbuf->head;
	end = logbuf->tail;

	if (start > end)
		used_entry_num = logbuf->size;
	else if (start == end)
		used_entry_num = 0;
	else
		used_entry_num = (end - start + 1);

	if (seq) {
		if (logbuf->tail >= logbuf->head) {
			seq_printf(seq, "---------- %s (%d/%d) ----------",
			           logbuf->name, used_entry_num, logbuf->size);
			seq_printf(seq, "%s", logbuf->entries);
		} else {
			seq_printf(seq, "---------- %s (%d/%d) ----------\n",
			           logbuf->name, used_entry_num, logbuf->size);
			seq_printf(seq, "%s", logbuf->entries + logbuf->head);
			seq_printf(seq, "%s", logbuf->entries);
		}
	}
}

static void mtk_logbuffer_init_internal(struct kbase_device *kbdev, struct mtk_logbuffer_info *logbuf,
                                        uint8_t *rmem_virt, size_t rmem_size, size_t offset, size_t size,
                                        bool is_circular, bool has_timestamp, const char *name)
{
	logbuf->fallback = (!rmem_virt || size > rmem_size) ? true : false;

	if (name && size) {
		spin_lock_init(&logbuf->access_lock);
		logbuf->tail = 0;
		logbuf->head = 0;
		logbuf->name[0] = '\0';
		logbuf->is_circular = is_circular;
		logbuf->has_timestamp = has_timestamp;
		logbuf->size = size;

		if (!logbuf->fallback)
			logbuf->entries = rmem_virt + offset;
		else
			logbuf->entries = kcalloc(1, size, GFP_KERNEL);
	}

	if (logbuf->entries)
		snprintf(logbuf->name, MTK_LOG_BUFFER_NAME_LEN, "%s", name);

	dev_info(kbdev->dev,
	         "@%s: name='%s' entries=0x%p size=%u is_circular=%d rmem_virt=0x%p rmem_size=%u fallback=%d",
	         __func__, logbuf->name, logbuf->entries, logbuf->size,
	         logbuf->is_circular, rmem_virt, rmem_size, logbuf->fallback);
}

static void mtk_logbuffer_term_internal(struct kbase_device *kbdev, struct mtk_logbuffer_info *logbuf)
{
	unsigned long flags;

	if (!logbuf->entries)
		return;

	spin_lock_irqsave(&logbuf->access_lock, flags);

	if (logbuf->fallback)
		kfree(logbuf->entries);

	logbuf->entries = NULL;
	logbuf->head = logbuf->tail = 0;

	spin_unlock_irqrestore(&logbuf->access_lock, flags);
}

int mtk_logbuffer_init(struct kbase_device *kbdev)
{
	struct device_node *rmem_node = NULL;
	struct reserved_mem *rmem = NULL;
	phys_addr_t rmem_remaining_size = 0;
	phys_addr_t logbuf_size = 0;

	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	rmem_node = of_find_compatible_node(NULL, NULL, "mediatek,me_gpu_reserved");
	if (rmem_node) {
		rmem = of_reserved_mem_lookup(rmem_node);
		if (rmem) {
			void *rmem_virt = ioremap_wc(rmem->base, rmem->size);
			if (rmem_virt) {
				/* Ensure that the memory is cleared */
				memset(rmem_virt, 0, rmem->size);

				/* Acquire valid memory region */
				reserved_mem_phys   = (phys_addr_t)rmem->base;
				reserved_mem_virt   = (phys_addr_t)rmem_virt;
				reserved_mem_size   = (phys_addr_t)rmem->size;
				rmem_remaining_size = (phys_addr_t)rmem->size;
			}
		}
	}

	dev_info(kbdev->dev, "@%s: Reserved memory region: phys=0x%llx size=0x%llx virt=0x%llx",
	         __func__, reserved_mem_phys, reserved_mem_size, reserved_mem_virt);

	/* Create a circular buffer for regular logs */
	logbuf_size = 1024 * 1536;
	mtk_logbuffer_init_internal(kbdev,
	                            &kbdev->logbuf_regular,      /* logbuf */
	                            (uint8_t *)reserved_mem_virt /* rmem_va */,
	                            (size_t)rmem_remaining_size  /* rmem_size */,
	                            0                            /* offset */,
	                            (size_t)logbuf_size          /* size */,
	                            true                         /* is_circular */,
	                            true                         /* has_timestamp */,
	                            "logbuf_regular"             /* name */);
	rmem_remaining_size -= logbuf_size;

	/* Create a non-circular buffer for exception logs */
	logbuf_size = 1024 * 512;
	mtk_logbuffer_init_internal(kbdev,
	                            &kbdev->logbuf_exception,    /* logbuf */
	                            (uint8_t *)reserved_mem_virt /* rmem_virt */,
	                            (size_t)rmem_remaining_size  /* rmem_size */,
	                            1024 * 1536                  /* offset */,
	                            (size_t)logbuf_size          /* size */,
	                            false                        /* is_circular */,
	                            false                        /* has_timestamp */,
	                            "logbuf_exception"           /* name */);
	rmem_remaining_size -= logbuf_size;

	return 0;
}

int mtk_logbuffer_term(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	/* Destroy a circular buffer for regular logs */
	mtk_logbuffer_term_internal(kbdev, &kbdev->logbuf_regular);

	/* Destroy a non-circular buffer for exception logs */
	mtk_logbuffer_term_internal(kbdev, &kbdev->logbuf_exception);

	if (reserved_mem_virt)
		iounmap((void __iomem *)reserved_mem_virt);

	return 0;
}
