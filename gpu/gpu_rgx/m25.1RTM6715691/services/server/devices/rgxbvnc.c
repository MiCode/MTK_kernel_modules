/*************************************************************************/ /*!
@File
@Title          BVNC handling specific routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Functions used for BVNC related work
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

#include "img_defs.h"
#include "rgxbvnc.h"
#define RGXBVNC_C
#include "rgx_bvnc_table_km.h"
#undef RGXBVNC_C
#include "os_apphint.h"
#include "pvrsrv.h"
#include "pdump_km.h"
#include "rgx_compat_bvnc.h"
#include "allocmem.h"

#define RGX_FEATURE_TRUE_VALUE_TYPE_UINT16 (RGX_FEATURE_VALUE_TYPE_UINT16 >> RGX_FEATURE_TYPE_BIT_SHIFT)
#define RGX_FEATURE_TRUE_VALUE_TYPE_UINT32 (RGX_FEATURE_VALUE_TYPE_UINT32 >> RGX_FEATURE_TYPE_BIT_SHIFT)
#define RGXBVNC_BUFFER_SIZE (((PVRSRV_MAX_DEVICES)*(RGX_BVNC_STR_SIZE_MAX))+1)

/* This function searches the feature array for a given BVNC */
static const RGX_BVNC_KM_FEATURE_TABLE_ENTRY * RGXGetFeatureEntryFromBVNC(IMG_UINT64 ui64SearchValue)
{
	IMG_UINT32 i;

	for (i = 0; i < ARRAY_SIZE(gaFeatures); i++)
	{
		if (gaFeatures[i].ui64BVNC == ui64SearchValue)
		{
			return &gaFeatures[i];
		}
	}

	return NULL;
}

/* This function searches the ERN/BRN array for a given BVNC */
static const IMG_UINT64 * RGXGetERNBRNFromBVNC(IMG_UINT64 ui64SearchValue)
{
	IMG_UINT32 i;

	for (i = 0; i < ARRAY_SIZE(gaErnsBrns); i++)
	{
		if (gaErnsBrns[i][0] == ui64SearchValue)
		{
			return gaErnsBrns[i];
		}
	}

	return NULL;
}

#if !defined(NO_HARDWARE)
/*************************************************************************/ /*!
@brief		This function reads the (P)BVNC core_ID register and extracts
			the BVNC configuration. Supports the old scheme and the newer
			PBVNC scheme.
@param		psDeviceNode - Device Node pointer
@param		ui32CoreNum  - Core/bank number (0 for single core)
@param		pB           - Address of branch value (output)
@param		pV           - Address of version value (output)
@param		pN           - Address of number of clusters/scalable shading units value (output)
@param		pC           - Address of configuration value (output)
@return		BVNC encoded in 64-bit value, 16-bits per field
*/ /**************************************************************************/
static
IMG_UINT64 _RGXReadBVNCFromReg(PVRSRV_DEVICE_NODE *psDeviceNode, IMG_UINT32 ui32CoreNum,
							   IMG_UINT32 *pB, IMG_UINT32 *pV, IMG_UINT32 *pN, IMG_UINT32 *pC)
{
	IMG_UINT64 ui64BVNC;
	IMG_UINT32 B=0, V=0, N=0, C=0;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

#if defined(RGX_CR_CORE_ID__PBVNC)
	/* Core ID reading code for Rogue */

	/* Read the BVNC, in to new way first, if B not set, use old scheme */
	ui64BVNC = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_CORE_ID__PBVNC + (ui32CoreNum << 16));

	if (GET_B(ui64BVNC))
	{
		B = GET_PBVNC_B(ui64BVNC);
		V = GET_PBVNC_V(ui64BVNC);
		N = GET_PBVNC_N(ui64BVNC);
		C = GET_PBVNC_C(ui64BVNC);
	}
	else
	{
		IMG_UINT64 ui32CoreID, ui32CoreRev;
		ui32CoreRev = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_CORE_REVISION + (ui32CoreNum << 16));
		ui32CoreID = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_CORE_ID + (ui32CoreNum << 16));
		B = (ui32CoreRev & ~RGX_CR_CORE_REVISION_MAJOR_CLRMSK) >>
												RGX_CR_CORE_REVISION_MAJOR_SHIFT;
		V = (ui32CoreRev & ~RGX_CR_CORE_REVISION_MINOR_CLRMSK) >>
												RGX_CR_CORE_REVISION_MINOR_SHIFT;
		N = (ui32CoreID & ~RGX_CR_CORE_ID_CONFIG_N_CLRMSK) >>
												RGX_CR_CORE_ID_CONFIG_N_SHIFT;
		C = (ui32CoreID & ~RGX_CR_CORE_ID_CONFIG_C_CLRMSK) >>
												RGX_CR_CORE_ID_CONFIG_C_SHIFT;
		ui64BVNC = rgx_bvnc_pack(B, V, N, C);
	}
