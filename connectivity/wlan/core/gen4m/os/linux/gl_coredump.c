// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "gl_coredump.h"
#if CFG_SUPPORT_CONNINFRA
#include "connsys_debug_utility.h"
#elif IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#include "connv3_debug_utility.h"
#endif

#ifdef MT6639
#include "coda/mt6639/bcrm_on_pwr_wrapper_u_bcrm_on_pwr_bcrm.h"
#endif

#if CFG_WIFI_SECURITY_COREDUMP
#include "mt6653.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
#if CFG_SUPPORT_CONNINFRA
static int coredump_check_reg_readable(void);
#endif
static int file_ops_coredump_open(struct inode *inode, struct file *file);
static int file_ops_coredump_release(struct inode *inode, struct file *file);
static ssize_t file_ops_coredump_read(struct file *filp, char __user *buf,
	size_t count, loff_t *f_pos);
#if CFG_WIFI_COREDUMP_SKIP_BY_REQUEST
static ssize_t file_ops_coredump_write(struct file *filp,
	const char __user *buf, size_t count, loff_t *f_pos);
#endif

#define PRINT_BYTES_PER_LOOP	512
#define PRINT_LONG_STR_MSG(__msg, __size) \
{ \
	uint8_t buf[PRINT_BYTES_PER_LOOP + 1]; \
	uint8_t *pos = __msg, *end = __msg + __size; \
	while (pos < end) { \
		uint32_t print_sz = (end - pos) > PRINT_BYTES_PER_LOOP ? \
			PRINT_BYTES_PER_LOOP : (end - pos); \
		kalMemCopy(buf, pos, print_sz); \
		buf[print_sz] = '\0'; \
		LOG_FUNC("%s\n", buf); \
		pos += print_sz; \
	} \
}

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
struct coredump_ctx g_coredump_ctx;

#if CFG_SUPPORT_CONNINFRA
struct coredump_event_cb g_wifi_coredump_cb = {
	.reg_readable = coredump_check_reg_readable,
	.poll_cpupcr = NULL,
};
#endif

const struct file_operations g_coredump_fops = {
	.open = file_ops_coredump_open,
	.release = file_ops_coredump_release,
	.read = file_ops_coredump_read,
#if CFG_WIFI_COREDUMP_SKIP_BY_REQUEST
	.write = file_ops_coredump_write,
#endif
};

#if CFG_WIFI_COREDUMP_SKIP_BY_REQUEST
static uint8_t  fgIsCoredumpSkipped;
#endif

#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
static enum COREDUMP_DFD_FSM_STATE g_DfdCoredumpState;
#endif
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#if CFG_SUPPORT_CONNINFRA
static int coredump_check_reg_readable(void)
{
	struct coredump_ctx *ctx = &g_coredump_ctx;
	int ret = 1;

	if (conninfra_reg_readable() == 0) {
		DBGLOG(INIT, ERROR,
			"conninfra_reg_readable check failed.\n");
		ret = 0;
	}

	if (ctx->fn_check_bus_hang) {
		if (ctx->fn_check_bus_hang(NULL, 0) != 0) {
			DBGLOG(INIT, ERROR, "bus check failed.\n");
			ret = 0;
		}
	}

	return ret;
}
#endif

#if CFG_WIFI_COREDUMP_SKIP_BY_REQUEST
static uint8_t get_coredump_skipped(void)
{
	DBGLOG(INIT, LOUD, "get skipped:%d\n", fgIsCoredumpSkipped);
	return fgIsCoredumpSkipped;
}

static void update_coredump_skipped(uint8_t skipped)
{
	fgIsCoredumpSkipped = skipped;
	DBGLOG(INIT, INFO, "set skipped:%d\n", fgIsCoredumpSkipped);
}
#endif

static int file_ops_coredump_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int file_ops_coredump_release(struct inode *inode, struct file *file)
{
	return 0;
}

#if CFG_WIFI_COREDUMP_SKIP_BY_REQUEST
static ssize_t file_ops_coredump_write(struct file *filp,
	const char __user *buf, size_t count, loff_t *f_pos)
{
	int8_t skip_buf[2] = { 0 };
	uint32_t copy_size = 0;
	ssize_t ret = 0;

	if (count <= 0) {
		DBGLOG(INIT, ERROR, "coredump_write invalid param\n");
		ret = -EINVAL;
		goto exit;
	}

	copy_size = (sizeof(skip_buf) - 1) < (uint32_t) count ?
		(sizeof(skip_buf) - 1) : (uint32_t) count;
	if (copy_from_user(skip_buf, buf, copy_size)) {
		ret = -EINVAL;
		goto exit;
	}

	ret = copy_size;
	if (skip_buf[0] == '1') {
		DBGLOG(INIT, INFO, "enable coredump\n");
		update_coredump_skipped(0);
	} else if (skip_buf[0] == '0') {
		DBGLOG(INIT, INFO, "disable coredump\n");
		update_coredump_skipped(1);
	}

exit:
	return ret;
}
#endif

static ssize_t file_ops_coredump_read(struct file *filp, char __user *buf,
	size_t count, loff_t *f_pos)
{
	struct mt66xx_chip_info *chip_info;
	uint8_t *tmp_buf = NULL;
	ssize_t ret = 0;
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	struct coredump_ctx *ctx = &g_coredump_ctx;
	struct GLUE_INFO *prGlueInfo = ctx->priv;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct HIF_MEM_OPS *prMemOps = &prHifInfo->rMemOps;
	struct HIF_MEM *prMem = NULL;
	void *emi2_buf = NULL;
	uint8_t *prEmi2Address = NULL;
	uint8_t uIdx = 0;
#endif

	glGetChipInfo((void **)&chip_info);
	if (!chip_info) {
		DBGLOG(INIT, ERROR, "chip info is NULL\n");
		ret = -EINVAL;
		goto exit;
	}

	if (count != (chip_info->rEmiInfo.coredump_size+
		chip_info->rEmiInfo.coredump2_size)) {
		DBGLOG(INIT, ERROR,
			"coredump size mismatch (%zu %u %u)\n",
			count,
			chip_info->rEmiInfo.coredump_size,
			chip_info->rEmiInfo.coredump2_size);
		ret = -EINVAL;
		goto exit;
	}

	tmp_buf = kalMemAlloc(count, VIR_MEM_TYPE);
	if (tmp_buf == NULL) {
		DBGLOG(INIT, ERROR,
			"buffer(%u) alloc failed\n",
			count);
		ret = -ENOMEM;
		goto exit;
	}
	kalMemZero(tmp_buf, count);

	/* coredump 1 */
	if (emi_mem_read(chip_info, 0,
			 tmp_buf,
			 chip_info->rEmiInfo.coredump_size)) {
		DBGLOG(INIT, ERROR,
			"emi read failed.\n");
		ret = -EFAULT;
		goto exit;
	}

	/* coredump 2 */
	if (chip_info->rEmiInfo.coredump2_size == 0)
		goto copy_to_user;

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	emi2_buf = tmp_buf+chip_info->rEmiInfo.coredump_size;

	if (prMemOps->getWifiMiscRsvEmi && chip_info->rsvMemWiFiMisc) {
		for (uIdx = 0; uIdx < chip_info->rsvMemWiFiMiscSize; uIdx++) {
			DBGLOG(INIT, LOUD, "Copy %d (%d)\n",
				uIdx, chip_info->rsvMemWiFiMisc[uIdx].size);

			prMem = prMemOps->getWifiMiscRsvEmi(chip_info, uIdx);
			if (prMem == NULL) {
				DBGLOG(NIC, INFO, "not support EMI2\n");
				goto copy_to_user;
			}

			prEmi2Address = (uint8_t *)prMem->va;
			if (prEmi2Address == NULL) {
				DBGLOG(NIC, INFO,
					"[%d] get EMI Address is NULL\n", uIdx);
				continue;
			}
			kalMemCopyFromIo(emi2_buf, prEmi2Address,
				chip_info->rsvMemWiFiMisc[uIdx].size);
			emi2_buf += chip_info->rsvMemWiFiMisc[uIdx].size;
		}
	} else
#endif
	{
		DBGLOG(INIT, INFO, "emi2 read failed.\n");
		goto copy_to_user;
	}

copy_to_user:
	ret = simple_read_from_buffer(buf, count, f_pos, tmp_buf, count);
	DBGLOG(INIT, INFO, "ret: %zd\n", ret);

exit:
	if (tmp_buf)
		kalMemFree(tmp_buf, VIR_MEM_TYPE, count);

	return ret;
}

