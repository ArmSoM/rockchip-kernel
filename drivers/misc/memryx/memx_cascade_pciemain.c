// SPDX-License-Identifier: GPL-2.0+
#include <linux/module.h>
#include <linux/version.h>
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#include <linux/time.h>
#include "memx_pcie.h"
#include "memx_pcie_dev_list_ctrl.h"
#include "memx_fw_cmd.h"
#include "memx_fw_init.h"
#include "memx_fs.h"
#include "memx_dma.h"

dev_t g_memx_devno;
dev_t g_feature_devno;
static struct class *g_char_device_class;
static u32 g_drv_fs_type = MEMX_FS_HIF_SYS;
static u32 fs_debug_en;
static u32 pcie_lane_no = 2;
static u32 pcie_lane_speed = 3;
static u32 pcie_aspm;
static u32 msix = 1;
u32 mxmf_boot_tick = 30;

ktime_t tx_start_time = 0, tx_end_time = 0;
ktime_t rx_start_time = 0, rx_end_time = 0;
u32 tx_time_us = 0, rx_time_us = 0;
u32 tx_size = 0, rx_size = 0;
static u32 dma_cohernet_buffer_size = DMA_COHERENT_BUFFER_SIZE_2MB;
struct memx_throughput_info udrv_throughput_info = {0};
struct hitcount_info g_hitcount_info = {0};

module_param(g_drv_fs_type, uint, 0);
MODULE_PARM_DESC(g_drv_fs_type, "debugfs control:: 0-Disable debugfs  1-proc filesys  2-sysfs filesys(default)");
module_param(fs_debug_en, uint, 0);
MODULE_PARM_DESC(fs_debug_en, "debugfs's debug option:: 0-Disable(default)  1-Enable");
module_param(pcie_lane_no, uint, 0);
MODULE_PARM_DESC(pcie_lane_no, "Internal chip2chip pcie link lane number. ValidRange: 1/2. 2 is default");
module_param(pcie_lane_speed, uint, 0);
MODULE_PARM_DESC(pcie_lane_speed, "Internal chip2chip pcie link speed. ValidRange: 1/2/3. 3 is default means GEN3");
module_param(pcie_aspm, uint, 0);
MODULE_PARM_DESC(pcie_aspm, "Internal chip2chip pcie link aspm control:: 0-FW_default(default) 1-L0_only 2-L0sL1 3-L0sL1.1");
module_param(mxmf_boot_tick, uint, 0);
MODULE_PARM_DESC(mxmf_boot_tick, "MXMF wait boot tick:: Around 30 ticks equals 1 second(default: 30)");
module_param(msix, uint, 0);
MODULE_PARM_DESC(msix, "msix enable:: 0-alloc msi only  1-alloc msix first and then msi(default)");

#define THROUGHPUT_ADD(current_size, additional_size) \
	do { \
		if ((current_size) > 0xffffffff - (additional_size)) { \
			tx_time_us = 0; \
			rx_time_us = 0; \
			tx_size = 0; \
			tx_size = 0; \
		} else { \
			(current_size) += (additional_size); \
		} \
	} while (0)

#if KERNEL_VERSION(6, 2, 0) > _LINUX_VERSION_CODE_
static char *memx_pcie_devnode(struct device *dev, umode_t *mode)
#else
static char *memx_pcie_devnode(const struct device *dev, umode_t *mode)
#endif
{
	if (!mode)
		return NULL;

	if ((dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID0)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID1)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID2)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID3)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID4)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID5)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID6)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID7)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID8)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID9)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID10)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID11)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID12)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID13)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID14)) ||
		(dev->devt == MKDEV(MAJOR(g_memx_devno), CHIP_ID15)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID0)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID1)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID2)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID3)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID4)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID5)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID6)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID7)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID8)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID9)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID10)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID11)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID12)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID13)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID14)) ||
		(dev->devt == MKDEV(MAJOR(g_feature_devno), CHIP_ID15))) {
		*mode = DEVICE_NODE_DEFAULT_ACCESS_RIGHT;
	}
	return NULL;
}

static s32 memx_pcie_abort_transfer(struct memx_pcie_dev *memx_dev)
{
	u8 chip_id = 0;

	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: pcie_abort_transfer: failed with -ENODEV\n");
		return -ENODEV;
	}
	memx_dev->mpu_data.rx_ctrl.is_abort = 1;
	memx_dev->mpu_data.rx_ctrl.indicator = -1;

	if (!memx_dev->mpu_data.rx_ctrl.is_read_abort) {
		spin_lock(&memx_dev->mpu_data.rx_ctrl.lock);
		while (!kfifo_is_empty(&memx_dev->rx_msix_fifo))
			kfifo_skip(&memx_dev->rx_msix_fifo);

		spin_unlock(&memx_dev->mpu_data.rx_ctrl.lock);
	}

	for (chip_id = 0; chip_id < MAX_SUPPORT_CHIP_NUM; chip_id++) {
		memx_dev->mpu_data.tx_ctrl[chip_id].is_abort = 1;
		memx_dev->mpu_data.tx_ctrl[chip_id].indicator = -1;
		wake_up_interruptible(&memx_dev->mpu_data.tx_ctrl[chip_id].wq);
	}
	memx_dev->mpu_data.fw_ctrl.is_abort = 1;
	memx_dev->mpu_data.fw_ctrl.indicator = -1;
	wake_up_interruptible(&memx_dev->mpu_data.rx_ctrl.wq);
	wake_up_interruptible(&memx_dev->mpu_data.fw_ctrl.wq);
#ifdef DEBUG
	pr_info("memryx: pcie_abort_transfer success\n");
#endif
	return 0;
}

static ssize_t memx_pcie_dummy_read(struct memx_pcie_dev *memx_dev)
{
	s32 indicator = -ERESTARTSYS;
	s32 wq_status = 0;

	if (!memx_dev || !memx_dev->pDev)
		pr_info("memryx: %s: warning -ENODEV\n", __func__);

	// Read data from device until there is empty.
	if (!kfifo_out_locked(&memx_dev->rx_msix_fifo, &indicator, sizeof(s32), &memx_dev->mpu_data.rx_ctrl.lock)) {
		wq_status = wait_event_interruptible_timeout(memx_dev->mpu_data.rx_ctrl.wq, (kfifo_len(&memx_dev->rx_msix_fifo) != 0), msecs_to_jiffies(100));

		if (wq_status == -ERESTARTSYS)
			pr_warn("memryx: fops_read: cancelled by interrupt signal\n");

		if (wq_status >= 1) {
			if (!kfifo_out_locked(&memx_dev->rx_msix_fifo, &indicator, sizeof(s32), &memx_dev->mpu_data.rx_ctrl.lock)) {
				pr_err("memryx: fops_read: kfifo_out is empty!\n");
				indicator = -EFAULT;
				return indicator;
			}
		}
	}

	return indicator;
}

static void memx_pcie_set_abort_read(struct memx_pcie_dev *memx_dev)
{
	memx_dev->mpu_data.rx_ctrl.is_read_abort = 1;
}

static s32 memx_pcie_config_mpu_group(struct memx_pcie_dev *memx_dev, struct hw_info *hw_info)
{
	u8 chip_id = 0;
	s32 ret = 0;

#ifdef DEBUG
	pr_info("memryx: into %s\n", __func__);
#endif
	for (chip_id = 0; chip_id < MAX_SUPPORT_CHIP_NUM; chip_id++)
		memx_dev->mpu_data.hw_info.chip.roles[chip_id] = hw_info->chip.roles[chip_id];

	memx_send_cmd_to_fw_and_get_result(memx_dev, PCIE_CMD_CONFIG_MPU_GROUP, 256, CHIP_ID0);
	ret = memx_get_hw_info(memx_dev);

	return ret;
}

