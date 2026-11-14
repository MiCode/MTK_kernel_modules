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
#define KM_GET_FW_CONNECTION(psDevInfo)				OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH3)
#define KM_GET_OS_CONNECTION(psDevInfo)				OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH2)
#define KM_SET_OS_CONNECTION(val, psDevInfo)		OSWriteHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH2, RGXFW_CONNECTION_OS_##val)

#define KM_GET_FW_ALIVE_TOKEN(psDevInfo)			OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH1)
#define KM_GET_OS_ALIVE_TOKEN(psDevInfo)			OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH0)
#define KM_SET_OS_ALIVE_TOKEN(val, psDevInfo)		OSWriteHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_OS0_SCRATCH0, val)

#define KM_ALIVE_TOKEN_CACHEOP(Target, CacheOp)
#define KM_CONNECTION_CACHEOP(Target, CacheOp)

#else

#if defined(SUPPORT_AUTOVZ)
#define KM_GET_FW_ALIVE_TOKEN(psDevInfo)			(psDevInfo->psRGXFWIfConnectionCtl->ui32AliveFwToken)
#define KM_GET_OS_ALIVE_TOKEN(psDevInfo)			(psDevInfo->psRGXFWIfConnectionCtl->ui32AliveOsToken)
#define KM_SET_OS_ALIVE_TOKEN(val, psDevInfo)		do { \
                                                    OSWriteDeviceMem32WithWMB((volatile IMG_UINT32 *) &psDevInfo->psRGXFWIfConnectionCtl->ui32AliveOsToken, val); \
                                                    KM_ALIVE_TOKEN_CACHEOP(Os, FLUSH); \
                                                    } while (0)

#define KM_ALIVE_TOKEN_CACHEOP(Target, CacheOp)     RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfConnectionCtl->ui32Alive##Target##Token, \
													                           CacheOp);
#endif /* defined(SUPPORT_AUTOVZ) */

#if !defined(NO_HARDWARE) && defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
/* static, dynamic and AutoVz DDKs using shared memory */
#define KM_GET_FW_CONNECTION(psDevInfo)			(psDevInfo->psRGXFWIfConnectionCtl->eConnectionFwState)
#define KM_GET_OS_CONNECTION(psDevInfo)			(psDevInfo->psRGXFWIfConnectionCtl->eConnectionOsState)
#define KM_SET_OS_CONNECTION(val, psDevInfo)	do { \
                                                OSWriteDeviceMem32WithWMB((void*)&psDevInfo->psRGXFWIfConnectionCtl->eConnectionOsState, RGXFW_CONNECTION_OS_##val); \
                                                KM_CONNECTION_CACHEOP(Os, FLUSH); \
                                                } while (0)

#define KM_CONNECTION_CACHEOP(Target, CacheOp)  RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfConnectionCtl->eConnection##Target##State, \
												                           CacheOp);
#else
/* nohw & native */
#define KM_GET_FW_CONNECTION(psDevInfo)			(RGXFW_CONNECTION_FW_ACTIVE)
#define KM_GET_OS_CONNECTION(psDevInfo)			(RGXFW_CONNECTION_OS_ACTIVE)
#define KM_SET_OS_CONNECTION(val, psDevInfo)
#define KM_CONNECTION_CACHEOP(Target, CacheOp)
#endif /* defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS) || (RGX_NUM_DRIVERS_SUPPORTED == 1) */
#endif /* defined(SUPPORT_AUTOVZ_HW_REGS) */

#if defined(RGX_PREMAP_FW_HEAPS)
#define RGX_FIRST_RAW_HEAP_DRIVER_ID		RGXFW_HOST_DRIVER_ID
#else
#define RGX_FIRST_RAW_HEAP_DRIVER_ID		RGXFW_GUEST_DRIVER_ID_START
#endif

#define KM_OS_CONNECTION_IS(val, psDevInfo)		(KM_GET_OS_CONNECTION(psDevInfo) == RGXFW_CONNECTION_OS_##val)
#define KM_FW_CONNECTION_IS(val, psDevInfo)		(KM_GET_FW_CONNECTION(psDevInfo) == RGXFW_CONNECTION_FW_##val)

#endif /* RGXFWUTILS_H */
/******************************************************************************
 End of file (rgxfwutils.h)
******************************************************************************/
