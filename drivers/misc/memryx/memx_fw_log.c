// SPDX-License-Identifier: GPL-2.0+
#include "memx_pcie.h"
#include "memx_xflow.h"
#include "memx_fw_log.h"

s32 memx_fw_log_dump(struct memx_pcie_dev *memx_dev, u8 chip_id)
{
	u8 *log_buf = NULL;	/* this is circular buffer */
	u8 *log_buf_seq = NULL;/* linear arranged */
	u32 *wp_addr = NULL, *rp_addr = NULL;
	u32 wp_value = 0, rp_value = 0;
	s32 size = 0;
	s32 total = 0, total_tmp = 0;

	if (!memx_dev) {
		pr_err("memryx: fw_log_dump: memx_dev is NULL\n");
		return -EINVAL;
	}

	if (chip_id >= memx_dev->mpu_data.hw_info.chip.total_chip_cnt) {
		pr_err("memryx: %s: invalid mpu chip id(%u), should be 0 to (%u - 1)\n", __func__, chip_id, memx_dev->mpu_data.hw_info.chip.total_chip_cnt);
		return -EINVAL;
	}

	log_buf = (u8 *)(MEMX_GET_CHIP_DBGLOG_BUFFER_VIRTUAl_ADDR(memx_dev, chip_id));
	if (!log_buf) {
		pr_err("memryx: memx_fw_log_ctrl: Get chip_id(%u) log_buf virtual Address fail!\n", chip_id);
		return -EINVAL;
	}
	wp_addr = (u32 *)(MEMX_GET_CHIP_DBGLOG_WRITER_PTR_VIRTUAl_ADDR(memx_dev, chip_id));
	if (!wp_addr) {
		pr_err("memryx: memx_fw_log_ctrl: Get chip_id(%u) wp_addr virtual Address fail!\n", chip_id);
		return -EINVAL;
	}
	rp_addr = (u32 *)(MEMX_GET_CHIP_DBGLOG_READ_PTR_VIRTUAl_ADDR(memx_dev, chip_id));
	if (!rp_addr) {
		pr_err("memryx: %s: Get chip_id(%u) rp_addr virtual Address fail!\n", __func__, chip_id);
		return -EINVAL;
	}

	wp_value = *wp_addr;
	rp_value = *rp_addr;

	total = (s32)((wp_value - rp_value) & (MEMX_DBGLOG_CHIP_BUFFER_SIZE(chip_id) - 1));
	log_buf_seq = kmalloc(total+1, GFP_KERNEL);
	if (!log_buf_seq) {
		pr_err("memryx: kmalloc for log_buf_seq failed\n");
		return -ENOMEM;
	}

	if (rp_value+total < MEMX_DBGLOG_CHIP_BUFFER_SIZE(chip_id)) {
		memcpy(log_buf_seq, log_buf + rp_value, total);
	} else {
		u32 slice1 = MEMX_DBGLOG_CHIP_BUFFER_SIZE(chip_id) - rp_value;
		u32 slice2 = total - slice1;

		memcpy(log_buf_seq, log_buf + rp_value, slice1);
		memcpy(log_buf_seq+slice1, log_buf, slice2);
	}
	log_buf_seq[total] = 0;

	pr_err("=============CHIP(%u) WP(%d) RP(%d) Total(%d)=============\n", chip_id, wp_value, rp_value, total);
	rp_value = 0;
	total_tmp = total;
	while (total_tmp) {
		size = pr_err("%s", log_buf_seq + rp_value);
		total_tmp -= (size + 2);
		rp_value = ((rp_value + (size + 2)));
		if (rp_value >= total)
			break;
	}
	*rp_addr = wp_value;
	pr_err("============================================================\n");
	kfree(log_buf_seq);
	return 0;
}

