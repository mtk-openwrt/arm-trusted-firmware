#
# Copyright (c) 2015-2019, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

MTK_PLAT		:=	plat/mediatek
MTK_PLAT_SOC		:=	${MTK_PLAT}/${PLAT}

PLAT_INCLUDES		:=	-I${MTK_PLAT}/common/				\
				-I${MTK_PLAT}/common/drivers/uart		\
				-Iinclude/plat/arm/common			\
				-Iinclude/plat/arm/common/aarch64		\
				-I${MTK_PLAT_SOC}/drivers/pinctrl/		\
				-I${MTK_PLAT_SOC}/drivers/pmic/			\
				-I${MTK_PLAT_SOC}/drivers/pll/			\
				-I${MTK_PLAT_SOC}/drivers/rtc/			\
				-I${MTK_PLAT_SOC}/drivers/spm/			\
				-I${MTK_PLAT_SOC}/drivers/timer/		\
				-I${MTK_PLAT_SOC}/include/

PLAT_BL_COMMON_SOURCES	:=	lib/xlat_tables/xlat_tables_common.c		\
				lib/xlat_tables/aarch64/xlat_tables.c		\
				plat/arm/common/arm_gicv2.c			\
				plat/common/plat_gicv2.c

BL2_SOURCES		:=	common/desc_image_load.c			\
				drivers/arm/gic/common/gic_common.c		\
				drivers/arm/gic/v2/gicv2_main.c			\
				drivers/arm/gic/v2/gicv2_helpers.c		\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				drivers/gpio/gpio.c				\
				drivers/io/io_storage.c				\
				drivers/io/io_block.c				\
				drivers/io/io_fip.c				\
				lib/cpus/aarch64/cortex_a53.S			\
				${MTK_PLAT}/common/drivers/uart/aarch64/hsuart.S	\
				${MTK_PLAT}/common/mtk_plat_common.c		\
				${MTK_PLAT_SOC}/aarch64/plat_helpers.S		\
				${MTK_PLAT_SOC}/aarch64/platform_common.c	\
				${MTK_PLAT_SOC}/bl2_plat_setup.c		\
				${MTK_PLAT_SOC}/drivers/pinctrl/pinctrl.c	\
				${MTK_PLAT_SOC}/drivers/pll/pll.c		\
				${MTK_PLAT_SOC}/drivers/pmic/pmic_wrap.c	\
				${MTK_PLAT_SOC}/drivers/pmic/pmic.c		\
				${MTK_PLAT_SOC}/drivers/spm/mtcmos.c		\
				${MTK_PLAT_SOC}/drivers/timer/cpuxgpt.c

ifndef BOOT_DEVICE
$(error You must specify the boot device by provide BOOT_DEVICE= to \
	make parameter. Avaliable values: nor emmc sdmmc snand)
else
ifeq ($(BOOT_DEVICE),ram)
BL2_SOURCES		+=	drivers/io/io_memmap.c				\
				${MTK_PLAT_SOC}/bl2_boot_ram.c
BROM_HEADER_TYPE	:=	nor
ifeq ($(RAM_BOOT_DEBUGGER_HOOK), 1)
CPPFLAGS		+=	-DRAM_BOOT_DEBUGGER_HOOK
endif
endif
ifeq ($(BOOT_DEVICE),nor)
BL2_SOURCES		+=	drivers/io/io_memmap.c				\
				${MTK_PLAT_SOC}/bl2_boot_nor.c
BROM_HEADER_TYPE	:=	nor
endif
ifeq ($(BOOT_DEVICE),emmc)
BL2_SOURCES		+=	drivers/mmc/mmc.c				\
				drivers/partition/partition.c			\
				drivers/partition/gpt.c				\
				${MTK_PLAT}/common/drivers/mmc/mtk-sd.c		\
				${MTK_PLAT_SOC}/bl2_boot_mmc.c
BROM_HEADER_TYPE	:=	emmc
CPPFLAGS		+=	-DMSDC_INDEX=0
endif
ifeq ($(BOOT_DEVICE),sdmmc)
BL2_SOURCES		+=	drivers/mmc/mmc.c				\
				drivers/partition/partition.c			\
				drivers/partition/gpt.c				\
				${MTK_PLAT}/common/drivers/mmc/mtk-sd.c		\
				${MTK_PLAT_SOC}/bl2_boot_mmc.c
