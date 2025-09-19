/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_rst.h
 *    \brief  Declaration of functions and finite state machine for
 *	    MT6620 Whole-Chip Reset Mechanism
 */

#ifndef _GL_RST_H
#define _GL_RST_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_typedef.h"

#if CFG_MTK_ANDROID_WMT && CFG_SUPPORT_CONNAC1X
#include "wmt_exp.h"
#endif

#if (CFG_SUPPORT_CONNINFRA == 1)
#include "conninfra.h"
#endif

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#include "conninfra.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define RST_FLAG_CHIP_RESET        0
#define RST_FLAG_DO_CORE_DUMP              BIT(0)
#define RST_FLAG_PREVENT_POWER_OFF         BIT(1)
#define RST_FLAG_DO_WHOLE_RESET            BIT(2)
#define RST_FLAG_DO_L0P5_RESET             BIT(3)
#define RST_FLAG_DO_L1_RESET               BIT(4)

#if CFG_CHIP_RESET_HANG
#define SER_L0_HANG_RST_NONE		0
#define SER_L0_HANG_RST_TRGING		1
#define SER_L0_HANG_RST_HAND_DISABLE	2
#define SER_L0_HANG_RST_HANG		3
#define SER_L0_HANG_RST_CMD_TRG		9

#define SER_L0_HANG_LOG_TIME_INTERVAL	3000
#endif

#ifndef CFG_SUPPORT_SER_DEBUGFS
#define CFG_SUPPORT_SER_DEBUGFS		0
#endif

#define WIFI_TRIGGER_ASSERT_TIMEOUT 2000
#define GLUE_FLAG_RST_PROCESS (GLUE_FLAG_HALT |\
				GLUE_FLAG_RST_START |\
				GLUE_FLAG_RST_END |\
				GLUE_FLAG_RST_FW_NOTIFY_L0 |\
				GLUE_FLAG_RST_FW_NOTIFY_L05)
#define RST_FLAG_WHOLE_RESET  (RST_FLAG_DO_CORE_DUMP | \
			       RST_FLAG_PREVENT_POWER_OFF |\
			       RST_FLAG_DO_WHOLE_RESET)
#define RST_FLAG_WF_RESET  (RST_FLAG_DO_CORE_DUMP | RST_FLAG_PREVENT_POWER_OFF)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
enum ENUM_RESET_STATUS {
	RESET_FAIL,
	RESET_SUCCESS
};

enum _ENUM_CHIP_RESET_REASON_TYPE_T {
	RST_PROCESS_ABNORMAL_INT = 1,
	RST_DRV_OWN_FAIL,
	RST_FW_ASSERT,
	RST_FW_ASSERT_TIMEOUT,
	RST_BT_TRIGGER,
	RST_OID_TIMEOUT,
	RST_CMD_TRIGGER,
	RST_REQ_CHL_FAIL,
	RST_FW_DL_FAIL,
	RST_SER_TIMEOUT,
	RST_SLP_PROT_TIMEOUT,
	RST_REG_READ_DEADFEED,
	RST_P2P_CHNL_GRANT_INVALID_TYPE,
	RST_P2P_CHNL_GRANT_INVALID_STATE,
	RST_SCAN_RECOVERY,
	RST_ACCESS_REG_FAIL,
	RST_WIFI_ON_DRV_OWN_FAIL,
	RST_CHECK_READY_BIT_TIMEOUT,
	RST_ALLOC_CMD_FAIL,
	RST_SDIO_RX_ERROR,
	RST_WHOLE_CHIP_TRIGGER,
	RST_MDDP_EXCEPTION,
	RST_MDDP_MD_TRIGGER_EXCEPTION,
	RST_FWK_TRIGGER,
	RST_SER_L1_FAIL,
	RST_SER_L0P5_FAIL,
	RST_CMD_EVT_FAIL,
	RST_WDT,
	RST_SUBSYS_BUS_HANG,
	RST_SMC_CMD_FAIL,
	RST_DEVAPC,
	RST_PCIE_NOT_READY,
	RST_AER,
	RST_AER_MALFTLP,
	RST_AER_RXERR,
	RST_AER_SDES,
	RST_MMIO_READ,
	RST_WFDMA_RX_HANG,
	RST_MAWD_WAKEUP_FAIL,
	RST_RFB_FAIL,
	RST_REASON_MAX
};

/* L0.5 reset state */
enum ENUM_WFSYS_RESET_STATE_TYPE_T {
	WFSYS_RESET_STATE_IDLE = 0,
	WFSYS_RESET_STATE_DETECT,
	WFSYS_RESET_STATE_RESET,
	WFSYS_RESET_STATE_REINIT,
	WFSYS_RESET_STATE_POSTPONE,
	WFSYS_RESET_STATE_MAX
};

#if CFG_WMT_RESET_API_SUPPORT
struct reset_pending_req {
	uint32_t flag;
	uint8_t file[256];
	uint32_t line;
	u_int8_t fw_acked;
};
#endif

struct RESET_STRUCT {
	struct GLUE_INFO *prGlueInfo;
	struct work_struct rst_work;
#if CFG_WMT_RESET_API_SUPPORT
	unsigned long ulFlag;
	u_int8_t force_dump;
	enum ENUM_RESET_STATUS rst_data;
	struct work_struct rst_trigger_work;
	uint32_t rst_trigger_flag;
	struct completion halt_comp;
	struct notifier_block pm_nb;
	u_int8_t is_suspend;
	struct reset_pending_req *pending_req;
#endif
	u_int8_t fgIsInitialized;
};

