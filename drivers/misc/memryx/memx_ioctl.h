/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef MEMX_IOCTL_H_
#define MEMX_IOCTL_H_
// Note: For multiple platform maintain reason, we keep two same memx_ioctl.h header files in different include path.
//	   one is in kdriver/include/memx_ioctl, the other is in udriver/include/mpuio/memx_ioctl.h.
//	   if anyone need to modify one of those ioctl file, please remember to update the other one.

// common define
#define MEMX_TOTAL_FLOW_COUNT	(32)
#define FILE_NAME_LENGTH		 (20)
#define FIRMWARE_CMD_DATA_DWORD_COUNT (63)
#define MAX_SUPPORT_CHIP_NUM (16)
#define MAX_SUPPORT_GROUP_NUM (8)
#define KBYTE (1024)

enum {
	DFP_RESERVED_0				= 0, //Remove Unsupport Option
	DFP_RESERVED_1				= 1, //Remove Unsupport Option
	DFP_FROM_SEPERATE_WTMEM		= 2,
	DFP_FROM_SEPERATE_CONFIG	= 3
};

enum memx_chip_role {
	ROLE_SINGLE = 0x0,
	ROLE_MULTI_FIRST = 0x1,
	ROLE_MULTI_LAST = 0x2,
	ROLE_MULTI_MIDDLE = 0x3,
	ROLE_UNCONFIGURED = 0xff
};

enum PCIE_FW_CMD_ID {
	PCIE_CMD_WAIT_FOR_ACK_ONLY		= 0,
	PCIE_CMD_VENDOR_1				= 1,
	PCIE_CMD_INIT_HOST_BUF_MAPPING  = 2,
	PCIE_CMD_GET_HW_INFO			= 3,
	PCIE_CMD_VENDOR_0			    = 4,
	PCIE_CMD_INIT_WTMEM_FMAP		= 5,
	PCIE_CMD_RESET_MPU			    = 6,
	PCIE_CMD_CONFIG_MPU_GROUP	    = 7,
	PCIE_CMD_SET_FEATURE			= 8,
	PCIE_MAX_SUPPORT_FW_CMD
};

enum PCIE_CMD_VENDOR_0_OPTION {
	DFP_DOWNLOAD_WEIGHT_MEMORY	= 0x01,
	DFP_DOWNLOAD_REG_CONFIG		= 0x02
};

enum ADMIN_CMD_OPCODE {
    MEMX_ADMIN_CMD_SET_FEATURE  = 0x1,
    MEMX_ADMIN_CMD_GET_FEATURE  = 0x2,
    MEMX_ADMIN_CMD_DOWNLOAD_DFP = 0x3,
    MEMX_ADMIN_CMD_SELFTEST     = 0x4,
    MEMX_ADMIN_CMD_DEVIOCTRL    = 0x5,
    MEMX_ADMIN_CMD_MAX
};

enum CASCADE_PLUS_ADMINCMD_STATUS {
	STATUS_IDLE     = 0x0,
	STATUS_RECEIVE  = 0x1,
	STATUS_PROCESS  = 0x2,
	STATUS_COMPLETE = 0x3
};

enum CASCADE_PLUS_ADMINCMD_ERROR_STATUS {
	ERROR_STATUS_NO_ERROR 					= 0x0,
	ERROR_STATUS_PARAMETER_FAIL 			= 0x1,
	ERROR_STATUS_OPCODE_NOT_SUPPORT_FAIL 	= 0x2,
	ERROR_STATUS_SUBOP_NOT_SUPPORT_FAIL 	= 0x3,
	ERROR_STATUS_UNKNOWN_FAIL 				= 0x4,
	ERROR_STATUS_TIMEOUT_FAIL 				= 0x5,
	ERROR_STATUS_MAX
};

struct mpu_group {
	unsigned char input_chip_id;
	unsigned char output_chip_id;
};

