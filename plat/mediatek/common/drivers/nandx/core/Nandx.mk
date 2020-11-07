#
# Copyright (C) 2017 MediaTek Inc.
# Licensed under either
#     BSD Licence, (see NOTICE for more details)
#     GNU General Public License, version 2.0, (see NOTICE for more details)
#

nandx-y += nand_device.c
nandx-y += nand_base.c
nandx-y += nand_chip.c
nandx-y += core_io.c

nandx-header-y += nand_device.h
nandx-header-y += nand_base.h
nandx-header-y += nand_chip.h
nandx-header-y += core_io.h
nandx-header-y += nfi.h

nandx-$(NANDX_NAND_SPI) += nand/device_spi.c
nandx-$(NANDX_NAND_SPI) += nand/nand_spi.c
nandx-$(NANDX_NAND_SLC) += nand/device_slc.c
nandx-$(NANDX_NAND_SLC) += nand/nand_slc.c

nandx-header-$(NANDX_NAND_SPI) += nand/device_spi.h
nandx-header-$(NANDX_NAND_SPI) += nand/nand_spi.h
nandx-header-$(NANDX_NAND_SLC) += nand/device_slc.h
nandx-header-$(NANDX_NAND_SLC) += nand/nand_slc.h

nandx-$(NANDX_NFI_BASE) += nfi/nfi_base.c
nandx-$(NANDX_NFI_ECC) += nfi/nfiecc.c
nandx-$(NANDX_NFI_SPI) += nfi/nfi_spi.c

nandx-header-$(NANDX_NFI_BASE) += nfi/nfi_base.h
nandx-header-$(NANDX_NFI_BASE) += nfi/nfi_regs.h
nandx-header-$(NANDX_NFI_ECC) += nfi/nfiecc.h
nandx-header-$(NANDX_NFI_ECC) += nfi/nfiecc_regs.h
nandx-header-$(NANDX_NFI_SPI) += nfi/nfi_spi.h
nandx-header-$(NANDX_NFI_SPI) += nfi/nfi_spi_regs.h
