/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __NFIECC_H__
#define __NFIECC_H__

enum nfiecc_mode {
	ECC_DMA_MODE,
	ECC_NFI_MODE,
	ECC_PIO_MODE
};

enum nfiecc_operation {
	ECC_ENCODE,
	ECC_DECODE
};

enum nfiecc_deccon {
	ECC_DEC_FER = 1,
	ECC_DEC_LOCATE = 2,
	ECC_DEC_CORRECT = 3
};

struct nfiecc_resource {
	int ic_ver;
	void *dev;
	void *regs;
	int irq_id;

};

struct nfiecc_status {
	u32 corrected;
	u32 failed;
	u32 bitflips;
};

struct nfiecc_caps {
	u32 err_mask;
	u32 ecc_mode_shift;
	u32 parity_bits;
	const int *ecc_strength;
	u32 ecc_strength_num;
};

struct nfiecc_config {
	enum nfiecc_operation op;
	enum nfiecc_mode mode;
	enum nfiecc_deccon deccon;

	void *dma_addr; /* DMA use only */
	u32 strength;
	u32 sectors;
	u32 len;
};

struct nfiecc {
	struct nfiecc_resource res;
	struct nfiecc_config config;
	struct nfiecc_caps *caps;

	bool ecc_irq_en;
	bool page_irq_en;

	void *done;

	int (*adjust_strength)(struct nfiecc *ecc, int strength);
	int (*enable)(struct nfiecc *ecc);
	int (*disable)(struct nfiecc *ecc);

	int (*decode)(struct nfiecc *ecc, u8 *data);
	int (*encode)(struct nfiecc *ecc, u8 *data);

	int (*decode_status)(struct nfiecc *ecc, u32 start_sector, u32 sectors);
	int (*correct_data)(struct nfiecc *ecc,
			    struct nfiecc_status *status,
			    u8 *data, u32 sector);
	int (*wait_done)(struct nfiecc *ecc);

	int (*nfiecc_ctrl)(struct nfiecc *ecc, int cmd, void *args);
};

struct nfiecc *nfiecc_init(struct nfiecc_resource *res);
void nfiecc_exit(struct nfiecc *ecc);

#endif /* __NFIECC_H__ */
