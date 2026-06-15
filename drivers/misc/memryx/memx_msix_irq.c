// SPDX-License-Identifier: GPL-2.0+

#include <linux/version.h>

#include "memx_pcie.h"
#include "memx_msix_irq.h"

#ifdef DEBUG
static const char *memx_get_msix_usage_by_irq(struct memx_pcie_dev *memx_dev, s32 irq);
static void memx_dump_msix_config_info(struct memx_pcie_dev *memx_dev);
#endif
static s32 memx_get_msix_idx_by_irq(struct memx_pcie_dev *memx_dev, s32 irq);

static irqreturn_t memx_single_isr_handler(s32 irq, void *data)
{
	if (data) {
		struct memx_pcie_dev *memx_dev = (struct memx_pcie_dev *)data;

		if (memx_dev) {
			u32 event;
			s32 msix_idx = -1;
			s32 chip_idx = -1;
			unsigned long timeout = 0;

			/* Get legacy interrupt message */
			event = memx_sram_read(memx_dev, MEMRYX_LEGACY_MSG_ADDR);

			/* Detect is dummy interrupt or not and just return immediately */
			if (event == MEMRYX_LEGACY_CLEAR_MSG) {
				//pr_info("%s: dummy\n", __func__);
				return IRQ_HANDLED;
			}

			/* Notify fw to clear legacy interrupt */
			memx_sram_write(memx_dev, MEMRYX_LEGACY_MSG_ADDR, MEMRYX_LEGACY_MSG_CLR_INTA);
			/* Wait fw complete cleared the leagcy interrupt */
			timeout = jiffies + 1 * HZ;
			while(memx_sram_read(memx_dev, MEMRYX_LEGACY_MSG_ADDR) != MEMRYX_LEGACY_MSG_INT_CLRED) {
				if (time_after(jiffies, timeout)) {
					pr_err("memryx: %s: timeout, event(0x%X)\n", __func__, event);
					break;
				}
			}
			/* Make fw can send next event */
			memx_sram_write(memx_dev, MEMRYX_LEGACY_MSG_ADDR, MEMRYX_LEGACY_CLEAR_MSG);

#ifdef DEBUG
			pr_info("memryx: %s: pci_dev(%0x:%0x), irq(%d), event(0x%X).\n", __func__, memx_dev->pDev->vendor, memx_dev->pDev->device, irq, event);
#endif
			msix_idx = event >> 8;
			chip_idx = (msix_idx - 1) >> 1;

			if (msix_idx == 0) {
				/* memx_firmware_msix_ack_isr*/
				spin_lock(&memx_dev->mpu_data.fw_ctrl.lock);
				memx_dev->mpu_data.fw_ctrl.indicator = msix_idx;
				spin_unlock(&memx_dev->mpu_data.fw_ctrl.lock);
				wake_up_interruptible(&memx_dev->mpu_data.fw_ctrl.wq);
			} else if (msix_idx & 0x1) {
				/* memx_egress_dcore_isr */
				if (!kfifo_in_locked(&memx_dev->rx_msix_fifo, &chip_idx, sizeof(s32), &memx_dev->mpu_data.rx_ctrl.lock))
					pr_err("memryx: isr: kfifo_in fail, rx_msix_fifo is full\n");

				wake_up_interruptible(&memx_dev->mpu_data.rx_ctrl.wq);
			} else {
				/* memx_ingress_dcore_isr */
				spin_lock(&memx_dev->mpu_data.tx_ctrl[chip_idx].lock);
				memx_dev->mpu_data.tx_ctrl[chip_idx].indicator = chip_idx;
				spin_unlock(&memx_dev->mpu_data.tx_ctrl[chip_idx].lock);
				wake_up_interruptible(&memx_dev->mpu_data.tx_ctrl[chip_idx].wq);
			}
		}
	}
	return IRQ_HANDLED;
}