enum ENUM_RST_MSG {
	ENUM_RST_MSG_L0_START = 0x0,
	ENUM_RST_MSG_L0_END,
	ENUM_RST_MSG_L04_START,
	ENUM_RST_MSG_L04_END,
	ENUM_RST_MSG_L05_START,
	ENUM_RST_MSG_L05_END,
	ENUM_RST_MSG_NUM
};

/*******************************************************************************
 *                    E X T E R N A L   F U N C T I O N S
 *******************************************************************************
 */
#if CFG_CHIP_RESET_SUPPORT
#if CFG_MTK_ANDROID_WMT
extern void update_driver_reset_status(uint8_t fgIsResetting);
extern int32_t get_wifi_process_status(void);
extern int32_t get_wifi_powered_status(void);
extern int wifi_reset_start(void);
extern int wifi_reset_end(enum ENUM_RESET_STATUS);
extern void update_whole_chip_rst_status(uint8_t fgIsL0Resetting);
#endif
#endif /* CFG_CHIP_RESET_SUPPORT */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if CFG_CHIP_RESET_SUPPORT
extern u_int8_t fgIsResetting;
extern u_int8_t fgIsRstPreventFwOwn;
extern enum COREDUMP_SOURCE_TYPE g_Coredump_source;

#if CFG_CHIP_RESET_HANG
extern u_int8_t fgIsResetHangState;
#endif

#endif
/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#if CFG_CHIP_RESET_SUPPORT
/* Each reset trigger reason has corresponding default reset action, which is
 * defined in glResetSelectAction(). You can use this macro to trigger default
 * action.
 */
#define GL_DEFAULT_RESET_TRIGGER(_prAdapter, _eReason)		\
({ \
	uint32_t ret; \
	glSetRstReason(_eReason);    \
	ret = glResetTrigger(_prAdapter,    \
		       glResetSelectAction(_prAdapter),    \
		       (const uint8_t *)__FILE__, __LINE__);    \
	ret; \
})

/* You can use this macro to trigger user defined reset actions instead of the
 * default ones.
 */
#define GL_USER_DEFINE_RESET_TRIGGER(_prAdapter, _eReason, _u4Flags)    \
do { \
	glSetRstReason(_eReason);    \
	glResetTrigger(_prAdapter, _u4Flags,	\
		       (const uint8_t *)__FILE__, __LINE__);    \
} while (FALSE)
#else
#define GL_DEFAULT_RESET_TRIGGER(_prAdapter, _eReason) \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n")

#define GL_USER_DEFINE_RESET_TRIGGER(_prAdapter, _eReason, _u4Flags) \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n")
#endif

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
#if CFG_CHIP_RESET_SUPPORT
extern uint64_t u8ResetTime;
extern u_int8_t fgSimplifyResetFlow;
extern char *g_reason;
#endif
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void glSetRstReason(enum _ENUM_CHIP_RESET_REASON_TYPE_T eReason);
int glGetRstReason(void);

u_int8_t kalIsResetting(void);
u_int8_t kalIsResetOnEnd(void);
u_int8_t kalIsRstPreventFwOwn(void);

void glResetUpdateFlag(u_int8_t fgIsResetting);
u_int8_t glIsFwAsserted(void);
void glResetUpdateFwAsserted(u_int8_t isFwAsserted);

#if CFG_CHIP_RESET_SUPPORT
void glResetInit(struct GLUE_INFO *prGlueInfo);

void glResetUninit(void);

void glReseProbeRemoveDone(struct GLUE_INFO *prGlueInfo, int32_t i4Status,
			   u_int8_t fgIsProbe);

void glSendResetRequest(void);

void glResetWholeChipResetTrigger(char *pcReason);

uint32_t glResetSelectAction(struct ADAPTER *prAdapter);

uint32_t glResetTrigger(struct ADAPTER *prAdapter,
		    uint32_t u4RstFlag, const uint8_t *pucFile,
		    uint32_t u4Line);

#if CFG_CHIP_RESET_KO_SUPPORT
void resetkoNotifyFunc(unsigned int event, void *data);
void resetkoReset(void);
#endif

#if CFG_WMT_RESET_API_SUPPORT
int32_t glIsWmtCodeDump(void);
int wlan_reset_thread_main(void *data);
#if (CFG_SUPPORT_CONNINFRA == 1)
int glRstwlanPreWholeChipReset(enum consys_drv_type type, char *reason);
int glRstwlanPostWholeChipReset(void);
#endif /* CFG_SUPPORT_CONNINFRA */
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
int wlan_pre_whole_chip_rst_v3(enum connv3_drv_type drv,
	char *reason, unsigned int reset_type);
int wlan_post_whole_chip_rst_v3(void);
int wlan_pre_whole_chip_rst_v2(enum consys_drv_type drv,
	char *reason);
int wlan_post_whole_chip_rst_v2(void);
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
int wlan_post_reset_on_v3(unsigned int type);
#endif
#endif
u_int8_t kalIsWholeChipResetting(void);
void glSetRstReasonString(char *reason);
void kalSetRstEvent(u_int8_t force_dump);
void kalSetRstFwNotifyL05Event(u_int8_t force_dump);
void kalSetRstFwNotifyTriggerL0Event(u_int8_t force_dump);
void glRstSetRstEndEvent(void);
int reset_wait_for_trigger_completion(void);
void reset_done_trigger_completion(void);
void glSetIsNeedWaitCoredumpFlag(uint8_t status);
#else
void glSetWfsysResetState(struct ADAPTER *prAdapter,
			  enum ENUM_WFSYS_RESET_STATE_TYPE_T state);
u_int8_t glReSchWfsysReset(struct ADAPTER *prAdapter);

void WfsysResetHdlr(struct work_struct *work);
#endif /* CFG_WMT_RESET_API_SUPPORT */
#endif /* CFG_CHIP_RESET_SUPPORT */
#endif /* _GL_RST_H */
