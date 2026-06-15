// SPDX-License-Identifier: GPL-2.0+
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/namei.h>

#include "memx_pcie.h"
#include "memx_xflow.h"
#include "memx_fs.h"
#include "memx_fw_init.h"

static s32 memx_proc_cmd_usage(struct seq_file *sfile, void *v)
{
	//struct memx_pcie_dev *memx_dev = sfile->private;
	seq_puts(sfile, "Usage: echo \"fwlog chip_id[0-7]\" > /proc/memx[dev_id 0-3]/cmd\n");
	seq_puts(sfile, "==========================================================================================\n");
	seq_puts(sfile, "Ex: Dump chip(0) firmware log\n");
	seq_puts(sfile, "echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n");
	seq_puts(sfile, "Ex: Dump chip(1) firmware log\n");
	seq_puts(sfile, "echo \"fwlog 1\" > /proc/memx0/cmd; sudo dmesg\n\n");

	seq_puts(sfile, "\nEx: Read chip(0) address 0x400A0000\n");
	seq_puts(sfile, "echo \"read 0 0x400A0000\" > /proc/memx0/cmd; sleep 1; echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n");
	seq_puts(sfile, "Ex: Write chip(0) address [0x400A0000]=0x12345678\n");
	seq_puts(sfile, "echo \"write 0 0x400A0000 0x12345678\" > /proc/memx0/cmd; sleep 1; echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n\n");

	seq_puts(sfile, "Ex: Issue chip(0) memx uart command\n");
	seq_puts(sfile, "echo \"memx0 0\" > /proc/memx0/cmd;sleep 1; echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n");
	seq_puts(sfile, "echo \"memx1 0\" > /proc/memx0/cmd;sleep 1; echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n");
	seq_puts(sfile, "echo \"memx2 0\" > /proc/memx0/cmd;sleep 1; echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n");
	seq_puts(sfile, "echo \"memx3 0\" > /proc/memx0/cmd;sleep 1; echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n");
	seq_puts(sfile, "echo \"memx6 0\" > /proc/memx0/cmd;sleep 1; echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n");
	seq_puts(sfile, "echo \"memx7 0\" > /proc/memx0/cmd;sleep 1; echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n");
	seq_puts(sfile, "echo \"memx8 0\" > /proc/memx0/cmd;sleep 1; echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n");
	seq_puts(sfile, "==========================================================================================\n");
	return 0;
}

static s32 memx_proc_debug_usage(struct seq_file *sfile, void *v)
{
	//struct memx_pcie_dev *memx_dev = sfile->private;
	seq_puts(sfile, "Usage: echo \"rmtcmd chip_id[0-7] [hex_cmdcode] [hex_cmdparam]\" > /proc/memx[dev_id 0-3]/debug\n");
	seq_puts(sfile, "==========================================================================================\n");
	seq_puts(sfile, "Ex: Issue chip(0) command memx0\n");
	seq_puts(sfile, "echo \"rmtcmd 0 0x6d656d30 0x0\" > /proc/memx0/debug; sleep 1; echo \"fwlog 0\" > /proc/memx0/cmd; sudo dmesg\n");
	seq_puts(sfile, "==========================================================================================\n");
	return 0;
}

static s32 memx_proc_i2ctrl_usage(struct seq_file *sfile, void *v)
{
	//struct memx_pcie_dev *memx_dev = sfile->private;

	seq_puts(sfile, "\nUsage: echo \"[rw-byte-cnt] data0 data0-param data1 data1-param ... dataN dataN-param \" > /proc/memx[dev_id 0-3]/i2ctrl\n");
	seq_puts(sfile, "[rw-byte-cnt] must be even number because each data byte must specified related paramter to assign such as I2C-START,STOP,NACK on i2c bus transaction\n");
	seq_puts(sfile, "parameter byte definition: bit[0]-START / bit[1]-STOP / bit[2]-NACK / bit[4]=0 means WRITE-data / bit[4]=1 means READ-data\n");
	seq_puts(sfile, "Ex: you need to set i2c slave address byte with START bit asserted to match i2c protocol\n");
	seq_puts(sfile, "=================================================================================================\n");
	seq_puts(sfile, "For example : 8bits-Salve address 0xC0 and you wait to read address 0x1234 and data length 2 byte\n");
	seq_puts(sfile, "Then the command should like this: echo \"12 0xC0 0x01 0x12 0x00 0x34 0x00 0xC1 0x01 0x00 0x10 0x00 0x16\" > /proc/memx0/i2ctrl; sudo dmesg | tail -n 10\n");
	seq_puts(sfile, "(12): there are 12 bytes in this console command follows\n");
	seq_puts(sfile, "(0xC0 0x01): This means i2c bus transmit BYTE[0]=0xC0 with START bit is set, This is WRITE data byte\n");
	seq_puts(sfile, "(0x12 0x00): This means i2c bus transmit BYTE[1]=0x12 ,no START/STOP/NACK should sned, This is WRITE data byte\n");
	seq_puts(sfile, "(0x34 0x00): This means i2c bus transmit BYTE[2]=0x34 ,no START/STOP/NACK should sned, This is WRITE data byte\n");
	seq_puts(sfile, "(0xC1 0x01): This means i2c bus transmit BYTE[3]=0xC1 with START bit is set, This is WRITE data byte, bit[0]=1 means read in the following data\n");
	seq_puts(sfile, "(0x00 0x10): This means i2c bus transmit BYTE[4] is READ data, the data field 0x00 here is dont care.\n");
	seq_puts(sfile, "(0x00 0x16): This means i2c bus transmit BYTE[5] is READ data, the data field 0x00 here is dont care. and also send NACK/STOP when this byte completed\n");
	seq_puts(sfile, "after completed, the BYTE[4]BYTE[5]read data value would shown on kernel messages to check\n");
	seq_puts(sfile, "=================================================================================================\n");
	seq_puts(sfile, "For example : 8bits-Salve address 0xB4 and you want to send PMBUS_VOUT_COMMAND(0x21) with value 0x0D99\n");
	seq_puts(sfile, "Then the command should like this: echo \"8 0xb4 0x01 0x21 0x00 0x99 0x00 0x0d 0x02\" > /proc/memx0/i2ctrl; sudo dmesg | tail -n 10\n");
	seq_puts(sfile, "This command can read back to confirm: echo \"10 0xb4 0x01 0x21 0x00 0xb5 0x01 0x00 0x10 0x00 0x16\" > /proc/memx0/i2ctrl; sudo dmesg | tail -n 10\n");
	seq_puts(sfile, "=================================================================================================\n");
	return 0;
}

