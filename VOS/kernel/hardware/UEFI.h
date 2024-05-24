#ifndef __HARDWARE_UEFI_H__
#define __HARDWARE_UEFI_H__

#include "../includes/lib.h"

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
};

extern struct KernelBootParameterInfo *bootParamInfo;
#endif