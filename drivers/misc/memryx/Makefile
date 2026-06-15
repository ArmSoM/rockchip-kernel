# SPDX-License-Identifier: GPL-2.0+
# drivers/misc/memryx/Makefile

ccflags-y += -I$(src)

# 1. 核心映射：告诉内核我们要根据宏配置生成对应的目标
obj-$(CONFIG_MEMRYX_CASCADE_PLUS) += memx_cascade_plus_pcie.o

# 2. 核心打包：无论 y 还是 m，都将这一堆 .o 统一打包到中间目标 memx_cascade_plus_pcie.o 中
memx_cascade_plus_pcie-objs := memx_feature.o \
                   memx_xflow.o \
                   memx_msix_irq.o \
                   memx_cascade_pciemain.o \
                   memx_fw_cmd.o \
                   memx_fw_init.o \
                   memx_pcie_dev_list_ctrl.o \
                   memx_fs_proc.o \
                   memx_fs_sys.o \
                   memx_fs.o \
                   memx_fw_log.o \
                   memx_fs_hwmon.o \
                   memx_dma.o