static long memx_fops_ioctl(struct file *filp, u32 cmd, unsigned long arg)
{
	long ret = 0;
	struct memx_pcie_dev *memx_dev = NULL;
	u32 major = 0;
	u32 minor = 0;

	if (!filp) {
		pr_err("memryx: fops_ioctl: invalid parameters\n");
		return -ENODEV;
	}
	major = imajor(filp->f_inode);
	minor = iminor(filp->f_inode);
#ifdef DEBUG
	pr_info("memryx: fops_ioctl: device(major(%d)-minor(%d)), cmd:0x%x\n", major, minor, _IOC_NR(cmd));
#endif
	if (_IOC_TYPE(cmd) != MEMX_IOC_MAJOR) {
		pr_err("memryx: fops_ioctl: _IOC_TYPE(cmd) != MEMX_IOC_MAGIC\n");
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > MEMX_IOC_MAXNR) {
		pr_err("memryx: fops_ioctl: _IOC_NR(cmd) > MEMX_IOC_MAXNR(%d)\n", MEMX_IOC_MAXNR);
		return -ENOTTY;
	}

	memx_dev = (struct memx_pcie_dev *)filp->private_data;
	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: fops_ioctl: no opened device!\n");
		return -ENODEV;
	}
	if (down_interruptible(&memx_dev->mutex)) {
		pr_err("memryx: fops_ioctl: get memx_dev->mutex failed\n");
		return -ERESTARTSYS;
	}

	switch (cmd) {
	case MEMX_DOWNLOAD_FIRMWARE: {
			struct memx_firmware_bin memx_fw_bin;
			struct pcie_fw_cmd_format *firmware_command_result_buffer = NULL;

			if (copy_from_user(&memx_fw_bin, (void __user *)arg, sizeof(struct memx_firmware_bin))) {
				pr_err("memryx: fops_ioctl: MEMX_DOWNLOAD_FIRMWARE copy_from_user failed\n");
				ret = -EFAULT;
				goto done;
			}

			if (!memx_fw_bin.buffer) {
				pr_err("memryx: fops_ioctl: MEMX_DOWNLOAD_FIRMWARE got NULL buffer\n");
				ret = -EFAULT;
				goto done;
			}

			if (copy_from_user((void *)(memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base + IFMAP_INGRESS_DCORE_DMA_COHERENT_BUFFER_SIZE_512KB), memx_fw_bin.buffer, memx_fw_bin.size)) {
				pr_err("memryx: fops_ioctl: MEMX_DOWNLOAD_FIRMWARE copy_from_user failed\n");
				ret = -EFAULT;
				goto done;
			}

			dma_sync_single_for_device(&memx_dev->pDev->dev, (dma_addr_t)memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, DMA_COHERENT_BUFFER_SIZE_2MB, DMA_BIDIRECTIONAL);
			firmware_command_result_buffer = memx_send_cmd_to_fw_and_get_result(memx_dev, PCIE_CMD_VENDOR_1, sizeof(struct transport_cmd), CHIP_ID0);

			if ((!firmware_command_result_buffer) || (firmware_command_result_buffer->data[0])) {
				pr_err("memryx: fops_ioctl: MEMX_DOWNLOAD_FIRMWARE failed\n");
				ret = -EFAULT;
				goto done;
			}
	}
	break;
	case MEMX_GET_HW_INFO: {
		struct hw_info hw_info;

		memmove(&hw_info, &memx_dev->mpu_data.hw_info, sizeof(struct hw_info));
		if (copy_to_user((void __user *)arg, &hw_info, sizeof(struct hw_info))) {
			pr_err("memryx: fops_ioctl: MEMX_GET_HW_INFO copy to userspace failed\n");
			ret = -EFAULT;
			goto done;
		}
	}
	break;
	case MEMX_WAIT_FW_MSIX_ACK: {
		memx_send_cmd_to_fw_and_get_result(memx_dev, PCIE_CMD_WAIT_FOR_ACK_ONLY, 0, CHIP_ID0);
		goto done;
	}
	break;
	case MEMX_ABORT_TRANSFER: {
		if (memx_pcie_abort_transfer(memx_dev)) {
			pr_err("memryx: PCIe abort transfer failed\n");
			ret = -EIO;
			goto done;
		}
	}
	break;
	case MEMX_DUMMY_READ: {
		ret = memx_pcie_dummy_read(memx_dev);
	}
	break;
	case MEMX_SET_ABORT_READ: {
		memx_pcie_set_abort_read(memx_dev);
	}
	break;
	case MEMX_CONFIG_MPU_GROUP: {
		struct hw_info hw_info = {0};

		if (copy_from_user(&hw_info, (struct hw_info *)arg, sizeof(struct hw_info))) {
			pr_err("memryx: fops_ioctl: MEMX_CONFIG_MPU_GROUP, copy_from_user failed\n");
			ret = -ENOMEM;
			goto done;
		}
		if (memx_pcie_config_mpu_group(memx_dev, &hw_info)) {
			pr_err("memryx: PCIe MEMX_CONFIG_MPU_GROUP failed\n");
			ret = -EIO;
			goto done;
		}
		memmove(&hw_info, &memx_dev->mpu_data.hw_info, sizeof(struct hw_info));
		if (copy_to_user((void __user *)arg, &hw_info, sizeof(struct hw_info))) {
			pr_err("memryx: fops_ioctl: MEMX_CONFIG_MPU_GROUP copy_to_user failed\n");
			ret = -ENOMEM;
			goto done;
		}
	}
	break;
	case MEMX_INIT_WTMEM_FMAP: {
		struct memx_chip_id memx_chip_id = {0};

		if (copy_from_user(&memx_chip_id, (struct memx_chip_id *)arg, sizeof(memx_chip_id))) {
			pr_err("memryx: MEMX_INIT_WTMEM_FMAP copy_from_user failed\n");
			ret = -ENOMEM;
			goto done;
		}
		memx_send_cmd_to_fw_and_get_result(memx_dev, PCIE_CMD_INIT_WTMEM_FMAP, sizeof(struct memx_chip_id), memx_chip_id.chip_id);
	}
	break;
	case MEMX_VENDOR_CMD: {
		size_t i = 0;
		struct transport_cmd tCmd = {0};
		struct transport_cmd *pCmd = &tCmd;

		_VOLATILE_ u8 *vCmd = (_VOLATILE_ u8 *)pCmd;

		if (copy_from_user((void *)pCmd, (struct transport_cmd *)arg, sizeof(struct transport_cmd))) {
			pr_err("memryx: MEMX_VENDOR_CMD copy_from_user failed\n");
			ret = -ENOMEM;
			goto done;
		}

		for (i = 0; i < sizeof(struct transport_cmd); i++)
			memx_dev->mpu_data.mmap_fw_cmd_buffer_base[i] = vCmd[i];

		switch (pCmd->SQ.subOpCode) {
		case DFP_DOWNLOAD_WEIGHT_MEMORY:
		case DFP_DOWNLOAD_REG_CONFIG:
			dma_sync_single_for_device(&memx_dev->pDev->dev, (dma_addr_t)memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, DMA_COHERENT_BUFFER_SIZE_2MB, DMA_BIDIRECTIONAL);
			memx_send_cmd_to_fw_and_get_result(memx_dev, PCIE_CMD_VENDOR_0, sizeof(struct transport_cmd), CHIP_ID0);
			pCmd->CQ.status = 0;
			break;
		default:
			break;
		}

		for (i = 0; i < sizeof(struct transport_cmd); i++)
			vCmd[i] = memx_dev->mpu_data.mmap_fw_cmd_buffer_base[i];

		if (copy_to_user((void __user *)arg, (void *) pCmd, sizeof(struct transport_cmd))) {
			pr_err("memryx: fops_ioctl: MEMX_SET_DEVICE_FEATURE copy_to_user failed\n");
			ret = -ENOMEM;
			goto done;
		}
	}
	break;
	case MEMX_SET_THROUGHPUT_INFO: {
		if (copy_from_user(&udrv_throughput_info, (struct memx_throughput_info *)arg, sizeof(struct memx_throughput_info))) {
			pr_err("memryx: MEMX_SET_THROUGHPUT_INFO copy_from_user failed\n");
			ret = -ENOMEM;
			goto done;
		}
	}
	break;
	default:
		ret = -EFAULT;
		pr_err("memryx: fops_ioctl: (%u-%u): unsupported ioctl cmd(%u)\n", major, minor, cmd);
	}
