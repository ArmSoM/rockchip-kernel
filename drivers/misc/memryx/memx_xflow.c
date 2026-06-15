// SPDX-License-Identifier: GPL-2.0+
#include <linux/semaphore.h>
#include "memx_pcie.h"
#include "memx_xflow.h"

s32 memx_xflow_basic_check(struct memx_pcie_dev *memx_dev, u8 chip_id)
{
	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: xflow_basic_check: No Opened Device!\n");
		return -ENODEV;
	}
	if ((memx_dev->xflow_conf_bar_idx == MAX_BAR) || (memx_dev->xflow_vbuf_bar_idx == MAX_BAR)) {
		pr_err("memryx: xflow_basic_check: xflow bar idx(%u)(%u) invalid\n", memx_dev->xflow_conf_bar_idx, memx_dev->xflow_vbuf_bar_idx);
		return -ENODEV;
	}
	if (!memx_dev->bar_info[memx_dev->xflow_conf_bar_idx].iobase ||
		!memx_dev->bar_info[memx_dev->xflow_conf_bar_idx].available) {
		pr_err("memryx: xflow_basic_check: bar_idx(%u) invalid\n", memx_dev->xflow_conf_bar_idx);
		return -ENODEV;
	}
	if (!memx_dev->bar_info[memx_dev->xflow_vbuf_bar_idx].iobase ||
		!memx_dev->bar_info[memx_dev->xflow_vbuf_bar_idx].available) {
		pr_err("memryx: xflow_basic_check: bar_idx(%u) invalid\n", memx_dev->xflow_vbuf_bar_idx);
		return -ENODEV;
	}
	if ((chip_id >= MAX_SUPPORT_CHIP_NUM) ||
		(memx_dev->bar_mode == MEMXBAR_3BAR_BAR0VB_BAR2CI_16MB_BAR4SRAM && chip_id >= 4)) {
		pr_err("memryx: xflow_basic_check: chip_id(%u) invalid\n", chip_id);
		return -ENODEV;
	}
	if (memx_dev->bar_mode == MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM) {
		if (!memx_dev->bar_info[4].iobase || !memx_dev->bar_info[4].available) {
			pr_err("xflow_basic_check: bar_idx(4) invalid.\n");
			return -ENODEV;
		}
	}
	return 0;
}

static void memx_xflow_set_access_mode(struct memx_pcie_dev *memx_dev, u8 chip_id, bool access_mpu)
{
	_VOLATILE_ u32 *control_register_addr = NULL;
	u32 barmapofs = memx_dev->xflow_conf_bar_offset;
	u8 bar_idx = memx_dev->xflow_conf_bar_idx;

	if (memx_xflow_basic_check(memx_dev, chip_id)) {
		pr_err("memryx: xflow_set_base_address: basic check failed\n");
		return;
	}

	if ((chip_id == 0) || (memx_dev->bar_mode != MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM)) {
		control_register_addr = (_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase + GET_XFLOW_OFFSET(chip_id, true) + XFLOW_CONTROL_REGISTER_OFFSET - barmapofs);
		*control_register_addr = access_mpu ? 0 : 1;
	} else {
		_VOLATILE_ u32 *indirect_base_addr_reg_addr = NULL;
		_VOLATILE_ u32 *indirect_control_register_addr = NULL;
		_VOLATILE_ u32 *indirect_virtual_buffer_target_address = NULL;
		u32 vbarmapofs = memx_dev->xflow_vbuf_bar_offset;
		u8 vbar_idx = memx_dev->xflow_vbuf_bar_idx;

		indirect_base_addr_reg_addr = (_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase + GET_XFLOW_OFFSET(0, true) + XFLOW_BASE_ADDRESS_REGISTER_OFFSET - barmapofs);
		indirect_control_register_addr = (_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase + GET_XFLOW_OFFSET(0, true) + XFLOW_CONTROL_REGISTER_OFFSET - barmapofs);
		indirect_virtual_buffer_target_address = (_VOLATILE_ u32 *)(memx_dev->bar_info[vbar_idx].iobase + GET_XFLOW_OFFSET(0, false) - vbarmapofs);

		*indirect_control_register_addr = 1;
		*indirect_base_addr_reg_addr = MXCNST_RP_XFLOW_ADDR + GET_XFLOW_OFFSET(chip_id, true) + XFLOW_CONTROL_REGISTER_OFFSET;
		*indirect_virtual_buffer_target_address = access_mpu ? 0 : 1;
	}
}

