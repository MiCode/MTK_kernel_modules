MET_ROOT_DIR := $(if $(filter /%,$(src)),,$(srctree)/)$(src)
MET_COMMON_DIR := $(wildcard $(MET_ROOT_DIR)/common)
MET_API_DIR := $(wildcard $(MET_ROOT_DIR)/met_api)
MET_BUILD_DEFAULT := n

ifeq ($(CONFIG_MODULES),y)

ifeq ($(CONFIG_FTRACE),y)
    ifeq ($(CONFIG_TRACING),y)
        FTRACE_READY := y
    endif
endif

ifeq ($(CONFIG_MTK_MET),m)
    MET_BUILD_KO := y
endif

$(info ******** Start to build met_drv ********)
ifneq (,$(filter $(CONFIG_MTK_MET),y m))
    ifeq ($(FTRACE_READY),y)
        ifeq ($(MET_BUILD_KO),y)
            include $(MET_COMMON_DIR)/Kbuild
        else
            $(warning Not building met.ko due to CONFIG_MTK_MET is not set to m, build met default)
            MET_BUILD_DEFAULT = y
        endif
    else
        $(warning Not building met.ko due to CONFIG_FTRACE/CONFIG_TRACING is not set, build met default)
        MET_BUILD_DEFAULT = y
    endif
else
    $(warning not support CONFIG_MTK_MET="$(CONFIG_MTK_MET)", build met default)
    MET_BUILD_DEFAULT = y
endif
else #CONFIG_MODULES = n
    $(warning Not building met.ko due to CONFIG_MODULES is not set, build met default)
    MET_BUILD_DEFAULT := y
endif

ifeq ($(MET_BUILD_DEFAULT),y)
    MET_DEF_DIR := $(MET_ROOT_DIR)/default
    include $(MET_DEF_DIR)/Kbuild
endif
