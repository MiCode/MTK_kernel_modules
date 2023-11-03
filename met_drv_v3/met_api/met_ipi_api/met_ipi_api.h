/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "met_ext_sym_util.h"

EXTERNAL_SYMBOL_FUNC(int,mtk_ipi_recv_reply,struct mtk_ipi_device *ipidev, int ipi_id, void *reply_data, int len)
EXTERNAL_SYMBOL_FUNC(int,mtk_ipi_send,struct mtk_ipi_device *ipidev, int ipi_id, int opt, void *data, int len, int retry_timeout)
EXTERNAL_SYMBOL_FUNC(int,mtk_ipi_send_compl,struct mtk_ipi_device *ipidev, int ipi_id, int opt, void *data, int len, unsigned long timeout)
EXTERNAL_SYMBOL_FUNC(int,mtk_ipi_register,struct mtk_ipi_device *ipidev, int ipi_id, mbox_pin_cb_t cb, void *prdata, void *msg)
EXTERNAL_SYMBOL_FUNC(int,mtk_ipi_unregister,struct mtk_ipi_device *ipidev, int ipi_id)
