/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _MEMX_FW_LOG_H_
#define _MEMX_FW_LOG_H_

#define MEMX_DBGLOG_CONTROL_BASE		            (0x40046F00)
#define  MEMX_DBGLOG_CTRL_BUFFERADDR_OFS                  (0x1C)
#define  MEMX_DBGLOG_CTRL_BUFFERSIZE_OFS                  (0x20)
#define  MEMX_DBGLOG_CTRL_WPTRADDR_OFS                    (0x2C)
#define  MEMX_DBGLOG_CTRL_RPTRADDR_OFS                    (0x30)
#define  MEMX_DBGLOG_CTRL_ENABLE_OFS                      (0x3C)
#define  MEMX_RMTCMD_CMDADDR_OFS                          (0x4C)
#define  MEMX_RMTCMD_PARAMADDR_OFS                        (0x50)
#define  MEMX_RMTCMD_PARAM2ADDR_OFS                       (0x74)
#define  MEMX_DVFS_MPU_UTI_ADDR                           (0x64)
#define MEMX_DBGLOG_PCIE_LINK0_BASE                 (0x58000000)

#define MEMX_DBGLOG_CHIP_LOG_64ID					4
#define MEMX_DBGLOG_STORE_ALL_FIFO_INFO_SIZE_64KB	(0x10000)
#define MEMX_DBGLOG_CHIP_BUFFER_SIZE_64KB			(0x10000)
#define MEMX_DBGLOG_CHIP_BUFFER_SIZE_32KB			(0x08000)
#define MEMX_DBGLOG_CHIP_BUFFER_SIZE(chip_id)		(((chip_id) >= MEMX_DBGLOG_CHIP_LOG_64ID)?MEMX_DBGLOG_CHIP_BUFFER_SIZE_32KB:MEMX_DBGLOG_CHIP_BUFFER_SIZE_64KB)

#define MEMX_DBGLOG_DMA_HOST_BUFFER_OFFSET          (0x100000)
/* This part is host buffer use from back direction, Back direction have 32KB space allowed */
#define MEMX_DBGLOG_RWPTR_OFFSET                    (0xFF000+MEMX_DBGLOG_DMA_HOST_BUFFER_OFFSET)
#define MEMX_RMTCMD_CONTROLBASE_OFFSET              (0xFE000+MEMX_DBGLOG_DMA_HOST_BUFFER_OFFSET)
#define MEMX_RMTCMD_PARAM2BASE_OFFSET               (0xFE200+MEMX_DBGLOG_DMA_HOST_BUFFER_OFFSET)
#define MEMX_DVFS_UTILBASE_OFFSET                   (0xFD000+MEMX_DBGLOG_DMA_HOST_BUFFER_OFFSET)

