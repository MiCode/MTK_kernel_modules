// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2023 MediaTek Inc.

#include <linux/list.h>
#include <linux/proc_fs.h>

#include "mtk_cam.h"
#include "mtk_cam-debug.h"
#include "mtk_cam-debug_dump_header.h"

/*
 * for normal dump
 */

#define MAX_DUMP_BUFFER_NUM		20
struct dump_buffer {
	struct list_head list;

	void *addr;
	size_t size;

	int seq;
};

struct dump_ctrl {
	int id;

	struct mtk_cam_normal_dump *dump;

	struct proc_dir_entry *dir_entry;
	struct proc_dir_entry *ctrl_entry;
	struct proc_dir_entry *data_entry;

	struct mutex op_lock;
	spinlock_t lock;
	int buffer_cnt;
	struct list_head buf_pool;
	struct list_head dump_list;

	struct dump_buffer *cur_read;
};

static int mtk_cam_debug_exp_open(struct inode *inode, struct file *file)
{
	struct mtk_cam_exception *exp = pde_data(inode);

	if (WARN_ON(!exp))
		return -EFAULT;

	file->private_data = exp;
	return 0;
}

static ssize_t mtk_cam_debug_exp_read(struct file *file, char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct mtk_cam_exception *exp = file->private_data;
	size_t read_count;

	if (WARN_ON(!exp))
		return 0;

	if (!atomic_read(&exp->ready)) {
		pr_info("%s: exp dump is not ready\n", __func__);
		return 0;
	}

	pr_debug("%s: read buf request: %zu bytes\n", __func__, count);

	mutex_lock(&exp->lock);
	read_count = simple_read_from_buffer(user_buf, count, ppos,
					     exp->buf,
					     exp->buf_size);
	mutex_unlock(&exp->lock);

	return read_count;
}

static int mtk_cam_debug_exp_release(struct inode *inode, struct file *file)
{
	struct mtk_cam_exception *exp = file->private_data;

	if (WARN_ON(!exp))
		return 0;

	/* note: not to reset after read */
	//atomic_set(&exp->ready, 0);
	//pr_info("%s: reset exp dump ready\n", __func__);
	return 0;
}

static const struct proc_ops exp_fops = {
	.proc_open	= mtk_cam_debug_exp_open,
	.proc_read	= mtk_cam_debug_exp_read,
	.proc_release	= mtk_cam_debug_exp_release,
};

static int mtk_cam_exception_dump_init(struct mtk_cam_exception *exp)
{
	struct mtk_cam_debug *dbg =
		container_of(exp, struct mtk_cam_debug, exp);

	memset(exp, 0, sizeof(*exp));

	/* proc file system */
	exp->dump_entry = proc_create_data("mtk_cam_exp_dump", 0644, NULL,
					   &exp_fops, exp);
	if (!exp->dump_entry) {
		dev_info(dbg->cam->dev, "Can't create proc fs\n");
		return -ENOMEM;
	}

	atomic_set(&exp->ready, 0);
	mutex_init(&exp->lock);
	exp->buf_size = 0;
	exp->buf = NULL;
	return 0;
}

static void mtk_cam_exception_dump_deinit(struct mtk_cam_exception *exp)
{
	if (exp->buf)
		vfree(exp->buf);

	if (exp->dump_entry)
		proc_remove(exp->dump_entry);
}

static struct dump_buffer *alloc_dump_buffer(size_t size)
{
	struct dump_buffer *buf;
	void *addr;

	addr = vmalloc(sizeof(*buf) + size);
	if (WARN_ON(!addr))
		return NULL;

	buf = addr;

	INIT_LIST_HEAD(&buf->list);
	buf->addr = addr + sizeof(*buf);
	buf->size = size;
	buf->seq = 0;

	return buf;
}

static bool enough_buf_size(struct dump_buffer *buf, void *arg)
{
	return buf->size >= *(size_t *)arg;
}