#else
	/* Core ID reading code for Volcanic */

	ui64BVNC = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_CORE_ID + (ui32CoreNum << 16));

	B = (ui64BVNC & ~RGX_CR_CORE_ID_BRANCH_ID_CLRMSK) >>
											RGX_CR_CORE_ID_BRANCH_ID_SHIFT;
	V = (ui64BVNC & ~RGX_CR_CORE_ID_VERSION_ID_CLRMSK) >>
											RGX_CR_CORE_ID_VERSION_ID_SHIFT;
	N = (ui64BVNC & ~RGX_CR_CORE_ID_NUMBER_OF_SCALABLE_UNITS_CLRMSK) >>
											RGX_CR_CORE_ID_NUMBER_OF_SCALABLE_UNITS_SHIFT;
	C = (ui64BVNC & ~RGX_CR_CORE_ID_CONFIG_ID_CLRMSK) >>
											RGX_CR_CORE_ID_CONFIG_ID_SHIFT;
#endif

	*pB = B; *pV = V; *pN = N; *pC = C;
	return ui64BVNC;
}
#endif

#if defined(DEBUG) || defined(SUPPORT_PERFORMANCE_RUN)

#define PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, szShortName, Feature)															\
	if ( psDevInfo->sDevFeatureCfg.ui32FeaturesValues[RGX_FEATURE_##Feature##_IDX] != RGX_FEATURE_VALUE_DISABLED )			\
		{ PVR_LOG(("%s %d", szShortName, psDevInfo->sDevFeatureCfg.ui32FeaturesValues[RGX_FEATURE_##Feature##_IDX])); }		\
	else																\
		{ PVR_LOG(("%s N/A", szShortName)); }

static void _RGXBvncDumpParsedConfig(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)psDeviceNode->pvDevice;
#if defined(FEATURE_NO_VALUES_NAMES_MAX_IDX)
	IMG_UINT16 ui16FeatureMaskIdx;
#endif
	IMG_UINT64 ui64Mask = 0;
#if defined(ERNSBRNS_IDS_MAX_IDX)
	IMG_UINT64 ui32IdOrNameIdx = 0;
#endif

	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "NC:       ", NUM_CLUSTERS);
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "CSF:      ", CDM_CONTROL_STREAM_FORMAT);
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "FBCDCA:   ", FBCDC_ARCHITECTURE);
#if defined(RGX_FEATURE_META_IDX)
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "META:     ", META);
#endif
#if defined(RGX_FEATURE_META_COREMEM_BANKS_IDX)
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "MCMB:     ", META_COREMEM_BANKS);
#endif
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "MCMS:     ", META_COREMEM_SIZE);
#if defined(RGX_FEATURE_META_DMA_CHANNEL_COUNT_IDX)
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "MDMACnt:  ", META_DMA_CHANNEL_COUNT);
#endif
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "NIIP:     ", NUM_ISP_IPP_PIPES);
#if defined(RGX_FEATURE_NUM_ISP_PER_SPU_MAX_VALUE_IDX)
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "NIPS:     ", NUM_ISP_PER_SPU);
#endif
#if defined(RGX_FEATURE_PBE_PER_SPU_MAX_VALUE_IDX)
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "PPS:      ", PBE_PER_SPU);
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "NSPU:     ", NUM_SPU);
#endif
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "PBW:      ", PHYS_BUS_WIDTH);
#if defined(RGX_FEATURE_SCALABLE_TE_ARCH_IDX)
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "STEArch:  ", SCALABLE_TE_ARCH);
#endif
#if defined(RGX_FEATURE_SCALABLE_VCE_IDX)
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "SVCEA:    ", SCALABLE_VCE);
#endif
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "SLCBanks: ", SLC_BANKS);
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "SLCCLS:   ", SLC_CACHE_LINE_SIZE_BITS);
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "VASB:     ", VIRTUAL_ADDRESS_SPACE_BITS);
	PVR_LOG_DUMP_FEATURE_VALUE(psDevInfo, "NOSIDS:   ", NUM_OSIDS);

#if defined(FEATURE_NO_VALUES_NAMES_MAX_IDX)
	/* Dump the features with no values */
	for (ui16FeatureMaskIdx = 0; ui16FeatureMaskIdx < RGX_BVNC_KM_FEATURE_FLAG_ARRAY_SIZE; ui16FeatureMaskIdx++)
	{
		IMG_UINT32 ui32NameBase = ui16FeatureMaskIdx * 64;
		IMG_UINT32 ui32Shift;

		ui64Mask = psDevInfo->sDevFeatureCfg.paui64Features[ui16FeatureMaskIdx];

		for (ui32Shift = 0; ui32Shift < 63; ui32Shift++)
		{
			if ((ui64Mask >> ui32Shift) == 0)
			{
				break;
			}

			if ((ui64Mask >> ui32Shift) & 0x01)
			{
				IMG_UINT32 ui32Index = ui32NameBase + ui32Shift;
				if (ui32Index < FEATURE_NO_VALUES_NAMES_MAX_IDX)
				{
					PVR_LOG(("%s", gaszFeaturesNoValuesNames[ui32Index]));
				}
				else
				{
					PVR_DPF((PVR_DBG_WARNING,
							"At index %u feature with mask doesn't exist: 0x%016" IMG_UINT64_FMTSPECx,
							ui16FeatureMaskIdx, ((IMG_UINT64)1 << (ui32Shift))));
				}
			}
		}
	}
#endif