static s32 memx_proc_gpioctrl_usage(struct seq_file *sfile, void *v)
{
	struct memx_pcie_dev *memx_dev = sfile->private;
	struct transport_cmd cmd = {0};

	//step1: echo "r goio_number" > /proc/memx0/gpioctrl
	//step2: cat /proc/memx0/gpioctrl

	cmd.SQ.opCode = MEMX_ADMIN_CMD_GET_FEATURE;
	cmd.SQ.subOpCode = FID_DEVICE_GPIO;
	cmd.CQ.data[0] = memx_dev->gpio_r & 0xFF;

	mutex_lock(&memx_dev->adminlock);
	memx_admin_trigger(memx_dev, ((memx_dev->gpio_r >> 8) & 0xF), &cmd);
	cmd.CQ.status = memx_admin_fetch_result(memx_dev, ((memx_dev->gpio_r >> 8) & 0xF), &cmd);
	mutex_unlock(&memx_dev->adminlock);

	seq_printf(sfile, "%d (chip%d io%d)", cmd.CQ.data[1], ((memx_dev->gpio_r >> 8) & 0xF), ((memx_dev->gpio_r >> 0) & 0xFF));
	return 0;
}

static s32 memx_proc_qspi_usage(struct seq_file *sfile, void *v)
{
	struct memx_pcie_dev *memx_dev = sfile->private;
	const struct firmware *firmware = NULL;
	u32 *firmware_buffer_pos = NULL;
	u32 firmware_size = 0;
	u32 i, base_addr = MXCNST_DATASRAM_BASE;
	unsigned long timeout;
	u32 crc_value = 0, crc_check = 1;
	u32 type_value = 0, type_check = 0, tvalue;

	// Release QSPI Rst
	tvalue = memx_xflow_read(memx_dev, 0, 0x20000208, 0, false);
	if ((tvalue & 0x70000000) != 0x70000000) {
		memx_xflow_write(memx_dev, 0, 0x20000208, 0, (tvalue | 0x70000000), false);
		msleep(10);
	}

	seq_puts(sfile,	 "================================================================================================\n");
	if (request_firmware(&firmware, FIRMWARE_BIN_NAME, &memx_dev->pDev->dev) < 0) {
		seq_printf(sfile, "download_fw: request_firmware for %s failed\n", FIRMWARE_BIN_NAME);
		return 0;
	}
	firmware_buffer_pos = (u32 *)firmware->data;
	firmware_size = firmware->size;

	if (*((u32 *)((u8 *)firmware_buffer_pos + 0x6F08)) == 0) {
		crc_value = *((u32 *)((u8 *)firmware_buffer_pos + firmware_size - 8));
		crc_check = memx_crc32((u8 *)firmware_buffer_pos + 4, firmware_size - 12);
		type_value = *((u32 *)((u8 *)firmware_buffer_pos + MXCNST_IMG_TYPE_OFS));
		type_check = memx_sram_read(memx_dev, MXCNST_FW_TYPE_OFS); //skip first 4bytes of bin length
	} else if (*((u32 *)((u8 *)firmware_buffer_pos + 0x6F08)) == 1) {
		crc_value = *((u32 *)((u8 *)firmware_buffer_pos + firmware_size - 4));
		crc_check = memx_crc32((u8 *)firmware_buffer_pos, firmware_size - 4);
		type_value = *((u32 *)((u8 *)firmware_buffer_pos + MXCNST_IMG_TYPE_OFS));
		type_check = memx_sram_read(memx_dev, MXCNST_FW_TYPE_OFS);
	}
	if (crc_value != crc_check) {
		seq_printf(sfile, "CHECK CRC(%#08x, %#08x) FAILED!!\n", crc_value, crc_check);
		release_firmware(firmware);
		return 0;
	}
	if (type_value != type_check) {
		seq_printf(sfile, "CHECK TYPE(%#08x, %#08x) FAILED!!\n", type_value, type_check);
		release_firmware(firmware);
		return 0;
	}

	for (i = 0; i < firmware_size; i += 4)
		memx_sram_write(memx_dev, base_addr+i, firmware_buffer_pos[i>>2]);


	memx_sram_write(memx_dev, MXCNST_RMTCMD_PARAM, 0);
	memx_sram_write(memx_dev, MXCNST_RMTCMD_COMMD, MXCNST_MEMXQ_CMD);

	timeout = jiffies + (HZ*5);
	while (memx_sram_read(memx_dev, MXCNST_RMTCMD_PARAM) == 0) {
		if (time_after(jiffies, timeout)) {
			seq_puts(sfile, "Update QSPI FLASH TIMEOUT FAILED!!\n");
			release_firmware(firmware);
			return 0;
		}
	};

	if (memx_sram_read(memx_dev, MXCNST_RMTCMD_PARAM) == 1)
		seq_printf(sfile, "Update QSPI FLASH PASS!! Verion:0x%08X DateCode:0x%08X (Please REBOOT to activate new fw)\n", firmware_buffer_pos[0x6F0C>>2], firmware_buffer_pos[0x6F10>>2]);
	else
		seq_puts(sfile, "Update QSPI FLASH FAILED!!\n");
	seq_puts(sfile,	 "================================================================================================\n");

	release_firmware(firmware);
	return 0;
}

