/*
 *  For CSF doorbell and IRQ validation
 */

#ifndef _KBASE_CSF_DB_VALIDATION_H_
#define _KBASE_CSF_DB_VALIDATION_H_

#include "mali_kbase_csf_db_irq_types.h"


struct kbase_device;
struct kbase_queue;

/**
 * kbase_csf_db_valid_init - Initialize the DB validation.
 *
 * @kbdev: Device pointer
 *
 * Return: 0 on success, or negative on failure.
 */
int kbase_csf_db_valid_init(struct kbase_device *kbdev);

/**
 * kbase_csf_db_valid_reset - Reset the DB validation.
 *
 * @kbdev: Device pointer
 *
 * Return: 0 on success, or negative on failure.
 */
int kbase_csf_db_valid_reset(struct kbase_device *kbdev);

/**
 * kbase_csf_db_valid_push_event - push event to ring buffer queue.
 *
 * @event: Event raw data
 *
 * Return: 0 on success, or negative on failure.
 */
int kbase_csf_db_valid_push_event(u32 event);

/**
 * kbase_csf_db_valid_pend_event - pend event and later push to ring buffer queue.
 *
 * @event: Event raw data
 *
 * Return: 0 on success, or negative on failure.
 */
int kbase_csf_db_valid_pend_event(u32 event);

/**
 * kbase_csf_db_valid_flush_pending_events - flush pending events to ring buffer queue.
 *
 * Return: 0 on success, or negative on failure.
 */
int kbase_csf_db_valid_flush_pending_events(void);

/**
 * kbase_csf_db_valid_push_user_event - increase user doorbell event counter in the CS_USER_INPUT page.
 *
 * @queue: command queue of the CS
 *
 * Return: 0 on success, or negative on failure.
 */
int kbase_csf_db_valid_push_user_event(struct kbase_queue *queue);

/**
 * kbase_csf_db_valid_debugfs_init - Initialize debug fs for the DB validation.
 *
 * @kbdev: Device pointer
 *
 * Return: 0 on success, or negative on failure.
 */
void kbase_csf_db_valid_debugfs_init(struct kbase_device *kbdev);

/**
 * kbasep_csf_db_valid_update_result - Request for FW to update result.
 *
 * @kbdev: Device pointer
 *
 * Return: 0 on success, or negative on failure.
 */
int kbasep_csf_db_valid_update_result(struct kbase_device *kbdev);


#endif /* _KBASE_CSF_DB_VALIDATION_H_ */