void coredump_register_bushang_chk_cb(bushang_chk_func_cb cb)
{
	struct coredump_ctx *ctx = &g_coredump_ctx;

	ctx->fn_check_bus_hang = cb;
}

static int coredump_add_cdev(struct coredump_ctx *ctx)
{
	int ret = 0;

	ret = alloc_chrdev_region(&ctx->devno, 0, 1,
		COREDUMP_WIFI_INF_NAME);
	if (ret) {
		DBGLOG(INIT, ERROR,
			"alloc_chrdev_region failed, ret: %d\n",
			ret);
		goto return_fn;
	}

	cdev_init(&ctx->cdev, &g_coredump_fops);
	ctx->cdev.owner = THIS_MODULE;

	ret = cdev_add(&ctx->cdev, ctx->devno, COREDUMP_WIFI_DEV_NUM);
	if (ret) {
		DBGLOG(INIT, ERROR,
			"cdev_add failed, ret: %d\n",
			ret);
		goto unregister_chrdev_region;
	}

	ctx->driver_class = KAL_CLASS_CREATE(COREDUMP_WIFI_INF_NAME);
	if (IS_ERR(ctx->driver_class)) {
		DBGLOG(INIT, ERROR,
			"class_create failed, ret: %d\n",
			ret);
		ret = PTR_ERR(ctx->driver_class);
		goto cdev_del;
	}

	ctx->class_dev = device_create(ctx->driver_class,
		NULL, ctx->devno, NULL, COREDUMP_WIFI_INF_NAME);
	if (IS_ERR(ctx->class_dev)) {
		ret = PTR_ERR(ctx->class_dev);
		DBGLOG(INIT, ERROR,
			"class_device_create failed, ret: %d\n",
			ret);
		goto class_destroy;
	}

	goto return_fn;

class_destroy:
	class_destroy(ctx->driver_class);
cdev_del:
	cdev_del(&ctx->cdev);
unregister_chrdev_region:
	unregister_chrdev_region(ctx->devno, COREDUMP_WIFI_DEV_NUM);
return_fn:
	if (ret)
		DBGLOG(INIT, ERROR, "ret: %d\n",
			ret);

	return ret;
}

static void coredump_del_cdev(struct coredump_ctx *ctx)
{
	device_destroy(ctx->driver_class, ctx->devno);
	class_destroy(ctx->driver_class);
	cdev_del(&ctx->cdev);
	unregister_chrdev_region(ctx->devno, COREDUMP_WIFI_DEV_NUM);
}

#if CFG_WIFI_SECURITY_COREDUMP
static void coredump_aes_drv_ctl_s2pcmd(struct ADAPTER *prAdapter,
	uint8_t s2p_type, uint32_t data, uint8_t RW)
{
	uint32_t bit_7_0, bit_15_8, bit_23_16, bit_31_24;

	bit_7_0 = (0x000000FF & data) >> 0;
	bit_15_8 = (0x0000FF00 & data) >> 8;
	bit_23_16 = (0x00FF0000 & data) >> 16;
	bit_31_24 = (0xFF000000 & data) >> 24;

	switch (s2p_type) {
	case S2P_CMD_AES_SRC:
	case S2P_CMD_AES_DST:
	case S2P_CMD_AES_LEN:
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_WR << 18) | (0x00 << 16) |
			(s2p_type << 8) | (bit_7_0));
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE, S2P_CMD_CLR_WR_BIT);
		DBGLOG(INIT, LOUD, "S2P composed cmd: %x\n",
			(S2P_CMD_WR << 18) | (0x00 << 16) |
			(s2p_type << 8) | (bit_7_0));
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_RD << 18) | (0x00 << 16) |
			(s2p_type << 8) | (bit_7_0));
		HAL_RMCR_RD(COREDUMP_DBG, prAdapter, S2P_CMD_RX_BASE, &bit_7_0);

		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_WR << 18) | (0x01 << 16) |
			(s2p_type << 8) | (bit_15_8));
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE, S2P_CMD_CLR_WR_BIT);
		DBGLOG(INIT, LOUD, "S2P composed cmd: %x\n",
			(S2P_CMD_WR << 18) | (0x01 << 16) |
			(s2p_type << 8) | (bit_15_8));
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_RD << 18) | (0x01 << 16) |
			(s2p_type << 8) | (bit_15_8));
		HAL_RMCR_RD(COREDUMP_DBG, prAdapter, S2P_CMD_RX_BASE,
			&bit_15_8);

		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_WR << 18) | (0x02 << 16) |
			(s2p_type << 8) | (bit_23_16));
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE, S2P_CMD_CLR_WR_BIT);
		DBGLOG(INIT, LOUD, "S2P composed cmd: %x\n",
			(S2P_CMD_WR << 18) | (0x02 << 16) |
			(s2p_type << 8) | (bit_23_16));
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_RD << 18) | (0x02 << 16) |
			(s2p_type << 8) | (bit_23_16));
		HAL_RMCR_RD(COREDUMP_DBG, prAdapter, S2P_CMD_RX_BASE,
			&bit_23_16);

		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_WR << 18) | (0x03 << 16) |
			(s2p_type << 8) | (bit_31_24));
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE, S2P_CMD_CLR_WR_BIT);
		DBGLOG(INIT, LOUD, "S2P composed cmd: %x\n",
			(S2P_CMD_WR << 18) | (0x03 << 16) |
			(s2p_type << 8) | (bit_31_24));
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_RD << 18) | (0x03 << 16) |
			(s2p_type << 8) | (bit_31_24));
		HAL_RMCR_RD(COREDUMP_DBG, prAdapter, S2P_CMD_RX_BASE,
			&bit_31_24);
		break;
	case S2P_CMD_SWDEF_AES_DRV_OWN:
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_WR << 18) | (0x00 << 16) |
			(S2P_CMD_AES_DRVOWN_TRIGGER << 8) | (bit_7_0));
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE, S2P_CMD_CLR_WR_BIT);
		DBGLOG(INIT, LOUD, "S2P composed cmd: %x\n",
			(S2P_CMD_WR << 18) | (0x00 << 16) |
			(S2P_CMD_AES_DRVOWN_TRIGGER << 8) | (bit_7_0));
		break;
	case S2P_CMD_SWDEF_AES_TRIGGER:
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_WR << 18) | (0x01 << 16) |
			(S2P_CMD_AES_DRVOWN_TRIGGER << 8) | (bit_7_0));
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE, S2P_CMD_CLR_WR_BIT);
		DBGLOG(INIT, LOUD, "S2P composed cmd: %x\n",
			(S2P_CMD_WR << 18) | (0x01 << 16) |
			(S2P_CMD_AES_DRVOWN_TRIGGER << 8) | (bit_7_0));
		break;
	}
}

static uint8_t coredump_aes_driver_control(struct GLUE_INFO *prGlueInfo,
	uint32_t src, uint32_t dest, uint32_t length)
{
#define SECURITY_COREDUMP_TIMEOUT_MS	1000
	struct ADAPTER *prAdapter = NULL;
	uint32_t aes_busy = 1;
	uint32_t u4CurrTick = 0;
	u_int8_t ret = TRUE, fgTimeout = FALSE;

	if (!prGlueInfo)
		return FALSE;

	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter)
		return FALSE;

	coredump_aes_drv_ctl_s2pcmd(prAdapter, S2P_CMD_AES_MAX_LEN,
		0x3, S2P_CMD_WR);
	coredump_aes_drv_ctl_s2pcmd(prAdapter, S2P_CMD_AES_SRC,
		src, S2P_CMD_WR);
	coredump_aes_drv_ctl_s2pcmd(prAdapter, S2P_CMD_AES_DST,
		dest, S2P_CMD_WR);
	coredump_aes_drv_ctl_s2pcmd(prAdapter, S2P_CMD_AES_LEN,
		length, S2P_CMD_WR);
	coredump_aes_drv_ctl_s2pcmd(prAdapter, S2P_CMD_SWDEF_AES_DRV_OWN,
		0x1, S2P_CMD_WR);
	coredump_aes_drv_ctl_s2pcmd(prAdapter, S2P_CMD_SWDEF_AES_TRIGGER,
		0x1, S2P_CMD_WR);

	/* clr TRIGGER */
	/* read out TRIGGER bits */
	HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
		(S2P_CMD_RD << 18) | (0x1 << 16) |
		(S2P_CMD_AES_DRVOWN_TRIGGER << 8) | 0x0);
	coredump_aes_drv_ctl_s2pcmd(prAdapter, S2P_CMD_SWDEF_AES_TRIGGER,
		0x0, S2P_CMD_WR);
	udelay(10);

	u4CurrTick = kalGetTimeTick();
	while (aes_busy != 0 && !fgTimeout) {
		/* read out busy bits */
		HAL_MCR_WR(prAdapter, S2P_CMD_TX_BASE,
			(S2P_CMD_RD << 18) | (0x0 << 16) |
			(S2P_CMD_AES_BUSY << 8) | 0x0);
		udelay(10);
		HAL_RMCR_RD(COREDUMP_DBG, prAdapter, S2P_CMD_RX_BASE,
			&aes_busy);
		fgTimeout = ((kalGetTimeTick() - u4CurrTick) >
			SECURITY_COREDUMP_TIMEOUT_MS)
			? TRUE : FALSE;
	}

	if (fgTimeout)
		DBGLOG(INIT, ERROR, "aes busy get timeout\n");

	return ret;
}
#endif

