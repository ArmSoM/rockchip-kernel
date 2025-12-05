# SPDX-License-Identifier: GPL-2.0
ifeq ($(CONFIG_ARCH_ROCKCHIP), y)

dtbo-$(CONFIG_CPU_RK3576) += \
	armsom-rk3576-can0-m2-overlay.dtbo \
	armsom-rk3576-can1-m1-overlay.dtbo \
	armsom-rk3576-can1-m2-overlay.dtbo \
	armsom-rk3576-can1-m3-overlay.dtbo \
	armsom-rk3576-i2c0-m1-overlay.dtbo \
	armsom-rk3576-i2c3-m0-overlay.dtbo \
	armsom-rk3576-i2c3-m1-overlay.dtbo \
	armsom-rk3576-i2c4-m1-overlay.dtbo \
	armsom-rk3576-i2c4-m3-overlay.dtbo \
	armsom-rk3576-i2c6-m2-overlay.dtbo \
	armsom-rk3576-i2c6-m3-overlay.dtbo \
	armsom-rk3576-i2c7-m1-overlay.dtbo \
	armsom-rk3576-i2c7-m2-overlay.dtbo \
	armsom-rk3576-i2c8-m2-overlay.dtbo \
	armsom-rk3576-i2c9-m1-overlay.dtbo \
	armsom-rk3576-pwm0-ch0-m1-overlay.dtbo \
	armsom-rk3576-pwm0-ch1-m0-overlay.dtbo \
	armsom-rk3576-pwm1-ch0-m1-overlay.dtbo \
	armsom-rk3576-pwm1-ch0-m3-overlay.dtbo \
	armsom-rk3576-pwm1-ch1-m1-overlay.dtbo \
	armsom-rk3576-pwm2-ch3-m3-overlay.dtbo \
	armsom-rk3576-pwm2-ch6-m3-overlay.dtbo \
	armsom-rk3576-pwm2-ch7-m3-overlay.dtbo \
	armsom-rk3576-spi1-m0-overlay.dtbo \
	armsom-rk3576-spi3-m0-overlay.dtbo \
	armsom-rk3576-spi3-m1-overlay.dtbo \
	armsom-rk3576-uart2-m1-overlay.dtbo \
	armsom-rk3576-uart2-m2-overlay.dtbo \
	armsom-rk3576-uart3-m0-overlay.dtbo \
	armsom-rk3576-uart3-m1-overlay.dtbo \
	armsom-rk3576-uart4-m0-overlay.dtbo \
	armsom-rk3576-uart5-m0-overlay.dtbo \
	armsom-rk3576-uart6-m0-overlay.dtbo \
	armsom-rk3576-uart6-m3-overlay.dtbo \
	armsom-rk3576-uart7-m0-overlay.dtbo \
	armsom-rk3576-uart8-m2-overlay.dtbo \
	armsom-rk3576-uart10-m1-overlay.dtbo \
	armsom-rk3576-uart10-m2-overlay.dtbo \
	armsom-cm5-io-camera-imx219-cs0.dtbo \
	armsom-cm5-io-camera-ov13850-cs1.dtbo \
	armsom-cm5-io-display-10hd.dtbo \
	armsom-sige5-camera-ov13850-cs0.dtbo \
	armsom-sige5-camera-ov13850-cs1.dtbo \
	armsom-sige5-display-10hd.dtbo \
	armsom-sige7-camera-imx415-4k.dtbo \
	armsom-sige7-camera-ov13850-csi0.dtbo \
	armsom-sige7-camera-ov13850-csi1.dtbo \
	armsom-sige7-display-10hd.dtbo \

endif

dtbotxt-$(CONFIG_ARCH_ROCKCHIP) += \
	README.md

targets += $(dtbo-y) $(dtbotxt-y)

always-y		:= $(dtbo-y) $(dtbotxt-y)
clean-files		:= *.dtbo