static bool match_seql(struct dump_buffer *buf, void *arg)
{
	return buf->seq == *(unsigned long *)arg;
}

static bool first_entry(struct dump_buffer *buf, void *arg)
{
	return 1;
}

static struct dump_buffer *find_buffer_from_locked(struct list_head *list_head,
	bool (*func)(struct dump_buffer *buf, void *arg),
	void *arg)
{
	struct dump_buffer *buf;
	bool found;

	list_for_each_entry(buf, list_head, list) {
		found = func(buf, arg);
		if (found) {
			list_del(&buf->list);
			return buf;
		}
	}
	return NULL;
}

static struct dump_buffer *acquire_dump_buffer(struct dump_ctrl *ctrl,
					       size_t size)
{
	struct dump_buffer *release = NULL;
	struct dump_buffer *buf;
	int cnt;

	spin_lock(&ctrl->lock);

	buf = find_buffer_from_locked(&ctrl->buf_pool, enough_buf_size, &size);
	if (buf) {
		spin_unlock(&ctrl->lock);
		return buf;
	}

	buf = find_buffer_from_locked(&ctrl->dump_list, enough_buf_size, &size);
	if (buf) {
		spin_unlock(&ctrl->lock);
		pr_info("%s: recycle seq %d\n", __func__, buf->seq);
		return buf;
	}

	if (ctrl->buffer_cnt == MAX_DUMP_BUFFER_NUM) {

		release = find_buffer_from_locked(&ctrl->buf_pool,
						  first_entry, NULL);
		if (!release)
			release = find_buffer_from_locked(&ctrl->dump_list,
							  first_entry, NULL);

		if (release)
			--ctrl->buffer_cnt;
	}

	spin_unlock(&ctrl->lock);

	if (release)
		vfree(release);

	buf = alloc_dump_buffer(size);
	if (WARN_ON(!buf))
		return NULL;

	spin_lock(&ctrl->lock);
	cnt = ++ctrl->buffer_cnt;
	spin_unlock(&ctrl->lock);

	pr_info("%s: alloc num %d\n", __func__, cnt);
	return buf;
}

static void dump_ctrl_reset_cur_read(struct dump_ctrl *ctrl,
				    struct dump_buffer *buf,
				    spinlock_t *lock)
{
	if (lock)
		spin_lock(lock);

	if (ctrl->cur_read)
		list_add_tail(&ctrl->cur_read->list, &ctrl->buf_pool);

	ctrl->cur_read = buf;

	if (buf)
		pr_info("%s: cur seq %d\n", __func__, buf->seq);

	if (lock)
		spin_unlock(lock);
}

static int dump_ctrl_set_cur_read(struct dump_ctrl *ctrl, int seq)
{
	struct dump_buffer *buf;
	int ret;

	spin_lock(&ctrl->lock);

	buf = find_buffer_from_locked(&ctrl->dump_list, match_seql, &seq);
	dump_ctrl_reset_cur_read(ctrl, buf, NULL);

	ret = ctrl->cur_read ? 0 : -1;

	spin_unlock(&ctrl->lock);

	if (ret)
		pr_info("%s: warn. seq %d not found.\n", __func__, seq);

	return ret;
}

static int dump_ctrl_set_cur_read_first(struct dump_ctrl *ctrl)
{
	struct dump_buffer *buf;
	int ret;

	spin_lock(&ctrl->lock);

	buf = list_first_entry_or_null(&ctrl->dump_list,
				       struct dump_buffer, list);
	if (buf)
		list_del(&buf->list);

	dump_ctrl_reset_cur_read(ctrl, buf, NULL);

	ret = ctrl->cur_read ? 0 : -1;

	spin_unlock(&ctrl->lock);

	if (ret)
		pr_info("%s: warn. not found.\n", __func__);

	return ret;
}