static irqreturn_t memx_firmware_msix_ack_isr(s32 irq, void *data)
{
	if (data) {
		struct memx_pcie_dev *memx_dev = (struct memx_pcie_dev *)data;

		if (memx_dev) {
			s32 msix_idx = -1;

			// memx_disable_msix(memx_dev);
			msix_idx = memx_get_msix_idx_by_irq(memx_dev, irq);
			if (msix_idx == -1) {
				pr_err("memryx: isr: received non-register msix irq(%d), ignoring it\n", irq);
				// memx_enable_msix(memx_dev);
				return IRQ_HANDLED;
			}
			g_hitcount_info.msi_hitcount[msix_idx]++;
#ifdef DEBUG
			pr_info("memryx: isr: driver processed pci_dev(%0x:%0x), msix irq(%d).\n", memx_dev->pDev->vendor, memx_dev->pDev->device, irq);
			pr_info("memryx: isr: %d-th msix usage is %s\n", msix_idx, memx_get_msix_usage_by_irq(memx_dev, irq));
#endif
			spin_lock(&memx_dev->mpu_data.fw_ctrl.lock);
			memx_dev->mpu_data.fw_ctrl.indicator = msix_idx;
			spin_unlock(&memx_dev->mpu_data.fw_ctrl.lock);
			wake_up_interruptible(&memx_dev->mpu_data.fw_ctrl.wq);
			// memx_enable_msix(memx_dev);
		}
	}
	return IRQ_HANDLED;
}

static irqreturn_t memx_egress_dcore_isr(s32 irq, void *data)
{
	if (data) {
		struct memx_pcie_dev *memx_dev = (struct memx_pcie_dev *)data;

		if (memx_dev) {
			s32 msix_idx = -1;
			s32 chip_idx = -1;
			// memx_disable_msix(memx_dev);
			msix_idx = memx_get_msix_idx_by_irq(memx_dev, irq);
			if (msix_idx == -1) {
				pr_err("memryx: isr: received non-register msix irq(%d), ignoring it\n", irq);
				// memx_enable_msix(memx_dev);
				return IRQ_HANDLED;
			}
			g_hitcount_info.msi_hitcount[msix_idx]++;
			chip_idx = (msix_idx - 1) >> 1;

#ifdef DEBUG
			pr_info("memryx: isr: driver processed pci_dev(%0x:%0x), msix irq(%d).\n", memx_dev->pDev->vendor, memx_dev->pDev->device, irq);
			pr_info("memryx: isr: %d-th msix usage is %s\n", msix_idx, memx_get_msix_usage_by_irq(memx_dev, irq));
#endif
			if (!kfifo_in_locked(&memx_dev->rx_msix_fifo, &chip_idx, sizeof(s32), &memx_dev->mpu_data.rx_ctrl.lock))
				pr_err("memryx: isr: kfifo_in fail, rx_msix_fifo is full\n");

			wake_up_interruptible(&memx_dev->mpu_data.rx_ctrl.wq);
			// memx_enable_msix(memx_dev);
		}
	}
	return IRQ_HANDLED;
}

static irqreturn_t memx_ingress_dcore_isr(s32 irq, void *data)
{
	if (data) {
		struct memx_pcie_dev *memx_dev = (struct memx_pcie_dev *)data;

		if (memx_dev) {
			s32 msix_idx = -1;
			u32 chip_id = 0;

			// memx_disable_msix(memx_dev);
			msix_idx = memx_get_msix_idx_by_irq(memx_dev, irq);
			if (msix_idx == -1) {
				pr_err("memryx: isr: received non-register msix irq(%d), ignoring it\n", irq);
				// memx_enable_msix(memx_dev);
				return IRQ_HANDLED;
			}
			g_hitcount_info.msi_hitcount[msix_idx]++;
#ifdef DEBUG
			pr_info("memryx: isr: driver processed pci_dev(%0x:%0x), msix irq(%d).\n", memx_dev->pDev->vendor, memx_dev->pDev->device, irq);
			pr_info("memryx: isr: %d-th msix usage is %s\n", msix_idx, memx_get_msix_usage_by_irq(memx_dev, irq));
#endif
			chip_id = (msix_idx - 1) >> 1;
			spin_lock(&memx_dev->mpu_data.tx_ctrl[chip_id].lock);
			memx_dev->mpu_data.tx_ctrl[chip_id].indicator = chip_id;
			spin_unlock(&memx_dev->mpu_data.tx_ctrl[chip_id].lock);
			wake_up_interruptible(&memx_dev->mpu_data.tx_ctrl[chip_id].wq);
			// memx_enable_msix(memx_dev);
		}
	}
	return IRQ_HANDLED;
}