int wifi_coredump_init(void *priv)
{
	struct coredump_ctx *ctx = &g_coredump_ctx;
	struct mt66xx_chip_info *chip_info;
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	struct connv3_coredump_event_cb cb;
#endif
	int ret = 0;

	kalMemZero(ctx, sizeof(*ctx));

	ctx->priv = priv;
	glGetChipInfo((void **)&chip_info);
	if (!chip_info) {
		DBGLOG(INIT, ERROR, "chip info is NULL\n");
		ret = -EINVAL;
		goto exit;
	}

	ret = coredump_add_cdev(ctx);
	if (ret)
		goto exit;

#if CFG_SUPPORT_CONNINFRA
	ctx->handler = connsys_coredump_init(CONN_DEBUG_TYPE_WIFI,
		&g_wifi_coredump_cb);
	if (!ctx->handler) {
		DBGLOG(INIT, ERROR, "connsys_coredump_init init failed.\n");
		ret = -EINVAL;
		goto free_cdev;
	}
#elif IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	kalMemZero(&cb, sizeof(cb));
	kalSnprintf(cb.dev_node, CONNV3_EMI_MAP_DEV_NODE_SIZE,
		"%s%s", "/dev/", COREDUMP_WIFI_INF_NAME);
	cb.emi_size = chip_info->rEmiInfo.coredump_size;
	cb.mcif_emi_size = 0;
	cb.emi2_size = chip_info->rEmiInfo.coredump2_size;
	ctx->handler = connv3_coredump_init(CONNV3_DEBUG_TYPE_WIFI,
		&cb);
	if (!ctx->handler) {
		DBGLOG(INIT, ERROR, "connv3_coredump_init init failed.\n");
		ret = -EINVAL;
		goto free_cdev;
	}
#endif

	ctx->initialized = TRUE;
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
	g_DfdCoredumpState = COREDUMP_FSM_NOT_START;
#endif
#if CFG_WIFI_COREDUMP_SKIP_BY_REQUEST
	fgIsCoredumpSkipped = 0;
#endif
	goto exit;

free_cdev:
	coredump_del_cdev(ctx);
exit:
	return ret;
}

void wifi_coredump_deinit(void)
{
	struct coredump_ctx *ctx = &g_coredump_ctx;

	ctx->initialized = FALSE;

#if CFG_SUPPORT_CONNINFRA
	connsys_coredump_deinit(ctx->handler);
#elif IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	connv3_coredump_deinit(ctx->handler);
#endif

	ctx->handler = NULL;
	coredump_del_cdev(ctx);
	ctx->fn_check_bus_hang = NULL;
	ctx->priv = NULL;
}

static int __coredump_init_ctrl_blk(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info)
{
	struct coredump_mem *mem = &ctx->mem;
	struct GLUE_INFO *glue = ctx->priv;
	struct ctrl_blk_layout ctrl_blk;
	int ret = 0;

	ret = emi_mem_read(chip_info,
		COREDUMP_OFFSET_CTRL_BLOCK,
		&ctrl_blk,
		sizeof(ctrl_blk));
	if (ret) {
		DBGLOG(INIT, ERROR,
			"Read CTRL BLK failed.\n");
		goto exit;
	}

	mem->state = ctrl_blk.state;

	mem->print_buff_offset = COREDUMP_OFFSET_CTRL_BLOCK +
		COREDUMP_MAX_BLOCK_SZ_CTRL_BLOCK +
		COREDUMP_MAX_BLOCK_SZ_DEBUG_BUFF;
	if (ctrl_blk.print_buff_len > COREDUMP_MAX_BLOCK_SZ_PRINT_BUFF)
		mem->print_buff_len = COREDUMP_MAX_BLOCK_SZ_PRINT_BUFF;
	else
		mem->print_buff_len = ctrl_blk.print_buff_len;
	if (mem->print_buff_len)
		mem->print_buff_len += 1;

	mem->dump_buff_offset = mem->print_buff_offset +
		COREDUMP_MAX_BLOCK_SZ_PRINT_BUFF;
	if (ctrl_blk.dump_buff_len > COREDUMP_MAX_BLOCK_SZ_DUMP_BUFF)
		mem->dump_buff_len = COREDUMP_MAX_BLOCK_SZ_DUMP_BUFF;
	else
		mem->dump_buff_len = ctrl_blk.dump_buff_len;
	if (mem->dump_buff_len)
		mem->dump_buff_len += 1;

	mem->cr_region_offset = mem->dump_buff_offset +
		COREDUMP_MAX_BLOCK_SZ_DUMP_BUFF;
	if (ctrl_blk.cr_region_len > COREDUMP_MAX_BLOCK_SZ_CR_REGION)
		mem->cr_region_num = COREDUMP_MAX_BLOCK_SZ_CR_REGION;
	else
		mem->cr_region_num = ctrl_blk.cr_region_len;
	mem->cr_region_num = mem->cr_region_num >> 3;

	mem->mem_region_offset = COREDUMP_OFFSET_CTRL_BLOCK +
		sizeof(struct ctrl_blk_layout);
	if (ctrl_blk.mem_region_num > COREDUMP_MAX_BLOCK_SZ_MEM_REGION)
		mem->mem_region_num = COREDUMP_MAX_BLOCK_SZ_MEM_REGION;
	else
		mem->mem_region_num = ctrl_blk.mem_region_num;

	if (chip_info->checkbushang) {
		if (chip_info->checkbushang(glue->prAdapter, TRUE)) {
			DBGLOG(INIT, INFO, "Bus check failed.\n");
			mem->mem_region_num = 0;
		}
	}

#ifdef MT6639
	/*
	 * Disable way_en for CR region dump.
	 * Enable this in top pos patch to protect conninfra RGU.
	 */
	HAL_MCR_WR(glue->prAdapter,
		   BCRM_ON_PWR_WRAPPER_U_BCRM_ON_PWR_BCRM_conn_infra_off2on_apb_bus_u_p_d_n9_CTRL_0_ADDR,
		   0xFFFF);
#endif

	DBGLOG(INIT, INFO,
		"state: %d, pdcm[0x%x 0x%x 0x%x 0x%x]\n",
		ctrl_blk.state,
		ctrl_blk.print_buff_len,
		ctrl_blk.dump_buff_len,
		ctrl_blk.cr_region_len,
		ctrl_blk.mem_region_num);
	DBGLOG(INIT, INFO,
		"pdcm_offset[0x%x 0x%x 0x%x 0x%x] pdcm_len[0x%x 0x%x 0x%x 0x%x]\n",
		mem->print_buff_offset,
		mem->dump_buff_offset,
		mem->cr_region_offset,
		mem->mem_region_offset,
		mem->print_buff_len,
		mem->dump_buff_len,
		mem->cr_region_num,
		mem->mem_region_num);

exit:
	return ret;
}

static int __coredump_wait_done(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info,
	u_int8_t force_dump,
	u_int8_t *state_ready)
{
	struct ctrl_blk_layout ctrl_blk = {0};
	unsigned long timeout;
	int ret = 0;

	timeout = jiffies + msecs_to_jiffies(COREDUMP_TIMEOUT);

	while (TRUE) {
		if (time_after(jiffies, timeout)) {
			DBGLOG(INIT, ERROR, "Coredump timeout.\n");
			DBGLOG_MEM32(INIT, ERROR, &ctrl_blk, sizeof(ctrl_blk));
			ret = -ETIMEDOUT;
			break;
		}

		ret = emi_mem_read(chip_info,
			COREDUMP_OFFSET_CTRL_BLOCK,
			&ctrl_blk,
			sizeof(ctrl_blk));
		if (ret) {
			DBGLOG(INIT, ERROR,
				"Read coredump state failed.\n");
			break;
		}

		if (ctrl_blk.state == COREDUMP_STATE_PUT_DONE) {
			*state_ready = TRUE;
			break;
		} else if (force_dump) {
			break;
		}

		kalMsleep(100);
	}

	return ret;
}