static int dump_ctrl_reset_buffers(struct dump_ctrl *ctrl, int release)
{
	struct dump_buffer *buf, *buf_prev;
	struct list_head release_list;

	INIT_LIST_HEAD(&release_list);

	spin_lock(&ctrl->lock);

	// move all buffers to pool
	dump_ctrl_reset_cur_read(ctrl, NULL, NULL);
	list_splice_init(&ctrl->dump_list, &ctrl->buf_pool);

	if (release) {
		list_splice_init(&ctrl->buf_pool, &release_list);
		ctrl->buffer_cnt = 0;
	}

	spin_unlock(&ctrl->lock);

	list_for_each_entry_safe(buf, buf_prev, &release_list, list) {

		list_del(&buf->list);

		vfree(buf);
	}

	return 0;
}

static void dump_ctrl_set_dump_status(struct dump_ctrl *ctrl, int enable)
{
	struct mtk_cam_normal_dump *d = ctrl->dump;
	long bit = BIT(ctrl->id);

	if (enable)
		atomic_long_or(bit, &d->enabled);
	else
		atomic_long_andnot(bit, &d->enabled);
}

static int dbg_ctrl_open(struct inode *inode, struct file *file)
{
	struct dump_ctrl *ctrl = pde_data(inode);

	if (WARN_ON(!ctrl))
		return -EFAULT;

	file->private_data = ctrl;
	return 0;
}

static ssize_t dbg_ctrl_write(struct file *file, const char __user *data,
			      size_t count, loff_t *ppos)
{
	struct dump_ctrl *ctrl;
	char str[16];
	char *parse_str;
	char *cmd_str;
	char *param_str_0;
	char *param_str_1;
	char cmd = 0;
	char sub_cmd = 0;
	unsigned long seq = 0;

	ctrl = file->private_data;
	if (WARN_ON(!ctrl))
		return 0;

	if (count > sizeof(str) - 1) {
		pr_info("%s: pipe %d: invalid cmd size %zu\n",
			__func__, ctrl->id, count);
		return -EINVAL;
	}

	memset(str, 0, sizeof(str));
	if (copy_from_user(str, data, count)) {
		pr_info("%s: copy_from_user failed: pipe %d count %zu\n",
			__func__, ctrl->id, count);
		return -EINVAL;
	}

	parse_str = str;
	cmd_str = strsep(&parse_str, ":");
	if (!cmd_str)
		goto PROCESS_CMD_FAILED;

	param_str_0 = strsep(&parse_str, ":");
	if (!param_str_0)
		goto PROCESS_CMD_FAILED;

	cmd = cmd_str[0];
	sub_cmd = param_str_0[0];

	if (cmd == 'r') {

		if (sub_cmd == 'a') {
			if (dump_ctrl_set_cur_read_first(ctrl))
				goto PROCESS_CMD_FAILED;
			else
				goto PROCESS_CMD_DONE;
		}

		param_str_1 = strsep(&parse_str, ":");
		if (!param_str_1)
			goto PROCESS_CMD_FAILED;

		if (kstrtoul(param_str_1, 10, &seq))
			goto PROCESS_CMD_FAILED;

		if (sub_cmd == 's') {
			if (dump_ctrl_set_cur_read(ctrl, (int)seq))
				goto PROCESS_CMD_FAILED;
		} else if (sub_cmd == 'e') {
			dump_ctrl_reset_cur_read(ctrl, NULL, &ctrl->lock);
		} else
			goto PROCESS_CMD_FAILED;

	} else if (cmd == 'd') {
		if (sub_cmd == 's' || sub_cmd == 'r') {

			mutex_lock(&ctrl->op_lock);
			dump_ctrl_reset_buffers(ctrl, 0);
			dump_ctrl_set_dump_status(ctrl, 1);
			mutex_unlock(&ctrl->op_lock);
		} else if (sub_cmd == 'e') {

			mutex_lock(&ctrl->op_lock);
			dump_ctrl_set_dump_status(ctrl, 0);
			dump_ctrl_reset_buffers(ctrl, 1);
			mutex_unlock(&ctrl->op_lock);
		} else
			goto PROCESS_CMD_FAILED;
	}

PROCESS_CMD_DONE:
	return count;

PROCESS_CMD_FAILED:
	pr_info("%s: failed. pipe %d cmd = %c sub_cmd = %c\n",
		__func__, ctrl->id, cmd, sub_cmd);
	return -EINVAL;
}