static struct memx_irq_entry g_msix_entries[MEMRYX_MAX_MSIX_NUMBER] = {
	{"Firmware MSI-X Acknowledge Notification", memx_firmware_msix_ack_isr},
	{"chip 00 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 00 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 01 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 01 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 02 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 02 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 03 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 03 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 04 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 04 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 05 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 05 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 06 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 06 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 07 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 07 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 08 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 08 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 09 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 09 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 10 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 10 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 11 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 11 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 12 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 12 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 13 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 13 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 14 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 14 Ingress Dcore Done Notification.", memx_ingress_dcore_isr},
	{"chip 15 Egress Dcore Done Notification.", memx_egress_dcore_isr},
	{"chip 15 Ingress Dcore Done Notification.", memx_ingress_dcore_isr}
};
#ifdef DEBUG
static const char *memx_get_msix_usage_by_irq(struct memx_pcie_dev *memx_dev, s32 irq)
{
	u32 idx = 0;

	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: msix_irq_query: invalid memx_dev\n");
		return NULL;
	}
	if (!memx_dev->int_info.init_done) {
		pr_info("memryx: msix_irq_query: memx_dev msix disabled\n");
		return NULL;
	}
	for (idx = 0; idx < memx_dev->int_info.curr_used_msix_count; idx++) {
		if (memx_dev->int_info.enable[idx] && memx_dev->int_info.irq[idx] == irq)
			return g_msix_entries[idx].name;
	}
	return NULL;
}
#endif

static s32 memx_get_msix_idx_by_irq(struct memx_pcie_dev *memx_dev, s32 irq)
{
	u32 idx = 0;

	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: msix_irq_query: invalid memx_dev\n");
		return -1;
	}
	if (!memx_dev->int_info.init_done) {
		pr_info("memryx: msix_irq_query: memx_dev msix disabled\n");
		return -1;
	}
	for (idx = 0; idx < memx_dev->int_info.curr_used_msix_count; idx++) {
		if (memx_dev->int_info.enable[idx] && memx_dev->int_info.irq[idx] == irq)
			return idx;
	}
	return -1;
}

s32 memx_init_msix_irq(struct memx_pcie_dev *memx_dev)
{
	u32 idx = 0;
	s32 ret = 0;
	s32 irq_request_or_err = 0;

	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: init_msix_irq: invalid memx_dev\n");
		return -ENODEV;
	}
	if (memx_dev->int_info.init_done) {
		pr_info("memryx: init_msix_irq: memx_dev already init irq\n");
		return ret;
	}

	if(memx_dev->msix) {
		irq_request_or_err = pci_alloc_irq_vectors(memx_dev->pDev, 1, MEMRYX_MAX_MSIX_NUMBER, PCI_IRQ_MSIX);

		if (irq_request_or_err < MEMRYX_MSI_ALLOW_MIN) {
			if (irq_request_or_err > 0) {
				pci_free_irq_vectors(memx_dev->pDev);
			}
			
			pr_info("memryx: msix interrupt number %d not enough try alloc msi interrupt\n", irq_request_or_err);

			irq_request_or_err = pci_alloc_irq_vectors(memx_dev->pDev, 1, MEMRYX_MAX_MSI_NUMBER, PCI_IRQ_MSI);

			if (irq_request_or_err < MEMRYX_MSI_ALLOW_MIN) {
				if (irq_request_or_err > 0) {
					pci_free_irq_vectors(memx_dev->pDev);
				}

				pr_info("memryx: init_msi/msix irq: pci_alloc_irq_vectors(%d) Try using Legacy interrupt\n", irq_request_or_err);

#if KERNEL_VERSION(6, 10, 0) > _LINUX_VERSION_CODE_
				irq_request_or_err = pci_alloc_irq_vectors(memx_dev->pDev, 1, 1, PCI_IRQ_LEGACY);
#else
				irq_request_or_err = pci_alloc_irq_vectors(memx_dev->pDev, 1, 1, PCI_IRQ_INTX);
#endif
				if (irq_request_or_err <= 0) {
					pr_err("memryx: ini pcie legacy irq: fail to call pci_alloc_irq_vectors(%d)\n", irq_request_or_err);
					return -1;
				} else {
					pr_info("memryx: alloc legacy %d", irq_request_or_err);
				}
			} else {
				pr_info("memryx: alloc msi %d", irq_request_or_err);
			}
		} else {
			pr_info("memryx: alloc msix %d", irq_request_or_err);
		}
#ifdef DEBUG
		memx_dump_msix_config_info(memx_dev);
#endif
	} else {
		irq_request_or_err = pci_alloc_irq_vectors(memx_dev->pDev, 1, MEMRYX_MAX_MSI_NUMBER, PCI_IRQ_MSI);

		if (irq_request_or_err < MEMRYX_MSI_ALLOW_MIN) {
			if (irq_request_or_err > 0) {
				pci_free_irq_vectors(memx_dev->pDev);
			}

			pr_info("memryx: init_msi/msix irq: pci_alloc_irq_vectors(%d) Try using Legacy interrupt\n", irq_request_or_err);

#if KERNEL_VERSION(6, 10, 0) > _LINUX_VERSION_CODE_
			irq_request_or_err = pci_alloc_irq_vectors(memx_dev->pDev, 1, 1, PCI_IRQ_LEGACY);
#else
			irq_request_or_err = pci_alloc_irq_vectors(memx_dev->pDev, 1, 1, PCI_IRQ_INTX);
#endif
			if (irq_request_or_err <= 0) {
				pr_err("memryx: ini pcie legacy irq: fail to call pci_alloc_irq_vectors(%d)\n", irq_request_or_err);
				return -1;
			} else {
				pr_info("memryx: alloc legacy %d", irq_request_or_err);
			}
		} else {
			pr_info("memryx: alloc msi %d", irq_request_or_err);
		}
	}
