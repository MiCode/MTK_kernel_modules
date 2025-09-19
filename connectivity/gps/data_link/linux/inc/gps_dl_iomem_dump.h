/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#ifndef _GPS_DL_IOMEM_DUMP_H
#define _GPS_DL_IOMEM_DUMP_H

/*
* dummy_addr:
*   - valid range: 0x18xx_xxxx, 0x70xxx_xxxx ~ 0x77xx_xxxx
*   - must 4byte allignment
* dump_len:
*   - 0: only dump 4bytes
*   - others: dump ceil(dump_len/4.0)*4 bytes
*/
void gps_dl_iomem_dump(unsigned int dummy_addr, unsigned int dump_len);

#endif /* _GPS_DL_IOMEM_DUMP_H */