#if defined(ERNSBRNS_IDS_MAX_IDX)
	/* Dump the ERN and BRN flags for this core */
	ui64Mask = psDevInfo->sDevFeatureCfg.ui64ErnsBrns;
	ui32IdOrNameIdx = 1;

	while (ui64Mask)
	{
		if (ui64Mask & 0x1)
		{
			if (ui32IdOrNameIdx <= ERNSBRNS_IDS_MAX_IDX)
			{
				PVR_LOG(("ERN/BRN : %d", gaui64ErnsBrnsIDs[ui32IdOrNameIdx - 1]));
			}
			else
			{
				PVR_LOG(("Unknown ErnBrn bit: 0x%0" IMG_UINT64_FMTSPECx, ((IMG_UINT64)1 << (ui32IdOrNameIdx - 1))));
			}
		}
		ui64Mask >>= 1;
		ui32IdOrNameIdx++;
	}
#endif

#if !defined(ERNSBRNS_IDS_MAX_IDX) && !defined(FEATURE_NO_VALUES_NAMES_MAX_IDX)
	PVR_UNREFERENCED_PARAMETER(ui64Mask);
#endif

}
#endif

static PVRSRV_ERROR _RGXBvncParseFeatureValues(PVRSRV_RGXDEV_INFO *psDevInfo,
                                               const RGX_BVNC_KM_FEATURE_TABLE_ENTRY *psFeatureConfig)
{
	IMG_UINT32 ui32Index;

	/* Read the feature values for the runtime BVNC */
	for (ui32Index = 0; ui32Index < RGX_FEATURE_WITH_VALUES_MAX_IDX; ui32Index++)
	{
		IMG_UINT16 ui16BitPosition = aui16FeaturesWithValuesBitPositions[ui32Index];

		IMG_UINT32 ui32ArrIdx = ui16BitPosition >> 6; // Log2 division (ui16BitPosition / 64)
		IMG_UINT16 ui16ValueIndex = (psFeatureConfig->aui64FeatureValues[ui32ArrIdx] & aui64FeaturesWithValuesBitMasks[ui32Index]) >> (ui16BitPosition % 64);

		if (ui16ValueIndex >= gaFeaturesValuesMaxIndexes[ui32Index])
		{
			/* This case should never be reached */
			psDevInfo->sDevFeatureCfg.ui32FeaturesValues[ui32Index] = RGX_FEATURE_VALUE_INVALID;
			PVR_DPF((PVR_DBG_ERROR, "%s: Feature with index (%d) decoded wrong value index (%d)", __func__, ui32Index, ui16ValueIndex));
			PVR_ASSERT(ui16ValueIndex < gaFeaturesValuesMaxIndexes[ui32Index]);
			return PVRSRV_ERROR_INVALID_BVNC_PARAMS;
		}

		switch (ui16BitPosition >> RGX_FEATURE_TYPE_BIT_SHIFT)
		{
			case RGX_FEATURE_TRUE_VALUE_TYPE_UINT16:
			{
				IMG_UINT16 *pui16FeatureValues = (IMG_UINT16*)gaFeaturesValues[ui32Index];
				if (pui16FeatureValues[ui16ValueIndex] == (IMG_UINT16)RGX_FEATURE_VALUE_DISABLED)
				{
					psDevInfo->sDevFeatureCfg.ui32FeaturesValues[ui32Index] =
						RGX_FEATURE_VALUE_DISABLED;
				}
				else
				{
					psDevInfo->sDevFeatureCfg.ui32FeaturesValues[ui32Index] =
						pui16FeatureValues[ui16ValueIndex];
				}
				break;
			}
			case RGX_FEATURE_TRUE_VALUE_TYPE_UINT32:
				psDevInfo->sDevFeatureCfg.ui32FeaturesValues[ui32Index] =
					((IMG_UINT32*)gaFeaturesValues[ui32Index])[ui16ValueIndex];
				break;
			default:
				PVR_DPF((PVR_DBG_ERROR,
				         "%s: Feature with index %d has invalid feature type",
				         __func__,
				         ui32Index));
				return PVRSRV_ERROR_INVALID_BVNC_PARAMS;
		}
	}


#if defined(RGX_FEATURE_POWER_ISLAND_VERSION_MAX_VALUE_IDX)
	/* Code path for Volcanic */

	psDevInfo->sDevFeatureCfg.ui32MAXDMCount = RGXFWIF_DM_CDM+1;
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, RAY_TRACING_ARCH) &&
		RGX_GET_FEATURE_VALUE(psDevInfo, RAY_TRACING_ARCH) > 1)
	{
		psDevInfo->sDevFeatureCfg.ui32MAXDMCount = MAX(psDevInfo->sDevFeatureCfg.ui32MAXDMCount, RGXFWIF_DM_RAY+1);
	}
#if defined(SUPPORT_AGP)
	psDevInfo->sDevFeatureCfg.ui32MAXDMCount = MAX(psDevInfo->sDevFeatureCfg.ui32MAXDMCount, RGXFWIF_DM_GEOM2+1);
#if defined(SUPPORT_AGP4)
	psDevInfo->sDevFeatureCfg.ui32MAXDMCount = MAX(psDevInfo->sDevFeatureCfg.ui32MAXDMCount, RGXFWIF_DM_GEOM4+1);
