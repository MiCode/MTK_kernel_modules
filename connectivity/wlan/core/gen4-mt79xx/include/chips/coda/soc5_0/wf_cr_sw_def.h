/*
** $Id: @(#) wf_cr_sw_def.h $
*/

/*! \file   "wf_cr_sw_def.h"
    \brief
*/

/*******************************************************************************
* Copyright (c) 2009 MediaTek Inc.
*
* All rights reserved. Copying, compilation, modification, distribution
* or any other use whatsoever of this material is strictly prohibited
* except in accordance with a Software License Agreement with
* MediaTek Inc.
********************************************************************************
*/

/*******************************************************************************
* LEGAL DISCLAIMER
*
* BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND
* AGREES THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK
* SOFTWARE") RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
* PROVIDED TO BUYER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY
* DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
* LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE
* ANY WARRANTY WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY
* WHICH MAY BE USED BY, INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK
* SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY
* WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE
* FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION OR TO
* CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
* BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
* LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL
* BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
* ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
* BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
* THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
* WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT
* OF LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING
* THEREOF AND RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN
* FRANCISCO, CA, UNDER THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE
* (ICC).
********************************************************************************
*/

#ifndef _WF_CR_SW_DEF_H
#define _WF_CR_SW_DEF_H

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


//****************************************************************************
//
//                     MCU_SYSRAM SW CR Definitions
//
//****************************************************************************
extern UINT_32 __sw_def_cr_begin;

#define WF_SW_DEF_CR_SER_STATUS_ADDR            ((volatile kal_uint32 *)&__sw_def_cr_begin) 	// 0x00401A00
#define WF_SW_DEF_CR_PLE_STATUS_ADDR            ((volatile kal_uint32 *)&__sw_def_cr_begin+4)
#define WF_SW_DEF_CR_PLE1_STATUS_ADDR           ((volatile kal_uint32 *)&__sw_def_cr_begin+8)
#define WF_SW_DEF_CR_PLE_AMSDU_STATUS_ADDR      ((volatile kal_uint32 *)&__sw_def_cr_begin+12)
#define WF_SW_DEF_CR_PSE_STATUS_ADDR            ((volatile kal_uint32 *)&__sw_def_cr_begin+16)
#define WF_SW_DEF_CR_PSE1_STATUS_ADDR           ((volatile kal_uint32 *)&__sw_def_cr_begin+20)
#define WF_SW_DEF_CR_LAMC_WISR6_BN0_STATUS_ADDR ((volatile kal_uint32 *)&__sw_def_cr_begin+24)
#define WF_SW_DEF_CR_LAMC_WISR6_BN1_STATUS_ADDR ((volatile kal_uint32 *)&__sw_def_cr_begin+28)
#define WF_SW_DEF_CR_LAMC_WISR7_BN0_STATUS_ADDR ((volatile kal_uint32 *)&__sw_def_cr_begin+32)
#define WF_SW_DEF_CR_LAMC_WISR7_BN1_STATUS_ADDR ((volatile kal_uint32 *)&__sw_def_cr_begin+36)
#define WF_SW_DEF_CR_USB_MCU_EVENT_ADD          ((volatile kal_uint32 *)&__sw_def_cr_begin+40)
#define WF_SW_DEF_CR_USB_HOST_ACK_ADDR          ((volatile kal_uint32 *)&__sw_def_cr_begin+44)
#define WF_SW_DEF_CR_ICAP_SPECTRUM_MODE_ADDR    ((volatile kal_uint32 *)&__sw_def_cr_begin+48)

#endif /* _WF_CR_SW_DEF_H */