enum memx_pcie_bar_mode {
	MEMXBAR_XFLOW256MB_SRAM1MB                   = 0,
	MEMXBAR_SRAM1MB                              = 1,
	MEMXBAR_XFLOW128MB64B_SRAM1MB                = 2,
	MEMXBAR_3BAR_BAR0VB_BAR2CI_16MB_BAR4SRAM     = 3,
	MEMXBAR_3BAR_BAR0VB_BAR2CI_64MB_BAR4SRAM     = 4,
	MEMXBAR_4BAR_BAR0VB_BAR2CI_BAR4MSIX_BAR5SRAM = 5,

	MEMXBAR_NOTVALID                             = 0xFF,
	MEMXBAR_XFLOW128MB_SRAM1MB = MEMXBAR_NOTVALID,
};

enum CASCADE_PLUS_INPUT_DMA_TRIGGER_TYPE {
  INPUT_DMA_TRIGGER_TYPE_CHIP = 0x0,
  INPUT_DMA_TRIGGER_TYPE_HOST = 0x1,
  INPUT_DMA_TRIGGER_TYPE_MAX
};

struct chip_info {
	unsigned short generation;
	unsigned char total_chip_cnt;
	unsigned char curr_config_chip_count;
	unsigned char group_count;
	enum memx_pcie_bar_mode pcie_bar_mode;
	enum memx_chip_role roles[MAX_SUPPORT_CHIP_NUM];
	struct mpu_group groups[MAX_SUPPORT_CHIP_NUM];
	enum CASCADE_PLUS_INPUT_DMA_TRIGGER_TYPE input_dma_trigger_type[MAX_SUPPORT_CHIP_NUM];
};

struct fw_info {
	unsigned int bar0_mapping_mpu_base;		 // fixed 0x30000000 which set for each chip
	unsigned int bar1_mapping_sram_base;		// fixed 0x40000000 which set for each chip

	unsigned int firmware_download_sram_base;   // fixed 0x40040000 which set for each chip
	unsigned int firmware_command_sram_base;	// fixed 0x40046E00 which set for each chip
	unsigned long long rx_dma_coherent_buffer_base;   // dynamic and only one which get from probe via platform dma api then set to firmware for each chip to transmit data back to host directly.

	unsigned int ingress_dcore_mapping_sram_base[MAX_SUPPORT_CHIP_NUM];
	unsigned int egress_dcore_mapping_sram_base[MAX_SUPPORT_CHIP_NUM];
	unsigned int egress_dcore_rx_dma_buffer_offset[MAX_SUPPORT_CHIP_NUM];
};

struct hw_info {
	struct chip_info chip;
	struct fw_info   fw;
};

struct memx_mpu_size {
	unsigned int cfg_size;
	unsigned int flow_size[MEMX_TOTAL_FLOW_COUNT]; // TODO: Remove it since we don't use flow size?
	unsigned int buffer_size[MEMX_TOTAL_FLOW_COUNT];
	unsigned int usb_first_chip_pipeline_flag;
	unsigned int usb_last_chip_pingpong_flag;
};

struct memx_reg {
	unsigned char *buf_addr;
	unsigned int  reg_start;
	unsigned int  size;
};

struct memx_bin {
	char file_name[FILE_NAME_LENGTH];
	unsigned char dfp_src;
	unsigned int dfp_cnt;
	unsigned int total_size;
	unsigned char *buf;
};

struct memx_firmware_bin {
	char name[FILE_NAME_LENGTH];
	unsigned char *buffer;
	unsigned int size;
#ifdef __linux__
	unsigned int request_firmware_update_in_linux;
#endif
};

struct memx_flow {
	unsigned char flow_id;
};

struct memx_chip_id {
	unsigned char chip_id;
};

struct memx_driver_info {
	unsigned int CDW[16];
};

struct memx_throughput_info {
	unsigned int stream_write_us;
	unsigned int stream_write_kb;
	unsigned int stream_read_us;
	unsigned int stream_read_kb;
};

