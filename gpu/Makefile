define build_kernel_modules_mali
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2)/$(3)/drivers/gpu/arm/midgard modules $(KBUILD_OPTIONS) MTK_PLATFORM_VERSION=$(1) LOCAL_MTK_GPU_VERSION=$(4) BUILD_RULE=$(RULE)
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2)/$(3)/drivers/base/arm/memory_group_manager modules $(KBUILD_OPTIONS) MTK_PLATFORM_VERSION=$(1) CONFIG_MALI_MEMORY_GROUP_MANAGER=$(MEMORY_GROUP_MANAGER)
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2)/$(3)/drivers/base/arm/protected_memory_allocator modules $(KBUILD_OPTIONS) MTK_PLATFORM_VERSION=$(1) CONFIG_MALI_PROTECTED_MEMORY_ALLOCATOR=$(PROTECTED_MEMORY_ALLOCATOR)
endef

define install_kernel_modules_mali
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2)/$(3)/drivers/gpu/arm/midgard modules_install $(KBUILD_OPTIONS) MTK_PLATFORM_VERSION=$(1) LOCAL_MTK_GPU_VERSION=$(4) BUILD_RULE=$(RULE)
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2)/$(3)/drivers/base/arm/memory_group_manager modules_install $(KBUILD_OPTIONS) MTK_PLATFORM_VERSION=$(1) CONFIG_MALI_MEMORY_GROUP_MANAGER=$(MEMORY_GROUP_MANAGER)
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2)/$(3)/drivers/base/arm/protected_memory_allocator modules_install $(KBUILD_OPTIONS) MTK_PLATFORM_VERSION=$(1) CONFIG_MALI_PROTECTED_MEMORY_ALLOCATOR=$(PROTECTED_MEMORY_ALLOCATOR)
endef

define clean_kernel_modules_mali
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2)/$(3)/drivers/gpu/arm/midgard clean $(KBUILD_OPTIONS) MTK_PLATFORM_VERSION=$(1) LOCAL_MTK_GPU_VERSION=$(4) BUILD_RULE=$(RULE)
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2)/$(3)/drivers/base/arm/memory_group_manager clean $(KBUILD_OPTIONS) MTK_PLATFORM_VERSION=$(1) CONFIG_MALI_MEMORY_GROUP_MANAGER=$(MEMORY_GROUP_MANAGER)
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2)/$(3)/drivers/base/arm/protected_memory_allocator clean $(KBUILD_OPTIONS) MTK_PLATFORM_VERSION=$(1) CONFIG_MALI_PROTECTED_MEMORY_ALLOCATOR=$(PROTECTED_MEMORY_ALLOCATOR)
endef

define build_kernel_modules_img
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2) modules $(KBUILD_OPTIONS) MTK_PLATFORM=$(1) DDK_VERSION=$(2) BUILD_RULE=$(RULE)
endef

define install_kernel_modules_img
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2) modules_install $(KBUILD_OPTIONS) MTK_PLATFORM=$(1) DDK_VERSION=$(2) BUILD_RULE=$(RULE)
endef

define clean_kernel_modules_img
+$(MAKE) -C $(KERNEL_SRC) M=$(M)/$(1)/$(2) clean $(KBUILD_OPTIONS) MTK_PLATFORM=$(1) DDK_VERSION=$(2) BUILD_RULE=$(RULE)
endef

BUILD_RULE := OOT
CONFIG_MALI_MEMORY_GROUP_MANAGER := y
CONFIG_MALI_PROTECTED_MEMORY_ALLOCATOR := y

comma := ,
PARAMS :=

ifneq (,$(wildcard mt6983))
	PARAMS += mali,mt6983,mali_valhall,mali-r38p0,mali_valhall_r38p0_mt6983
endif

ifneq (,$(wildcard mt6985))
	PARAMS += mali,mt6985,mali_valhall,mali-r38p1,mali_valhall_r38p1_mt6985
endif

ifneq (,$(wildcard mt6886))
	PARAMS += mali,mt6886,mali_valhall,mali-r38p1,mali_valhall_r38p1_mt6886
endif

ifneq (,$(wildcard mt6835))
	PARAMS += mali,mt6835,mali_valhall,mali-r38p1,mali_valhall_r38p1_mt6835
endif

all modules_install clean: RULE := $(BUILD_RULE)
all modules_install clean: MEMORY_GROUP_MANAGER := $(CONFIG_MALI_MEMORY_GROUP_MANAGER)
all modules_install clean: PROTECTED_MEMORY_ALLOCATOR := $(CONFIG_MALI_PROTECTED_MEMORY_ALLOCATOR)

targets_build := $(addprefix build_,$(PARAMS))
all: $(targets_build)
$(targets_build):
	$(eval param=$(subst $(comma), ,$(subst build_,,$@)))
	$(call build_kernel_modules_$(word 1,$(param)),$(word 2,$(param)),$(word 3,$(param)),$(word 4,$(param)),$(word 5,$(param)))

targets_install := $(addprefix install_,$(PARAMS))
modules_install: $(targets_install)
$(targets_install):
	$(eval param=$(subst $(comma), ,$(subst install_,,$@)))
	$(call install_kernel_modules_$(word 1,$(param)),$(word 2,$(param)),$(word 3,$(param)),$(word 4,$(param)),$(word 5,$(param)))

targets_clean := $(addprefix clean_,$(PARAMS))
clean: $(targets_clean)
$(targets_clean):
	$(eval param=$(subst $(comma), ,$(subst clean_,,$@)))
	$(call clean_kernel_modules_$(word 1,$(param)),$(word 2,$(param)),$(word 3,$(param)),$(word 4,$(param)),$(word 5,$(param)))
