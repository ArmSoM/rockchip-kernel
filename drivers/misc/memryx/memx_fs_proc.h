/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _MEMX_FILE_SYSTEM_PROC_H_
#define _MEMX_FILE_SYSTEM_PROC_H_

struct memx_pcie_dev;
s32 memx_fs_proc_init(struct memx_pcie_dev *memx_dev);
void memx_fs_proc_deinit(struct memx_pcie_dev *memx_dev);

#endif
