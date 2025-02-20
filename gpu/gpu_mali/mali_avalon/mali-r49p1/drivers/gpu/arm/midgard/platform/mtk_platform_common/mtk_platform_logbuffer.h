// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_LOGBUF_H__
#define __MTK_PLATFORM_LOGBUF_H__

#define MTK_LOG_BUFFER_NAME_LEN 64
#define MTK_LOG_BUFFER_ENTRY_SIZE 256

struct mtk_logbuffer_info {
	spinlock_t access_lock;
	uint32_t tail;
	uint32_t head;
	uint32_t size;
	char name[MTK_LOG_BUFFER_NAME_LEN];
	uint8_t tmp_entry[MTK_LOG_BUFFER_ENTRY_SIZE];
	uint8_t *entries;
	bool is_circular;
	bool fallback;
};


#define MTK_LOGBUFFER_TYPE_REGULAR       ((u32)0x1<<1)
#define MTK_LOGBUFFER_TYPE_CRITICAL      ((u32)0x1<<2)
#define MTK_LOGBUFFER_TYPE_EXCEPTION     ((u32)0x1<<3)
#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
#define MTK_LOGBUFFER_TYPE_DEFERRED      ((u32)0x1<<4)
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */

int mtk_logbuffer_init(struct kbase_device *kbdev);
int mtk_logbuffer_term(struct kbase_device *kbdev);
int mtk_logbuffer_procfs_init(struct kbase_device *kbdev, struct proc_dir_entry *parent);
int mtk_logbuffer_procfs_term(struct kbase_device *kbdev, struct proc_dir_entry *parent);
bool mtk_logbuffer_is_empty(struct mtk_logbuffer_info *logbuf);
bool mtk_logbuffer_is_full(struct mtk_logbuffer_info *logbuf);
void mtk_logbuffer_clear(struct mtk_logbuffer_info *logbuf);
void mtk_logbuffer_type_print(struct kbase_device *const kbdev, uint32_t logType, const char *fmt, ...);
void mtk_logbuffer_dump(struct mtk_logbuffer_info *logbuf, struct seq_file *seq);
u64 mtk_logbuffer_get_timestamp(struct kbase_device *kbdev);
#if IS_ENABLED(CONFIG_MALI_MTK_DEFERRED_LOGGING)
void mtk_logbuffer_dump_to_dev_and_clear(struct kbase_device *const kbdev, uint32_t logType);
int mtk_logbuffer_deferred_enable_debugfs_init(struct kbase_device *kbdev);
#endif /* CONFIG_MALI_MTK_DEFERRED_LOGGING */

#endif /* __MTK_PLATFORM_LOGBUF_H__ */
