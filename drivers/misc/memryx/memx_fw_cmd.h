/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _MEMX_FIRMWARE_COMMAND_H_
#define _MEMX_FIRMWARE_COMMAND_H_

#define FIRMWARE_CMD_DATA_DWORD_COUNT (63)

// Note: The whole fw cmd format is 256 bytes.
struct pcie_fw_cmd_format {
	u16 firmware_command;
	u16 expected_data_length;
	u32 data[FIRMWARE_CMD_DATA_DWORD_COUNT]; // for now, data area almost writed by mpu pcie firmware.
};

struct memx_pcie_dev;
struct pcie_fw_cmd_format *memx_send_cmd_to_fw_and_get_result(struct memx_pcie_dev *memx_dev, enum PCIE_FW_CMD_ID op_code, u16 expected_payload_length, u8 chip_id);
#endif