static s32 memx_proc_thermal_usage(struct seq_file *sfile, void *v)
{
	struct memx_pcie_dev *memx_dev = sfile->private;

	seq_printf(sfile, "%s\n", memx_dev->ThermalThrottlingDisable ? "Disable":"Enable");
	return 0;
}

static s32 memx_proc_verinfo_usage(struct seq_file *sfile, void *v)
{
	struct memx_pcie_dev *memx_dev = sfile->private;
	uint32_t value;
	char chip_version[4] = "N/A";

	seq_puts(sfile, "pcie intf device:\n");
	seq_printf(sfile, "SDK version: %s\n", SDK_RELEASE_VERSION);
	seq_printf(sfile, "kdriver version: %s\n", PCIE_VERSION);
	seq_printf(sfile, "FW_CommitID=0x%08x DateCode=0x%08x\n", memx_sram_read(memx_dev, MXCNST_COMMITID), memx_sram_read(memx_dev, MXCNST_DATECODE));
	seq_printf(sfile, "ManufacturerID=0x%08x%08x\n", memx_sram_read(memx_dev, MXCNST_MANUFACTID2), memx_sram_read(memx_dev, MXCNST_MANUFACTID1));
	seq_printf(sfile, "Cold+Warm-RebootCnt=%d  Warm-RebootCnt=%d\n", ((memx_sram_read(memx_dev, MXCNST_COLDRSTCNT_ADDR)&0xFFFF0000) == 0x9ABC0000) ? (memx_sram_read(memx_dev, MXCNST_COLDRSTCNT_ADDR)&0xFFFF) : 0,
		((memx_sram_read(memx_dev, MXCNST_WARMRSTCNT_ADDR)&0xFFFF0000) == 0x9ABC0000) ? (memx_sram_read(memx_dev, MXCNST_WARMRSTCNT_ADDR)&0xFFFF) : 0);

	value = memx_xflow_read(memx_dev, 0, MXCNST_CHIP_VERSION, 0, false);
	if ((value & 0xF) == 5)
		snprintf(chip_version, 4, "A1");
	else
		snprintf(chip_version, 4, "A0");

	value = memx_xflow_read(memx_dev, 0, MXCNST_BOOT_MODE, 0, false);
	switch ((value >> 7) & 0x3) {
	case 0: {
		seq_printf(sfile, "BootMode=QSPI  Chip=%s\n", chip_version);
	} break;
	case 1: {
		seq_printf(sfile, "BootMode=USB  Chip=%s\n", chip_version);
	} break;
	case 2: {
		seq_printf(sfile, "BootMode=PCIe  Chip=%s\n", chip_version);
	} break;
	case 3: {
		seq_printf(sfile, "BootMode=UART  Chip=%s\n", chip_version);
	} break;
	default:
		seq_printf(sfile, "BootMode=N/A  Chip=%s\n", chip_version);
		break;
	}
	return 0;
}

static s32 memx_proc_mpuuti_usage(struct seq_file *sfile, void *v)
{
	u8 chip_id, grpid = 0;
	struct memx_pcie_dev *memx_dev = sfile->private;

	for (chip_id = 0; chip_id < memx_dev->mpu_data.hw_info.chip.total_chip_cnt; chip_id++) {
		u32 data = memx_sram_read(memx_dev, (MXCNST_MPUUTIL_BASE+(chip_id<<2)));

		if (data != 0xFF) {
			seq_printf(sfile, "chip%d(group%d):%u%% ", chip_id, grpid, data);
			grpid++;
		}
	}
	seq_puts(sfile, "\n");
	return 0;
}

static s32 memx_proc_temperature_usage(struct seq_file *sfile, void *v)
{
	u8 chip_id;
	struct memx_pcie_dev *memx_dev = sfile->private;
	u32 data;
	s16 temp_Celsius = 0;

	for (chip_id = 0; chip_id < memx_dev->mpu_data.hw_info.chip.total_chip_cnt; chip_id++) {
		data = memx_sram_read(memx_dev, (MXCNST_TEMP_BASE+(chip_id<<2)));
		temp_Celsius = (data&0xFFFF) - 273;
		seq_printf(sfile, "CHIP(%d) PVT%d Temperature: %d C (%u Kelvin) (ThermalThrottlingState: %d)\n", chip_id, (data>>16)&0xF, temp_Celsius, (data&0xFFFF), (data>>20)&0xF);
	}

	return 0;
}