static int __coredump_init_mem_region(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info)
{
	struct coredump_mem *mem = &ctx->mem;
	struct mem_region *region;
	uint32_t idx = 0, total_sz = 0;
	int ret = 0;

	if (mem->mem_region_num == 0)
		goto exit;

	total_sz = mem->mem_region_num * sizeof(struct mem_region);
	mem->mem_regions = kalMemAlloc(
		total_sz,
		VIR_MEM_TYPE);
	if (!mem->mem_regions) {
		ret = -ENOMEM;
		goto exit;
	}
	kalMemZero(mem->mem_regions, total_sz);

	for (idx = 0, region = mem->mem_regions;
	     idx < mem->mem_region_num;
	     idx++, region++) {
		struct mem_region_layout layout;
		uint32_t offset = mem->mem_region_offset +
			idx * sizeof(struct mem_region_layout);

		kalMemZero(&layout, sizeof(layout));
		ret = emi_mem_read(chip_info,
			offset,
			&layout,
			sizeof(struct mem_region_layout));
		if (ret) {
			DBGLOG(INIT, ERROR,
				"Read memory region failed, %u %lu\n",
				offset,
				sizeof(struct mem_region_layout));
			goto exit;
		}
		kalMemCopy(region->name, layout.name, sizeof(layout.name));
		region->name[sizeof(layout.name)] = '\0';
		region->base = layout.base;
		region->size = layout.size;
		region->buf = kalMemAlloc(region->size, VIR_MEM_TYPE);
		if (!region->buf) {
			DBGLOG(INIT, ERROR,
				"[%d] Alloc buffer failed, size: 0x%x\n",
				idx,
				region->size);
			ret = -ENOMEM;
			goto exit;
		}
		DBGLOG(INIT, INFO,
			"[%d] name: %s, base: 0x%x, size: 0x%x\n",
			idx,
			region->name,
			region->base,
			region->size);
	}

exit:
	return ret;
}

static void __coredump_deinit_mem_region(struct coredump_ctx *ctx)
{
	struct coredump_mem *mem = &ctx->mem;
	struct mem_region *region;
	uint32_t idx = 0;

	if (!mem->mem_regions || mem->mem_region_num == 0)
		return;

	for (idx = 0, region = mem->mem_regions;
	     idx < mem->mem_region_num;
	     idx++, region++) {
		if (region->buf) {
			kalMemFree(region->buf,
				VIR_MEM_TYPE,
				region->size);
			region->buf = NULL;
		}
	}
	kalMemFree(mem->mem_regions,
		VIR_MEM_TYPE,
		mem->mem_region_num * sizeof(struct mem_region));
	mem->mem_regions = NULL;
}

static int __coredump_init_cr_region(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info)
{
	struct coredump_mem *mem = &ctx->mem;
	struct cr_region *region;
	uint32_t idx = 0, total_sz = 0;
	int ret = 0;

	if (mem->cr_region_num == 0)
		goto exit;

	total_sz = mem->cr_region_num * sizeof(struct cr_region);
	mem->cr_regions = kalMemAlloc(
		total_sz,
		VIR_MEM_TYPE);
	if (!mem->cr_regions) {
		ret = -ENOMEM;
		goto exit;
	}
	kalMemZero(mem->cr_regions, total_sz);

	for (idx = 0, region = mem->cr_regions;
	     idx < mem->cr_region_num;
	     idx++, region++) {
		struct cr_region_layout layout;
		uint32_t offset = mem->cr_region_offset +
			idx * sizeof(struct cr_region_layout);

		kalMemZero(&layout, sizeof(layout));
		ret = emi_mem_read(chip_info,
			offset,
			&layout,
			sizeof(struct cr_region_layout));
		if (ret) {
			DBGLOG(INIT, ERROR,
				"Read cr region failed, %u %lu\n",
				offset,
				sizeof(struct cr_region_layout));
			goto exit;
		}
		region->base = layout.base;
		region->size = layout.size;

		if (region->base == 0xFFFFFFFF || region->size == 0) {
			region->buf = NULL;
			continue;
		}

		region->buf = kalMemAlloc(region->size, VIR_MEM_TYPE);
		if (!region->buf) {
			DBGLOG(INIT, ERROR,
				"[%d] Alloc buffer failed, size: 0x%x\n",
				idx,
				region->size);
			ret = -ENOMEM;
			goto exit;
		}
	}

exit:
	return ret;
}

static void __coredump_deinit_cr_region(struct coredump_ctx *ctx)
{
	struct coredump_mem *mem = &ctx->mem;
	struct cr_region *region;
	uint32_t idx = 0;

	if (!mem->cr_regions || mem->cr_region_num == 0)
		return;

	for (idx = 0, region = mem->cr_regions;
	     idx < mem->cr_region_num;
	     idx++, region++) {
		if (region->buf) {
			kalMemFree(region->buf,
				VIR_MEM_TYPE,
				region->size);
			region->buf = NULL;
		}
	}
	kalMemFree(mem->cr_regions,
		VIR_MEM_TYPE,
		mem->cr_region_num * sizeof(struct cr_region));
	mem->cr_regions = NULL;
}

static void __coredump_deinit(struct coredump_ctx *ctx)
{
	struct coredump_mem *mem = &ctx->mem;

	if (mem->aee_str_buff) {
		kalMemFree(mem->aee_str_buff,
			VIR_MEM_TYPE,
			AEE_STR_LEN);
		mem->aee_str_buff = NULL;
	}

	__coredump_deinit_mem_region(ctx);

	__coredump_deinit_cr_region(ctx);

	if (mem->dump_buff && mem->dump_buff_len) {
		kalMemFree(mem->dump_buff,
			VIR_MEM_TYPE,
			mem->dump_buff_len);
		mem->dump_buff = NULL;
	}

	if (mem->print_buff && mem->print_buff_len) {
		kalMemFree(mem->print_buff,
			VIR_MEM_TYPE,
			mem->print_buff_len);
		mem->print_buff = NULL;
	}

#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
	g_DfdCoredumpState = COREDUMP_FSM_NOT_START;
#endif
}

static int __coredump_init(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info)
{
#define AEE_STR_LEN		256

	struct coredump_mem *mem = &ctx->mem;
	int ret = 0;

	if (mem->print_buff_len) {
		mem->print_buff = kalMemAlloc(
			mem->print_buff_len,
			VIR_MEM_TYPE);
		if (!mem->print_buff) {
			ret = -ENOMEM;
			goto exit;
		}
	}

	if (mem->dump_buff_len) {
		mem->dump_buff = kalMemAlloc(
			mem->dump_buff_len,
			VIR_MEM_TYPE);
		if (!mem->dump_buff) {
			ret = -ENOMEM;
			goto exit;
		}
	}

	ret = __coredump_init_cr_region(ctx, chip_info);
	if (ret)
		goto exit;

	ret = __coredump_init_mem_region(ctx, chip_info);
	if (ret)
		goto exit;

	mem->aee_str_buff = kalMemAlloc(AEE_STR_LEN, VIR_MEM_TYPE);

	return 0;

exit:
	__coredump_deinit(ctx);
	return ret;
}

static int __coredump_handle_print_buff(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info)
{
	struct coredump_mem *mem = &ctx->mem;
	int ret = 0;

	if (mem->print_buff_len == 0)
		goto exit;

	ret = emi_mem_read(chip_info,
		mem->print_buff_offset,
		mem->print_buff,
		mem->print_buff_len - 1);
	if (ret)
		goto exit;
	mem->print_buff[mem->print_buff_len - 1] = '\0';

exit:
	return ret;
}

static int __coredump_handle_dump_buff(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info)
{
	struct coredump_mem *mem = &ctx->mem;
	int ret = 0;

	if (mem->dump_buff_len == 0)
		goto exit;

	ret = emi_mem_read(chip_info,
		mem->dump_buff_offset,
		mem->dump_buff,
		mem->dump_buff_len - 1);
	if (ret)
		goto exit;
	mem->dump_buff[mem->dump_buff_len - 1] = '\0';

	DBGLOG(INIT, INFO, "++ Coredump message ++\n");
	PRINT_LONG_STR_MSG(mem->dump_buff, mem->dump_buff_len);
	DBGLOG(INIT, INFO, "-- Coredump message --\n");

exit:
	return ret;
}

