/**
 *  Copyright (c) 2018 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#ifndef __BTMTK_EXTERN_H__
#define __BTMTK_EXTERN_H__

typedef void (*event_callback)(struct sk_buff *skb);

int btmtk_vendor_module_register(event_callback cb, unsigned char *cfg_file, int file_len);
int btmtk_send_vendor_cmd(struct sk_buff *skb);
int btmtk_vendor_module_unregister(void);
#endif
