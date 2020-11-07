/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#include "nandx_util.h"
#include "nandx_core.h"
#include "nand_chip.h"
#include "nand_device.h"
#include "nfi.h"
#include "nand_base.h"

#if NANDX_GLOBAL_VARIBLE
u8 g_page_buf[4096];
struct nand_chip g_nand_chip;
#endif

static int nand_chip_read_page(struct nand_chip *chip,
			       struct nand_ops *ops,
			       int count)
{
	struct nand_base *nand = chip->nand;
	struct nand_device *dev = nand->dev;
	int i, ret = 0;
	int row, col, sectors;
	u8 *data, *oob;

	for (i = 0; i < count; i++) {
		row = ops[i].row;
		col = ops[i].col;

		nand->addressing(nand, &row, &col);
		ops[i].status = nand->read_page(nand, row);
		if (ops[i].status < 0) {
			ret = ops[i].status;
			continue;
		}

		data = ops[i].data;
		oob = ops[i].oob;
		sectors = ops[i].len / chip->sector_size;
		ops[i].status = nand->read_data(nand, row, col,
						sectors, data, oob);
		if (ops[i].status > 0)
			ops[i].status = ops[i].status >=
					dev->endurance->max_bitflips ?
					-ENANDFLIPS : 0;

		ret = min_t(int, ret, ops[i].status);
	}

	return ret;
}

static int nand_chip_write_page(struct nand_chip *chip,
				struct nand_ops *ops,
				int count)
{
	struct nand_base *nand = chip->nand;
	struct nand_device *dev = nand->dev;
	int i, ret = 0;
	int row, col;
	u8 *data, *oob;

	for (i = 0; i < count; i++) {
		row = ops[i].row;
		col = ops[i].col;

		nand->addressing(nand, &row, &col);

		ops[i].status = nand->write_enable(nand);
		if (ops[i].status) {
			pr_debug("Write Protect at %x!\n", row);
			ops[i].status = -ENANDWP;
			return -ENANDWP;
		}

		data = ops[i].data;
		oob = ops[i].oob;
		ops[i].status = nand->program_data(nand, row, col, data, oob);
		if (ops[i].status < 0) {
			ret = ops[i].status;
			continue;
		}

		ops[i].status = nand->program_page(nand, row);
		if (ops[i].status < 0) {
			ret = ops[i].status;
			continue;
		}

		ops[i].status = nand->read_status(nand);
		if (ops[i].status & dev->status->program_fail)
			ops[i].status = -ENANDWRITE;

		ret = min_t(int, ret, ops[i].status);
	}

	return ret;
}

static int nand_chip_erase_block(struct nand_chip *chip,
				 struct nand_ops *ops,
				 int count)
{
	struct nand_base *nand = chip->nand;
	struct nand_device *dev = nand->dev;
	int i, ret = 0;
	int row, col;

	for (i = 0; i < count; i++) {
		row = ops[i].row;
		col = ops[i].col;

		nand->addressing(nand, &row, &col);

		ops[i].status = nand->write_enable(nand);
		if (ops[i].status) {
			pr_debug("Write Protect at %x!\n", row);
			ops[i].status = -ENANDWP;
			return -ENANDWP;
		}

		ops[i].status = nand->erase_block(nand, row);
		if (ops[i].status < 0) {
			ret = ops[i].status;
			continue;
		}

		ops[i].status = nand->read_status(nand);
		if (ops[i].status & dev->status->erase_fail)
			ops[i].status = -ENANDERASE;

		ret = min_t(int, ret, ops[i].status);
	}

	return ret;
}

