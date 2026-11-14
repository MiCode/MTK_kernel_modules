// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>

#include "mtk_platform_debug.h"

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include <platform/mtk_platform_common/mtk_platform_logbuffer.h>
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

static bool kbase_is_register_accessible(u32 offset)
{
#ifdef CONFIG_MALI_DEBUG
	if (((offset >= MCU_SUBSYSTEM_BASE) && (offset < IPA_CONTROL_BASE)) ||
	    ((offset >= GPU_CONTROL_MCU_BASE) && (offset < USER_BASE))) {
		WARN(1, "Invalid register offset 0x%x", offset);
		return false;
	}
#endif

	return true;
}

static u32 kbase_reg_read(struct kbase_device *kbdev, u32 offset)
{
	u32 val;

	if (WARN_ON(!kbase_io_is_gpu_powered(kbdev)))
		return 0;

	if (WARN_ON(kbdev->dev == NULL))
		return 0;

	if (!kbase_is_register_accessible(offset))
		return 0;

	val = readl(kbdev->reg + offset);

#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (unlikely(kbdev->io_history.enabled))
		kbase_io_history_add(&kbdev->io_history, kbdev->reg + offset,
				     val, 0);
#endif /* CONFIG_DEBUG_FS */
	dev_dbg(kbdev->dev, "r: reg %08x val %08x", offset, val);

	return val;
}

#define CSHW_BASE 0x0030000
#define CSHW_CSHWIF_0 0x4000 /* () CSHWIF 0 registers */
#define CSHWIF(n) (CSHW_BASE + CSHW_CSHWIF_0 + (n)*256)
#define CSHWIF_REG(n, r) (CSHWIF(n) + r)
#define NR_HW_INTERFACES 4

#define CSHW_CTRL_REG(r)    (CSHW_BASE + 0x0000 + r)
#define CSHW_IT_COMP_REG(r) (CSHW_BASE + 0x1000 + r)
#define CSHW_IT_FRAG_REG(r) (CSHW_BASE + 0x2000 + r)
#define CSHW_IT_TILER_REG(r)(CSHW_BASE + 0x3000 + r)

static void dump_iterator_registers_locked(struct kbase_device *kbdev)
{
	unsigned long flags;

	lockdep_assert_held(&kbdev->hwaccess_lock);
	dev_err(kbdev->dev, "dump_iterator_registers");
	if (kbase_io_is_gpu_powered(kbdev)) {
		dev_err(kbdev->dev, "Compute  CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x QUEUE_COUNT %x",
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x4)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x8)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xD0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xDC)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xA4)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xA0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xE0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x20)));
		dev_err(kbdev->dev, "Fragment CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x QUEUE_COUNT %x",
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x4)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x8)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xD0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xDC)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xA4)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xA0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xE0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x20)));
		dev_err(kbdev->dev, "Tiler    CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x QUEUE_COUNT %x",
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x4)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x8)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xD0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xDC)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xA4)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xA0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xE0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x20)));
		dev_err(kbdev->dev, "\n");

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Compute  CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x QUEUE_COUNT %x\n",
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x4)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x8)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xD0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xDC)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xA4)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xA0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0xE0)),
			kbase_reg_read(kbdev, CSHW_IT_COMP_REG(0x20)));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Fragment CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x QUEUE_COUNT %x\n",
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x4)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x8)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xD0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xDC)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xA4)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xA0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0xE0)),
			kbase_reg_read(kbdev, CSHW_IT_FRAG_REG(0x20)));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Tiler    CTRL: %x STATUS: %x JASID: %u IRQ_RAW: %8x IRQ_STATUS: %8x EP_EVT_STATUS: %x BLOCKED_SB_ENTRY: %8x FAULT_STATUS %x QUEUE_COUNT %x\n",
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x4)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x8)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xD0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xDC)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xA4)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xA0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0xE0)),
			kbase_reg_read(kbdev, CSHW_IT_TILER_REG(0x20)));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}
}

