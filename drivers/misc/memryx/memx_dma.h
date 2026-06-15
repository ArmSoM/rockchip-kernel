/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _MEMX_DMA_H_
#define _MEMX_DMA_H_

struct memx_pcie_dev;

#define ARM_DMA_TypeDef uint32_t

#define ARM_DMA_BASE                                (0x20040000ul)
#define DMAC_ID_REG_OFS                             0x00
#define DMAC_COMPVER_REG_OFS                        0x08
#define DMAC_CFG_REG_OFS                            0x10
#define DMAC_CHEN0_REG_OFS                          0x18
#define DMAC_CHEN1_REG_OFS                          0x1C
#define DMAC_INTSTATUS_REG_OFS                      0x30
#define DMAC_COMMONREG_INTCLEAR_REG_OFS             0x38
#define DMAC_COMMONREG_INTSTATUS_ENABLE_REG_OFS     0x40
#define DMAC_COMMONREG_INTSIGNAL_ENABLE_REG_OFS     0x48
#define DMAC_COMMONREG_INTSTATUS_REG_OFS            0x50
#define DMAC_RESET_REG_OFS                          0x58
#define DMAC_LOWPOWER0_REG_OFS                      0x60
#define DMAC_LOWPOWER1_REG_OFS                      0x64
#define DMAC_COMMON_PARCTL_REG_OFS                  0x70
#define DMAC_COMMON_ECCCTLSTATUS_REG_OFS            0x78
#define DMAC_CHx_SAR0_REG_OFS                       0x100
#define DMAC_CHx_SAR1_REG_OFS                       0x104
#define DMAC_CHx_DAR0_REG_OFS                       0x108
#define DMAC_CHx_DAR1_REG_OFS                       0x10C
#define DMAC_CHx_BLOCK_TS_REG_OFS                   0x110
#define DMAC_CHx_CTL0_REG_OFS                       0x118
#define DMAC_CHx_CTL1_REG_OFS                       0x11C
#define DMAC_CHx_CFG0_REG_OFS                       0x120
#define DMAC_CHx_CFG1_REG_OFS                       0x124
#define DMAC_CHx_LLP0_REG_OFS                       0x128
#define DMAC_CHx_LLP1_REG_OFS                       0x12C
#define DMAC_CHx_STATUS0_REG_OFS                    0x130
#define DMAC_CHx_STATUS1_REG_OFS                    0x134
#define DMAC_CHx_SWHSSRC_REG_OFS                    0x138
#define DMAC_CHx_SWHSDST_REG_OFS                    0x140
#define DMAC_CHx_BLK_TFR_RESUMEREQ_REG_OFS          0x148
#define DMAC_CHx_AXI_ID_REG_OFS                     0x150
#define DMAC_CHx_AXI_QOS_REG_OFS                    0x158
#define DMAC_CHx_SSTAT_REG_OFS                      0x160
#define DMAC_CHx_DSTAT_REG_OFS                      0x168
#define DMAC_CHx_SSTATAR0_OFS                       0x170
#define DMAC_CHx_SSTATAR1_OFS                       0x174
#define DMAC_CHx_DSTATAR0_OFS                       0x178
#define DMAC_CHx_DSTATAR1_OFS                       0x17C
#define DMAC_CHx_INTSTATUS_ENABLE0_REG_OFS          0x180
#define DMAC_CHx_INTSTATUS_ENABLE1_REG_OFS          0x184
#define DMAC_CHx_INTSTATUS0_REG_OFS                 0x188
#define DMAC_CHx_INTSTATUS1_REG_OFS                 0x18C
#define DMAC_CHx_INTSIGNAL_ENABLE0_REG_OFS          0x190
#define DMAC_CHx_INTSIGNAL_ENABLE1_REG_OFS          0x194
#define DMAC_CHx_INTCLEAR0_REG_OFS                  0x198
#define DMAC_CHx_INTCLEAR1_REG_OFS                  0x19C

#define ARM_DMAC_EN 				0x01
#define ARM_INT_EN 					0x02
#define SB_SIZE						0x4000

enum ARMDMA_CH {
	ARMDMA_CH0,
	ARMDMA_CH1,
	ARMDMA_CH2,
	ARMDMA_CH3,
	ARMDMA_CH4,
	ARMDMA_CH5,
	ARMDMA_CH6,
	ARMDMA_CH7,

	ARMDMA_CH_MAX
};

struct ARMDMA_LL_MEM{

	uint64_t CHx_SAR;   	//0x00-0x07
	uint64_t CHx_DAR;   	//0x08-0xF
	uint32_t CHx_BLOCK_TS; 	//0x10-0x13
	uint32_t RSVD1;		    //0x14-0x17
	uint64_t CHx_LLP;   	//0x18-0x1F
	uint64_t CHx_CTL;   	//0x20-0x27
	uint32_t CHx_SSTAT;    	//0x28-0x2B
	uint32_t CHx_DSTAT;    	//0x2C-ox2F
	uint64_t CHx_LLP_STATUS;
	uint32_t RSVD2;
	uint32_t RSVD3;
};

void memx_dma_transfer_mb(struct memx_pcie_dev *memx_dev, u8 chip_id, enum ARMDMA_CH ch, uint32_t src_addr, uint32_t dst_addr, uint32_t size_bytes);

#endif