done:
	up(&memx_dev->mutex);
#ifdef DEBUG
	pr_info("memryx: fops_ioctl: (%u-%u): finished\n", major, minor);
#endif
	return ret;
}

static s32 memx_fops_open(struct inode *inode, struct file *filp)
{
	s32 ret = 0;
	u8 chip_id = 0;
#ifdef DEBUG
	u32 major = imajor(filp->f_inode);
#endif
	u32 minor = iminor(filp->f_inode);

	struct memx_pcie_dev *memx_dev = memx_get_device_by_index(minor);

	if (!memx_dev) {
		pr_err("memryx: fops_open: PCIe device not found for /dev/memx%d node\n", minor);
		ret = -ENODEV;
		goto exit;
	}

	if (down_interruptible(&memx_dev->mutex)) {
		pr_err("memryx: fops_close: down_interruptible failed\n");
		return -ERESTARTSYS;
	}

	if (memx_dev->reference_count == 0) {
		memx_dev->mpu_data.rx_ctrl.is_abort = 0;
		memx_dev->mpu_data.rx_ctrl.is_read_abort = 0;
		for (chip_id = 0; chip_id < MAX_SUPPORT_CHIP_NUM; chip_id++)
			memx_dev->mpu_data.tx_ctrl[chip_id].is_abort = 0;

		memx_dev->mpu_data.fw_ctrl.is_abort = 0;
	}

	memx_dev->reference_count++;
	filp->private_data = memx_dev;
	up(&memx_dev->mutex);
#ifdef DEBUG
	pr_info("memryx: fops_open: open on /dev/memx%d(%d-%d), vendor_id(%0x), devid_id(%0x))\n",
		memx_dev->minor_index, major, minor, (memx_dev ? memx_dev->pDev->vendor : 0), (memx_dev ? memx_dev->pDev->device : 0));
#endif
exit:
	return ret;
}

static s32 memx_fops_release(struct inode *inode, struct file *filp)
{
#ifdef DEBUG
	u32 major = imajor(filp->f_inode);
	u32 minor = iminor(filp->f_inode);
#endif
	struct memx_pcie_dev *memx_dev = (struct memx_pcie_dev *)filp->private_data;

	if (memx_dev) {
		if (down_interruptible(&memx_dev->mutex)) {
			pr_err("memryx: fops_close: down_interruptible failed\n");
			return -ERESTARTSYS;
		}

		memx_dev->reference_count--;
		up(&memx_dev->mutex);
	}
#ifdef DEBUG
	pr_info("memryx: fops_close: (%d-%d) success\n", major, minor);
#endif
	return 0;
}

static ssize_t memx_fops_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	s32 indicator = -ERESTARTSYS;
	s32 wq_status = 0;
	struct memx_pcie_dev *memx_dev = (struct memx_pcie_dev *)filp->private_data;

	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: fops_read: failed with -ENODEV\n");
		indicator = -ENODEV;
		return indicator;
	}

	rx_start_time = ktime_get();
	// check until received ofmap process done msix if there no pending rx msix
	if (!kfifo_out_locked(&memx_dev->rx_msix_fifo, &indicator, sizeof(s32), &memx_dev->mpu_data.rx_ctrl.lock)) {
		do {
			wq_status = wait_event_interruptible_timeout(memx_dev->mpu_data.rx_ctrl.wq, (memx_dev->mpu_data.rx_ctrl.is_abort == 1) || (kfifo_len(&memx_dev->rx_msix_fifo) != 0), msecs_to_jiffies(10000));
			if (memx_dev->mpu_data.rx_ctrl.is_abort) {
				if (memx_dev->mpu_data.fw_ctrl.is_abort)
					memx_dev->mpu_data.fw_ctrl.is_abort = 0;

				wq_status = -ERESTARTSYS;
				memx_dev->mpu_data.rx_ctrl.is_abort = 0;
				return indicator;
			}
			if (wq_status == -ERESTARTSYS) {
				pr_warn("memryx: fops_read: cancelled by interrupt signal\n");
				break;
			}

			if (wq_status < 1)
				pr_info("memryx: fops_read: wait timeout 10(s), retrying again\n");

		} while (wq_status < 1);
		if (wq_status >= 1) {
			if (!kfifo_out_locked(&memx_dev->rx_msix_fifo, &indicator, sizeof(s32), &memx_dev->mpu_data.rx_ctrl.lock)) {
				pr_err("memryx: fops_read: kfifo_out is empty\n");
				indicator = -EFAULT;
				return indicator;
			}
		}
	}

	if (indicator >= 0) {
		rx_end_time = ktime_get();
		THROUGHPUT_ADD(rx_size, *((uint32_t *)(memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base + 12))); //MEMX_OFMAP_SRAM_COMMON_HEADER_TOTAL_LENGTH_OFFSET
		THROUGHPUT_ADD(rx_time_us, ktime_us_delta(rx_end_time, rx_start_time));
		dma_sync_single_for_cpu(&memx_dev->pDev->dev, (dma_addr_t)memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, OFMAP_EGRESS_DCORE_DMA_COHERENT_BUFFER_SIZE_512KB, DMA_BIDIRECTIONAL);
		if (copy_to_user((void __user *)buf, memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base, count)) {
			pr_err("memryx: fops_read: copy egress_dcore_flow_data to user failed\n");
			indicator = -EFAULT;
			return indicator;
		}
#ifdef DEBUG
		pr_info("memryx: read: received ofmap rx done notification from msix isr(%d)\n", indicator);
#endif
	}
	return indicator;
}

static ssize_t memx_fops_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	_VOLATILE_ u32 *chip0_igr_sram_buf = 0;
	s32 write_len = -ERESTARTSYS;
	s32 wq_status = 0;
	u32 target_chip_id = 0;
	void *tx_dma_buf = NULL;
	struct memx_pcie_dev *memx_dev = NULL;

	memx_dev = (struct memx_pcie_dev *)filp->private_data;
	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: fops_write: failed with -ENODEV\n");
		return -ENODEV;
	}
	if (down_interruptible(&memx_dev->mutex)) {
		pr_err("memryx: fops_write: get memx_dev->mutex failed\n");
		return -ERESTARTSYS;
	}

	// Todo: serperate tx_dma_buf for different chip
	tx_dma_buf = memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base + IFMAP_INGRESS_DCORE_DMA_COHERENT_BUFFER_SIZE_512KB;
	dma_sync_single_for_device(&memx_dev->pDev->dev, (dma_addr_t)(memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base + IFMAP_INGRESS_DCORE_DMA_COHERENT_BUFFER_SIZE_512KB), IFMAP_INGRESS_DCORE_DMA_COHERENT_BUFFER_SIZE_512KB, DMA_BIDIRECTIONAL);

	target_chip_id = *(u32 *)(tx_dma_buf + 8);
	if (target_chip_id >= MAX_SUPPORT_CHIP_NUM) {
		pr_err("memryx: fops_write: invalid target chip id: %d\n", target_chip_id);
		goto done;
	}
#ifdef DEBUG
	pr_info("memryx: fops_write: copy from user %ld\n", count);
	pr_info("memryx: fops_write: target_chip_id(%d)\n", target_chip_id);
