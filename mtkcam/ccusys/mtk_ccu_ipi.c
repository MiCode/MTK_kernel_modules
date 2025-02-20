// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2021 MediaTek Inc.

#include <linux/clocksource.h>
#include <linux/remoteproc/mtk_ccu.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/module.h>

#include "mtk_ccu_common.h"
#include "mtk_ccu_isp71.h"
#if defined(SECURE_CCU)
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <linux/arm-smccc.h>
#endif

#define MTK_CCU_IPI_TAG "[ccu_ipc]"
#define LOG_DBG_IPI(format, args...)
#define LOG_DBG_IPI_MUST(format, args...) \
	pr_info(MTK_CCU_IPI_TAG "[%s] " format, __func__, ##args)

#define MTK_CCU_IPC_NO_ACK 0x66669999
#define MTK_CCU_IPC_CMD_TIMEOUT_SPEC 30 //3000us = 3ms
#define MTK_CCU_IPC_IOBUF_CAPACITY (1024*4-16-64-64)
#define MTK_CCU_IPC_IOBUF_CAPACITY_COMPACT (68-2*4)

struct __aligned(8) shared_buf_map
{
	/*** from CCU->APMCU ***/
	uint32_t ipc_data_addr_ccu;
	uint32_t ipc_data_base_offset;
	uint32_t ipc_base_offset;
	uint32_t ipc_data_size;
};

static int mtk_ccu_copyCmdInData(struct mtk_ccu *ccu,
	void *inDataPtr, uint32_t inDataSize)
{
	uint32_t i;
	uint32_t *realIoBuf = (uint32_t *)ccu->ccu_ipc.ipcDataPtr;

	if (inDataSize == 0)
		return 0;

	if (inDataSize > ccu->ccu_ipc.ipcDataSize) {
		dev_err(ccu->dev, "failed: data length over capacity(%d < %d)\n",
			ccu->ccu_ipc.ipcDataSize, inDataSize);
		return -EINVAL;
	}

	for (i = 0; i < (inDataSize/4) ; i++)
		writel(((uint32_t *)inDataPtr)[i], &realIoBuf[i]);

	return 0;
}

static int mtk_ccu_copyCmdOutData(struct mtk_ccu *ccu,
	void *outDataPtr, uint32_t outDataSize)
{
	uint32_t i;
	uint32_t *realIoBuf = (uint32_t *)ccu->ccu_ipc.ipcDataPtr;

	if (outDataSize == 0)
		return 0;

	if (outDataSize > ccu->ccu_ipc.ipcDataSize) {
		dev_err(ccu->dev, "failed: data length over capacity(%d < %d)\n",
			ccu->ccu_ipc.ipcDataSize, outDataSize);
		return -EINVAL;
	}

	for (i = 0; i < (outDataSize/4); i++)
		writel(realIoBuf[i], &((uint32_t *)outDataPtr)[i]);

	return 0;
}

static int mtk_ccu_rproc_ipc_trigger(struct mtk_ccu *ccu,
	struct mtk_ccu_msg *msg)
{
	uint32_t loop_cnt = 0;
	bool ackValue = 0;
	bool timeout = false;
	uint32_t write_cnt = 0;
	uint32_t read_cnt = 0;

	/* LOG_DBG_IPI("+, ftype(%d), msg_id(%d)", msg->feature_type, msg->msg_id); */

	if (ccu->ccu_ipc.is_initialized == false) {
		dev_err(ccu->dev, "IPC not initialized, invalid operation.");
		return -EINVAL;
	}

	write_cnt = readl(&ccu->ccu_ipc.ccuIpcPtr->write_cnt);
	read_cnt = readl(&ccu->ccu_ipc.ccuIpcPtr->read_cnt);

	//since ipc is synchronized, check no previous ipc
	if (read_cnt != write_cnt) {
		dev_err(ccu->dev,
			"CCU IPC violation, rcnt:%d, wcnt:%d, tout_fid:%d, tout_mid:%d",
			read_cnt, write_cnt, ccu->ipc_tout_fid, ccu->ipc_tout_mid);
		if (ccu->compact_ipc)
			dev_err(ccu->dev, "r_assert:0x%x, i28:0x%x, i29:0x%x, i30:0x%x, i31:0x%x",
				read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG20),
				read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG28),
				read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG29),
				read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG30),
				read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG31));
		else
			dev_err(ccu->dev, "r_assert:0x%x, i28:0x%x, i29:0x%x, i30:0x%x, i31:0x%x",
				readl(ccu->ccu_spare_base + MTK_CCU_SPARE_REG20),
				readl(ccu->ccu_spare_base + MTK_CCU_SPARE_REG28),
				readl(ccu->ccu_spare_base + MTK_CCU_SPARE_REG29),
				readl(ccu->ccu_spare_base + MTK_CCU_SPARE_REG30),
				readl(ccu->ccu_spare_base + MTK_CCU_SPARE_REG31));
		return -EINVAL;
	}

	writel(MTK_CCU_IPC_NO_ACK, &ccu->ccu_ipc.ccuIpcPtr->ack);
	write_cnt = readl(&ccu->ccu_ipc.ccuIpcPtr->write_cnt);
	writel(write_cnt + 1, &ccu->ccu_ipc.ccuIpcPtr->write_cnt);

	if (ccu->compact_ipc) {
		struct mtk_ccu_msg_compact cmsg;

		cmsg.feature_type = msg->feature_type;
		cmsg.msg_id = msg->msg_id;
		cmsg.in_data_ptr = msg->in_data_ptr;
		cmsg.tg_info = msg->tg_info;
		cmsg.sensor_idx = msg->sensor_idx;
		mtk_ccu_memcpy(&ccu->ccu_ipc.ccuIpcPtr->msg, &cmsg,
			sizeof(struct mtk_ccu_msg_compact));
	} else {
		writel(msg->feature_type, &ccu->ccu_ipc.ccuIpcPtr->msg.feature_type);
		writel(msg->msg_id, &ccu->ccu_ipc.ccuIpcPtr->msg.msg_id);
		writel(msg->in_data_ptr, &ccu->ccu_ipc.ccuIpcPtr->msg.in_data_ptr);
		writel(msg->tg_info, &ccu->ccu_ipc.ccuIpcPtr->msg.tg_info);
		writel(msg->sensor_idx, &ccu->ccu_ipc.ccuIpcPtr->msg.sensor_idx);
	}

	writel(1, ccu->ccu_ipc.ccuIntTrigPtr);
	while (readl(&ccu->ccu_ipc.ccuIpcPtr->ack) == MTK_CCU_IPC_NO_ACK) {
		loop_cnt++;
		udelay(100);
		if (loop_cnt > MTK_CCU_IPC_CMD_TIMEOUT_SPEC) {
			timeout = true;
			break;
		}
	}

	if (timeout) {
		dev_err(ccu->dev,
			"CCU IPC timeout, ft(%d), mid(%d), ack(%d), cnt(%d)\n",
			msg->feature_type, msg->msg_id, ackValue, loop_cnt);
		if (ccu->compact_ipc)
			dev_err(ccu->dev, "r_assert:0x%x, i28:0x%x, i29:0x%x, i30:0x%x, i31:0x%x",
				read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG20),
				read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG28),
				read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG29),
				read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG30),
				read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG31));
		else
			dev_err(ccu->dev, "r_assert:0x%x, i28:0x%x, i29:0x%x, i30:0x%x, i31:0x%x",
				readl(ccu->ccu_spare_base + MTK_CCU_SPARE_REG20),
				readl(ccu->ccu_spare_base + MTK_CCU_SPARE_REG28),
				readl(ccu->ccu_spare_base + MTK_CCU_SPARE_REG29),
				readl(ccu->ccu_spare_base + MTK_CCU_SPARE_REG30),
				readl(ccu->ccu_spare_base + MTK_CCU_SPARE_REG31));
		ccu->ipc_tout_fid = msg->feature_type;
		ccu->ipc_tout_mid = msg->msg_id;
		return -ETIMEDOUT;
	}

	/*
	 * LOG_DBG_IPI("-, ftype(%d), msg_id(%d), loop_cnt(%d), ackValue(%d)",
	 *	msg->feature_type, msg->msg_id, loop_cnt, ackValue);
	 */

	return 0;
}

