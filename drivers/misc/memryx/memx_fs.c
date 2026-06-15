// SPDX-License-Identifier: GPL-2.0+
#include <linux/vmalloc.h>
#include "memx_pcie.h"
#include "memx_fs.h"
#include "memx_fs_proc.h"
#include "memx_fs_sys.h"
#include "memx_fs_hwmon.h"
#include "memx_xflow.h"

#define MEMX_NUM_OF_FS_CMDS (sizeof(g_fs_cmd_tbl) / sizeof(struct memx_fs_cmd_cb))
#define DATA_BEGIN_CHIP_0 (0)
#define DATA_END_CHIP_0 CQ_DATA_LEN
#define MEMX_GET_CHIP_ADMIN_CMD_BASE_VIRTUAL_ADDR(memx_dev, chip_id) (((memx_dev)->mpu_data.rx_dma_coherent_buffer_virtual_base) + \
																		MEMX_ADMCMD_VIRTUAL_OFFSET + ((chip_id) * MEMX_ADMCMD_SIZE))

static struct memx_fs_cmd_cb g_fs_cmd_tbl[] = {
	{ "fwlog", FS_CMD_FWLOG_ARGC, memx_fs_cmd_handler},
	{ "read", FS_CMD_READ_ARGC, memx_fs_cmd_handler},
	{ "write", FS_CMD_WRITE_ARGC, memx_fs_cmd_handler},
	{ "memx0", FS_CMD_MEMX0_ARGC, memx_fs_cmd_handler},
	{ "memx1", FS_CMD_MEMX1_ARGC, memx_fs_cmd_handler},
	{ "memx2", FS_CMD_MEMX2_ARGC, memx_fs_cmd_handler},
	{ "memx3", FS_CMD_MEMX3_ARGC, memx_fs_cmd_handler},
	{ "memx6", FS_CMD_MEMX6_ARGC, memx_fs_cmd_handler},
	{ "memx7", FS_CMD_MEMX7_ARGC, memx_fs_cmd_handler},
	{ "memx8", FS_CMD_MEMX8_ARGC, memx_fs_cmd_handler},
	{ "memx9", FS_CMD_MEMX9_ARGC, memx_fs_cmd_handler},
	{ "rmtcmd", FS_CMD_RMTCMD_ARGC, memx_fs_cmd_handler},
	{ "msi", FS_CMD_FWLOG_ARGC, memx_msi_trigger_handler},
};

s32 memx_msi_trigger_handler(struct memx_pcie_dev *memx_dev, u8 argc, char **argv)
{
	u8 i, msi_id  = 0;
	u32 msi_addr_lsb, msi_data, msi_enable;

	if (!memx_dev) {
		pr_err("memryx: fs_cmd_handler: memx_dev is NULL\n");
		return -EINVAL;
	}

	if (!argv) {
		pr_err("memryx: fs_cmd_handler: argv is NULL\n");
		return -EINVAL;
	}

	if (kstrtou8(argv[1], 0, &msi_id))
		return -EINVAL;

	if (msi_id >= memx_dev->int_info.curr_used_msix_count) {
		pr_err("memryx: invalid msi/msix id(%d) Max(%d)\n", msi_id, memx_dev->int_info.curr_used_msix_count-1);
		return -EINVAL;
	}

	if ((memx_xflow_read(memx_dev, 0, (MEMX_CHIPMSITABADDR+0x0)+msi_id*0x10, 0, false) == 0) && (memx_xflow_read(memx_dev, 0, (MEMX_CHIPMSITABADDR+0x4)+msi_id*0x10, 0, false) == 0)) {
		pr_err("memryx: invalid msi/msix id(%d) addr/data all 0.(legacy?)\n", msi_id);
		return -EINVAL;
	}

	msi_addr_lsb = memx_xflow_read(memx_dev, 0, (MEMX_CHIPMSITABADDR+0x0)+msi_id*0x10, 0, false) & 0x1FFFFF;
	msi_data     = memx_xflow_read(memx_dev, 0, (MEMX_CHIPMSITABADDR+0x8)+msi_id*0x10, 0, false);
	msi_enable   = memx_xflow_read(memx_dev, 0, (MEMX_CHIPMSITABADDR+0xC)+msi_id*0x10, 0, false);

	if (msi_enable & 0x1) {
		pr_err("memryx: msi/msix id(%d) is disabled\n", msi_id);
		return -EINVAL;
	}

	memx_xflow_write(memx_dev, 0, MEMX_CHIPMSIMAPADDR+msi_addr_lsb, 0, msi_data, false);

	/* Vector Table Dump */
	for (i=0; i < memx_dev->int_info.curr_used_msix_count; i++) {
		pr_info("memryx: (%2d): 0x%08X 0x%08X 0x%08X 0x%08X\n", i
		,  memx_xflow_read(memx_dev, 0, (MEMX_CHIPMSITABADDR+0x0)+i*0x10, 0, false)
		,  memx_xflow_read(memx_dev, 0, (MEMX_CHIPMSITABADDR+0x4)+i*0x10, 0, false)
		,  memx_xflow_read(memx_dev, 0, (MEMX_CHIPMSITABADDR+0x8)+i*0x10, 0, false)
		,  memx_xflow_read(memx_dev, 0, (MEMX_CHIPMSITABADDR+0xC)+i*0x10, 0, false));
	}

	pr_info("memryx: xflow write address 0x%X = 0x%X\n", MEMX_CHIPMSIMAPADDR+msi_addr_lsb, msi_data);
	pr_info("memryx: debug trigger msi/msix (%d)\n", msi_id);
	return 0;
}

