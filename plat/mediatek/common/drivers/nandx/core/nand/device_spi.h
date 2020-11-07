/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __DEVICE_SPI_H__
#define __DEVICE_SPI_H__

/*
 * extend commands
 * @die_select: select nand device die command
 * @write_enable: enable write command before write data to spi nand
 *    spi nand device will auto to be disable after write done
 */
struct spi_extend_cmds {
	short die_select;
	short write_enable;
};

/*
 * protection feature register
 * @addr: register address
 * @wp_en_bit: write protection enable bit
 * @bp_start_bit: block protection mask start bit
 * @bp_end_bit: block protection mask end bit
 */
struct feature_protect {
	u8 addr;
	u8 wp_en_bit;
	u8 bp_start_bit;
	u8 bp_end_bit;
};

/*
 * configuration feature register
 * @addr: register address
 * @ecc_en_bit: in-die ecc enable bit
 * @otp_en_bit: enter otp access mode bit
 * @need_qe: quad io enable bit
 */
struct feature_config {
	u8 addr;
	u8 ecc_en_bit;
	u8 otp_en_bit;
	u8 need_qe;
};

/*
 * status feature register
 * @addr: register address
 * @ecc_start_bit: ecc status mask start bit for error bits number
 * @ecc_end_bit: ecc status mask end bit for error bits number
 * note that:
 *   operations status (ex. array busy status) could see on struct nand_status
 */
struct feature_status {
	u8 addr;
	u8 ecc_start_bit;
	u8 ecc_end_bit;
};

/*
 * character feature register
 * @addr: register address
 * @die_sel_bit: die select bit
 * @drive_start_bit: drive strength mask start bit
 * @drive_end_bit: drive strength mask end bit
 */
struct feature_character {
	u8 addr;
	u8 die_sel_bit;
	u8 drive_start_bit;
	u8 drive_end_bit;
};

/*
 * spi features
 * @protect: protection feature register
 * @config: configuration feature register
 * @status: status feature register
 * @character: character feature register
 */
struct spi_features {
	struct feature_protect protect;
	struct feature_config config;
	struct feature_status status;
	struct feature_character character;
};

/*
 * device_spi
 *    configurations of spi nand device table
 * @dev: base information of nand device
 * @feature: feature information for spi nand
 * @extend_cmds: extended the nand base commands
 * @tx_mode_mask: tx mode mask for chip read
 * @rx_mode_mask: rx mode mask for chip write
 */
struct device_spi {
	struct nand_device dev;
	struct spi_features feature;
	struct spi_extend_cmds *extend_cmds;

	u8 tx_mode_mask;
	u8 rx_mode_mask;
};

#define NAND_SPI_PROTECT(addr, wp_en_bit, bp_start_bit, bp_end_bit) \
	{addr, wp_en_bit, bp_start_bit, bp_end_bit}

#define NAND_SPI_CONFIG(addr, ecc_en_bit, otp_en_bit, need_qe) \
	{addr, ecc_en_bit, otp_en_bit, need_qe}

#define NAND_SPI_STATUS(addr, ecc_start_bit, ecc_end_bit) \
	{addr, ecc_start_bit, ecc_end_bit}

#define NAND_SPI_CHARACTER(addr, die_sel_bit, drive_start_bit, drive_end_bit) \
	{addr, die_sel_bit, drive_start_bit, drive_end_bit}

static inline struct device_spi *device_to_spi(struct nand_device *dev)
{
	return container_of(dev, struct device_spi, dev);
}

u8 spi_replace_rx_cmds(u8 mode);
u8 spi_replace_tx_cmds(u8 mode);
u8 spi_replace_rx_col_cycle(u8 mode);
u8 spi_replace_tx_col_cycle(u8 mode);

#endif /* __DEVICE_SPI_H__ */
