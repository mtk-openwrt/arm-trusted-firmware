#
# Copyright (C) 2017 MediaTek Inc.
# Licensed under either
#     BSD Licence, (see NOTICE for more details)
#     GNU General Public License, version 2.0, (see NOTICE for more details)
#

nandx-$(NANDX_SIMULATOR_SUPPORT) += simulator/driver.c

nandx-$(NANDX_CTP_SUPPORT) += ctp/ts_nand.c
nandx-$(NANDX_CTP_SUPPORT) += ctp/nand_test.c
nandx-header-$(NANDX_CTP_SUPPORT) += ctp/nand_test.h

nandx-$(NANDX_BBT_SUPPORT) += bbt/bbt.c
nandx-$(NANDX_BROM_SUPPORT) += brom/driver.c
nandx-$(NANDX_KERNEL_SUPPORT) += kernel/driver.c
nandx-$(NANDX_LK_SUPPORT) += lk/driver.c
nandx-$(NANDX_UBOOT_SUPPORT) += uboot/driver.c

ifneq ($(NMBM),1)
nandx-$(NANDX_ATF_SUPPORT) += atf/driver.c
endif
