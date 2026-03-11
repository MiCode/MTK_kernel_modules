
// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#if SUPPORT_BEIF
#include "btmtk_beif_plat.h"

#ifdef BEIF_CTP_LOAD
// Copy data from IO memory space to "real" memory space.
void memcpy_fromio(void *to, void *from, unsigned long count)
{
	if ((((phys_addr_t)to | (phys_addr_t)from) & 0x3) == 0) {
		for (; count > 3; count -= 4) {
			*(u32 *)to = *(volatile u32 *)from;
			to += 4;
			from += 4;
		}
	}

	for (; count > 0; count--) {
		*(u8 *)to = *(volatile u8 *)from;
		to++;
		from++;
	}

	mb();
}

// Copy data from "real" memory space to IO memory space.
void memcpy_toio(volatile void *to, const void *from, unsigned long count)
{
	if ((((phys_addr_t)to | (phys_addr_t)from) & 0x3) == 0) {
		for ( ; count > 3; count -= 4) {
			*(volatile u32 *)to = *(u32 *)from;
			to += 4;
			from += 4;
		}
	}

	for (; count > 0; count--) {
		*(volatile u8 *)to = *(u8 *)from;
		to++;
		from++;
	}

	mb();
}

// "memset" on IO memory space.
void memset_io(volatile void *dst, int c, unsigned long count)
{
	while (count > 0) {
		count -= 4;
		EMI_WRITE32(dst, c);
		dst += 4;
	}
}

void beif_get_local_time(u64 *sec, unsigned long *nsec)
{
	u64 ns = timer_get_ns();

	if (sec != NULL && nsec != NULL) {
		*sec = ns / 1000000;
		*nsec = ns % 1000000;
	} else
		pr_info("The input parameters error when get local time\n");
}

void *ioremap(phys_addr_t addr, unsigned int size)
{
	return (void *)addr;
}

void iounmap(void *addr)
{

}

void mutex_init(void *mutex)
{

}

void mutex_destroy(void *mutex)
{

}

int mutex_lock_killable(void *mutex)
{
	return 0;
}

int mutex_unlock(void *mutex)
{
	return 0;
}

#else
void beif_get_local_time(u64 *sec, unsigned long *nsec)
{
	if (sec != NULL && nsec != NULL) {
		*sec = local_clock();
		*nsec = do_div(*sec, 1000000000)/1000;
	} else
		pr_info("The input parameters error when get local time\n");
}

#endif
#endif