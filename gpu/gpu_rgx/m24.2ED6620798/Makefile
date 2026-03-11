OPTIM = -Os

MTK_PLATFORM := $(MTK_PLATFORM_VERSION)

ifeq ($(MTK_PLATFORM),mt6855)
	PVR_ARCH := volcanic
	CONFIG_MACH_MT6855 := y
else ifeq ($(MTK_PLATFORM),mt6761)
	PVR_ARCH := rogue
	CONFIG_MACH_MT6761 := y
	ccflags-y += -DCONFIG_MACH_MT6761
else ifeq ($(MTK_PLATFORM),mt6739)
	PVR_ARCH := rogue
	CONFIG_MACH_MT6739 := y
else ifeq ($(MTK_PLATFORM),mt6765)
	PVR_ARCH := rogue
	CONFIG_MACH_MT6765 := y
endif

ccflags-$(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) += -DSSPM_DEBUG

RGX_TOP := $(if $(filter /%,$(src)),,$(srctree)/)$(src)

$(info [GPU_DEBUG] MTK_PLATFORM = $(MTK_PLATFORM))
$(info [GPU_DEBUG] RGX_TOP=$(RGX_TOP))

ifeq ($(MTK_DDK_DEBUG),y)
	ccflags-y += -include $(RGX_TOP)/include/config_kernel_debug_$(MTK_PLATFORM).h
else ifeq ($(CONFIG_MTK_ENG_BUILD),y)
	ccflags-y += -include $(RGX_TOP)/include/config_kernel_eng_$(MTK_PLATFORM).h
else
	ccflags-y += -include $(RGX_TOP)/include/config_kernel_user_$(MTK_PLATFORM).h
endif

ccflags-y += \
	-D__linux__ $(OPTIM) -g \
	-fno-strict-aliasing \
	-W -Wall \
	-Wno-missing-field-initializers \
	-Wdeclaration-after-statement \
	-Wno-format-zero-length \
	-Wmissing-prototypes \
	-Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-sign-compare \
	-Wno-type-limits \
	-Wno-error