#endif
#endif

	/* Get the max number of dusts in the core */
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, NUM_CLUSTERS))
	{
		RGX_LAYER_PARAMS sParams = {.psDevInfo = psDevInfo};

		if ((RGX_DEVICE_GET_FEATURE_VALUE(&sParams, POWER_ISLAND_VERSION) == 1) ||
			(RGX_DEVICE_GET_FEATURE_VALUE(&sParams, POWER_ISLAND_VERSION) >= 7))
		{
			/* per SPU power island */
			psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount = MAX(1, (RGX_GET_FEATURE_VALUE(psDevInfo, NUM_CLUSTERS) / 2));
		}
		else if (RGX_DEVICE_GET_FEATURE_VALUE(&sParams, POWER_ISLAND_VERSION) >= 2)
		{
			/* per Cluster power island */
			psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount = RGX_GET_FEATURE_VALUE(psDevInfo, NUM_CLUSTERS);
		}
		else
		{
			/* All volcanic cores support power islanding */
			psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount = RGX_FEATURE_VALUE_INVALID;
			PVR_DPF((PVR_DBG_ERROR, "%s: Power island feature version not found!", __func__));
			PVR_ASSERT(0);
			return PVRSRV_ERROR_FEATURE_DISABLED;
		}

		if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, RAY_TRACING_ARCH) &&
			RGX_GET_FEATURE_VALUE(psDevInfo, RAY_TRACING_ARCH) > 1)
		{
			if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, RT_RAC_PER_SPU))
			{
				psDevInfo->sDevFeatureCfg.ui32MAXRACCount = RGX_GET_FEATURE_VALUE(psDevInfo, NUM_SPU);
			}
			else
			{
				psDevInfo->sDevFeatureCfg.ui32MAXRACCount = 1;
			}
		}
	}
	else
	{
		/* This case should never be reached as all cores have clusters */
		psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount = RGX_FEATURE_VALUE_INVALID;
		PVR_DPF((PVR_DBG_ERROR, "%s: Number of clusters feature value missing!", __func__));
		PVR_ASSERT(0);
		return PVRSRV_ERROR_FEATURE_DISABLED;
	}
#else /* defined(RGX_FEATURE_POWER_ISLAND_VERSION_MAX_VALUE_IDX) */
	/* Code path for Rogue */

	psDevInfo->sDevFeatureCfg.ui32MAXDMCount = RGXFWIF_DM_CDM+1;
#if defined(SUPPORT_AGP)
	psDevInfo->sDevFeatureCfg.ui32MAXDMCount = MAX(psDevInfo->sDevFeatureCfg.ui32MAXDMCount, RGXFWIF_DM_GEOM2+1);
#endif

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		psDevInfo->sDevFeatureCfg.ui32FeaturesValues[RGX_FEATURE_META_IDX] = RGX_FEATURE_VALUE_DISABLED;
	}

	/* Get the max number of dusts in the core */
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, NUM_CLUSTERS))
	{
		psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount = MAX(1, (RGX_GET_FEATURE_VALUE(psDevInfo, NUM_CLUSTERS) / 2));
	}
	else
	{
		/* This case should never be reached as all cores have clusters */
		psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount = RGX_FEATURE_VALUE_INVALID;
		PVR_DPF((PVR_DBG_ERROR, "%s: Number of clusters feature value missing!", __func__));
		PVR_ASSERT(0);
		return PVRSRV_ERROR_FEATURE_DISABLED;
	}