static s32 memx_proc_throughput_usage(struct seq_file *sfile, void *v)
{
	u32 tx_size_kb = tx_size / 1024;
	u32 rx_size_kb = rx_size / 1024;
	u32 udrv_w_quotient = udrv_throughput_info.stream_write_us ? (udrv_throughput_info.stream_write_kb * 976 / udrv_throughput_info.stream_write_us) : 0;
	u32 udrv_w_decimal = udrv_throughput_info.stream_write_us ? (udrv_throughput_info.stream_write_kb * 976 % udrv_throughput_info.stream_write_us) * 1000 / udrv_throughput_info.stream_write_us : 0;
	u32 udrv_r_quotient = udrv_throughput_info.stream_read_us ? (udrv_throughput_info.stream_read_kb * 976 / udrv_throughput_info.stream_read_us) : 0;
	u32 udrv_r_decimal = udrv_throughput_info.stream_read_us ? (udrv_throughput_info.stream_read_kb * 976 % udrv_throughput_info.stream_read_us) * 1000 / udrv_throughput_info.stream_read_us : 0;
	u32 kdrv_w_quotient = tx_time_us ? (tx_size_kb * 976 / tx_time_us) : 0;
	u32 kdrv_w_decimal = tx_time_us ? (tx_size_kb * 976 % tx_time_us) * 1000 / tx_time_us : 0;
	u32 kdrv_r_quotient = rx_time_us ? (rx_size_kb * 976 / rx_time_us) : 0;
	u32 kdrv_r_decimal = rx_time_us ? (rx_size_kb * 976 % rx_time_us) * 1000 / rx_time_us : 0;
	u32 kdrv_w_value = kdrv_w_quotient * 1000 + kdrv_w_decimal;
	u32 kdrv_r_value = kdrv_r_quotient * 1000 + kdrv_r_decimal;
	u32 udrv_w_value = udrv_w_quotient * 1000 + udrv_w_decimal;
	u32 udrv_r_value = udrv_r_quotient * 1000 + udrv_r_decimal;
	u32 write_quotient = udrv_w_value ? (kdrv_w_value * 100 / udrv_w_value) : 0;
	u32 write_decimal = udrv_w_value ? (kdrv_w_value * 100 % udrv_w_value) * 1000 / udrv_w_value : 0;
	u32 read_quotient = udrv_r_value ? (kdrv_r_value * 100 / udrv_r_value) : 0;
	u32 read_decimal = udrv_r_value ? (kdrv_r_value * 100 % udrv_r_value) * 1000 / udrv_r_value : 0;

	seq_puts(sfile, "  Item  |  Period(us)  |   Data(KB)   |   TP(MB/s)   | Kdrv/Udrv\n");
	seq_puts(sfile, "--------+--------------+--------------+--------------+-------------\n");
	seq_printf(sfile, " Kdrv_W |  %#10x  |  %#10x  | %8u.%03u\n", tx_time_us, tx_size_kb, kdrv_w_quotient, kdrv_w_decimal);
	seq_printf(sfile, " Udrv_W |  %#10x  |  %#10x  | %8u.%03u | %3u.%03u %%\n", udrv_throughput_info.stream_write_us, udrv_throughput_info.stream_write_kb, udrv_w_quotient, udrv_w_decimal, write_quotient, write_decimal);
	seq_printf(sfile, " Kdrv_R |  %#10x  |  %#10x  | %8u.%03u\n", rx_time_us, rx_size_kb, kdrv_r_quotient, kdrv_r_decimal);
	seq_printf(sfile, " Udrv_R |  %#10x  |  %#10x  | %8u.%03u | %3u.%03u %%\n", udrv_throughput_info.stream_read_us, udrv_throughput_info.stream_read_kb, udrv_r_quotient, udrv_r_decimal, read_quotient, read_decimal);

	udrv_throughput_info.stream_write_kb = 0;
	udrv_throughput_info.stream_read_kb = 0;
	udrv_throughput_info.stream_write_us = 0;
	udrv_throughput_info.stream_read_us = 0;
	tx_time_us = 0;
	tx_size = 0;
	rx_time_us = 0;
	rx_size = 0;
	return 0;
}

static s32 memx_proc_frequency_usage(struct seq_file *sfile, void *v)
{
	u32 data[2];
	struct memx_pcie_dev *memx_dev = sfile->private;
	u8 chip_id = 0;

	for (chip_id = 0; chip_id < memx_dev->mpu_data.hw_info.chip.total_chip_cnt; chip_id++) {
		memx_fs_get_frequency(memx_dev, &data[0], chip_id);
		seq_printf(sfile, "CHIP(%d) Frequency %d MHz, %d MHz\n", chip_id, data[1], data[0]);
	}

	return 0;
}

static int memx_proc_open(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_cmd_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_cmd_usage, PDE_DATA(inode));
#endif
}

static int memx_proc_open_debug(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_debug_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_debug_usage, PDE_DATA(inode));
#endif
}

static int memx_proc_open_qspi(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_qspi_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_qspi_usage, PDE_DATA(inode));
#endif
}

static int memx_proc_open_i2ctrl(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_i2ctrl_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_i2ctrl_usage, PDE_DATA(inode));
#endif
}

static int memx_proc_open_gpioctrl(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_gpioctrl_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_gpioctrl_usage, PDE_DATA(inode));
#endif
}

static int memx_proc_open_thermal(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_thermal_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_thermal_usage, PDE_DATA(inode));
#endif
}

static int memx_proc_open_ver(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_verinfo_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_verinfo_usage, PDE_DATA(inode));
#endif
}

static int memx_proc_open_mpuuti(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_mpuuti_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_mpuuti_usage, PDE_DATA(inode));
#endif
}

static int memx_proc_open_temperature(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_temperature_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_temperature_usage, PDE_DATA(inode));
#endif
}

