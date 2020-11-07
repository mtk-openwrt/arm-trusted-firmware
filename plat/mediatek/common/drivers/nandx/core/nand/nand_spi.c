/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#include "nandx_util.h"
#include "nandx_core.h"
#include "../nand_chip.h"
#include "../nand_device.h"
#include "../nfi.h"
#include "../nand_base.h"
#include "device_spi.h"
#include "nand_spi.h"

#define READY_TIMEOUT   500000 /* us */

#if NANDX_GLOBAL_VARIBLE
struct nand_spi g_nand_spi;
#endif

static int nand_spi_read_status(struct nand_base *nand)
{
	struct device_spi *dev = device_to_spi(nand->dev);
	u8 status;

	nand->get_feature(nand, dev->feature.status.addr, &status, 1);

	return status;
}

static int nand_spi_wait_ready(struct nand_base *nand, u32 timeout)
{
	int status;

	do {
		status = nand_spi_read_status(nand);
		status &= nand->dev->status->array_busy;
		timeout--;

		if (timeout == 0)
			break;
	} while (status);

	return status ? -EBUSY : 0;
}

static int nand_spi_set_op_mode(struct nand_base *nand, u8 mode)
{
	struct nand_spi *spi_nand = base_to_spi(nand);
	struct nfi *nfi = nand->nfi;
	int ret = 0;

	if (spi_nand->op_mode != mode) {
		ret = nfi->nfi_ctrl(nfi, SNFI_CTRL_OP_MODE, (void *)&mode);
		spi_nand->op_mode = mode;
	}

	return ret;
}

static int nand_spi_set_config(struct nand_base *nand, u8 addr, u8 mask,
			       bool en)
{
	u8 configs = 0;

	nand->get_feature(nand, addr, &configs, 1);

	if (en)
		configs |= mask;
	else
		configs &= ~mask;

	nand->set_feature(nand, addr, &configs, 1);

	configs = 0;
	nand->get_feature(nand, addr, &configs, 1);

	return (configs & mask) == en ? 0 : -EFAULT;
}

static int nand_spi_die_select(struct nand_base *nand, int *row)
{
	struct device_spi *dev = device_to_spi(nand->dev);
	struct nfi *nfi = nand->nfi;
	int lun_blocks, block_pages, lun, blocks;
	int page = *row, ret = 0;
	u8 param = 0, die_sel;

	if (nand->dev->lun_num < 2)
		return 0;

	block_pages = nand_block_pages(nand->dev);
	lun_blocks = nand_lun_blocks(nand->dev);
	blocks = div_down(page, block_pages);
	lun = div_down(blocks, lun_blocks);

	if (dev->extend_cmds->die_select == -1) {
		die_sel = (u8)(lun << dev->feature.character.die_sel_bit);
		nand->get_feature(nand, dev->feature.character.addr, &param, 1);
		param |= die_sel;
		nand->set_feature(nand, dev->feature.character.addr, &param, 1);
		param = 0;
		nand->get_feature(nand, dev->feature.character.addr, &param, 1);
		ret = (param & die_sel) ? 0 : -EFAULT;
	} else {
		nfi->reset(nfi);
		nfi->send_cmd(nfi, dev->extend_cmds->die_select);
		nfi->send_addr(nfi, lun, 0, 1, 0);
		nfi->trigger(nfi);
	}

	*row =  page - (lun_blocks * block_pages) * lun;

	return ret;
}

static int nand_spi_select_device(struct nand_base *nand, int cs)
{
	struct nand_spi *spi = base_to_spi(nand);
	struct nand_base *parent = spi->parent;

	nand_spi_set_op_mode(nand, SNFI_MAC_MODE);

	return parent->select_device(nand, cs);
}

static int nand_spi_reset(struct nand_base *nand)
{
	struct nand_spi *spi = base_to_spi(nand);
	struct nand_base *parent = spi->parent;

	nand_spi_set_op_mode(nand, SNFI_MAC_MODE);

	parent->reset(nand);

	return nand_spi_wait_ready(nand, READY_TIMEOUT);
}

static int nand_spi_read_id(struct nand_base *nand, u8 *id, int count)
{
	struct nand_spi *spi = base_to_spi(nand);
	struct nand_base *parent = spi->parent;

	nand_spi_set_op_mode(nand, SNFI_MAC_MODE);

	return parent->read_id(nand, id, count);
}