#endif /* defined(RGX_FEATURE_POWER_ISLAND_VERSION_MAX_VALUE_IDX) */

	/* Transform the META coremem size info in bytes */
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META_COREMEM_SIZE))
	{
		psDevInfo->sDevFeatureCfg.ui32FeaturesValues[RGX_FEATURE_META_COREMEM_SIZE_IDX] *= 1024;
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR _RGXBvncAcquireAppHint(IMG_CHAR *pszBVNC, const IMG_UINT32 ui32RGXDevCount)
{
	const IMG_CHAR *pszAppHintDefault = PVRSRV_APPHINT_RGXBVNC;
	void *pvAppHintState = NULL;
	IMG_UINT32 ui32BVNCCount = 0;
	IMG_BOOL bRet;
	IMG_CHAR *pszBVNCAppHint;
	IMG_CHAR *pszCurrentBVNC;
	pszBVNCAppHint = (IMG_CHAR *)OSAllocMem(RGXBVNC_BUFFER_SIZE);
	if (pszBVNCAppHint == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	pszBVNCAppHint[0] = '\0';

	pszCurrentBVNC = pszBVNCAppHint;

	OSCreateAppHintState(&pvAppHintState);

	bRet = (IMG_BOOL)OSGetAppHintSTRING(APPHINT_NO_DEVICE,
						pvAppHintState,
						RGXBVNC,
						pszAppHintDefault,
						pszBVNCAppHint,
						RGXBVNC_BUFFER_SIZE
	);

	OSFreeAppHintState(pvAppHintState);

	if (!bRet || (pszBVNCAppHint[0] == '\0'))
	{
		OSFreeMem(pszBVNCAppHint);
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "%s: BVNC module param list: %s",__func__, pszBVNCAppHint));

	while (*pszCurrentBVNC != '\0')
	{
		IMG_CHAR *pszNext = pszCurrentBVNC;

		if (ui32BVNCCount >= PVRSRV_MAX_DEVICES)
		{
			break;
		}

		while (1)
		{
			if (*pszNext == ',')
			{
				pszNext[0] = '\0';
				pszNext++;
				break;
			} else if (*pszNext == '\0')
			{
				break;
			}
			pszNext++;
		}

		if (ui32BVNCCount == ui32RGXDevCount)
		{
			OSStringSafeCopy(pszBVNC, pszCurrentBVNC, RGX_BVNC_STR_SIZE_MAX);
			OSFreeMem(pszBVNCAppHint);
			return PVRSRV_OK;
		}

		ui32BVNCCount++;
		pszCurrentBVNC = pszNext;
	}

	PVR_DPF((PVR_DBG_ERROR, "%s: Given module parameters list is shorter than "
	"number of actual devices", __func__));

	/* If only one BVNC parameter is specified, the same is applied for all RGX
	 * devices detected */
	if (1 == ui32BVNCCount)
	{
		OSStringSafeCopy(pszBVNC, pszBVNCAppHint, RGX_BVNC_STR_SIZE_MAX);
	}

	OSFreeMem(pszBVNCAppHint);

	return PVRSRV_OK;
}

/* Function that parses the BVNC List passed as module parameter */
static PVRSRV_ERROR _RGXBvncParseList(IMG_UINT32 *pB,
									  IMG_UINT32 *pV,
									  IMG_UINT32 *pN,
									  IMG_UINT32 *pC,
									  const IMG_UINT32 ui32RGXDevCount)
{
	IMG_CHAR aszBVNCString[RGX_BVNC_STR_SIZE_MAX];
	IMG_CHAR *pcTemp, *pcNext;
	PVRSRV_ERROR eError;

	aszBVNCString[0] = '\0';

	/* 4 components of a BVNC string is B, V, N & C */
#define RGX_BVNC_INFO_PARAMS (4)

	eError = _RGXBvncAcquireAppHint(aszBVNCString, ui32RGXDevCount);

	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	if ('\0' == aszBVNCString[0])
	{
		return PVRSRV_ERROR_INVALID_BVNC_PARAMS;
	}

	/* Parse the given RGX_BVNC string */
	pcTemp = &aszBVNCString[0];
	pcNext = strchr(pcTemp, '.');
	if (pcNext == NULL)
	{
		return PVRSRV_ERROR_INVALID_BVNC_PARAMS;
	}

	*pcNext = '\0';
	if (OSStringToUINT32(pcTemp, 0, pB) != PVRSRV_OK)
	{
		return PVRSRV_ERROR_INVALID_BVNC_PARAMS;
	}
	pcTemp = pcNext+1;
	/* remove any 'p' from the V string, as this will
	 * cause the call to OSStringToUINT32 to fail
	 */
	pcNext = strchr(pcTemp, 'p');
	if (pcNext)
	{
		/* found one- - changing to '\0' */
		*pcNext = '\0';
		/* Move to next '.' */
		pcNext++;
	}
	else
	{
		/* none found, so find next '.' and change to '\0' */
		pcNext = strchr(pcTemp, '.');
		if (pcNext == NULL)
		{
			return PVRSRV_ERROR_INVALID_BVNC_PARAMS;
		}
		*pcNext = '\0';
	}
	if (OSStringToUINT32(pcTemp, 0, pV) != PVRSRV_OK)
	{
		return PVRSRV_ERROR_INVALID_BVNC_PARAMS;
	}
	pcTemp = pcNext+1;
	pcNext = strchr(pcTemp, '.');
	if (pcNext == NULL)
	{
		return PVRSRV_ERROR_INVALID_BVNC_PARAMS;
	}
	*pcNext = '\0';
	if (OSStringToUINT32(pcTemp, 0, pN) != PVRSRV_OK)
	{
		return PVRSRV_ERROR_INVALID_BVNC_PARAMS;
	}
	pcTemp = pcNext+1;
	if (OSStringToUINT32(pcTemp, 0, pC) != PVRSRV_OK)
	{
		return PVRSRV_ERROR_INVALID_BVNC_PARAMS;
	}
	PVR_LOG(("BVNC module parameter honoured: %d.%d.%d.%d", *pB, *pV, *pN, *pC));

	return PVRSRV_OK;
}

static PVRSRV_ERROR RGXGetBVNCFromModParam(IMG_UINT32 ui32DeviceCount,
                                           IMG_UINT32 *pui32B,
                                           IMG_UINT32 *pui32V,
                                           IMG_UINT32 *pui32N,
                                           IMG_UINT32 *pui32C,
                                           const RGX_BVNC_KM_FEATURE_TABLE_ENTRY **ppsFeatureConfig)
{
	IMG_UINT32 B=0, V=0, N=0, C=0;
	PVRSRV_ERROR eError;

	/* Check for load time RGX BVNC parameter */
	eError = _RGXBvncParseList(&B,&V,&N,&C, ui32DeviceCount);
	if (eError == PVRSRV_OK)
	{
		*pui32B = B;
		*pui32V = V;
		*pui32N = N;
		*pui32C = C;

		PVR_LOG(("Read BVNC " RGX_BVNC_STR_FMTSPEC
		         " from driver load parameter", B, V, N, C));

		/* Extract the BVNC config from the Features table.
		 * This is done to verify the BVNC is valid. */
		*ppsFeatureConfig = RGXGetFeatureEntryFromBVNC(BVNC_PACK(B,0,N,C));
		if (*ppsFeatureConfig != NULL)
		{
			return PVRSRV_OK;
		}

		PVR_LOG(("Driver parameter BVNC configuration not found!"));
	}

	return PVRSRV_ERROR_BVNC_UNSUPPORTED;
}

#if !defined(NO_HARDWARE)
static PVRSRV_ERROR RGXGetBVNCFromHW(PVRSRV_DEVICE_NODE* psDeviceNode,
                                     IMG_UINT32 *pui32B,
                                     IMG_UINT32 *pui32V,
                                     IMG_UINT32 *pui32N,
                                     IMG_UINT32 *pui32C,
                                     IMG_UINT32 *pui32CoreCount)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	IMG_UINT32 B=0, V=0, N=0, C=0;
	void *pvAppHintState = NULL;
	const IMG_BOOL bAppHintDefault = PVRSRV_APPHINT_IGNOREHWREPORTEDBVNC;

	OSCreateAppHintState(&pvAppHintState);
	OSGetAppHintBOOL(APPHINT_NO_DEVICE,
	                 pvAppHintState,
	                 IgnoreHWReportedBVNC,
	                 &bAppHintDefault,
	                 &psDevInfo->bIgnoreHWReportedBVNC);
	OSFreeAppHintState(pvAppHintState);

	/* Try to detect the RGX BVNC from the HW device */
	if (!psDevInfo->bIgnoreHWReportedBVNC)
	{
		IMG_BOOL bPowerDown = !PVRSRVIsSystemPowered(psDeviceNode);
		PVRSRV_ERROR eError;

		/* Power-up the device as required to read the registers */
		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) && bPowerDown)
		{
			eError = PVRSRVSetSystemPowerState(psDeviceNode->psDevConfig, PVRSRV_SYS_POWER_STATE_ON);
			PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVSetSystemPowerState ON");
		}

		/* Read the BVNC from HW */
		_RGXReadBVNCFromReg(psDeviceNode, 0 /*core0*/, &B, &V, &N, &C);

		PVR_LOG(("Read BVNC " RGX_BVNC_STR_FMTSPEC
		         " from HW device registers", B, V, N, C));

		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
		{
			/* Read the number of cores in the system for newer BVNC (Branch ID > 20) */
			if (B > 20)
			{
#if defined(RGX_FEATURE_ERYX_TOP_INFRASTRUCTURE)
				*pui32CoreCount = OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MULTICORE_DOMAIN);
#else
				*pui32CoreCount = OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MULTICORE_SYSTEM);