static int memx_proc_open_throughput(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_throughput_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_throughput_usage, PDE_DATA(inode));
#endif
}

static int memx_proc_open_frequency(struct inode *inode, struct file *file)
{
#if KERNEL_VERSION(5, 17, 11) <= _LINUX_VERSION_CODE_
	return single_open(file, memx_proc_frequency_usage, pde_data(inode));
#else
	return single_open(file, memx_proc_frequency_usage, PDE_DATA(inode));
#endif
}

ssize_t memx_proc_write(struct file *file, const char __user *user_input_buf, size_t user_input_buf_size, loff_t *off);
ssize_t memx_proc_write(struct file *file, const char __user *user_input_buf, size_t user_input_buf_size, loff_t *off)
{
	s32 ret = -EINVAL;
	struct memx_pcie_dev *memx_dev = NULL;

	if (!file || !file->private_data) {
		pr_err("memryx: %s: file or file->private_data is NULL!\n", __func__);
		return ret;
	}
	memx_dev = ((struct seq_file *)file->private_data)->private;
	if (!memx_dev) {
		pr_err("memryx: %s: memx_dev is NULL!\n", __func__);
		return ret;
	}
	if (!user_input_buf) {
		pr_err("memryx: %s: user input buf is NULL!\n", __func__);
		return ret;
	}
	if (user_input_buf_size == 0) {
		pr_err("memryx: Command length is invalid!\n");
		return ret;
	}

	ret = memx_fs_parse_cmd_and_exec(memx_dev, user_input_buf, user_input_buf_size);
	if (ret != 0) {
		pr_err("memryx: %s: parse or exec fail!, err(%d)\n", __func__, ret);
		return ret;
	}

	return user_input_buf_size;
}

ssize_t memx_proc_write_thermal(struct file *file, const char __user *user_input_buf, size_t user_input_buf_size, loff_t *off);
ssize_t memx_proc_write_thermal(struct file *file, const char __user *user_input_buf, size_t user_input_buf_size, loff_t *off)
{
	s32 ret = -EINVAL;
	struct memx_pcie_dev *memx_dev = NULL;
	char *input_parser_buffer_ptr = NULL;
	u32 chip_id;

	if (!file || !file->private_data) {
		pr_err("memryx: %s: file or file->private_data is NULL!\n", __func__);
		return ret;
	}
	memx_dev = ((struct seq_file *)file->private_data)->private;
	if (!memx_dev) {
		pr_err("memryx: %s: memx_dev is NULL!\n", __func__);
		return ret;
	}
	if (!user_input_buf) {
		pr_err("memryx: %s: user input buf is NULL!\n", __func__);
		return ret;
	}
	if (user_input_buf_size == 0) {
		pr_err("memryx: Command length is invalid!\n");
		return ret;
	}

	input_parser_buffer_ptr = memdup_user_nul(user_input_buf, user_input_buf_size);
	if (IS_ERR(input_parser_buffer_ptr)) {
		pr_err("memryx: %s: memdup_user_nul fail!\n", __func__);
		return PTR_ERR(input_parser_buffer_ptr);
	}

	if (strncmp(input_parser_buffer_ptr, "Enable", 6) == 0) {
		pr_err("memryx: set thermal throttling Enable\n");
		for (chip_id = 0; chip_id < memx_dev->mpu_data.hw_info.chip.total_chip_cnt; chip_id++) {
			*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_PARAM_VIRTUAl_ADDR(memx_dev, chip_id)))   = 1;
			*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_COMMAND_VIRTUAl_ADDR(memx_dev, chip_id))) = MXCNST_MEMXt_CMD;
		}
		memx_dev->ThermalThrottlingDisable = 0;
	} else if (strncmp(input_parser_buffer_ptr, "Disable", 7) == 0) {
		pr_err("memryx: set thermal throttling Disable\n");
		for (chip_id = 0; chip_id < memx_dev->mpu_data.hw_info.chip.total_chip_cnt; chip_id++) {
			*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_PARAM_VIRTUAl_ADDR(memx_dev, chip_id)))   = 0;
			*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_COMMAND_VIRTUAl_ADDR(memx_dev, chip_id))) = MXCNST_MEMXt_CMD;
		}
		memx_dev->ThermalThrottlingDisable = 1;
	} else {
		pr_err("memryx: unsupported cmd:  %s(Only \"Enable\" and \"Disable\" are valid)\n", input_parser_buffer_ptr);
	}

	return user_input_buf_size;
}

ssize_t memx_proc_write_i2ctrl(struct file *file, const char __user *user_input_buf, size_t user_input_buf_size, loff_t *off);
ssize_t memx_proc_write_i2ctrl(struct file *file, const char __user *user_input_buf, size_t user_input_buf_size, loff_t *off)
{
	s32 ret = -EINVAL;
	struct memx_pcie_dev *memx_dev = NULL;

	if (!file || !file->private_data) {
		pr_err("%s: file or file->private_data is NULL!\n", __func__);
		return ret;
	}
	memx_dev = ((struct seq_file *)file->private_data)->private;
	if (!memx_dev) {
		pr_err("%s: memx_dev is NULL!\n", __func__);
		return ret;
	}
	if (!user_input_buf) {
		pr_err("%s: user input buf is NULL!\n", __func__);
		return ret;
	}
	if (user_input_buf_size == 0) {
		pr_err("Command length is invild!\n");
		return ret;
	}

	ret = memx_fs_parse_i2ctrl_and_exec(memx_dev, user_input_buf, user_input_buf_size);
	if (ret != 0) {
		pr_err("%s: parse or exec fail!, err(%d)\n", __func__, ret);
		return ret;
	}

	return user_input_buf_size;
}

