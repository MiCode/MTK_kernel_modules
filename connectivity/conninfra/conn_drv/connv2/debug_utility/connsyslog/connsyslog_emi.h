/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef _CONNSYSLOG_EMI_H_
#define _CONNSYSLOG_EMI_H_

//
// ---------- |---------------------------------------------------------
//            |    |  4 | primary base offset
//            |    |-----------------------------------------------------
//            |    |  4 | primary size
//            |    |-----------------------------------------------------
//  Header    | 64 |  4 | mcu base offset
//            |    |-----------------------------------------------------
//            |    |  4 | mcu size
//            |    |-----------------------------------------------------
//            |    | 16 | Reserved
//            |    |-----------------------------------------------------
//            |    |  4 | suspend/resume state
//            |    |-----------------------------------------------------
//            |    | 12 | Reserved
//            |    |-----------------------------------------------------
//            |    |  4 | irq counter
//            |    |-----------------------------------------------------
//            |    |  4 | Reserved
//            |    |-----------------------------------------------------
//            |    |  8 | ready pattern
// ---------- |---------------------------------------------------------
//            |    |  4 | read_offset
//            |    |-----------------------------------------------------
//            | 32 |  4 | write_offset
//            |    |-----------------------------------------------------
//  Primary   |    | 24 | reserved
//            |----------------------------------------------------------
//            |    |
//            |  x | buffer
//            |    |
//            |----------------------------------------------------------
//            | 32 | END_PATTERN
// ---------- |----------------------------------------------------------
//            |    |  4 | read_offset
//            |    |-----------------------------------------------------
//            | 32 |  4 | write_offset
//            |    |-----------------------------------------------------
//  MCU       |    | 24 | reserved
//            |----------------------------------------------------------
//            |    |
//            |  x | buffer
//            |    |
//            |----------------------------------------------------------
//            | 32 | END_PATTERN
// ---------- |----------------------------------------------------------
//
//


#define CONNLOG_EMI_32_BYTE_ALIGNED					32 /* connsys EMI cache is 32-byte aligned */

#define CONNLOG_CONTROL_RING_BUFFER_BASE_SIZE		64 /* Reserve for setup ring buffer base address  */
#define CONNLOG_CONTROL_RING_BUFFER_RESERVE_SIZE 32
#define CONNLOG_IRQ_COUNTER_BASE 48
#define CONNLOG_READY_PATTERN_BASE 56
#define CONNLOG_READY_PATTERN_BASE_SIZE 8
#define CONNLOG_EMI_END_PATTERN_SIZE CONNLOG_EMI_32_BYTE_ALIGNED
#define CONNLOG_EMI_END_PATTERN "FWLOGENDFWLOGENDFWLOGENDFWLOGEND"

#define CONNLOG_EMI_BASE_OFFSET CONNLOG_CONTROL_RING_BUFFER_BASE_SIZE
#define CONNLOG_EMI_READ         (CONNLOG_EMI_BASE_OFFSET + 0)
#define CONNLOG_EMI_WRITE        (CONNLOG_EMI_BASE_OFFSET + 4)
#define CONNLOG_EMI_BUF          (CONNLOG_EMI_BASE_OFFSET + \
				  CONNLOG_EMI_32_BYTE_ALIGNED)

#define CONNLOG_EMI_READ_OFFSET(base)	(base + 0)
#define CONNLOG_EMI_WRITE_OFFSET(base)	(base + 4)
#define CONNLOG_EMI_BUF_OFFSET(base)	(base + CONNLOG_EMI_32_BYTE_ALIGNED)
#define CONNLOG_EMI_AP_STATE_OFFSET(base)	(base + 32)

#define CONNLOG_EMI_SUB_HEADER  CONNLOG_EMI_32_BYTE_ALIGNED

#endif /* _CONNSYSLOG_EMI_H_  */