static int dbg_data_open(struct inode *inode, struct file *file)
{
	struct dump_ctrl *ctrl = pde_data(inode);

	if (WARN_ON(!ctrl))
		return -EFAULT;

	file->private_data = ctrl;

	return 0;
}

static ssize_t dbg_data_read(struct file *file, char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	struct dump_ctrl *ctrl = file->private_data;
	struct dump_buffer *cur;
	size_t read_count;

	if (WARN_ON(!ctrl))
		return 0;

	cur = ctrl->cur_read;
	if (!cur) {
		pr_info("%s: not set cur_read yet\n", __func__);
		return 0;
	}

	read_count = simple_read_from_buffer(user_buf, count, ppos,
					     cur->addr, cur->size);
	return read_count;
}

static const struct proc_ops dbg_ctrl_fops = {
	.proc_open = dbg_ctrl_open,
	.proc_write = dbg_ctrl_write,
};

static const struct proc_ops dbg_data_fops = {
	.proc_open = dbg_data_open,
	.proc_read = dbg_data_read,
	.proc_lseek = default_llseek,
};

static int dump_ctrl_init(struct dump_ctrl *ctrl,
				 struct mtk_cam_normal_dump *dump,
				 int id)
{
	struct mtk_cam_debug *dbg =
		container_of(dump, struct mtk_cam_debug, dump);
	struct device *dev = dbg->cam->dev;
	char name[4];

	if (snprintf(name, 4, "%d", id) < 0)
		return -ENOMEM;

	ctrl->id = id;
	ctrl->dump = dump;

	mutex_init(&ctrl->op_lock);
	spin_lock_init(&ctrl->lock);
	INIT_LIST_HEAD(&ctrl->buf_pool);
	INIT_LIST_HEAD(&ctrl->dump_list);

	ctrl->dir_entry = proc_mkdir(name, dump->dbg_entry);
	if (!ctrl->dir_entry) {
		dev_info(dev, "failed to mkdir for %d\n", id);
		return -ENOMEM;
	}

	ctrl->ctrl_entry = proc_create_data("ctrl", 0664,
					    ctrl->dir_entry,
					    &dbg_ctrl_fops, ctrl);
	if (!ctrl->ctrl_entry) {
		dev_info(dev, "failed to create ctrl file for %d\n", id);
		return -ENOMEM;
	}

	ctrl->data_entry = proc_create_data("data", 0444,
					    ctrl->dir_entry,
					    &dbg_data_fops, ctrl);
	if (!ctrl->data_entry) {
		dev_info(dev, "failed to create data file for %d\n", id);
		return -ENOMEM;
	}
	return 0;
}

static void dump_ctrl_deinit(struct dump_ctrl *ctrl)
{
	if (ctrl->data_entry)
		proc_remove(ctrl->data_entry);

	if (ctrl->ctrl_entry)
		proc_remove(ctrl->ctrl_entry);

	if (ctrl->dir_entry)
		proc_remove(ctrl->dir_entry);

	dump_ctrl_reset_buffers(ctrl, 1);
}

static inline bool mtk_cam_normal_dump_enabled(struct mtk_cam_normal_dump *d,
					       int pipe_id)
{
	long enabled = atomic_long_read(&d->enabled);

	return !!(enabled & BIT(pipe_id));
}