struct memx_xflow_param {
	unsigned char chip_id;
	unsigned char access_mpu;
	unsigned char is_read;
	unsigned char reserved;
	unsigned int base_addr;
	unsigned int base_offset;
	unsigned int value;
};

struct fw_hw_info_pkt {
	unsigned int chip_role[MAX_SUPPORT_CHIP_NUM];
	unsigned int total_chip_cnt;
	unsigned int igr_buf_sram_base_addr;
	unsigned int egr_pbuf_sram_base_addr[MAX_SUPPORT_CHIP_NUM];
	unsigned int egr_dst_buf_start_addr[MAX_SUPPORT_CHIP_NUM];
	unsigned int chip_generation;
	unsigned int reserved[12];
};

enum CMD_FEATURE_ID {
    FID_DEVICE_FW_INFO                  = 0x01,
    FID_DEVICE_TEMPERATURE              = 0x02,
    FID_DEVICE_INFO                     = 0x03,
    FID_DEVICE_FREQUENCY                = 0x04,
    FID_DEVICE_VOLTAGE                  = 0x05,
    FID_DEVICE_THROUGHPUT               = 0x06,
    FID_DEVICE_POWER                    = 0x07,
    FID_DEVICE_POWERMANAGEMENT          = 0x08,
    FID_DEVICE_POWER_THRESHOLD          = 0x09,
    FID_DEVICE_POWER_ALERT              = 0x0a,
    FID_DEVICE_POWER_ALERT_FREQUENCY    = 0x0b,
    FID_DEVICE_INTERFACE_INFO           = 0x0c,
    FID_DEVICE_I2C_TRANSCEIVE           = 0x0d,
    FID_DEVICE_GPIO                     = 0x0e,
    FID_DEVICE_MPU_UTILIZATION          = 0x0f,
    FID_DEVICE_HW_INFO                  = 0x10,
    FID_DEVICE_FREQUENCY_EFFECTIVE      = 0x11,
    FID_DEVICE_DMA_TRIGGER_TYPE         = 0x12,
    FID_FW_MAX
};

enum CMD_SELFTEST_ID {
    TESTID_PCIE_BANDWIDTH                  = 0x01,
	TESTID_FW_MAX
};

struct transport_sq {
	unsigned int opCode	 : 16;
	unsigned int cmdLen	 : 16;
	unsigned int subOpCode  : 8;
	unsigned int attr	   : 8;
	unsigned int reqLen	 : 16;
	unsigned int cdw2;
	unsigned int cdw3;
	unsigned int cdw4;
	unsigned int cdw5;
	unsigned int cdw6;
	unsigned int cdw7;
	unsigned int cdw8;
	unsigned int cdw9;
	unsigned int cdw10;
	unsigned int cdw11;
	unsigned int cdw12;
	unsigned int cdw13;
	unsigned int cdw14;
	unsigned int cdw15;
};


#define CQ_DATA_LEN 31
struct transport_cq {
	unsigned int status;
	unsigned int data[CQ_DATA_LEN];
};

struct transport_cmd {
	union{
		unsigned int sq_data[16];
		struct transport_sq SQ;
	};
	union{
		unsigned int cq_data[CQ_DATA_LEN+1];
		struct transport_cq CQ;
	};
};

#define MEMX_CHIP_SRAM_BASE		   (0x40000000)
#define MEMX_CHIP_SRAM_DATA_SRAM_OFFS (0x40000)
#define MEMX_CHIP_SRAM_MAX_SIZE	   (0x100000)

// EXTINFO CMD for single bar mode
#define MEMX_EXTINFO_CMD_BASE		 (0x40046D80)
#define MEMX_EXTINFO_DATA_BASE		(0x40046D88)
#define MEMX_EXTINFO_RESULT_BASE	  (0x40046D8C)
#define MEMX_EXTINFO_EN_BUF0_IRQ_BASE (0x40046DC0)