s32 memx_fs_cmd_handler(struct memx_pcie_dev *memx_dev, u8 argc, char **argv)
{
	u8 chip_id  = 0;
	u32 reg_addr = 0, reg_value = 0;
	bool is_access_mpu = true;

	if (!memx_dev) {
		pr_err("memryx: fs_cmd_handler: memx_dev is NULL\n");
		return -EINVAL;
	}
	if (argc < FS_CMD_FWLOG_ARGC) {
		pr_err("memryx: fs_cmd_handler: invalid argc(%u), should be 2-4\n", argc);
		return -EINVAL;
	}
	if (!argv) {
		pr_err("memryx: fs_cmd_handler: argv is NULL\n");
		return -EINVAL;
	}

	if (argc >= FS_CMD_FWLOG_ARGC &&
		kstrtou8(argv[FS_CMD_FWLOG_ARGC - 1], 0, &chip_id) &&
		chip_id >= memx_dev->mpu_data.hw_info.chip.total_chip_cnt) {
		pr_err("memryx: fs_cmd_handler: invalid chip id(%u), should be 0 - %u\n", chip_id, memx_dev->mpu_data.hw_info.chip.total_chip_cnt);
		return -EINVAL;
	}

	if (((argc == FS_CMD_READ_ARGC) || (argc == FS_CMD_WRITE_ARGC) || (argc == FS_CMD_RMTCMD_ARGC)) && kstrtou32(argv[FS_CMD_READ_ARGC - 1], 0, &reg_addr)) {
		pr_err("memryx: fs_cmd_handler: invalid reg_addr:%s\n", argv[2]);
		return -EINVAL;
	}

	if (((argc == FS_CMD_WRITE_ARGC) || (argc == FS_CMD_RMTCMD_ARGC)) && kstrtou32(argv[FS_CMD_WRITE_ARGC - 1], 0, &reg_value)) {
		pr_err("memryx: fs_cmd_handler: invalid reg_value:%s\n", argv[3]);
		return -EINVAL;
	}

	// only xflow need to do this check
	if (argc >= FS_CMD_READ_ARGC && (reg_addr < MPU_REGISTER_START || reg_addr > MPU_REGISTER_END))
		is_access_mpu = false;

	dma_sync_single_for_cpu(&memx_dev->pDev->dev, (dma_addr_t)(memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base+MEMX_RMTCMD_CONTROLBASE_OFFSET), 0x2000, DMA_BIDIRECTIONAL);

	// TODO : if we want to add new cmd handler, we can change to take the cmd name(i.e argv[0]) to check, I use argc as switch flow just cause it's more faster.
	//		For now, I just comment all the real action for demo purpose with VM environemnt.
	switch (argc) {
	case FS_CMD_FWLOG_ARGC: {
		s32 ret = memx_fw_log_dump(memx_dev, chip_id);

		if (ret) {
			pr_err("memryx: fs_cmd_handler: try to dump chip_id(%u)'s fw_log failed(%d)\n", chip_id, ret);
			return ret;
		}
		pr_info("memryx: fs_cmd_handler: dump of chip_id(%u)'s fw_log success\n", chip_id);
	} break;
	case FS_CMD_READ_ARGC: {
		*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_PARAM_VIRTUAl_ADDR(memx_dev, chip_id))) = reg_addr;
		*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_COMMAND_VIRTUAl_ADDR(memx_dev, chip_id))) = MXCNST_MEMXR_CMD;
		//pr_info("fs_cmd_handler: read addr(0x%x) from chip_id(%u)= 0x%x is_access_mpu(%d)\n", reg_addr, chip_id, memx_xflow_read(memx_dev, BAR0, chip_id, reg_addr, 0, is_access_mpu), is_access_mpu);
	} break;
	case FS_CMD_WRITE_ARGC: {
		*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_PARAM_VIRTUAl_ADDR(memx_dev, chip_id)))   = reg_addr;
		*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_PARAM2_VIRTUAl_ADDR(memx_dev, chip_id)))  = reg_value;
		*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_COMMAND_VIRTUAl_ADDR(memx_dev, chip_id))) = MXCNST_MEMXW_CMD;

		//memx_xflow_write(memx_dev, BAR0, chip_id, reg_addr, 0, reg_value, is_access_mpu);
		//pr_info("fs_cmd_handler: write addr(0x%x) with val(0x%x) to chip_id(%u) is_access_mpu(%d)\n", reg_addr, reg_value, chip_id, is_access_mpu);
	} break;
	case FS_CMD_MEMX0_ARGC:
	case FS_CMD_MEMX1_ARGC:
	case FS_CMD_MEMX2_ARGC:
	case FS_CMD_MEMX3_ARGC:
	case FS_CMD_MEMX6_ARGC:
	case FS_CMD_MEMX7_ARGC:
	case FS_CMD_MEMX8_ARGC:
	case FS_CMD_MEMX9_ARGC: {
		u32 cmdbase = MXCNST_MEMX0_CMD;

		cmdbase = cmdbase + ((argc>>4) - (FS_CMD_MEMX0_ARGC>>4));
		*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_COMMAND_VIRTUAl_ADDR(memx_dev, chip_id))) = cmdbase;
	} break;
	case FS_CMD_RMTCMD_ARGC: {
		*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_PARAM_VIRTUAl_ADDR(memx_dev, chip_id))) = reg_value;
		*((_VOLATILE_ u32 *)(MEMX_GET_CHIP_RMTCMD_COMMAND_VIRTUAl_ADDR(memx_dev, chip_id))) = reg_addr;
	} break;

	default: {
		pr_err("memryx: fs_cmd_handler: expected argc[2-4] but (%u)\n", argc);
		return -EINVAL;
	}
	}

	dma_sync_single_for_device(&memx_dev->pDev->dev, (dma_addr_t)(memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base+MEMX_RMTCMD_CONTROLBASE_OFFSET), 0x2000, DMA_BIDIRECTIONAL);

	return 0;
}

