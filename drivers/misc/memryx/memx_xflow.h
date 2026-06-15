/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _MEMX_XFLOW_H_
#define _MEMX_XFLOW_H_
#include <linux/types.h>

#define XFLOW_BASE_ADDRESS_REGISTER_OFFSET      (0x0)
#define XFLOW_CONTROL_REGISTER_OFFSET           (0x4)

#define XFLOW_CONFIG_REG_PREFIX                 (0xC000000)
#define XFLOW_VIRTUAL_BUFFER_PREFIX             (0x8000000)

#define XFLOW_CHIP_ID_WIDTH                     (0xF)
#define XFLOW_CHIP_ID_SHIFT                     (22)

#define AHB_HUB_IRQ_EN_BASE                     (0x30400008)

#define GET_XFLOW_OFFSET(chip_id, is_config) \
	(((is_config) ? XFLOW_CONFIG_REG_PREFIX : XFLOW_VIRTUAL_BUFFER_PREFIX) | \
	(((chip_id) & XFLOW_CHIP_ID_WIDTH) << XFLOW_CHIP_ID_SHIFT))

enum xflow_mpu_sw_irq_idx {
	egress_dcore_done_idx_0 = 0,    //  should only be used in our fw.
	dump_xflow_err_status_idx_1,    //  only for debug fw xflow purpose.
	reserve_idx_2,
	reset_device_idx_3,
	fw_cmd_idx_4,
	move_sram_data_to_di_port_idx_5,
	init_wtmem_and_fmem_idx_6,
	reset_mpu_idx_7,
	max_support_mpu_sw_irq_num
};

#define DEVICE_IRQ_COUNT ((reset_mpu_idx_7) - (reserve_idx_2) + 1)

struct memx_pcie_dev;
void memx_xflow_write(struct memx_pcie_dev *memx_dev, u8 chip_id, u32 base_addr, u32 base_addr_offset, u32 value, bool access_mpu);
u32 memx_xflow_read(struct memx_pcie_dev *memx_dev, u8 chip_id, u32 base_addr, u32 base_addr_offset, bool access_mpu);
void memx_sram_write(struct memx_pcie_dev *memx_dev, u32 base_addr, u32 value);
u32 memx_sram_read(struct memx_pcie_dev *memx_dev, u32 base_addr);
s32 memx_xflow_basic_check(struct memx_pcie_dev *memx_dev, u8 chip_id);
#endif
