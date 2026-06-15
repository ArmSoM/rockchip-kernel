// SPDX-License-Identifier: GPL-2.0+
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "memx_ioctl.h"

#define PCIE_DEV_FILE_0 "/dev/memx0"
#define _VOLATILE_

//int ioctl_firmware_download_test(char *dve_file)
//{
//	int fd = 0;
//	long ret = 0;
//	int err = 1;
//
//	fd = open(dve_file, O_RDWR);
//	if (fd == -1) {
//		perror("open fail -1\n");
//		return -err;
//	}
//
//
//	err++;
//	memx_firmware_bin_t memx_bin;
//	const char *bin_name = "cascade_plus.bin";
//
//	memset(&memx_bin, 0, sizeof(memx_bin));
//	strncpy(memx_bin.name, bin_name, strlen(bin_name));
//	memx_bin.name[strlen(bin_name)] = '\0';
//	unsigned char dummy_buffer[16] = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};
//
//	memx_bin.buffer = &dummy_buffer[0];
//	memx_bin.size = sizeof(dummy_buffer);
//	ret = ioctl(fd, MEMX_DOWNLOAD_FIRMWARE, &memx_bin);
//	if (ret == -1) {
//		printf("errno %d\n", errno);
//		perror("ioctl fail (-5)");
//		return -err;
//
//	} else {
//		printf("IOCTL MEMX_DOWNLOAD_FIRMWARE flow Pass!\n");
//	}
//
//
//	close(fd);
//	perror("close");
//	return 0;
//}


int mmap_xflow_test(char *dve_file, unsigned int *data, unsigned int dataByteSize)
{
#define MMAP_SIZE_256M_BYTE							(0x10000000)

#define	MPU_WRAPPER_REGISTER_OFFSET					(0xC000000)

#define	CHIP0_XFLOW_CONFIG_REGISTER_OFFSET			(0xC000000)
#define	CHIP0_XFLOW_VIRTUAL_BUFFER_OFFSET			(0x8000000)

#define	SYSTEM_MEMORY_ON_CHIP_RAM_DATA				(0x400a0000)
#define	MPU_WRAPPER_BASE_ADDR_REGISTER_OFFSET	   (0x0)
#define	MPU_WRAPPER_CONTROL_REGISTER_OFFSET		 (0x4)
#define	MPU_WRAPPER_CONTROL_REGISTER_ACCESS_MPU	 (0x00000000)
#define	MPU_WRAPPER_CONTROL_REGISTER_ACCESS_OTHERS  (0x00000001)

	int chipfd = 0;
	long ret = 0;
	char *ptr = NULL;
	int idx = 0;
	int fourByteCount = dataByteSize / 4;

	fourByteCount = (fourByteCount * 4 < dataByteSize) ? fourByteCount + 1 : fourByteCount;

	chipfd = open(dve_file, O_RDWR | O_SYNC);
	if (chipfd == -1) {
		perror("open chip dev fail (-1)");
		return -1;
	}

	unsigned long *map_base = (unsigned long *)mmap(NULL, MMAP_SIZE_256M_BYTE, PROT_READ | PROT_WRITE, MAP_SHARED, chipfd, 0);

	if (map_base == MAP_FAILED) {
		perror("fail to mmap");
		return -1;
	}

	printf("Phy Addr map to virt addr = %p\n", map_base);
	printf("Config xflow(i.e mpu_wraper) register first\n");
	_VOLATILE_ unsigned int *virtual_mpu_wrapper_base_addr = (_VOLATILE_ unsigned int *)((char *)map_base + MPU_WRAPPER_REGISTER_OFFSET + MPU_WRAPPER_BASE_ADDR_REGISTER_OFFSET);
	_VOLATILE_ unsigned int *virtual_mpu_wrapper_control_addr = (_VOLATILE_ unsigned int *)((char *)map_base + MPU_WRAPPER_REGISTER_OFFSET + MPU_WRAPPER_CONTROL_REGISTER_OFFSET);

	*virtual_mpu_wrapper_control_addr = MPU_WRAPPER_CONTROL_REGISTER_ACCESS_OTHERS;

	printf("Config xflow base (%p) = 0x%x\n", &virtual_mpu_wrapper_base_addr[0], virtual_mpu_wrapper_base_addr[0]);


	_VOLATILE_ unsigned int *virtual_chip0_xflow_config_reg_addr = (_VOLATILE_ unsigned int *)((char *)map_base + CHIP0_XFLOW_CONFIG_REGISTER_OFFSET);
	*virtual_chip0_xflow_config_reg_addr = SYSTEM_MEMORY_ON_CHIP_RAM_DATA;
	if (*virtual_chip0_xflow_config_reg_addr != SYSTEM_MEMORY_ON_CHIP_RAM_DATA) {
		perror("fail to config xflow base addr\n");
		return -2;
	}
	printf("Config xflow enable (%p) = 0x%x\n", virtual_mpu_wrapper_control_addr, *virtual_mpu_wrapper_base_addr);
	if (*virtual_mpu_wrapper_control_addr != MPU_WRAPPER_CONTROL_REGISTER_ACCESS_OTHERS) {
		perror("fail to config xflow access others\n");
		return -3;
	}

	_VOLATILE_ unsigned int *virtual_chip0_vbuffer_map_sram_addr = (_VOLATILE_ unsigned int *)((char *)map_base + CHIP0_XFLOW_VIRTUAL_BUFFER_OFFSET);

	printf("Init Sram Content:\n");
	for (idx = 0; idx < fourByteCount; idx++)
		printf("Sram_addr(%p) = 0x%x\n", &virtual_chip0_vbuffer_map_sram_addr[idx], virtual_chip0_vbuffer_map_sram_addr[idx]);

	memcpy((void *)virtual_chip0_vbuffer_map_sram_addr, data, dataByteSize);

	printf("After Memcpy Sram Content:\n");
	for (idx = 0; idx < fourByteCount; idx++)
		printf("Sram_addr(%p) = 0x%x\n", &virtual_chip0_vbuffer_map_sram_addr[idx], virtual_chip0_vbuffer_map_sram_addr[idx]);

	close(chipfd);
	munmap(map_base, MMAP_SIZE_256M_BYTE);
	perror("close");
	return 0;
}