#define MEMX_EXTINFO_CMD(chip_id, cmd_code, access_mode) ((unsigned int)((chip_id << 28) | (access_mode << 4) | cmd_code))
#define MEMX_EXTINFO_CMD_TIMEOUT (1e6)

#define MEMX_CHIP_HOST_RMT_CMD (0x40046F44)

#define MEMX_CHIP_HOST_RMT_CMD_ADMIN_SQ	  (0x40046C00)
    #define U32_ADMCMD_CQ_STATUS_OFFSET     (16)
	#define U32_ADMCMD_CQ_DATA_OFFSET		(17)
	#define U32_ADMCMD_STATUS_OFFSET       	(48)
#define MEMX_CHIP_ADMIN_TASK_EN_ADR       (0x40046CC4)

#define MEMX_ADMCMD_VIRTUAL_OFFSET  	(0x1FF300)
#define MEMX_ADMCMD_VIRTUAL_PAGE_OFFSET (0x1FF000)
#define MEMX_ADMCMD_VIRTUAL_PAGE_SIZE   (0x1000)
#define MEMX_ADMCMD_SIZE				(208)

enum memx_extend_cmd {
	MEMX_EXTCMD_COMPLETE	  = 0x0000000,
	MEMX_EXTCMD_XFLOW_WRITE_SRAM,
	MEMX_EXTCMD_XFLOW_READ_SRAM,
	MEMX_EXTCMD_XFLOW_WRITE_REG,
	MEMX_EXTCMD_XFLOW_READ_REG,
};

#ifdef __linux__
#include <linux/ioctl.h>

// device files
#define MEMX_PCIE_SINGLE_DEV_FILE		"/dev/memx"  // PCIE signle device
#define MEMX_PCIE_CASCADE_IN_DEV_FILE	"/dev/memxi" // PCIE cascade plus in device
#define MEMX_PCIE_CASCADE_OUT_DEV_FILE   "/dev/memxo" // PCIE cascade plus out device

#define MEMX_USB_SINGLE_DEV_FILE		"/dev/memxchip"  // USB signle device
#define MEMX_USB_CASCADE_IN_DEV_FILE	"/dev/memxchipo" // USB cascade bulkin device
#define MEMX_USB_CASCADE_OUT_DEV_FILE   "/dev/memxchipi" // USB cascade bulkout device

