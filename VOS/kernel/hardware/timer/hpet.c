#include "hpet.h"
#include "../../includes/hardware.h"
#include "../../includes/log.h"
#include "../../includes/memory.h"
#include "../../includes/interrupt.h"

static u32 _minTick = 0;

IntrController HW_Timer_HPET_intrController;
IntrHandler HW_Timer_HPET_intrHandler;
APICRteDescriptor HW_Timer_HPET_intrDesc;

HPETDescriptor *_hpetDesc;
static u64 _jiffies = 0;

static RBTree _rqTree;

static inline void _setTimerConfig(u32 id, u64 config) {
	*(u64 *)(DMAS_phys2Virt(_hpetDesc->address.Address) + 0x100 + 0x20 * id) = config;
	IO_mfence();
}
static inline void _setTimerComparator(u32 id, u64 comparator) {
	*(u64 *)(DMAS_phys2Virt(_hpetDesc->address.Address) + 0x108 + 0x20 * id) = comparator;
	IO_mfence();
}

IntrHandlerDeclare(HW_Timer_HPET_handler) {
	// print the counter
	printk(RED, BLACK, "HPET\t");
	_jiffies++;
	
}

void HW_Timer_HPET_init() {
	// initializ the data structure
	_jiffies = 0;
	RBTree_init(&_rqTree);
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
	_hpetDesc = NULL;
	for (i32 i = 0; i < xsdt->length; i++) {
		HPETDescriptor *desc = (HPETDescriptor *)DMAS_phys2Virt(xsdt->entry[i]);
		if (desc->signature[0] == 'H' && desc->signature[1] == 'P' && desc->signature[2] == 'E' && desc->signature[3] == 'T') {
			_hpetDesc = desc;
			break;
		}
	}
	if (_hpetDesc == NULL) {
		printk(RED, BLACK, "HPET not found\n");
		return;
	} else {
		printk(RED, BLACK, "HPET found at %#018lx, addres: %#018lx\n", _hpetDesc, _hpetDesc->address.Address);
	}
	
	// initialize controller
	HW_Timer_HPET_intrController.install = HW_APIC_install;
	HW_Timer_HPET_intrController.uninstall = HW_APIC_uninstall;

	HW_Timer_HPET_intrController.enable = HW_APIC_enableIntr;
	HW_Timer_HPET_intrController.disable = HW_APIC_disableIntr;
	HW_Timer_HPET_intrController.ack = NULL;

	// initialize handler
	HW_Timer_HPET_intrHandler = HW_Timer_HPET_handler;

	// initialize descriptor
	memset(&HW_Timer_HPET_intrDesc, 0, sizeof(APICRteDescriptor));
	HW_Timer_HPET_intrDesc.vector = 0x22;
	HW_Timer_HPET_intrDesc.deliveryMode = HW_APIC_DeliveryMode_Fixed;
	HW_Timer_HPET_intrDesc.destMode = HW_APIC_DestMode_Physical;
	HW_Timer_HPET_intrDesc.deliveryStatus = HW_APIC_DeliveryStatus_Idle;
	HW_Timer_HPET_intrDesc.pinPolarity = HW_APIC_PinPolarity_High;
	HW_Timer_HPET_intrDesc.remoteIRR = HW_APIC_RemoteIRR_Reset;
	HW_Timer_HPET_intrDesc.triggerMode = HW_APIC_TriggerMode_Edge;
	HW_Timer_HPET_intrDesc.mask = HW_APIC_Mask_Masked;
	
	Intr_register(0x22, &HW_Timer_HPET_intrDesc, HW_Timer_HPET_handler, 0, &HW_Timer_HPET_intrController, "HPET");
	// set the general configuration register
	*(u64 *)(DMAS_phys2Virt(_hpetDesc->address.Address) + 0x10) = 0x3;
	IO_mfence();
	_setTimerConfig(0, 0x4c);
	_setTimerComparator(0, 14318179);
	*(u64 *)(DMAS_phys2Virt(_hpetDesc->address.Address) + 0xf0) = 0x0;
	IO_mfence();
}

u64 HW_Timer_HPET_jiffies() { return _jiffies; }