static int mtk_ccu_mb_rx(struct mtk_ccu *ccu,
	struct mtk_ccu_msg *task)
{
	int ret = 0;
	uint32_t rear;
	uint32_t front;
	uint32_t next;

	if (!spin_trylock(&ccu->ccu_poweron_lock))
		return 0;

	if (!ccu->poweron) {
		spin_unlock(&ccu->ccu_poweron_lock);
		return 0;
	}

	rear = readl(&ccu->mb->rear);
	front = readl(&ccu->mb->front);

	/*Check if queue is empty*/
	/*empty when front=rear*/
	if (rear != front) {
		/*modulus add: rear+1 = rear+1 % CCU_MAILBOX_QUEUE_SIZE*/
		next =
		(ccu->mb->front + 1) & (ccu_mailbox_max - 1);

		if (ccu->compact_ipc) {
			struct mtk_ccu_msg_compact cmsg = {0};

			mtk_ccu_readl(&cmsg, &(ccu->mb_compact->queue[next]),
				sizeof(struct mtk_ccu_msg_compact));
			task->feature_type = cmsg.feature_type;
			task->msg_id = cmsg.msg_id;
			task->in_data_ptr = (uint32_t)(int32_t)(int16_t)(cmsg.in_data_ptr);
			task->inDataSize = cmsg.inDataSize;
			task->tg_info = cmsg.tg_info;
			task->sensor_idx = cmsg.sensor_idx;
		} else
			memcpy(task, &(ccu->mb->queue[next]),
				sizeof(struct mtk_ccu_msg));
		ccu->mb->front = next;

		if (ccu->compact_ipc)
			LOG_DBG_IPI_MUST(
			"[%u] received cmd: f(%d), r(%d), cmd(%d), in(%x)\n",
			(uint32_t)arch_timer_read_counter(),
			ccu->mb->front,
			ccu->mb->rear,
			task->msg_id,
			task->in_data_ptr);
		else
			LOG_DBG_IPI_MUST(
			"[%u] received cmd: f(%d), r(%d), cmd(%d), in(%x)\n",
			(uint32_t)arch_timer_read_counter(),
			ccu->mb->front,
			ccu->mb->rear,
			ccu->mb->queue[next].msg_id,
			ccu->mb->queue[next].in_data_ptr);

		ret = rear - front + 1;
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
		LOG_DBG_IPI_MUST("fs[%d,%d,%d],sc[%d,%d,%d],cam[%d,%d,%d],senif[%d,%d,%d]\n",
			ccu->bootcnt[0][0].counter, ccu->bootcnt[0][1].counter,
			ccu->bootcnt[0][2].counter, ccu->bootcnt[1][0].counter,
			ccu->bootcnt[1][1].counter, ccu->bootcnt[1][2].counter,
			ccu->bootcnt[2][0].counter, ccu->bootcnt[2][1].counter,
			ccu->bootcnt[2][2].counter, ccu->bootcnt[3][0].counter,
			ccu->bootcnt[3][1].counter, ccu->bootcnt[3][2].counter);
		LOG_DBG_IPI_MUST("img[%d,%d,%d],icmdq[%d,%d,%d],dvfs[%d,%d,%d],gce[%d,%d,%d]\n",
			ccu->bootcnt[4][0].counter, ccu->bootcnt[4][1].counter,
			ccu->bootcnt[4][2].counter, ccu->bootcnt[5][0].counter,
			ccu->bootcnt[5][1].counter, ccu->bootcnt[5][2].counter,
			ccu->bootcnt[6][0].counter, ccu->bootcnt[6][1].counter,
			ccu->bootcnt[6][2].counter, ccu->bootcnt[7][0].counter,
			ccu->bootcnt[7][1].counter, ccu->bootcnt[7][2].counter);
#endif
	} else
		ret = 0;

