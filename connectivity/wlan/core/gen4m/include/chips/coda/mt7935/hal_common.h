/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef ___HAL_COMMON_H__
#define ___HAL_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PACKING
//typedef unsigned int FIELD;
#define FIELD unsigned int


#define AP2CONN_REMAPPING_OFFSET 0x3FC00000
#define CONN_INFRA_REMAPPING_OFFSET 0x64000000
#define CONN_INFRA_ON_REMAPPING_OFFSET 0x08000000

// ---------------------------------------------------------------------------
//  Register Manipulations
// ---------------------------------------------------------------------------

#ifndef CONFIG_MCU_X86_SIMULATION

#define IO_R_32(reg) \
    (*(volatile UINT32 * const)(reg))

#define IO_W_32(reg, val) \
    (*(volatile UINT32 * const)(reg)) = (val)

#define IO_R_16(reg) \
    (*(volatile UINT16 * const)(reg))

#define IO_W_16(reg, val) \
    (*(volatile UINT16 * const)(reg)) = (val)

#define IO_R_8(reg) \
    (*(volatile UINT8 * const)(reg))

#define IO_W_8(reg, val) \
    (*(volatile UINT8 * const)(reg)) = (val)

#else  //CONFIG_MCU_X86_SIMULATION

#include <stdio.h>

#define IO_R_32(reg)      x86_IO_R_32((kal_uint32)(reg))
#define IO_W_32(reg, val) x86_IO_W_32((kal_uint32)(reg), (kal_uint32)(val))
#define IO_R_16(reg)      x86_IO_R_16((kal_uint32)(reg))
#define IO_W_16(reg, val) x86_IO_W_16((kal_uint32)(reg), (kal_uint32)(val))
#define IO_R_8(reg)       x86_IO_R_8((kal_uint32)(reg))
#define IO_W_8(reg, val)  x86_IO_W_8((kal_uint32)(reg), (kal_uint32)(val))

extern kal_uint32 x86_IO_R_32(kal_uint32 reg);
extern kal_uint16 x86_IO_R_16(kal_uint32 reg);
extern kal_uint8  x86_IO_R_8(kal_uint32 reg);
extern kal_uint32 x86_IO_W_32(kal_uint32 reg, kal_uint32 val);
extern kal_uint16 x86_IO_W_16(kal_uint32 reg, kal_uint32 val);
extern kal_uint8  x86_IO_W_8(kal_uint32 reg, kal_uint32 val);

#endif //CONFIG_MCU_X86_SIMULATION

#define IO_CLR_BIT_32(reg, val) IO_W_32(reg, (IO_R_32(reg) & (~(val))))
#define IO_SET_BIT_32(reg, val) IO_W_32(reg, (IO_R_32(reg) | (val)))

#define VAR_GET_FIELD_32(var, low, high)                    \
    ({                                                          \
        STATIC_ASSERT((low) <= (high) && (high) <= 31);         \
        ((var) & BITS((low), (high)))>>(low);          \
    })

#define VAR_SET_FIELD_32(var, val, low, high)                        \
    do{                                                             \
        STATIC_ASSERT((low) <= (high) && (high) <= 31);             \
        STATIC_ASSERT(((val) >> ((high) - (low) + 1)) == 0);      \
        (var) &= ~BITS((low), (high));                              \
        (var) |= ((val)<<(low));                                    \
    }while(0)



#ifdef __cplusplus
}
#endif


#endif // ___HAL_COMMON_H__
