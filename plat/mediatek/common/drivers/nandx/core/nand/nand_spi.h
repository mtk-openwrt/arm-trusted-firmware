/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __NAND_SPI_H__
#define __NAND_SPI_H__

/*
 * spi nand handler
 * @base: spi nand base functions
 * @parent: common parent nand base functions
 * @tx_mode: spi bus width of transfer to device
 * @rx_mode: spi bus width of transfer from device
 * @op_mode: spi nand controller (NFI) operation mode
 * @ondie_ecc: spi nand on-die ecc flag
 */

struct nand_spi {
	struct nand_base base;
	struct nand_base *parent;
	u8 tx_mode;
	u8 rx_mode;
	u8 op_mode;
	bool ondie_ecc;
};

static inline struct nand_spi *base_to_spi(struct nand_base *base)
{
	return container_of(base, struct nand_spi, base);
}

#endif /* __NAND_SPI_H__ */
