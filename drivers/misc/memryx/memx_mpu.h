/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _MEMX_MPU_H_
#define _MEMX_MPU_H_
#include <linux/wait.h>
#include <linux/spinlock.h>
#include "memx_ioctl.h"
#include "memx_fw_log.h"

/*
 *0x0     +--------------------------+
		|       RX DMA Buf         |
0x80000 +--------------------------+
		|       TX DMA Buf         |
0x100000+--------------------------+
		|       Debug log Buf      |
0x200000+--------------------------+
*/
#define OFMAP_EGRESS_DCORE_DMA_COHERENT_BUFFER_SIZE_512KB  (0x80000)
#define IFMAP_INGRESS_DCORE_DMA_COHERENT_BUFFER_SIZE_512KB (0x80000)
#define FW_LOG_DUMP_BUFFER_SIZE_1MB (0x100000)

#define DMA_COHERENT_BUFFER_SIZE_1MB (OFMAP_EGRESS_DCORE_DMA_COHERENT_BUFFER_SIZE_512KB + IFMAP_INGRESS_DCORE_DMA_COHERENT_BUFFER_SIZE_512KB)
#define DMA_COHERENT_BUFFER_SIZE_2MB (DMA_COHERENT_BUFFER_SIZE_1MB + FW_LOG_DUMP_BUFFER_SIZE_1MB)

#define MPU_REGISTER_BASE   (0x30000000)
#define MPU_REGISTER_START  (MPU_REGISTER_BASE)
#define MPU_REGISTER_END    (0x3FFFFFFF)
#define MPU_SRAM_BASE       (0x40000000)
#define MPU_FW_DL_BASE      (0x40040000)
#define MPU_FW_CMD_BASE     (0x40046E00)
#define MEMX_IFMAP_INGRESS_DONE_MSIX_OFFS (32)

enum memx_chip_ids {
	CHIP_ID0 = 0,
	CHIP_ID1,
	CHIP_ID2,
	CHIP_ID3,
	CHIP_ID4,
	CHIP_ID5,
	CHIP_ID6,
	CHIP_ID7,
	CHIP_ID8,
	CHIP_ID9,
	CHIP_ID10,
	CHIP_ID11,
	CHIP_ID12,
	CHIP_ID13,
	CHIP_ID14,
	CHIP_ID15,
	MAX_CHIP_NUM
};

enum memx_group_ids {
	MEMX_GROUP_0 = 0,
	MEMX_GROUP_1,
	MEMX_GROUP_2,
	MEMX_GROUP_3,
	MEMX_GROUP_4,
	MEMX_GROUP_5,
	MEMX_GROUP_6,
	MEMX_GROUP_7,
	MAX_GROUP_NUM
};


struct control {
	u32 is_abort;
	u32 is_read_abort;
	spinlock_t lock;
	// -1 means no data need to process, otherwise the value represent chip_idx(i.e range 0 ~ 31) means some chip output flow data need to process
	s32 indicator;
	wait_queue_head_t wq;
};

struct memx_mpu_data {
	struct control rx_ctrl;
	struct control tx_ctrl[MAX_CHIP_NUM];
	struct control fw_ctrl;

	struct hw_info hw_info;

	struct page *dma_pages;
	void *rx_dma_coherent_buffer_virtual_base;
	u8 *mmap_fw_cmd_buffer_base;
	u8 *mmap_chip0_sram_buffer_base;
	struct memx_fw_log_fifo fw_log;
};
#endif