static int __coredump_handle_cr_region(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info)
{
	struct coredump_mem *mem = &ctx->mem;
	struct GLUE_INFO *glue = ctx->priv;
	struct cr_region *region;
	uint32_t i = 0, j = 0;
	int ret = 0;
#if CFG_MTK_WIFI_MBU || defined(_HIF_PCIE)
	struct CHIP_DBG_OPS *debug_ops = NULL;
#endif
#if CFG_MTK_WIFI_MBU
	uint8_t uCurMbuTimeout = 0;
	u_int8_t fgRet = FALSE;
#endif

	if (mem->cr_region_num == 0)
		goto exit;

#if defined(_HIF_PCIE)
	debug_ops = glue->prAdapter->chip_info->prDebugOps;
	if (debug_ops && debug_ops->dumpPcieStatus) {
		if (debug_ops->dumpPcieStatus(glue) == FALSE)
			goto exit;
	}
#endif

#if CFG_MTK_WIFI_MBU
	if (debug_ops && debug_ops->getMbuTimeoutStatus)
		uCurMbuTimeout = debug_ops->getMbuTimeoutStatus();
#endif

	for (i = 0, region = mem->cr_regions;
	     i < mem->cr_region_num;
	     i++, region++) {
		if (!region->buf)
			continue;

		for (j = 0; j < region->size; j += 4) {
#if CFG_MTK_WIFI_MBU
			if (!uCurMbuTimeout) {
				HAL_MCR_EMI_RD(glue->prAdapter,
					region->base + j,
					(uint32_t *)&region->buf[j], &fgRet);
			} else
#endif
				HAL_RMCR_RD(COREDUMP_DBG, glue->prAdapter,
				    region->base + j,
				    (uint32_t *)&region->buf[j]);
		}
		region->ready = TRUE;
	}

exit:
	return ret;
}

static int __coredump_handle_mem_region(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info)
{
	struct coredump_mem *mem = &ctx->mem;
	struct GLUE_INFO *glue = ctx->priv;
	struct mem_region *region;
	uint32_t idx = 0;
	int ret = 0;
	u_int8_t read_ret = FALSE;

	for (idx = 0, region = mem->mem_regions;
	     idx < mem->mem_region_num;
	     idx++, region++) {

#if CFG_WIFI_SECURITY_COREDUMP
		if (coredump_aes_driver_control(glue, region->base,
			SEC_COREDUMP_EMI_BASE, region->size) == FALSE) {
			DBGLOG(INIT, ERROR,
				"[%d] Trigger coredump failed, %s 0x%x 0x%x\n",
				idx,
				region->name,
				region->base,
				region->size);
			ret = -ENOMEM;
			break;
		}

		read_ret = (emi_mem_read(chip_info, SEC_COREDUMP_EMI_OFFSET,
				region->buf, region->size)) ? FALSE : TRUE;
#else
		HAL_RMCR_RD_RANGE(COREDUMP_DBG,
				  glue->prAdapter,
				  region->base,
				  region->buf,
				  region->size, read_ret);
#endif
		if (read_ret == FALSE) {
			DBGLOG(INIT, ERROR,
				"[%d] Read mem region failed, %s 0x%x 0x%x\n",
				idx,
				region->name,
				region->base,
				region->size);
			ret = -ENOMEM;
			break;
		}

		region->ready = TRUE;

		DBGLOG(INIT, INFO,
			"[%d] mem region %s 0x%x 0x%x\n",
			idx,
			region->name,
			region->base,
			region->size);
	}

	return ret;
}

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
static int __coredump_to_userspace_cr_region(struct coredump_ctx *ctx)
{
/* "[%08x,%08x]" */
#define BUFF_SZ_PER_CR		19

	struct coredump_mem *mem = &ctx->mem;
	struct cr_region *cr_region;
	uint32_t i = 0, j = 0, total_sz = 0;
	uint8_t *buf, *pos;
	int ret = 0;

	/* calculate log buffer size */
	for (i = 0, cr_region = mem->cr_regions;
	     i < mem->cr_region_num;
	     i++, cr_region++) {
		if (!cr_region->buf || !cr_region->ready)
			continue;

		total_sz += cr_region->size;
	}
	total_sz *= BUFF_SZ_PER_CR;

	buf = kalMemAlloc(total_sz, VIR_MEM_TYPE);
	if (!buf) {
		DBGLOG(INIT, ERROR,
			"Alloc cr region log buffer failed.\n");
		ret = -ENOMEM;
		goto exit;
	}
	for (i = 0, cr_region = mem->cr_regions, pos = buf;
	     i < mem->cr_region_num;
	     i++, cr_region++) {
		if (!cr_region->buf || !cr_region->ready)
			continue;

		for (j = 0; j < cr_region->size / 4; j++) {
			uint32_t addr = 0, val = 0;

			addr = cr_region->base + j * 4;
			val = *((uint32_t *)(cr_region->buf + j * 4));

			*pos++ = '[';
			kalMemCopy(pos,
				   &addr,
				   sizeof(addr));
			pos += 4;
			*pos++ = ',';
			kalMemCopy(pos,
				   &val,
				   sizeof(val));
			pos += 4;
			*pos++ = ']';
		}
	}

	/* send to userspace */
	connv3_coredump_send(ctx->handler,
			     "[M]",
			     buf,
			     total_sz);

exit:
	if (buf)
		kalMemFree(buf, VIR_MEM_TYPE, total_sz);

	return ret;
}

static void __coredump_to_userspace_aee_str(struct coredump_ctx *ctx,
	struct connv3_issue_info *issue_info,
	uint8_t *aee_str,
	uint32_t aee_str_len)
{
	struct coredump_mem *mem = &ctx->mem;
	int32_t written = 0;

	switch (issue_info->issue_type) {
	case CONNV3_ISSUE_DRIVER_ASSERT:
		written += kalSnprintf(aee_str + written,
			aee_str_len - written,
			"<DRIVER> WFSYS %s: %s",
			issue_info->task_name,
			issue_info->reason);
		break;
	case CONNV3_ISSUE_FW_ASSERT:
	{
		u_int8_t casan = FALSE;

		casan = kalStrStr(mem->dump_buff, "_asan.c") != NULL;
		if (casan) {
			written += kalSnprintf(aee_str + written,
				aee_str_len - written,
				"<CASAN> WFSYS ");
		} else {
			written += kalSnprintf(aee_str + written,
				aee_str_len - written,
				"<ASSERT> WFSYS ");
		}

		written += kalSnprintf(aee_str + written,
			aee_str_len - written,
			"%s",
			issue_info->assert_info);
	}
		break;
	case CONNV3_ISSUE_FW_EXCEPTION:
	{
		/* dump_buff:
		 *  <EXCEPTION> WFSYS, id=0x0 WIFI, lr=0xE00F5B3C, swid=0x9B2,
		 *  e0=0x60, e1=0x63, e2=0x12C2, mcause=0x9, exp_t=77202116
		 * assert_info:
		 *  swid=0x9B2, e0=0x60, e1=0x63, e2=0x12C2
		 * aee_str:
		 *  <FATAL_ERR> WFSYS swid=0x9B2, e0=0x60, e1=0x63, e2=0x12C2
		 */
		uint8_t *puSwid = kalStrStr(mem->dump_buff, "swid=");

		if (puSwid != NULL) {
			/* overwrite assert_info */
			/* swid=0xXXXX */
			#define SWID_SIZE_MIN 11
			/* swid=0xXXXX, e0=0xXXXXXXXX, e1=... */
			#define SWID_SIZE_MAX 56
			uint32_t u4SwidSize = SWID_SIZE_MIN;
			uint32_t u4AssertSize = sizeof(issue_info->assert_info);
			uint8_t *puMcause;

			puMcause = kalStrStr(mem->dump_buff, ", mcause=");
			if (puMcause != NULL) {
				u4SwidSize = puMcause - puSwid;
				if (u4SwidSize < SWID_SIZE_MIN)
					u4SwidSize = SWID_SIZE_MIN;
				if (u4SwidSize > SWID_SIZE_MAX)
					u4SwidSize = SWID_SIZE_MAX;
				if (u4SwidSize >= u4AssertSize)
					u4SwidSize = u4AssertSize - 1;
			}

			kalMemCopy(issue_info->assert_info,
				puSwid,
				u4SwidSize);
			issue_info->assert_info[u4SwidSize] = '\0';

			written += kalSnprintf(aee_str + written,
				aee_str_len - written,
				"<FATAL_ERR> WFSYS ");

			written += kalSnprintf(aee_str + written,
				aee_str_len - written,
				"%s",
				issue_info->assert_info);
		} else {
			written += kalSnprintf(aee_str + written,
				aee_str_len - written,
				"<EXCEPTION> WFSYS ");

			written += kalSnprintf(aee_str + written,
				aee_str_len - written,
				"%s",
				issue_info->assert_info);
		}
	}
		break;
	case CONNV3_ISSUE_FORCE_DUMP:
	{
		written += kalSnprintf(aee_str + written,
			aee_str_len - written,
			"FORCE DUMP");
	}
		break;
	default:
		written += kalSnprintf(aee_str + written,
			aee_str_len - written,
			"UNKNOWN");
		break;
	}

	DBGLOG(INIT, INFO, "aee_str: %s\n", aee_str);
}