#endif
			}
		}

		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode) && bPowerDown)
		{
			eError = PVRSRVSetSystemPowerState(psDeviceNode->psDevConfig, PVRSRV_SYS_POWER_STATE_OFF);
			PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVSetSystemPowerState OFF");
		}

		/* Extract the BVNC config from the Features table */
		if (BVNC_PACK(B,0,N,C) != 0)
		{
			*pui32B = B;
			*pui32V = V;
			*pui32N = N;
			*pui32C = C;

			return PVRSRV_OK;
		}
		else if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
		{
			/*
			 * On host OS we should not get here as CORE_ID should not be zero, so flag an error.
			 * On older cores, guest OS only has CORE_ID if defined(RGX_FEATURE_COREID_PER_OS)
			 */
			PVR_LOG_ERROR(PVRSRV_ERROR_DEVICE_REGISTER_FAILED, "CORE_ID register returns zero. Unknown BVNC");
		}
	}

	return PVRSRV_ERROR_BVNC_UNSUPPORTED;
}
#endif

static PVRSRV_ERROR RGXGetBVNCFromBuild(IMG_UINT32 *pui32B,
                                        IMG_UINT32 *pui32V,
                                        IMG_UINT32 *pui32N,
                                        IMG_UINT32 *pui32C)
{

#if defined(RGX_BVNC_KM_B) && defined(RGX_BVNC_KM_N) && defined(RGX_BVNC_KM_C)
	IMG_UINT32 B=0, V=0, N=0, C=0;
	IMG_CHAR acVStr[5] = RGX_BVNC_KM_V_ST;

	/* We reach here if the HW is not present,
	 * or we are running in a guest OS with no COREID_PER_OS feature,
	 * or HW is unstable during register read giving invalid values,
	 * or runtime detection has been disabled - fall back to compile time BVNC
	 */
	B = RGX_BVNC_KM_B;
	N = RGX_BVNC_KM_N;
	C = RGX_BVNC_KM_C;

	/* Clear any 'p' that may have been in RGX_BVNC_KM_V_ST,
		* as OSStringToUINT32() will otherwise return an error.
		*/
	if (acVStr[strlen(acVStr)-1] == 'p')
	{
		acVStr[strlen(acVStr)-1] = '\0';
	}

	if (OSStringToUINT32(&acVStr[0], 0, &V) != PVRSRV_OK)
	{
		V = 0;
	}
	PVR_LOG(("Reverting to compile time BVNC %s", RGX_BVNC_KM));

	*pui32B = B;
	*pui32V = V;
	*pui32N = N;
	*pui32C = C;

	return PVRSRV_OK;
#else /* defined(RGX_BVNC) */

	return PVRSRV_ERROR_BVNC_UNSUPPORTED;
#endif /* defined(RGX_BVNC) */
}


/* This function detects the Rogue variant and configures the essential
 * config info associated with such a device.
 * The config info includes features, errata, etc
 */