#define MEMX_IOC_MAJOR 'u'
#define MEMX_DRIVER_MPU_IN_SIZE  _IOW(MEMX_IOC_MAJOR, 1, struct memx_mpu_size)
#define MEMX_DRIVER_MPU_OUT_SIZE _IOW(MEMX_IOC_MAJOR, 2, struct memx_mpu_size)
#define MEMX_FW_MPU_OUT_SIZE     _IOW(MEMX_IOC_MAJOR, 3, struct memx_mpu_size)
#define MEMX_DOWNLOAD_FIRMWARE   _IOW(MEMX_IOC_MAJOR, 4, struct memx_firmware_bin)
#define MEMX_DOWNLOAD_DFP        _IOW(MEMX_IOC_MAJOR, 5, struct memx_bin)
#define MEMX_RUNTIMEDWN_DFP      _IOW(MEMX_IOC_MAJOR, 6, struct memx_bin)
#define MEMX_WRITE_REG           _IOW(MEMX_IOC_MAJOR, 7, struct memx_reg)
#define MEMX_READ_REG            _IOR(MEMX_IOC_MAJOR, 8, struct memx_reg)
#define MEMX_IFMAP_FLOW          _IOW(MEMX_IOC_MAJOR, 9, struct memx_flow)
#define MEMX_ABORT_TRANSFER      _IO(MEMX_IOC_MAJOR, 10)
#define MEMX_SET_CHIP_ID         _IOW(MEMX_IOC_MAJOR, 11, struct memx_chip_id)
#define MEMX_READ_CHIP_ID        _IOR(MEMX_IOC_MAJOR, 12, struct memx_reg)
#define MEMX_GET_FWUPDATE_STATUS _IOR(MEMX_IOC_MAJOR, 13, struct memx_reg)
#define MEMX_WAIT_FW_MSIX_ACK    _IO(MEMX_IOC_MAJOR, 14)
#define MEMX_GET_HW_INFO         _IOR(MEMX_IOC_MAJOR, 15, struct hw_info)
#define MEMX_CONFIG_MPU_GROUP    _IOWR(MEMX_IOC_MAJOR, 16, struct hw_info)
#define MEMX_INIT_WTMEM_FMAP     _IOW(MEMX_IOC_MAJOR, 17, struct memx_chip_id)
#define MEMX_RESET_DEVICE        _IO(MEMX_IOC_MAJOR, 18)
#define MEMX_GET_DEVICE_FEATURE  _IOWR(MEMX_IOC_MAJOR, 19, struct transport_cmd)
#define MEMX_SET_DEVICE_FEATURE  _IOWR(MEMX_IOC_MAJOR, 20, struct transport_cmd)
#define MEMX_SET_THROUGHPUT_INFO _IOWR(MEMX_IOC_MAJOR, 21, struct memx_throughput_info)
#define MEMX_VENDOR_CMD          _IOWR(MEMX_IOC_MAJOR, 22, struct transport_cmd)
#define MEMX_DUMMY_READ          _IO(MEMX_IOC_MAJOR, 23)
#define MEMX_SET_ABORT_READ      _IO(MEMX_IOC_MAJOR, 24)
#define MEMX_ADMIN_DOWNLOAD_DFP  _IOWR(MEMX_IOC_MAJOR, 25, struct transport_cmd)
#define MEMX_ADMIN_COMMAND  	 _IOWR(MEMX_IOC_MAJOR, 26, struct transport_cmd)
#define MEMX_IOC_MAXNR (26)

#elif _WIN32
//#include <stdint.h>
#include <initguid.h>

// device files
// {8119A3E8-E632-4CD2-B65F-E835670C83B5}
DEFINE_GUID(GUID_CLASS_MEMX_CASCADE_SINGLE_PCIE, 0x8119a3e8, 0xe632, 0x4cd2,
			0xb6, 0x5f, 0xe8, 0x35,
			0x67, 0xc, 0x83, 0xb5);

// {6068EB61-98E7-4c98-9E20-1F068295909A}
DEFINE_GUID(GUID_CLASS_MEMX_CASCADE_SINGLE_USB,
	0x873fdf, 0x61a8, 0x11d1, 0xaa, 0x5e, 0x0, 0xc0, 0x4f, 0xb1, 0x72, 0x8b);

// Group 0 ID
// {EFDAB47D-C95B-4A83-939D-29508108DC1D}
DEFINE_GUID(GUID_CLASS_MEMX_CASCADE_MUTLI_G0_FIRST_USB,
	0xefdab47d, 0xc95b, 0x4a83, 0x93, 0x9d, 0x29, 0x50, 0x81, 0x8, 0xdc, 0x1d);
// {788BA88D-7436-4F05-94F2-293697DAEB97}
DEFINE_GUID(GUID_CLASS_MEMX_CASCADE_MUTLI_G0_LAST_USB,
	0x788ba88d, 0x7436, 0x4f05, 0x94, 0xf2, 0x29, 0x36, 0x97, 0xda, 0xeb, 0x97);

// Group 1 ID
// {7247A4D2-1F68-4654-8207-7C1B5F087802}
DEFINE_GUID(GUID_CLASS_MEMX_CASCADE_MUTLI_G1_FIRST_USB,
	0x7247a4d2, 0x1f68, 0x4654, 0x82, 0x7, 0x7c, 0x1b, 0x5f, 0x8, 0x78, 0x2);
