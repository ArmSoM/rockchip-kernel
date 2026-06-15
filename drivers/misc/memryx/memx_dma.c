// SPDX-License-Identifier: GPL-2.0+
#include <linux/firmware.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include "memx_xflow.h"
#include "memx_pcie.h"
#include "memx_fw_cmd.h"
#include "memx_ioctl.h"
#include "memx_fw_init.h"
#include "memx_dma.h"

#define DMAC_CH_SETREG(memx_dev, chip_id, reg_offset, dma_ch, value) memx_xflow_write(memx_dev, chip_id, (ARM_DMA_BASE+(0x100*ch)+reg_offset), 0, value, false)
#define DMAC_CH_GETREG(memx_dev, chip_id, reg_offset, dma_ch)        memx_xflow_read(memx_dev,  chip_id, (ARM_DMA_BASE+(0x100*ch)+reg_offset), 0, false)
#define DMAC_SETREG(memx_dev, chip_id, reg_offset, value)            memx_xflow_write(memx_dev, chip_id, (ARM_DMA_BASE+reg_offset),            0, value, false)
#define DMAC_GETREG(memx_dev, chip_id, reg_offset)                   memx_xflow_read(memx_dev,  chip_id, (ARM_DMA_BASE+reg_offset),            0, false)

void memx_dma_transfer_mb(struct memx_pcie_dev *memx_dev, u8 chip_id, enum ARMDMA_CH ch, uint32_t src_addr, uint32_t dst_addr, uint32_t size_bytes)
{
	struct ARMDMA_LL_MEM *pArmLlp;
	uint32_t tmp, i;
	uint64_t chx_ctl_reg_val;
	uint32_t no,slice,done, descriptor_block_offset;

	descriptor_block_offset = MEMX_DMA_CHIP0_DESC0_OFFSET + (ch * MEXM_DMA_DESC_BLKSZ) + (chip_id * MEMX_DMA_DESC_DESC_COUNT_PER_CHIP * MEXM_DMA_DESC_BLKSZ);

#ifdef DEBUG
	pr_info("%s: chip(%d) dma_ch(%d) src(0x%X) dst(0x%X) size(0x%X)\r\n", __func__, chip_id, ch, src_addr, dst_addr, size_bytes);
#endif
	pArmLlp = (struct ARMDMA_LL_MEM *)(memx_dev->mpu_data.rx_dma_coherent_buffer_virtual_base + descriptor_block_offset);

	DMAC_CH_SETREG(memx_dev, chip_id, DMAC_CHx_INTSIGNAL_ENABLE0_REG_OFS, ch, 0xFFFFFFFE);
	DMAC_SETREG(memx_dev, chip_id, DMAC_CFG_REG_OFS, (ARM_DMAC_EN|ARM_INT_EN));

	/* [1:0]SRC_MULTBLK_TYPE = 3, [3:2]DST_MULTBLK_TYPE=3 */
	tmp = 0xF;
	DMAC_CH_SETREG(memx_dev, chip_id, DMAC_CHx_CFG0_REG_OFS, ch, tmp);

	/* [3]HS_SEL_SRC=1 [4]HS_SEL_DST=1 [19:17]CH_PRIOR=1 [26:23]SRC_OSR_LMT=15 [30:27]DST_OSR_LMT=15 */
	tmp = 0x7F820018;
	DMAC_CH_SETREG(memx_dev, chip_id, DMAC_CHx_CFG1_REG_OFS, ch, tmp);

	DMAC_CH_SETREG(memx_dev, chip_id, DMAC_CHx_LLP0_REG_OFS, ch, MEMX_DMA_DESC_CHIPADDR(ch, 0, chip_id));

	/* [0]SMS=1 [2]DMS=1 [4]SINC=0 [6]DINC=0 [10:8]SRC_TR_WIDTH=2 [13:11]DST_TR_WIDTH=2 [17:14]SRC_MSIZE=3 [21:18]DST_MSIZE=3
	   [38][6]ARLEN_EN=1 [46:39][14:7]ARLEN=15 [47][15]AWLEN_EN=1 [55:48][23:16]AWLEN=15 [58][26]IOC_BlkTfr=1 [63][31]SHADOWREG_OR_LLI_VALID=1 */
	chx_ctl_reg_val = 0x840F87C0000CD205ull;


	no   = (size_bytes+SB_SIZE-1)/SB_SIZE;
	done = 0;
	for (i = 0; i < no; i++) {

		if((size_bytes - done) > SB_SIZE)
			slice = SB_SIZE;
		else
			slice = (size_bytes - done);
		
		pArmLlp[i].CHx_SAR 		 = (uint64_t)(src_addr + done);
		pArmLlp[i].CHx_DAR 		 = (uint64_t)(dst_addr + done);
		pArmLlp[i].CHx_BLOCK_TS  = (slice>>2)-1;
		pArmLlp[i].CHx_CTL 		 = chx_ctl_reg_val;
		pArmLlp[i].CHx_SSTAT 	 = 0;
		pArmLlp[i].CHx_DSTAT 	 = 0;
		pArmLlp[i].CHx_LLP_STATUS= 0;	
		if ( i== (no-1)) {
			pArmLlp[i].CHx_LLP   = 0;
			pArmLlp[i].CHx_CTL  |= (0x1ull << 62);
		} else {
			pArmLlp[i].CHx_LLP   = MEMX_DMA_DESC_CHIPADDR(ch, (i+1), chip_id);
		}
		done += slice;
	}
#ifdef CONFIG_ARCH_HAS_SYNC_DMA_FOR_DEVICE
	dma_sync_single_for_device(&memx_dev->pDev->dev, (dma_addr_t)(memx_dev->mpu_data.hw_info.fw.rx_dma_coherent_buffer_base + descriptor_block_offset), MEXM_DMA_DESC_BLKSZ, DMA_BIDIRECTIONAL);
#endif
	DMAC_SETREG(memx_dev, chip_id, DMAC_CHEN0_REG_OFS, (0x101<< ch));
}

