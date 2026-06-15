/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _MEMX_PCIE_H_
#define _MEMX_PCIE_H_
#include <linux/pci.h>
#include <linux/kfifo.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include "memx_msix_irq.h"
#include "memx_mpu.h"
#include "memx_fs.h"
#include "memx_xflow.h"

#define PCIE_VERSION "1.3.13"
#define SDK_RELEASE_VERSION "2.2"

#define PCIE_NAME "memx_pcie_ai_chip"

#define DEVICE_NODE_NAME "memx%d"
#define DEVICE_NODE_DEFAULT_ACCESS_RIGHT (0x0666)
#define DEVICE_CLASS_NAME "memx_cdev"

#define MEMX_PCIE_VENDOR_ID (0x1FE9)
#define MEMX_PCIE_DEVICE_ID (0x0100)

#define _VOLATILE_
#define _LINUX_VERSION_CODE_	LINUX_VERSION_CODE

#define MEMX_PCIE_BAR0_MMAP_SIZE_256MB (0x10000000)
#define MEMX_PCIE_BAR0_MMAP_SIZE_128MB (0x8000000)
#define MEMX_PCIE_BAR0_MMAP_SIZE_64MB  (0x4000000)
#define MEMX_PCIE_BAR0_MMAP_SIZE_16MB  (0x1000000)
#define MEMX_PCIE_BAR1_MMAP_SIZE_1MB   (0x100000)
#define MEMX_PCIE_BAR0_MMAP_SIZE_512KB (0x80000)
#define MEMX_PCIE_BAR0_MMAP_SIZE_256KB (0x40000)
#define MEMX_PCIE_BAR0_MMAP_SIZE_4KB   (0x1000)

#define MXCNST_RWACCESS        (0666)
#define MXCNST_FW_START_BASE   (0x40040000)
#define MXCNST_FW_ZSBL_INITVAL (0x4D4D4D4D)
#define MXCNST_PCIE_LOCINTR    (0x2110020C)
#define MXCNST_CQDATA0_ADDR    (0x40046f70)
#define MXCNST_MEMXR_CMD       (0x6d656d72)
#define MXCNST_MEMXW_CMD       (0x6d656d77)
#define MXCNST_MEMX0_CMD       (0x6d656d30)
#define MXCNST_MEMXQ_CMD       (0x6D656D51)
#define MXCNST_MEMXt_CMD       (0x6d656d74)
#define MXCNST_HANDSHAKE_MAGIC (0xABCDEFA9)
#define MXCNST_TEMP_BASE       (0x40046d40)
#define MXCNST_DATASRAM_BASE   (0x40080000)
#define MXCNST_MPUUTIL_BASE    (0x40046d00)
#define MXCNST_FW_TYPE_OFS     (0x40046FF4)
#define MXCNST_IMG_TYPE_OFS    (0x00006FF8)
#define MXCNST_RMTCMD_PARAM    (0x40046F48)
#define MXCNST_RMTCMD_COMMD    (0x40046F44)
#define MXCNST_COLDRSTCNT_ADDR (0x400fdf40)
#define MXCNST_WARMRSTCNT_ADDR (0x400fdf44)
#define MXCNST_MANUFACTID1     (0x40046f58)
#define MXCNST_MANUFACTID2     (0x40046f5c)
#define MXCNST_COMMITID        (0x40046f08)
#define MXCNST_DATECODE        (0x40046f0c)
#define MXCNST_BOOT_MODE       (0x20000100)
#define MXCNST_CHIP_VERSION    (0x20000500)
#define MXCNST_FW_SYS_TICK     (0x40046f14)
#define MXCNST_RP_XFLOW_ADDR   (0x60000000)
#define MXCNST_IGR_BUF_WPTR    (0x40046F88)

#define MEMX_PCIE_IRQ_OFFSET(C, I) ((((15 - (C)) * DEVICE_IRQ_COUNT) + (I)) * 4 + 0x200)

enum memx_bar_id {
	BAR0 = 0,
	BAR1,
	BAR2,
	BAR3,
	BAR4,
	BAR5,

	MAX_BAR
};

enum memx_drv_log_level {
	ERROR_OCCUR_ONLY = 0, // default

	DRV_INIT_FLOW = 1,
	DRV_DEINIT_FLOW = 2,
	DRV_INIT_AND_DEINIT = (DRV_INIT_FLOW | DRV_DEINIT_FLOW),

	FW_INIT_FLOW = 4,
	FW_DEINIT_FLOW = 8,
	FW_INIT_AND_DEINIT = (FW_INIT_FLOW | FW_DEINIT_FLOW),

	DRV_TX_FLOW = 16,
	DRV_RX_FLOW = 32,
	DRV_TX_AND_RX = (DRV_TX_FLOW | DRV_RX_FLOW),

	DRV_ISR_FLOW = 64,
	DRV_IOCTL_FLOW = 128,
	DRV_FUNC_EXEC_FLOW = 256,
	VERBOSE_MODE = (DRV_INIT_AND_DEINIT | FW_INIT_AND_DEINIT | DRV_TX_AND_RX | DRV_ISR_FLOW | DRV_IOCTL_FLOW | DRV_FUNC_EXEC_FLOW),
};

enum fw_log_dump_ctrl {
	FW_LOG_DISABLE = 0,
	FW_LOG_ENABLE = 1,
};

struct memx_bar {
	u64 base;		// kernel physical address
	u8 *iobase;		// kernel vitual address
	u64 size;		// resource length
	u32 available;	// resouce available
};

struct memx_runtime_cfg {
	enum memx_drv_log_level drv_log_level;
	enum fw_log_dump_ctrl fw_log_ctrl;
};

struct memx_pcie_dev {
	struct list_head device_list;
	struct pci_dev *pDev;
	struct semaphore mutex;
	u32 reference_count;

	struct memx_runtime_cfg rt_cfg;

	u32 major_index;
	u32 minor_index;
	u32 ThermalThrottlingDisable;
	u32 gpio_r;
	u32 msix;

	struct memx_bar bar_info[MAX_BAR];
	struct memx_interrupt int_info;

	struct memx_mpu_data mpu_data;

	struct memx_file_sys fs;

	struct kfifo rx_msix_fifo;

	char devname[16];
	enum memx_pcie_bar_mode bar_mode;
	enum memx_bar_id xflow_conf_bar_idx;
	enum memx_bar_id xflow_vbuf_bar_idx;
	enum memx_bar_id sram_bar_idx;
	enum memx_bar_id device_irq_bar_idx;
	u32 xflow_conf_bar_offset;
	u32 xflow_vbuf_bar_offset;
	struct cdev char_cdev;
	struct cdev feature_cdev;

	struct mutex   adminlock;
};

#define MEMX_MAX_MSIX_NUMBER 	((MEMRYX_MAX_MSIX_NUMBER + 3) & ~0x3)
#define MEMX_MAX_SW_IRQ_NUMBER	((max_support_mpu_sw_irq_num + 3) & ~0x3)
struct hitcount_info {
	u32 msi_hitcount[MEMX_MAX_MSIX_NUMBER];
	u32 sw_irq_hitcount[MEMX_MAX_SW_IRQ_NUMBER];
};
extern struct hitcount_info g_hitcount_info;

extern struct file_operations memx_feature_fops;
extern u32 tx_time_us;
extern u32 rx_time_us;
extern u32 tx_size;
extern u32 rx_size;
extern struct memx_throughput_info udrv_throughput_info;

void memx_pcie_trigger_device_irq(struct memx_pcie_dev *memx_dev, u8 chip_id, enum xflow_mpu_sw_irq_idx sw_irq_idx);

#endif
