/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _MEMX_FIRMWARE_INIT_H_
#define _MEMX_FIRMWARE_INIT_H_

#define FIRMWARE_BIN_NAME "cascade.bin"
struct memx_pcie_dev;
struct memx_firmware_bin;
s32 memx_init_chip_info(struct memx_pcie_dev *memx_dev);
s32 memx_firmware_init(struct memx_pcie_dev *memx_dev, struct memx_firmware_bin *memx_bin);
s32 memx_get_hw_info(struct memx_pcie_dev *memx_dev);
#endif
