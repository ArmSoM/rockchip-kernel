// SPDX-License-Identifier: GPL-2.0+
#include "memx_xflow.h"
#include "memx_pcie.h"
#include "memx_fw_cmd.h"
#define FAIL_ACK_COUNT 5
static struct semaphore g_memx_fw_cmd_mutex = __SEMAPHORE_INITIALIZER(g_memx_fw_cmd_mutex, 1);
static int memx_wait_for_firmware_msix_ack(struct memx_pcie_dev *memx_dev);
static int memx_wait_for_firmware_msix_ack(struct memx_pcie_dev *memx_dev)
{
	s32 wq_status = 0;
	int fail_count = 0;

	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: wait_for_fw_ack: failed with -ENODEV\n");
		return -ENODEV;
	}
	// check until received cmd process done msix
	do {
		wq_status = wait_event_interruptible_timeout(memx_dev->mpu_data.fw_ctrl.wq, memx_dev->mpu_data.fw_ctrl.indicator != -1, msecs_to_jiffies(1000));
		if (memx_dev->mpu_data.fw_ctrl.is_abort) {
			wq_status = -ERESTARTSYS;
			memx_dev->mpu_data.fw_ctrl.is_abort = 0;
			return wq_status;
		}
		if (wq_status == -ERESTARTSYS) {
			pr_err("memryx: wait_for_fw_ack: cancelled by interrupt\n");
			return wq_status;
		}
		if (wq_status < 1) {
			fail_count++;
			pr_err("memryx: wait_for_fw_ack: wait timeout 1(s), retrying again\n");
		}

	} while ((wq_status < 1) && (fail_count < FAIL_ACK_COUNT));

	if (wq_status >= 1) {
#ifdef DEBUG
		pr_info("memryx: wait_for_fw_ack: received ack notification from msix isr(%d)\n", memx_dev->mpu_data.fw_ctrl.indicator);
#endif
		spin_lock(&memx_dev->mpu_data.fw_ctrl.lock);
		memx_dev->mpu_data.fw_ctrl.indicator = -1;
		spin_unlock(&memx_dev->mpu_data.fw_ctrl.lock);
#ifdef DEBUG
		pr_info("memryx: wait_for_fw_ack:: success.\n");
#endif
		return 0;

	} else {
		pr_err("memryx: wait_for_fw_ack: failed!\n");
		return -ENODEV;
	}
}

static s32 memx_send_command_to_firmware(struct memx_pcie_dev *memx_dev, enum PCIE_FW_CMD_ID op_code, u16 expected_payload_length, u8 chip_id)
{
	struct pcie_fw_cmd_format *fw_cmd_buffer = NULL;
	u8 chip_idx = 0;

	if (!memx_dev || !memx_dev->mpu_data.mmap_fw_cmd_buffer_base) {
		pr_err("memryx: invalid mmap_host_fw_command_event_base\n");
		return -1;
	}
	if (op_code == PCIE_CMD_WAIT_FOR_ACK_ONLY)
		return 0;

	if (expected_payload_length == 0) {
		pr_err("memryx: invaild expected length(%d)\n", expected_payload_length);
		return -2;
	}

	fw_cmd_buffer = (struct pcie_fw_cmd_format *)memx_dev->mpu_data.mmap_fw_cmd_buffer_base;
	fw_cmd_buffer->firmware_command = op_code;
	fw_cmd_buffer->expected_data_length = expected_payload_length;

	switch (op_code) {
	case PCIE_CMD_INIT_HOST_BUF_MAPPING: {
		fw_cmd_buffer->data[0] = memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base & 0xffffffff;
		fw_cmd_buffer->data[1] = memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base >> 32;
		fw_cmd_buffer->data[2] = DMA_COHERENT_BUFFER_SIZE_2MB;
	}
	break;
	case PCIE_CMD_CONFIG_MPU_GROUP: {
		for (chip_idx = 0; chip_idx < MAX_CHIP_NUM; chip_idx++)
			fw_cmd_buffer->data[chip_idx] = memx_dev->mpu_data.hw_info.chip.roles[chip_idx];
	}
	break;
	case PCIE_CMD_INIT_WTMEM_FMAP: {
		fw_cmd_buffer->data[0] = chip_id;
	}
	break;
	default:
		// don't need to modify data area.
		break;
	}

	// trigger mpu sw irq 4 to request mpu to process firmware command which host already put in command event buffer and
	// when fw write back to the same buffer in data area as event result, firmware will issue a msix to notify udriver the cmd process done.
	memx_pcie_trigger_device_irq(memx_dev, 0, fw_cmd_idx_4);
#ifdef DEBUG
	pr_info("memryx: send cmd[%u] with expected_len[%u] to fw\n", op_code, expected_payload_length);
#endif
	return 0;
}

static struct pcie_fw_cmd_format *memx_get_firmware_command_result(struct memx_pcie_dev *memx_dev)
{
	struct pcie_fw_cmd_format *firmware_command_result_buffer = NULL;

	if (!memx_dev || !memx_dev->mpu_data.mmap_fw_cmd_buffer_base) {
		pr_err("memryx: invalid context\n");
		return NULL;
	}
	firmware_command_result_buffer = (struct pcie_fw_cmd_format *)memx_dev->mpu_data.mmap_fw_cmd_buffer_base;
	return firmware_command_result_buffer;
}

struct pcie_fw_cmd_format *memx_send_cmd_to_fw_and_get_result(struct memx_pcie_dev *memx_dev, enum PCIE_FW_CMD_ID op_code, u16 expected_payload_length, u8 chip_id)
{
	struct pcie_fw_cmd_format *firmware_command_result_buffer = NULL;

	if (!memx_dev || !memx_dev->mpu_data.mmap_fw_cmd_buffer_base) {
		pr_err("memryx: invalid mmap_host_fw_command_event_base\n");
		return NULL;
	}
	down(&g_memx_fw_cmd_mutex);
	if (memx_send_command_to_firmware(memx_dev, op_code, expected_payload_length, chip_id)) {
		up(&g_memx_fw_cmd_mutex);
		return NULL;
	}

	if (memx_wait_for_firmware_msix_ack(memx_dev)) {
		up(&g_memx_fw_cmd_mutex);
		return NULL;
	}

	firmware_command_result_buffer = memx_get_firmware_command_result(memx_dev);
	up(&g_memx_fw_cmd_mutex);
	return firmware_command_result_buffer;
}