static int nand_spi_read_param_page(struct nand_base *nand, u8 *data,
				    int count)
{
	struct device_spi *dev = device_to_spi(nand->dev);
	struct nand_spi *spi = base_to_spi(nand);
	struct nfi *nfi = nand->nfi;
	int sectors, value;
	u8 param = 0;

	sectors = div_round_up(count, nfi->sector_size);

	nand->get_feature(nand, dev->feature.config.addr, &param, 1);
	param |= BIT(dev->feature.config.otp_en_bit);
	nand->set_feature(nand, dev->feature.config.addr, &param, 1);

	param = 0;
	nand->get_feature(nand, dev->feature.config.addr, &param, 1);
	if (param & BIT(dev->feature.config.otp_en_bit)) {
		value = 0;
		nfi->nfi_ctrl(nfi, NFI_CTRL_ECC, &value);
		nand->dev->col_cycle  = spi_replace_rx_col_cycle(spi->rx_mode);
		nand->read_page(nand, 0x01);
		nand->read_data(nand, 0x01, 0, sectors, data, NULL);
	}

	param &= ~BIT(dev->feature.config.otp_en_bit);
	nand->set_feature(nand, dev->feature.config.addr, &param, 1);

	return 0;
}

static int nand_spi_set_feature(struct nand_base *nand, u8 addr,
				u8 *param,
				int count)
{
	struct nand_spi *spi = base_to_spi(nand);
	struct nand_base *parent = spi->parent;

	nand->write_enable(nand);

	nand_spi_set_op_mode(nand, SNFI_MAC_MODE);

	return parent->set_feature(nand, addr, param, count);
}

static int nand_spi_get_feature(struct nand_base *nand, u8 addr,
				u8 *param,
				int count)
{
	struct nand_spi *spi = base_to_spi(nand);
	struct nand_base *parent = spi->parent;

	nand_spi_set_op_mode(nand, SNFI_MAC_MODE);

	return parent->get_feature(nand, addr, param, count);
}

static int nand_spi_addressing(struct nand_base *nand, int *row,
			       int *col)
{
	struct nand_device *dev = nand->dev;
	int plane, block, block_pages;
	int ret;

	ret = nand_spi_die_select(nand, row);
	if (ret)
		return ret;

	block_pages = nand_block_pages(dev);
	block = div_down(*row, block_pages);

	plane = block % dev->plane_num;
	*col |= (plane << dev->addressing->plane_bit_start);

	return 0;
}

static int nand_spi_read_page(struct nand_base *nand, int row)
{
	struct nand_spi *spi = base_to_spi(nand);
	struct nand_base *parent = spi->parent;

	if (spi->op_mode == SNFI_AUTO_MODE)
		nand_spi_set_op_mode(nand, SNFI_AUTO_MODE);
	else
		nand_spi_set_op_mode(nand, SNFI_MAC_MODE);

	parent->read_page(nand, row);

	return nand_spi_wait_ready(nand, READY_TIMEOUT);
}

static int nand_spi_read_data(struct nand_base *nand, int row, int col,
			      int sectors, u8 *data, u8 *oob)
{
	struct device_spi *dev = device_to_spi(nand->dev);
	struct nand_spi *spi = base_to_spi(nand);
	struct nand_base *parent = spi->parent;
	int ret;

	if ((spi->rx_mode == SNFI_RX_114 || spi->rx_mode == SNFI_RX_144) &&
	    dev->feature.config.need_qe)
		nand_spi_set_config(nand, dev->feature.config.addr,
				    BIT(0), true);

	nand->dev->col_cycle  = spi_replace_rx_col_cycle(spi->rx_mode);

	nand_spi_set_op_mode(nand, SNFI_CUSTOM_MODE);

	ret = parent->read_data(nand, row, col, sectors, data, oob);
	if (ret)
		return -ENANDREAD;

	if (spi->ondie_ecc) {
		ret = nand_spi_read_status(nand);
		ret &= GENMASK(dev->feature.status.ecc_end_bit,
			       dev->feature.status.ecc_start_bit);
		ret >>= dev->feature.status.ecc_start_bit;
		if (ret > nand->dev->endurance->ecc_req)
			return -ENANDREAD;
		else if (ret > nand->dev->endurance->max_bitflips)
			return -ENANDFLIPS;
	}

	return 0;
}

