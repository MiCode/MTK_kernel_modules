/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#ifndef _GPS_DSP_FSM_H_
#define _GPS_DSP_FSM_H_

#include "gps_dl_config.h"

enum gps_dsp_state_t {
	/* GPS and DSP are function off */
	GPS_DSP_ST_OFF,             /* 0 */

	/* MCU turned DSP on and just released the reset flag */
	GPS_DSP_ST_TURNED_ON,       /* 1 */

	/* DSP notified MCU about it's reset flow done */
	/* It will also stand for DSP finished Mbist flow if DSP has this flow */
	/* MCU should suspend the data reoute until change to this state */
	/* When change to this state, MVCD flow should start immediately */
	GPS_DSP_ST_RESET_DONE,      /* 2 */

	/* DSP raw code download finish/DSP wakeup and raw code start working */
	GPS_DSP_ST_WORKING,         /* 3 */

	/* DSP in HW sleep mode for GLP duty cycle mode */
	GPS_DSP_ST_HW_SLEEP_MODE,   /* 4 */

	/* DSP in HW stop mode. MVCD flow can be skipped after wakeup */
	GPS_DSP_ST_HW_STOP_MODE,    /* 5 */

	/* Wakeup from sleep/stop mode */
	/* No MVCD procedure, should directly goto GPS_DSP_ST_WORKING */
	GPS_DSP_ST_WAKEN_UP,        /* 6 */

	GPS_DSP_ST_MAX
};


/* DSP to MCU MCUB flag bit usage definition */
#define GPS_MCUB_D2AF_MASK_DSP_RAMCODE_READY        0x1  /* bit0 */
#define GPS_MCUB_D2AF_MASK_DSP_REG_READ_READY       0x2  /* bit1 */
#define GPS_MCUB_D2AF_MASK_DSP_REQUEST_MCU_ACTION   0x4  /* bit2 */
/* To be renamed to GPS_MCUB_D2AF_MASK_DSP_RESET_DONE */
/* #define GPS_MCUB_D2AF_MASK_DSP_MBIST_READY          0b1000 */
#define GPS_MCUB_D2AF_MASK_DSP_RESET_DONE           0x8  /* bit3 */


/* MCU to DSP MCUB flag bit usage definition */
#define GPS_MCUB_A2DF_MASK_DSP_DCLK_79MHZ_REQ       0x1  /* bit0 */
#define GPS_MCUB_A2DF_MASK_DSP_DCLK_88MHZ_REQ       0x2  /* bit1 */
#define GPS_MCUB_A2DF_MASK_DSP_REG_READ_REQ         0x4  /* bit2 */
/* Will be available on MT6779. The previous chips don't use it */
#define GPS_MCUB_A2DF_MASK_DSP_SET_CFG_REQ          0x8  /* bit3 */


enum gps_dsp_event_t {
	GPS_DSP_EVT_FUNC_OFF,                   /* 0 From host */
	GPS_DSP_EVT_FUNC_ON,                    /* 1 From host */
	GPS_DSP_EVT_RESET_DONE,                 /* 2 From DSP D2AF[3] */
	GPS_DSP_EVT_RAM_CODE_READY,             /* 3 From DSP D2AF[0] */
	GPS_DSP_EVT_CTRL_TIMER_EXPIRE,          /* 4 From timer */
	GPS_DSP_EVT_HW_SLEEP_REQ,               /* 5 From DSP D2AF[2] - DSP request it; from host for test */
	GPS_DSP_EVT_HW_SLEEP_EXIT,              /* 6 From host - just for test */
	GPS_DSP_EVT_HW_STOP_REQ,                /* 7 From host */
	GPS_DSP_EVT_HW_STOP_EXIT,               /* 8 From host */
	GPS_DSP_EVT_MAX
};

#define GPS_DSP_RESET_TIMEOUT_MS        (180)
#define GPS_DSP_MVCD_TIMEOUT_MS         (1000)
#define GPS_DSP_WAKEUP_TIMEOUT_MS       (180)

enum dsp_ctrl_enum {
	GPS_L1_DSP_ON,
	GPS_L1_DSP_OFF,
	GPS_L5_DSP_ON,
	GPS_L5_DSP_OFF,
	GPS_L1_DSP_ENTER_DSLEEP,
	GPS_L1_DSP_EXIT_DSLEEP,
	GPS_L1_DSP_ENTER_DSTOP,
	GPS_L1_DSP_EXIT_DSTOP,
	GPS_L5_DSP_ENTER_DSLEEP,
	GPS_L5_DSP_EXIT_DSLEEP,
	GPS_L5_DSP_ENTER_DSTOP,
	GPS_L5_DSP_EXIT_DSTOP,
	GPS_L1_DSP_CLEAR_PWR_STAT,
	GPS_L5_DSP_CLEAR_PWR_STAT,
	GPS_DSP_CTRL_MAX
};

#define GPS_DSP_STATE_HISTORY_ITEM_MAX (16)

struct gps_dsp_state_history_item_t {
	unsigned long tick;
	enum gps_dsp_state_t state;
};

struct gps_dsp_state_history_struct_t {
	struct gps_dsp_state_history_item_t items[GPS_DSP_STATE_HISTORY_ITEM_MAX];
	/* index may larger than GPS_DSP_STATE_HISTORY_ITEM_MAX, only index % GPS_DSP_STATE_HISTORY_ITEM_MAX*/
	/*can be used for indexing of items*/
	unsigned int index;
};

bool gps_dsp_state_is_dump_needed_for_reset_done(enum gps_dl_link_id_enum link_id);
enum gps_dsp_state_t gps_dsp_history_state_get(enum gps_dl_link_id_enum link_id, unsigned int item_index);
enum gps_dsp_state_t gps_dsp_state_get(enum gps_dl_link_id_enum link_id);
void gps_dsp_state_change_to(enum gps_dsp_state_t state, enum gps_dl_link_id_enum link_id);
bool gps_dsp_state_is(enum gps_dsp_state_t state, enum gps_dl_link_id_enum link_id);

void gps_dsp_fsm(enum gps_dsp_event_t evt, enum gps_dl_link_id_enum link_id);

#endif /* _GPS_DSP_FSM_H_ */