static void __coredump_to_userspace_issue_info(struct coredump_ctx *ctx,
	uint8_t *aee_str,
	uint32_t aee_str_len)
{
#define BUF_SIZE	2048

	struct coredump_mem *mem = &ctx->mem;
	struct connv3_issue_info issue_info;
	struct mem_region *mem_region;
	uint32_t idx = 0;
	int32_t written = 0;
	uint8_t *buf, *pos;

	kalMemZero(&issue_info, sizeof(issue_info));
	buf = kalMemAlloc(BUF_SIZE, VIR_MEM_TYPE);
	if (!buf) {
		DBGLOG(INIT, ERROR, "Alloc buffer failed.\n");
		return;
	}
	kalMemZero(buf, BUF_SIZE);

	connv3_coredump_get_issue_info(ctx->handler,
				       &issue_info,
				       buf,
				       BUF_SIZE);

	__coredump_to_userspace_aee_str(ctx, &issue_info, aee_str,
		aee_str_len);

	pos = kalStrStr(buf, "</main>");
	if (!pos) {
		DBGLOG(INIT, ERROR, "No </main> pattern from %s\n",
			buf);
		goto exit;
	}

	written += kalSnprintf(pos + written,
			       BUF_SIZE - written,
			       "\t<map>\n");
	for (idx = 0, mem_region = mem->mem_regions;
	     idx < mem->mem_region_num;
	     idx++, mem_region++) {
		if (!mem_region->ready)
			continue;

		written += kalSnprintf(pos + written,
				       BUF_SIZE - written,
				       "\t\t<%s>\n",
				       mem_region->name);
		written += kalSnprintf(pos + written,
				       BUF_SIZE - written,
				       "\t\t\t<offset>0x%x</offset>\n",
				       mem_region->base);
		written += kalSnprintf(pos + written,
				       BUF_SIZE - written,
				       "\t\t\t<size>0x%x</size>\n",
				       mem_region->size);
		written += kalSnprintf(pos + written,
				       BUF_SIZE - written,
				       "\t\t</%s>\n",
				       mem_region->name);
	}
	written += kalSnprintf(pos + written,
			       BUF_SIZE - written,
			       "\t</map>\n");
	written += kalSnprintf(pos + written,
			       BUF_SIZE - written,
			       "</main>\n");

	connv3_coredump_send(ctx->handler,
			     "INFO",
			     buf,
			     kalStrLen(buf));

exit:
	kalMemFree(buf, VIR_MEM_TYPE, BUF_SIZE);
}

static int __coredump_to_userspace_mem_region(struct coredump_ctx *ctx)
{
	struct coredump_mem *mem = &ctx->mem;
	struct mem_region *mem_region;
	uint32_t idx = 0;

	for (idx = 0, mem_region = mem->mem_regions;
	     idx < mem->mem_region_num;
	     idx++, mem_region++) {
		if (!mem_region->ready)
			continue;

		connv3_coredump_send(ctx->handler,
				     mem_region->name,
				     mem_region->buf,
				     mem_region->size);
	}

	return 0;
}

static int __coredump_to_userspace_scp_dump(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info)
{
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
	u64 u8ScpDumpPhyAddr = 0;
	unsigned int u4ScpDumpSize = 0;
	uint8_t *pScpDumpBuf = NULL;
	void *vir_addr = NULL;
	int32_t i4Ret;

	i4Ret = kalGetScpDumpInfo(&u8ScpDumpPhyAddr, &u4ScpDumpSize);
	if (i4Ret) {
		DBGLOG(INIT, INFO, "no scp dump info\n");
		return 0;
	}
	DBGLOG(INIT, INFO, "scp dump addr:0x%llx, size:%u\n",
		u8ScpDumpPhyAddr, u4ScpDumpSize);

	pScpDumpBuf = kalMemAlloc(u4ScpDumpSize, VIR_MEM_TYPE);
	if (pScpDumpBuf == NULL) {
		DBGLOG(INIT, ERROR,
				"Alloc scp dump buffer failed.\n");
				return 0;
	}

	kalMemZero(pScpDumpBuf, u4ScpDumpSize);
	vir_addr = ioremap(u8ScpDumpPhyAddr, u4ScpDumpSize);
	if (!vir_addr) {
		DBGLOG(INIT, ERROR, "ioremap fail.\n");
		goto exit;
	}

	kalMemCopyFromIo(pScpDumpBuf, vir_addr, u4ScpDumpSize);
	connv3_coredump_send(ctx->handler,
			     "PRED",
			     pScpDumpBuf,
			     u4ScpDumpSize);
	iounmap(vir_addr);
exit:
	kalMemFree(pScpDumpBuf, VIR_MEM_TYPE, u4ScpDumpSize);
	DBGLOG(INIT, INFO, "scp dump done\n");
#endif
	return 0;
}

#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
static uint32_t wlanSendDFDInfo(
	struct ADAPTER *prAdapter, uint32_t u4InfoIdx,
	uint32_t u4Length, uint8_t *pBuf, uint32_t *u4RetLen)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4ReservedLength = u4Length;
	uint32_t u4Offset = 0, u4Size = u4Length;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;

	prChipInfo = prAdapter->chip_info;
	if (!prChipInfo->queryDFDInfo)
		return WLAN_STATUS_FAILURE;

	while (u4ReservedLength) {
		if (u4ReservedLength > DEBUG_INFO_DFD_MAX_EVENT_LEN) {
			u4Size = DEBUG_INFO_DFD_MAX_EVENT_LEN;
			u4ReservedLength -= DEBUG_INFO_DFD_MAX_EVENT_LEN;
		} else {
			u4Size = u4ReservedLength;
			u4ReservedLength = 0;
		}

		DBGLOG(INIT, INFO, "[%d] offset:%d, size:%d, reservedLen:%d\n",
				u4InfoIdx, u4Offset, u4Size, u4ReservedLength);
		if (prChipInfo->queryDFDInfo(prAdapter,
				u4InfoIdx, u4Offset, u4Size,
				(pBuf+u4Offset)) != u4Size) {
			u4Status = WLAN_STATUS_FAILURE;
			break;
		}

		u4Offset += u4Size;
	}
	*u4RetLen = u4Offset;
	return u4Status;
}

