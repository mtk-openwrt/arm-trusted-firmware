/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#include "nandx_util.h"
#include "../nand_device.h"
#include "device_spi.h"

/* spi nand basic commands */
static struct nand_cmds spi_cmds = {
	.reset = 0xff,
	.read_id = 0x9f,
	.read_status = 0x0f,
	.read_param_page = 0x03,
	.set_feature = 0x1f,
	.get_feature = 0x0f,
	.read_1st = 0x13,
	.read_2nd = -1,
	.random_out_1st = 0x03,
	.random_out_2nd = -1,
	.program_1st = 0x02,
	.program_2nd = 0x10,
	.erase_1st = 0xd8,
	.erase_2nd = -1,
	.read_cache = 0x30,
	.read_cache_last = 0x3f,
	.program_cache = 0x02
};

/* spi nand extend commands */
static struct spi_extend_cmds spi_extend_cmds = {
	.die_select = 0xc2,
	.write_enable = 0x06
};

/* means the start bit of addressing type */
static struct nand_addressing spi_addressing = {
	.row_bit_start = 0,
	.block_bit_start = 0,
	.plane_bit_start = 12,
	.lun_bit_start = 0,
};

/* spi nand endurance */
static struct nand_endurance spi_endurance = {
	.pe_cycle = 100000,
	.ecc_req = 1,
	.max_bitflips = 1
};

/* array_busy, write_protect, erase_fail, program_fail */
static struct nand_status spi_status[] = {
	{.array_busy = BIT(0),
	.write_protect = BIT(1),
	.erase_fail = BIT(2),
	.program_fail = BIT(3)}
};

/* measure time by the us */
static struct nand_array_timing spi_array_timing = {
	.tRST = 500,
	.tWHR = 1,
	.tR = 25,
	.tRCBSY = 25,
	.tFEAT = 1,
	.tPROG = 600,
	.tPCBSY = 600,
	.tBERS = 10000,
	.tDBSY = 1
};

/* spi nand device table */
static struct device_spi spi_nand[] = {
	{
		NAND_DEVICE("W25N01GV",
			    NAND_PACK_ID(0xef, 0xaa, 0x21, 0, 0, 0, 0, 0),
			    3, 0, 3, 3,
			    1, 1, 1, 1024, KB(128), KB(2), 64, 1,
			    &spi_cmds, &spi_addressing, &spi_status[0],
			    &spi_endurance, &spi_array_timing),
		{
			NAND_SPI_PROTECT(0xa0, 1, 2, 6),
			NAND_SPI_CONFIG(0xb0, 4, 6, 0),
			NAND_SPI_STATUS(0xc0, 4, 5),
			NAND_SPI_CHARACTER(0xff, 0xff, 0xff, 0xff)
		},
		&spi_extend_cmds, 0xff, 0xff
	},
	{
		NAND_DEVICE("MX35LF1G",
			    NAND_PACK_ID(0xc2, 0x12, 0x21, 0, 0, 0, 0, 0),
			    2, 0, 3, 3,
			    1, 1, 1, 1024, KB(128), KB(2), 64, 1,
			    &spi_cmds, &spi_addressing, &spi_status[0],
			    &spi_endurance, &spi_array_timing),
		{
			NAND_SPI_PROTECT(0xa0, 1, 2, 6),
			NAND_SPI_CONFIG(0xb0, 4, 6, 1),
			NAND_SPI_STATUS(0xc0, 4, 5),
			NAND_SPI_CHARACTER(0xff, 0xff, 0xff, 0xff)
		},
		&spi_extend_cmds, 0xff, 0xff
	},
	{
		NAND_DEVICE("MT29F4G01ABAFDWB",
			    NAND_PACK_ID(0x2c, 0x34, 0, 0, 0, 0, 0, 0),
			    2, 0, 3, 3,
			    1, 1, 1, 2048, KB(256), KB(4), 256, 1,
			    &spi_cmds, &spi_addressing, &spi_status[0],
			    &spi_endurance, &spi_array_timing),
		{
			NAND_SPI_PROTECT(0xa0, 1, 2, 6),
			NAND_SPI_CONFIG(0xb0, 4, 6, 1),
			NAND_SPI_STATUS(0xc0, 4, 5),
			NAND_SPI_CHARACTER(0xff, 0xff, 0xff, 0xff)
		},
		&spi_extend_cmds, 0xff, 0xff
	},
	{
		NAND_DEVICE("GD5F4GQ4UB",
			    NAND_PACK_ID(0xc8, 0xd4, 0, 0, 0, 0, 0, 0),
			    2, 0, 3, 3,
			    1, 1, 1, 2048, KB(256), KB(4), 256, 1,
			    &spi_cmds, &spi_addressing, &spi_status[0],
			    &spi_endurance, &spi_array_timing),
		{
			NAND_SPI_PROTECT(0xa0, 1, 2, 6),
			NAND_SPI_CONFIG(0xb0, 4, 6, 1),
			NAND_SPI_STATUS(0xc0, 4, 5),
			NAND_SPI_CHARACTER(0xff, 0xff, 0xff, 0xff)
		},
		&spi_extend_cmds, 0xff, 0xff
	},
	{
		NAND_DEVICE("TC58CVG2S0HRAIJ",
			    NAND_PACK_ID(0x98, 0xED, 0x51, 0, 0, 0, 0, 0),
			    3, 0, 3, 3,
			    1, 1, 1, 2048, KB(256), KB(4), 256, 1,
			    &spi_cmds, &spi_addressing, &spi_status[0],
			    &spi_endurance, &spi_array_timing),
		{
			NAND_SPI_PROTECT(0xa0, 1, 2, 6),
			NAND_SPI_CONFIG(0xb0, 4, 6, 1),
			NAND_SPI_STATUS(0xc0, 4, 5),
			NAND_SPI_CHARACTER(0xff, 0xff, 0xff, 0xff)
		},
		&spi_extend_cmds, 0xff, 0xff
	},
	{
		NAND_DEVICE("NO-DEVICE",
			    NAND_PACK_ID(0, 0, 0, 0, 0, 0, 0, 0), 0, 0, 0, 0,
			    0, 0, 0, 0, 0, 0, 0, 1,
			    &spi_cmds, &spi_addressing, &spi_status[0],
			    &spi_endurance, &spi_array_timing),
		{
			NAND_SPI_PROTECT(0xa0, 1, 2, 6),
			NAND_SPI_CONFIG(0xb0, 4, 6, 0),
			NAND_SPI_STATUS(0xc0, 4, 5),
			NAND_SPI_CHARACTER(0xff, 0xff, 0xff, 0xff)
		},
		&spi_extend_cmds, 0xff, 0xff
	}
};

u8 spi_replace_rx_cmds(u8 mode)
{
	u8 rx_replace_cmds[] = {0x03, 0x3b, 0x6b, 0xbb, 0xeb};

	return rx_replace_cmds[mode];
}

u8 spi_replace_tx_cmds(u8 mode)
{
	u8 tx_replace_cmds[] = {0x02, 0x32};

	return tx_replace_cmds[mode];
}

u8 spi_replace_rx_col_cycle(u8 mode)
{
	u8 rx_replace_col_cycle[] = {3, 3, 3, 3, 4};

	return rx_replace_col_cycle[mode];
}

u8 spi_replace_tx_col_cycle(u8 mode)
{
	u8 tx_replace_col_cycle[] = {2, 2};

	return tx_replace_col_cycle[mode];
}

struct nand_device *nand_get_device(int index)
{
	return &spi_nand[index].dev;
}

