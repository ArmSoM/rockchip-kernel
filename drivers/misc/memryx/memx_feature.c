// SPDX-License-Identifier: GPL-2.0+
#include <linux/module.h>
#include <linux/version.h>
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#include <linux/time.h>
#include "memx_pcie.h"
#include "memx_xflow.h"
#include "memx_pcie_dev_list_ctrl.h"

#define DATA_BEGIN_CHIP_0 (0)
#define DATA_END_CHIP_0 CQ_DATA_LEN

#define THROUGHPUT_DATA_BEGIN_CHIP_0 DATA_BEGIN_CHIP_0
#define THROUGHPUT_DATA_BEGIN_CHIP_LAST (4)
#define THROUGHPUT_DATA_END_CHIP_LAST (8)

#define MEMX_GET_CHIP_ADMIN_CMD_BASE_VIRTUAL_ADDR(memx_dev, chip_id) (((memx_dev)->mpu_data.rx_dma_coherent_buffer_virtual_base) + \
																		MEMX_ADMCMD_VIRTUAL_OFFSET + ((chip_id) * MEMX_ADMCMD_SIZE))
void memx_admin_trigger(struct memx_pcie_dev *memx_dev, uint8_t chip_id, struct transport_cmd *pCmd);
void memx_admin_trigger(struct memx_pcie_dev *memx_dev, uint8_t chip_id, struct transport_cmd *pCmd)
{
	uint32_t *cmd =  (uint32_t *) (MEMX_GET_CHIP_ADMIN_CMD_BASE_VIRTUAL_ADDR(memx_dev, chip_id));
	memcpy((void *)cmd, pCmd, sizeof(struct transport_cmd));
	cmd[U32_ADMCMD_STATUS_OFFSET] = STATUS_RECEIVE;
	dma_sync_single_for_device(&memx_dev->pDev->dev, (dma_addr_t)(memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base + MEMX_ADMCMD_VIRTUAL_PAGE_OFFSET), MEMX_ADMCMD_VIRTUAL_PAGE_SIZE, DMA_BIDIRECTIONAL);
}

static void memx_admin_data_from_device(struct memx_pcie_dev *memx_dev, uint8_t chip_id, struct transport_cmd *cmd)
{
	uint32_t index = 0;
	uint8_t read_data_start = 0;
	uint8_t read_data_end = 0;
	uint32_t read_offset = U32_ADMCMD_CQ_DATA_OFFSET;

	if (cmd->SQ.subOpCode == FID_DEVICE_THROUGHPUT && cmd->SQ.opCode == MEMX_ADMIN_CMD_GET_FEATURE) {
		read_data_start = (chip_id == CHIP_ID0) ? THROUGHPUT_DATA_BEGIN_CHIP_0 : THROUGHPUT_DATA_BEGIN_CHIP_LAST;
		read_data_end   = (chip_id == CHIP_ID0) ? THROUGHPUT_DATA_BEGIN_CHIP_LAST : THROUGHPUT_DATA_END_CHIP_LAST;
	} else {
		read_data_start = DATA_BEGIN_CHIP_0;
		read_data_end = DATA_END_CHIP_0;
	}
	
	if (cmd->SQ.subOpCode == FID_DEVICE_I2C_TRANSCEIVE)
		read_offset = 0;

	dma_sync_single_for_cpu(&memx_dev->pDev->dev, (dma_addr_t)(memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base + MEMX_ADMCMD_VIRTUAL_PAGE_OFFSET), MEMX_ADMCMD_VIRTUAL_PAGE_SIZE, DMA_BIDIRECTIONAL);
	for (index = read_data_start; index < read_data_end; index++) {
		uint32_t *AdminCmd =  (uint32_t *) (MEMX_GET_CHIP_ADMIN_CMD_BASE_VIRTUAL_ADDR(memx_dev, chip_id));
		cmd->CQ.data[index] = AdminCmd[read_offset + index];
	}
}
enum CASCADE_PLUS_ADMINCMD_ERROR_STATUS memx_admin_fetch_result(struct memx_pcie_dev *memx_dev, uint8_t chip_id, struct transport_cmd *cmd);
enum CASCADE_PLUS_ADMINCMD_ERROR_STATUS memx_admin_fetch_result(struct memx_pcie_dev *memx_dev, uint8_t chip_id, struct transport_cmd *cmd)
{
	enum CASCADE_PLUS_ADMINCMD_STATUS device_status = STATUS_IDLE;
	enum CASCADE_PLUS_ADMINCMD_ERROR_STATUS error_status = ERROR_STATUS_NO_ERROR;
	unsigned long timeout;
	uint32_t *AdminCmd = NULL;
	uint8_t subOpCode = cmd->SQ.subOpCode;

