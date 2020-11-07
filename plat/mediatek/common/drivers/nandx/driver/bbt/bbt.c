/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#include "nandx_util.h"
#include "nandx_core.h"
#include "bbt.h"


#if NANDX_GLOBAL_VARIBLE
u8 g_bbt_buf[4096 + 256];
u8 g_bbt_manage[8192];
#endif

/* Not support: multi-chip */
static u8 main_bbt_pattern[] = {'B', 'b', 't', '0' };
static u8 mirror_bbt_pattern[] = {'1', 't', 'b', 'B' };

static struct bbt_manager g_bbt_manager = {
	{	{{main_bbt_pattern, 4}, 0, BBT_INVALID_ADDR},
		{{mirror_bbt_pattern, 4}, 0, BBT_INVALID_ADDR}
	},
	NAND_BBT_SCAN_MAXBLOCKS, NULL
};

static inline void set_bbt_mark(u8 *bbt, int block, u8 mark)
{
	int index, offset;

	index = GET_ENTRY(block);
	offset = GET_POSITION(block);

	bbt[index] &= ~(BBT_ENTRY_MASK << offset);
	bbt[index] |= (mark & BBT_ENTRY_MASK) << offset;
	pr_info("%s %d block:%d, bbt[%d]:0x%x, offset:%d, mark:%d\n",
		__func__, __LINE__, block, index, bbt[index], offset, mark);
}

static inline u8 get_bbt_mark(u8 *bbt, int block)
{
	int offset = GET_POSITION(block);
	int index = GET_ENTRY(block);
	u8 value = bbt[index];

	return (value >> offset) & BBT_ENTRY_MASK;
}

static void mark_nand_bad(struct nandx_info *nand, int block)
{
	u8 *buf;

#if NANDX_GLOBAL_VARIBLE
	buf = g_bbt_buf;
#else
	buf = mem_alloc(1, nand->page_size + nand->oob_size);
	if (!buf) {
		pr_info("%s, %d, memory alloc fail, pagesize:%d, oobsize:%d\n",
		       __func__, __LINE__, nand->page_size, nand->oob_size);
		return;
	}
#endif
	memset(buf, 0, nand->page_size + nand->oob_size);
	nandx_erase(block * nand->block_size, nand->block_size);
	nandx_write(buf, buf + nand->page_size, block * nand->block_size,
		    nand->page_size);

#if !NANDX_GLOBAL_VARIBLE
	mem_free(buf);
#endif
}

static inline bool is_bbt_data(u8 *buf, struct bbt_pattern *pattern)
{
	int i;

	for (i = 0; i < pattern->len; i++) {
		if (buf[i] != pattern->data[i])
			return false;
	}

	return true;
}

static u64 get_bbt_address(struct nandx_info *nand, u8 *bbt,
			   u64 mirror_addr,
			   int max_blocks)
{
	u64 addr, end_addr;
	u8 mark;

	addr = nand->total_size;
	end_addr = nand->total_size - nand->block_size * max_blocks;

	while (addr > end_addr) {
		addr -= nand->block_size;
		mark = get_bbt_mark(bbt, div_down(addr, nand->block_size));

		if (mark == BBT_BLOCK_WORN || mark == BBT_BLOCK_FACTORY_BAD)
			continue;
		if (addr != mirror_addr)
			return addr;
	}

	return BBT_INVALID_ADDR;
}

static int read_bbt(struct bbt_desc *desc, u8 *bbt, u32 len)
{
	int ret;

	ret = nandx_read(bbt, NULL, desc->bbt_addr + desc->pattern.len + 1,
			 len);
	if (ret < 0)
		pr_info("nand_bbt: error reading BBT page, ret:-%x\n", ret);

	return ret;
}

static void create_bbt(struct nandx_info *nand, u8 *bbt)
{
	u32 offset = 0, block = 0;

	do {
		if (nandx_is_bad_block(offset)) {
			pr_info("Create bbt at bad block:%d\n", block);
			set_bbt_mark(bbt, block, BBT_BLOCK_FACTORY_BAD);
		}
		block++;
		offset += nand->block_size;
	} while (offset < nand->total_size);
}

