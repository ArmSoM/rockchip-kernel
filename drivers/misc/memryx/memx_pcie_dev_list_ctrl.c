// SPDX-License-Identifier: GPL-2.0+
#include "memx_pcie.h"
#include "memx_pcie_dev_list_ctrl.h"
extern dev_t g_memx_devno;

static LIST_HEAD(g_memx_pcie_device_list);
static struct semaphore g_memx_add_device_mutex = __SEMAPHORE_INITIALIZER(g_memx_add_device_mutex, 1);


struct memx_pcie_dev *memx_get_device_by_index(u32 index)
{
	struct memx_pcie_dev *memx_dev = NULL;
	struct memx_pcie_dev *target = NULL;

	down(&g_memx_add_device_mutex);
	list_for_each_entry(memx_dev, &g_memx_pcie_device_list, device_list) {
		if (memx_dev->minor_index == index) {
			target = memx_dev;
			break;
		}
	}
	up(&g_memx_add_device_mutex);
	return target;
}


void memx_insert_device(struct memx_pcie_dev *memx_dev)
{
	u32 index = 0;
	struct memx_pcie_dev *pCurrent = NULL;
	struct memx_pcie_dev *pNext = NULL;

	down(&g_memx_add_device_mutex);
	if (list_empty(&g_memx_pcie_device_list)  ||
		list_first_entry(&g_memx_pcie_device_list, struct memx_pcie_dev, device_list)->minor_index > 0) {
		memx_dev->minor_index = 0;
		list_add(&memx_dev->device_list, &g_memx_pcie_device_list);
		up(&g_memx_add_device_mutex);
		return;
	}

	list_for_each_entry_safe(pCurrent, pNext, &g_memx_pcie_device_list, device_list) {
		index = pCurrent->minor_index + 1;
		if ((list_is_last(&pCurrent->device_list, &g_memx_pcie_device_list) ||
			(index != pNext->minor_index))) {
			break;
		}
	}
	memx_dev->major_index = MAJOR(g_memx_devno);
	memx_dev->minor_index = index;
	list_add(&memx_dev->device_list, &pCurrent->device_list);
	up(&g_memx_add_device_mutex);
}


void memx_pcie_remove_device(struct memx_pcie_dev *memx_dev)
{
	down(&g_memx_add_device_mutex);
	if (memx_dev)
		list_del(&memx_dev->device_list);
	up(&g_memx_add_device_mutex);
}