// {2F87FF5E-7D69-49BB-ABB0-7E484E288459}
DEFINE_GUID(GUID_CLASS_MEMX_CASCADE_MUTLI_G1_LAST_USB,
	0x2f87ff5e, 0x7d69, 0x49bb, 0xab, 0xb0, 0x7e, 0x48, 0x4e, 0x28, 0x84, 0x59);

// Group 2 ID
// {DE6DACBA-B927-4A4B-BE66-312D4207C4B5}
DEFINE_GUID(GUID_CLASS_MEMX_CASCADE_MUTLI_G2_FIRST_USB,
	0xde6dacba, 0xb927, 0x4a4b, 0xbe, 0x66, 0x31, 0x2d, 0x42, 0x7, 0xc4, 0xb5);
// {F96239AB-7790-42AC-B4A2-9D0FE2F8FA42}
DEFINE_GUID(GUID_CLASS_MEMX_CASCADE_MUTLI_G2_LAST_USB,
	0xf96239ab, 0x7790, 0x42ac, 0xb4, 0xa2, 0x9d, 0xf, 0xe2, 0xf8, 0xfa, 0x42);

// Group 3 ID
// {185BC9E9-BB7A-4DAE-8CED-99C6A5724E8B}
DEFINE_GUID(GUID_CLASS_MEMX_CASCADE_MUTLI_G3_FIRST_USB,
	0x185bc9e9, 0xbb7a, 0x4dae, 0x8c, 0xed, 0x99, 0xc6, 0xa5, 0x72, 0x4e, 0x8b);
// {5489BF82-CF5A-4050-AB1F-BFFC477E3F66}
DEFINE_GUID(GUID_CLASS_MEMX_CASCADE_MUTLI_G3_LAST_USB,
	0x5489bf82, 0xcf5a, 0x4050, 0xab, 0x1f, 0xbf, 0xfc, 0x47, 0x7e, 0x3f, 0x66);


#define MEMX_IOCTL_INDEX			 0x0000
// MEMX IOCTL
#define MEMX_USB_GET_CONFIG_DESCRIPTOR	CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX,	  \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_RESET_DEVICE				CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 1,  \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_USB_RESET_PIPE				CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 2,  \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_DOWNLOAD_FIRMWARE			CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													MEMX_IOCTL_INDEX + 3,   \
													METHOD_BUFFERED,		\
													FILE_ANY_ACCESS)

#define MEMX_RUNTIMEDWN_DFP				CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 4,  \
													 METHOD_IN_DIRECT,	   \
													 FILE_ANY_ACCESS)

#define MEMX_IFMAP_FLOW					CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 5,  \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_DRIVER_MPU_IN_SIZE			CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 6,  \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_ABORT_TRANSFER				CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 7,  \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_GET_FWUPDATE_STATUS		CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 8,  \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_WAIT_FW_MSIX_ACK			CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 9,  \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_SET_CHIP_ID				CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 10, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_READ_REG					CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 11, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)
#define MEMX_GET_HW_INFO				CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 12, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_INIT_WTMEM_FMAP			CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 13, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_RESET_MPU					CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 14, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_XFLOW_ACCESS				CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 15, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_SET_DRIVER_INFO			CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 16, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_CONFIG_MPU_GROUP			CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 17, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_GET_DEVICE_FEATURE			CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 18, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_SET_DEVICE_FEATURE			CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 19, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_SET_THROUGHPUT_INFO		CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 20, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_ADMIN_COMMAND				CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 21, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_DEVICE_IRQ					CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 22, \
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define MEMX_WRITE_REG					CTL_CODE(FILE_DEVICE_UNKNOWN,	   \
													 MEMX_IOCTL_INDEX + 23,\
													 METHOD_BUFFERED,	   \
													 FILE_ANY_ACCESS)

#define UNIMPLEMENT_METHOD  0xff
#define MEMX_READ_CHIP_ID   MEMX_READ_REG


typedef struct memx_device_irq_param {
    unsigned char chip_id;
    unsigned int irq_id;
} memx_device_irq_param_t;

#endif
#endif /* MEMX_IOCTL_H_ */
