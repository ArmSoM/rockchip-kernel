// SPDX-License-Identifier: GPL-2.0+
#include <linux/firmware.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include "memx_xflow.h"
#include "memx_pcie.h"
#include "memx_fw_cmd.h"
#include "memx_ioctl.h"
#include "memx_fw_init.h"

extern u32 mxmf_boot_tick;
#define FW_INIT_TIMEOUT_MSEC	10000
static s32 memx_download_firmware_to_sram_code_section(struct memx_pcie_dev *memx_dev, struct memx_firmware_bin *memx_bin)
{
	const struct firmware *firmware = NULL;
	u32 firmware_size = 0;
	u8 *firmware_buffer_pos = NULL;
	u32 epram = 0;
	u32 loop_count = 0;
	u32 offset = 0;
	u32 epsts = 0;
	unsigned long timeout = 0;
	u8 ImgFmt = 0;

	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: download_fw: invalid memx_dev\n");
		return -ENODEV;
	}

	// check if firmware is already downloaded
	epram = memx_sram_read(memx_dev, MXCNST_FW_START_BASE);
#ifdef DEBUG
	pr_info("memryx: download_fw: Read EPRAM = 0x%08x\n", epram);
#endif

	// firmware is already downloaded, skip loading from file system
	if (epram != MXCNST_FW_ZSBL_INITVAL) {
		pr_info("memryx: download_fw: FW image already existed\n");
		return 1;
	}

	// there is no firmware, try to load from file system
	if (!memx_bin) {
		pr_err("memryx: download_fw: invalid memx_bin\n");
		return -ENODEV;
	}
	if (memx_bin->request_firmware_update_in_linux) {
		if (request_firmware(&firmware, memx_bin->name, &memx_dev->pDev->dev) < 0) {
			pr_err("memryx: download_fw: request_firmware for %s failed\n", memx_bin->name);
			return -ENODEV;
		}
		firmware_buffer_pos = (u8 *)firmware->data;
		firmware_size = firmware->size;
	} else {
		if (!memx_bin->buffer) {
			pr_err("memryx: download_fw: memx_bin->buffer is NULL\n");
			return -ENODEV;
		}
		if (memx_bin->size == 0) {
			pr_err("memryx: download_fw: invalid memx_bin->size\n");
			return -ENODEV;
		}
		firmware_buffer_pos = kmalloc(memx_bin->size, GFP_KERNEL);
		if (!firmware_buffer_pos) {
			//pr_err("memryx: kmalloc for firmware failed\n");
			return -ENOMEM;
		}
		if (copy_from_user(firmware_buffer_pos, memx_bin->buffer, memx_bin->size)) {
			kfree(firmware_buffer_pos);
			pr_err("memryx: cascade.bin request failed\n");
			return -ENOMEM;
		}
		firmware_size = firmware->size - 4;
	}

	if (firmware_size >= 0x7004)
		ImgFmt = *((u32 *)(firmware_buffer_pos + 0x6F08));

	if (ImgFmt == 1)
		memx_sram_write(memx_dev, MXCNST_FW_START_BASE, firmware_size+8);
	else
		memx_sram_write(memx_dev, MXCNST_FW_START_BASE, *((u32 *)firmware_buffer_pos));


	// start to download firmware from file system
	timeout = jiffies + 5 * HZ;

	epram = memx_sram_read(memx_dev, MXCNST_FW_START_BASE);
#ifdef DEBUG
	pr_info("memryx: download_fw: Size value written in EP 0x%x\n", epram);
	pr_info("memryx: download_fw: Waiting for size value to be cleared by EP...\n");