static void memx_xflow_set_base_address(struct memx_pcie_dev *memx_dev, u8 chip_id, u32 base_addr)
{
	_VOLATILE_ u32 *base_addr_reg_addr = NULL;
	u32 barmapofs = memx_dev->xflow_conf_bar_offset;
	u8 bar_idx = memx_dev->xflow_conf_bar_idx;

	if (memx_xflow_basic_check(memx_dev, chip_id)) {
		pr_err("memryx: xflow_set_base_address: basic check failed\n");
		return;
	}

	if ((chip_id == 0) || (memx_dev->bar_mode != MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM)) {
		base_addr_reg_addr = (_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase + GET_XFLOW_OFFSET(chip_id, true) + XFLOW_BASE_ADDRESS_REGISTER_OFFSET - barmapofs);
		*base_addr_reg_addr = base_addr;
	} else {
		_VOLATILE_ u32 *indirect_base_addr_reg_addr = NULL;
		_VOLATILE_ u32 *indirect_control_register_addr = NULL;
		_VOLATILE_ u32 *indirect_virtual_buffer_target_address = NULL;
		u32 vbarmapofs = memx_dev->xflow_vbuf_bar_offset;
		u8 vbar_idx = memx_dev->xflow_vbuf_bar_idx;

		indirect_base_addr_reg_addr = (_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase + GET_XFLOW_OFFSET(0, true) + XFLOW_BASE_ADDRESS_REGISTER_OFFSET - barmapofs);
		indirect_control_register_addr = (_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase + GET_XFLOW_OFFSET(0, true) + XFLOW_CONTROL_REGISTER_OFFSET - barmapofs);
		indirect_virtual_buffer_target_address = (_VOLATILE_ u32 *)(memx_dev->bar_info[vbar_idx].iobase + GET_XFLOW_OFFSET(0, false) - vbarmapofs);

		*indirect_control_register_addr = 1;
		*indirect_base_addr_reg_addr = MXCNST_RP_XFLOW_ADDR + GET_XFLOW_OFFSET(chip_id, true) + XFLOW_BASE_ADDRESS_REGISTER_OFFSET;
		*indirect_virtual_buffer_target_address = base_addr;
	}
}

static void memx_xflow_write_virtual_buffer_address(struct memx_pcie_dev *memx_dev, u8 chip_id, u32 base_addr_offset, u32 value)
{
	_VOLATILE_ u32 *virtual_buffer_target_address = NULL;
	u32 barmapofs = memx_dev->xflow_vbuf_bar_offset;
	u8 bar_idx = memx_dev->xflow_vbuf_bar_idx;

	if (memx_xflow_basic_check(memx_dev, chip_id)) {
		pr_err("memryx: xflow_write_virtual_buffer: basic check failed\n");
		return;
	}

	if ((chip_id == 0) || (memx_dev->bar_mode != MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM)) {
		virtual_buffer_target_address = (_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase + GET_XFLOW_OFFSET(chip_id, false) + base_addr_offset - barmapofs);
		*virtual_buffer_target_address = value;
	} else {
		_VOLATILE_ u32 *indirect_base_addr_reg_addr = NULL;
		_VOLATILE_ u32 *indirect_control_register_addr = NULL;
		_VOLATILE_ u32 *indirect_virtual_buffer_target_address = NULL;
		u32 cbarmapofs = memx_dev->xflow_conf_bar_offset;
		u8 cbar_idx = memx_dev->xflow_conf_bar_idx;

		indirect_base_addr_reg_addr = (_VOLATILE_ u32 *)(memx_dev->bar_info[cbar_idx].iobase + GET_XFLOW_OFFSET(0, true) + XFLOW_BASE_ADDRESS_REGISTER_OFFSET - cbarmapofs);
		indirect_control_register_addr = (_VOLATILE_ u32 *)(memx_dev->bar_info[cbar_idx].iobase + GET_XFLOW_OFFSET(0, true) + XFLOW_CONTROL_REGISTER_OFFSET - cbarmapofs);
		indirect_virtual_buffer_target_address = (_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase + GET_XFLOW_OFFSET(0, false) + base_addr_offset - barmapofs);

		*indirect_control_register_addr = 1;
		*indirect_base_addr_reg_addr = MXCNST_RP_XFLOW_ADDR + GET_XFLOW_OFFSET(chip_id, false);
		*indirect_virtual_buffer_target_address = value;
	}

}