static int mtk_cam_normal_dump_init(struct mtk_cam_normal_dump *dump)
{
	struct mtk_cam_debug *dbg =
		container_of(dump, struct mtk_cam_debug, dump);
	struct mtk_cam_device *cam = dbg->cam;
	int i;
	int ret;

	memset(dump, 0, sizeof(*dump));

	atomic_long_set(&dump->enabled, 0);

	dump->dbg_entry = proc_mkdir("mtk_cam_dbg", NULL);
	if (!dump->dbg_entry) {
		dev_info(dbg->cam->dev, "Can't create proc fs\n");
		return -ENOMEM;
	}

	dump->num_ctrls = cam->pipelines.num_raw;
	if (dump->num_ctrls <= 0)
		return 0;

	dump->ctrls = vzalloc(dump->num_ctrls * sizeof(*dump->ctrls));
	if (!dump->ctrls)
		return -ENOMEM;

	ret = 0;
	for (i = 0; i < dump->num_ctrls; i++)
		ret = ret || dump_ctrl_init(&dump->ctrls[i],
						   dump, i);
	return ret;
}

static void mtk_cam_normal_dump_deinit(struct mtk_cam_normal_dump *dump)
{
	int i;

	for (i = 0; i < dump->num_ctrls; i++)
		dump_ctrl_deinit(&dump->ctrls[i]);

	if (dump->dbg_entry)
		proc_remove(dump->dbg_entry);
}

int mtk_cam_debug_init(struct mtk_cam_debug *dbg, struct mtk_cam_device *cam)
{
	int ret;

	memset(dbg, 0, sizeof(*dbg));

	if (!cam)
		return -1;

	dbg->cam = cam;

	ret = mtk_cam_exception_dump_init(&dbg->exp);
	if (ret)
		return ret;

	ret = mtk_cam_normal_dump_init(&dbg->dump);
	if (ret)
		return ret;

	dev_info(cam->dev, "[%s] success\n", __func__);
	return ret;
}

void mtk_cam_debug_deinit(struct mtk_cam_debug *dbg)
{
	mtk_cam_exception_dump_deinit(&dbg->exp);
	mtk_cam_normal_dump_deinit(&dbg->dump);
}

static size_t required_buffer_size(struct mtk_cam_dump_param *p)
{
	return sizeof(struct mtk_cam_dump_header)
		+ p->cq_size
		+ p->meta_in_dump_buf_size
		+ p->meta_out_0_dump_buf_size
		+ p->meta_out_1_dump_buf_size
		+ p->frame_param_size
		+ p->config_param_size;
}

static int mtk_cam_write_header(struct mtk_cam_dump_param *p,
				struct mtk_cam_dump_header *hdr,
				size_t buf_size)
{
	strncpy(hdr->desc, p->desc, sizeof(hdr->desc) - 1);

	hdr->request_fd = p->request_fd;
	hdr->stream_id = p->stream_id;
	hdr->timestamp = p->timestamp;
	hdr->sequence = p->sequence;
	hdr->header_size = sizeof(*hdr);
	hdr->payload_offset = hdr->header_size;
	hdr->payload_size = buf_size - hdr->header_size;

	hdr->meta_version_major = GET_PLAT_V4L2(meta_major);
	hdr->meta_version_minor = GET_PLAT_V4L2(meta_minor);

	/* CQ dump */
	hdr->cq_dump_buf_offset = hdr->payload_offset;
	hdr->cq_size = p->cq_size;
	hdr->cq_iova = p->cq_iova;
	hdr->cq_desc_offset = p->cq_desc_offset;
	hdr->cq_desc_size = p->cq_desc_size;
	hdr->sub_cq_desc_offset = p->sub_cq_desc_offset;
	hdr->sub_cq_desc_size = p->sub_cq_desc_size;

	/* meta in */
	hdr->meta_in_dump_buf_offset = hdr->cq_dump_buf_offset +
		hdr->cq_size;
	hdr->meta_in_dump_buf_size = p->meta_in_dump_buf_size;
	hdr->meta_in_iova = p->meta_in_iova;

	/* meta out 0 */
	hdr->meta_out_0_dump_buf_offset = hdr->meta_in_dump_buf_offset +
		hdr->meta_in_dump_buf_size;
	hdr->meta_out_0_dump_buf_size = p->meta_out_0_dump_buf_size;
	hdr->meta_out_0_iova = p->meta_out_0_iova;

	/* meta out 1 */
	hdr->meta_out_1_dump_buf_offset =
		hdr->meta_out_0_dump_buf_offset +
		hdr->meta_out_0_dump_buf_size;
	hdr->meta_out_1_dump_buf_size = p->meta_out_1_dump_buf_size;
	hdr->meta_out_1_iova = p->meta_out_1_iova;

	/* meta out 2 */
	hdr->meta_out_2_dump_buf_offset =
		hdr->meta_out_1_dump_buf_offset +
		hdr->meta_out_1_dump_buf_size;
	hdr->meta_out_2_dump_buf_size = p->meta_out_2_dump_buf_size;
	hdr->meta_out_2_iova = p->meta_out_2_iova;

	/* ipi frame param */
	hdr->frame_dump_offset =
		hdr->meta_out_2_dump_buf_offset +
		hdr->meta_out_2_dump_buf_size;
	hdr->frame_dump_size = p->frame_param_size;

	/* ipi config param */
	hdr->config_dump_offset =
		hdr->frame_dump_offset +
		hdr->frame_dump_size;
	hdr->config_dump_size = p->config_param_size;
	hdr->used_stream_num = 1;

	if (hdr->config_dump_offset + hdr->config_dump_size > buf_size) {
		pr_info("[%s] buf is not enough\n", __func__);
		return -1;
	}

	return 0;
}