	spin_unlock(&ccu->ccu_poweron_lock);

	return ret;
}

irqreturn_t mtk_ccu_isr_handler(int irq, void *priv)
{
	int mb_cnt;
	static struct mtk_ccu_msg msg;
	struct mtk_ccu *ccu = (struct mtk_ccu *)priv;
	mtk_ccu_ipc_handle_t handler;
	void *handler_priv;
#if defined(SECURE_CCU)
	struct arm_smccc_res res;
#endif

	if (!spin_trylock(&ccu->ccu_poweron_lock)) {
		LOG_DBG_IPI_MUST("trylock failed.\n");
		goto ISR_EXIT;
	}

	if (!ccu->poweron) {
		LOG_DBG_IPI_MUST("ccu->poweron false.\n");
#ifdef REQUEST_IRQ_IN_INIT
		if (spin_trylock(&ccu->ccu_irq_lock)) {
			if (ccu->disirq) {
				ccu->disirq = false;
				spin_unlock(&ccu->ccu_irq_lock);
				disable_irq_nosync(ccu->irq_num);
			} else
				spin_unlock(&ccu->ccu_irq_lock);
		}
#endif
		spin_unlock(&ccu->ccu_poweron_lock);
		goto ISR_EXIT;
	}

	/*clear interrupt status*/
#if defined(SECURE_CCU)
	if (ccu->ccu_version >= CCU_VER_ISP8)
		writel(1, ccu->ccu_exch_base + MTK_CCU_INT_CLR_ISP8);
	else if (ccu->ccu_version > CCU_VER_ISP71)
		writel(1, ccu->ccu_base + MTK_CCU_INT_CLR_EXCH);
	else {
#ifdef CONFIG_ARM64
		arm_smccc_smc(MTK_SIP_KERNEL_CCU_CONTROL, (u64) CCU_SMC_REQ_CLEAR_INT,
				(u64)1, 0, 0, 0, 0, 0, &res);
#endif
#ifdef CONFIG_ARM_PSCI
		arm_smccc_smc(MTK_SIP_KERNEL_CCU_CONTROL, (u32) CCU_SMC_REQ_CLEAR_INT,
				(u32)1, 0, 0, 0, 0, 0, &res);
#endif
	}
#else
	writel(0xFF, ccu->ccu_base + MTK_CCU_INT_CLR);
	// int_st = readl(ccu->ccu_base + MTK_CCU_INT_ST);
#endif

	spin_unlock(&ccu->ccu_poweron_lock);

	while (1) {
		mb_cnt = mtk_ccu_mb_rx(ccu, &msg);
		if (mb_cnt == 0) {
			if (spin_trylock(&ccu->ccu_irq_lock)) {
				if (ccu->disirq) {
					ccu->disirq = false;
					spin_unlock(&ccu->ccu_irq_lock);
					disable_irq_nosync(ccu->irq_num);
				} else
					spin_unlock(&ccu->ccu_irq_lock);
			}
			goto ISR_EXIT;
		}

		if (msg.msg_id >= MTK_CCU_MSG_TO_APMCU_MAX)
			continue;

		mutex_lock(&ccu->ipc_desc_lock);
		handler = ccu->ipc_desc[msg.msg_id].handler;
		handler_priv = ccu->ipc_desc[msg.msg_id].priv;
		mutex_unlock(&ccu->ipc_desc_lock);
		if (handler != NULL)
			handler(msg.in_data_ptr, msg.inDataSize, handler_priv);
	}

ISR_EXIT:

	return IRQ_HANDLED;
}