s32 memx_fs_parse_cmd_and_exec(struct memx_pcie_dev *memx_dev, const char __user *user_input_buf, size_t user_input_buf_size)
{
	s32 ret = -EINVAL;
	char *found = NULL;
	const char delimiters[] = {' ', '\0'}; // space, \0
	char *argv[4] = {NULL, NULL, NULL, NULL}; // cmd_name, chip_id, addr, val
	u8 argc = 0;
	u8 cmd_idx = 0;

	char *input_parser_buffer_ptr = NULL;

	if (memx_dev->fs.type == MEMX_FS_HIF_SYS) {
		input_parser_buffer_ptr = kstrdup(user_input_buf, GFP_KERNEL);
		if (!input_parser_buffer_ptr) {
			//pr_err("memryx: memx_fs_parser: kstrdup fail!\n");
			return ret;
		}
	} else {
		input_parser_buffer_ptr = memdup_user_nul(user_input_buf, user_input_buf_size);
		if (IS_ERR(input_parser_buffer_ptr)) {
			pr_err("memryx: memx_fs_parser: memdup_user_nul fail!\n");
			return PTR_ERR(input_parser_buffer_ptr);
		}
	}

	// parse command string by deleimiters and get the final argc
	while ((found = strsep(&input_parser_buffer_ptr, delimiters)) != NULL)
		argv[argc++] = found;

	if (argc < FS_CMD_FWLOG_ARGC) {
		pr_err("memryx: memx_fs_parser: argc should be te range in 2-4 but (%u)\n", argc);
		return ret;
	}

	// check argc and name meet requirement
	for (cmd_idx = 0; cmd_idx < MEMX_NUM_OF_FS_CMDS; cmd_idx++) {
		if ((argc == (g_fs_cmd_tbl[cmd_idx].expected_argc & 0xF)) &&
			(strncmp(argv[0], g_fs_cmd_tbl[cmd_idx].name, strlen(g_fs_cmd_tbl[cmd_idx].name)) == 0)) {
			ret = g_fs_cmd_tbl[cmd_idx].exec(memx_dev, g_fs_cmd_tbl[cmd_idx].expected_argc, &argv[0]);
			break;
		}
	}

	kfree(input_parser_buffer_ptr);
	return ret;
}

