#
# Copyright (C) 2017 MediaTek Inc.
# Licensed under either
#     BSD Licence, (see NOTICE for more details)
#     GNU General Public License, version 2.0, (see NOTICE for more details)
#

nandx_dir := $(shell dirname $(lastword $(MAKEFILE_LIST)))
include $(nandx_dir)/Nandx.config

ifeq ($(NANDX_SIMULATOR_SUPPORT), y)
sim-obj :=
sim-inc :=
nandx-obj := sim-obj
nandx-prefix := .
nandx-postfix := %.o
sim-inc += -I$(nandx-prefix)/include/internal
sim-inc += -I$(nandx-prefix)/include/simulator
endif

ifeq ($(NANDX_CTP_SUPPORT), y)
nandx-obj := C_SRC_FILES
nandx-prefix := $(nandx_dir)
nandx-postfix := %.c
INC_DIRS += $(nandx_dir)/include/internal
INC_DIRS += $(nandx_dir)/include/ctp
endif

ifeq ($(NANDX_DA_SUPPORT), y)
nandx-obj := obj-y
nandx-prefix := $(nandx_dir)
nandx-postfix := %.o
INCLUDE_PATH += $(TOPDIR)/platform/$(CODE_BASE)/dev/nand/nandx/include/internal
INCLUDE_PATH += $(TOPDIR)/platform/$(CODE_BASE)/dev/nand/nandx/include/da
endif

ifeq ($(NANDX_PRELOADER_SUPPORT), y)
nandx-obj := MOD_SRC
nandx-prefix := $(nandx_dir)
nandx-postfix := %.c
C_OPTION += -I$(MTK_PATH_PLATFORM)/src/drivers/nandx/include/internal
C_OPTION += -I$(MTK_PATH_PLATFORM)/src/drivers/nandx/include/preloader
endif

ifeq ($(NANDX_ATF_SUPPORT), y)
nandx-obj := BL2_SOURCES
nandx-prefix := $(nandx_dir)
nandx-postfix := %.c
PLAT_INCLUDES += -I${MTK_PLAT}/common/drivers/nandx/include/internal
PLAT_INCLUDES += -I${MTK_PLAT}/common/drivers/nandx/include/atf
endif

ifeq ($(NANDX_LK_SUPPORT), y)
nandx-obj := MODULE_SRCS
nandx-prefix := $(nandx_dir)
nandx-postfix := %.c
GLOBAL_INCLUDES += $(nandx_dir)/include/internal
GLOBAL_INCLUDES += $(nandx_dir)/include/lk
endif

ifeq ($(NANDX_KERNEL_SUPPORT), y)
nandx-obj := obj-y
nandx-prefix := nandx
nandx-postfix := %.o
ccflags-y += -I$(nandx_dir)/include/internal
ccflags-y += -I$(nandx_dir)/include/kernel
endif

ifeq ($(NANDX_UBOOT_SUPPORT), y)
nandx-obj := obj-y
nandx-prefix := nandx
nandx-postfix := %.o
ccflags-y += -I$(nandx_dir)/include/internal
ccflags-y += -I$(nandx_dir)/include/uboot
endif

nandx-y :=
include $(nandx_dir)/core/Nandx.mk
nandx-target := $(nandx-prefix)/core/$(nandx-postfix)
$(nandx-obj) += $(patsubst %.c, $(nandx-target), $(nandx-y))
$(warning $(BL2_SOURCES))
$(warning $(nandx-y))

nandx-y :=
$(warning $(nandx-y))
$(warning $(BL2_SOURCES))
include $(nandx_dir)/driver/Nandx.mk
nandx-target := $(nandx-prefix)/driver/$(nandx-postfix)
$(nandx-obj) += $(patsubst %.c, $(nandx-target), $(nandx-y))
$(warning $(BL2_SOURCES))

ifeq ($(NANDX_SIMULATOR_SUPPORT), y)
cc := gcc
CFLAGS += $(sim-inc)

.PHONY:nandx
nandx: $(sim-obj)
	$(cc)  $(sim-obj) -o nandx

.PHONY:clean
clean:
	rm -rf $(sim-obj) nandx
endif
