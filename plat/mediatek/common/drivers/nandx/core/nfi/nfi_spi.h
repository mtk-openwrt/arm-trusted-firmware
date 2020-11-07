/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __NFI_SPI_H__
#define __NFI_SPI_H__

#define SPI_NAND_MAX_DELAY      6
#define SPI_NAND_MAX_OP         4

/*TODO - add comments */
struct nfi_spi_delay {
	u8 tCLK_SAM_DLY;
	u8 tCLK_OUT_DLY;
	u8 tCS_DLY;
	u8 tWR_EN_DLY;
	u8 tIO_IN_DLY[4];
	u8 tIO_OUT_DLY[4];
	u8 tREAD_LATCH_LATENCY;
};

/* SPI Nand structure */
struct nfi_spi {
	struct nfi_base base;
	struct nfi_base *parent;

	u8 snfi_mode;
	u8 tx_count;

	u8 cmd[SPI_NAND_MAX_OP];
	u8 cur_cmd_idx;

	u32 row_addr[SPI_NAND_MAX_OP];
	u32 col_addr[SPI_NAND_MAX_OP];
	u8 cur_addr_idx;

	u8 read_cache_mode;
	u8 write_cache_mode;
};

#endif /* __NFI_SPI_H__ */