/* read first bad mark on spare */
static int nand_chip_is_bad_block(struct nand_chip *chip,
				  struct nand_ops *ops,
				  int count)
{
	int i, ret, value;
	int status = 0;
	u8 *data, *tmp_buf;

#if NANDX_GLOBAL_VARIBLE
	tmp_buf  = g_page_buf;
#else
	tmp_buf = mem_alloc(1, chip->page_size);
	if (!tmp_buf)
		return -ENOMEM;
#endif

	memset(tmp_buf, 0x00, chip->page_size);

	/* Disable ECC */
	value = 0;
	ret = chip->chip_ctrl(chip, NFI_CTRL_ECC, &value);
	if (ret)
		goto out;

	ret = chip->read_page(chip, ops, count);
	if (ret)
		goto out;

	for (i = 0; i < count; i++) {
		data = ops[i].data;

		/* temp solution for mt7622, because of no bad mark swap */
		if (!memcmp(data, tmp_buf, chip->page_size)) {
			ops[i].status = -ENANDBAD;
			status = -ENANDBAD;
		} else {
			ops[i].status = 0;
		}
	}

	/* Enable ECC */
	value = 1;
	ret = chip->chip_ctrl(chip, NFI_CTRL_ECC, &value);
	if (ret)
		goto out;

#if !NANDX_GLOBAL_VARIBLE
	mem_free(tmp_buf);
#endif
	return status;

out:
#if !NANDX_GLOBAL_VARIBLE
	mem_free(tmp_buf);
#endif
	return ret;
}

static int nand_chip_ctrl(struct nand_chip *chip, int cmd, void *args)
{
	return -EOPNOTSUPP;
}

static int nand_chip_suspend(struct nand_chip *chip)
{
	return 0;
}

static int nand_chip_resume(struct nand_chip *chip)
{
	return 0;
}

struct nand_chip *nand_chip_init(struct nfi_resource *res)
{
	struct nand_chip *chip;
	struct nand_base *nand;
	struct nfi *nfi;

#if NANDX_GLOBAL_VARIBLE
	chip  = &g_nand_chip;
#else
	chip = mem_alloc(1, sizeof(struct nand_chip));
	if (!chip) {
		pr_info("nand chip alloc fail!\n");
		return NULL;
	}
#endif

	nfi = nfi_init(res);
	if (!nfi) {
		pr_info("nfi init fail!\n");
		goto nfi_err;
	}

	nand = nand_base_init(NULL, nfi);
	if (!nand) {
		pr_info("nand base init fail!\n");
		goto base_err;
	}

	chip->nand = (void *)nand;
	chip->read_page = nand_chip_read_page;
	chip->write_page = nand_chip_write_page;
	chip->erase_block = nand_chip_erase_block;
	chip->is_bad_block = nand_chip_is_bad_block;
	chip->chip_ctrl = nand_chip_ctrl;
	chip->suspend = nand_chip_suspend;
	chip->resume = nand_chip_resume;

	nand = nand_device_init(chip);
	if (!nand)
		goto nand_err;

	chip->nand = (void *)nand;
	chip->plane_num = nand->dev->plane_num;
	chip->block_num = nand_total_blocks(nand->dev);
	chip->block_size = nand->dev->block_size;
	chip->block_pages = nand_block_pages(nand->dev);
	chip->page_size = nand->dev->page_size;
	chip->oob_size = nfi->fdm_size * div_down(chip->page_size,
						  nfi->sector_size);
	chip->spare_size = nand->dev->spare_size;
	chip->sector_size = nfi->sector_size;
	chip->sector_spare_size = nfi->sector_spare_size;
	chip->min_program_pages = nand->dev->min_program_pages;
	chip->ecc_strength = nfi->ecc_strength;
	chip->ecc_parity_size = nfi->ecc_parity_size;
	chip->fdm_ecc_size = nfi->fdm_ecc_size;
	chip->fdm_reg_size = nfi->fdm_size;

	return chip;

nand_err:
#if !NANDX_GLOBAL_VARIBLE
	mem_free(nand);
#endif
base_err:
	nfi_exit(nfi);
nfi_err:
#if !NANDX_GLOBAL_VARIBLE
	mem_free(chip);
#endif
	return NULL;
}

void nand_chip_exit(struct nand_chip *chip)
{
	nand_exit(chip->nand);
#if !NANDX_GLOBAL_VARIBLE
	mem_free(chip);
#endif
}
