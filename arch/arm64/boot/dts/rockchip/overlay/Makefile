# SPDX-License-Identifier: GPL-2.0
ifeq ($(CONFIG_ARCH_ROCKCHIP), y)

dtbo-$(CONFIG_CPU_RK3576) += \
	rk3576-uart7-m0-overlay.dtbo \

endif

dtbotxt-$(CONFIG_ARCH_ROCKCHIP) += \
	README.md

targets += $(dtbo-y) $(dtbotxt-y)

always-y		:= $(dtbo-y) $(dtbotxt-y)
clean-files		:= *.dtbo