ssize_t memx_proc_write_gpioctrl(struct file *file, const char __user *user_input_buf, size_t user_input_buf_size, loff_t *off);
ssize_t memx_proc_write_gpioctrl(struct file *file, const char __user *user_input_buf, size_t user_input_buf_size, loff_t *off)
{
	s32 ret = -EINVAL;
	struct memx_pcie_dev *memx_dev = NULL;

	if (!file || !file->private_data) {
		pr_err("%s: file or file->private_data is NULL!\n", __func__);
		return ret;
	}
	memx_dev = ((struct seq_file *)file->private_data)->private;
	if (!memx_dev) {
		pr_err("%s: memx_dev is NULL!\n", __func__);
		return ret;
	}
	if (!user_input_buf) {
		pr_err("%s: user input buf is NULL!\n", __func__);
		return ret;
	}
	if (user_input_buf_size == 0) {
		pr_err("Command length is invild!\n");
		return ret;
	}

	ret = memx_fs_parse_gpioctrl_and_exec(memx_dev, user_input_buf, user_input_buf_size);
	if (ret != 0) {
		pr_err("%s: parse or exec fail!, err(%d)\n", __func__, ret);
		return ret;
	}

	return user_input_buf_size;
}

#if  KERNEL_VERSION(5, 6, 0) <= _LINUX_VERSION_CODE_
static const struct proc_ops proc_cmd_fops = {
	.proc_open	= memx_proc_open,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
	.proc_write   = memx_proc_write
};
static const struct proc_ops proc_debug_fops = {
	.proc_open	= memx_proc_open_debug,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
	.proc_write   = memx_proc_write
};
static const struct proc_ops proc_qspi_fops = {
	.proc_open	= memx_proc_open_qspi,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};
static const struct proc_ops proc_i2ctrl_fops = {
	.proc_open	= memx_proc_open_i2ctrl,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
	.proc_write   = memx_proc_write_i2ctrl
};
static const struct proc_ops proc_gpioctrl_fops = {
	.proc_open	= memx_proc_open_gpioctrl,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
	.proc_write   = memx_proc_write_gpioctrl
};
static const struct proc_ops proc_thermal_fops = {
	.proc_open	= memx_proc_open_thermal,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
	.proc_write   = memx_proc_write_thermal
};
static const struct proc_ops proc_verinfo_fops = {
	.proc_open	= memx_proc_open_ver,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};
static const struct proc_ops proc_mpu_uti_fops = {
	.proc_open	= memx_proc_open_mpuuti,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};
static const struct proc_ops proc_temperature_fops = {
	.proc_open	= memx_proc_open_temperature,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};
static const struct proc_ops proc_throughput_fops = {
	.proc_open	= memx_proc_open_throughput,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};
static const struct proc_ops proc_frequency_fops = {
	.proc_open	= memx_proc_open_frequency,
	.proc_read	= seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};
#else
static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open	= memx_proc_open,
	.read	= seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = memx_proc_write
	};
static struct file_operations proc_debug_fops = {
	.owner   = THIS_MODULE,
	.open	= memx_proc_open_debug,
	.read	= seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = memx_proc_write
	};
static struct file_operations proc_qspi_fops = {
	.owner   = THIS_MODULE,
	.open	= memx_proc_open_qspi,
	.read	= seq_read,
	.llseek  = seq_lseek,
	.release = single_release
	};
static struct file_operations proc_i2ctrl_fops = {
	.owner   = THIS_MODULE,
	.open	= memx_proc_open_i2ctrl,
	.read	= seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = memx_proc_write_i2ctrl
	};
static struct file_operations proc_gpioctrl_fops = {
	.owner   = THIS_MODULE,
	.open	= memx_proc_open_gpioctrl,
	.read	= seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = memx_proc_write_gpioctrl
	};
static struct file_operations proc_thermal_fops = {
	.owner   = THIS_MODULE,
	.open	= memx_proc_open_thermal,
	.read	= seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = memx_proc_write_thermal
	};
static struct file_operations proc_verinfo_fops = {
	.owner   = THIS_MODULE,
	.open	= memx_proc_open_ver,
	.read	= seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	};
static struct file_operations proc_mpu_uti_fops = {
	.owner   = THIS_MODULE,
	.open	= memx_proc_open_mpuuti,
	.read	= seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};
static struct file_operations proc_temperature_fops = {
	.owner   = THIS_MODULE,
	.open	= memx_proc_open_temperature,
	.read	= seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};
static struct file_operations throughput_fops = {
	.owner   = THIS_MODULE,
	.open	= memx_proc_open_throughput,
	.read	= seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};
#endif