static u32 memx_xflow_read_virtual_buffer_address(struct memx_pcie_dev *memx_dev, u8 chip_id, u32 base_addr_offset)
{
	u32 result = 0;
	_VOLATILE_ u32 *virtual_buffer_target_address = NULL;
	u32 barmapofs = memx_dev->xflow_vbuf_bar_offset;
	u8 bar_idx = memx_dev->xflow_vbuf_bar_idx;

	if (memx_xflow_basic_check(memx_dev, chip_id)) {
		pr_err("memryx: xflow_read_virtual_buffer: basic check failed\n");
		return 0;
	}

	if ((chip_id == 0) || (memx_dev->bar_mode != MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM)) {
		virtual_buffer_target_address = (_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase + GET_XFLOW_OFFSET(chip_id, false) + base_addr_offset - barmapofs);
		result = *virtual_buffer_target_address;
	} else {
		_VOLATILE_ u32 *indirect_base_addr_reg_addr = NULL;
		_VOLATILE_ u32 *indirect_control_register_addr = NULL;
		_VOLATILE_ u32 *indirect_virtual_buffer_target_address = NULL;
		u32 cbarmapofs = memx_dev->xflow_conf_bar_offset;
		u8 cbar_idx = memx_dev->xflow_conf_bar_idx;

		indirect_base_addr_reg_addr = (_VOLATILE_ u32 *)(memx_dev->bar_info[cbar_idx].iobase + GET_XFLOW_OFFSET(0, true) + XFLOW_BASE_ADDRESS_REGISTER_OFFSET - cbarmapofs);
		indirect_control_register_addr = (_VOLATILE_ u32 *)(memx_dev->bar_info[cbar_idx].iobase + GET_XFLOW_OFFSET(0, true) + XFLOW_CONTROL_REGISTER_OFFSET - cbarmapofs);
		indirect_virtual_buffer_target_address = (_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase + GET_XFLOW_OFFSET(0, false) + base_addr_offset - barmapofs);

		*indirect_control_register_addr = 1;
		*indirect_base_addr_reg_addr = MXCNST_RP_XFLOW_ADDR + GET_XFLOW_OFFSET(chip_id, false);
		result = *indirect_virtual_buffer_target_address;
	}
	return result;
}

void memx_xflow_write(struct memx_pcie_dev *memx_dev, u8 chip_id, u32 base_addr, u32 base_addr_offset, u32 value, bool access_mpu)
{
	if (memx_xflow_basic_check(memx_dev, chip_id)) {
		pr_err("memryx: xflow_write: basic check failed\n");
		return;
	}

	if (memx_dev->bar_mode == MEMXBAR_SRAM1MB) {
		unsigned long timeout = jiffies + (HZ);

		while (memx_sram_read(memx_dev, MEMX_EXTINFO_CMD_BASE) != MEMX_EXTCMD_COMPLETE) {
			if (time_after(jiffies, timeout)) {
				pr_err("memryx: ERROR: %s cmd timeout\n", __func__);
				return;
			}
			// speed up the hotplug
			if (memx_sram_read(memx_dev, MEMX_EXTINFO_CMD_BASE) == 0xFFFFFFFF)
				return;
		}

		memx_sram_write(memx_dev, MEMX_EXTINFO_DATA_BASE, (base_addr + base_addr_offset));
		memx_sram_write(memx_dev, MEMX_EXTINFO_DATA_BASE + 4, value);
		memx_sram_write(memx_dev, MEMX_EXTINFO_CMD_BASE, MEMX_EXTINFO_CMD(chip_id, MEMX_EXTCMD_XFLOW_WRITE_REG, (access_mpu ? 0 : 1)));

		while (memx_sram_read(memx_dev, MEMX_EXTINFO_CMD_BASE) != MEMX_EXTCMD_COMPLETE) {
			if (time_after(jiffies, timeout)) {
				pr_err("memryx: ERROR: %s wait cmd complete timeout\n", __func__);
				return;
			}
			// speed up the hotplug
			if (memx_sram_read(memx_dev, MEMX_EXTINFO_CMD_BASE) == 0xFFFFFFFF)
				return;
		}
	} else {
		memx_xflow_set_access_mode(memx_dev, chip_id, access_mpu);
		memx_xflow_set_base_address(memx_dev, chip_id, base_addr);
		memx_xflow_write_virtual_buffer_address(memx_dev, chip_id, base_addr_offset, value);
		if (!access_mpu)
			memx_xflow_set_access_mode(memx_dev, chip_id, true);

		if (((memx_dev->bar_mode == MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM)) && (chip_id > 0))
			memx_xflow_set_access_mode(memx_dev, 0, true);
	}
}

