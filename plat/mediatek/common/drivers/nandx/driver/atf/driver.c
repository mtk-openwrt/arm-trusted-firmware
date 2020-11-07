#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_block.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include <bl2_boot_dev.h>

#include "nandx_util.h"
#include "nandx_core.h"

#define NFI_BASE 0x1100d000
#define NFIECC_BASE 0x1100e000

typedef int (*func_nandx_operation)(u8 *, u8 *, u64, size_t);
struct nandx_info nandxi;

u32 nand_erase(int offset, size_t len)
{
	u32 block_size = nandxi.block_size;
	u32 bytes_erase = 0;
	int ret;

	while (len) {
		ret = nandx_erase(offset, block_size);
		if (ret < 0)
			pr_warn("erase fail at blk %d\n", offset);
		else
			bytes_erase += block_size;

		offset += block_size;
		len -= block_size;
	}

	pr_debug("%s end, bytes_erase:0x%x\n", __func__, bytes_erase);

	return bytes_erase;
}

static size_t rw_operation(void *buf, int lba, size_t len, bool read)
{
	struct nandx_split64 split = {0};
	func_nandx_operation operation;
	int i, pages, offset;
	u8 *lbuf = (u8 *)buf;
	u64 val;

	operation = read ? nandx_read : nandx_write;
	offset = lba * nandxi.page_size;

	nandx_split(&split, offset, len, val, nandxi.page_size);

	if (split.head_len) {
		operation(lbuf, NULL, split.head, split.head_len);
		lbuf += split.head_len;
	}

	if (split.body_len) {
		pages = div_down(split.body_len, nandxi.page_size);
		for (i = 0; i < pages; i++) {
			operation(lbuf + i * nandxi.page_size , NULL,
				  split.body + i * nandxi.page_size,
				  nandxi.page_size);
		}

		lbuf += split.body_len;
	}

	if (split.tail_len) {
		operation(lbuf, NULL, split.tail, split.tail_len);
	}

	return len;
}

size_t snand_read_blocks(int lba, uintptr_t buf, size_t size)
{
	return rw_operation((void *)buf, lba, size, true);
}

size_t snand_write_blocks(int lba, uintptr_t buf, size_t size)
{
	return rw_operation((void *)buf, lba, size, false);
}

int nand_is_bad_block(u32 ofs)
{
	int ret;

	ret = nandx_is_bad_block(ofs);
	if (ret) {
		pr_info("nand block 0x%x is bad, ret %d!\n", ofs, ret);
		return 1;
	} else {
		return 0;
	}
}

static void nand_gpio_init(void)
{
#define GPIO_BASE    0x10211000
#define GPIO_MODE(x) (GPIO_BASE + 0x300 + 0x10 * x)
#define GPIO_DRIV(x) (GPIO_BASE + 0x900 + 0x10 * x)

	nandx_set_bits32(GPIO_MODE(0), 0xf00, 0x200);
	nandx_set_bits32(GPIO_MODE(5), 0xff00000, 0x2200000);

	nandx_set_bits32(GPIO_DRIV(6), 0x3f00, 0x0);
	nandx_set_bits32(GPIO_DRIV(7), 0x3f00, 0x3f00);
}

static u32 nand_clock_init(void)
{
#define SYS_CLK_CFG_1		0x10210050

	nandx_set_bits32(SYS_CLK_CFG_1, 0xf0000, 0x80000);

	return 26* 1000 * 1000;
}

int mtk_snand_init(unsigned int *blk_size)
{
	struct nfi_resource res = {
			NANDX_MT7622, NULL,
			(void *)NFIECC_BASE, 0,
			(void *)NFI_BASE, 0,
			26000000, NULL, 0, 32
	};
	int ret = 0, arg = 1;

	pr_debug("%s @ %d ...\n", __func__, __LINE__);

	nand_gpio_init();

	res.clock_1x = nand_clock_init();

	ret = nandx_init(&res);
	if (ret) {
		pr_err("nandx init error (%d)!\n", ret);
		return ret;
	}

#ifdef NANDX_UNIT_TEST
	ret = nandx_unit_test(0xc80000, 2048);
	if (ret) {
		pr_err("nand unit test fail (%d)!\n", ret);
		return ret;
	}
#endif

	nandx_ioctl(NFI_CTRL_DMA, &arg);
	nandx_ioctl(NFI_CTRL_ECC, &arg);
	nandx_ioctl(CORE_CTRL_NAND_INFO, &nandxi);
	*blk_size = nandxi.page_size;

	return ret;
}

static io_block_dev_spec_t snand_dev_spec = {
	.buffer = {
		.offset = 0x41000000,
		.length = 0x1000000,
	},

	.ops = {
		.read = snand_read_blocks,
		.write = snand_write_blocks,
	},
};

const io_block_spec_t mtk_boot_dev_fip_spec = {
	.offset	= 0x80000,
	.length = 0x200000,
};

void mtk_boot_dev_setup(const io_dev_connector_t **boot_dev_con,
			uintptr_t *boot_dev_handle)
{
	u32 blk_size = 0;
	int result;

	result = mtk_snand_init(&blk_size);
	assert(result == 0);

	snand_dev_spec.block_size = nandxi.page_size;

	result = register_io_dev_block(boot_dev_con);
	assert(result == 0);

	result = io_dev_open(*boot_dev_con, (uintptr_t)&snand_dev_spec,
			     boot_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}