/* This part is host buffer use from end of debuglog in front direction, Front direction have (1024 - 32 - debuglog_used) allowed */
#define MEMX_DBGLOG_END_OFFSET                (MEMX_DBGLOG_DMA_HOST_BUFFER_OFFSET+(MEMX_DBGLOG_CHIP_LOG_64ID*MEMX_DBGLOG_CHIP_BUFFER_SIZE_64KB)+((16-MEMX_DBGLOG_CHIP_LOG_64ID)*MEMX_DBGLOG_CHIP_BUFFER_SIZE_32KB))
#define MEMX_DMA_DESC_START_OFFSET            MEMX_DBGLOG_END_OFFSET //0xA0000 for case4
#define MEMX_DMA_DESC_DESC_COUNT_PER_CHIP     (2)
#define MEXM_DMA_DESC_BLKSZ                   (0x800) //((512/16)*64) = 2KB
#define MEMX_DMA_CHIP0_DESC0_OFFSET           (MEMX_DMA_DESC_START_OFFSET) //0xA0000
#define MEMX_DMA_CHIP0_DESC1_OFFSET           (MEMX_DMA_CHIP0_DESC0_OFFSET+MEXM_DMA_DESC_BLKSZ) //0xA0800
#define MEMX_DMA_CHIP1_DESC0_OFFSET           (MEMX_DMA_CHIP0_DESC1_OFFSET+MEXM_DMA_DESC_BLKSZ) //0xA1000
#define MEMX_DMA_CHIP1_DESC1_OFFSET           (MEMX_DMA_CHIP1_DESC0_OFFSET+MEXM_DMA_DESC_BLKSZ) //0xA1800
#define MEMX_DMA_CHIP2_DESC0_OFFSET           (MEMX_DMA_CHIP1_DESC1_OFFSET+MEXM_DMA_DESC_BLKSZ) //0xA2000
#define MEMX_DMA_CHIP2_DESC1_OFFSET           (MEMX_DMA_CHIP2_DESC0_OFFSET+MEXM_DMA_DESC_BLKSZ) //0xA2800
#define MEMX_DMA_CHIP3_DESC0_OFFSET           (MEMX_DMA_CHIP2_DESC1_OFFSET+MEXM_DMA_DESC_BLKSZ) //0xA3000
#define MEMX_DMA_CHIP3_DESC1_OFFSET           (MEMX_DMA_CHIP3_DESC0_OFFSET+MEXM_DMA_DESC_BLKSZ) //0xA3800
#define MEMX_DMA_DESC_CHIPADDR(ch, idx, chip) (MEMX_DBGLOG_PCIE_LINK0_BASE + MEMX_DMA_CHIP0_DESC0_OFFSET + ((ch)*MEXM_DMA_DESC_BLKSZ)+((idx)*64)+(chip)*MEMX_DMA_DESC_DESC_COUNT_PER_CHIP*MEXM_DMA_DESC_BLKSZ)

#define MEMX_GET_CHIP_DBGLOG_BUFFER_BUS_ADDR(chip_id) (((chip_id) >= MEMX_DBGLOG_CHIP_LOG_64ID) ? \
		(MEMX_DBGLOG_PCIE_LINK0_BASE + MEMX_DBGLOG_DMA_HOST_BUFFER_OFFSET + (MEMX_DBGLOG_CHIP_BUFFER_SIZE_64KB*MEMX_DBGLOG_CHIP_LOG_64ID) + ((chip_id-MEMX_DBGLOG_CHIP_LOG_64ID) * MEMX_DBGLOG_CHIP_BUFFER_SIZE_32KB)) : \
		(MEMX_DBGLOG_PCIE_LINK0_BASE + MEMX_DBGLOG_DMA_HOST_BUFFER_OFFSET + ((chip_id) * MEMX_DBGLOG_CHIP_BUFFER_SIZE_64KB)))
#define MEMX_GET_CHIP_DBGLOG_WRITER_PTR_BUS_ADDR(chip_id) (MEMX_DBGLOG_PCIE_LINK0_BASE + MEMX_DBGLOG_RWPTR_OFFSET + ((chip_id<<3)))
#define MEMX_GET_CHIP_DBGLOG_READ_PTR_BUS_ADDR(chip_id) (MEMX_DBGLOG_PCIE_LINK0_BASE + MEMX_DBGLOG_RWPTR_OFFSET + ((chip_id<<3)) + 4)
#define MEMX_GET_CHIP_RMTCMD_COMMAND_BUS_ADDR(chip_id) (MEMX_DBGLOG_PCIE_LINK0_BASE + MEMX_RMTCMD_CONTROLBASE_OFFSET + ((chip_id<<3)))
#define MEMX_GET_CHIP_RMTCMD_PARAM_BUS_ADDR(chip_id) (MEMX_DBGLOG_PCIE_LINK0_BASE + MEMX_RMTCMD_CONTROLBASE_OFFSET + ((chip_id<<3)) + 4)
#define MEMX_GET_CHIP_RMTCMD_PARAM2_BUS_ADDR(chip_id) (MEMX_DBGLOG_PCIE_LINK0_BASE + MEMX_RMTCMD_PARAM2BASE_OFFSET + ((chip_id<<4)) + 0)
#define MEMX_GET_DVFS_UTIL_BUS_ADDR                  (MEMX_DBGLOG_PCIE_LINK0_BASE + MEMX_DVFS_UTILBASE_OFFSET)

