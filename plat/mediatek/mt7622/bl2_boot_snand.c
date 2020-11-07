/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/io/io_driver.h>
#include <lib/utils_def.h>
#include <bl2_boot_dev.h>
#include <mt7622_def.h>
#include <pinctrl.h>
#include <common/debug.h>
#include <plat/common/platform.h>

#include <nandx_util.h>
#include <nandx_core.h>

#include <nmbm/nmbm.h>

#define FIP_BASE			0x80000
#define FIP_SIZE			0x200000

#define NIF_CLK				50000000

#define NMBM_INST_MEM_ADDR		0x42000000

#ifndef NMBM_MAX_RATIO
#define NMBM_MAX_RATIO			1
#endif

#ifndef NMBM_MAX_RESERVED_BLOCKS
#define NMBM_MAX_RESERVED_BLOCKS	256
#endif

struct snfi_gpio_mux_info {
	const uint32_t *pins;
	uint32_t count;
	uint32_t mux;
};

static const uint32_t snfi_pins[] = { 8, 9, 10, 11, 12, 13 };

static const struct snfi_gpio_mux_info snfi_pinmux = {
	.pins = snfi_pins,
	.count = ARRAY_SIZE(snfi_pins),
	.mux = 2,
};

static struct nandx_info nandxi;
static struct nmbm_instance *ni;

static int nmbm_lower_read_page(void *arg, uint64_t addr, void *buf, void *oob,
				enum nmbm_oob_mode mode)
{
	int ret, val = 1;
	
	if (mode == NMBM_MODE_RAW)
		val = 0;

	nandx_ioctl(NFI_CTRL_ECC, &val);

	ret = nandx_read(buf, oob, addr, nandxi.page_size);

	if (ret == -ENANDFLIPS)
		return 1;

	return ret;
}

static int nmbm_lower_write_page(void *arg, uint64_t addr, const void *buf,
				 const void *oob, enum nmbm_oob_mode mode)
{
	int val = 1;
	
	if (mode == NMBM_MODE_RAW)
		val = 0;

	nandx_ioctl(NFI_CTRL_ECC, &val);

	return nandx_write((void *)buf, (void *)oob, addr, nandxi.page_size);
}

static int nmbm_lower_erase_block(void *arg, uint64_t addr)
{
	return nandx_erase(addr, nandxi.block_size);
}

static int nmbm_lower_is_bad_block(void *arg, uint64_t addr)
{
	return nandx_is_bad_block(addr);
}

static void nmbm_lower_log(void *arg, enum nmbm_log_category level,
			   const char *fmt, va_list ap)
{
	int log_level;
	const char *prefix_str;

	switch (level) {
	case NMBM_LOG_DEBUG:
		log_level = LOG_LEVEL_VERBOSE;
		break;
	case NMBM_LOG_WARN:
		log_level = LOG_LEVEL_WARNING;
		break;
	case NMBM_LOG_ERR:
	case NMBM_LOG_EMERG:
		log_level = LOG_LEVEL_ERROR;
		break;
	default:
		log_level = LOG_LEVEL_NOTICE;
	}

	if (log_level > LOG_LEVEL)
		return;

	prefix_str = plat_log_get_prefix(log_level);

	while (*prefix_str != '\0') {
		(void)putchar(*prefix_str);
		prefix_str++;
	}

	vprintf(fmt, ap);
}

static int nmbm_init(void)
{
	struct nmbm_lower_device nld;
	size_t ni_size;
	int ret;

	memset(&nld, 0, sizeof(nld));

	nld.flags = NMBM_F_CREATE;
	nld.max_ratio = NMBM_MAX_RATIO;
	nld.max_reserved_blocks = NMBM_MAX_RESERVED_BLOCKS;

	nld.size = nandxi.total_size;
	nld.erasesize = nandxi.block_size;
	nld.writesize = nandxi.page_size;
	nld.oobsize = nandxi.spare_size;
	nld.oobavail = nandxi.oob_size;

	nld.read_page = nmbm_lower_read_page;
	nld.write_page = nmbm_lower_write_page;
	nld.erase_block = nmbm_lower_erase_block;
	nld.is_bad_block = nmbm_lower_is_bad_block;

	nld.logprint = nmbm_lower_log;

	ni = (struct nmbm_instance *)NMBM_INST_MEM_ADDR;

	ni_size = nmbm_calc_structure_size(&nld);
	memset(ni, 0, ni_size);

	NOTICE("Initializing NMBM ...\n");

	ret = nmbm_attach(&nld, ni);
	if (ret) {
		ni = NULL;
		return ret;
	}

	return 0;
}

static size_t snand_read_range(int lba, uintptr_t buf, size_t size)
{
	size_t retlen = 0;

	nmbm_read_range(ni, lba * nandxi.page_size, size, (void *)buf,
			NMBM_MODE_PLACE_OOB, &retlen);

	return retlen;
}

static size_t snand_write_range(int lba, uintptr_t buf, size_t size)
{
	size_t retlen = 0;
	int ret;

	ret = nmbm_erase_block_range(ni, lba * nandxi.page_size, size, NULL);
	if (ret)
		return 0;

	nmbm_write_range(ni, lba * nandxi.page_size, size, (void *)buf,
			 NMBM_MODE_PLACE_OOB, &retlen);

	return retlen;
}

static io_block_dev_spec_t snand_dev_spec = {
	.buffer = {
		.offset = 0x41000000,
		.length = 0x1000000,
	},

	.ops = {
		.read = snand_read_range,
		.write = snand_write_range,
	},
};

const io_block_spec_t mtk_boot_dev_fip_spec = {
	.offset	= FIP_BASE,
	.length = FIP_SIZE,
};

static int mtk_snand_init(void)
{
	struct nfi_resource res = {
		.ic_ver = NANDX_MT7622,
		.ecc_regs = (void *)NFIECC_BASE,
		.nfi_regs = (void *)NFI_BASE,
		.clock_1x = NIF_CLK,
		.min_oob_req = 32,
	};
	int ret = 0, arg = 1;

	ret = nandx_init(&res);
	if (ret) {
		ERROR("Failed to initialize SNFI, error %d\n", ret);
		return ret;
	}

	nandx_ioctl(NFI_CTRL_DMA, &arg);
	nandx_ioctl(NFI_CTRL_ECC, &arg);
	nandx_ioctl(CORE_CTRL_NAND_INFO, &nandxi);

	snand_dev_spec.block_size = nandxi.page_size;

	return ret;
}

void mtk_boot_dev_setup(const io_dev_connector_t **boot_dev_con,
			uintptr_t *boot_dev_handle)
{
	int i, result;

	for (i = 0; i < snfi_pinmux.count; i++)
		mtk_set_pin_mode(snfi_pinmux.pins[i], snfi_pinmux.mux);

	result = mtk_snand_init();
	assert(result == 0);

	result = nmbm_init();
	assert(result == 0);

	result = register_io_dev_block(boot_dev_con);
	assert(result == 0);

	result = io_dev_open(*boot_dev_con, (uintptr_t)&snand_dev_spec,
			     boot_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}