#endif
	while (epram != 0) {
		epram = memx_sram_read(memx_dev, MXCNST_FW_START_BASE);
		if (epram == MXCNST_FW_ZSBL_INITVAL)
			memx_sram_write(memx_dev, MXCNST_FW_START_BASE, *((u32 *)firmware_buffer_pos));

		epsts =  memx_xflow_read(memx_dev, 0, MXCNST_PCIE_LOCINTR, 0, false);
		if (epsts != 0) {
			u32 i;

			for (i = 0; i < 64; i++)
				memx_xflow_write(memx_dev, 0, MXCNST_PCIE_LOCINTR, 0, epsts, false);

			pr_info("memryx: epsts 0x%08X 0x%08X\n", epram, epsts);
		}

		if (time_after(jiffies, timeout)) {
			pr_err("memryx: download_fw: timeout\n");
			if (memx_bin->request_firmware_update_in_linux)
				release_firmware(firmware);
			else
				kfree(firmware_buffer_pos);

			return -ENODEV;
		}
	}

	firmware_buffer_pos += 4;
	loop_count = (firmware_size) / 4;
	for (offset = 0; offset < loop_count; offset++) {
		u32 curr_offset = offset * 4;
		u32 curr_value = *((u32 *)(firmware_buffer_pos + curr_offset));

		memx_sram_write(memx_dev, MXCNST_FW_START_BASE + curr_offset, curr_value);
	}

	if (ImgFmt == 1) {
		memx_sram_write(memx_dev, MXCNST_FW_START_BASE + firmware_size, *((u32 *)(firmware_buffer_pos-4)));
		memx_sram_write(memx_dev, MXCNST_FW_START_BASE + firmware_size + 4, 1);
	}


#ifdef DEBUG
	pr_info("memryx: download_fw: FW image written in EP!\n");
#endif

	pr_info("memryx: download_fw: success\n");
	if (memx_bin->request_firmware_update_in_linux)
		release_firmware(firmware);
	else
		kfree(firmware_buffer_pos);



	return 0;
}

#ifdef DEBUG
static const char *get_chip_role_from_enum(enum memx_chip_role role)
{
	switch (role) {
	case ROLE_SINGLE:
		return "ROLE_SINGLE";
	case ROLE_MULTI_FIRST:
		return "ROLE_MULTI_FIRST";
	case ROLE_MULTI_LAST:
		return "ROLE_MULTI_LAST";
	case ROLE_MULTI_MIDDLE:
		return "ROLE_MULTI_MIDDLE";
	default:
		return "ROLE_UNCONFIGURED";
	}
}