#define MEMX_GET_CHIP_DBGLOG_BUFFER_VIRTUAl_ADDR(memx_dev, chip_id) (((chip_id) >= MEMX_DBGLOG_CHIP_LOG_64ID) ? \
				(((memx_dev)->mpu_data.rx_dma_coherent_buffer_virtual_base) + MEMX_DBGLOG_DMA_HOST_BUFFER_OFFSET + (MEMX_DBGLOG_CHIP_BUFFER_SIZE_64KB*MEMX_DBGLOG_CHIP_LOG_64ID) + ((chip_id-MEMX_DBGLOG_CHIP_LOG_64ID) * MEMX_DBGLOG_CHIP_BUFFER_SIZE_32KB)) : \
				(((memx_dev)->mpu_data.rx_dma_coherent_buffer_virtual_base) + MEMX_DBGLOG_DMA_HOST_BUFFER_OFFSET + ((chip_id) * MEMX_DBGLOG_CHIP_BUFFER_SIZE_64KB)))

// | chip0_write_ptr(4B) | chip0__read_ptr(4B) | chip1_write_ptr(4B) | chip1__read_ptr(4B) | ..... | chipN_write_ptr(4B) | chipN__read_ptr(4B)
#define MEMX_GET_CHIP_DBGLOG_WRITER_PTR_VIRTUAl_ADDR(memx_dev, chip_id) (((memx_dev)->mpu_data.rx_dma_coherent_buffer_virtual_base) + \
									MEMX_DBGLOG_RWPTR_OFFSET + ((chip_id<<3)))
#define MEMX_GET_CHIP_DBGLOG_READ_PTR_VIRTUAl_ADDR(memx_dev, chip_id) (((memx_dev)->mpu_data.rx_dma_coherent_buffer_virtual_base) + \
									MEMX_DBGLOG_RWPTR_OFFSET + ((chip_id<<3)) + 4)
#define MEMX_GET_CHIP_RMTCMD_COMMAND_VIRTUAl_ADDR(memx_dev, chip_id) (((memx_dev)->mpu_data.rx_dma_coherent_buffer_virtual_base) + \
									MEMX_RMTCMD_CONTROLBASE_OFFSET + ((chip_id<<3)))
#define MEMX_GET_CHIP_RMTCMD_PARAM_VIRTUAl_ADDR(memx_dev, chip_id) (((memx_dev)->mpu_data.rx_dma_coherent_buffer_virtual_base) + \
									MEMX_RMTCMD_CONTROLBASE_OFFSET + ((chip_id<<3)) + 4)
#define MEMX_GET_CHIP_RMTCMD_PARAM2_VIRTUAl_ADDR(memx_dev, chip_id) (((memx_dev)->mpu_data.rx_dma_coherent_buffer_virtual_base) + \
									MEMX_RMTCMD_PARAM2BASE_OFFSET + ((chip_id<<4)) + 0)

#define MEMX_DGBLOG_RPTR_DEFAULT    0x26F28
#define MEMX_DGBLOG_WPTR_DEFAULT    0x26F24
#define MEMX_DGBLOG_SIZE_DEFAULT    0x1000
#define MEMX_DGBLOG_ADDRESS_DEFAULT 0x25C00

#define MEMX_RMTCMD_COMMAND_DEFAULT    0x40046F44
#define MEMX_RMTCMD_PARAMTER_DEFAULT   0x40046F48


struct memx_fw_log_fifo {
	void *buffer[16];
	u32 *write_ptr[16];
	u32 *read_ptr[16];
};

struct memx_pcie_dev;
s32 memx_fw_log_init(struct memx_pcie_dev *memx_dev);
void memx_fw_log_deinit(struct memx_pcie_dev *memx_dev);
s32 memx_fw_log_dump(struct memx_pcie_dev *memx_dev, u8 chip_id);

#endif
