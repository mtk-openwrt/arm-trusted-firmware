/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __NAND_CHIP_H__
#define __NAND_CHIP_H__

enum nand_type {
	NAND_SPI,
	NAND_SLC,
	NAND_MLC,
	NAND_TLC
};

/*
 * nand chip operation unit
 *    one nand_ops indicates one row operation
 * @row: nand chip row address, like as nand row
 * @col: nand chip column address, like as nand column
 * @len: operate data length, min is sector_size,
 *    max is page_size and sector_size aligned
 * @status: one operation result status
 * @data: data buffer for operation
 * @oob: oob buffer for operation, like as nand spare area
 */
struct nand_ops {
	int row;
	int col;
	int len;
	int status;
	void *data;
	void *oob;
};

/*
 * nand chip descriptions
 *    nand chip includes nand controller and the several same nand devices
 * @nand_type: the nand type on this chip,
 *    the chip maybe have several nand device and the type must be same
 * @plane_num: the whole plane number on the chip
 * @block_num: the whole block number on the chip
 * @block_size: nand device block size
 * @block_pages: nand device block has page number
 * @page_size: nand device page size
 * @oob_size: chip out of band size, like as nand spare szie,
 *    but restricts this:
 *    the size is provied by nand controller(NFI),
 *    because NFI would use some nand spare size
 * @min_program_pages: chip needs min pages per program operations
 *    one page as one nand_ops
 * @sector_size: chip min read size
 * @sector_spare_size: spare size for sector, is spare_size/page_sectors
 * @ecc_strength: ecc stregth per sector_size, it would be for calculated ecc
 * @ecc_parity_size: ecc parity size for one  sector_size data
 * @nand: pointer to inherited struct nand_base
 * @read_page: read %count pages on chip
 * @write_page: write %count pages on chip
 * @erase_block: erase %count blocks on chip, one block is one nand_ops
 *    it is better to set nand_ops.row to block start row
 * @is_bad_block: judge the %count blocks on chip if they are bad
 *    by vendor specification
 * @chip_ctrl: control the chip features by nandx_ctrl_cmd
 * @suspend: suspend nand chip
 * @resume: resume nand chip
 */
struct nand_chip {
	int nand_type;
	int plane_num;
	int block_num;
	int block_size;
	int block_pages;
	int page_size;
	int oob_size;
	int spare_size;

	int min_program_pages;
	int sector_size;
	int sector_spare_size;
	int ecc_strength;
	int ecc_parity_size;
	u32 fdm_ecc_size;
	u32 fdm_reg_size;

	void *nand;

	int (*read_page)(struct nand_chip *chip, struct nand_ops *ops,
			 int count);
	int (*write_page)(struct nand_chip *chip, struct nand_ops *ops,
			  int count);
	int (*erase_block)(struct nand_chip *chip, struct nand_ops *ops,
			   int count);
	int (*is_bad_block)(struct nand_chip *chip, struct nand_ops *ops,
			    int count);
	int (*chip_ctrl)(struct nand_chip *chip, int cmd, void *args);
	int (*suspend)(struct nand_chip *chip);
	int (*resume)(struct nand_chip *chip);
};

struct nand_chip *nand_chip_init(struct nfi_resource *res);
void nand_chip_exit(struct nand_chip *chip);
#endif /* __NAND_CHIP_H__ */