pvrsrvkm_$(MTK_PLATFORM)-y += \
	generated/$(PVR_ARCH)/cache_bridge/server_cache_bridge.o \
	generated/$(PVR_ARCH)/cache_bridge/client_cache_direct_bridge.o \
	generated/$(PVR_ARCH)/rgxta3d_bridge/server_rgxta3d_bridge.o \
	generated/$(PVR_ARCH)/sync_bridge/client_sync_direct_bridge.o \
	generated/$(PVR_ARCH)/sync_bridge/server_sync_bridge.o \
	generated/$(PVR_ARCH)/synctracking_bridge/client_synctracking_direct_bridge.o \
	generated/$(PVR_ARCH)/synctracking_bridge/server_synctracking_bridge.o \
	generated/$(PVR_ARCH)/srvcore_bridge/server_srvcore_bridge.o \
	generated/$(PVR_ARCH)/devicememhistory_bridge/client_devicememhistory_direct_bridge.o \
	generated/$(PVR_ARCH)/devicememhistory_bridge/server_devicememhistory_bridge.o \
	generated/$(PVR_ARCH)/rgxcmp_bridge/server_rgxcmp_bridge.o \
	generated/$(PVR_ARCH)/rgxhwperf_bridge/server_rgxhwperf_bridge.o \
	generated/$(PVR_ARCH)/rgxfwdbg_bridge/server_rgxfwdbg_bridge.o \
	generated/$(PVR_ARCH)/rgxregconfig_bridge/server_rgxregconfig_bridge.o \
	generated/$(PVR_ARCH)/cmm_bridge/server_cmm_bridge.o \
	generated/$(PVR_ARCH)/mm_bridge/client_mm_direct_bridge.o \
	generated/$(PVR_ARCH)/mm_bridge/server_mm_bridge.o \
	generated/$(PVR_ARCH)/rgxkicksync_bridge/server_rgxkicksync_bridge.o \
	generated/$(PVR_ARCH)/pvrtl_bridge/server_pvrtl_bridge.o \
	generated/$(PVR_ARCH)/pvrtl_bridge/client_pvrtl_direct_bridge.o \
	generated/$(PVR_ARCH)/rgxtq2_bridge/server_rgxtq2_bridge.o \
	generated/$(PVR_ARCH)/dmabuf_bridge/server_dmabuf_bridge.o \
	generated/$(PVR_ARCH)/rgxtimerquery_bridge/server_rgxtimerquery_bridge.o \
	generated/$(PVR_ARCH)/di_bridge/server_di_bridge.o \
	services/system/$(PVR_ARCH)/rgx_mtk/ion_support.o \
	services/system/$(PVR_ARCH)/rgx_mtk/sysconfig.o \
	services/system/$(PVR_ARCH)/rgx_mtk/mtk_pp.o \
	services/system/$(PVR_ARCH)/common/vmm_type_stub.o \
	services/system/common/env/linux/interrupt_support.o \
	services/system/$(PVR_ARCH)/common/env/linux/dma_support.o \
	services/server/devices/rgxtimecorr.o \
	services/server/devices/rgx_bridge_init.o \
	services/server/devices/rgxfwdbg.o \
	services/server/devices/rgxshader.o \
	services/server/devices/rgxfwtrace_strings.o \
	services/server/devices/rgxutils.o \
	services/server/devices/rgxmem.o \
	services/server/devices/rgxfwimageutils.o \
	services/server/devices/rgxccb.o \
	services/server/devices/rgxdebug_common.o \
	services/server/devices/rgxbreakpoint.o \
	services/server/devices/rgxbvnc.o \
	services/server/devices/rgxkicksync.o \
	services/server/devices/rgxregconfig.o \
	services/server/devices/rgxsyncutils.o \
	services/server/devices/rgxtimerquery.o \
	services/server/devices/rgxhwperf_common.o \
	services/server/devices/$(PVR_ARCH)/rgxhwperf.o \
	services/server/devices/$(PVR_ARCH)/rgxfwutils.o \
	services/server/devices/rgxfwcmnctx.o \
	services/server/devices/rgxfwriscv.o \
	services/server/devices/rgxmmuinit.o \
	services/server/devices/$(PVR_ARCH)/rgxinit.o \
	services/server/devices/$(PVR_ARCH)/rgxlayer_impl.o \
	services/server/devices/rgxlayer_impl_common.o \
	services/server/devices/rgxtdmtransfer.o \
	services/server/devices/$(PVR_ARCH)/rgxdebug.o \
	services/server/devices/$(PVR_ARCH)/rgxta3d.o \
	services/server/devices/$(PVR_ARCH)/rgxsrvinit.o \
	services/server/devices/rgxcompute.o \
	services/server/devices/$(PVR_ARCH)/rgxstartstop.o \
	services/server/devices/rgxpower.o \
	services/server/devices/$(PVR_ARCH)/rgxmulticore.o \
	services/server/common/tlstream.o \
	services/server/common/pvrsrv_pool.o \
	services/server/common/cache_km.o \
	services/server/common/tlintern.o \
	services/server/common/pvr_notifier.o \
	services/server/common/devicemem_history_server.o \
	services/server/common/physmem.o \
	services/server/common/tlserver.o \
	services/server/common/physheap.o \
	services/server/common/handle.o \
	services/server/common/devicemem_heapcfg.o \
	services/server/common/sync_server.o \
	services/server/common/sync_checkpoint.o \
	services/server/common/lists.o \
	services/server/common/info_page_km.o \
	services/server/common/physmem_lma.o \
	services/server/common/physmem_ramem.o \
	services/server/common/physmem_osmem.o \
	services/server/common/process_stats.o \
	services/server/common/connection_server.o \
	services/server/common/pmr.o \
	services/server/common/pvrsrv_bridge_init.o \
	services/server/common/mmu_common.o \
	services/server/common/devicemem_server.o \
	services/server/common/power.o \
	services/server/common/physmem_hostmem.o \
	services/server/common/pvrsrv.o \
	services/server/common/srvcore.o \
	services/server/common/vz_vmm_pvz.o \
	services/server/common/vz_vmm_vm.o \
	services/server/common/vmm_pvz_client.o \
	services/server/common/vmm_pvz_server.o \
	services/server/common/di_server.o \
	services/server/common/di_impl_brg.o \
	services/server/common/debug_common.o \
	services/server/env/linux/km_apphint.o \
	services/server/env/linux/osmmap_stub.o \
	services/server/env/linux/physmem_test.o \
	services/server/env/linux/allocmem.o \
	services/server/env/linux/pvr_bridge_k.o \
	services/server/env/linux/physmem_dmabuf.o \
	services/server/env/linux/pvr_debugfs.o \
	services/server/env/linux/osconnection_server.o \
	services/server/env/linux/handle_idr.o \
	services/server/env/linux/event.o \
	services/server/env/linux/fwload.o \
	services/server/env/linux/pmr_os.o \
	services/server/env/linux/pmr_env.o \
	services/server/env/linux/module_common.o \
	services/server/env/linux/physmem_osmem_linux.o \
	services/server/env/linux/osfunc.o \
	services/server/env/linux/pvr_debug.o \
	services/server/env/linux/pvr_procfs.o \
	services/server/env/linux/pvr_drm.o \
	services/server/env/linux/pvr_platform_drv.o \
	services/shared/common/devicemem_utils.o \
	services/shared/common/hash.o \