#endif

	if (target_chip_id == 0 && memx_dev->mpu_data.hw_info.chip.roles[target_chip_id] == ROLE_SINGLE && memx_dev->bar_mode != MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM) {
		chip0_igr_sram_buf = (_VOLATILE_ u32 *)(memx_dev->mpu_data.mmap_chip0_sram_buffer_base + (memx_dev->mpu_data.hw_info.fw.ingress_dcore_mapping_sram_base[target_chip_id] - memx_dev->mpu_data.hw_info.fw.bar1_mapping_sram_base));
		chip0_igr_sram_buf[1] = count;
		chip0_igr_sram_buf[3] = 0x1;
		g_hitcount_info.sw_irq_hitcount[move_sram_data_to_di_port_idx_5]++;
	} else {
		tx_start_time = ktime_get();
		if (memx_dev->mpu_data.hw_info.chip.input_dma_trigger_type[target_chip_id] == INPUT_DMA_TRIGGER_TYPE_CHIP) {
			memx_pcie_trigger_device_irq(memx_dev, target_chip_id, move_sram_data_to_di_port_idx_5);
		} else {
			u32 transfersz = (*(u32 *)(tx_dma_buf + 4) + 64);
			u32 igr_buf_idx = memx_xflow_read(memx_dev,  target_chip_id, MXCNST_IGR_BUF_WPTR, 0, false) & 0x1;

			memx_dma_transfer_mb(memx_dev, target_chip_id, ARMDMA_CH0, 0x58080000, 0x4006D000+(igr_buf_idx<<18), ((transfersz>>1) & ~0x3));
			memx_dma_transfer_mb(memx_dev, target_chip_id, ARMDMA_CH1, 0x58080000+((transfersz>>1) & ~0x3), 0x4006D000+(igr_buf_idx<<18)+((transfersz>>1) & ~0x3), transfersz - ((transfersz>>1) & ~0x3));
			g_hitcount_info.sw_irq_hitcount[move_sram_data_to_di_port_idx_5]++;
		}
	}

	do {
		wq_status = wait_event_interruptible_timeout(memx_dev->mpu_data.tx_ctrl[target_chip_id].wq, memx_dev->mpu_data.tx_ctrl[target_chip_id].indicator == target_chip_id, msecs_to_jiffies(1000));
		if (memx_dev->mpu_data.tx_ctrl[target_chip_id].is_abort) {
			wq_status = -ERESTARTSYS;
			memx_dev->mpu_data.tx_ctrl[target_chip_id].is_abort = 0;
			up(&memx_dev->mutex);
			return 0;
		}
		if (wq_status == -ERESTARTSYS) {
			pr_warn("memryx: fops_write: cancelled by interrupt signal\n");
			break;
		}
		if (wq_status < 1)
			pr_info("memryx: fops_write: wait timeout 1(s), retrying again\n");

	} while (wq_status < 1);
	if (wq_status >= 1) {
#ifdef DEBUG
		pr_info("memryx: write: received ifmap tx done notification from msix isr(%d)\n", memx_dev->mpu_data.tx_ctrl[target_chip_id].indicator);
#endif
		tx_end_time = ktime_get();
		THROUGHPUT_ADD(tx_size, count);
		THROUGHPUT_ADD(tx_time_us, ktime_us_delta(tx_end_time, tx_start_time));
		spin_lock(&memx_dev->mpu_data.tx_ctrl[target_chip_id].lock);
		memx_dev->mpu_data.tx_ctrl[target_chip_id].indicator = -1;
		spin_unlock(&memx_dev->mpu_data.tx_ctrl[target_chip_id].lock);
		write_len = count;
	}
done:
	up(&memx_dev->mutex);
	return write_len;
}

static s32 memx_fops_mmap(struct file *filp, struct vm_area_struct *vma)
{
	s32 ret = 0;
	size_t map_size = 0;
	size_t map_offs = 0;
	struct memx_pcie_dev *memx_dev = NULL;

	memx_dev = (struct memx_pcie_dev *)filp->private_data;
	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: fops_mmap: failed with -ENODEV\n");
		return -ENODEV;
	}
	if (down_interruptible(&memx_dev->mutex)) {
		pr_err("memryx: fops_mmap: get memx_dev->mutex failed\n");
		return -ERESTARTSYS;
	}

	map_size = vma->vm_end - vma->vm_start;
	map_offs = vma->vm_pgoff << PAGE_SHIFT;
#ifdef DEBUG
	pr_info("memryx: fops_mmap: vma->vm_pgoff %ld, map_size %ld\n", vma->vm_pgoff, map_size);
#endif

	if (map_offs == 0) {
		switch (map_size) {
		case MEMX_PCIE_BAR0_MMAP_SIZE_256MB:
		case MEMX_PCIE_BAR0_MMAP_SIZE_128MB:
		case MEMX_PCIE_BAR1_MMAP_SIZE_1MB: {
			if (((map_size == MEMX_PCIE_BAR0_MMAP_SIZE_256MB) && (memx_dev->bar_mode != MEMXBAR_XFLOW256MB_SRAM1MB)) ||
				((map_size == MEMX_PCIE_BAR0_MMAP_SIZE_128MB) && (memx_dev->bar_mode != MEMXBAR_XFLOW128MB64B_SRAM1MB)) ||
				((map_size == MEMX_PCIE_BAR1_MMAP_SIZE_1MB) && (memx_dev->bar_mode != MEMXBAR_SRAM1MB))) {
				pr_err("memryx: fops_mmap: wrong mmap size %zx for bar mode %d\n", map_size, memx_dev->bar_mode);
				ret = -1;
				break;
			}
			// mapping BAR0 to user
			ret = remap_pfn_range(vma, vma->vm_start,
					(memx_dev->bar_info[0].base) >> PAGE_SHIFT, map_size,
					pgprot_noncached(vma->vm_page_prot));
		} break;
		case DMA_COHERENT_BUFFER_SIZE_2MB: {
			// mapping DMA coherent buffer to user
#if (KERNEL_VERSION(5, 13, 0) > _LINUX_VERSION_CODE_)
			ret = remap_pfn_range(vma, vma->vm_start,
								(memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base) >> PAGE_SHIFT, DMA_COHERENT_BUFFER_SIZE_2MB,
								vma->vm_page_prot);
#else
			ret = dma_mmap_pages(&memx_dev->pDev->dev, vma, DMA_COHERENT_BUFFER_SIZE_2MB,
								 virt_to_page(memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base));

#endif
		} break;
		case MEMX_PCIE_BAR0_MMAP_SIZE_16MB:
		case MEMX_PCIE_BAR0_MMAP_SIZE_64MB:
		case MEMX_PCIE_BAR0_MMAP_SIZE_256KB: {
			// mapping xflow_conf to user
			ret = remap_pfn_range(vma, vma->vm_start,
					(memx_dev->bar_info[memx_dev->xflow_conf_bar_idx].base) >> PAGE_SHIFT, map_size,
					pgprot_noncached(vma->vm_page_prot));
		} break;

		default: {
			pr_err("memryx: fops_mmap: wrong map_size: %ld\n", map_size);
			ret = -1;
		} break;
		}
	} else if ((map_offs == MEMX_PCIE_BAR0_MMAP_SIZE_16MB) || (map_offs == MEMX_PCIE_BAR0_MMAP_SIZE_64MB) ||
				(map_offs == MEMX_PCIE_BAR0_MMAP_SIZE_256KB)) {
		if (((map_offs == MEMX_PCIE_BAR0_MMAP_SIZE_16MB) && (map_size != MEMX_PCIE_BAR0_MMAP_SIZE_16MB)) ||
			((map_offs == MEMX_PCIE_BAR0_MMAP_SIZE_64MB) && (map_size != MEMX_PCIE_BAR0_MMAP_SIZE_64MB)) ||
			((map_offs == MEMX_PCIE_BAR0_MMAP_SIZE_256KB) && (map_size != MEMX_PCIE_BAR0_MMAP_SIZE_512KB))) {
			pr_err("memryx: fops_mmap: wrong map_size: %ld map_offs %ld\n", map_size, map_offs);
			ret = -1;
		} else {
			// mapping xflow_vbuf to user
			ret = remap_pfn_range(vma, vma->vm_start,
					(memx_dev->bar_info[memx_dev->xflow_vbuf_bar_idx].base) >> PAGE_SHIFT, map_size,
					pgprot_noncached(vma->vm_page_prot));
		}
	} else if (map_offs == MEMX_PCIE_BAR0_MMAP_SIZE_4KB) {
		if (map_size != MEMX_PCIE_BAR0_MMAP_SIZE_4KB) {
			pr_err("fops_mmap: wrong map_size: %ld map_offs %ld\n", map_size, map_offs);
			ret = -1;
		} else {
			// mapping device irq base to user
			ret = remap_pfn_range(vma, vma->vm_start,
					(memx_dev->bar_info[memx_dev->device_irq_bar_idx].base) >> PAGE_SHIFT, map_size,
					pgprot_noncached(vma->vm_page_prot));
		}
	} else {
		pr_err("memryx: fops_mmap: wrong pgoff: %ld\n", vma->vm_pgoff);
		ret = -1;
	}

	up(&memx_dev->mutex);
	return ret;
}

