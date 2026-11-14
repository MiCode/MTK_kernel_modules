# SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
#
# (C) COPYRIGHT 2025 ARM Limited. All rights reserved.
#
# This program is free software and is provided to you under the terms of the
# GNU General Public License version 2 as published by the Free Software
# Foundation, and any use by you of this program is subject to the terms
# of such GNU license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you can access it online at
# http://www.gnu.org/licenses/gpl-2.0.html.
#
#

KUTF_CFLAGS = [
    "-Wno-sign-compare",
    "-Wno-unused-but-set-variable",
    "-Wno-unused-parameter",
]

CFLAGS_MODULE = [
    "-Wall",
    "-Werror",
    "-Wextra",
    "-Wunused",
    "-Wno-unused-parameter",
    "-Wmissing-declarations",
    "-Wno-missing-prototypes",
    "-Wold-style-definition",
    "-Wunused-const-variable",
    "-Wno-sign-compare",
    "-Wno-shift-negative-value",
    "-Wno-cast-function-type",
    "-Wframe-larger-than=4096",
    "-Wdisabled-optimization",
    "-Wno-missing-field-initializers",
    "-Wno-type-limits",
    "-Wno-unused-macros",
    "-Wno-unused-variable",
    "-Wno-unused-function",
    "-Wno-unused-but-set-variable",
    "-Wno-visibility",
]

COPTS_KBASE = [
    "MALI_COVERAGE=0",
    "MALI_JIT_PRESSURE_LIMIT_BASE=0",
    "MALI_USE_CSF=0",
] + select({
    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/mali-r49p1/config:mali_debug": ["MALI_UNIT_TEST=1"],
    "//conditions:default": ["MALI_UNIT_TEST=0"],
}) + select({

    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/mali-r49p1/config:mali_has_cl": ["MALI_HAS_CL=1"],
    "//conditions:default": ["MALI_HAS_CL=0"],
}) + select({
    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/mali-r49p1/config:mali_has_gles": ["MALI_HAS_GLES=1"],
    "//conditions:default": ["MALI_HAS_GLES=0"],
}) + select({
    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/mali-r49p1/config:mali_customer_release": ["MALI_CUSTOMER_RELEASE=0"],
    "//conditions:default": ["MALI_CUSTOMER_RELEASE=0"],
}) + select({
    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/mali-r49p1/config:mali_kutf": ["MALI_KERNEL_TEST_API=1"],
    "//conditions:default": ["MALI_KERNEL_TEST_API=0"],
}) + select({
    "//conditions:default": [
        "MALI_RELEASE_NAME=\"r49p1-03bet0\"",
    ],
    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/mali-r49p1/config:mali_release_name_r49p1-03bet0": [
        "MALI_RELEASE_NAME=\"r49p1-03bet0\"",
    ],
})  + select({
    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/mali-r49p1/config:cov_kernel": [
        "GCOV_PROFILE=1",
        "-ftest-coverage",
        "-fprofile-arcs",
    ],
    "//conditions:default": [],
}) + select({
    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/mali-r49p1/config:mali_kcov": [
        "KCOV=1",
        "KCOV_ENABLE_COMPARISONS=1",
        "-fsanitize-coverage=trace-cmp",
    ],
    "//conditions:default": [],
}) 
COPTS_MTK = [
    "-I$(srctree)/include",
    "-I$(srctree)/drivers/staging/android",
    "-I$(srctree)/drivers/misc/mediatek/base/power/include",
    "-I$(srctree)/drivers/misc/mediatek/gpu/ged/include",
    "-I$(srctree)/drivers/misc/mediatek/sspm/",
    "-I$(srctree)/drivers/misc/mediatek/sspm/v3",
    "-I$(srctree)/drivers/misc/mediatek/qos",
    "-I$(srctree)/drivers/misc/mediatek/gpu/gpu_bm",
    "-I$(srctree)/drivers/misc/mediatek/include",
    "-I$(srctree)/drivers/misc/mediatek/trusted_mem/public",
    "-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/trusted_mem/public",
    "-I$(srctree)/drivers/gpu/mediatek/ged/include",
    "-I$(srctree)/drivers/gpu/mediatek/gpueb/include",
    "-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpu_bm",
    "-I$(srctree)/drivers/gpu/mediatek/mt-plat",
    "-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/mt-plat",
    "-I$(srctree)/drivers/gpu/mediatek/gpufreq",
    "-I$(DEVICE_MODULES_PATH)/drivers/gpu/mediatek/gpufreq",
    "-I$(srctree)/drivers/gpu/mediatek/gpueb/include",
    "-I$(srctree)/drivers/staging/android/ion",
    "-I$(srctree)/drivers/iommu",
    "-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat",
    "-I$(srctree)/drivers/misc/mediatek/sda/btm/v1",
    "-I$(srctree)/drivers/misc/mediatek/slbc",
    "-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/perf_common",
    "-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/tinysys_scmi",
    "-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/",
]
