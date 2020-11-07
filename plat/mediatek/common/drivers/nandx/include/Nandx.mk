#
# Copyright (C) 2017 MediaTek Inc.
# Licensed under either
#     BSD Licence, (see NOTICE for more details)
#     GNU General Public License, version 2.0, (see NOTICE for more details)
#

nandx-header-y += internal/nandx_core.h
nandx-header-y += internal/nandx_errno.h
nandx-header-y += internal/nandx_util.h
nandx-header-$(NANDX_BBT_SUPPORT) += internal/bbt.h
nandx-header-$(NANDX_SIMULATOR_SUPPORT) += simulator/nandx_os.h
nandx-header-$(NANDX_CTP_SUPPORT) += ctp/nandx_os.h
nandx-header-$(NANDX_LK_SUPPORT) += lk/nandx_os.h
nandx-header-$(NANDX_KERNEL_SUPPORT) += kernel/nandx_os.h
nandx-header-$(NANDX_UBOOT_SUPPORT) += uboot/nandx_os.h