struct file_operations memx_pcie_fops = {
owner: THIS_MODULE,
unlocked_ioctl : memx_fops_ioctl,
open : memx_fops_open,
release : memx_fops_release,
read : memx_fops_read,
write : memx_fops_write,
mmap : memx_fops_mmap
};

static struct pci_device_id memx_pcie_id_table[] = {
{
vendor: MEMX_PCIE_VENDOR_ID,
device : MEMX_PCIE_DEVICE_ID,
subvendor : PCI_ANY_ID,
subdevice : PCI_ANY_ID,
class : 0,
class_mask : 0,
driver_data : 0,
},
{0},
};
MODULE_DEVICE_TABLE(pci, memx_pcie_id_table);

#define CACHE_LINE_SIZE_MASK	 (~((cache_line_size()) - 1))
static s32 memx_pcie_probe(struct pci_dev *pDev, const struct pci_device_id *id)
{
	s32 ret = 0;
	u32 bar = 0;
	s32 msix_vec_count = 0;
	u8 chip_id = 0;
	int i = 0;
	struct memx_bar bars[MAX_BAR] = {0};
	struct memx_pcie_dev *memx_dev = NULL;
	struct device *char_dev = NULL;
	struct device *feature_dev = NULL;
	struct memx_firmware_bin memx_fw_bin;

#ifdef DEBUG
	pr_info("memryx: bdf(bus(%04x):device(%02x):func(%x)), Vid(%04x):Did(%04x)\n",
		(pDev->bus) ? pDev->bus->number : 0, (pDev->slot) ? pDev->slot->number : 0, pDev->devfn,
																	pDev->vendor, pDev->device);
#endif
	// Sanity check for device identification
	if (pDev->vendor != MEMX_PCIE_VENDOR_ID) {
		pr_err("memryx: detected vendor mismatch Vid(%0x):Did(%0x)\n", pDev->vendor, pDev->device);
		ret = -ENODEV;
		goto probe_exit;
	}
	memx_dev = devm_kzalloc(&pDev->dev, sizeof(struct memx_pcie_dev), GFP_KERNEL);
	if (memx_dev == NULL) {
		//pr_err("Failed to allocate memory for device extension structure\n");
		ret = -ENOMEM;
		goto probe_exit;
	}

	// Enable the device before we access any pci resource.
	ret = pcim_enable_device(pDev);
	if (ret) {
		pr_err("memryx: failed calling pci_enable_device: %0x:%0x, ret(%d)\n", pDev->vendor, pDev->device, ret);
		goto err_devm_init;
	}
#ifdef DEBUG
	pr_info("memryx: device enabled: %0x:%0x\n", pDev->vendor, pDev->device);
#endif
	pci_set_master(pDev);
	// Set the DMA mask size
	ret = dma_set_mask_and_coherent(&pDev->dev, DMA_BIT_MASK(64));
	if (ret) {
		pr_info("memryx: 64-bit DMA not supported, falling back to 32-bit\n");
		ret = dma_set_mask_and_coherent(&pDev->dev, DMA_BIT_MASK(32));
		if (ret) {
			pr_err("memryx: 32-bit DMA also not supported!\n");
			goto err_dma_init;
		}
	}

#if (KERNEL_VERSION(5, 13, 0) > _LINUX_VERSION_CODE_)
	memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base = kzalloc((DMA_COHERENT_BUFFER_SIZE_2MB + cache_line_size()), GFP_KERNEL | GFP_DMA);
	if (memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base == NULL) {
		//pr_err("Failed to allocate memory for ofmap egress dcore dma buffer\n");
		ret = -ENOMEM;
		goto err_dma_init;
	}

	// Check if it's aligned to a cache line
	if ((unsigned long)memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base % cache_line_size()) {
		pr_err("memryx: check alignment failed for ofmap egress dcore dma buffer, va(%p)\n", memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base);
		memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base = (uint8_t *)((((unsigned long)memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base + cache_line_size() - 1)) & CACHE_LINE_SIZE_MASK);
	}

	memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base = (unsigned int)dma_map_single(&pDev->dev,
																							 memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base,
																							 dma_cohernet_buffer_size,
																							 DMA_BIDIRECTIONAL);
	if (dma_mapping_error(&pDev->dev, (dma_addr_t)memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base)) {
		pr_err("memryx: failed to memory mapping for ofmap egress dcore dma buffer, va(%p)\n", memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base);
		kfree(memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base);
		ret = -EIO;
		goto err_dma_init;
	}
#else
	memx_dev->mpu_data.dma_pages = dma_alloc_pages(&pDev->dev, dma_cohernet_buffer_size,
													(dma_addr_t *)&memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base,
													DMA_BIDIRECTIONAL, GFP_KERNEL);
	memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base = page_address(memx_dev->mpu_data.dma_pages);
	if ((memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base == 0) || (memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base == NULL)) {
		pr_err("memryx: failed to allocate pages for ofmap egress dcore dma buffer, pa(%#llx) va(%#lx)\n", memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, (uintptr_t)memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base);
		ret = -ENOMEM;
		goto err_dma_init;
	}

	if (memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base % DMA_COHERENT_BUFFER_SIZE_2MB) {
		uint32_t aligned_offset = 0;

		dma_free_pages(&pDev->dev, dma_cohernet_buffer_size, memx_dev->mpu_data.dma_pages,
						memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, DMA_BIDIRECTIONAL);

		// Reallocate double the size to handle the alignment issue
		dma_cohernet_buffer_size = DMA_COHERENT_BUFFER_SIZE_2MB * 2;
		memx_dev->mpu_data.dma_pages = dma_alloc_pages(&pDev->dev, dma_cohernet_buffer_size,
														(dma_addr_t *)&memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base,
														DMA_BIDIRECTIONAL, GFP_KERNEL);
		memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base = page_address(memx_dev->mpu_data.dma_pages);
		if ((memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base == 0) || (memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base == NULL)) {
			pr_err("memryx: failed to allocate pages for ofmap egress dcore dma buffer, pa(%#llx) va(%#lx) required size(%u)\n",
					memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, (uintptr_t)memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base, dma_cohernet_buffer_size);
			ret = -ENOMEM;
			goto err_dma_init;
		}

		aligned_offset = DMA_COHERENT_BUFFER_SIZE_2MB - (memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base % DMA_COHERENT_BUFFER_SIZE_2MB);
		if (aligned_offset != DMA_COHERENT_BUFFER_SIZE_2MB) {
			memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base += aligned_offset;
			memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base = (void *)((uint8_t *)memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base + aligned_offset);
		}
	}
#endif

	// Allocate and configure BARs(i.e request MMIO resources)
	ret = pci_request_regions(pDev, PCIE_NAME);
	if (ret) {
		pr_err("memryx: error(%d) allocating bars for %0x:%0x\n", ret, pDev->vendor, pDev->device);
		goto err_pcie_init;
	}
	for (bar = 0; bar < MAX_BAR; bar++) {
		if ((pci_resource_flags(pDev, bar) & IORESOURCE_MEM) && pci_resource_len(pDev, bar)) {
			bars[bar].base = pci_resource_start(pDev, bar);
			bars[bar].size = pci_resource_len(pDev, bar);
			bars[bar].iobase = (u8 *)pcim_iomap(pDev, bar, bars[bar].size);
			bars[bar].available = 1;
		}
	}
	if (!(bars[BAR0].available || bars[BAR1].available)) {
		pr_err("memryx: no available bars for %0x:%0x\n", pDev->vendor, pDev->device);
		ret = -ENODEV;
		goto err_bar_init;
	}

	msix_vec_count = (pci_msix_vec_count(pDev) > pci_msi_vec_count(pDev)) ? pci_msix_vec_count(pDev): pci_msi_vec_count(pDev);

#ifdef DEBUG
	pr_info("memryx: msix/msi vec_count = 0x%x/0x%x \n", pci_msix_vec_count(pDev), pci_msi_vec_count(pDev));
#endif
	// We can do some verdor init and setting according to our own configuration space registers (64 - 256 Byte)
	memx_dev->int_info.max_hw_support_msix_count = msix_vec_count;
	memx_dev->int_info.curr_used_msix_count = (msix_vec_count < MEMRYX_MAX_MSIX_NUMBER) ? msix_vec_count : MEMRYX_MAX_MSIX_NUMBER;
	spin_lock_init(&memx_dev->int_info.lock);

	memx_dev->pDev = pDev;
	sema_init(&memx_dev->mutex, 1);
	memx_dev->reference_count = 0;

	memx_dev->mpu_data.rx_ctrl.is_abort = 0;
	memx_dev->mpu_data.rx_ctrl.is_read_abort = 0;
	for (chip_id = 0; chip_id < MAX_SUPPORT_CHIP_NUM; chip_id++) {
		memx_dev->mpu_data.tx_ctrl[chip_id].is_abort = 0;
		init_waitqueue_head(&memx_dev->mpu_data.tx_ctrl[chip_id].wq);
		spin_lock_init(&memx_dev->mpu_data.tx_ctrl[chip_id].lock);

		spin_lock(&memx_dev->mpu_data.tx_ctrl[chip_id].lock);
		memx_dev->mpu_data.tx_ctrl[chip_id].indicator = -1;
		spin_unlock(&memx_dev->mpu_data.tx_ctrl[chip_id].lock);
	}
	memx_dev->mpu_data.fw_ctrl.is_abort = 0;

	init_waitqueue_head(&memx_dev->mpu_data.rx_ctrl.wq);
	init_waitqueue_head(&memx_dev->mpu_data.fw_ctrl.wq);

	spin_lock_init(&memx_dev->mpu_data.rx_ctrl.lock);
	spin_lock_init(&memx_dev->mpu_data.fw_ctrl.lock);

	mutex_init(&memx_dev->adminlock);

	spin_lock(&memx_dev->mpu_data.rx_ctrl.lock);
	memx_dev->mpu_data.rx_ctrl.indicator = -1;
	spin_unlock(&memx_dev->mpu_data.rx_ctrl.lock);


	spin_lock(&memx_dev->mpu_data.fw_ctrl.lock);
	memx_dev->mpu_data.fw_ctrl.indicator = -1;
	spin_unlock(&memx_dev->mpu_data.fw_ctrl.lock);

	ret = kfifo_alloc(&memx_dev->rx_msix_fifo, sizeof(s32)*MAX_CHIP_NUM, GFP_KERNEL);
	if (ret) {
		pr_err("memryx: kfifo_alloc failed(%d)\n", ret);
		goto err_bar_init;
	}

	for (bar = 0; bar < MAX_BAR; bar++) {
		memx_dev->bar_info[bar] = bars[bar];
#ifdef DEBUG
		pr_info("memryx: bar(%d) - vaddr(%p) paddr_hi(0x%08x), paddr_lo(%08x), map_size(0x%llx), active(%d)\n", bar,
							memx_dev->bar_info[bar].iobase,
							(u32)((memx_dev->bar_info[bar].base >> 32) & 0xFFFFFFFF),
							(u32)((memx_dev->bar_info[bar].base) & 0xFFFFFFFF),
							(u64)memx_dev->bar_info[bar].size,
							memx_dev->bar_info[bar].available);
#endif
	}

	memx_dev->sram_bar_idx = MAX_BAR;
	memx_dev->xflow_conf_bar_idx = MAX_BAR;
	memx_dev->xflow_vbuf_bar_idx = MAX_BAR;
	memx_dev->xflow_conf_bar_offset = 0;
	memx_dev->xflow_vbuf_bar_offset = 0;

	if ((memx_dev->bar_info[0].size == MEMX_PCIE_BAR0_MMAP_SIZE_256MB) && (memx_dev->bar_info[1].size == MEMX_PCIE_BAR1_MMAP_SIZE_1MB)) {
		memx_dev->bar_mode = MEMXBAR_XFLOW256MB_SRAM1MB;
		memx_dev->xflow_conf_bar_idx = BAR0;
		memx_dev->xflow_vbuf_bar_idx = BAR0;
		memx_dev->sram_bar_idx = BAR1;
		#ifdef DEBUG
		pr_info("memryx: MEMX 2BAR-256MB+1MB\r\n");
		#endif
	} else if ((memx_dev->bar_info[1].size == 0) && (memx_dev->bar_info[2].size == MEMX_PCIE_BAR1_MMAP_SIZE_1MB)) {
		memx_dev->bar_mode = MEMXBAR_XFLOW128MB64B_SRAM1MB;
		memx_dev->xflow_conf_bar_idx = BAR0;
		memx_dev->xflow_vbuf_bar_idx = BAR0;
		memx_dev->sram_bar_idx = BAR2;
		memx_dev->xflow_conf_bar_offset = XFLOW_VIRTUAL_BUFFER_PREFIX;
		memx_dev->xflow_vbuf_bar_offset = XFLOW_VIRTUAL_BUFFER_PREFIX;

		#ifdef DEBUG
		pr_info("memryx: MEMX 2BAR-256MB64+1MB\r\n");
		#endif
	} else if ((memx_dev->bar_info[1].size == 0) && (memx_dev->bar_info[0].size == MEMX_PCIE_BAR1_MMAP_SIZE_1MB)) {
		memx_dev->bar_mode = MEMXBAR_SRAM1MB;
		memx_dev->xflow_conf_bar_idx = BAR0;
		memx_dev->xflow_vbuf_bar_idx = BAR0;
		memx_dev->sram_bar_idx = BAR0;

		#ifdef DEBUG
		pr_info("memryx: MEMX 1BAR-1MB\r\n");
		#endif
	} else if ((memx_dev->bar_info[0].size == MEMX_PCIE_BAR0_MMAP_SIZE_512KB ) && (memx_dev->bar_info[2].size == MEMX_PCIE_BAR0_MMAP_SIZE_256KB) &&
			   (memx_dev->bar_info[4].size == MEMX_PCIE_BAR0_MMAP_SIZE_4KB) && (memx_dev->bar_info[5].size == MEMX_PCIE_BAR1_MMAP_SIZE_1MB)) {

		memx_dev->bar_mode = MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM;

		memx_dev->xflow_conf_bar_idx = BAR2;
		memx_dev->xflow_vbuf_bar_idx = BAR0;
		memx_dev->sram_bar_idx = BAR5;
		memx_dev->device_irq_bar_idx = BAR4;
		memx_dev->xflow_conf_bar_offset = XFLOW_CONFIG_REG_PREFIX;
		memx_dev->xflow_vbuf_bar_offset = XFLOW_VIRTUAL_BUFFER_PREFIX;

		#ifdef DEBUG
		pr_info("MEMX 4BAR-VB+CI+MSIX+SRAM\r\n");
		#endif
	} else if (((memx_dev->bar_info[0].size == MEMX_PCIE_BAR0_MMAP_SIZE_16MB) || (memx_dev->bar_info[0].size == MEMX_PCIE_BAR0_MMAP_SIZE_64MB)) &&
			   ((memx_dev->bar_info[2].size == MEMX_PCIE_BAR0_MMAP_SIZE_16MB) || (memx_dev->bar_info[2].size == MEMX_PCIE_BAR0_MMAP_SIZE_64MB)) &&
			   (memx_dev->bar_info[4].size == MEMX_PCIE_BAR1_MMAP_SIZE_1MB)) {

		if (memx_dev->bar_info[0].size == MEMX_PCIE_BAR0_MMAP_SIZE_16MB)
			memx_dev->bar_mode = MEMXBAR_3BAR_BAR0VB_BAR2CI_16MB_BAR4SRAM;
		else
			memx_dev->bar_mode = MEMXBAR_3BAR_BAR0VB_BAR2CI_64MB_BAR4SRAM;

		memx_dev->xflow_conf_bar_idx = BAR2;
		memx_dev->xflow_vbuf_bar_idx = BAR0;
		memx_dev->sram_bar_idx = BAR4;
		memx_dev->xflow_conf_bar_offset = XFLOW_CONFIG_REG_PREFIX;
		memx_dev->xflow_vbuf_bar_offset = XFLOW_VIRTUAL_BUFFER_PREFIX;

		#ifdef DEBUG
		pr_info("memryx: MEMX 3BAR-VB+CI+SRAM %s MB\r\n", (memx_dev->bar_info[0].size == MEMX_PCIE_BAR0_MMAP_SIZE_16MB)?"16":"64");
		#endif
	} else {
		pr_err("memryx: invalid BAR combination\r\n");
		memx_dev->bar_mode = MEMXBAR_NOTVALID;
		goto err_bar_init;
	}

	pci_set_drvdata(pDev, memx_dev);

	memx_dev->msix = msix;
	memx_dev->mpu_data.hw_info.fw.bar0_mapping_mpu_base = MPU_REGISTER_BASE;
	memx_dev->mpu_data.hw_info.fw.bar1_mapping_sram_base = MPU_SRAM_BASE;
	memx_dev->mpu_data.hw_info.fw.firmware_download_sram_base = MPU_FW_DL_BASE;
	memx_dev->mpu_data.hw_info.fw.firmware_command_sram_base = MPU_FW_CMD_BASE;

	memx_dev->mpu_data.mmap_fw_cmd_buffer_base = (u8 *)(memx_dev->bar_info[memx_dev->sram_bar_idx].iobase +
		(memx_dev->mpu_data.hw_info.fw.firmware_command_sram_base - memx_dev->mpu_data.hw_info.fw.bar1_mapping_sram_base));
	memx_dev->mpu_data.mmap_chip0_sram_buffer_base = (u8 *)(memx_dev->bar_info[memx_dev->sram_bar_idx].iobase);

	memx_insert_device(memx_dev);

	// Create device node here
	cdev_init(&memx_dev->char_cdev, &memx_pcie_fops);
	cdev_add(&memx_dev->char_cdev, MKDEV(MAJOR(g_memx_devno), memx_dev->minor_index), 1);
	char_dev = device_create(g_char_device_class, NULL, MKDEV(MAJOR(g_memx_devno), memx_dev->minor_index), NULL, DEVICE_NODE_NAME, memx_dev->minor_index);
	if (IS_ERR(char_dev)) {
		pr_err("memryx: failed createing memx device node(%d)\n", memx_dev->minor_index);
		ret = PTR_ERR(char_dev);
		goto err_dev_init;
	}

	cdev_init(&memx_dev->feature_cdev, &memx_feature_fops);
	cdev_add(&memx_dev->feature_cdev, MKDEV(MAJOR(g_feature_devno), memx_dev->minor_index), 1);
	feature_dev = device_create(g_char_device_class, NULL, MKDEV(MAJOR(g_feature_devno), memx_dev->minor_index), NULL, DEVICE_NODE_NAME "_feature", memx_dev->minor_index);
	if (IS_ERR(feature_dev)) {
		pr_err("memryx: failed createing feature device node(%d)\n", memx_dev->minor_index);
		ret = PTR_ERR(feature_dev);
		goto err_dev_init;
	}

	memx_dev->fs.type = g_drv_fs_type;
	memx_dev->fs.debug_en = fs_debug_en;
	if (memx_dev->fs.type) {
		ret = memx_fs_init(memx_dev);
		if (ret) {
			pr_err("memryx: creating debug file system node failed\n");
			goto err_dev_init;
		}
	}

	memx_fw_bin.request_firmware_update_in_linux = true;
	strscpy(&memx_fw_bin.name[0], FIRMWARE_BIN_NAME, FILE_NAME_LENGTH - 1);
	memx_fw_bin.buffer = NULL;
	memx_fw_bin.size = 0;
	ret = memx_firmware_init(memx_dev, &memx_fw_bin);
	if (ret) {
		pr_err("memryx: failed init firmware(%d)\n", ret);
		goto err_fs_init;
	}

	if (memx_dev->fs.type) {
		ret = memx_fw_log_init(memx_dev);
		if (ret) {
			pr_err("memryx: failed memx_fw_log_init(%d)\n", ret);
			goto err_fs_init;
		}
	}

	if (memx_dev->bar_mode == MEMXBAR_XFLOW256MB_SRAM1MB) {
		if ((pcie_lane_no > 2) || (pcie_lane_no < 1) || (pcie_lane_speed > 3) || (pcie_lane_speed < 1)) {
			pcie_lane_no = 2;
			pcie_lane_speed = 3;
		}
		for (i = 0; i < memx_dev->mpu_data.hw_info.chip.total_chip_cnt; i++) {
			//pr_err("write(%d) %d %d\r\n", pcie_lane_no, pcie_lane_speed);
			memx_xflow_write(memx_dev, i, MEMX_DBGLOG_CONTROL_BASE, 0x68, (0x80010000 + ((0x1 << pcie_lane_no)-1) + ((pcie_lane_speed-1)<<24)), false);
		}

		if (pcie_aspm) {
			for (i = memx_dev->mpu_data.hw_info.chip.total_chip_cnt-1; i >= 0 ; i--)
				memx_xflow_write(memx_dev, i, MEMX_DBGLOG_CONTROL_BASE, 0x6C, pcie_aspm&0xF, false);
		}
	}
	pr_info("memryx: PCIe probe success\n");

	return 0;

err_fs_init:
	if (memx_dev->fs.type)
		memx_fs_deinit(memx_dev);

err_dev_init:
	device_destroy(g_char_device_class, MKDEV(MAJOR(g_feature_devno), memx_dev->minor_index));
	device_destroy(g_char_device_class, MKDEV(MAJOR(g_memx_devno), memx_dev->minor_index));
	cdev_del(&memx_dev->feature_cdev);
	cdev_del(&memx_dev->char_cdev);
	kfifo_free(&memx_dev->rx_msix_fifo);
	memx_pcie_remove_device(memx_dev);

err_bar_init:
	for (bar = 0; bar < MAX_BAR; bar++) {
		if (memx_dev->bar_info[bar].available)
			pcim_iounmap(pDev, memx_dev->bar_info[bar].iobase);
	}
	pci_release_regions(pDev);

err_pcie_init:
#if (KERNEL_VERSION(5, 13, 0) > _LINUX_VERSION_CODE_)
	dma_unmap_single(&pDev->dev, (dma_addr_t)memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, dma_cohernet_buffer_size, DMA_BIDIRECTIONAL);
	kfree(memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base);
#else
	dma_free_pages(&pDev->dev, dma_cohernet_buffer_size, memx_dev->mpu_data.dma_pages,
					memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, DMA_BIDIRECTIONAL);
#endif

err_dma_init:
	pci_disable_device(pDev);

err_devm_init:
	devm_kfree(&pDev->dev, memx_dev);

probe_exit:

	return ret;
}