static void dump_hwif_registers_locked(struct kbase_device *kbdev)
{
	unsigned long flags;
	unsigned int i;

	lockdep_assert_held(&kbdev->hwaccess_lock);
	dev_err(kbdev->dev, "dump_hwif_registers");
	for (i = 0; kbase_io_is_gpu_powered(kbdev) && (i < NR_HW_INTERFACES); i++) {
		u64 cmd_ptr = kbase_reg_read(kbdev, CSHWIF_REG(i, 0x0)) |
			((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0x4)) << 32);
		u64 cmd_ptr_end = kbase_reg_read(kbdev, CSHWIF_REG(i, 0x8)) |
			((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0xC)) << 32);
		int as_nr = kbase_reg_read(kbdev, CSHWIF_REG(i, 0x34));

		if (!cmd_ptr)
			continue;

		dev_err(kbdev->dev, "Register dump of CSHWIF %d", i);
		dev_err(kbdev->dev, "CMD_PTR: %llx CMD_PTR_END: %llx STATUS: %x JASID: %x EMUL_INSTR: %llx WAIT_STATUS: %x SB_SET_SEL: %x SB_SEL: %x",
			cmd_ptr,
			cmd_ptr_end,
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x24)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x34)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x60)) | ((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0x64)) << 32),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x74)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x78)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x7C)));
		dev_err(kbdev->dev, "CMD_COUNTER: %x EVT_RAW: %x EVT_IRQ_STATUS: %x EVT_HALT_STATUS: %x FAULT_STATUS: %x FAULT_ADDR: %llx",
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x80)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x98)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xA4)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xAC)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xB0)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xB8)) | ((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0xBC)) << 32));
		dev_err(kbdev->dev, "ITER_COMPUTE: %x ITER_FRAGMENT: %x ITER_TILER: %x",
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x28)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x2C)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x30)));
		dev_err(kbdev->dev, "\n");

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "Register dump of CSHWIF %d\n", i);
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"CMD_PTR: %llx CMD_PTR_END: %llx STATUS: %x JASID: %x EMUL_INSTR: %llx WAIT_STATUS: %x SB_SET_SEL: %x SB_SEL: %x\n",
			cmd_ptr,
			cmd_ptr_end,
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x24)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x34)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x60)) | ((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0x64)) << 32),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x74)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x78)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x7C)));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"CMD_COUNTER: %x EVT_RAW: %x EVT_IRQ_STATUS: %x EVT_HALT_STATUS: %x FAULT_STATUS: %x FAULT_ADDR: %llx\n",
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x80)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x98)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xA4)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xAC)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xB0)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0xB8)) | ((u64)kbase_reg_read(kbdev, CSHWIF_REG(i, 0xBC)) << 32));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"ITER_COMPUTE: %x ITER_FRAGMENT: %x ITER_TILER: %x\n",
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x28)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x2C)),
			kbase_reg_read(kbdev, CSHWIF_REG(i, 0x30)));
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}
}

static void dump_cshw_registers_locked(struct kbase_device *kbdev)
{
	unsigned long flags;

	lockdep_assert_held(&kbdev->hwaccess_lock);

	dev_err(kbdev->dev, "dump_cshw_registers");
	if (kbase_io_is_gpu_powered(kbdev)) {
		dev_err(kbdev->dev, "CSHW_IDVS_CTRL: %x", kbase_reg_read(kbdev, CSHW_CTRL_REG(0xD0)));
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION, "Register dump of CSHW\n");
		mtk_logbuffer_type_print(kbdev, MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"CSHW_IDVS_CTRL: %x\n",
			kbase_reg_read(kbdev, CSHW_CTRL_REG(0xD0)));
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}
}

void mtk_debug_csf_dump_iterator_hwif(struct kbase_device *kbdev)
{
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	dump_iterator_registers_locked(kbdev);
	dump_hwif_registers_locked(kbdev);
	dump_cshw_registers_locked(kbdev);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

void mtk_debug_csf_dump_iterator_hwif_locked(struct kbase_device *kbdev)
{
	lockdep_assert_held(&kbdev->hwaccess_lock);

	dump_iterator_registers_locked(kbdev);
	dump_hwif_registers_locked(kbdev);
	dump_cshw_registers_locked(kbdev);
}


#if IS_ENABLED(CONFIG_DEBUG_FS)
static int mtk_debug_cshw_ctrl_dump_show(struct seq_file *m, void *v)
{
	struct kbase_device *kbdev = (struct kbase_device *)m->private;

	if (kbdev && kbase_io_is_gpu_powered(kbdev))
		seq_printf(m, "CSHW_IDVS_CTRL = %x\n", kbase_reg_read(kbdev, CSHW_CTRL_REG(0xD0)));

	return 0;
}

static int mtk_debug_cshw_ctrl_dump_open(struct inode *in, struct file *file)
{
	struct kbase_device *kbdev = in->i_private;
	file->private_data = kbdev;

	if (file->f_mode & FMODE_WRITE)
		return 0;

	return single_open(file, mtk_debug_cshw_ctrl_dump_show, in->i_private);
}

static int mtk_debug_cshw_ctrl_dump_release(struct inode *in, struct file *file)
{
	if (!(file->f_mode & FMODE_WRITE)) {
		struct seq_file *m = (struct seq_file *)file->private_data;

		if (m)
			seq_release(in, file);
	}

	return 0;
}

static const struct file_operations mtk_debug_cshw_ctrl_dump_fops = {
	.open    = mtk_debug_cshw_ctrl_dump_open,
	.release = mtk_debug_cshw_ctrl_dump_release,
	.read    = seq_read,
	.llseek  = seq_lseek
};

int mtk_debug_cshw_ctrl_dump_debugfs_init(struct kbase_device *kbdev)
{
	if (IS_ERR_OR_NULL(kbdev))
		return -1;

	debugfs_create_file("cshw_ctrl_dump", 0444,
			kbdev->mali_debugfs_directory, kbdev,
			&mtk_debug_cshw_ctrl_dump_fops);
	return 0;
}
#else /* CONFIG_DEBUG_FS */
int mtk_debug_cshw_ctrl_dump_debugfs_init(struct kbase_device *kbdev)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */
