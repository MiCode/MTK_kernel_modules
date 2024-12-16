/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
** Id: //Department/DaVinci/TRUNK/WiFi_P2P_Driver/include/mgmt/p2p_fsm.h#23
*/

/*! \file   p2p_fsm.h
 *  \brief  Declaration of functions and finite state machine for P2P Module.
 *
 *  Declaration of functions and finite state machine for P2P Module.
 */


#ifndef _P2P_FSM_H
#define _P2P_FSM_H

VOID p2pFsmRunEventScanRequest(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

VOID p2pFsmRunEventChGrant(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

VOID p2pFsmRunEventNetDeviceRegister(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

VOID p2pFsmRunEventUpdateMgmtFrame(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);

#if CFG_SUPPORT_WFD
VOID p2pFsmRunEventWfdSettingUpdate(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);
#endif

VOID p2pFsmRunEventScanDone(IN P_ADAPTER_T prAdapter, IN P_MSG_HDR_T prMsgHdr);


#endif /* _P2P_FSM_H */
