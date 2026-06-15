// SPDX-License-Identifier: GPL-2.0+
#include <linux/hwmon.h>
#include "memx_pcie.h"
#include "memx_xflow.h"
#include "memx_fs.h"
#include "memx_fs_hwmon.h"

#if IS_REACHABLE(CONFIG_HWMON)
static int memx_temp_read(struct device *dev,
				  enum hwmon_sensor_types type,
				  u32 attr, int channel, long *temp)
{
	struct memx_pcie_dev *memx_dev = dev_get_drvdata(dev);
	u32 temperature_Kelvin = 0;

	/*Read and calculate the average chip temperature*/
	if (channel < memx_dev->mpu_data.hw_info.chip.total_chip_cnt) {
		temperature_Kelvin = memx_sram_read(memx_dev, (MXCNST_TEMP_BASE+(channel<<2)));
		*temp = (int)(((temperature_Kelvin & 0xFFFF) - 273) * 1000);

		return 0;
	} else {
		return -EOPNOTSUPP;
	}
}

static umode_t memx_is_visible(const void *data,
				 enum hwmon_sensor_types type,
				 u32 attr, int channel)
{
	switch (attr) {
	case hwmon_temp:
		return 0444;
	default:
		return 0;
	}
}

static const struct hwmon_channel_info *memx_info[] = {
	HWMON_CHANNEL_INFO(temp,
		HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT,
		HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT,
		HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT,
		HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT),
	NULL,
};

static const struct hwmon_ops memx_hwmon_ops = {
	.is_visible = memx_is_visible,
	.read = memx_temp_read,
};

static const struct hwmon_chip_info memx_chip_info = {
	.ops = &memx_hwmon_ops,
	.info = memx_info,
};

s32 memx_fs_hwmon_init(struct memx_pcie_dev *memx_dev)
{
	int minor_index;

	if (!memx_dev) {
		pr_err("memryx: memx_fs_hwmon init: memx_dev is NULL\n");
		return -EINVAL;
	}

	minor_index = memx_dev->minor_index;

	snprintf(memx_dev->devname, sizeof(memx_dev->devname), DEVICE_NODE_NAME, minor_index);

	devm_hwmon_device_register_with_info(&memx_dev->pDev->dev, memx_dev->devname, memx_dev, &memx_chip_info, NULL);

	return 0;
}

void memx_fs_hwmon_deinit(struct memx_pcie_dev *memx_dev)
{
	if (!memx_dev) {
		pr_err("memryx: memx_hwmon_deinit: memx_dev is NULL\n");
		return;
	}
}

#else
s32 memx_fs_hwmon_init(struct memx_pcie_dev *memx_dev)
{
	if (!memx_dev) {
		pr_err("memryx: memx_fs_hwmon init: memx_dev is NULL\n");
		return -EINVAL;
	}
	return 0;
}


void memx_fs_hwmon_deinit(struct memx_pcie_dev *memx_dev)
{
	if (!memx_dev) {
		pr_err("memryx: memx_hwmon_deinit: memx_dev is NULL\n");
		return;
	}
}
#endif
