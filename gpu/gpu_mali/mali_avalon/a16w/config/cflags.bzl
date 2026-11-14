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

# + select({
#    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/a16w/config:mali_gcov_kernel": [
#        "-DGCOV_PROFILE=1",
#        "-ftest-coverage",
#        "-fprofile-arcs",
#    ],
#    "//conditions:default": [],
#}) + select({
#    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/a16w/config:mali_kcov": [
#        "-DKCOV=1",
#        "-DKCOV_ENABLE_COMPARISONS=1",
#        "-fsanitize-coverage=trace-cmp",
#    ],
#    "//conditions:default": [],
#})

CFLAGS_CORESIGHT = [
    "-Wmissing-include-dirs",
    "-Wunused-but-set-variable",
    "-Wunused-const-variable",
    # "-Wstringop-truncation",
]

COPTS_KBASE = [
    "MALI_COVERAGE=0",
    "MALI_JIT_PRESSURE_LIMIT_BASE=0",
    "MALI_RELEASE_NAME=\"r54p1-11eac0\"",
] + select({
    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/a16w/config:mali_debug": ["MALI_UNIT_TEST=1"],
    "//conditions:default": ["MALI_UNIT_TEST=0"],
}) + select({
    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/a16w/config:mali_customer_release": ["MALI_CUSTOMER_RELEASE=1"],
    "//conditions:default": ["MALI_CUSTOMER_RELEASE=0"],
}) + select({
    "//vendor/mediatek/kernel_modules/gpu/gpu_mali/mali_avalon/a16w/config:mali_debug_kutf": ["MALI_KERNEL_TEST_API=1"],
    "//conditions:default": ["MALI_KERNEL_TEST_API=0"],
})

COPTS_MTK = [
    "-I$(srctree)/include",
    "-I$(srctree)/drivers/iommu",
    "-I$(srctree)/drivers/staging/android",
    "-I$(srctree)/drivers/staging/android/ion",
    "-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat",
    "-I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/"
]