	timeout = jiffies + (HZ * 3);
	AdminCmd =  (uint32_t *) (MEMX_GET_CHIP_ADMIN_CMD_BASE_VIRTUAL_ADDR(memx_dev, chip_id));

	do {
		dma_sync_single_for_cpu(&memx_dev->pDev->dev, (dma_addr_t)(memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base + MEMX_ADMCMD_VIRTUAL_PAGE_OFFSET), MEMX_ADMCMD_VIRTUAL_PAGE_SIZE, DMA_BIDIRECTIONAL);
		device_status = AdminCmd[U32_ADMCMD_STATUS_OFFSET];

		if (device_status == STATUS_COMPLETE) {
			error_status = AdminCmd[U32_ADMCMD_CQ_STATUS_OFFSET];

			if (error_status != ERROR_STATUS_NO_ERROR)
				pr_err("memryx: admin error subOpCode %d\n", subOpCode);
			else
				memx_admin_data_from_device(memx_dev, chip_id, cmd);

		} else if (time_after(jiffies, timeout)) {
			error_status = ERROR_STATUS_TIMEOUT_FAIL;
			pr_err("memryx: admin timeout device status %d subop %d chip %d\n", device_status, subOpCode, chip_id);
			break;
		}
	} while (device_status != STATUS_COMPLETE);

	AdminCmd[U32_ADMCMD_STATUS_OFFSET] = STATUS_IDLE;
	dma_sync_single_for_device(&memx_dev->pDev->dev, (dma_addr_t)(memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base + MEMX_ADMCMD_VIRTUAL_PAGE_OFFSET), MEMX_ADMCMD_VIRTUAL_PAGE_SIZE, DMA_BIDIRECTIONAL);

	return error_status;
}

