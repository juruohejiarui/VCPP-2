#ifndef __HARDWARE_UEFI_H__
#define __HARDWARE_UEFI_H__

#include "../includes/lib.h"

// ACPI2.0 GUID
#define HW_UEFI_GUID_ACPI2_data1 0x8868e871
#define HW_UEFI_GUID_ACPI2_data2 0xe4f1
#define HW_UEFI_GUID_ACPI2_data3 0x11d3
#define HW_UEFI_GUID_ACPI2_data4 {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81}

typedef struct {
	u8 signature[4];
	u32 length;
	u8 revision;
	u8 chkSum;
	u8 OEMID[6];
	u8 OEMTableID[8];
	u32 OEMRevision;
	u32 creatorID;
	u32 creatorRevision;
} __attribute__ ((packed)) ACPIHeader;

// ACPI2.0 RSDP
typedef struct {
	u8 signature[8];
	u8 chkSum;
	u8 OEMID[6];
	u8 revision;
	u32 RsdtAddress;
	u32 length;
	u64 xsdtAddr;
	u8 extendedChkSum;
	u8 reserved[3];
} __attribute__ ((packed)) RSDPDescriptor;

// ACPI2.0 XSDT
typedef struct {
	ACPIHeader header;
	u64 entry[0];
} __attribute__ ((packed)) XSDTDescriptor;

struct EFI_GraphicsOutputInfo
{
	u32 HorizontalResolution;
	u32 VerticalResolution;
	u32 PixelsPerScanLine;

	u64 FrameBufferBase;
	u64 FrameBufferSize;
};

typedef struct tmpEFIE820MemoryDescriptor
{
	u64 address;
	u64 length;
	u32 type;
} __attribute__((packed)) EFI_E820MemoryDescriptor;

struct EFI_E820MemoryDescriptorInfo
{
	u32 entryCount;
	EFI_E820MemoryDescriptor entry[0];
};

typedef struct {
	u32 Data1;
	u16 Data2;
	u16 Data3;
	u8 Data4[8];
} EFI_GUID;

struct EFI_CONFIGURATION_TABLE {
	EFI_GUID VendorGuid;
	void *VendorTable;
};

struct KernelBootParameterInfo
{
	u64 ConfigurationTableCount;
	struct EFI_CONFIGURATION_TABLE *ConfigurationTable;
	struct EFI_GraphicsOutputInfo graphicsInfo;
	struct EFI_E820MemoryDescriptorInfo E820Info;
	u64 memDesc;
	u64 memDescSize;
};

extern struct KernelBootParameterInfo *HW_UEFI_bootParamInfo;

void HW_UEFI_init();

RSDPDescriptor *HW_UEFI_getRSDP();
XSDTDescriptor *HW_UEFI_getXSDT();

#endif