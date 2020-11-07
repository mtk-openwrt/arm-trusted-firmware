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
struct nand_base g_nand_base;
#endif

static int nand_base_select_device(struct nand_base *nand, int cs)
{
	struct nfi *nfi = nand->nfi;

	nfi->reset(nfi);

	return nfi->select_chip(nfi, cs);
}

static int nand_base_reset(struct nand_base *nand)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->reset);
	nfi->trigger(nfi);

	return nfi->wait_ready(nfi, NAND_WAIT_POLLING, dev->array_timing->tRST);
}

static int nand_base_read_id(struct nand_base *nand, u8 *id, int count)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->read_id);
	nfi->wait_ready(nfi, NAND_WAIT_POLLING, dev->array_timing->tWHR);
	nfi->send_addr(nfi, 0, 0, 1, 0);

	return nfi->read_bytes(nfi, id, count);
}

static int nand_base_read_param_page(struct nand_base *nand, u8 *data,
				     int count)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->read_param_page);
	nfi->send_addr(nfi, 0, 0, 1, 0);

	nfi->wait_ready(nfi, NAND_WAIT_POLLING, dev->array_timing->tR);

	return nfi->read_bytes(nfi, data, count);
}

static int nand_base_set_feature(struct nand_base *nand, u8 addr,
				 u8 *param,
				 int count)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->set_feature);
	nfi->send_addr(nfi, addr, 0, 1, 0);

	nfi->write_bytes(nfi, param, count);

	return nfi->wait_ready(nfi, NAND_WAIT_POLLING,
			       dev->array_timing->tFEAT);
}

static int nand_base_get_feature(struct nand_base *nand, u8 addr,
				 u8 *param,
				 int count)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->get_feature);
	nfi->send_addr(nfi, addr, 0, 1, 0);
	nfi->wait_ready(nfi, NAND_WAIT_POLLING, dev->array_timing->tFEAT);

	return nfi->read_bytes(nfi, param, count);
}

static int nand_base_read_status(struct nand_base *nand)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;
	u8 status = 0;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->read_status);
	nfi->wait_ready(nfi, NAND_WAIT_POLLING, dev->array_timing->tWHR);
	nfi->read_bytes(nfi, &status, 1);

	return status;
}

static int nand_base_addressing(struct nand_base *nand, int *row,
				int *col)
{
	struct nand_device *dev = nand->dev;
	int lun, plane, block, page, cs = 0;
	int block_pages, target_blocks, wl = 0;
	int icol = *col;

	if (dev->target_num > 1) {
		block_pages = nand_block_pages(dev);
		target_blocks = nand_target_blocks(dev);
		cs = div_down(*row, block_pages * target_blocks);
		*row -= cs * block_pages * target_blocks;
	}

	nand->select_device(nand, cs);

	block_pages = nand_block_pages(dev);
	block = div_down(*row, block_pages);
	page = *row - block * block_pages;
	plane = reminder(block, dev->plane_num);
	lun = div_down(block, nand_lun_blocks(dev));

	wl |= (page << dev->addressing->row_bit_start);
	wl |= (block << dev->addressing->block_bit_start);
	wl |= (plane << dev->addressing->plane_bit_start);
	wl |= (lun << dev->addressing->lun_bit_start);

	*row = wl;
	*col = icol;

	return 0;
}

static int nand_base_read_page(struct nand_base *nand, int row)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->read_1st);
	nfi->send_addr(nfi, 0, row, dev->col_cycle, dev->row_cycle);
	nfi->send_cmd(nfi, dev->cmds->read_2nd);
	nfi->trigger(nfi);

	return nfi->wait_ready(nfi, NAND_WAIT_POLLING, dev->array_timing->tR);
}