static void memx_pcie_remove(struct pci_dev *pDev)
{
	u32 bar = 0;
	u8 chip_id = 0;
	struct memx_pcie_dev *memx_dev = (struct memx_pcie_dev *)pci_get_drvdata(pDev);

	if (!memx_dev) {
		pr_info("memryx: memx_remove: pcie device already removed\n");
		return;
	}

	down(&memx_dev->mutex);

	if (memx_dev->fs.type) {
		memx_fw_log_deinit(memx_dev);
		memx_fs_deinit(memx_dev);
	}

	memx_sram_write(memx_dev, (MEMX_DBGLOG_CONTROL_BASE+MEMX_DVFS_MPU_UTI_ADDR), 0);
	if (memx_sram_read(memx_dev, (MEMX_DBGLOG_CONTROL_BASE+MEMX_DVFS_MPU_UTI_ADDR)) == 0) {
		// Issue PCIE_CMD_INIT_HOST_BUF_MAPPING would trigger buf_init to pass DVFS_MPU_UTI_ADDR update to chip1/2/3
		memx_send_cmd_to_fw_and_get_result(memx_dev, PCIE_CMD_INIT_HOST_BUF_MAPPING, 8, CHIP_ID0);
	}

	memx_pcie_remove_device(memx_dev);

	device_destroy(g_char_device_class, MKDEV(MAJOR(g_feature_devno), memx_dev->minor_index));
	device_destroy(g_char_device_class, MKDEV(MAJOR(g_memx_devno), memx_dev->minor_index));
	cdev_del(&memx_dev->feature_cdev);
	cdev_del(&memx_dev->char_cdev);

	memx_deinit_msix_irq(memx_dev);

	wake_up_interruptible(&memx_dev->mpu_data.rx_ctrl.wq);
	for (chip_id = 0; chip_id < MAX_SUPPORT_CHIP_NUM; chip_id++)
		wake_up_interruptible(&memx_dev->mpu_data.tx_ctrl[chip_id].wq);
	wake_up_interruptible(&memx_dev->mpu_data.fw_ctrl.wq);

	kfifo_free(&memx_dev->rx_msix_fifo);

	// deassociate device from device to be picked up by char device
	memx_dev->pDev = NULL;
	for (bar = 0; bar < MAX_BAR; bar++) {
		if (memx_dev->bar_info[bar].available)
			pcim_iounmap(pDev, memx_dev->bar_info[bar].iobase);
	}
	pci_release_regions(pDev);

#if (KERNEL_VERSION(5, 13, 0) > _LINUX_VERSION_CODE_)
	dma_unmap_single(&pDev->dev, (dma_addr_t)memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, dma_cohernet_buffer_size, DMA_BIDIRECTIONAL);
	kfree(memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base);
#else
	dma_free_pages(&pDev->dev, dma_cohernet_buffer_size, memx_dev->mpu_data.dma_pages,
					memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base, DMA_BIDIRECTIONAL);
#endif

	pci_disable_device(pDev);
	pci_set_drvdata(pDev, NULL);

	up(&memx_dev->mutex);

	pr_info("memryx: memx_remove: pcie remove success\n");
}

