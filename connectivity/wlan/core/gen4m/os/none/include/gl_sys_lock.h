/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

void wfsys_lock(void);
int wfsys_trylock(void);
void wfsys_unlock(void);
int wfsys_is_locked(void);