static uint32_t wlanShowDFDInfo(struct coredump_ctx *ctx,
	uint8_t *pDumBuf, uint32_t *pDumSize)
{
	struct GLUE_INFO *glue = ctx->priv;
	struct ADAPTER *prAdapter = glue->prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4DumpCount = 0, u4CurPos = 0;
	uint32_t u4Status;

	*pDumSize = 0;
	if (!prAdapter)
		return WLAN_STATUS_FAILURE;

	DBGLOG(INIT, TRACE, "DFD dump Info:\n");
	prChipInfo = prAdapter->chip_info;
	if (prChipInfo->queryDFDInfo) {
		/* DFD_CB_INFRA_INFO */
		u4Status = wlanSendDFDInfo(prAdapter,
				DEBUG_INFO_DFD_CB_INFRA_INFO,
				DEBUG_INFO_DFD_CB_INFRA_INFO_LENG,
				(pDumBuf+u4CurPos), &u4DumpCount);
		u4CurPos += u4DumpCount;
		if (u4Status != WLAN_STATUS_SUCCESS)
			goto exit;
		DBGLOG(INIT, INFO, "[%d] curPos:%d\n",
				DEBUG_INFO_DFD_CB_INFRA_INFO, u4CurPos);

		/* DFD_CB_INFRA_SRAM */
		u4Status = wlanSendDFDInfo(prAdapter,
				DEBUG_INFO_DFD_CB_INFRA_SRAM,
				DEBUG_INFO_DFD_CB_INFRA_SRAM_LENG,
				(pDumBuf+u4CurPos), &u4DumpCount);
		u4CurPos += u4DumpCount;
		if (u4Status != WLAN_STATUS_SUCCESS)
			goto exit;
		DBGLOG(INIT, INFO, "[%d] curPos:%d\n",
				DEBUG_INFO_DFD_CB_INFRA_SRAM, u4CurPos);

		/* DFD_CB_INFRA_WF_SRAM */
		u4Status = wlanSendDFDInfo(prAdapter,
				DEBUG_INFO_DFD_CB_INFRA_WF_SRAM,
				DEBUG_INFO_DFD_CB_INFRA_WF_SRAM_LENG,
				(pDumBuf+u4CurPos), &u4DumpCount);
		u4CurPos += u4DumpCount;
		if (u4Status != WLAN_STATUS_SUCCESS)
			goto exit;
		DBGLOG(INIT, INFO, "[%d] curPos:%d\n",
				DEBUG_INFO_DFD_CB_INFRA_WF_SRAM, u4CurPos);

		/* DFD_CB_INFRA_DEBUG_INFO */
		u4Status = wlanSendDFDInfo(prAdapter,
				DEBUG_INFO_DFD_CB_INFRA_DEBUG_INFO,
				DEBUG_INFO_DFD_CB_INFRA_DEBUG_INFO_LENG,
				(pDumBuf+u4CurPos), &u4DumpCount);
		u4CurPos += u4DumpCount;
		if (u4Status != WLAN_STATUS_SUCCESS)
			goto exit;
		DBGLOG(INIT, INFO, "[%d] curPos:%d\n",
				DEBUG_INFO_DFD_CB_INFRA_DEBUG_INFO, u4CurPos);

		/* DFD_WF_DEBUG_INFO */
		u4Status = wlanSendDFDInfo(prAdapter,
				DEBUG_INFO_DFD_WF_DEBUG_INFO,
				DEBUG_INFO_DFD_WF_DEBUG_INFO_LENG,
				(pDumBuf+u4CurPos), &u4DumpCount);
		u4CurPos += u4DumpCount;
		if (u4Status != WLAN_STATUS_SUCCESS)
			goto exit;
		DBGLOG(INIT, INFO, "[%d] curPos:%d\n",
				DEBUG_INFO_DFD_WF_DEBUG_INFO, u4CurPos);
	}

exit:
	*pDumSize = u4CurPos;
	DBGLOG(INIT, TRACE, "dump total size:%d\n", *pDumSize);

	if (*pDumSize == 0)
		return WLAN_STATUS_FAILURE;
	return WLAN_STATUS_SUCCESS;
}
#endif /* #if CFG_MTK_WIFI_DFD_DUMP_SUPPORT */

static int __coredump_to_userspace_dfd_dump(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info)
{
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
	uint8_t *pDfdDumpBuf = NULL;
	uint32_t u4DfdDumpSize = 0;
	uint32_t u4Ret = 0;

	pDfdDumpBuf = kalMemAlloc(DEBUG_INFO_WIFI_DFD_DUMP_LENG, VIR_MEM_TYPE);
	if (pDfdDumpBuf == NULL) {
		DBGLOG(INIT, ERROR,
				"Alloc dfd dump buffer failed.\n");
				return 0;
	}

	kalMemZero(pDfdDumpBuf, DEBUG_INFO_WIFI_DFD_DUMP_LENG);
	u4Ret = wlanShowDFDInfo(ctx, pDfdDumpBuf, &u4DfdDumpSize);
	if (u4Ret) {
		DBGLOG(INIT, ERROR, "read empty dfd dump\n");
		goto exit;
	}

	connv3_coredump_send(ctx->handler,
			     "PSTD",
			     pDfdDumpBuf,
			     u4DfdDumpSize);
exit:
	kalMemFree(pDfdDumpBuf, VIR_MEM_TYPE, DEBUG_INFO_WIFI_DFD_DUMP_LENG);
#endif
	return 0;
}

static int __coredump_to_userspace(struct coredump_ctx *ctx,
	struct mt66xx_chip_info *chip_info,
	enum COREDUMP_SOURCE_TYPE source,
	enum ENUM_COREDUMP_BY_CHIP_RESET_TYPE_T type,
	char *reason,
	u_int8_t force_dump,
	u_int8_t state_ready)
{
#define AEE_STR_LEN		256
#define FW_VER_LEN		256
#define PACKAGE_MODE_LEN	24

	struct coredump_mem *mem = &ctx->mem;
	struct GLUE_INFO *glue = ctx->priv;
	enum connv3_drv_type drv_type;
	uint8_t *fw_version = NULL;
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
	uint8_t *package_mode = NULL;
	uint8_t package_mode_str[PACKAGE_MODE_LEN] = {0};
#endif
	uint32_t u4Len = 0;
	int32_t ret = 0;

	fw_version = kalMemAlloc(FW_VER_LEN, VIR_MEM_TYPE);
	if (!fw_version) {
		DBGLOG(INIT, ERROR,
			"Alloc mem failed, fw_version: 0x%p\n",
			fw_version);
		goto exit;
	}
	kalMemZero(mem->aee_str_buff, AEE_STR_LEN);
	kalMemZero(fw_version, FW_VER_LEN);

	if (glue->u4ReadyFlag == 0) {
		if (chip_info->fw_dl_ops->getFwVerInfo)
			chip_info->fw_dl_ops->getFwVerInfo(fw_version,
				&u4Len,
				FW_VER_LEN);
		else
			DBGLOG(INIT, ERROR, "NULL getFwVerInfo\n");
	} else {
		struct ADAPTER *prAdapter = glue->prAdapter;
		struct WIFI_VER_INFO *prVerInfo = &prAdapter->rVerInfo;

		if (kalStrLen(prVerInfo->aucReleaseManifest)) {
			u4Len = kalStrLen(prVerInfo->aucReleaseManifest);
			kalSnprintf(fw_version,
				    u4Len + 1,
				    "%s",
				    prVerInfo->aucReleaseManifest);
		} else {
			u4Len = kalStrLen(
				prVerInfo->rCommonTailer.aucRamBuiltDate);
			kalSnprintf(fw_version,
				    u4Len + 1,
				    "%s",
				    prVerInfo->rCommonTailer.aucRamBuiltDate);
		}
	}

#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
	u4Len = kalStrLen(fw_version);
	package_mode = fw_version + u4Len;

	kalScnprintf(package_mode_str,
		PACKAGE_MODE_LEN,
		(fgIsCurrentInTestMode) ?
		"Package:TestMode" : "Package:NormalMode");

	kalSnprintf(package_mode,
		FW_VER_LEN-u4Len,
		"\n%s",
		package_mode_str);

	DBGLOG(INIT, LOUD, "fw_version:%s", fw_version);
#endif

	drv_type = coredump_src_to_connv3_type(source);
	if (!state_ready) {
		uint8_t force_dump_buf[32];

		kalSnprintf(force_dump_buf, sizeof(force_dump_buf), "%s",
			    CONNV3_COREDUMP_FORCE_DUMP);
		ret = connv3_coredump_start(ctx->handler,
					    drv_type,
					    reason,
					    force_dump_buf,
					    fw_version);
	} else {
		if (mem->dump_buff != NULL) {
			ret = connv3_coredump_start(
					ctx->handler,
					drv_type,
					reason,
					mem->dump_buff,
					fw_version);
		} else {
			DBGLOG(INIT, ERROR,
				"NULL dump buffer.\n");
			ret = -EINVAL;
		}
	}
	if (ret) {
		DBGLOG(INIT, ERROR,
			"connv3_coredump_start ret: %d\n",
			ret);
		goto exit;
	}

#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
	g_DfdCoredumpState = COREDUMP_FSM_TRIGGER_CONNV3_DONE;
#endif

	__coredump_to_userspace_cr_region(ctx);

	__coredump_to_userspace_issue_info(ctx,
		mem->aee_str_buff,
		AEE_STR_LEN);

	__coredump_to_userspace_mem_region(ctx);

	if (type == ENUM_COREDUMP_BY_CHIP_RST_DFD_DUMP)
		__coredump_to_userspace_scp_dump(ctx, chip_info);

	if (type != ENUM_COREDUMP_BY_CHIP_RST_DFD_DUMP) {
		DBGLOG(INIT, INFO, "do coredump end\n");
		connv3_coredump_end(ctx->handler, mem->aee_str_buff);
	}

exit:
	if (fw_version)
		kalMemFree(fw_version, VIR_MEM_TYPE, FW_VER_LEN);

	return 0;
}
#endif

static u_int8_t is_coredump_source_valid(enum COREDUMP_SOURCE_TYPE source)
{
	switch (source) {
	case COREDUMP_SOURCE_WF_DRIVER:
	case COREDUMP_SOURCE_WF_MAWD:
	case COREDUMP_SOURCE_WF_FW:
	case COREDUMP_SOURCE_BT:
	case COREDUMP_SOURCE_CONNV3:
	case COREDUMP_SOURCE_CONNINFRA:
		return TRUE;
	default:
		return FALSE;
	}
}