void memx_pcie_trigger_device_irq(struct memx_pcie_dev *memx_dev, u8 chip_id, enum xflow_mpu_sw_irq_idx sw_irq_idx)
{
	u32 write_value = 0;

	if (memx_xflow_basic_check(memx_dev, chip_id)) {
		pr_err("memx_pcie_trigger_device_irq: basic check fail.\n");
	} else {
		g_hitcount_info.sw_irq_hitcount[sw_irq_idx]++;
		switch (sw_irq_idx) {
		case reserve_idx_2:
		case reset_device_idx_3:
		case fw_cmd_idx_4:
		case move_sram_data_to_di_port_idx_5:
		case init_wtmem_and_fmem_idx_6:
		case reset_mpu_idx_7:
			if (memx_dev->bar_mode != MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM) {
				write_value = (0x1 << sw_irq_idx);
				memx_xflow_write(memx_dev, chip_id, AHB_HUB_IRQ_EN_BASE, 0x0, write_value, true);
			} else {
				volatile uint32_t *device_irq_register_addr = NULL;
				write_value = sw_irq_idx - 2;
				device_irq_register_addr = (u32*)(memx_dev->bar_info[memx_dev->device_irq_bar_idx].iobase + MEMX_PCIE_IRQ_OFFSET(chip_id, write_value));
				*device_irq_register_addr = chip_id;
			}
		break;
		default:
			pr_err("Invalid sw_irq_idx(%u), it should not be used.\n", sw_irq_idx);
		}
	}
}

