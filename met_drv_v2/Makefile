MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
MET_ROOT_DIR := $(src)
MET_COMMON_DIR := $(wildcard $(MET_ROOT_DIR)/common)
MET_BUILD_DEFAULT := n

ifneq ($(MTK_PLATFORM),)
    MET_PLF_DIR := $(wildcard $(MET_ROOT_DIR)/$(MTK_PLATFORM))
else
    MET_PLF_DIR :=
endif

ifeq ($(CONFIG_MODULES),y)

ifeq ($(CONFIG_FTRACE),y)
    ifeq ($(CONFIG_TRACING),y)
        FTRACE_READY := y
    endif
endif

ifeq ($(CONFIG_MTK_MET_PLF),m)
    MET_BUILD_KO := y
endif

$(info ******** Start to build met_drv for $(MTK_PLATFORM) ********)
ifneq ($(MET_PLF_DIR),)
    ifeq ($(FTRACE_READY),y)
        ifeq ($(MET_BUILD_KO),y)
            include $(MET_COMMON_DIR)/Kbuild
        else
            $(warning Not building met.ko due to CONFIG_MTK_MET_PLF is not set to m, build met default)
            MET_BUILD_DEFAULT = y
        endif
    else
        $(warning Not building met.ko due to CONFIG_FTRACE/CONFIG_TRACING is not set, build met default)
        MET_BUILD_DEFAULT = y
    endif
else
    $(warning not support "$(MTK_PLATFORM)", build met default)
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
