/*************************************************************************/ /*!
@File
@Title          Power Management Functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Main APIs for power management functions
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
#ifndef POWER_H
#define POWER_H

#include "img_defs.h"
#include "img_types.h"
#include "pvrsrv_device.h"
#include "pvrsrv_error.h"
#include "servicesext.h"
#include "opaque_types.h"
#include "di_common.h"

/*!
 *****************************************************************************
 *	Power management
 *****************************************************************************/

typedef struct _PVRSRV_POWER_DEV_TAG_ PVRSRV_POWER_DEV;

typedef IMG_BOOL (*PFN_SYS_DEV_IS_DEFAULT_STATE_OFF)(PVRSRV_POWER_DEV *psPowerDevice);

/* Power transition handler prototypes */

/*!
  Typedef for a pointer to a Function that will be called before a transition
  from one power state to another. See also PFN_POST_POWER.
 */
typedef PVRSRV_ERROR (*PFN_PRE_POWER)(PPVRSRV_DEVICE_NODE psDeviceNode,
                                      PVRSRV_DEV_POWER_STATE eNewPowerState,
                                      PVRSRV_DEV_POWER_STATE eCurrentPowerState,
                                      PVRSRV_POWER_FLAGS ePwrFlags);
/*!
  Typedef for a pointer to a Function that will be called after a transition
  from one power state to another. See also PFN_PRE_POWER.
 */
typedef PVRSRV_ERROR (*PFN_POST_POWER)(PPVRSRV_DEVICE_NODE psDeviceNode,
                                       PVRSRV_DEV_POWER_STATE eNewPowerState,
                                       PVRSRV_DEV_POWER_STATE eCurrentPowerState,
                                       PVRSRV_POWER_FLAGS ePwrFlags);

/* Clock speed handler prototypes */

/*!
  Typedef for a pointer to a Function that will be called before a transition
  from one clock speed to another. See also PFN_POST_CLOCKSPEED_CHANGE.
 */
typedef PVRSRV_ERROR (*PFN_PRE_CLOCKSPEED_CHANGE)(PPVRSRV_DEVICE_NODE psDeviceNode,
                                                  PVRSRV_DEV_POWER_STATE eCurrentPowerState);

/*!
  Typedef for a pointer to a Function that will be called after a transition
  from one clock speed to another. See also PFN_PRE_CLOCKSPEED_CHANGE.
 */
typedef PVRSRV_ERROR (*PFN_POST_CLOCKSPEED_CHANGE)(PPVRSRV_DEVICE_NODE psDeviceNode,
                                                   PVRSRV_DEV_POWER_STATE eCurrentPowerState);

/*!
  Typedef for a pointer to a function that will be called to transition the
  device to a forced idle state. Used in unison with (forced) power requests,
  DVFS and cluster count changes.
 */
typedef PVRSRV_ERROR (*PFN_FORCED_IDLE_REQUEST)(PPVRSRV_DEVICE_NODE psDeviceNode,
                                                IMG_BOOL bDeviceOffPermitted);

/*!
  Typedef for a pointer to a function that will be called to cancel a forced
  idle state and return the firmware back to a state where the hardware can be
  scheduled.
 */
typedef PVRSRV_ERROR (*PFN_FORCED_IDLE_CANCEL_REQUEST)(PPVRSRV_DEVICE_NODE psDeviceNode);

typedef PVRSRV_ERROR (*PFN_GPU_UNITS_POWER_CHANGE)(PPVRSRV_DEVICE_NODE psDeviceNode,
                                                   IMG_UINT32 ui32SESPowerState);

const char *PVRSRVSysPowerStateToString(PVRSRV_SYS_POWER_STATE eState);
const char *PVRSRVDevPowerStateToString(PVRSRV_DEV_POWER_STATE eState);

