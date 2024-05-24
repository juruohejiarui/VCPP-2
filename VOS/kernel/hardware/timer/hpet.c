#include "hpet.h"
#include "../../includes/hardware.h"
#include "../../includes/log.h"
#include "../../includes/memory.h"

void HW_Timer_HPET_init() {
	// find RSDP in configuration table
	i64 rsdpId = -1;
	u8 tmp_data4[8] = HW_UEFI_GUID_ACPI2_data4;
	printk(RED, BLACK, "Search for RSDP in configuration table(%#018lx)\n", HW_UEFI_bootParamInfo->ConfigurationTable);
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
		if (rsdpId != -1) break;
	}
	if (rsdpId == -1) {
		printk(RED, BLACK, "RSDP not found\n");
		return;
	}
	printk(RED, BLACK, "RSDP found at %#018lx\t", ((struct EFI_CONFIGURATION_TABLE *)DMAS_phys2Virt(HW_UEFI_bootParamInfo->ConfigurationTable))[rsdpId].VendorTable);
	// find HPET in RSDP
	RSDPDescriptor *rsdpTable = (RSDPDescriptor *)DMAS_phys2Virt(((struct EFI_CONFIGURATION_TABLE *)DMAS_phys2Virt(HW_UEFI_bootParamInfo->ConfigurationTable))[rsdpId].VendorTable);
	// print the signature
	for (int i = 0; i < 8; i++)
		putchar(BLACK, WHITE, rsdpTable->signature[i]);
	putchar(BLACK, WHITE, '\t');
	// get XSDT address
	XSDTDescriptor *xsdt = (XSDTDescriptor *)DMAS_phys2Virt(rsdpTable->xsdtAddr);
	// print the signature
	for (int i = 0; i < 4; i++)
		putchar(BLACK, WHITE, xsdt->signature[i]);
	putchar(BLACK, WHITE, '\n');

	// find HPET in XSDT
	HPETDescriptor *hpetDesc = NULL;
	for (i32 i = 0; i < xsdt->length; i++) {
		HPETDescriptor *desc = (HPETDescriptor *)DMAS_phys2Virt(xsdt->entry[i]);
		if (desc->signature[0] == 'H' && desc->signature[1] == 'P' && desc->signature[2] == 'E' && desc->signature[3] == 'T') {
			hpetDesc = desc;
			break;
		}
	}
	if (hpetDesc == NULL) {
		printk(RED, BLACK, "HPET not found\n");
		return;
	}
	printk(RED, BLACK, "HPET found at %#018lx\n", hpetDesc);
}