static int mtk_cam_dump_content_to_buf(void *buf, size_t buf_size,
				       void *src, size_t size,
				       size_t offset)
{
	if (!size)
		return 0;

	if (!buf || offset + size > buf_size)
		return -1;

	memcpy(buf + offset, src, size);
	return 0;
}

static int mtk_cam_dump_to_buf(struct mtk_cam_dump_param *p,
			       void *buf, size_t buf_size)
{
	struct mtk_cam_dump_header *hdr;
	int ret;

	if (buf_size < sizeof(*hdr))
		return -1;

	hdr = (struct mtk_cam_dump_header *)buf;
	memset(hdr, 0, sizeof(*hdr));

	if (mtk_cam_write_header(p, hdr, buf_size))
		return -1;

	ret = mtk_cam_dump_content_to_buf(buf, buf_size,
					  p->cq_cpu_addr,
					  hdr->cq_size,
					  hdr->cq_dump_buf_offset);

	ret = ret ||
		mtk_cam_dump_content_to_buf(buf, buf_size,
					    p->meta_in_cpu_addr,
					    hdr->meta_in_dump_buf_size,
					    hdr->meta_in_dump_buf_offset);

	ret = ret ||
		mtk_cam_dump_content_to_buf(buf, buf_size,
					    p->meta_out_0_cpu_addr,
					    hdr->meta_out_0_dump_buf_size,
					    hdr->meta_out_0_dump_buf_offset);

	ret = ret ||
		mtk_cam_dump_content_to_buf(buf, buf_size,
					    p->meta_out_1_cpu_addr,
					    hdr->meta_out_1_dump_buf_size,
					    hdr->meta_out_1_dump_buf_offset);

	ret = ret ||
		mtk_cam_dump_content_to_buf(buf, buf_size,
					    p->meta_out_2_cpu_addr,
					    hdr->meta_out_2_dump_buf_size,
					    hdr->meta_out_2_dump_buf_offset);

	ret = ret ||
		mtk_cam_dump_content_to_buf(buf, buf_size,
					    p->frame_params,
					    hdr->frame_dump_size,
					    hdr->frame_dump_offset);

	ret = ret ||
		mtk_cam_dump_content_to_buf(buf, buf_size,
					    p->config_params,
					    hdr->config_dump_size,
					    hdr->config_dump_offset);
	return ret;
}

bool mtk_cam_debug_dump_enabled(struct mtk_cam_debug *dbg, int pipe_id)
{
	return mtk_cam_normal_dump_enabled(&dbg->dump, pipe_id);
}