static void dump_firmware_info(struct memx_pcie_dev *memx_dev)
{
	s32 idx = 0;

	if (!memx_dev)
		return;

	pr_info("memryx: firmware_download_sram_base: 0x%x\n", memx_dev->mpu_data.hw_info.fw.firmware_download_sram_base);
	pr_info("memryx: firmware_command_sram_base: 0x%x\n", memx_dev->mpu_data.hw_info.fw.firmware_command_sram_base);
	for (idx = 0; idx < memx_dev->mpu_data.hw_info.chip.total_chip_cnt; idx++)
		pr_info("memryx: chip_idx[%d] = %s\n", idx, get_chip_role_from_enum(memx_dev->mpu_data.hw_info.chip.roles[idx]));

	for (idx = 0; idx < memx_dev->mpu_data.hw_info.chip.total_chip_cnt; idx++)
		pr_info("memryx: ingress_dcore_mapping_sram_base[%d] = 0x%x\n", idx, memx_dev->mpu_data.hw_info.fw.ingress_dcore_mapping_sram_base[idx]);

	for (idx = 0; idx < memx_dev->mpu_data.hw_info.chip.total_chip_cnt; idx++)
		pr_info("memryx: egress_dcore_mapping_sram_base[%d] = 0x%x\n", idx, memx_dev->mpu_data.hw_info.fw.egress_dcore_mapping_sram_base[idx]);

	for (idx = 0; idx < memx_dev->mpu_data.hw_info.chip.total_chip_cnt; idx++)
		pr_info("memryx: egress_dcore_dma_destination_buffer_base[%d] = 0x%x\n", idx, memx_dev->mpu_data.hw_info.fw.egress_dcore_rx_dma_buffer_offset[idx]);

}
#endif
s32 memx_init_chip_info(struct memx_pcie_dev *memx_dev);
s32 memx_init_chip_info(struct memx_pcie_dev *memx_dev)
{
	u8 curr_mpu_group_id = 0;
	u8 curr_chip_count = 0;
	u8 chip_id = 0;

	if (!memx_dev) {
		pr_err("memryx: init_chip_info probing: failed\n");
		return -1;
	}

	for (chip_id = 0; chip_id < MAX_SUPPORT_CHIP_NUM; chip_id++) {
		switch (memx_dev->mpu_data.hw_info.chip.roles[chip_id]) {
		case ROLE_SINGLE: {
			memx_dev->mpu_data.hw_info.chip.groups[curr_mpu_group_id].input_chip_id = chip_id;
			memx_dev->mpu_data.hw_info.chip.groups[curr_mpu_group_id].output_chip_id = chip_id;
			curr_mpu_group_id++;
			curr_chip_count++;
		} break;
		case ROLE_MULTI_FIRST: {
			memx_dev->mpu_data.hw_info.chip.groups[curr_mpu_group_id].input_chip_id = chip_id;
			curr_chip_count++;
		} break;
		case ROLE_MULTI_LAST: {
			memx_dev->mpu_data.hw_info.chip.groups[curr_mpu_group_id].output_chip_id = chip_id;
			curr_mpu_group_id++;
			curr_chip_count++;
		} break;
		case ROLE_MULTI_MIDDLE: {
			curr_chip_count++;
		} break;
		default:
			// The first unknow ROLE_UNCONFIGURED which means all chip already scan finsh! we can just break the loop early.
			break;
		}
	}
	memx_dev->mpu_data.hw_info.chip.group_count = curr_mpu_group_id;
	memx_dev->mpu_data.hw_info.chip.curr_config_chip_count = curr_chip_count;

#ifdef DEBUG
	pr_info("memryx: Init Chip Info Success:\n");
	pr_info("memryx: Total mpu group count %d\n", memx_dev->mpu_data.hw_info.chip.group_count);
	pr_info("memryx: Total chip count %d\n", memx_dev->mpu_data.hw_info.chip.total_chip_cnt);
	pr_info("memryx: Current config chip count %d\n", memx_dev->mpu_data.hw_info.chip.curr_config_chip_count);
#endif
	return 0;
}

