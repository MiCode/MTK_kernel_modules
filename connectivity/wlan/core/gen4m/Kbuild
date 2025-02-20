################################
## Add specified modules here ##
################################
$(info MODULE_NAME $(MODULE_NAME))
$(info Segment: $(SEGMENT))

KO_CODE_PATH := $(if $(filter /%,$(src)),,$(srctree)/)$(src)
ifeq ($(SEGMENT), SP)
    # build ko by connac version
    include $(KO_CODE_PATH)/Kbuild.$(subst wlan_drv_gen4m_,,$(MODULE_NAME))
else
    include $(KO_CODE_PATH)/Kbuild.main
endif