services/shared/common/hash_functions.o \
	services/shared/common/ra.o \
	services/shared/common/sync.o \
	services/shared/common/uniq_key_splay_tree.o \
	services/shared/common/tlclient.o \
	services/shared/common/devicemem.o \
	services/server/env/linux/dkf_server.o \
	services/shared/common/mem_utils.o \
	services/shared/common/pvrsrv_error.o

ifeq ($(PVRSRV_ENABLE_DYNAMIC_PHYSHEAPS),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
		services/server/common/physmem_dlm.o \
		services/server/common/physmem_ima.o
endif

ifeq ($(PVRSRV_ENABLE_PVR_ION_STATS),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
		services/server/env/linux/pvr_ion_stats.o
endif


	pvrsrvkm_$(MTK_PLATFORM)-$(CONFIG_EVENT_TRACING) += services/server/env/linux/pvr_gputrace.o


$(info [GPU_DEBUG] PVRSRV_TRACE_ROGUE_EVENTS = $(PVRSRV_TRACE_ROGUE_EVENTS))
$(info [GPU_DEBUG] PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD=$(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD))


ifeq ($(PVRSRV_ENABLE_HTB),1)
pvrsrvkm_$(MTK_PLATFORM)-y += \
services/shared/common/htbuffer.o \
services/server/common/htbserver.o \
generated/$(PVR_ARCH)/htbuffer_bridge/server_htbuffer_bridge.o \
generated/$(PVR_ARCH)/htbuffer_bridge/client_htbuffer_direct_bridge.o \
services/server/common/htb_debug.o
endif

	pvrsrvkm_$(MTK_PLATFORM)-y += \
	 services/server/env/linux/pvr_gpufreq.o


ifeq ($(SUPPORT_DMA_TRANSFER),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
		services/server/common/dma_km.o
endif

ifeq ($(SUPPORT_DMA_TRANSFER),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
		generated/$(PVR_ARCH)/dma_bridge/server_dma_bridge.o
endif

ifeq ($(SUPPORT_VALIDATION),1)
	ccflags-y += -I$(RGX_TOP)/generated/$(PVR_ARCH)/validation_bridge
	pvrsrvkm_$(MTK_PLATFORM)-y += generated/$(PVR_ARCH)/validation_bridge/server_validation_bridge.o
	pvrsrvkm_$(MTK_PLATFORM)-y += services/server/common/validation.o
	pvrsrvkm_$(MTK_PLATFORM)-y += services/server/common/validation_soc.o
endif

ifeq ($(PVR_TESTING_UTILS),1)
	ccflags-y += -I$(RGX_TOP)/generated/$(PVR_ARCH)/tutils_bridge
	pvrsrvkm_$(MTK_PLATFORM)-y += generated/$(PVR_ARCH)/tutils_bridge/server_tutils_bridge.o
endif

pvrsrvkm_$(MTK_PLATFORM)-$(CONFIG_PCI) += \
 services/system/common/env/linux/pci_support.o

ifeq ($(MTK_PLATFORM),mt6761)
ifneq ($(filter $(CONFIG_MTK_GPUFREQ_V2),y m),)
pvrsrvkm_$(MTK_PLATFORM)-y += \
	services/system/$(PVR_ARCH)/rgx_mtk/mt6761/mtk_mfgsys.o
endif
else
pvrsrvkm_$(MTK_PLATFORM)-y += \
	services/system/$(PVR_ARCH)/rgx_mtk/$(MTK_PLATFORM)/mtk_mfgsys.o
endif

ifeq ($(PVR_ARCH),volcanic)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
		generated/volcanic/rgxray_bridge/server_rgxray_bridge.o \
		services/server/devices/volcanic/rgxray.o
endif

ifeq ($(PVR_ARCH),rogue)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
		services/shared/devices/rogue/rgx_hwperf_table.o \
		services/server/devices/rogue/rgxmipsmmuinit.o \
		services/server/devices/rogue/rgxtransfer.o \
		generated/rogue/rgxtq_bridge/server_rgxtq_bridge.o \
		generated/rogue/rgxbreakpoint_bridge/server_rgxbreakpoint_bridge.o
endif
#TODO: Wrap in corresponding macros
pvrsrvkm_$(MTK_PLATFORM)-y += \
	generated/$(PVR_ARCH)/mmextmem_bridge/server_mmextmem_bridge.o \
	services/server/env/linux/physmem_extmem_linux.o \
	services/server/common/physmem_extmem.o

ifeq ($(SUPPORT_PDVFS),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
	 services/server/devices/rgxpdvfs.o
endif

ifeq ($(SUPPORT_LINUX_WRAP_EXTMEM_PAGE_TABLE_WALK),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
	 services/server/env/linux/pg_walk_through.o
endif

ifeq ($(SUPPORT_WORKLOAD_ESTIMATION),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
		services/server/devices/rgxworkest.o
 ifeq ($(PVR_ARCH),volcanic)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
		services/server/devices/$(PVR_ARCH)/rgxworkest_ray.o
 endif
endif


ifeq ($(SUPPORT_DISPLAY_CLASS),1)
	ccflags-y += -I$(RGX_TOP)/generated/$(PVR_ARCH)/dc_bridge
	pvrsrvkm_$(MTK_PLATFORM)-y += generated/$(PVR_ARCH)/dc_bridge/server_dc_bridge.o
endif

ifeq ($(SUPPORT_SECURE_EXPORT),1)
	ccflags-y += -I$(RGX_TOP)/generated/$(PVR_ARCH)/smm_bridge
	pvrsrvkm_$(MTK_PLATFORM)-y += generated/$(PVR_ARCH)/smm_bridge/server_smm_bridge.o
endif

ifeq ($(PDUMP),1)
	ccflags-y += \
	 -I$(RGX_TOP)/generated/$(PVR_ARCH)/pdump_bridge \
	 -I$(RGX_TOP)/generated/$(PVR_ARCH)/pdumpctrl_bridge \
	 -I$(RGX_TOP)/generated/$(PVR_ARCH)/pdumpmm_bridge

	ifeq ($(SUPPORT_RGX),1)
	ccflags-y += \
		-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxpdump_bridge
	pvrsrvkm_$(MTK_PLATFORM)-y += \
		generated/$(PVR_ARCH)/rgxpdump_bridge/server_rgxpdump_bridge.o \
		generated/$(PVR_ARCH)/rgxpdump_bridge/client_rgxpdump_direct_bridge.o
	endif

	pvrsrvkm_$(MTK_PLATFORM)-y += \
	 generated/$(PVR_ARCH)/pdump_bridge/server_pdump_bridge.o \
	 generated/$(PVR_ARCH)/pdumpctrl_bridge/server_pdumpctrl_bridge.o \
	 generated/$(PVR_ARCH)/pdumpmm_bridge/server_pdumpmm_bridge.o \
	 generated/$(PVR_ARCH)/pdumpmm_bridge/client_pdumpmm_direct_bridge.o \
	 generated/$(PVR_ARCH)/pdump_bridge/client_pdump_direct_bridge.o \
	 generated/$(PVR_ARCH)/pdumpctrl_bridge/client_pdumpctrl_direct_bridge.o
endif

ifeq ($(PVRSRV_ENABLE_GPU_MEMORY_INFO),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += generated/$(PVR_ARCH)/ri_bridge/client_ri_direct_bridge.o
endif

ifeq ($(PVRSRV_ENABLE_GPU_MEMORY_INFO),1)
	ccflags-y += -I$(RGX_TOP)/generated/$(PVR_ARCH)/ri_bridge
	pvrsrvkm_$(MTK_PLATFORM)-y += generated/$(PVR_ARCH)/ri_bridge/server_ri_bridge.o
endif

ifeq ($(SUPPORT_FALLBACK_FENCE_SYNC),1)
	ccflags-y += \
	 -I$(RGX_TOP)/generated/$(PVR_ARCH)/syncfallback_bridge
	pvrsrvkm_$(MTK_PLATFORM)-y += generated/$(PVR_ARCH)/syncfallback_bridge/server_syncfallback_bridge.o
endif

ifeq ($(SUPPORT_VALIDATION),1)
	ifeq ($(PVR_TESTING_UTILS),1)
		pvrsrvkm_$(MTK_PLATFORM)-y += \
			services/server/devices/rgxgpumap.o
	endif
endif

ifeq ($(SUPPORT_VALIDATION),1)
	 pvrsrvkm_$(MTK_PLATFORM)-y += \
	 services/server/devices/rgxsoctimer.o
endif


ifeq ($(SUPPORT_DISPLAY_CLASS),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
	 services/server/common/dc_server.o \
	 services/server/common/scp.o
endif

ifeq ($(SUPPORT_BUFFER_SYNC),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
	 services/server/env/linux/pvr_buffer_sync.o 
endif

pvrsrvkm_$(MTK_PLATFORM)-y += \
		services/server/env/linux/pvr_sync_ioctl_common.o \
		services/server/env/linux/pvr_sync_file.o \
		services/server/env/linux/pvr_counting_timeline.o \
		services/server/env/linux/pvr_sw_fence.o \
		services/server/env/linux/pvr_export_fence.o \
		services/server/env/linux/pvr_fence.o
pvrsrvkm_$(MTK_PLATFORM)-y += \
		services/server/env/linux/pvr_sync_ioctl_dev.o


ifeq ($(SUPPORT_FALLBACK_FENCE_SYNC),1)
pvrsrvkm_$(MTK_PLATFORM)-y += \
 services/server/common/sync_fallback_server.o \
 services/server/env/linux/pvr_sync2.o \
 services/server/env/linux/ossecure_export.o
endif

ifeq ($(PDUMP),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += \
	 services/server/common/pdump_server.o \
	 services/server/common/pdump_mmu.o \
	 services/server/common/pdump_physmem.o \
	 services/shared/common/devicemem_pdump.o \
	 services/shared/common/devicememx_pdump.o
	ifeq ($(SUPPORT_RGX),1)
		pvrsrvkm_$(MTK_PLATFORM)-y += \
			services/server/devices/$(PVR_ARCH)/rgxpdump.o	
		ifeq ($(PDUMP),1)
			pvrsrvkm_$(MTK_PLATFORM)-y += \
				services/server/devices/rgxpdump_common.o
		endif
	endif
endif

pvrsrvkm_$(MTK_PLATFORM)-$(CONFIG_ARM) += services/server/env/linux/osfunc_arm.o
pvrsrvkm_$(MTK_PLATFORM)-$(CONFIG_ARM64) += services/server/env/linux/osfunc_arm64.o
pvrsrvkm_$(MTK_PLATFORM)-$(CONFIG_X86) += services/server/env/linux/osfunc_x86.o
pvrsrvkm_$(MTK_PLATFORM)-$(CONFIG_METAG) += services/server/env/linux/osfunc_metag.o
pvrsrvkm_$(MTK_PLATFORM)-$(CONFIG_MIPS) += services/server/env/linux/osfunc_mips.o
pvrsrvkm_$(MTK_PLATFORM)-$(CONFIG_RISCV) += services/server/env/linux/osfunc_riscv.o
pvrsrvkm_$(MTK_PLATFORM)-$(CONFIG_EVENT_TRACING) += services/server/env/linux/trace_events.o

ifeq ($(PVRSRV_ENABLE_GPU_MEMORY_INFO),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += services/server/common/ri_server.o
endif

ifeq ($(PVR_TESTING_UTILS),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += services/server/common/tutils.o
endif

ifeq ($(PVRSRV_PHYSMEM_CPUMAP_HISTORY),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += services/server/common/physmem_cpumap_history.o
endif

ifeq ($(PVR_HANDLE_BACKEND),generic)
	pvrsrvkm_$(MTK_PLATFORM)-y += services/server/common/handle_generic.o
else
	ifeq ($(PVR_HANDLE_BACKEND),idr)
		pvrsrvkm_$(MTK_PLATFORM)-y += services/server/env/linux/handle_idr.o
	endif
endif

ifeq ($(PVRSRV_ENABLE_LINUX_MMAP_STATS),1)
	pvrsrvkm_$(MTK_PLATFORM)-y += services/server/env/linux/mmap_stats.o
endif

ccflags-y += \
	-I$(RGX_TOP)/hwdefs/$(PVR_ARCH) \
	-I$(RGX_TOP)/hwdefs/$(PVR_ARCH)/km \
	-I$(RGX_TOP)/hwdefs/$(PVR_ARCH)/km/configs \
	-I$(RGX_TOP)/hwdefs/$(PVR_ARCH)/km/cores \
	-I$(RGX_TOP)/include \
	-I$(RGX_TOP)/include/$(PVR_ARCH) \
	-I$(RGX_TOP)/include/drm \
	-I$(RGX_TOP)/include/public \
	-I$(RGX_TOP)/kernel/drivers/staging/imgtec \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/cache_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/cmm_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/dmabuf_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/mm_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/pvrtl_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxbreakpoint_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxcmp_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxfwdbg_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxhwperf_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxinit_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxkicksync_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxregconfig_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxtimerquery_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxta3d_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxray_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxtq_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxtq2_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/srvcore_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/sync_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/synctracking_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/devicememhistory_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/ri_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/di_bridge \
	-I$(RGX_TOP)/generated/$(PVR_ARCH)/rgxtimerquery_bridge/server_rgxtimerquery_bridge \
	-I$(RGX_TOP)/services/include \
	-I$(RGX_TOP)/services/include/$(PVR_ARCH) \
	-I$(RGX_TOP)/services/include/env/linux \
	-I$(RGX_TOP)/services/server/devices \
	-I$(RGX_TOP)/services/server/devices/$(PVR_ARCH) \
	-I$(RGX_TOP)/services/server/env/linux \
	-I$(RGX_TOP)/services/server/env/linux/$(PVR_ARCH) \
	-I$(RGX_TOP)/services/server/include \
	-I$(RGX_TOP)/services/server/common \
	-I$(RGX_TOP)/services/server/include/$(PVR_ARCH) \
	-I$(RGX_TOP)/services/shared/common \
	-I$(RGX_TOP)/services/shared/devices/$(PVR_ARCH) \
	-I$(RGX_TOP)/services/shared/include \
	-I$(RGX_TOP)/services/system/include \
	-I$(RGX_TOP)/services/system/$(PVR_ARCH)/include \
	-I$(RGX_TOP)/services/system/$(PVR_ARCH)/rgx_mtk
#TODO: WRAP IN CORRESPONDING MACRO
ccflags-y += -I$(RGX_TOP)/generated/$(PVR_ARCH)/mmextmem_bridge

ifeq ($(SUPPORT_DMA_TRANSFER),1)
	ccflags-y += \
	 -I$(RGX_TOP)/generated/$(PVR_ARCH)/dma_bridge
endif

ifeq ($(MTK_PLATFORM),mt6761)
ifneq ($(filter $(CONFIG_MTK_GPUFREQ_V2),y m),)
ccflags-y += \
	-I$(RGX_TOP)/services/system/$(PVR_ARCH)/rgx_mtk/mt6761
endif
else
ccflags-y += \
	-I$(RGX_TOP)/services/system/$(PVR_ARCH)/rgx_mtk/$(MTK_PLATFORM)
endif

ifeq ($(PVRSRV_ENABLE_HTB),1)
ccflags-y += \
 -I$(RGX_TOP)/generated/$(PVR_ARCH)/htbuffer_bridge
endif



ccflags-y += \
	-I$(DEVICE_MODULES_PATH)/include \
	-I$(DEVICE_MODULES_PATH)/drivers/staging/android/ion \
	-I$(DEVICE_MODULES_PATH)/drivers/staging/android/ion/mtk \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/ged/include \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/mt-plat \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpufreq/v2_legacy/include \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpufreq/ \
	-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpu_bm \
	-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat

ifneq ($(filter $(CONFIG_MTK_GPU_SUPPORT),y m),)
	obj-m += pvrsrvkm_$(MTK_PLATFORM).o
	# pvrsrvkm_$(MTK_PLATFORM)-y := pvrsrvkm.o
endif