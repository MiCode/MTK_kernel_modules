/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#ifndef _GPS_DL_PROCFS_H
#define _GPS_DL_PROCFS_H

int gps_dl_procfs_setup(void);
int gps_dl_procfs_remove(void);

int gps_dl_procfs_get_dev_open_delay(void);
void gps_dl_procfs_set_dev_open_delay(int val);

typedef int(*gps_dl_procfs_test_func_type)(int y, int z);

#endif /* _GPS_DL_PROCFS_H */