u32 memx_xflow_read(struct memx_pcie_dev *memx_dev, u8 chip_id, u32 base_addr, u32 base_addr_offset, bool access_mpu)
{
	u32 result = 0;

	if (memx_xflow_basic_check(memx_dev, chip_id)) {
		pr_err("memryx: xflow_read: get memx_dev->mutex failed\n");
		return 0;
	}

	if (memx_dev->bar_mode == MEMXBAR_SRAM1MB) {
		unsigned long timeout = jiffies + (HZ);

		while (memx_sram_read(memx_dev, MEMX_EXTINFO_CMD_BASE) != MEMX_EXTCMD_COMPLETE) {
			if (time_after(jiffies, timeout)) {
				pr_err("memryx: ERROR: %s cmd timeout 1\n", __func__);
				return 0;
			}
		}

		memx_sram_write(memx_dev, MEMX_EXTINFO_DATA_BASE, (base_addr+base_addr_offset));
		memx_sram_write(memx_dev, MEMX_EXTINFO_CMD_BASE, MEMX_EXTINFO_CMD(chip_id, MEMX_EXTCMD_XFLOW_READ_REG, (access_mpu ? 0 : 1)));

		timeout = jiffies + (HZ);
		while (memx_sram_read(memx_dev, MEMX_EXTINFO_CMD_BASE) != MEMX_EXTCMD_COMPLETE) {
			if (time_after(jiffies, timeout)) {
				pr_err("memryx: ERROR: %s cmd timeout 2\n", __func__);
				return 0;
			}
		}
		result = memx_sram_read(memx_dev, MEMX_EXTINFO_DATA_BASE + 4);
	} else {
		memx_xflow_set_access_mode(memx_dev, chip_id, access_mpu);
		memx_xflow_set_base_address(memx_dev, chip_id, base_addr);
		result = memx_xflow_read_virtual_buffer_address(memx_dev, chip_id, base_addr_offset);
		if (!access_mpu)
			memx_xflow_set_access_mode(memx_dev, chip_id, true);

		if (((memx_dev->bar_mode == MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM)) && (chip_id > 0))
			memx_xflow_set_access_mode(memx_dev, 0, true);
	}
	return result;
}

void memx_sram_write(struct memx_pcie_dev *memx_dev, u32 base_addr, u32 value)
{
	u8 bar_idx = memx_dev->sram_bar_idx;

	if (bar_idx == MAX_BAR) {
		pr_err("memryx: %s: Invalid bar_idx!\n", __func__);
		return;
	}
	if ((base_addr < (MEMX_CHIP_SRAM_BASE + MEMX_CHIP_SRAM_DATA_SRAM_OFFS)) || (base_addr >= (MEMX_CHIP_SRAM_BASE + MEMX_CHIP_SRAM_MAX_SIZE))) {
		pr_err("memryx: %s: Invalid base_addr!\n", __func__);
		return;
	}
	base_addr = base_addr - MEMX_CHIP_SRAM_BASE;

	*((_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase+base_addr)) = value;
}

u32 memx_sram_read(struct memx_pcie_dev *memx_dev, u32 base_addr)
{
	u8 bar_idx = memx_dev->sram_bar_idx;

	if (bar_idx == MAX_BAR) {
		pr_err("memryx: %s: Invalid bar_idx!\n", __func__);
		return 0;
	}
	if ((base_addr < (MEMX_CHIP_SRAM_BASE + MEMX_CHIP_SRAM_DATA_SRAM_OFFS)) || (base_addr >= (MEMX_CHIP_SRAM_BASE + MEMX_CHIP_SRAM_MAX_SIZE))) {
		pr_err("memryx: %s: Invalid base_addr!\n", __func__);
		return 0;
	}
	base_addr = base_addr - MEMX_CHIP_SRAM_BASE;

	return *((_VOLATILE_ u32 *)(memx_dev->bar_info[bar_idx].iobase+base_addr));
}