static long _admin_get_feature(struct memx_pcie_dev *memx_dev, struct transport_cmd *pCmd){
	long 	ret 	= 0;

	if (pCmd->SQ.subOpCode == FID_DEVICE_THROUGHPUT) {
		pCmd->CQ.data[0]  = tx_time_us;
		pCmd->CQ.data[1]  = tx_size / KBYTE;
		pCmd->CQ.data[2] = rx_time_us;
		pCmd->CQ.data[3] = rx_size / KBYTE;
		pCmd->CQ.data[4] = udrv_throughput_info.stream_write_us;
		pCmd->CQ.data[5] = udrv_throughput_info.stream_write_kb;
		pCmd->CQ.data[6] = udrv_throughput_info.stream_read_us;
		pCmd->CQ.data[7] = udrv_throughput_info.stream_read_kb;
		tx_time_us = 0;
		tx_size = 0;
		rx_time_us = 0;
		rx_size = 0;
		udrv_throughput_info.stream_write_us = 0;
		udrv_throughput_info.stream_write_kb = 0;
		udrv_throughput_info.stream_read_us = 0;
		udrv_throughput_info.stream_read_kb = 0;
	} else if (pCmd->SQ.subOpCode == FID_DEVICE_INTERFACE_INFO) {
		int offset = pci_find_capability(memx_dev->pDev, PCI_CAP_ID_EXP);
		if (offset == 0) {
			pr_err("memryx: failed to find capability\n");
			ret = -ENODEV;
		} else {
			pci_read_config_dword(memx_dev->pDev, offset + PCI_EXP_LNKCAP, &pCmd->CQ.data[0]);
			pci_read_config_word(memx_dev->pDev, offset + PCI_EXP_LNKSTA, (u16*)&pCmd->CQ.data[1]);
		}
	} else if ((pCmd->SQ.subOpCode == FID_DEVICE_POWERMANAGEMENT) || (pCmd->SQ.subOpCode == FID_DEVICE_FREQUENCY) || (pCmd->SQ.subOpCode == FID_DEVICE_GPIO) ||
               (pCmd->SQ.subOpCode == FID_DEVICE_DMA_TRIGGER_TYPE)) {
		uint8_t chip_id = pCmd->SQ.cdw2;

		if (chip_id < memx_dev->mpu_data.hw_info.chip.total_chip_cnt) {
			memx_admin_trigger(memx_dev, chip_id, pCmd);
			pCmd->CQ.status = memx_admin_fetch_result(memx_dev, chip_id, pCmd);

			if (pCmd->SQ.subOpCode == FID_DEVICE_DMA_TRIGGER_TYPE) {
				memx_dev->mpu_data.hw_info.chip.input_dma_trigger_type[chip_id] = pCmd->CQ.data[0];
			}
		} else {
			pCmd->CQ.status = ERROR_STATUS_PARAMETER_FAIL;
		}
	} else if (pCmd->SQ.subOpCode == FID_DEVICE_HW_INFO) {
		pCmd->CQ.data[0] = memx_dev->mpu_data.hw_info.chip.generation;
		pCmd->CQ.data[1] = memx_dev->mpu_data.hw_info.chip.total_chip_cnt;
		pCmd->CQ.data[2] = memx_dev->mpu_data.hw_info.chip.curr_config_chip_count;
		pCmd->CQ.data[3] = memx_dev->mpu_data.hw_info.chip.group_count;
		pCmd->CQ.status = ERROR_STATUS_NO_ERROR;
	} else if (pCmd->SQ.subOpCode == FID_DEVICE_MPU_UTILIZATION) {
		pCmd->CQ.data[0] = memx_sram_read(memx_dev, (MXCNST_MPUUTIL_BASE+(pCmd->SQ.cdw2 << 2)));
		pCmd->CQ.status = ERROR_STATUS_NO_ERROR;
	} else {
		memx_admin_trigger(memx_dev, CHIP_ID0, pCmd);
		pCmd->CQ.status = memx_admin_fetch_result(memx_dev, CHIP_ID0, pCmd);
	}

	if (pCmd->SQ.subOpCode == FID_DEVICE_INFO) {
		char version[8] = PCIE_VERSION;

		memcpy(&pCmd->CQ.data[7], version, sizeof(unsigned int));
		memcpy(&pCmd->CQ.data[8], &version[sizeof(unsigned int)], sizeof(unsigned int));
	}

	return ret;
}