static int nand_base_read_data(struct nand_base *nand, int row, int col,
			       int sectors, u8 *data, u8 *oob)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->random_out_1st);
	nfi->send_addr(nfi, col, row, dev->col_cycle, dev->row_cycle);
	nfi->send_cmd(nfi, dev->cmds->random_out_2nd);
	nfi->wait_ready(nfi, NAND_WAIT_POLLING, dev->array_timing->tRCBSY);

	return nfi->read_sectors(nfi, data, oob, sectors);
}

static int nand_base_write_enable(struct nand_base *nand)
{
	struct nand_device *dev = nand->dev;
	int status;

	status = nand_base_read_status(nand);
	if (status & dev->status->write_protect)
		return 0;

	return -ENANDWP;
}

static int nand_base_program_data(struct nand_base *nand, int row,
				  int col,
				  u8 *data, u8 *oob)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->program_1st);
	nfi->send_addr(nfi, 0, row, dev->col_cycle, dev->row_cycle);

	return nfi->write_page(nfi, data, oob);
}

static int nand_base_program_page(struct nand_base *nand, int row)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->program_2nd);
	nfi->trigger(nfi);

	return nfi->wait_ready(nfi, NAND_WAIT_POLLING,
			       dev->array_timing->tPROG);
}

static int nand_base_erase_block(struct nand_base *nand, int row)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->erase_1st);
	nfi->send_addr(nfi, 0, row, 0, dev->row_cycle);
	nfi->send_cmd(nfi, dev->cmds->erase_2nd);
	nfi->trigger(nfi);

	return nfi->wait_ready(nfi, NAND_WAIT_POLLING,
			       dev->array_timing->tBERS);
}

static int nand_base_read_cache(struct nand_base *nand, int row)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->read_1st);
	nfi->send_addr(nfi, 0, row, dev->col_cycle, dev->row_cycle);
	nfi->send_cmd(nfi, dev->cmds->read_cache);
	nfi->trigger(nfi);

	return nfi->wait_ready(nfi, NAND_WAIT_POLLING,
			       dev->array_timing->tRCBSY);
}

static int nand_base_read_last(struct nand_base *nand)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->read_cache_last);
	nfi->trigger(nfi);

	return nfi->wait_ready(nfi, NAND_WAIT_POLLING,
			       dev->array_timing->tRCBSY);
}

static int nand_base_program_cache(struct nand_base *nand)
{
	struct nfi *nfi = nand->nfi;
	struct nand_device *dev = nand->dev;

	nfi->reset(nfi);
	nfi->send_cmd(nfi, dev->cmds->program_cache);
	nfi->trigger(nfi);

	return nfi->wait_ready(nfi, NAND_WAIT_POLLING,
			       dev->array_timing->tPCBSY);
}

struct nand_base *nand_base_init(struct nand_device *dev,
				 struct nfi *nfi)
{
	struct nand_base *nand;

#if NANDX_GLOBAL_VARIBLE
	nand  = &g_nand_base;
#else
	nand = mem_alloc(1, sizeof(struct nand_base));
	if (!nand)
		return NULL;
#endif
	nand->dev = dev;
	nand->nfi = nfi;
	nand->select_device = nand_base_select_device;
	nand->reset = nand_base_reset;
	nand->read_id = nand_base_read_id;
	nand->read_param_page = nand_base_read_param_page;
	nand->set_feature = nand_base_set_feature;
	nand->get_feature = nand_base_get_feature;
	nand->read_status = nand_base_read_status;
	nand->addressing = nand_base_addressing;
	nand->read_page = nand_base_read_page;
	nand->read_data = nand_base_read_data;
	nand->read_cache = nand_base_read_cache;
	nand->read_last = nand_base_read_last;
	nand->write_enable = nand_base_write_enable;
	nand->program_data = nand_base_program_data;
	nand->program_page = nand_base_program_page;
	nand->program_cache = nand_base_program_cache;
	nand->erase_block = nand_base_erase_block;

	return nand;
}

void nand_base_exit(struct nand_base *base)
{
	nfi_exit(base->nfi);
#if !NANDX_GLOBAL_VARIBLE
	mem_free(base);
#endif
}