int mtk_cam_debug_dump(struct mtk_cam_debug *dbg,
		       int raw_pipe_id, struct mtk_cam_dump_param *p)
{
	struct device *dev = dbg->cam->dev;
	struct mtk_cam_normal_dump *dump;
	struct dump_ctrl *ctrl;
	size_t size;
	struct dump_buffer *buf;
	int ret = -1;

	dump = &dbg->dump;

	if (raw_pipe_id < 0 || raw_pipe_id >= dump->num_ctrls)
		return -1;

	ctrl = &dump->ctrls[raw_pipe_id];

	size  = required_buffer_size(p);

	mutex_lock(&ctrl->op_lock);

	/* check again within op_lock */
	if (!mtk_cam_normal_dump_enabled(dump, raw_pipe_id))
		goto EXIT;

	buf = acquire_dump_buffer(ctrl, size);
	if (!buf) {
		dev_info(dev, "%s: no buf to dump, size %zu\n", __func__, size);
		goto EXIT;
	}

	ret = mtk_cam_dump_to_buf(p, buf->addr, buf->size);
	if (ret) {
		spin_lock(&ctrl->lock);
		list_add_tail(&buf->list, &ctrl->buf_pool);
		spin_unlock(&ctrl->lock);
		goto EXIT;
	}

	buf->seq = p->sequence;

	spin_lock(&ctrl->lock);
	list_add_tail(&buf->list, &ctrl->dump_list);
	spin_unlock(&ctrl->lock);

EXIT:
	mutex_unlock(&ctrl->op_lock);

	dev_info(dev, "%s: ctx %d seq %d, buf_size %zu ret = %d\n",
		 __func__,
		 p->stream_id, p->sequence, size, ret);
	return 0;
}

static void set_exp_buf_size_locked(struct mtk_cam_exception *exp,
				    size_t new_size)
{
	if (!new_size) {
		if (exp->buf)
			vfree(exp->buf);

		exp->buf_size = 0;
		exp->buf = 0;
		return;
	}

	if (new_size <= exp->buf_size && exp->buf)
		return;

	if (exp->buf)
		vfree(exp->buf);

	exp->buf_size = max(exp->buf_size, new_size);
	exp->buf = vmalloc(exp->buf_size);
}

/* TODO: free if all users exit */
void mtk_cam_debug_exp_reset(struct mtk_cam_debug *dbg)
{
	struct mtk_cam_exception *exp = &dbg->exp;
	int before;

	before = atomic_read(&exp->ready);

	if (before)
		pr_info("%s: do reset status\n", __func__);

	atomic_set(&exp->ready, 0);

	mutex_lock(&exp->lock);
	set_exp_buf_size_locked(exp, 0);
	mutex_unlock(&exp->lock);
}

int mtk_cam_debug_exp_dump(struct mtk_cam_debug *dbg,
			   struct mtk_cam_dump_param *p)
{
	struct mtk_cam_exception *exp = &dbg->exp;
	struct device *dev = dbg->cam->dev;
	size_t new_size;
	int ret;

	if (!exp->dump_entry)
		return -1;

	if (atomic_read(&exp->ready)) {
		dev_info_ratelimited(dev,
				     "%s: skip due to unread dump\n", __func__);
		return -1;
	}

	mutex_lock(&exp->lock);

	new_size = required_buffer_size(p);
	set_exp_buf_size_locked(exp, new_size);

	if (!exp->buf) {
		dev_info(dev, "%s: no buf to dump, size %zu\n", __func__,
			 exp->buf_size);

		mutex_unlock(&exp->lock);
		return -1;
	}

	ret = mtk_cam_dump_to_buf(p, exp->buf, exp->buf_size);

	mutex_unlock(&exp->lock);

	if (!ret)
		atomic_set(&exp->ready, 1);

	dev_info(dev, "%s: ctx %d seq %d, buf_size %zu ret = %d\n",
		 __func__,
		 p->stream_id, p->sequence, exp->buf_size, ret);

	return ret;
}
