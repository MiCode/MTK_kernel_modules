# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2020 MediaTek Inc.

# CROSS_COMPILE = aarch64-linux-gnu-
# CFLAGS = -DFS_UT -Werror -Wall -Wframe-larger-than=512 --static
ASAN = -fsanitize=kernel-address,leak,undefined
CFLAGS = -DFS_UT -Werror -Wall $(ASAN) -Wframe-larger-than=512 -Walloc-zero -Walloca -Wnonnull
# CFLAGS = -DFS_UT -Werror -Wall -Wextra -Wconversion -Wno-unused-parameter $(ASAN) -Wframe-larger-than=512 -Walloc-zero -Walloca -Wnonnull
LDFLAGS = --static
LIBS = -static-libasan -lasan -lpthread -lm
$(info ASAN = $(ASAN))
$(info CFLAGS = $(CFLAGS))
$(info LDFLAGS = $(LDFLAGS))
$(info LIBS = $(LIBS))
$(info $(NEWLINE))


# TSREC path
ISP7S_CAM_LEGACY = ../../../../../../media/platform/mtk-isp/camsys/isp7s/cam_legacy
ISP7SP_CAM = ../../../../../../media/platform/mtk-isp/camsys/isp7sp/cam
MTKCAM_ISP7SP_CAM = ../../../../camsys/isp7sp/cam
## Please manually change/set TSREC file path for unit test file building
TSREC_DIR = $(MTKCAM_ISP7SP_CAM)
$(info TSREC_DIR = $(TSREC_DIR))
$(info $(NEWLINE))


INCS = -I ./  \
	   -I ../
#	   -I ../../../inc/

# $(info INCS = $(INCS))
TSREC_INCS = -I $(TSREC_DIR)/ \
#	   -I ../../../../../../misc/mediatek/imgsensor/src-v4l2/frame-sync/fs-ut-test/


SRCS = ut_fs_test.c \
	   ../frame_sync.c \
	   ../frame_sync_algo.c \
	   ../frame_monitor.c \
	   ../frame_sync_util.c \
	   ../sensor_recorder.c \
	   ../hw_sensor_sync_algo.c \
	   ../custom/custom_hw_sync.c \
	   ut_fs_tsrec.c \
	   $(TSREC_DIR)/mtk_cam-seninf-tsrec-regs.c \
	   $(TSREC_DIR)/mtk_cam-seninf-tsrec.c \

#OBJS = ut_fs_test.o \
#	   frame_sync.o \
#	   frame_sync_algo.o \
#	   frame_monitor.o \
#	   frame_sync_util.o \

TARGET = ut_fs_test

all: $(OPTS) $(TARGET)

debug: DEBUG_FLAGS = -g
debug: ut_fs_test

ut_fs_test: $(SRCS)
#	gcc $(LDFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(INCS) $^ -o $@ $(LIBS)
	gcc $(CFLAGS) $(DEBUG_FLAGS) $(INCS) $(TSREC_INCS) $^ -o $@ $(LIBS)

# ut_fs_test: $(OBJS)
#	gcc $(LDFLAGS) $(INCS) -o $(LIBS) $@ $^

%.o: %.c
	gcc $(CFLAGS) $(INCS) -c $<

%.o: ../%.c
	gcc $(CFLAGS) $(INCS) $(LIBS) -c $<

clean:
	rm -f *.o $(TARGET)