static int __coredump_start(struct coredump_ctx *ctx,
	enum COREDUMP_SOURCE_TYPE source,
	enum ENUM_COREDUMP_BY_CHIP_RESET_TYPE_T type,
	char *reason,
	u_int8_t force_dump)
{
	struct coredump_mem *mem = &ctx->mem;
	struct mt66xx_chip_info *chip_info;
	u_int8_t state_ready = FALSE;
	int ret = 0;

	if (!ctx->enable) {
		DBGLOG(INIT, WARN,
			"Skip coredump due to NOT enabled.\n");
		goto exit;
	}

	if (!is_coredump_source_valid(source)) {
		DBGLOG(INIT, WARN,
			"Skip coredump due to invalid source(%d).\n",
			source);
		goto exit;
	}

	glGetChipInfo((void **)&chip_info);
	if (!chip_info) {
		DBGLOG(INIT, ERROR, "chip info is NULL\n");
		goto exit;
	}

	kalMemZero(mem, sizeof(*mem));

	ret = __coredump_wait_done(ctx,
				   chip_info,
				   force_dump,
				   &state_ready);

	ret = __coredump_init_ctrl_blk(ctx, chip_info);
	if (ret)
		goto exit;

	ret = __coredump_init(ctx, chip_info);
	if (ret)
		goto exit;

#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
	g_DfdCoredumpState = COREDUMP_FSM_INIT_DONE;
#endif

	ret = __coredump_handle_print_buff(ctx, chip_info);
	if (ret)
		goto deinit;

	ret = __coredump_handle_dump_buff(ctx, chip_info);
	if (ret)
		goto deinit;

	ret = __coredump_handle_cr_region(ctx, chip_info);
	if (ret)
		goto deinit;

	ret = __coredump_handle_mem_region(ctx, chip_info);
	if (ret)
		goto deinit;

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	__coredump_to_userspace(ctx,
				chip_info,
				source,
				type,
				reason,
				force_dump,
				state_ready);
#endif

deinit:
	if (type != ENUM_COREDUMP_BY_CHIP_RST_DFD_DUMP)
		__coredump_deinit(ctx);
exit:
	return ret;
}

#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
int wifi_coredump_post_start(void)
{
	struct coredump_ctx *ctx = &g_coredump_ctx;
	struct coredump_mem *mem = &ctx->mem;
	struct mt66xx_chip_info *chip_info;
	int ret = 0;

	if (g_DfdCoredumpState == COREDUMP_FSM_NOT_START) {
		DBGLOG(INIT, WARN,
			"Skip coredump due to coredump NOT started.\n");
		return ret;
	}

	if (g_DfdCoredumpState == COREDUMP_FSM_INIT_DONE) {
		DBGLOG(INIT, WARN,
			"Skip coredump due to connv3 trigger NOT started\n");
		goto deinit;
	}

	if (!ctx->initialized) {
		DBGLOG(INIT, WARN,
			"Skip coredump due to NOT initialized.\n");
		goto coredump_end;
	}

	glGetChipInfo((void **)&chip_info);
	if (!chip_info) {
		DBGLOG(INIT, ERROR, "chip info is NULL\n");
		goto coredump_end;
	}

	ctx->processing = TRUE;

	__coredump_to_userspace_dfd_dump(ctx, chip_info);

coredump_end:
	DBGLOG(INIT, INFO, "do coredump end\n");
	connv3_coredump_end(ctx->handler, mem->aee_str_buff);
deinit:
	__coredump_deinit(ctx);
	ctx->processing = FALSE;
	return ret;
}
#endif

void wifi_coredump_start(enum COREDUMP_SOURCE_TYPE source,
	char *reason,
	enum ENUM_COREDUMP_BY_CHIP_RESET_TYPE_T type,
	u_int8_t force_dump)
{
	struct coredump_ctx *ctx = &g_coredump_ctx;

	if (!ctx->initialized) {
		DBGLOG(INIT, WARN,
			"Skip coredump due to NOT initialized.\n");
		return;
	}

#if CFG_WIFI_COREDUMP_SKIP_BY_REQUEST
	if (get_coredump_skipped()) {
		DBGLOG(INIT, INFO,
			"skip coredump due to skip enabled.\n");
		return;
	}
#endif

	DBGLOG(INIT, INFO, "source: %d, reason: %s, force_dump: %d\n",
		source, reason, force_dump);
	ctx->processing = TRUE;

#if CFG_SUPPORT_CONNINFRA
	{
		enum consys_drv_type drv_type;

		drv_type = coredump_src_to_conn_type(source);
		connsys_coredump_start(ctx->handler, 0,
			drv_type, reason);
		connsys_coredump_clean(ctx->handler);
	}
#elif IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	__coredump_start(ctx, source, type, reason, force_dump);
#endif
	ctx->processing = FALSE;
}

#if CFG_SUPPORT_CONNINFRA || IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
enum consys_drv_type coredump_src_to_conn_type(enum COREDUMP_SOURCE_TYPE src)
{
	enum consys_drv_type drv_type;

	switch (src) {
	case COREDUMP_SOURCE_WF_FW:
		drv_type = CONNDRV_TYPE_MAX;
		break;
	case COREDUMP_SOURCE_BT:
		drv_type = CONNDRV_TYPE_BT;
		break;
	case COREDUMP_SOURCE_FM:
		drv_type = CONNDRV_TYPE_FM;
		break;
	case COREDUMP_SOURCE_GPS:
		drv_type = CONNDRV_TYPE_GPS;
		break;
	case COREDUMP_SOURCE_CONNINFRA:
		drv_type = CONNDRV_TYPE_CONNINFRA;
		break;
	case COREDUMP_SOURCE_WF_DRIVER:
	default:
		drv_type = CONNDRV_TYPE_WIFI;
		break;
	}

	return drv_type;
}

enum COREDUMP_SOURCE_TYPE coredump_conn_type_to_src(enum consys_drv_type src)
{
	enum COREDUMP_SOURCE_TYPE type;

	switch (src) {
	case CONNDRV_TYPE_BT:
		type = COREDUMP_SOURCE_BT;
		break;
	case CONNDRV_TYPE_FM:
		type = COREDUMP_SOURCE_FM;
		break;
	case CONNDRV_TYPE_GPS:
		type = COREDUMP_SOURCE_GPS;
		break;
	case CONNDRV_TYPE_CONNINFRA:
		type = COREDUMP_SOURCE_CONNINFRA;
		break;
	case CONNDRV_TYPE_WIFI:
	default:
		type = COREDUMP_SOURCE_WF_DRIVER;
		break;
	}

	return type;
}
#endif

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
enum connv3_drv_type coredump_src_to_connv3_type(enum COREDUMP_SOURCE_TYPE src)
{
	enum connv3_drv_type drv_type;

	switch (src) {
	case COREDUMP_SOURCE_BT:
		drv_type = CONNV3_DRV_TYPE_BT;
		break;
	case COREDUMP_SOURCE_MD:
		drv_type = CONNV3_DRV_TYPE_MODEM;
		break;
	case COREDUMP_SOURCE_CONNV3:
		drv_type = CONNV3_DRV_TYPE_CONNV3;
		break;
	case COREDUMP_SOURCE_WF_FW:
		drv_type = CONNV3_DRV_TYPE_MAX;
		break;
	case COREDUMP_SOURCE_WF_DRIVER:
	default:
		drv_type = CONNV3_DRV_TYPE_WIFI;
		break;
	}

	return drv_type;
}

enum COREDUMP_SOURCE_TYPE coredump_connv3_type_to_src(enum connv3_drv_type src)
{
	enum COREDUMP_SOURCE_TYPE type;

	switch (src) {
	case CONNV3_DRV_TYPE_BT:
		type = COREDUMP_SOURCE_BT;
		break;
	case CONNV3_DRV_TYPE_MODEM:
		type = COREDUMP_SOURCE_MD;
		break;
	case CONNV3_DRV_TYPE_CONNV3:
		type = COREDUMP_SOURCE_CONNV3;
		break;
	case CONNV3_DRV_TYPE_WIFI:
	default:
		type = COREDUMP_SOURCE_WF_DRIVER;
		break;
	}

	return type;
}
#endif

void wifi_coredump_set_enable(u_int8_t enable)
{
	struct coredump_ctx *ctx = &g_coredump_ctx;

	ctx->enable = enable;
}

u_int8_t is_wifi_coredump_processing(void)
{
	struct coredump_ctx *ctx = &g_coredump_ctx;

	return ctx->processing;
}