static int nand_spi_write_enable(struct nand_base *nand)
{
	struct device_spi *dev = device_to_spi(nand->dev);
	struct nfi *nfi = nand->nfi;
	int status;

	nand_spi_set_op_mode(nand, SNFI_MAC_MODE);

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->extend_cmds->write_enable);

	nfi->trigger(nfi);

	status = nand_spi_read_status(nand);
	status &= nand->dev->status->write_protect;

	return !status;
}

static int nand_spi_program_data(struct nand_base *nand, int row,
				 int col,
				 u8 *data, u8 *oob)
{
	struct device_spi *dev = device_to_spi(nand->dev);
	struct nand_spi *spi = base_to_spi(nand);

	if (spi->tx_mode == SNFI_TX_114 && dev->feature.config.need_qe)
		nand_spi_set_config(nand, dev->feature.config.addr,
				    BIT(0), true);

	nand_spi_set_op_mode(nand, SNFI_CUSTOM_MODE);

	nand->dev->col_cycle  = spi_replace_tx_col_cycle(spi->tx_mode);

	return spi->parent->program_data(nand, row, col, data, oob);
}

static int nand_spi_program_page(struct nand_base *nand, int row)
{
	struct nand_spi *spi = base_to_spi(nand);
	struct nand_device *dev = nand->dev;
	struct nfi *nfi = nand->nfi;

	if (spi->op_mode == SNFI_AUTO_MODE)
		nand_spi_set_op_mode(nand, SNFI_AUTO_MODE);
	else
		nand_spi_set_op_mode(nand, SNFI_MAC_MODE);

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->program_2nd);
	nfi->send_addr(nfi, 0, row, dev->col_cycle, dev->row_cycle);
	nfi->trigger(nfi);

	return nand_spi_wait_ready(nand, READY_TIMEOUT);
}

static int nand_spi_erase_block(struct nand_base *nand, int row)
{
	struct nand_spi *spi = base_to_spi(nand);
	struct nand_base *parent = spi->parent;

	nand_spi_set_op_mode(nand, SNFI_MAC_MODE);

	parent->erase_block(nand, row);

	return nand_spi_wait_ready(nand, READY_TIMEOUT);
}

static int nand_chip_spi_ctrl(struct nand_chip *chip, int cmd,
			      void *args)
{
	struct nand_base *nand = chip->nand;
	struct device_spi *dev = device_to_spi(nand->dev);
	struct nand_spi *spi = base_to_spi(nand);
	struct nfi *nfi = nand->nfi;
	int ret = 0, value = *(int *)args;

	switch (cmd) {
	case CHIP_CTRL_ONDIE_ECC:
		spi->ondie_ecc = (bool)value;
		ret = nand_spi_set_config(nand, dev->feature.config.addr,
					  BIT(dev->feature.config.ecc_en_bit),
					  spi->ondie_ecc);
		break;

	case SNFI_CTRL_TX_MODE:
		if (value < 0 || value > SNFI_TX_114)
			return -EOPNOTSUPP;

		if (dev->tx_mode_mask & BIT(value)) {
			spi->tx_mode = value;
			nand->dev->cmds->random_out_1st = spi_replace_tx_cmds(
								  spi->tx_mode);
			ret = nfi->nfi_ctrl(nfi, cmd, args);
		}

		break;

	case SNFI_CTRL_RX_MODE:
		if (value < 0 || value > SNFI_RX_144)
			return -EOPNOTSUPP;

		if (dev->rx_mode_mask & BIT(value)) {
			spi->rx_mode = value;
			nand->dev->cmds->program_1st = spi_replace_rx_cmds(
							       spi->rx_mode);
			ret = nfi->nfi_ctrl(nfi, cmd, args);
		}

		break;

	case CHIP_CTRL_OPS_CACHE:
	case CHIP_CTRL_OPS_MULTI:
	case CHIP_CTRL_PSLC_MODE:
	case CHIP_CTRL_DDR_MODE:
	case CHIP_CTRL_DRIVE_STRENGTH:
	case CHIP_CTRL_TIMING_MODE:
		ret = -EOPNOTSUPP;
		break;

	default:
		ret = nfi->nfi_ctrl(nfi, cmd, args);
		break;
	}

	return ret;
}