BROM_HEADER_TYPE	:=	sdmmc
CPPFLAGS		+=	-DMSDC_INDEX=1
DEVICE_HEADER_OFFSET	?=	0x80000
endif
ifeq ($(BOOT_DEVICE),snand)
include ${MTK_PLAT}/common/drivers/snfi/mtk-snand.mk
BROM_HEADER_TYPE	:=	snand
NAND_TYPE		?=	2k+64
BL2_SOURCES		+=	${MTK_SNAND_SOURCES}				\
				${MTK_PLAT_SOC}/bl2_boot_snand.c
PLAT_INCLUDES		+=	-I${MTK_PLAT}/common/drivers/snfi
CPPFLAGS		+=	-DMTK_MEM_POOL_BASE=0x40100000			\
				-DPRIVATE_MTK_SNAND_HEADER
endif
ifeq ($(BROM_HEADER_TYPE),)
$(error BOOT_DEVICE has invalid value. Please re-check.)
endif
endif

BL31_SOURCES		+=	drivers/arm/cci/cci.c				\
				drivers/arm/gic/common/gic_common.c		\
				drivers/arm/gic/v2/gicv2_main.c			\
				drivers/arm/gic/v2/gicv2_helpers.c		\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				lib/cpus/aarch64/aem_generic.S			\
				lib/cpus/aarch64/cortex_a53.S			\
				${MTK_PLAT}/common/drivers/uart/aarch64/hsuart.S	\
				${MTK_PLAT}/common/mtk_plat_common.c		\
				${MTK_PLAT}/common/mtk_sip_svc.c		\
				${MTK_PLAT_SOC}/aarch64/plat_helpers.S		\
				${MTK_PLAT_SOC}/aarch64/platform_common.c	\
				${MTK_PLAT_SOC}/bl31_plat_setup.c		\
				${MTK_PLAT_SOC}/drivers/pmic/pmic_wrap.c	\
				${MTK_PLAT_SOC}/drivers/rtc/rtc.c		\
				${MTK_PLAT_SOC}/drivers/spm/mtcmos.c		\
				${MTK_PLAT_SOC}/drivers/spm/spm.c		\
				${MTK_PLAT_SOC}/drivers/spm/spm_hotplug.c	\
				${MTK_PLAT_SOC}/drivers/spm/spm_mcdi.c		\
				${MTK_PLAT_SOC}/drivers/spm/spm_suspend.c	\
				${MTK_PLAT_SOC}/drivers/timer/cpuxgpt.c		\
				${MTK_PLAT_SOC}/plat_pm.c			\
				${MTK_PLAT_SOC}/plat_sip_calls.c		\
				${MTK_PLAT_SOC}/plat_topology.c			\
				${MTK_PLAT_SOC}/plat_mt_gic.c			\
				${MTK_PLAT_SOC}/power_tracer.c			\
				${MTK_PLAT_SOC}/scu.c

BL2_BASE		:=	0x201000
CPPFLAGS		+=	-DBL2_BASE=$(BL2_BASE)
CPPFLAGS		+=	-DKERNEL_IS_DEFAULT_64BIT

DOIMAGEPATH		:=	tools/mediatek/bromimage
DOIMAGETOOL		:=	${DOIMAGEPATH}/bromimage

# Enable workarounds for selected Cortex-A53 erratas.
ERRATA_A53_826319	:=	1
ERRATA_A53_836870	:=	1
ERRATA_A53_855873	:=	1

# indicate the reset vector address can be programmed
PROGRAMMABLE_RESET_ADDRESS	:=	1

$(eval $(call add_define,MTK_SIP_SET_AUTHORIZED_SECURE_REG_ENABLE))

# Do not enable SVE
ENABLE_SVE_FOR_NS		:=	0
MULTI_CONSOLE_API		:=	1

BL2_AT_EL3			:=	1

ifneq (${TRUSTED_BOARD_BOOT},0)