static int search_bbt(struct nandx_info *nand, struct bbt_desc *desc,
		      int max_blocks)
{
	u64 addr, end_addr;
	u8 *buf;
	int ret;

#if NANDX_GLOBAL_VARIBLE
	buf = g_bbt_buf;
#else
	buf = mem_alloc(1, nand->page_size);
	if (!buf) {
		pr_info("%s, %d, mem alloc fail!!! len:%d\n",
		       __func__, __LINE__, nand->page_size);
		return -ENOMEM;
	}
#endif
	addr = nand->total_size;
	end_addr = nand->total_size - max_blocks * nand->block_size;
	while (addr > end_addr) {
		addr -= nand->block_size;

		nandx_read(buf, NULL, addr, nand->page_size);

		if (is_bbt_data(buf, &desc->pattern)) {
			desc->bbt_addr = addr;
			desc->version = buf[desc->pattern.len];
			pr_info("BBT is found at addr 0x%llx, version %d\n",
				desc->bbt_addr, desc->version);
			ret = 0;
			break;
		}
		ret = -EFAULT;
	}

#if !NANDX_GLOBAL_VARIBLE
	mem_free(buf);
#endif
	return ret;
}

static int save_bbt(struct nandx_info *nand, struct bbt_desc *desc,
		    u8 *bbt)
{
	u32 page_size_mask, total_block;
	int write_len;
	u8 *buf;
	int ret;

	ret = nandx_erase(desc->bbt_addr, nand->block_size);
	if (ret) {
		pr_info("erase addr 0x%llx fail !!!, ret %d\n",
			desc->bbt_addr, ret);
		return ret;
	}

	total_block = div_down(nand->total_size, nand->block_size);
	write_len = GET_BBT_LENGTH(total_block) + desc->pattern.len + 1;
	page_size_mask = nand->page_size - 1;
	write_len = (write_len + page_size_mask) & (~page_size_mask);

#if NANDX_GLOBAL_VARIBLE
	buf = g_bbt_buf;
#else
	buf = (u8 *)mem_alloc(1, write_len);
	if (!buf) {
		pr_info("%s, %d, mem alloc fail!!! len:%d\n",
		       __func__, __LINE__, write_len);
		return -ENOMEM;
	}
#endif
	memset(buf, 0xFF, write_len);

	memcpy(buf, desc->pattern.data, desc->pattern.len);
	buf[desc->pattern.len] = desc->version;

	memcpy(buf + desc->pattern.len + 1, bbt, GET_BBT_LENGTH(total_block));

	ret = nandx_write(buf, NULL, desc->bbt_addr, write_len);

	if (ret)
		pr_info("nandx_write fail(%d), offset:0x%llx, len(%d)\n",
		       ret, desc->bbt_addr, write_len);
#if !NANDX_GLOBAL_VARIBLE
	mem_free(buf);
#endif

	return ret;
}

static int write_bbt(struct nandx_info *nand, struct bbt_desc *main,
		     struct bbt_desc *mirror, u8 *bbt, int max_blocks)
{
	int block;
	int ret;

	do {
		if (main->bbt_addr == BBT_INVALID_ADDR) {
			main->bbt_addr = get_bbt_address(nand, bbt,
					 mirror->bbt_addr, max_blocks);
			if (main->bbt_addr == BBT_INVALID_ADDR)
				return -ENOSPC;
		}

		ret = save_bbt(nand, main, bbt);
		if (!ret)
			break;

		block = div_down(main->bbt_addr, nand->block_size);
		set_bbt_mark(bbt, block, BBT_BLOCK_WORN);
		main->version++;
		mark_nand_bad(nand, block);
		main->bbt_addr = BBT_INVALID_ADDR;
	} while (1);

	return 0;
}

static void mark_bbt_region(struct nandx_info *nand, u8 *bbt, int bbt_blocks)
{
	int total_block;
	int block;
	u8 mark;

	total_block = div_down(nand->total_size, nand->block_size);
	block = total_block - bbt_blocks;

	while (bbt_blocks) {
		mark = get_bbt_mark(bbt, block);
		if (mark == BBT_BLOCK_GOOD)
			set_bbt_mark(bbt, block, BBT_BLOCK_RESERVED);
		block++;
		bbt_blocks--;
	}
}

static void unmark_bbt_region(struct nandx_info *nand, u8 *bbt, int bbt_blocks)
{
	int total_block;
	int block;
	u8 mark;

	total_block = div_down(nand->total_size, nand->block_size);
	block = total_block - bbt_blocks;

	while (bbt_blocks) {
		mark = get_bbt_mark(bbt, block);
		if (mark == BBT_BLOCK_RESERVED)
			set_bbt_mark(bbt, block, BBT_BLOCK_GOOD);
		block++;
		bbt_blocks--;
	}
}

static int update_bbt(struct nandx_info *nand, struct bbt_desc *desc,
		      u8 *bbt,
		      int max_blocks)
{
	int ret = 0, i;

	/* The reserved info is not stored in NAND*/
	unmark_bbt_region(nand, bbt, max_blocks);

	desc[0].version++;
	for (i = 0; i < 2; i++) {
		if (i > 0)
			desc[i].version = desc[i - 1].version;

		ret = write_bbt(nand, &desc[i], &desc[1 - i], bbt, max_blocks);
		if (ret)
			break;
	}
	mark_bbt_region(nand, bbt, max_blocks);

	return ret;
}

