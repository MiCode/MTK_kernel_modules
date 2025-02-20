/*************************************************************************/ /*!
@File
@Title          Services definitions required by external drivers
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Provides services data structures, defines and prototypes
                required by external drivers
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

#if !defined(SERVICESEXT_H)
#define SERVICESEXT_H

/* include/ */
#include "pvrsrv_error.h"
#include "img_types.h"
#include "img_3dtypes.h"
#include "pvrsrv_device_types.h"

/*
 * Lock buffer read/write flags
 */
#define PVRSRV_LOCKFLG_READONLY		(1)		/*!< The locking process will only read the locked surface */

/*!
 *****************************************************************************
 *	Services State
 *****************************************************************************/
typedef enum _PVRSRV_SERVICES_STATE_
{
	PVRSRV_SERVICES_STATE_UNDEFINED = 0,
	PVRSRV_SERVICES_STATE_OK,
	PVRSRV_SERVICES_STATE_BAD,
} PVRSRV_SERVICES_STATE;


/*!
 *****************************************************************************
 *	States for power management
 *****************************************************************************/

/*!
  System Power State Enum
 */

#define _PVRSRV_SYS_POWER_STATES \
	X(Unspecified, -1) /*!< Unspecified : Uninitialised */ \
	X(OFF,          0) /*!< Off */ \
	X(ON,           1) /*!< On */

typedef enum _PVRSRV_SYS_POWER_STATE_
{
#define X(name, value) PVRSRV_SYS_POWER_STATE_##name = value,
	_PVRSRV_SYS_POWER_STATES
#undef X

	PVRSRV_SYS_POWER_STATE_FORCE_I32 = 0x7fffffff	/*!< Force enum to be at least 32-bits wide */

} PVRSRV_SYS_POWER_STATE, *PPVRSRV_SYS_POWER_STATE; /*!< Typedef for ptr to PVRSRV_SYS_POWER_STATE */

/*!
  Device Power State Enum
 */
typedef IMG_INT32 PVRSRV_DEV_POWER_STATE;
typedef IMG_INT32 *PPVRSRV_DEV_POWER_STATE;	/*!< Typedef for ptr to PVRSRV_DEV_POWER_STATE */ /* PRQA S 3205 */
#define PVRSRV_DEV_POWER_STATE_DEFAULT	-1	/*!< Default state for the device */
#define PVRSRV_DEV_POWER_STATE_OFF		 0	/*!< Unpowered */
#define PVRSRV_DEV_POWER_STATE_ON		 1	/*!< Running */

/*!
  Power Flags Enum
 */
typedef IMG_UINT32 PVRSRV_POWER_FLAGS;
#define PVRSRV_POWER_FLAGS_NONE             0U      /*!< No flags */
#define PVRSRV_POWER_FLAGS_FORCED           (1U << 0) /*!< Power the transition should not fail */
#define PVRSRV_POWER_FLAGS_OSPM_SUSPEND_REQ (1U << 1) /*!< Indicates an OS Power Management
                                                           transition (S3/S4) has been requested.
                                                           Allows system modules to save VRAM */
#define PVRSRV_POWER_FLAGS_OSPM_RESUME_REQ  (1U << 2) /*!< Indicates an OS Power Management
                                                           transition has been requested.
                                                           Allows system modules to load VRAM */

/*!
 *****************************************************************************
 * This structure is used for OS independent registry (profile) access
 *****************************************************************************/

typedef struct PVRSRV_REGISTRY_INFO_TAG
{
	IMG_UINT32	ui32DevCookie;
	IMG_PCHAR	pszKey;
	IMG_PCHAR	pszValue;
	IMG_PCHAR	pszBuf;
	IMG_UINT32	ui32BufSize;
} PVRSRV_REGISTRY_INFO, *PPVRSRV_REGISTRY_INFO;

#endif /* SERVICESEXT_H */
/******************************************************************************
 End of file (servicesext.h)
******************************************************************************/