s32 memx_get_hw_info(struct memx_pcie_dev *memx_dev)
{
	u8 chip_id = 0;
	struct pcie_fw_cmd_format *fw_cmd_result = NULL;
	struct fw_hw_info_pkt *hw_info = NULL;

	if (!memx_dev) {
		pr_err("memryx: memx_update_hw_info: NULL pointer\n");
		return -1;
	}

	fw_cmd_result = memx_send_cmd_to_fw_and_get_result(memx_dev, PCIE_CMD_GET_HW_INFO, 256, CHIP_ID0);
	if (fw_cmd_result == NULL) {
		pr_err("memryx: memx_firmware_init: get hardware info from fw failed\n");
		return -1;
	}

	// parsing hardware info packet
	hw_info = (struct fw_hw_info_pkt *) (fw_cmd_result->data);
	memx_dev->mpu_data.hw_info.chip.total_chip_cnt = hw_info->total_chip_cnt;
	memx_dev->mpu_data.hw_info.chip.generation = hw_info->chip_generation;
	for (chip_id = 0; chip_id < MAX_SUPPORT_CHIP_NUM; chip_id++) {
		memx_dev->mpu_data.hw_info.chip.roles[chip_id] = hw_info->chip_role[chip_id];
		memx_dev->mpu_data.hw_info.fw.ingress_dcore_mapping_sram_base[chip_id] = hw_info->igr_buf_sram_base_addr;
		memx_dev->mpu_data.hw_info.fw.egress_dcore_mapping_sram_base[chip_id] = hw_info->egr_pbuf_sram_base_addr[chip_id];
		memx_dev->mpu_data.hw_info.fw.egress_dcore_rx_dma_buffer_offset[chip_id] = hw_info->egr_dst_buf_start_addr[chip_id];
#ifdef DEBUG
		if (memx_dev->mpu_data.hw_info.chip.roles[chip_id] != ROLE_UNCONFIGURED) {
			pr_info("memryx: memx_dev->mpu_data.hw_info.chip.roles[%d]: %d\n", chip_id, memx_dev->mpu_data.hw_info.chip.roles[chip_id]);
			pr_info("memryx: memx_dev->mpu_data.hw_info.fw.ingress_dcore_mapping_sram_base: %d\n", memx_dev->mpu_data.hw_info.fw.ingress_dcore_mapping_sram_base[0]);
			pr_info("memryx: memx_dev->mpu_data.hw_info.fw.egress_dcore_mapping_sram_base[%d]: %d\n", chip_id, memx_dev->mpu_data.hw_info.fw.egress_dcore_mapping_sram_base[chip_id]);
			pr_info("memryx: memx_dev->mpu_data.hw_info.fw.egress_dcore_rx_dma_buffer_offset[%d]: %d\n", chip_id, memx_dev->mpu_data.hw_info.fw.egress_dcore_rx_dma_buffer_offset[chip_id]);
		}
#endif
	}

	if (memx_init_chip_info(memx_dev)) {
		pr_err("memryx: probing: memx_init_chip_info failed\n");
		return -44;
	}
	memx_dev->mpu_data.hw_info.chip.pcie_bar_mode = memx_dev->bar_mode;

	return 0;
}
s32 memx_firmware_init(struct memx_pcie_dev *memx_dev, struct memx_firmware_bin *memx_bin);
s32 memx_firmware_init(struct memx_pcie_dev *memx_dev, struct memx_firmware_bin *memx_bin)
{
	s32 ret = 0;

	ret = memx_init_msix_irq(memx_dev);
	if (ret) {
		pr_err("memryx: firmware_init probing: msix setup failed(%d)\n", ret);
		return ret;
	}

	ret = memx_download_firmware_to_sram_code_section(memx_dev, memx_bin);
	if (ret < 0) {
		pr_err("memryx: firmware_init probing: download firmware image failed\n");
		return ret;
	}

	// wait for chip boot complete ack only when we first download firmware bin file.
	if (ret == 0) // PCIe boot
		memx_send_cmd_to_fw_and_get_result(memx_dev, PCIE_CMD_WAIT_FOR_ACK_ONLY, 0, CHIP_ID0);
	else if (ret == 1) { // QSPI boot
		unsigned long timeout = jiffies + msecs_to_jiffies(FW_INIT_TIMEOUT_MSEC);
		u32 sleep_ms = 100;
		u32 elapsed_ms = 0;
		u32 tick0 = memx_xflow_read(memx_dev, 0, MXCNST_FW_SYS_TICK, 0, false);

		while (!time_after(jiffies, timeout)) {
			u32 tick1 = memx_xflow_read(memx_dev, 0, MXCNST_FW_SYS_TICK, 0, false);

			if (tick0 > tick1)
				tick0 = tick1;
			else if ((tick1 > mxmf_boot_tick) && (tick1 > tick0)) {
				pr_info("memryx: firmware_init probing: fw systick = %u > %u (boot successful within %d msec)\n", tick1, mxmf_boot_tick, elapsed_ms);
				break;
			}
			pr_info("memryx: firmware_init probing: fw systick = %u -> %u < %u (%d msec elapsed)\n", tick0, tick1, mxmf_boot_tick, elapsed_ms);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(msecs_to_jiffies(sleep_ms));
			elapsed_ms += sleep_ms;
		}
	}

	// provide dvfs info change buffer for chips communications
	memx_sram_write(memx_dev, (MEMX_DBGLOG_CONTROL_BASE+MEMX_DVFS_MPU_UTI_ADDR), MEMX_GET_DVFS_UTIL_BUS_ADDR);

	memx_send_cmd_to_fw_and_get_result(memx_dev, PCIE_CMD_INIT_HOST_BUF_MAPPING, 8, CHIP_ID0);
	ret = memx_get_hw_info(memx_dev);
	if (ret) {
		pr_err("memryx: firmware_init probing: get hardware info failed\n");
		return ret;
	}

#ifdef DEBUG
	dump_firmware_info(memx_dev);
#endif
	return ret;
}
