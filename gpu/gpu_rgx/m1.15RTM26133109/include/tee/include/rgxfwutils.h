/*************************************************************************/ /*!
@File
@Title          RGX firmware utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX firmware utility routines
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

#ifndef RGXFWUTILS_H
#define RGXFWUTILS_H

#if defined(SUPPORT_AUTOVZ_HW_REGS) && !defined(SUPPORT_AUTOVZ)
#error "VZ build configuration error: use of OS scratch registers supported only in AutoVz drivers."
#endif

#if defined(SUPPORT_AUTOVZ_HW_REGS)
/* AutoVz with hw support */
#define KM_GET_FW_CONNECTION(param)				OSReadHWReg32(param->pvRegsBaseKM, RGX_CR_OS0_SCRATCH3)
#define KM_GET_OS_CONNECTION(param)				OSReadHWReg32(param->pvRegsBaseKM, RGX_CR_OS0_SCRATCH2)
#define KM_SET_OS_CONNECTION(val, param)		OSWriteHWReg32(param->pvRegsBaseKM, RGX_CR_OS0_SCRATCH2, RGXFW_CONNECTION_OS_##val)

#define KM_GET_FW_ALIVE_TOKEN(param)			OSReadHWReg32(param->pvRegsBaseKM, RGX_CR_OS0_SCRATCH1)
#define KM_GET_OS_ALIVE_TOKEN(param)			OSReadHWReg32(param->pvRegsBaseKM, RGX_CR_OS0_SCRATCH0)
#define KM_SET_OS_ALIVE_TOKEN(val, param)		OSWriteHWReg32(param->pvRegsBaseKM, RGX_CR_OS0_SCRATCH0, val)
#else

#if defined(SUPPORT_AUTOVZ)
#define KM_GET_FW_ALIVE_TOKEN(param)			(param->psRGXFWIfConnectionCtl->ui32AliveFwToken)
#define KM_GET_OS_ALIVE_TOKEN(param)			(param->psRGXFWIfConnectionCtl->ui32AliveOsToken)
#define KM_SET_OS_ALIVE_TOKEN(val, param)		OSWriteDeviceMem32WithWMB(&param->psRGXFWIfConnectionCtl->ui32AliveOsToken, val)
#endif /* defined(SUPPORT_AUTOVZ) */

#if !defined(NO_HARDWARE) && (defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS) || (defined(RGX_NUM_OS_SUPPORTED) && (RGX_NUM_OS_SUPPORTED == 1)))
/* native, static-vz and AutoVz using shared memory */
#define KM_GET_FW_CONNECTION(param)			(param->psRGXFWIfConnectionCtl->eConnectionFwState)
#define KM_GET_OS_CONNECTION(param)			(param->psRGXFWIfConnectionCtl->eConnectionOsState)
#define KM_SET_OS_CONNECTION(val, param)	OSWriteDeviceMem32WithWMB(&param->psRGXFWIfConnectionCtl->eConnectionOsState, RGXFW_CONNECTION_OS_##val)
#else
/* dynamic-vz & nohw */
#define KM_GET_FW_CONNECTION(param)			(RGXFW_CONNECTION_FW_ACTIVE)
#define KM_GET_OS_CONNECTION(param)			(RGXFW_CONNECTION_OS_ACTIVE)
#define KM_SET_OS_CONNECTION(val, param)
#endif /* defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS) || (RGX_NUM_OS_SUPPORTED == 1) */
#endif /* defined(SUPPORT_AUTOVZ_HW_REGS) */

#if defined(SUPPORT_AUTOVZ)
#define RGX_FIRST_RAW_HEAP_OSID		RGXFW_HOST_OS
#else
#define RGX_FIRST_RAW_HEAP_OSID		RGXFW_GUEST_OSID_START
#endif

#define KM_OS_CONNECTION_IS(val, param)		(KM_GET_OS_CONNECTION(param) == RGXFW_CONNECTION_OS_##val)
#define KM_FW_CONNECTION_IS(val, param)		(KM_GET_FW_CONNECTION(param) == RGXFW_CONNECTION_FW_##val)

#endif /* RGXFWUTILS_H */
/******************************************************************************
 End of file (rgxfwutils.h)
******************************************************************************/