void mtk_ccu_rproc_ipc_init(struct mtk_ccu *ccu)
{
	uint8_t *dmbase = (uint8_t *)ccu->dmem_base;
	uint8_t *ctrlbase = (ccu->ccu_version >= CCU_VER_ISP8) ?
		(uint8_t *)ccu->ccu_exch_base : (uint8_t *)ccu->ccu_base;
	uint32_t trig_reg_offset;
	struct shared_buf_map *sb_map_ptr =
		(struct shared_buf_map *)(dmbase + MTK_CCU_SHARED_BUF_OFFSET);
	uint8_t *ipc_base = (ccu->compact_ipc) ?
		((ccu->ccu_version == CCU_VER_ISP8) ? ccu->ccu_exch_base : ccu->ccu_base) :
		((ccu->ccu_version >= CCU_VER_ISP8L) ? ccu->ccu_exch_base : ccu->dmem_base);

	if (ccu->ccu_version >= CCU_VER_ISP8)
		trig_reg_offset = MTK_CCU_INT_TRG_ISP8;
	else if ((ccu->ccu_version == CCU_VER_ISP7SP) || (ccu->ccu_version == CCU_VER_ISP7SPL))
		trig_reg_offset = MTK_CCU_INT_TRG_ISP7SP;
	else
		trig_reg_offset = MTK_CCU_INT_TRG;
	ccu->ccu_ipc.ccuIntTrigPtr = (uint32_t *)(ctrlbase + trig_reg_offset);
	ccu->ccu_ipc.ccuIpcPtr =
		(struct ap2ccu_ipc *)(ipc_base + sb_map_ptr->ipc_base_offset);
	ccu->ccu_ipc.ipcDataPtr = ipc_base + sb_map_ptr->ipc_data_base_offset;
	ccu->ccu_ipc.ipcDataAddrCcu = sb_map_ptr->ipc_data_addr_ccu;
	ccu->ccu_ipc.ipcDataSize = sb_map_ptr->ipc_data_size;
	if (ccu->ccu_ipc.ipcDataSize == 0)
		ccu->ccu_ipc.ipcDataSize = (ccu->compact_ipc) ?
		MTK_CCU_IPC_IOBUF_CAPACITY_COMPACT : MTK_CCU_IPC_IOBUF_CAPACITY;
	ccu->ccu_ipc.is_initialized = true;
	LOG_DBG_IPI("IPC max size %d bytes", ccu->ccu_ipc.ipcDataSize);
}

