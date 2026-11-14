/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef _GPS_DL_INFO_NODE_H
#define _GPS_DL_INFO_NODE_H

void gps_dl_info_node_set_ecid_info(unsigned long ecid_data1,
	unsigned long ecid_data2);
void gps_dl_info_node_set_adie_info(unsigned int adie_info);
int gps_dl_info_node_setup(void);
int gps_dl_info_node_remove(void);


#endif /* _GPS_DL_INFO_NODE_H */