int scan_bbt(struct nandx_info *nand)
{
	struct bbt_manager *manager = &g_bbt_manager;
	struct bbt_desc *pdesc;
	int total_block, len, i;
	int valid_desc = 0;
	int ret = 0;
	u8 *bbt;

	total_block = div_down(nand->total_size, nand->block_size);
	len = GET_BBT_LENGTH(total_block);

	if (!manager->bbt) {
	#if NANDX_GLOBAL_VARIBLE
		manager->bbt = g_bbt_manage;
	#else
		manager->bbt = (u8 *)mem_alloc(1, len);
		if (!manager->bbt) {
			pr_info("%s, %d, mem alloc fail!!! len:%d\n",
			       __func__, __LINE__, len);
			return -ENOMEM;
		}
	#endif
	}
	bbt = manager->bbt;
	memset(bbt, 0xFF, len);

	/* scan bbt */
	for (i = 0; i < 2; i++) {
		pdesc = &manager->desc[i];
		pdesc->bbt_addr = BBT_INVALID_ADDR;
		pdesc->version = 0;
		ret = search_bbt(nand, pdesc, manager->max_blocks);
		if (!ret && (pdesc->bbt_addr != BBT_INVALID_ADDR))
			valid_desc += 1 << i;
	}

	pdesc = &manager->desc[0];
	if ((valid_desc == 0x3) && (pdesc[0].version != pdesc[1].version))
		valid_desc = (pdesc[0].version > pdesc[1].version) ? 1 : 2;

	/* read bbt */
	for (i = 0; i < 2; i++) {
		if (!(valid_desc & (1 << i)))
			continue;
		ret = read_bbt(&pdesc[i], bbt, len);
		if (ret) {
			pdesc->bbt_addr = BBT_INVALID_ADDR;
			pdesc->version = 0;
			valid_desc &= ~(1 << i);
		}
		/* If two BBT version is same, only need to read the first bbt*/
		if ((valid_desc == 0x3) &&
		    (pdesc[0].version == pdesc[1].version))
			break;
	}

	if (!valid_desc) {
		create_bbt(nand, bbt);
		pdesc[0].version = 1;
		pdesc[1].version = 1;
	}

	pdesc[0].version = max_t(u8, pdesc[0].version, pdesc[1].version);
	pdesc[1].version = pdesc[0].version;

	for (i = 0; i < 2; i++) {
		if (valid_desc & (1 << i))
			continue;

		ret = write_bbt(nand, &pdesc[i], &pdesc[1 - i], bbt,
				manager->max_blocks);
		if (ret) {
			pr_info("write bbt(%d) fail, ret:%d\n", i, ret);
			manager->bbt = NULL;
			return ret;
		}
	}

	/* Prevent the bbt regions from erasing / writing */
	mark_bbt_region(nand, manager->bbt, manager->max_blocks);

	for (i = 0; i < total_block; i++) {
		if (get_bbt_mark(manager->bbt, i) == BBT_BLOCK_WORN)
			pr_info("Checked WORN bad blk: %d\n", i);
		else if (get_bbt_mark(manager->bbt, i) == BBT_BLOCK_FACTORY_BAD)
			pr_info("Checked Factory bad blk: %d\n", i);
		else if (get_bbt_mark(manager->bbt, i) == BBT_BLOCK_RESERVED)
			pr_info("Checked Reserved blk: %d\n", i);
		else if (get_bbt_mark(manager->bbt, i) != BBT_BLOCK_GOOD)
			pr_info("Checked unknown blk: %d\n", i);
	}

	return 0;
}

int bbt_mark_bad(struct nandx_info *nand, off_t offset)
{
	struct bbt_manager *manager = &g_bbt_manager;
	int block = div_down(offset, nand->block_size);
	int ret = 0;

	mark_nand_bad(nand, block);

#if 0
	set_bbt_mark(manager->bbt, block, BBT_BLOCK_WORN);

	/* Update flash-based bad block table */
	ret = update_bbt(nand, manager->desc, manager->bbt,
			 manager->max_blocks);
#endif
	pr_info("block %d, update result %d.\n", block, ret);

	return ret;
}

int bbt_is_bad(struct nandx_info *nand, off_t offset)
{
	int block;

	block = div_down(offset, nand->block_size);

	return get_bbt_mark(g_bbt_manager.bbt, block) != BBT_BLOCK_GOOD;
}