#ifdef CONFIG_PM
static int memx_pcie_suspend(struct pci_dev *pDev, pm_message_t mesg)
{
	struct memx_pcie_dev *memx_dev = (struct memx_pcie_dev *)pci_get_drvdata(pDev);
	int rc = 0;

	if (mesg.event != pDev->dev.power.power_state.event
			&& (mesg.event & PM_EVENT_SLEEP)) {

		pr_info("memryx: into %s\n", __func__);

		down(&memx_dev->mutex);
		memx_deinit_msix_irq(memx_dev);
		up(&memx_dev->mutex);

		pDev->dev.power.power_state = mesg;
	}


	return rc;
}

static int memx_pcie_resume(struct pci_dev *pDev)
{
	struct memx_pcie_dev *memx_dev = (struct memx_pcie_dev *)pci_get_drvdata(pDev);
	struct memx_firmware_bin memx_fw_bin;

	int rc = 0;

	if (pDev->dev.power.power_state.event != PM_EVENT_ON) {
		pr_info("memryx: into %s\n", __func__);
		memx_fw_bin.request_firmware_update_in_linux = true;
		strscpy(&memx_fw_bin.name[0], FIRMWARE_BIN_NAME, FILE_NAME_LENGTH - 1);
		memx_fw_bin.buffer = NULL;
		memx_fw_bin.size = 0;
		rc = memx_firmware_init(memx_dev, &memx_fw_bin);
		memx_fw_log_init(memx_dev);
		if (rc)
			pr_err("memryx: failed init firmware(%d)\n", rc);

		if (rc == 0)
			pDev->dev.power.power_state = PMSG_ON;
	}

	return rc;
}
#else
#define memx_pcie_suspend NULL
#define memx_pcie_resume  NULL
#endif

static struct pci_driver memx_pcie_driver = {
name: PCIE_NAME,
id_table : memx_pcie_id_table,
probe : memx_pcie_probe,
remove : memx_pcie_remove,
suspend : memx_pcie_suspend,
resume : memx_pcie_resume,
};

static s32 __init memx_pcie_module_init(void);
static s32 __init memx_pcie_module_init(void)
{
	s32 ret = 0;

	ret = alloc_chrdev_region(&g_memx_devno, 0, MAX_CHIP_NUM, PCIE_NAME);
	if (ret < 0) {
		pr_err("memryx: module_init: failed to call alloc_chrdev_region for g_memx_devno, ret(%d)\n", ret);
		return ret;
	}
	ret = alloc_chrdev_region(&g_feature_devno, 0, MAX_CHIP_NUM, PCIE_NAME);
	if (ret < 0) {
		pr_err("memryx: module_init: failed to call alloc_chrdev_region for g_feature_devno, ret(%d)\n", ret);
		return ret;
	}

#ifdef DEBUG
	pr_info("memryx: alloc_chrdev_region g_memx_devno(%u, %u) g_feature_devno(%u, %u)\n", MAJOR(g_memx_devno), MINOR(g_memx_devno), MAJOR(g_feature_devno), MINOR(g_feature_devno));
#endif

#if (KERNEL_VERSION(6, 4, 0) > _LINUX_VERSION_CODE_)
	g_char_device_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
#else
	g_char_device_class = class_create(DEVICE_CLASS_NAME);
#endif
	if (g_char_device_class == NULL) {
		pr_err("memryx: module_init: failed to call class_create\n");
		return -1;
	}
	g_char_device_class->devnode = memx_pcie_devnode;

	ret = pci_register_driver(&memx_pcie_driver);
	if (ret != 0) {
		pr_err("memryx: module_init: failed to call pci_register_driver(%d)\n", ret);
		class_destroy(g_char_device_class);
		unregister_chrdev_region(g_memx_devno, MAX_CHIP_NUM);
		unregister_chrdev_region(g_feature_devno, MAX_CHIP_NUM);
		return ret;
	}

	pr_info("memryx: memx_init: pcie init success, char major Id(%d)\n", MAJOR(g_memx_devno));
	return ret;
}

void __exit memx_pcie_module_exit(void);
void __exit memx_pcie_module_exit(void)
{
	pci_unregister_driver(&memx_pcie_driver);
	class_destroy(g_char_device_class);
	g_char_device_class = NULL;
	unregister_chrdev_region(g_memx_devno, MAX_CHIP_NUM);
	unregister_chrdev_region(g_feature_devno, MAX_CHIP_NUM);

	pr_info("memryx: memx_exit: pcie exit success\n");
}

module_init(memx_pcie_module_init);
module_exit(memx_pcie_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MemryX");
MODULE_DESCRIPTION("MemryX Cascade Plus PCIe driver");
MODULE_VERSION(PCIE_VERSION);
