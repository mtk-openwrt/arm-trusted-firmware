/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __NAND_BASE_H__
#define __NAND_BASE_H__

/*
 * nand base functions
 * @dev: nand device infomations
 * @nfi: nand host controller
 * @select_device: select one nand device of multi nand on chip
 * @reset: reset current nand device
 * @read_id: read current nand id
 * @read_param_page: read current nand parameters page
 * @set_feature: configurate the nand device feature
 * @get_feature: get the nand device feature
 * @read_status: read nand device status
 * @addressing: addressing the address to nand device physical address
 * @read_page: read page data to device cache register
 * @read_data: read data from device cache register by bus protocol
 * @read_cache: nand cache read operation for data output
 * @read_last: nand cache read operation for last page output
 * @write_enable: enable program/erase for nand, especially spi nand
 * @program_data: program data to nand device cache register
 * @program_page: program page data from nand device cache register to array
 * @program_cache: nand cache program operation for data input
 * @erase_block: erase nand block operation
 */
struct nand_base {
	struct nand_device *dev;
	struct nfi *nfi;
	int (*select_device)(struct nand_base *nand, int cs);
	int (*reset)(struct nand_base *nand);
	int (*read_id)(struct nand_base *nand, u8 *id, int count);
	int (*read_param_page)(struct nand_base *nand, u8 *data, int count);
	int (*set_feature)(struct nand_base *nand, u8 addr, u8 *param,
			   int count);
	int (*get_feature)(struct nand_base *nand, u8 addr, u8 *param,
			   int count);
	int (*read_status)(struct nand_base *nand);
	int (*addressing)(struct nand_base *nand, int *row, int *col);

	int (*read_page)(struct nand_base *nand, int row);
	int (*read_data)(struct nand_base *nand, int row, int col, int sectors,
			 u8 *data, u8 *oob);
	int (*read_cache)(struct nand_base *nand, int row);
	int (*read_last)(struct nand_base *nand);

	int (*write_enable)(struct nand_base *nand);
	int (*program_data)(struct nand_base *nand, int row, int col, u8 *data,
			    u8 *oob);
	int (*program_page)(struct nand_base *nand, int row);
	int (*program_cache)(struct nand_base *nand);

	int (*erase_block)(struct nand_base *nand, int row);
};

struct nand_base *nand_base_init(struct nand_device *device,
				 struct nfi *nfi);
void nand_base_exit(struct nand_base *base);

struct nand_base *nand_device_init(struct nand_chip *nand);
void nand_exit(struct nand_base *nand);

int nand_detect_device(struct nand_base *nand);

#endif /* __NAND_BASE_H__ */