include drivers/auth/mbedtls/mbedtls_crypto.mk
include drivers/auth/mbedtls/mbedtls_x509.mk
ifeq ($(MBEDTLS_DIR),)
$(error You must specify MBEDTLS_DIR when TRUSTED_BOARD_BOOT enabled)
endif

AUTH_SOURCES		:=	drivers/auth/auth_mod.c			\
				drivers/auth/crypto_mod.c		\
				drivers/auth/img_parser_mod.c		\
				drivers/auth/tbbr/tbbr_cot_bl2.c	\
				drivers/auth/tbbr/tbbr_cot_common.c	\
				$(MBEDTLS_DIR)/library/md_wrap.c

BL2_SOURCES		+=	${AUTH_SOURCES}				\
				plat/common/tbbr/plat_tbbr.c		\
				${MTK_PLAT_SOC}/mtk_tbbr.c		\
				${MTK_PLAT_SOC}/mtk_rotpk.S

ROT_KEY			=	$(BUILD_PLAT)/rot_key.pem
ROTPK_HASH		=	$(BUILD_PLAT)/rotpk_sha256.bin

$(eval $(call add_define_val,ROTPK_HASH,'"$(ROTPK_HASH)"'))
$(BUILD_PLAT)/bl1/mtk_rotpk.o: $(ROTPK_HASH)
$(BUILD_PLAT)/bl2/mtk_rotpk.o: $(ROTPK_HASH)

certificates: $(ROT_KEY)
$(ROT_KEY): | $(BUILD_PLAT)
	@echo "  OPENSSL $@"
	$(Q)openssl genrsa 2048 > $@ 2>/dev/null

$(ROTPK_HASH): $(ROT_KEY)
	@echo "  OPENSSL $@"
	$(Q)openssl rsa -in $< -pubout -outform DER 2>/dev/null |\
	openssl dgst -sha256 -binary > $@ 2>/dev/null
endif

all: $(BUILD_PLAT)/bl2.img

ifneq ($(USE_MKIMAGE),1)
ifneq ($(BROM_SIGN_KEY),)
$(BUILD_PLAT)/bl2.img: $(BROM_SIGN_KEY)
endif

$(BUILD_PLAT)/bl2.img: $(BUILD_PLAT)/bl2.bin $(DOIMAGETOOL)
	-$(Q)rm -f $@.signkeyhash
	$(Q)$(DOIMAGETOOL) -f $(BROM_HEADER_TYPE) -a $(BL2_BASE) -d -e		\
		$(if $(BROM_SIGN_KEY), -s sha256+rsa-m1 -k $(BROM_SIGN_KEY))	\
		$(if $(BROM_SIGN_KEY), -p $@.signkeyhash)			\
		$(if $(DEVICE_HEADER_OFFSET), -o $(DEVICE_HEADER_OFFSET))	\
		$(if $(ANTI_ROLLBACK_VER), -r $(ANTI_ROLLBACK_VER))		\
		$(if $(NAND_TYPE), -n $(NAND_TYPE))				\
		$(BUILD_PLAT)/bl2.bin $@
else
MKIMAGE ?= mkimage

ifneq ($(BROM_SIGN_KEY)$(BL2_AR_VER),)
$(warning BL2 signing/anti-rollback is not supported using mkimage)
endif

$(BUILD_PLAT)/bl2.img: $(BUILD_PLAT)/bl2.bin
	$(Q)$(MKIMAGE) -T mtk_image -a $(BL2_BASE) -e $(BL2_BASE)		\
		-n "arm64=1;media=$(BROM_HEADER_TYPE)$(if $(NAND_TYPE),;nandinfo=$(NAND_TYPE))$(if $(DEVICE_HEADER_OFFSET),;hdroffset=$(DEVICE_HEADER_OFFSET))"	\
		-d $(BUILD_PLAT)/bl2.bin $@
endif

.PHONY: $(BUILD_PLAT)/bl2.img

distclean realclean clean: cleandoimage

cleandoimage:
	$(Q)$(MAKE) --no-print-directory -C $(DOIMAGEPATH) clean

$(DOIMAGETOOL):
	$(Q)$(MAKE) --no-print-directory -C $(DOIMAGEPATH)