static long _admin_set_feature(struct memx_pcie_dev *memx_dev, struct transport_cmd *pCmd){
    long    ret     = 0;
    uint8_t chip_id = pCmd->SQ.cdw2;

	if (pCmd->SQ.subOpCode == FID_DEVICE_DMA_TRIGGER_TYPE) {
		memx_dev->mpu_data.hw_info.chip.input_dma_trigger_type[chip_id] = pCmd->SQ.cdw3;
	}

    if (chip_id < memx_dev->mpu_data.hw_info.chip.total_chip_cnt) {
        memx_admin_trigger(memx_dev, chip_id, pCmd);
        pCmd->CQ.status = memx_admin_fetch_result(memx_dev, chip_id, pCmd);
    } else {
        pCmd->CQ.status = ERROR_STATUS_PARAMETER_FAIL;
    }

	return ret;
}
//not finished
static long _admin_download_dfp(struct memx_pcie_dev *memx_dev, struct transport_cmd *pCmd){
	long                    ret 	                = 0;
	const uint8_t           start_index             = 4;
	const uint8_t           max_parallel_chip_count = 4;

	struct transport_cmd    cmd_k[4];
	uint8_t                 index                   = 0;
	uint32_t                des_type                = 0;

	memset(&cmd_k, 0, sizeof(struct transport_cmd) * max_parallel_chip_count);

	//sync dfp data area
	dma_sync_single_for_device(&memx_dev->pDev->dev, (dma_addr_t)memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, DMA_COHERENT_BUFFER_SIZE_1MB, DMA_BIDIRECTIONAL);

	//prepare SQ
	for(index = 0; index < max_parallel_chip_count; index++){
		if(pCmd->sq_data[start_index + 3 * index]){
			cmd_k[index].SQ.opCode = pCmd->SQ.opCode;
			cmd_k[index].SQ.cmdLen = pCmd->SQ.cmdLen;
			cmd_k[index].SQ.subOpCode = pCmd->SQ.subOpCode;
			cmd_k[index].SQ.reqLen = pCmd->SQ.reqLen;
			cmd_k[index].SQ.attr = ((pCmd->sq_data[start_index + 3 * index] & 0xFF000000) >> 24);
			cmd_k[index].SQ.cdw2 =  (pCmd->sq_data[start_index + 3 * index] & 0x000000FF);

			des_type = ((pCmd->sq_data[start_index + 3 * index] & 0x0000FF00) >> 8);
			cmd_k[index].SQ.cdw3 = (des_type != 0x38) ? (des_type << 24) : ((des_type << 24)|0x800000);
			cmd_k[index].SQ.cdw4 =  pCmd->sq_data[start_index + 3 * index + 1];
			cmd_k[index].SQ.cdw5 =  pCmd->sq_data[start_index + 3 * index + 2];
			cmd_k[index].SQ.cdw6 = ((pCmd->sq_data[start_index + 3 * index] & 0x00FF0000) >> 16);
		}
	}

	//trigger admin command
	for(index = 0; index < max_parallel_chip_count; index++){
		if(cmd_k[index].SQ.opCode){
			memx_admin_trigger(memx_dev, cmd_k[index].SQ.cdw2, &cmd_k[index]);
		}
	}

	//polling admin command done, require all command done
	for(index = 0; index < max_parallel_chip_count; index++){
		if(cmd_k[index].SQ.opCode && (cmd_k[index].SQ.cdw2 < memx_dev->mpu_data.hw_info.chip.total_chip_cnt)){
			cmd_k[index].CQ.status = memx_admin_fetch_result(memx_dev, cmd_k[index].SQ.cdw2, &cmd_k[index]);
			if(cmd_k[index].CQ.status != ERROR_STATUS_NO_ERROR){
				pCmd->CQ.status = cmd_k[index].CQ.status;
				break;
			}
		}
	}

	return ret;
}

static long _admin_selftest(struct memx_pcie_dev *memx_dev, struct transport_cmd *pCmd){
    long    ret     = 0;
    uint8_t chip_id = pCmd->SQ.cdw2;

    if (chip_id < memx_dev->mpu_data.hw_info.chip.total_chip_cnt) {
        memx_admin_trigger(memx_dev, chip_id, pCmd);
        pCmd->CQ.status = memx_admin_fetch_result(memx_dev, chip_id, pCmd);
    } else {
        pCmd->CQ.status = ERROR_STATUS_PARAMETER_FAIL;
    }

	return ret;
}

static long _admin_command(struct memx_pcie_dev *memx_dev, struct transport_cmd *pCmd){
	long ret = 0;

	mutex_lock(&memx_dev->adminlock);

	switch (pCmd->SQ.opCode) {
		case MEMX_ADMIN_CMD_SET_FEATURE:
			ret = _admin_set_feature(memx_dev, pCmd);
			break;
		case MEMX_ADMIN_CMD_GET_FEATURE:
			ret = _admin_get_feature(memx_dev, pCmd);
			break;
		case MEMX_ADMIN_CMD_DOWNLOAD_DFP:
			ret = _admin_download_dfp(memx_dev, pCmd);
			break;
		case MEMX_ADMIN_CMD_SELFTEST:
			ret = _admin_selftest(memx_dev, pCmd);
			break;
		case MEMX_ADMIN_CMD_DEVIOCTRL:
			if (pCmd->SQ.subOpCode == FID_DEVICE_I2C_TRANSCEIVE) {
				pCmd->SQ.opCode = MEMX_ADMIN_CMD_SET_FEATURE;
				ret = _admin_set_feature(memx_dev, pCmd);
			} else {
				ret = -EFAULT;
				pr_err(" _admin_command: non-support admin cmd(%u) sub(%u)\n", pCmd->SQ.opCode, pCmd->SQ.subOpCode);
			}
			break;
		default:
			ret = -EFAULT;
			pr_err("memryx: _admin_command: unsupported admin cmd(%u)\n", pCmd->SQ.opCode);
			break;
	}

	mutex_unlock(&memx_dev->adminlock);

	return ret;
}

