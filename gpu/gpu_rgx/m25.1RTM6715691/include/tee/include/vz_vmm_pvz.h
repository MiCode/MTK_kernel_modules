/*************************************************************************/ /*!
@File           vz_vmm_pvz.h
@Title          System virtualization VM manager management APIs
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    This header provides VM manager para-virtz management APIs
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#ifndef VZ_VMM_PVZ_H
#define VZ_VMM_PVZ_H

#include "img_types.h"

/*!
*******************************************************************************
 @Function      PvzConfigInit()
 @Description   Initialises the driver-wide paravirtualisation infrastructure.
                It is device-agnostic and is called once on driver load.
                This init does NOT include pvz connection init, necessary to
                communicate with the VMM or other drivers running inside VMs.
 @Return        PVRSRV_OK on success. Otherwise, a PVRSRV error code
******************************************************************************/
PVRSRV_ERROR PvzConfigInit(void);

/*!
*******************************************************************************
 @Function      PvzConfigDeInit()
 @Description   Deinitialises the entire paravirtualisation functionality,
                including the pvz connections.
                It is device-agnostic and is called on driver unload.
******************************************************************************/
void PvzConfigDeInit(void);

/*!
*******************************************************************************
 @Function      PvzConnectionInit()
 @Description   Initialises the paravirtualisation connection with the VMM,
                if applicable. There are two types of pvz interfaces:
                the server interface used by VZ Hosts and the client
                interface used by VZ Guests. A driver usually determines
                which role(s) it needs to fulfil at probe time, when the
                devices are enumerated. This function is called on every
                PVRSRVCommonDeviceCreate() to ensure that the pvz interface
                type required by that device is initialised.
                Once open, a pvz connection remains in effect until driver
                exit, regardless of the state of individual devices.
 @Return        PVRSRV_OK on success. Otherwise, a PVRSRV error code
******************************************************************************/
PVRSRV_ERROR PvzConnectionInit(PVRSRV_DRIVER_MODE eDriverMode);

/*!
*******************************************************************************
 @Function      PvzServerLockAcquire()
 @Description   Acquire the PVZ server lock
******************************************************************************/
void PvzServerLockAcquire(void);

/*!
*******************************************************************************
 @Function      PvzServerLockRelease()
 @Description   Release the PVZ server lock
******************************************************************************/
void PvzServerLockRelease(void);

/*!
*******************************************************************************
 @Function      PvzClientLockAcquire()
 @Description   Acquire the PVZ client lock
******************************************************************************/
void PvzClientLockAcquire(void);

/*!
*******************************************************************************
 @Function      PvzClientLockRelease()
 @Description   Release the PVZ client lock
******************************************************************************/
void PvzClientLockRelease(void);

#endif /* VZ_VMM_PVZ_H */

/******************************************************************************
 End of file (vz_vmm_pvz.h)
******************************************************************************/