PVRSRV_ERROR PVRSRVPowerLockInit(PPVRSRV_DEVICE_NODE psDeviceNode);
void PVRSRVPowerLockDeInit(PPVRSRV_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

 @Function	PVRSRVPowerLock

 @Description	Obtain the mutex for power transitions. Only allowed when
                system power is on.

 @Return	PVRSRV_ERROR_SYSTEM_STATE_POWERED_OFF or PVRSRV_OK

******************************************************************************/
#if defined(DEBUG)
PVRSRV_ERROR PVRSRVPowerLock_Debug(PPVRSRV_DEVICE_NODE psDeviceNode,
                                   const char *pszFile, const unsigned int ui32LineNum);
#define PVRSRVPowerLock(DEV_NODE)	PVRSRVPowerLock_Debug(DEV_NODE, __FILE__, __LINE__)
#else
PVRSRV_ERROR PVRSRVPowerLock(PPVRSRV_DEVICE_NODE psDeviceNode);
#endif

/*!
******************************************************************************

 @Function	PVRSRVPowerUnlock

 @Description	Release the mutex for power transitions

 @Return	PVRSRV_ERROR

******************************************************************************/
void PVRSRVPowerUnlock(PPVRSRV_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

 @Function	PVRSRVPowerTryLock

 @Description	Try to obtain the mutex for power transitions. Only allowed when
		system power is on.

 @Return	PVRSRV_ERROR_RETRY or PVRSRV_ERROR_SYSTEM_STATE_POWERED_OFF or
		PVRSRV_OK

******************************************************************************/
#if defined(DEBUG)
PVRSRV_ERROR PVRSRVPowerTryLock_Debug(PPVRSRV_DEVICE_NODE psDeviceNode,
                                      const char *pszFile, const unsigned int ui32LineNum);
#define PVRSRVPowerTryLock(DEV_NODE)	PVRSRVPowerTryLock_Debug(DEV_NODE, __FILE__, __LINE__)
#else
PVRSRV_ERROR PVRSRVPowerTryLock(PPVRSRV_DEVICE_NODE psDeviceNode);
#endif

/*!
******************************************************************************

 @Function	PVRSRVPowerTryLockWaitForTimeout

 @Description	Try to obtain the mutex for power transitions. Only allowed when
		system power is on. The call blocks until either the lock is acquired,
		or the timeout is reached.

		*** Debug only. DO NOT use in core GPU functions which cannot fail. ***
		If the power lock cannot be taken the device may be powered down at
		any time in another worker thread.

 @Return	PVRSRV_ERROR_RETRY or PVRSRV_ERROR_SYSTEM_STATE_POWERED_OFF or
		PVRSRV_OK

******************************************************************************/
PVRSRV_ERROR PVRSRVPowerTryLockWaitForTimeout(PPVRSRV_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

 @Function     PVRSRVPwrLockIsLockedByMe

 @Description  Determine if the calling context is holding the device power-lock

 @Return       IMG_BOOL

******************************************************************************/
IMG_BOOL PVRSRVPwrLockIsLockedByMe(PCPVRSRV_DEVICE_NODE psDeviceNode);
IMG_BOOL PVRSRVDeviceIsDefaultStateOFF(PVRSRV_POWER_DEV *psPowerDevice);

/*!
******************************************************************************

 @Function	PVRSRVSetDevicePowerStateKM

 @Description	Set the Device into a new state

 @Input		psDeviceNode : Device node
 @Input		eNewPowerState : New power state
 @Input		ePwrFlags : Power state change flags

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR PVRSRVSetDevicePowerStateKM(PPVRSRV_DEVICE_NODE	psDeviceNode,
										 PVRSRV_DEV_POWER_STATE	eNewPowerState,
										 PVRSRV_POWER_FLAGS		ePwrFlags);

/*************************************************************************/ /*!
@Function     PVRSRVSetDeviceSystemPowerState
@Description  Set the device into a new power state based on the systems power
              state
@Input        psDeviceNode       Device node
@Input        eNewSysPowerState  New system power state
@Input        ePwrFlags          Power state change flags
@Return       PVRSRV_ERROR       PVRSRV_OK on success or an error otherwise
*/ /**************************************************************************/
PVRSRV_ERROR PVRSRVSetDeviceSystemPowerState(PPVRSRV_DEVICE_NODE psDeviceNode,
											 PVRSRV_SYS_POWER_STATE eNewSysPowerState,
											 PVRSRV_POWER_FLAGS ePwrFlags);

/*!
******************************************************************************

 @Function      PVRSRVSetDeviceDefaultPowerState

 @Description   Set the default device power state to eNewPowerState

 @Input         psDeviceNode : Device node
 @Input         eNewPowerState : New power state

 @Return        PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR PVRSRVSetDeviceDefaultPowerState(PCPVRSRV_DEVICE_NODE psDeviceNode,
					PVRSRV_DEV_POWER_STATE eNewPowerState);

/*!
******************************************************************************

 @Function      PVRSRVSetDeviceCurrentPowerState

 @Description   Set the current device power state to eNewPowerState

 @Input         psPowerDevice : Power device
 @Input         eNewPowerState : New power state

 @Return        PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR PVRSRVSetDeviceCurrentPowerState(PVRSRV_POWER_DEV *psPowerDevice,
					PVRSRV_DEV_POWER_STATE eNewPowerState);

/*!
******************************************************************************

 @Function      PVRSRVSetSystemPowerState

 @Description   Set the system power state to eNewPowerState

 @Input         psDeviceConfig : Device config
 @Input         eNewPowerState : New power state

 @Return        PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR PVRSRVSetSystemPowerState(PVRSRV_DEVICE_CONFIG * psDeviceConfig,
											 PVRSRV_SYS_POWER_STATE eNewSysPowerState);

/*!
******************************************************************************

 @Function      PVRSRVSetPowerCallbacks

 @Description   Initialise the Power Device's function pointers
                to the appropriate callbacks depending on driver mode and
                system setup.

 @Input         psDeviceNode : Device node
 @Input         psPowerDevice : Power device
 @Input         pfnDevicePrePower : regular device pre power callback
 @Input         pfnDevicePostPower : regular device post power callback
 @Input         pfnSystemPrePower : regular system pre power callback
 @Input         pfnDevicePostPower : regular system post power callback
 @Input         pfnSystemPrePower : regular device pre power callback
 @Input         pfnSystemPostPower : regular device pre power callback
 @Input         pfnForcedIdleRequest : forced idle request callback
 @Input         pfnForcedIdleCancelRequest : forced idle request cancel callback

******************************************************************************/
void PVRSRVSetPowerCallbacks(PPVRSRV_DEVICE_NODE				psDeviceNode,
							 PVRSRV_POWER_DEV					*psPowerDevice,
							 PFN_PRE_POWER						pfnDevicePrePower,
							 PFN_POST_POWER					    pfnDevicePostPower,
							 PFN_SYS_PRE_POWER				    pfnSystemPrePower,
							 PFN_SYS_POST_POWER			        pfnSystemPostPower,
							 PFN_FORCED_IDLE_REQUEST			pfnForcedIdleRequest,
							 PFN_FORCED_IDLE_CANCEL_REQUEST	pfnForcedIdleCancelRequest);

/* Type PFN_DC_REGISTER_POWER */
PVRSRV_ERROR PVRSRVRegisterPowerDevice(PPVRSRV_DEVICE_NODE				psDeviceNode,
									   PFN_PRE_POWER					pfnDevicePrePower,
									   PFN_POST_POWER					pfnDevicePostPower,
									   PFN_SYS_PRE_POWER			    pfnSystemPrePower,
									   PFN_SYS_POST_POWER			    pfnSystemPostPower,
									   PFN_PRE_CLOCKSPEED_CHANGE		pfnPreClockSpeedChange,
									   PFN_POST_CLOCKSPEED_CHANGE		pfnPostClockSpeedChange,
									   PFN_FORCED_IDLE_REQUEST			pfnForcedIdleRequest,
									   PFN_FORCED_IDLE_CANCEL_REQUEST	pfnForcedIdleCancelRequest,
									   PFN_GPU_UNITS_POWER_CHANGE		pfnGPUUnitsPowerChange,
									   IMG_HANDLE						hDevCookie,
									   PVRSRV_DEV_POWER_STATE			eCurrentPowerState,
									   PVRSRV_DEV_POWER_STATE			eDefaultPowerState);

/*!
******************************************************************************

 @Function	PVRSRVRemovePowerDevice

 @Description

 Removes device from power management register. Device is located by Device Index

 @Input		psDeviceNode : Device node

******************************************************************************/
void PVRSRVRemovePowerDevice(PPVRSRV_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

 @Function	PVRSRVGetDevicePowerState

 @Description

	Return the device power state

 @Input		psDeviceNode : Device node
 @Output	pePowerState : Current power state

 @Return	PVRSRV_ERROR_UNKNOWN_POWER_STATE if device could not be found.
            PVRSRV_OK otherwise.

******************************************************************************/
PVRSRV_ERROR PVRSRVGetDevicePowerState(PCPVRSRV_DEVICE_NODE psDeviceNode,
									   PPVRSRV_DEV_POWER_STATE pePowerState);

/*!
******************************************************************************

 @Function	PVRSRVIsDevicePowered

 @Description

	Whether the device is powered, for the purposes of lockup detection.

 @Input		psDeviceNode : Device node

 @Return	IMG_BOOL

******************************************************************************/
IMG_BOOL PVRSRVIsDevicePowered(PPVRSRV_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

 @Function	PVRSRVGetSystemPowerState

 @Description

	Return the system power state

 @Input		psDeviceNode : Device node
 @Output	peCurrentSysPowerState : Current power state

 @Return	PVRSRV_ERROR_UNKNOWN_POWER_STATE if device could not be found.
            PVRSRV_OK otherwise.

******************************************************************************/
PVRSRV_ERROR PVRSRVGetSystemPowerState(PPVRSRV_DEVICE_NODE psDeviceNode,
	                                   PPVRSRV_SYS_POWER_STATE peCurrentSysPowerState);

/*!
******************************************************************************

 @Function	PVRSRVIsSystemPowered

 @Description

	Whether the system layer is powered, for ensuring the RGX regbank is powered
	during initial GPU driver configuration.

 @Input		psDeviceNode : Device node

 @Return	IMG_BOOL

******************************************************************************/
IMG_BOOL PVRSRVIsSystemPowered(PPVRSRV_DEVICE_NODE psDeviceNode);

/**************************************************************************/ /*!
@Function       PVRSRVDevicePreClockSpeedChange

@Description    This function is called before a voltage/frequency change is
                made to the GPU HW. It informs the host driver of the intention
                to make a DVFS change. If allows the host driver to idle
                the GPU and begin a hold off period from starting new work
                on the GPU.
                When this call succeeds the caller *must* call
                PVRSRVDevicePostClockSpeedChange() to end the hold off period
                to allow new work to be submitted to the GPU.

                Called from system layer or OS layer implementation that
                is responsible for triggering a GPU DVFS transition.

@Input          psDeviceNode pointer to the device affected by DVFS transition.
@Input          bIdleDevice  when True, the driver will wait for the GPU to
                             reach an idle state before the call returns.
@Input          pvInfo       unused

@Return         PVRSRV_OK    on success, power lock acquired and held on exit,
                             GPU idle.
                PVRSRV_ERROR on failure, power lock not held on exit, do not
                             call PVRSRVDevicePostClockSpeedChange().
*/ /**************************************************************************/
PVRSRV_ERROR PVRSRVDevicePreClockSpeedChange(PPVRSRV_DEVICE_NODE psDeviceNode,
											 IMG_BOOL	bIdleDevice,
											 void	*pvInfo);

/**************************************************************************/ /*!
@Function       PVRSRVDevicePostClockSpeedChange

@Description    This function is called after a voltage/frequency change has
                been made to the GPU HW following a call to
                PVRSRVDevicePreClockSpeedChange().
                Before calling this function the caller must ensure the system
                data RGX_DATA->RGX_TIMING_INFORMATION->ui32CoreClockSpeed has
                been updated with the new frequency set, measured in Hz.
                The function informs the host driver that the DVFS change has
                completed. The driver will end the work hold off period, cancel
                the device idle period and update its time data records.
                When this call returns work submissions are unblocked and
                are submitted to the GPU as normal.
                This function *must* not be called if the preceding call to
                PVRSRVDevicePreClockSpeedChange() failed.

                Called from system layer or OS layer implementation that
                is responsible for triggering a GPU DVFS transition.

@Input          psDeviceNode pointer to the device affected by DVFS transition.
@Input          bIdleDevice  when True, the driver will cancel the GPU
                             device idle state before the call returns. Value
                             given must match that used in the call to
                             PVRSRVDevicePreClockSpeedChange() otherwise
                             undefined behaviour will result.
@Input          pvInfo       unused

@Return         void         power lock released, no longer held on exit.
*/ /**************************************************************************/
void PVRSRVDevicePostClockSpeedChange(PPVRSRV_DEVICE_NODE psDeviceNode,
									  IMG_BOOL		bIdleDevice,
									  void		*pvInfo);

/*!
******************************************************************************

 @Function    PVRSRVDeviceIdleRequestKM

 @Description Perform device-specific processing required to force the device
              idle. The device power-lock might be temporarily released (and
              again re-acquired) during the course of this call, hence to
              maintain lock-ordering power-lock should be the last acquired
              lock before calling this function

 @Input       psDeviceNode         : Device node

 @Input       pfnIsDefaultStateOff : When specified, the idle request is only
                                     processed if this function passes.

 @Input       bDeviceOffPermitted  : IMG_TRUE if the transition should not fail
                                       if device off
                                     IMG_FALSE if the transition should fail if
                                       device off

 @Return      PVRSRV_ERROR_PWLOCK_RELEASED_REACQ_FAILED
                                     When re-acquisition of power-lock failed.
                                     This error NEEDS EXPLICIT HANDLING at call
                                     site as it signifies the caller needs to
                                     AVOID calling PVRSRVPowerUnlock, since
                                     power-lock is no longer "possessed" by
                                     this context.

              PVRSRV_OK              When idle request succeeded.
              PVRSRV_ERROR           Other system errors.

******************************************************************************/
PVRSRV_ERROR PVRSRVDeviceIdleRequestKM(PPVRSRV_DEVICE_NODE psDeviceNode,
					PFN_SYS_DEV_IS_DEFAULT_STATE_OFF	pfnIsDefaultStateOff,
					IMG_BOOL				bDeviceOffPermitted);

/*!
******************************************************************************

 @Function	PVRSRVDeviceIdleCancelRequestKM

 @Description Perform device-specific processing required to cancel the forced idle state
              on the device, returning to normal operation.

 @Input		psDeviceNode : Device node

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR PVRSRVDeviceIdleCancelRequestKM(PPVRSRV_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

@Function       PVRSRVDeviceGPUUnitsPowerChange
@Description    Request from system layer for changing power state of GPU
                units
@Input          psDeviceNode            RGX Device Node.
@Input          ui32NewValue            Value indicating the new power state
                                        of GPU units. how this is interpreted
                                        depends upon the device-specific
                                        function subsequently called by the
                                        server via a pfn.
@Return         PVRSRV_ERROR.
*/ /**************************************************************************/
PVRSRV_ERROR PVRSRVDeviceGPUUnitsPowerChange(PPVRSRV_DEVICE_NODE psDeviceNode,
					IMG_UINT32	ui32NewValue);

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
void PVRSRVSetFirmwareStartTime(PVRSRV_POWER_DEV *psPowerDevice, IMG_UINT32 ui32TimeStamp);

void PVRSRVSetFirmwareHandshakeIdleTime(PVRSRV_POWER_DEV *psPowerDevice, IMG_UINT64 ui64Duration);

int PVRSRVPowerStatsPrintElements(OSDI_IMPL_ENTRY *psEntry, void *pvData);
#endif

#endif /* POWER_H */

/******************************************************************************
 End of file (power.h)
******************************************************************************/
