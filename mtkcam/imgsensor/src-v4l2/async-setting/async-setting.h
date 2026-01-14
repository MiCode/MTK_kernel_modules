#ifndef __ASYNC_SETTING_H__
#define __ASYNC_SETTING_H__

#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include "task-queue.h"
#include "adaptor-subdrv.h"

#define NAME_LEN 64

struct subdrv_ctx;
typedef void (*setting_work_fnc)(struct subdrv_ctx *ctx);

typedef enum {
	INIT_SETTING,
	RES_SETTING,
	EXP_SETTING,
} SETTING_TYPE;

struct setting_workqueue {
	char name[NAME_LEN];
	int count;
	struct mutex mutex;
	wait_queue_head_t wq;
	task_queue_t *queue;
};

struct setting_work {
	char name[NAME_LEN];
	bool used;
	SETTING_TYPE type;
	struct subdrv_ctx *ctx;
	setting_work_fnc write_setting;
	struct setting_workqueue *setting_workqueue;
	task_t work;
};

#define destroy_setting_workqueue(setting_workqueue) \
({                                                   \
	__destroy_setting_workqueue(setting_workqueue);  \
	setting_workqueue = NULL;                        \
})

struct setting_work* create_and_queue_setting_work(
			struct setting_workqueue *setting_workqueue,
			char *name,
			SETTING_TYPE type,
			setting_work_fnc write_setting,
			struct subdrv_ctx *ctx);

struct setting_workqueue* create_setting_workqueue(const char *name);
void __destroy_setting_workqueue(struct setting_workqueue *setting_workqueue);

void queue_setting_work(struct setting_work *setting_work);
void wait_workqueue_done(struct setting_workqueue *setting_workqueue);

void lock_setting_work(struct setting_workqueue *setting_workqueue);
void unlock_setting_work(struct setting_workqueue *setting_workqueue);
#endif
