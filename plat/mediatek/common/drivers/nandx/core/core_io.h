/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __CORE_IO_H__
#define __CORE_IO_H__

typedef int (*func_chip_ops)(struct nand_chip *, struct nand_ops *,
			     int);

enum nandx_op_mode {
	NANDX_IDLE,
	NANDX_WRITE,
	NANDX_READ,
	NANDX_ERASE
};

struct nandx_desc {
	struct nand_chip *chip;
	struct nandx_info info;
	enum nandx_op_mode mode;

	bool multi_en;
	bool ecc_en;

	struct nand_ops *ops;
	int ops_len;
	int ops_multi_len;
	int ops_current;
	int min_write_pages;

	u8 *head_buf;
	u8 *tail_buf;
};

#endif /* __CORE_IO_H__ */