void mtk_ccu_rproc_ipc_uninit(struct mtk_ccu *ccu)
{
	if (!ccu)
		return;

	ccu->ccu_ipc.is_initialized = false;
}

int mtk_ccu_ipc_register(struct platform_device *pdev,
	enum mtk_ccu_to_ap_msg_id id,
	mtk_ccu_ipc_handle_t handle, void *priv)
{
	struct mtk_ccu *ccu = platform_get_drvdata(pdev);

	if (!ccu) {
		dev_err(&pdev->dev, "ccu device is not ready\n");
		return -EPROBE_DEFER;
	}

	if (WARN_ON(id < 0) || WARN_ON(id >= MTK_CCU_MSG_TO_APMCU_MAX) ||
		WARN_ON(handle == NULL))
		return -EINVAL;

	mutex_lock(&ccu->ipc_desc_lock);
	ccu->ipc_desc[id].handler = handle;
	ccu->ipc_desc[id].priv = priv;
	mutex_unlock(&ccu->ipc_desc_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_ccu_ipc_register);

void mtk_ccu_ipc_unregister(struct platform_device *pdev,
	enum mtk_ccu_to_ap_msg_id id)
{
	struct mtk_ccu *ccu = platform_get_drvdata(pdev);

	if (!ccu)
		return;
	if (WARN_ON(id < 0) || WARN_ON(id >= MTK_CCU_MSG_TO_APMCU_MAX))
		return;
	mutex_lock(&ccu->ipc_desc_lock);
	ccu->ipc_desc[id].handler = NULL;
	ccu->ipc_desc[id].priv = NULL;
	mutex_unlock(&ccu->ipc_desc_lock);
}
EXPORT_SYMBOL_GPL(mtk_ccu_ipc_unregister);

int mtk_ccu_rproc_ipc_send(struct platform_device *pdev,
	enum mtk_ccu_feature_type featureType,
	uint32_t msgId, void *inDataPtr, uint32_t inDataSize)
{
	struct mtk_ccu_msg msg = {0};
	uint32_t ret;
	struct mtk_ccu *ccu;

	if (!pdev)
		return -EINVAL;

	ccu = platform_get_drvdata(pdev);

	if (!ccu) {
		dev_err(&pdev->dev, "ccu device is not ready\n");
		return -EPROBE_DEFER;
	}

	if (!ccu->ccu_ipc.is_initialized) {
		dev_err(ccu->dev, "sendCcuCommnadIpc not init, ft (%d) msgId(%d)",
			featureType, msgId);
		return -EINVAL;
	}

	if (featureType == MTK_CCU_FEATURE_SYSCTRL)
		LOG_DBG_IPI_MUST("[%u] ft(%d), msgId(%d)\n",
			(uint32_t)arch_timer_read_counter(), featureType, msgId);

	if ((inDataSize) && (!inDataPtr)) {
		dev_err(ccu->dev, "inDataPtr is NULL\n");
		return -EINVAL;
	}

	spin_lock(&ccu->ipc_send_lock);

	//Check if need input data
	ret = mtk_ccu_copyCmdInData(ccu, inDataPtr, inDataSize);
	if (ret != 0) {
		spin_unlock(&ccu->ipc_send_lock);
		return -EINVAL;
	}

	msg.feature_type = featureType;
	msg.msg_id = msgId;
	msg.in_data_ptr = ccu->ccu_ipc.ipcDataAddrCcu;
	msg.inDataSize = inDataSize;
	ret = mtk_ccu_rproc_ipc_trigger(ccu, &msg);
	if (ret != 0) {
		dev_err(ccu->dev, "sendCcuCommnadIpc failed, msgId(%d)", msgId);
		spin_unlock(&ccu->ipc_send_lock);
		return -EINVAL;
	}

	//check if need to copy output data
	ret = mtk_ccu_copyCmdOutData(ccu, inDataPtr, inDataSize);

	spin_unlock(&ccu->ipc_send_lock);

	/* LOG_DBG_IPI("-"); */
	return ret;
}
EXPORT_SYMBOL_GPL(mtk_ccu_rproc_ipc_send);

int mtk_ccu_rproc_get_inforeg(struct platform_device *pdev,
	uint32_t regno, uint32_t *data)
{
	struct mtk_ccu *ccu;

	if ((!pdev) || (!data))
		return -EINVAL;

	ccu = platform_get_drvdata(pdev);

	if (!ccu)
		return -EPROBE_DEFER;

	if ((regno < 28) || (regno > 31))
		return -EINVAL;

	if (!spin_trylock(&ccu->ccu_poweron_lock))
		return -EINVAL;

	if (!ccu->poweron) {
		spin_unlock(&ccu->ccu_poweron_lock);
		return -EINVAL;
	}

	regno <<= 2;
	if (ccu->compact_ipc)
		*data = read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG00 + regno);
	else
		*data = readl(ccu->ccu_base + MTK_CCU_SPARE_REG00 + regno);

	spin_unlock(&ccu->ccu_poweron_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_ccu_rproc_get_inforeg);

MODULE_DESCRIPTION("MTK CCU Rproc Driver");
MODULE_LICENSE("GPL v2");