s32 memx_fs_proc_init(struct memx_pcie_dev *memx_dev);
s32 memx_fs_proc_init(struct memx_pcie_dev *memx_dev)
{
	char root_dir_name[16];
	int minor = 0;
#ifndef ANDROID
	char name[128];
	struct path path;
#endif

	if (!memx_dev) {
		pr_err("memryx: memx_fs_sys init: memx_dev is NULL\n");
		return -EINVAL;
	}

#ifndef ANDROID
	for (minor = 0; minor < 128; minor++) {
		sprintf(name, "/proc/memx%d/cmd", minor);
		if (kern_path(name, LOOKUP_FOLLOW, &path)) {
			pr_info("memryx: register for %s\n", name);
			break;

		} else {
            path_put(&path);
		}
	}
#endif

	sprintf(root_dir_name, DEVICE_NODE_NAME, minor);
	memx_dev->fs.hif.proc.root_dir = proc_mkdir(root_dir_name, NULL);
	if (!memx_dev->fs.hif.proc.root_dir) {
		pr_err("memryx: create proc root_dir failed!\n");
		return -EINVAL;
	}

	memx_dev->fs.hif.proc.cmd_entry = proc_create_data("cmd", MXCNST_RWACCESS, memx_dev->fs.hif.proc.root_dir, &proc_cmd_fops, memx_dev);
	if (!memx_dev->fs.hif.proc.cmd_entry) {
		pr_err("memryx: failed to create proc file for cmd_entry!\n");
		proc_remove(memx_dev->fs.hif.proc.root_dir);
		return -EINVAL;
	}

	if (memx_dev->fs.debug_en) {
		memx_dev->fs.hif.proc.debug_entry = proc_create_data("debug", MXCNST_RWACCESS, memx_dev->fs.hif.proc.root_dir, &proc_debug_fops, memx_dev);
		if (!memx_dev->fs.hif.proc.debug_entry) {
			pr_err("memryx: failed to create proc file for debug_entry!\n");
			proc_remove(memx_dev->fs.hif.proc.cmd_entry);
			proc_remove(memx_dev->fs.hif.proc.root_dir);
			return -EINVAL;
		}

		memx_dev->fs.hif.proc.qspi_entry = proc_create_data("update_flash", 0444, memx_dev->fs.hif.proc.root_dir, &proc_qspi_fops, memx_dev);
		if (!memx_dev->fs.hif.proc.qspi_entry) {
			pr_err("memryx: failed to create proc file for qspi_entry!\n");
			proc_remove(memx_dev->fs.hif.proc.debug_entry);
			proc_remove(memx_dev->fs.hif.proc.cmd_entry);
			proc_remove(memx_dev->fs.hif.proc.root_dir);
			return -EINVAL;
		}

		memx_dev->fs.hif.proc.i2ctrl_entry = proc_create_data("i2ctrl", MXCNST_RWACCESS, memx_dev->fs.hif.proc.root_dir, &proc_i2ctrl_fops, memx_dev);
		if (!memx_dev->fs.hif.proc.i2ctrl_entry) {
			pr_err("failed to create proc file for i2ctrl!\n");
			proc_remove(memx_dev->fs.hif.proc.qspi_entry);
			proc_remove(memx_dev->fs.hif.proc.debug_entry);
			proc_remove(memx_dev->fs.hif.proc.cmd_entry);
			proc_remove(memx_dev->fs.hif.proc.root_dir);
			return -EINVAL;
		}

		memx_dev->fs.hif.proc.gpio_entry = proc_create_data("gpioctrl", MXCNST_RWACCESS, memx_dev->fs.hif.proc.root_dir, &proc_gpioctrl_fops, memx_dev);
		if (!memx_dev->fs.hif.proc.gpio_entry) {
			pr_err("failed to create proc file for gpio!\n");
			proc_remove(memx_dev->fs.hif.proc.i2ctrl_entry);
			proc_remove(memx_dev->fs.hif.proc.qspi_entry);
			proc_remove(memx_dev->fs.hif.proc.debug_entry);
			proc_remove(memx_dev->fs.hif.proc.cmd_entry);
			proc_remove(memx_dev->fs.hif.proc.root_dir);
			return -EINVAL;
		}
	}

	memx_dev->fs.hif.proc.verinfo_entry = proc_create_data("verinfo", 0444, memx_dev->fs.hif.proc.root_dir, &proc_verinfo_fops, memx_dev);
	if (!memx_dev->fs.hif.proc.verinfo_entry) {
		pr_err("memryx: failed to create proc file for verinfo_entry!\n");
		if (memx_dev->fs.debug_en) {
			proc_remove(memx_dev->fs.hif.proc.debug_entry);
			proc_remove(memx_dev->fs.hif.proc.gpio_entry);
			proc_remove(memx_dev->fs.hif.proc.i2ctrl_entry);
			proc_remove(memx_dev->fs.hif.proc.qspi_entry);
		}
		proc_remove(memx_dev->fs.hif.proc.cmd_entry);
		proc_remove(memx_dev->fs.hif.proc.root_dir);
		return -EINVAL;
	}

	memx_dev->fs.hif.proc.mpu_uti_entry = proc_create_data("utilization", 0444, memx_dev->fs.hif.proc.root_dir, &proc_mpu_uti_fops, memx_dev);
	if (!memx_dev->fs.hif.proc.mpu_uti_entry) {
		pr_err("memryx: failed to create proc file for mpu_uti_entry!\n");
		proc_remove(memx_dev->fs.hif.proc.verinfo_entry);
		if (memx_dev->fs.debug_en) {
			proc_remove(memx_dev->fs.hif.proc.debug_entry);
			proc_remove(memx_dev->fs.hif.proc.gpio_entry);
			proc_remove(memx_dev->fs.hif.proc.i2ctrl_entry);
			proc_remove(memx_dev->fs.hif.proc.qspi_entry);
		}
		proc_remove(memx_dev->fs.hif.proc.cmd_entry);
		proc_remove(memx_dev->fs.hif.proc.root_dir);
		return -EINVAL;
	}

	memx_dev->fs.hif.proc.temperature_entry = proc_create_data("temperature", 0444, memx_dev->fs.hif.proc.root_dir, &proc_temperature_fops, memx_dev);
	if (!memx_dev->fs.hif.proc.temperature_entry) {
		pr_err("memryx: failed to create proc file for temperature_entry!\n");
		proc_remove(memx_dev->fs.hif.proc.mpu_uti_entry);
		proc_remove(memx_dev->fs.hif.proc.verinfo_entry);
		if (memx_dev->fs.debug_en) {
			proc_remove(memx_dev->fs.hif.proc.debug_entry);
			proc_remove(memx_dev->fs.hif.proc.gpio_entry);
			proc_remove(memx_dev->fs.hif.proc.i2ctrl_entry);
			proc_remove(memx_dev->fs.hif.proc.qspi_entry);
		}
		proc_remove(memx_dev->fs.hif.proc.cmd_entry);
		proc_remove(memx_dev->fs.hif.proc.root_dir);
		return -EINVAL;
	}

	if (memx_dev->fs.debug_en) {
		memx_dev->fs.hif.proc.thermal_entry = proc_create_data("thermalthrottling", MXCNST_RWACCESS, memx_dev->fs.hif.proc.root_dir, &proc_thermal_fops, memx_dev);
		if (!memx_dev->fs.hif.proc.thermal_entry) {
			pr_err("memryx: failed to create proc file for debug_entry!\n");
			proc_remove(memx_dev->fs.hif.proc.temperature_entry);
			proc_remove(memx_dev->fs.hif.proc.mpu_uti_entry);
			proc_remove(memx_dev->fs.hif.proc.verinfo_entry);
			if (memx_dev->fs.debug_en) {
				proc_remove(memx_dev->fs.hif.proc.debug_entry);
				proc_remove(memx_dev->fs.hif.proc.gpio_entry);
				proc_remove(memx_dev->fs.hif.proc.i2ctrl_entry);
				proc_remove(memx_dev->fs.hif.proc.qspi_entry);
			}
			proc_remove(memx_dev->fs.hif.proc.cmd_entry);
			proc_remove(memx_dev->fs.hif.proc.root_dir);
			return -EINVAL;
		}

		memx_dev->fs.hif.proc.throughput_entry = proc_create_data("throughput", 0444, memx_dev->fs.hif.proc.root_dir, &proc_throughput_fops, memx_dev);
		if (!memx_dev->fs.hif.proc.throughput_entry) {
			pr_err("memryx: failed to create proc file for throughput_entry!\n");
			proc_remove(memx_dev->fs.hif.proc.mpu_uti_entry);
			proc_remove(memx_dev->fs.hif.proc.verinfo_entry);
			if (memx_dev->fs.debug_en) {
				proc_remove(memx_dev->fs.hif.proc.thermal_entry);
				proc_remove(memx_dev->fs.hif.proc.debug_entry);
				proc_remove(memx_dev->fs.hif.proc.gpio_entry);
				proc_remove(memx_dev->fs.hif.proc.i2ctrl_entry);
				proc_remove(memx_dev->fs.hif.proc.qspi_entry);
			}
			proc_remove(memx_dev->fs.hif.proc.cmd_entry);
			proc_remove(memx_dev->fs.hif.proc.root_dir);
			return -EINVAL;
		}

		memx_dev->fs.hif.proc.frequency_entry = proc_create_data("frequency", 0444, memx_dev->fs.hif.proc.root_dir, &proc_frequency_fops, memx_dev);
		if (!memx_dev->fs.hif.proc.frequency_entry) {
			pr_err("memryx: failed to create proc file for frequency_entry!\n");
			proc_remove(memx_dev->fs.hif.proc.mpu_uti_entry);
			proc_remove(memx_dev->fs.hif.proc.verinfo_entry);
			if (memx_dev->fs.debug_en) {
				proc_remove(memx_dev->fs.hif.proc.throughput_entry);
				proc_remove(memx_dev->fs.hif.proc.thermal_entry);
				proc_remove(memx_dev->fs.hif.proc.debug_entry);
				proc_remove(memx_dev->fs.hif.proc.gpio_entry);
				proc_remove(memx_dev->fs.hif.proc.i2ctrl_entry);
				proc_remove(memx_dev->fs.hif.proc.qspi_entry);
			}
			proc_remove(memx_dev->fs.hif.proc.cmd_entry);
			proc_remove(memx_dev->fs.hif.proc.root_dir);
			return -EINVAL;
		}
	}

	return 0;
}

void memx_fs_proc_deinit(struct memx_pcie_dev *memx_dev);
void memx_fs_proc_deinit(struct memx_pcie_dev *memx_dev)
{

	if (!memx_dev) {
		pr_err("memryx: memx_proc_deinit: memx_dev is NULL\n");
		return;
	}
	proc_remove(memx_dev->fs.hif.proc.temperature_entry);
	proc_remove(memx_dev->fs.hif.proc.mpu_uti_entry);
	proc_remove(memx_dev->fs.hif.proc.verinfo_entry);
	if (memx_dev->fs.debug_en) {
		proc_remove(memx_dev->fs.hif.proc.frequency_entry);
		proc_remove(memx_dev->fs.hif.proc.throughput_entry);
		proc_remove(memx_dev->fs.hif.proc.thermal_entry);
		proc_remove(memx_dev->fs.hif.proc.debug_entry);
		proc_remove(memx_dev->fs.hif.proc.qspi_entry);
		proc_remove(memx_dev->fs.hif.proc.gpio_entry);
		proc_remove(memx_dev->fs.hif.proc.i2ctrl_entry);
	}
	proc_remove(memx_dev->fs.hif.proc.cmd_entry);
	proc_remove(memx_dev->fs.hif.proc.root_dir);
}