int file_operation_read_test(char *dve_file, unsigned int *data, unsigned int dataByteSize)
{
	int chipfd = 0;
	long ret = 0;
	unsigned int buf = 0;

	chipfd = open(dve_file, O_RDWR | O_SYNC);
	if (chipfd == -1) {
		perror("open chip dev fail (-1)");
		return -1;
	}

	read(chipfd, &buf, sizeof(unsigned int));

	close(chipfd);

	perror("close");
	return 0;
}

int file_operation_write_test(char *dve_file, unsigned int *data, unsigned int dataByteSize)
{
	int chipfd = 0;
	long ret = 0;
	unsigned int buf = 0;

	chipfd = open(dve_file, O_RDWR | O_SYNC);
	if (chipfd == -1) {
		perror("open chip dev fail (-1)");
		return -1;
	}

	write(chipfd, &buf, sizeof(unsigned int));

	close(chipfd);

	perror("close");
	return 0;
}

int msix_test(char *dve_file)
{
	int fd = 0;
	long ret = 0;
	char c = 0;

	printf("this test need to cowork with emu firmware which can random trigger msix interrupt\n");
	fd = open(dve_file, O_RDWR);
	if (fd == -1) {
		perror("open fail (-1)");
		return -1;
	}
	printf("enter 'q' will exit test and free the irq resource\n");
	do {
		c = getc(stdin);
	} while (c != 'q');
	close(fd);
	perror("close");
	return 0;
}


int main(void)
{
	int res = 0;
	unsigned int init_data[] = {0x400a8000, 0x8, 0x425, 0x0};
	unsigned int data[] = {0x12345678, 0x9ABCDEF0, 0x95279527, 0xDEADBEEF};

	res = msix_test(PCIE_DEV_FILE_0);
	if (res != 0)
		printf("ioctl_trigger_msix_test for %s Fail!(%d)\n", PCIE_DEV_FILE_0, res);
	else
		printf("ioctl_trigger_msix_test for %s SUCCESS!\n", PCIE_DEV_FILE_0);

	res = file_operation_read_test(PCIE_DEV_FILE_0, init_data, sizeof(init_data));
	if (res != 0)
		printf("read test for %s Fail!(%d)\n", PCIE_DEV_FILE_0, res);
	else
		printf("read test for %s success!\n", PCIE_DEV_FILE_0);


	res = file_operation_write_test(PCIE_DEV_FILE_0, init_data, sizeof(init_data));
	if (res != 0)
		printf("write test for %s Fail!(%d)\n", PCIE_DEV_FILE_0, res);
	else
		printf("write test for %s success!\n", PCIE_DEV_FILE_0);


	res = mmap_xflow_test(PCIE_DEV_FILE_0, data, sizeof(data));
	if (res != 0)
		printf("mmap pci test for %s Fail!(%d)\n", PCIE_DEV_FILE_0, res);
	else
		printf("mmap_test for %s SUCCESS!\n", PCIE_DEV_FILE_0);


	//res = ioctl_firmware_download_test(PCIE_DEV_FILE_0);
	//if (res != 0)
	//	printf("ioctl_firmware_download_test for %s Fail!(%d)\n", PCIE_DEV_FILE_0, res);
	//else
	//	printf("ioctl_firmware_download_test for %s SUCCESS!\n", PCIE_DEV_FILE_0);

	return 0;
}
