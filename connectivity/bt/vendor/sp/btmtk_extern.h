/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#ifndef __BTMTK_EXTERN_H__
#define __BTMTK_EXTERN_H__

typedef void (*event_callback)(struct sk_buff *skb);

int btmtk_vendor_module_register(event_callback cb, unsigned char *cfg_file, int file_len);
int btmtk_send_vendor_cmd(struct sk_buff *skb);
int btmtk_vendor_module_unregister(void);
#endif