int nand_chip_spi_resume(struct nand_chip *chip)
{
	struct nand_base *nand = chip->nand;
	struct nand_spi *spi = base_to_spi(nand);
	struct device_spi *dev = device_to_spi(nand->dev);
	struct nfi *nfi = nand->nfi;
	struct nfi_format format;
	u8 mask;

	nand->reset(nand);

	mask = GENMASK(dev->feature.protect.bp_end_bit,
		       dev->feature.protect.bp_start_bit);
	nand_spi_set_config(nand, dev->feature.config.addr, mask, false);
	mask =  BIT(dev->feature.config.ecc_en_bit);
	nand_spi_set_config(nand, dev->feature.config.addr, mask,
			    spi->ondie_ecc);

	format.page_size = nand->dev->page_size;
	format.spare_size = nand->dev->spare_size;
	format.ecc_req = nand->dev->endurance->ecc_req;

	return nfi->set_format(nfi, &format);
}

static int nand_spi_set_format(struct nand_base *nand)
{
	struct nfi_format format = {
		nand->dev->page_size,
		nand->dev->spare_size,
		nand->dev->endurance->ecc_req
	};

	return nand->nfi->set_format(nand->nfi, &format);
}

struct nand_base *nand_device_init(struct nand_chip *chip)
{
	struct nand_base *nand;
	struct nand_spi *spi;
	struct device_spi *dev;
	int ret, val;
	u8 mask;

#if NANDX_GLOBAL_VARIBLE
	spi = &g_nand_spi;
#else
	spi = mem_alloc(1, sizeof(struct nand_spi));
	if (!spi) {
		pr_info("alloc nand_spi fail\n");
		return NULL;
	}
#endif

	spi->ondie_ecc = false;
	spi->op_mode = SNFI_CUSTOM_MODE;
	spi->rx_mode = SNFI_RX_114;
	spi->tx_mode = SNFI_TX_114;

	spi->parent = chip->nand;
	nand = &spi->base;
	nand->dev = spi->parent->dev;
	nand->nfi = spi->parent->nfi;

	nand->select_device = nand_spi_select_device;
	nand->reset = nand_spi_reset;
	nand->read_id = nand_spi_read_id;
	nand->read_param_page = nand_spi_read_param_page;
	nand->set_feature = nand_spi_set_feature;
	nand->get_feature = nand_spi_get_feature;
	nand->read_status = nand_spi_read_status;
	nand->addressing = nand_spi_addressing;
	nand->read_page = nand_spi_read_page;
	nand->read_data = nand_spi_read_data;
	nand->write_enable = nand_spi_write_enable;
	nand->program_data = nand_spi_program_data;
	nand->program_page = nand_spi_program_page;
	nand->erase_block = nand_spi_erase_block;

	chip->chip_ctrl = nand_chip_spi_ctrl;
	chip->nand_type = NAND_SPI;
	chip->resume = nand_chip_spi_resume;

	val = 2;
	ret = chip->chip_ctrl(chip, SNFI_CTRL_DELAY_MODE, &val);
	if (ret)
		goto err;

	ret = nand_detect_device(nand);
	if (ret)
		goto err;

	nand->select_device(nand, 0);

	ret = nand_spi_set_format(nand);
	if (ret)
		goto err;

	dev = (struct device_spi *)nand->dev;

	nand->dev->cmds->random_out_1st =
		spi_replace_rx_cmds(spi->rx_mode);
	nand->dev->cmds->program_1st =
		spi_replace_tx_cmds(spi->tx_mode);

	mask = GENMASK(dev->feature.protect.bp_end_bit,
		       dev->feature.protect.bp_start_bit);
	ret = nand_spi_set_config(nand, dev->feature.protect.addr, mask, false);
	if (ret)
		goto err;

	mask =  BIT(dev->feature.config.ecc_en_bit);
	ret = nand_spi_set_config(nand, dev->feature.config.addr, mask,
				  spi->ondie_ecc);
	if (ret)
		goto err;

	return nand;

err:
#if !NANDX_GLOBAL_VARIBLE
	mem_free(spi);
#endif
	return NULL;
}

void nand_exit(struct nand_base *nand)
{
	struct nand_spi *spi = base_to_spi(nand);

	nand_base_exit(spi->parent);
#if !NANDX_GLOBAL_VARIBLE
	mem_free(spi);
#endif
}