#ifdef DEBUG
	pr_info("memryx: requesting MSIx number %d\n", irq_request_or_err);
#endif
	// Register IRQ handler
	memx_dev->int_info.curr_used_msix_count = irq_request_or_err;
	for (idx = 0; idx < memx_dev->int_info.curr_used_msix_count; idx++) {
		s32 irq = pci_irq_vector(memx_dev->pDev, idx);

		if (memx_dev->int_info.curr_used_msix_count > 1)
			ret = devm_request_irq(&memx_dev->pDev->dev, irq, g_msix_entries[idx].handler, 0, g_msix_entries[idx].name, memx_dev);
		else
			ret = devm_request_irq(&memx_dev->pDev->dev, irq, memx_single_isr_handler, IRQF_SHARED, "MEMX SINGLE ISR Handler", memx_dev);
			
		if (ret < 0) {
			pr_err("memryx: init_msix_irq: fail to call devm_request_irq(%d).\n", ret);
			return ret;
		}
		memx_dev->int_info.irq[idx] = irq;
		memx_dev->int_info.enable[idx] = true;
	}
	memx_dev->int_info.init_done = true;
	return ret;
}

void memx_deinit_msix_irq(struct memx_pcie_dev *memx_dev)
{
	u32 idx = 0;

	if (!memx_dev || !memx_dev->pDev) {
		pr_err("memryx: deinit_msix_irq: invalid memx_dev\n");
		return;
	}
	if (!memx_dev->int_info.init_done) {
		pr_info("memryx: deinit_msix_irq: memx_dev already deinit irq\n");
		return;
	}

	for (idx = 0; idx < memx_dev->int_info.curr_used_msix_count; idx++) {
		if (memx_dev->int_info.enable[idx]) {
			s32 irq_nr = pci_irq_vector(memx_dev->pDev, idx);

			devm_free_irq(&memx_dev->pDev->dev, irq_nr, memx_dev);
			memx_dev->int_info.enable[idx] = false;
			memx_dev->int_info.irq[idx] = 0;
		}
	}
	pci_free_irq_vectors(memx_dev->pDev);
	memx_dev->int_info.init_done = false;
}

