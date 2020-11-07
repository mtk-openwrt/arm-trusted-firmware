/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __NFI_H__
#define __NFI_H__

struct nfi_format {
	int page_size;
	int spare_size;
	int ecc_req;
};

struct nfi {
	int sector_size;
	int sector_spare_size;
	int fdm_size; /*for sector*/
	int fdm_ecc_size;
	int ecc_strength;
	int ecc_parity_size; /*for sector*/

	int (*select_chip)(struct nfi *nfi, int cs);
	int (*set_format)(struct nfi *nfi, struct nfi_format *format);
	int (*set_timing)(struct nfi *nfi, void *timing, int type);
	int (*nfi_ctrl)(struct nfi *nfi, int cmd, void *args);

	int (*reset)(struct nfi *nfi);
	int (*send_cmd)(struct nfi *nfi, short cmd);
	int (*send_addr)(struct nfi *nfi, int col, int row,
			 int col_cycle, int row_cycle);
	int (*trigger)(struct nfi *nfi);

	int (*write_page)(struct nfi *nfi, u8 *data, u8 *fdm);
	int (*write_bytes)(struct nfi *nfi, u8 *data, int count);
	int (*read_sectors)(struct nfi *nfi, u8 *data, u8 *fdm,
			    int sectors);
	int (*read_bytes)(struct nfi *nfi, u8 *data, int count);

	int (*wait_ready)(struct nfi *nfi, int type, u32 timeout);

	int (*enable_randomizer)(struct nfi *nfi, u32 row, bool encode);
	int (*disable_randomizer)(struct nfi *nfi);
};

struct nfi *nfi_init(struct nfi_resource *res);
void nfi_exit(struct nfi *nfi);

#endif /* __NFI_H__ */