s32 memx_fw_log_init(struct memx_pcie_dev *memx_dev)
{
	u8 chip_id = 0, maxcnt;

	if (!memx_dev) {
		pr_err("memryx: fw debug log init: memx_dev is NULL\n");
		return -EINVAL;
	}

	maxcnt = memx_dev->mpu_data.hw_info.chip.total_chip_cnt;

	// Connect chip dram buffer to driver
	for (chip_id = CHIP_ID0; chip_id < maxcnt; chip_id++) {
		memx_dev->mpu_data.fw_log.write_ptr[chip_id] = (u32 *)(MEMX_GET_CHIP_DBGLOG_WRITER_PTR_VIRTUAl_ADDR(memx_dev, chip_id));
		memx_dev->mpu_data.fw_log.read_ptr[chip_id] = (u32 *)(MEMX_GET_CHIP_DBGLOG_READ_PTR_VIRTUAl_ADDR(memx_dev, chip_id));
		*memx_dev->mpu_data.fw_log.write_ptr[chip_id] = 0;
		*memx_dev->mpu_data.fw_log.read_ptr[chip_id] = 0;

		// DebugLog Buffer Address
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_DBGLOG_CTRL_BUFFERADDR_OFS, MEMX_GET_CHIP_DBGLOG_BUFFER_BUS_ADDR(chip_id), false);
		// DebugLog Buffer Size
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_DBGLOG_CTRL_BUFFERSIZE_OFS, MEMX_DBGLOG_CHIP_BUFFER_SIZE(chip_id), false);
		// DebugLog Buffer Write Pointer address
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_DBGLOG_CTRL_WPTRADDR_OFS, MEMX_GET_CHIP_DBGLOG_WRITER_PTR_BUS_ADDR(chip_id), false);
		// DebugLog Buffer Read Pointer address
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_DBGLOG_CTRL_RPTRADDR_OFS, MEMX_GET_CHIP_DBGLOG_READ_PTR_BUS_ADDR(chip_id), false);
		// DebugLog Enable
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_DBGLOG_CTRL_ENABLE_OFS, 0x1, false);

		// RemoteCommand Control
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_RMTCMD_CMDADDR_OFS, MEMX_GET_CHIP_RMTCMD_COMMAND_BUS_ADDR(chip_id), false);
		// RemoteCommand Parameter
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_RMTCMD_PARAMADDR_OFS, MEMX_GET_CHIP_RMTCMD_PARAM_BUS_ADDR(chip_id), false);
		// RemoteCommand Parameter2
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_RMTCMD_PARAM2ADDR_OFS, MEMX_GET_CHIP_RMTCMD_PARAM2_BUS_ADDR(chip_id), false);

		// Admin Command Enable
		memx_xflow_write(memx_dev, chip_id, MEMX_CHIP_ADMIN_TASK_EN_ADR, 0, 0x1, false);
	}
	return 0;
}

void memx_fw_log_deinit(struct memx_pcie_dev *memx_dev)
{
	u8 chip_id = 0, maxcnt;

	if (!memx_dev) {
		pr_err("memryx: memx_proc_deinit: memx_dev is NULL\n");
		return;
	}

	maxcnt = memx_dev->mpu_data.hw_info.chip.total_chip_cnt;

	// dis-connect dram buffer to CHIP's DebugLog.
	for (chip_id = CHIP_ID0; chip_id < maxcnt; chip_id++) {
		// Disabled DebugLog
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_DBGLOG_CTRL_ENABLE_OFS, 0x0, false);
		// Clear DebugLog Buffer Read Pointer address
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_DBGLOG_CTRL_RPTRADDR_OFS, MEMX_DGBLOG_RPTR_DEFAULT, false);
		// Clear DebugLog Buffer Write Pointer address
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_DBGLOG_CTRL_WPTRADDR_OFS, MEMX_DGBLOG_WPTR_DEFAULT, false);
		// DebugLog Buffer Size
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_DBGLOG_CTRL_BUFFERSIZE_OFS, MEMX_DGBLOG_SIZE_DEFAULT, false);
		// DebugLog Buffer Address
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_DBGLOG_CTRL_BUFFERADDR_OFS, MEMX_DGBLOG_ADDRESS_DEFAULT, false);

		// RemoteCommand Control
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_RMTCMD_CMDADDR_OFS, MEMX_RMTCMD_COMMAND_DEFAULT, false);
		// RemoteCommand Parameter
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_RMTCMD_PARAMADDR_OFS, MEMX_RMTCMD_PARAMTER_DEFAULT, false);
		// RemoteCommand Parameter2
		memx_xflow_write(memx_dev, chip_id, MEMX_DBGLOG_CONTROL_BASE, MEMX_RMTCMD_PARAM2ADDR_OFS, 0, false);

		// Admin Command Disable
		memx_xflow_write(memx_dev, chip_id, MEMX_CHIP_ADMIN_TASK_EN_ADR, 0, 0x0, false);

		*memx_dev->mpu_data.fw_log.write_ptr[chip_id] = 0;
		*memx_dev->mpu_data.fw_log.read_ptr[chip_id] = 0;
		memx_dev->mpu_data.fw_log.write_ptr[chip_id] = NULL;
		memx_dev->mpu_data.fw_log.read_ptr[chip_id] = NULL;
	}
}