#ifdef DEBUG
static void memx_dump_msix_config_info(struct memx_pcie_dev *memx_dev)
{
	s32 pcie_msix_cap_offset = 0;

	u32 msix_capability_doward0 = 0;
	u8 msix_capability_id = 0;
	u8 msix_next_ptr = 0;
	u16 msix_control = 0;

	u16 msix_table_size = 0;

	u8 msix_global_msix_function_mask = 0;
	u8 msix_enable = 0;

	u32 table_offset_and_mapping_in_bar_idx = 0;
	u8 table_mapping_in_bar_idx = 0;
	u32 table_offset = 0;

	u32 pending_bit_array_offset_and_mapping_in_bar_idx = 0;
	u8 pending_bit_array_mapping_in_bar_idx = 0;
	u32 pending_bit_array_offset = 0;

	s32 read_dword_offset = 0;

	pcie_msix_cap_offset = pci_find_capability(memx_dev->pDev, PCI_CAP_ID_MSIX);
	pr_info("memryx: pcie_msix_cap_offset = 0x%x\n", pcie_msix_cap_offset);
	if (pci_read_config_dword(memx_dev->pDev, pcie_msix_cap_offset + read_dword_offset, &msix_capability_doward0) != 0) {
		pr_err("memryx: Read MSIx Capability Dword0 from Configuration Space Failed!\n");
		return;
	}

	msix_capability_id = (msix_capability_doward0 & 0x000000FF);
	msix_next_ptr = (msix_capability_doward0 & 0x0000FF00) >> 8;
	msix_control = (msix_capability_doward0 & 0xFFFF0000) >> 16;

	msix_table_size = (msix_control & (BIT(10)|BIT(9)|BIT(8)|BIT(7)|BIT(6)|BIT(5)|BIT(4)|BIT(3)|BIT(2)|BIT(1)|BIT(0)) >> 0);
	msix_global_msix_function_mask = ((msix_control & (BIT(14))) >> 14);
	msix_enable = ((msix_control & (BIT(15))) >> 15);

	pr_info("memryx: msix_capability_id = 0x%x\n", msix_capability_id);
	pr_info("memryx: msix_capability_next_ptr = 0x%x\n", msix_next_ptr);
	pr_info("memryx: msix_capability_ctrl = 0x%x\n", msix_control);
	pr_info("memryx: msix_function_mask = 0x%x\n", msix_global_msix_function_mask);
	pr_info("memryx: msix_enable = 0x%x\n", msix_enable);
	read_dword_offset += 4;

	if (pci_read_config_dword(memx_dev->pDev, pcie_msix_cap_offset + read_dword_offset, &table_offset_and_mapping_in_bar_idx) != 0) {
		pr_err("memryx: Read MSIx Capability table_offset_and_mapping_in_bar_idx from Configuration Space Failed!\n");
		return;
	}

	table_mapping_in_bar_idx = ((table_offset_and_mapping_in_bar_idx & 0x00000007) >> 0);
	table_offset = ((table_offset_and_mapping_in_bar_idx & 0xFFFFFFF8) >> 3);
	pr_info("memryx: table_offset_and_mapping_in_bar_idx = 0x%x\n", table_offset_and_mapping_in_bar_idx);
	pr_info("memryx: table_mapping_in_bar_idx = %d\n", table_mapping_in_bar_idx);
	pr_info("memryx: table_offset = 0x%x\n", table_offset);

	memx_dev->int_info.msix_table = (struct memx_msix_entry *)(memx_dev->bar_info[table_mapping_in_bar_idx].iobase + table_offset);
	memx_dev->int_info.msix_table_phy_addr = memx_dev->bar_info[table_mapping_in_bar_idx].base + table_offset;
	read_dword_offset += 4;

	if (pci_read_config_dword(memx_dev->pDev, pcie_msix_cap_offset + read_dword_offset, &pending_bit_array_offset_and_mapping_in_bar_idx) != 0) {
		pr_err("memryx: Read MSIx Capability pending_bit_array_offset_and_mapping_in_bar_idx from Configuration Space Failed!\n");
		return;
	}
	pending_bit_array_mapping_in_bar_idx = ((pending_bit_array_offset_and_mapping_in_bar_idx & 0x00000007) >> 0);
	pending_bit_array_offset = ((pending_bit_array_offset_and_mapping_in_bar_idx & 0xFFFFFFF8));
	pr_info("memryx: pending_bit_array_offset_and_mapping_in_bar_idx = 0x%x\n", pending_bit_array_offset_and_mapping_in_bar_idx);
	pr_info("memryx: pending_bit_array_mapping_in_bar_idx = %d\n", pending_bit_array_mapping_in_bar_idx);
	pr_info("memryx: pending_bit_array_offset = 0x%x\n", pending_bit_array_offset);
	memx_dev->int_info.pending_table = (struct memx_pending_entry *)(memx_dev->bar_info[pending_bit_array_mapping_in_bar_idx].iobase + pending_bit_array_offset);
	memx_dev->int_info.pba_table_phy_addr = memx_dev->bar_info[pending_bit_array_mapping_in_bar_idx].base + pending_bit_array_offset;
	memx_dev->int_info.pba_table_offset = pending_bit_array_offset;
}
#endif