s32 memx_fs_parse_i2ctrl_and_exec(struct memx_pcie_dev *memx_dev, const char __user *user_input_buf, size_t user_input_buf_size)
{
	s32 ret = -EINVAL;
	char *found = NULL;
	const char delimiters[] = {' ', '\0'}; // space, \0
	// [w-byte-cnt], wdata0, wdata1,..., [r-byte-cnt]
	u8 max_len = 17;
	char *argv[17] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	u8 argc = 0;
	char *input_parser_buffer_ptr = NULL;
	u32 wlen = 0, data, i;
	struct transport_cmd cmd = {0};
	u8 *pbuf8;

	if (memx_dev->fs.type == MEMX_FS_HIF_SYS) {
		input_parser_buffer_ptr = kstrdup(user_input_buf, GFP_KERNEL);
		if (!input_parser_buffer_ptr) {
			//pr_err("memx_fs_parser: kstrdup fail!\n");
			return ret;
		}
	} else {
		input_parser_buffer_ptr = memdup_user_nul(user_input_buf, user_input_buf_size);
		if (IS_ERR(input_parser_buffer_ptr)) {
			pr_err("memx_fs_parser: memdup_user_nul fail!\n");
			return PTR_ERR(input_parser_buffer_ptr);
		}
	}

	// parse command string by deleimiters and get the final argc
	while ((found = strsep(&input_parser_buffer_ptr, delimiters)) != NULL) {
		if (argc < max_len) {
			argv[argc++] = found;
			//pr_info("argv[%d]=%s\n", argc-1, argv[argc-1]);
		} else {
			argc++;
		}
	}

	if ((argc < FS_CMD_FWLOG_ARGC) || (argc > max_len)) {
		pr_err("memx_i2ctrl_parser: argc should be te range in 2-%d but (%u)\n", max_len, argc);
		return ret;
	}

	if (kstrtou32(argv[0], 0, &wlen)) {
		pr_err("kstrtou32 convert error 1\n");
		return ret;
	}

	if (wlen > max_len-1) {
		pr_err("memx_i2ctrl_parser: max write len is %d (%u)\n", max_len-1, wlen);
		return ret;
	}
	//pr_info("len=%u\r\n", wlen);
	wlen = wlen&0xFE;

	cmd.SQ.opCode = MEMX_ADMIN_CMD_SET_FEATURE;
	cmd.SQ.subOpCode = FID_DEVICE_I2C_TRANSCEIVE;
	cmd.SQ.reqLen = wlen>>1;
	pbuf8 = (u8 *) &(cmd.SQ.cdw3);
	for (i = 0; i < wlen; i++) {
		if (!kstrtou32(argv[i+1], 0, &data))
			pbuf8[i] = data & 0xFF;
	}

	mutex_lock(&memx_dev->adminlock);
	memx_admin_trigger(memx_dev, 0, &cmd);
	cmd.CQ.status = memx_admin_fetch_result(memx_dev, 0, &cmd);
	mutex_unlock(&memx_dev->adminlock);
	pbuf8 = (u8 *) &(cmd.CQ.data[3]);
	pr_info("--------------------\n");
	for (i = 0; i < (wlen >> 1); i++) {
		if (pbuf8[(i<<1)+1] & 0x10)
			pr_info("R data[%d]=0x%02X - OK\n", i, pbuf8[(i<<1)]);
		else if (pbuf8[(i<<1)+1] & 0x4)
			pr_info("W data[%d]=0x%02X - I2C_NACK\n", i, pbuf8[(i<<1)]);
		else
			pr_info("W data[%d]=0x%02X - OK\n", i, pbuf8[(i<<1)]);
	}

	kfree(input_parser_buffer_ptr);
	return 0;
}