PVRSRV_ERROR RGXBvncInitialiseConfiguration(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	static IMG_UINT32 ui32RGXDevCnt = 0;
	PVRSRV_ERROR eError;
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	IMG_UINT64 ui64BVNC;
	IMG_UINT32 B=0, V=0, N=0, C=0;
	const RGX_BVNC_KM_FEATURE_TABLE_ENTRY *psFeatureConfig = NULL;
	const IMG_UINT64 *pui64Cfg = NULL;
	IMG_UINT32 ui32Cores = 1U;

	eError = RGXGetBVNCFromModParam(ui32RGXDevCnt, &B, &V, &N, &C, &psFeatureConfig);
	if (eError != PVRSRV_OK)
	{
		IMG_UINT64 ui64BNC;
		/* If we don't find the BVNC here, something is really wrong. */
#if !defined(NO_HARDWARE)
		eError = RGXGetBVNCFromHW(psDeviceNode, &B, &V, &N, &C, &ui32Cores);
		if (eError != PVRSRV_OK)
#endif
		{
			eError = RGXGetBVNCFromBuild(&B, &V, &N, &C);
			PVR_LOG_RETURN_IF_ERROR(eError, "RGXGetBVNCFromBuild");
		}

		ui64BNC = BVNC_PACK(B, 0, N, C);
		psFeatureConfig = RGXGetFeatureEntryFromBVNC(ui64BNC);
	}

	/* Have we failed to identify the BVNC to use? */
	if (psFeatureConfig == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: BVNC Detection and feature lookup failed. "
		    "Unsupported BVNC: %u.%u.%u.%u", __func__, B, V, N, C));
		return PVRSRV_ERROR_BVNC_UNSUPPORTED;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "%s: BVNC 0x%016"IMG_UINT64_FMTSPECx" feature config found", __func__, psFeatureConfig->ui64BVNC));

#if defined(DEBUG)
	{
		IMG_UINT32 i;

		PVR_DPF((PVR_DBG_MESSAGE, "Features without values:"));
		for (i = 0; i < RGX_BVNC_KM_FEATURE_FLAG_ARRAY_SIZE; i++)
		{
			PVR_DPF((PVR_DBG_MESSAGE,
			         "\t[%u]: 0x%016"IMG_UINT64_FMTSPECx,
			         i,
			         psFeatureConfig->aui64FeatureFlags[i]));
		}

		PVR_DPF((PVR_DBG_MESSAGE, "Features with values:"));
		for (i = 0; i < RGX_BVNC_KM_FEATURE_VALUE_ARRAY_SIZE; i++)
		{
			PVR_DPF((PVR_DBG_MESSAGE,
			         "\t[%u]: 0x%016"IMG_UINT64_FMTSPECx,
			         i,
			         psFeatureConfig->aui64FeatureValues[i]));
		}
	}
#endif

	/* Parsing feature config depends on available features on the core
	* hence this parsing should always follow the above feature assignment */
	psDevInfo->sDevFeatureCfg.paui64Features = psFeatureConfig->aui64FeatureFlags;

	eError = _RGXBvncParseFeatureValues(psDevInfo, psFeatureConfig);
	PVR_RETURN_IF_ERROR(eError);


	/* Add 'V' to the packed BVNC value to get the BVNC ERN and BRN config. */
	ui64BVNC = BVNC_PACK(B,V,N,C);
	pui64Cfg = RGXGetERNBRNFromBVNC(ui64BVNC);
	if (NULL == pui64Cfg)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: BVNC ERN/BRN lookup failed. "
			"Unsupported BVNC: 0x%016" IMG_UINT64_FMTSPECx, __func__, ui64BVNC));
		psDevInfo->sDevFeatureCfg.ui64ErnsBrns = 0;
		return PVRSRV_ERROR_BVNC_UNSUPPORTED;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "%s: BVNC ERN/BRN Cfg: 0x%016" IMG_UINT64_FMTSPECx
		" 0x%016" IMG_UINT64_FMTSPECx, __func__, *pui64Cfg, pui64Cfg[1]));
	psDevInfo->sDevFeatureCfg.ui64ErnsBrns = pui64Cfg[1];


	psDevInfo->sDevFeatureCfg.ui32B = B;
	psDevInfo->sDevFeatureCfg.ui32V = V;
	psDevInfo->sDevFeatureCfg.ui32N = N;
	psDevInfo->sDevFeatureCfg.ui32C = C;


	/* Message to confirm configuration look up was a success */
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, GPU_MULTICORE_SUPPORT) &&
		!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
#if defined(NO_HARDWARE)
		{
			PVR_UNREFERENCED_PARAMETER(ui32Cores);
			PVR_LOG(("RGX Device registered with BVNC " RGX_BVNC_STR_FMTSPEC,
					B, V, N, C));
		}
#else
		{
			PVR_LOG(("RGX Device registered BVNC " RGX_BVNC_STR_FMTSPEC
					" with %u %s in the system", B ,V ,N ,C, ui32Cores ,
					((ui32Cores == 1U)?"core":"cores")));
		}
#endif
	}
	else
	{
		PVR_LOG(("RGX Device registered with BVNC " RGX_BVNC_STR_FMTSPEC,
					B, V, N, C));
	}

	ui32RGXDevCnt++;

