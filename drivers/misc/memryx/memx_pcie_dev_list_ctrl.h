/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _MEMX_PCIE_DEVICE_LIST_CONTROL_H_
#define _MEMX_PCIE_DEVICE_LIST_CONTROL_H_

struct memx_pcie_dev;
struct memx_pcie_dev *memx_get_device_by_index(u32 index);
void memx_insert_device(struct memx_pcie_dev *memx_dev);
void memx_pcie_remove_device(struct memx_pcie_dev *memx_dev);
#endif