s32 memx_fs_parse_gpioctrl_and_exec(struct memx_pcie_dev *memx_dev, const char __user *user_input_buf, size_t user_input_buf_size)
{
	s32 ret = -EINVAL;
	char *found = NULL;
	const char delimiters[] = {' ', '\0'}; // space, \0
	u8 max_len = 3;
	char *argv[3] = {NULL, NULL, NULL};
	u8 argc = 0;
	char *input_parser_buffer_ptr = NULL;
	struct transport_cmd cmd = {0};
	u32 gpio_number = 0, gpio_value = 0;

	if (memx_dev->fs.type == MEMX_FS_HIF_SYS) {
		input_parser_buffer_ptr = kstrdup(user_input_buf, GFP_KERNEL);
		if (!input_parser_buffer_ptr) {
			//pr_err("memx_fs_parser: kstrdup fail!\n");
			return ret;
		}
	} else {
		input_parser_buffer_ptr = memdup_user_nul(user_input_buf, user_input_buf_size);
		if (IS_ERR(input_parser_buffer_ptr)) {
			pr_err("memx_fs_parser: memdup_user_nul fail!\n");
			return PTR_ERR(input_parser_buffer_ptr);
		}
	}

	// parse command string by deleimiters and get the final argc
	while ((found = strsep(&input_parser_buffer_ptr, delimiters)) != NULL) {
		if (argc < max_len) {
			argv[argc++] = found;
			//pr_info("argv[%d]=%s\n", argc-1, argv[argc-1]);
		} else {
			argc++;
		}
	}

	if ((argc < FS_CMD_FWLOG_ARGC) || (argc > max_len)) {
		pr_err("memx_i2ctrl_parser: argc should be te range in 2-%d but (%u)\n", max_len, argc);
		return ret;
	}

	if (kstrtou32(argv[1], 0, &gpio_number)) {
		pr_err("%s: kstrtou32 convert error\n", __func__);
		return ret;
	}
	gpio_number = gpio_number & 0xFFF;

	if (strncmp(argv[0], "r", 1) == 0) {
		memx_dev->gpio_r = gpio_number;
		return 0;
	} else if ((strncmp(argv[0], "w", 1) == 0) && (argc == 3)) {
		if (kstrtou32(argv[2], 0, &gpio_value)) {
			pr_err("%s: kstrtou32 convert error\n", __func__);
			return ret;
		}
	} else {
		pr_err("%s: command error (%s)(%d)\n", __func__, argv[0], argc);
		return ret;
	}

	cmd.SQ.opCode    = MEMX_ADMIN_CMD_SET_FEATURE;
	cmd.SQ.subOpCode = FID_DEVICE_GPIO; 
	cmd.SQ.cdw3      = (gpio_number >> 0) & 0xFF;
	cmd.SQ.cdw4      = gpio_value > 0;

	mutex_lock(&memx_dev->adminlock);
	memx_admin_trigger(memx_dev, (gpio_number >> 8) & 0xF, &cmd);
	cmd.CQ.status = memx_admin_fetch_result(memx_dev, (gpio_number >> 8) & 0xF, &cmd);
	mutex_unlock(&memx_dev->adminlock);

	kfree(input_parser_buffer_ptr);
	return 0;
}