#if defined(DEBUG) || defined(SUPPORT_PERFORMANCE_RUN)
	_RGXBvncDumpParsedConfig(psDeviceNode);
#endif
	return PVRSRV_OK;
}

/*
 * This function checks if a particular feature is available on the given rgx device */
IMG_BOOL RGXBvncCheckFeatureSupported(PVRSRV_DEVICE_NODE *psDeviceNode, IMG_UINT16 ui16FeatureArrayIndex, IMG_UINT64 ui64FeatureMask)
{
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;

	if (psDevInfo->sDevFeatureCfg.paui64Features[ui16FeatureArrayIndex] & ui64FeatureMask)
	{
		return IMG_TRUE;
	}
	return IMG_FALSE;
}

/*
 * This function returns the value of a feature on the given rgx device */
IMG_INT32 RGXBvncGetSupportedFeatureValue(PVRSRV_DEVICE_NODE *psDeviceNode, RGX_FEATURE_WITH_VALUE_INDEX eFeatureIndex)
{
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;

	if (eFeatureIndex >= RGX_FEATURE_WITH_VALUES_MAX_IDX)
	{
		return -1;
	}

	if (psDevInfo->sDevFeatureCfg.ui32FeaturesValues[eFeatureIndex] == RGX_FEATURE_VALUE_DISABLED)
	{
		return -1;
	}

	return psDevInfo->sDevFeatureCfg.ui32FeaturesValues[eFeatureIndex];
}

/**************************************************************************/ /*!
@Function       RGXVerifyBVNC
@Description    Checks that the device's BVNC registers have the correct values.
@Input          psDeviceNode		Device node
@Return         PVRSRV_ERROR
*/ /***************************************************************************/
#define NUM_RGX_CORE_IDS    8
PVRSRV_ERROR RGXVerifyBVNC(PVRSRV_DEVICE_NODE *psDeviceNode, IMG_UINT64 ui64GivenBVNC, IMG_UINT64 ui64CoreIdMask)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT64 ui64MatchBVNC;
#if !defined(NO_HARDWARE)
	IMG_UINT32 B=0, V=0, N=0, C=0;
#endif
	IMG_UINT32 i;

	PVR_ASSERT(psDeviceNode != NULL);
	PVR_ASSERT(psDeviceNode->pvDevice != NULL);

	/* The device info */
	psDevInfo = psDeviceNode->pvDevice;

	PDUMPCOMMENT(psDeviceNode, "PDUMP VERIFY CORE_ID registers for all OSIDs\n");

	/* construct the value to match against */
	if ((ui64GivenBVNC | ui64CoreIdMask) == 0) /* both zero means use configured DDK value */
	{
		ui64MatchBVNC = rgx_bvnc_pack(psDevInfo->sDevFeatureCfg.ui32B,
									psDevInfo->sDevFeatureCfg.ui32V,
									psDevInfo->sDevFeatureCfg.ui32N,
									psDevInfo->sDevFeatureCfg.ui32C);
	}
	else
	{
		/* use the value in CORE_ID for any zero elements in the BVNC */
#if !defined(NO_HARDWARE)
		IMG_UINT64 ui64BVNC = _RGXReadBVNCFromReg(psDeviceNode, 0, &B, &V, &N, &C);
		ui64MatchBVNC = (ui64GivenBVNC & ~ui64CoreIdMask) | (ui64BVNC & ui64CoreIdMask);
#else
		ui64MatchBVNC = 0;
#endif
	}
	PVR_LOG(("matchBVNC %d.%d.%d.%d",
	        (int) ((ui64MatchBVNC >> RGX_BVNC_PACK_SHIFT_B) & 0xffff),
	        (int) ((ui64MatchBVNC >> RGX_BVNC_PACK_SHIFT_V) & 0xffff),
	        (int) ((ui64MatchBVNC >> RGX_BVNC_PACK_SHIFT_N) & 0xffff),
	        (int) ((ui64MatchBVNC >> RGX_BVNC_PACK_SHIFT_C) & 0xffff)));

	/* read in all the CORE_ID registers */
	for (i = 0; i < NUM_RGX_CORE_IDS; ++i)
	{
#if !defined(NO_HARDWARE)
		IMG_UINT64 ui64BVNC = _RGXReadBVNCFromReg(psDeviceNode, i, &B, &V, &N, &C);
		PVR_LOG(("CORE_ID%d returned %d.%d.%d.%d", i, B, V, N, C));

		if (ui64BVNC != ui64MatchBVNC)
		{
			eError = PVRSRV_ERROR_BVNC_MISMATCH;
			PVR_DPF((PVR_DBG_ERROR, "%s: Invalid CORE_ID%d %d.%d.%d.%d, Expected %d.%d.%d.%d", __func__, i,
					B, V, N, C,
			        (int) ((ui64MatchBVNC >> RGX_BVNC_PACK_SHIFT_B) & 0xffff),
			        (int) ((ui64MatchBVNC >> RGX_BVNC_PACK_SHIFT_V) & 0xffff),
			        (int) ((ui64MatchBVNC >> RGX_BVNC_PACK_SHIFT_N) & 0xffff),
			        (int) ((ui64MatchBVNC >> RGX_BVNC_PACK_SHIFT_C) & 0xffff)));
			break;
		}
#endif

	}

	return eError;
}
