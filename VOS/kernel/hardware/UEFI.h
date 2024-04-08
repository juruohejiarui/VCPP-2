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

struct KernelBootParameterInfo
{
	struct EFI_GraphicsOutputInfo graphicsInfo;
	struct EFI_E820MemoryDescriptorInfo E820Info;
};

extern struct KernelBootParameterInfo *bootParamInfo;
#endif