void memx_fs_get_frequency(struct memx_pcie_dev *memx_dev, u32 *data, u8 chip_id)
{
	struct transport_cmd cmd = {0};

	cmd.SQ.opCode    = MEMX_ADMIN_CMD_GET_FEATURE;
	cmd.SQ.subOpCode = FID_DEVICE_FREQUENCY_EFFECTIVE;

	mutex_lock(&memx_dev->adminlock);
	memx_admin_trigger(memx_dev, chip_id, &cmd);
	cmd.CQ.status = memx_admin_fetch_result(memx_dev, chip_id, &cmd);
	mutex_unlock(&memx_dev->adminlock);

	if (cmd.CQ.status == 0) {
		data[0] = cmd.CQ.data[0];
	}

	memset(&cmd, 0, sizeof(struct transport_cmd));
	cmd.SQ.opCode    = MEMX_ADMIN_CMD_GET_FEATURE;
	cmd.SQ.subOpCode = FID_DEVICE_FREQUENCY;

	mutex_lock(&memx_dev->adminlock);
	memx_admin_trigger(memx_dev, chip_id, &cmd);
	cmd.CQ.status = memx_admin_fetch_result(memx_dev, chip_id, &cmd);
	mutex_unlock(&memx_dev->adminlock);

	if (cmd.CQ.status == 0) {
		data[1] = cmd.CQ.data[0];
	}
}

u32 memx_crc32(const uint8_t *data, size_t length)
{
	u32 *crc32_table;
	u32 polynomial = 0xEDB88320;
	u32 crc32 = 0xFFFFFFFF;
	u32 i = 0, j = 0, crc = 0;
	u8 byte = 0;

	crc32_table = vmalloc(256 * sizeof(u32));
	if (crc32_table == NULL) {
		//pr_err("memryx: %s: malloc crc32_table failed.\n", __func__);
		return 0;
	}

	// init crc32 table
	for (i = 0; i < 256; i++) {
		crc = i;
		for (j = 8; j > 0; j--) {
			if (crc & 1)
				crc = (crc >> 1) ^ polynomial;
			else
				crc >>= 1;
		}
		crc32_table[i] = crc;
	}

	// compute the CRC32 checksum
	for (i = 0; i < length; i++) {
		byte = data[i];
		crc32 = (crc32 >> 8) ^ crc32_table[(crc32 ^ byte) & 0xFF];
	}

	vfree(crc32_table);
	return crc32 ^ 0xFFFFFFFF;
}

s32 memx_fs_init(struct memx_pcie_dev *memx_dev)
{
	s32 ret = -EINVAL;

	if (!memx_dev) {
		pr_err("memryx: memx_dev is NULL\n");
		return ret;
	}

	switch (memx_dev->fs.type) {
	case MEMX_FS_HIF_PROC: {
		ret = memx_fs_proc_init(memx_dev);
	} break;
	case MEMX_FS_HIF_SYS: {
		ret = memx_fs_sys_init(memx_dev);
	} break;
	default:
		pr_err("memryx: %s: non support file system type(%u)\n", __func__, memx_dev->fs.type);
		return ret;
	}

	if (!ret)
		ret = memx_fs_hwmon_init(memx_dev);

	return ret;
}

void memx_fs_deinit(struct memx_pcie_dev *memx_dev)
{
	if (!memx_dev) {
		pr_err("memryx: %s: memx_dev is NULL\n", __func__);
		return;
	}

	switch (memx_dev->fs.type) {
	case MEMX_FS_HIF_PROC: {
		memx_fs_proc_deinit(memx_dev);
	} break;
	case MEMX_FS_HIF_SYS: {
		memx_fs_sys_deinit(memx_dev);
	} break;
	default:
		pr_err("memryx: %s: non support file system type(%u)\n", __func__, memx_dev->fs.type);
	}

	memx_fs_hwmon_deinit(memx_dev);
}
