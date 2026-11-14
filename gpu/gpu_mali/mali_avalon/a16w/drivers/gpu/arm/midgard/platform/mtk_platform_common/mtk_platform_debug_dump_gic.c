// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <mali_kbase.h>
#include <mali_kbase_defs.h>

#include <linux/of_irq.h>

#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
#include "mtk_platform_logbuffer.h"
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */

extern void mt_irq_dump_status(unsigned int irq);

void mtk_debug_dump_gic_status(struct kbase_device *kbdev)
{
	int i = 0;
	unsigned int irq = 0;
	unsigned long flags;
	unsigned int dump_irq_num = 0;

	/* Check if the irq is combined */
	if (kbdev->nr_irqs == 1)
		/* 0: IRQRAW */
		dump_irq_num = 1;
	else
		/* 0: GPU, 1: MMU, 2: JOB */
		dump_irq_num = 3;

	/* Dump gic information */
	for (i = 0; i < dump_irq_num; i++) {
		irq = irq_of_parse_and_map(kbdev->dev->of_node, i);
		if (irq)
			mt_irq_dump_status(irq);
	}

	/* Get further debug information for each IRQ */
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	if (!kbase_io_is_gpu_powered(kbdev)) {
		dev_info(kbdev->dev, "Bypass IRQ register dump because GPU is power off");
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev,
			MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"Bypass IRQ register dump because GPU is power off\n");
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	} else {
		dev_info(kbdev->dev, "GPU_IRQ_RAWSTAT=0x%08x, GPU_IRQ_MASK=0x%08x, GPU_IRQ_STATUS=0x%08x",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_RAWSTAT)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_MASK)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_STATUS)));
		dev_info(kbdev->dev, "JOB_IRQ_RAWSTAT=0x%08x, JOB_IRQ_MASK=0x%08x, JOB_IRQ_STATUS=0x%08x",
			kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_RAWSTAT)),
			kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_MASK)),
			kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_STATUS)));
		dev_info(kbdev->dev, "MMU_IRQ_RAWSTAT=0x%08x, MMU_IRQ_MASK=0x%08x, MMU_IRQ_STATUS=0x%08x",
			kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_RAWSTAT)),
			kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_MASK)),
			kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_STATUS)));
		if (kbdev->pm.backend.has_host_pwr_iface) {
			dev_info(kbdev->dev, "PWR_IRQ_RAWSTAT=0x%08x, PWR_IRQ_MASK=0x%08x, PWR_IRQ_STATUS=0x%08x\n",
				kbase_reg_read32(kbdev, HOST_POWER_ENUM(PWR_IRQ_RAWSTAT)),
				kbase_reg_read32(kbdev, HOST_POWER_ENUM(PWR_IRQ_MASK)),
				kbase_reg_read32(kbdev, HOST_POWER_ENUM(PWR_IRQ_STATUS)));
		}
#if IS_ENABLED(CONFIG_MALI_MTK_LOG_BUFFER)
		mtk_logbuffer_type_print(kbdev,
			MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"GPU_IRQ_RAWSTAT=0x%08x, GPU_IRQ_MASK=0x%08x, GPU_IRQ_STATUS=0x%08x\n",
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_RAWSTAT)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_MASK)),
			kbase_reg_read32(kbdev, GPU_CONTROL_ENUM(GPU_IRQ_STATUS)));
		mtk_logbuffer_type_print(kbdev,
			MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"JOB_IRQ_RAWSTAT=0x%08x, JOB_IRQ_MASK=0x%08x, JOB_IRQ_STATUS=0x%08x\n",
			kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_RAWSTAT)),
			kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_MASK)),
			kbase_reg_read32(kbdev, JOB_CONTROL_ENUM(JOB_IRQ_STATUS)));
		mtk_logbuffer_type_print(kbdev,
			MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
			"MMU_IRQ_RAWSTAT=0x%08x, MMU_IRQ_MASK=0x%08x, MMU_IRQ_STATUS=0x%08x\n",
			kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_RAWSTAT)),
			kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_MASK)),
			kbase_reg_read32(kbdev, MMU_CONTROL_ENUM(IRQ_STATUS)));
		if (kbdev->pm.backend.has_host_pwr_iface) {
			mtk_logbuffer_type_print(kbdev,
				MTK_LOGBUFFER_TYPE_CRITICAL | MTK_LOGBUFFER_TYPE_EXCEPTION,
				"PWR_IRQ_RAWSTAT=0x%08x, PWR_IRQ_MASK=0x%08x, PWR_IRQ_STATUS=0x%08x\n",
				kbase_reg_read32(kbdev, HOST_POWER_ENUM(PWR_IRQ_RAWSTAT)),
				kbase_reg_read32(kbdev, HOST_POWER_ENUM(PWR_IRQ_MASK)),
				kbase_reg_read32(kbdev, HOST_POWER_ENUM(PWR_IRQ_STATUS)));
		}
#endif /* CONFIG_MALI_MTK_LOG_BUFFER */
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}
