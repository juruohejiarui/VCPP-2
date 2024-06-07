#include "UEFI.h"
#include "../includes/memory.h"

struct KernelBootParameterInfo *HW_UEFI_bootParamInfo = (struct KernelBootParameterInfo *)0xffff800000060000;

#include "../includes/log.h"

RSDPDescriptor *_rsdpTable;
XSDTDescriptor *_xsdtTable;

RSDPDescriptor *HW_UEFI_getRSDP() { return _rsdpTable; }
XSDTDescriptor *HW_UEFI_getXSDT() { return _xsdtTable; }

void HW_UEFI_init() {
	printk(RED, BLACK, "HW_UEFI_init()\n");
    _rsdpTable = NULL;
	_xsdtTable = NULL;
    // find RSDP in configuration table
	i64 rsdpId = -1;
	u8 tmp_data4[8] = HW_UEFI_GUID_ACPI2_data4;
	printk(WHITE, BLACK, "Search for RSDP in configuration table(%#018lx)\n", HW_UEFI_bootParamInfo->ConfigurationTable);
	for (u64 i = 0; i < HW_UEFI_bootParamInfo->ConfigurationTableCount; i++) {
		EFI_GUID *guid = &((struct EFI_CONFIGURATION_TABLE *)DMAS_phys2Virt(HW_UEFI_bootParamInfo->ConfigurationTable))[i].VendorGuid;
		// compare the GUID with HW_UEFI_GUID_RSDP
		if (guid->Data1 != HW_UEFI_GUID_ACPI2_data1 || guid->Data2 != HW_UEFI_GUID_ACPI2_data2 || guid->Data3 != HW_UEFI_GUID_ACPI2_data3)
			continue;
		u8 *data4 = guid->Data4;
		for (u64 j = 0; j < 8; j++) {
			if (data4[j] != tmp_data4[j]) break;
			if (j == 7) rsdpId = i;
		}
	}
	if (rsdpId == -1) {
		printk(WHITE, BLACK, "RSDP not found\n");
		return;
	}
	printk(WHITE, BLACK, "RSDP found at %#018lx\t", ((struct EFI_CONFIGURATION_TABLE *)DMAS_phys2Virt(HW_UEFI_bootParamInfo->ConfigurationTable))[rsdpId].VendorTable);
	// find HPET in RSDP
	_rsdpTable = (RSDPDescriptor *)DMAS_phys2Virt(((struct EFI_CONFIGURATION_TABLE *)DMAS_phys2Virt(HW_UEFI_bootParamInfo->ConfigurationTable))[rsdpId].VendorTable);
    // get XSDT address
	_xsdtTable = (XSDTDescriptor *)DMAS_phys2Virt(_rsdpTable->xsdtAddr);
	printk(WHITE, BLACK, "XSDT at %#018lx\n", _xsdtTable);
}