static long memx_admin_ioctl(struct file *filp, u32 cmd, unsigned long arg)
{
	long ret = 0;
	struct memx_pcie_dev *memx_dev = NULL;
	struct transport_cmd sCmd = {0};
	u32 major = 0;
	u32 minor = 0;

	if (!filp) {
		pr_err("memryx: feature_ioctl: invalid parameters\n");
		return -ENODEV;
	}
	major = imajor(filp->f_inode);
	minor = iminor(filp->f_inode);
#ifdef DEBUG
	pr_info("memryx: feature_ioctl: device(major(%d)-minor(%d)), cmd:0x%x\n", major, minor, _IOC_NR(cmd));
#endif
	if (_IOC_TYPE(cmd) != MEMX_IOC_MAJOR) {
		pr_err("memryx: feature_ioctl: _IOC_TYPE(cmd) != MEMX_IOC_MAGIC\n");
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > MEMX_IOC_MAXNR) {
		pr_err("memryx: feature_ioctl: _IOC_NR(cmd) > MEMX_IOC_MAXNR(%d)\n", MEMX_IOC_MAXNR);
		return -ENOTTY;
	}

	memx_dev = (struct memx_pcie_dev *)filp->private_data;
	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: feature_ioctl: No Opened Device!\n");
		return -ENODEV;
	}

	if (copy_from_user(&sCmd, (struct transport_cmd *)arg, sizeof(struct transport_cmd))) {
		pr_err("memryx: feature_ioctl copy_from_user failed\n");
		ret = -ENOMEM;
		goto done;
	}

	switch (cmd) {
		case MEMX_GET_DEVICE_FEATURE: //backward
		case MEMX_SET_DEVICE_FEATURE: //backward
		case MEMX_ADMIN_DOWNLOAD_DFP: //backward
		case MEMX_ADMIN_COMMAND:
			ret = _admin_command(memx_dev, &sCmd);
			break;
		default:
			ret = -EFAULT;
			pr_err("memryx: feature_ioctl: (%u-%u): unsupported ioctl cmd(%u)\n", major, minor, cmd);
			break;
	}

	if (copy_to_user((void __user *)arg, &sCmd, sizeof(struct transport_cmd))) {
		pr_err("memryx: feature_ioctl: copy_to_user failed\n");
		ret = -ENOMEM;
		goto done;
	}

done:
#ifdef DEBUG
	pr_info("memryx: feature_ioctl: (%u-%u): finished\n", major, minor);
#endif
	return ret;
}

static s32 memx_admin_open(struct inode *inode, struct file *filp)
{
	u32 minor = iminor(filp->f_inode);
#ifdef DEBUG
	u32 major = imajor(filp->f_inode);
#endif

	struct memx_pcie_dev *memx_dev = memx_get_device_by_index(minor);

	if (!memx_dev) {
		pr_err("memryx: feature_open: PCIe device not found for /dev/memx%d node\n", minor);
		return -ENODEV;
	}
	filp->private_data = memx_dev;

#ifdef DEBUG
	pr_info("memryx: feature_open: open on /dev/memx%d_feature (%d-%d), vendor_id(%0x), devid_id(%0x))\n",
		memx_dev->minor_index, major, minor, (memx_dev ? memx_dev->pDev->vendor : 0), (memx_dev ? memx_dev->pDev->device : 0));
#endif
	return 0;
}

static s32 memx_admin_release(struct inode *inode, struct file *filp)
{
#ifdef DEBUG
	u32 major = imajor(filp->f_inode);
	u32 minor = iminor(filp->f_inode);
#endif

#ifdef DEBUG
	pr_info("memryx: feature_close: (%d-%d) success\n", major, minor);
#endif
	return 0;
}

struct file_operations memx_feature_fops = {
owner: THIS_MODULE,
unlocked_ioctl : memx_admin_ioctl,
open : memx_admin_open,
release : memx_admin_release,
};
