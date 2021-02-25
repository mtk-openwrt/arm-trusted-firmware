/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/io/io_driver.h>
#include <lib/utils_def.h>
#include <bl2_boot_dev.h>
#include <mt7629_def.h>
#include <pinctrl.h>
#include <common/debug.h>
#include <plat/common/platform.h>
#include <lib/mmio.h>
#include <string.h>

#include <mtk-snand.h>
#include <mempool.h>

#define FIP_BASE			0x80000
#define FIP_SIZE			0x200000

struct snfi_gpio_mux_info {
	const uint32_t *pins;
	uint32_t count;
	uint32_t mux;
};

static const uint32_t snfi_pins[] = { 62, 63, 64, 65, 66, 67 };

static const struct snfi_gpio_mux_info snfi_pinmux = {
	.pins = snfi_pins,
	.count = ARRAY_SIZE(snfi_pins),
	.mux = 2,
};

static struct mtk_snand *snf;
static struct mtk_snand_chip_info cinfo;
static uint32_t oobavail;
static uint8_t *page_cache;

static size_t snand_read_range(int lba, uintptr_t buf, size_t size)
{
	size_t sizeremain = size, chunksize;
	uint64_t off = lba * cinfo.pagesize;
	uint8_t *ptr = (uint8_t *)buf;
	int ret = 0;

	if (!snf)
		return 0;

	while (sizeremain) {
		chunksize = cinfo.pagesize;
		if (chunksize > sizeremain)
			chunksize = sizeremain;

		ret = mtk_snand_read_page(snf, off, page_cache, NULL, false);
		if (ret < 0)
			break;

		memcpy(ptr, page_cache, chunksize);

		off += chunksize;
		ptr += chunksize;
		sizeremain -= chunksize;
	}

	return size - sizeremain;
}

static size_t snand_write_range(int lba, uintptr_t buf, size_t size)
{
	/* Do not use write in BL2 */
	return 0;
}

static io_block_dev_spec_t snand_dev_spec = {
	/* Buffer should not exceed BL33_BASE */
	.buffer = {
		.offset = 0x41000000,
		.length = 0xe00000,
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

static const struct mtk_snand_platdata mt7629_snand_pdata = {
	.nfi_base = (void *)NFI_BASE,
	.ecc_base = (void *)NFIECC_BASE,
	.soc = SNAND_SOC_MT7629,
	.quad_spi = true
};

static int mt7629_snand_init(void)
{
	int ret;

	ret = mtk_snand_init(NULL, &mt7629_snand_pdata, &snf);
	if (ret) {
		snf = NULL;
		return ret;
	}

	mtk_snand_get_chip_info(snf, &cinfo);
	oobavail = cinfo.num_sectors * (cinfo.fdm_size - 1);
	snand_dev_spec.block_size = cinfo.pagesize;
	page_cache = mtk_mem_pool_alloc(cinfo.pagesize + cinfo.sparesize);

	NOTICE("SPI-NAND: %s (%lluMB)\n", cinfo.model, cinfo.chipsize >> 20);

	return ret;
}

void mtk_boot_dev_setup(const io_dev_connector_t **boot_dev_con,
			uintptr_t *boot_dev_handle)
{
	int i, result;

	for (i = 0; i < snfi_pinmux.count; i++)
		mtk_set_pin_mode(snfi_pinmux.pins[i], snfi_pinmux.mux);

	result = mt7629_snand_init();
	assert(result == 0);

	result = register_io_dev_block(boot_dev_con);
	assert(result == 0);

	result = io_dev_open(*boot_dev_con, (uintptr_t)&snand_dev_spec,
			     boot_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}
