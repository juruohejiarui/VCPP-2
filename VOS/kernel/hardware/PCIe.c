#include "PCIe.h"
#include "../includes/memory.h"
#include "../includes/log.h"

PCIeDescriptor *_desc;
i32 devCnt;

char *PCIeClassDesc[256][256] = {
    [0x01][0x0] = "SCSI Bus Controller",
    [0x01][0x1] = "IDE Controller",
    [0x01][0x2] = "Floppy Disk Controller",
    [0x01][0x3] = "IPI Controller",
    [0x01][0x4] = "RAID Controller",
    [0x01][0x5] = "ATA Controller",
    [0x01][0x6] = "SATA Controller",
    [0x01][0x7] = "SAS Controller",
    [0x01][0x8] = "NVM Controller",
    [0x01][0x80] = "Other Mass Storage Controller",

    [0x02][0x0] = "Ethernet Controller",
    [0x02][0x1] = "Token Ring Controller",
    [0x02][0x2] = "FDDI Controller",
    [0x02][0x3] = "ATM Controller",
    [0x02][0x4] = "ISDN Controller",
    [0x02][0x5] = "WorldFip Controller",
    [0x02][0x6] = "PICMG Controller",
    [0x02][0x80] = "Other Network Controller",

    [0x03][0x0] = "VGA Controller",
    [0x03][0x1] = "XGA Controller",
    [0x03][0x2] = "3D Controller",
    [0x03][0x80] = "Other Display Controller",
    [0x04][0x0] = "Multimedia Video Controller",
    [0x04][0x1] = "Multimedia Audio Controller",
    [0x04][0x2] = "Computer Telephony Device",
    [0x04][0x3] = "Audio Device",
    [0x04][0x80] = "Other Multimedia Controller",

    [0x05][0x0] = "RAM Controller",
    [0x05][0x1] = "Flash Controller",
    [0x05][0x80] = "Other Memory Controller",

    [0x06][0x0] = "Host Bridge",
    [0x06][0x1] = "ISA Bridge",
    [0x06][0x2] = "EISA Bridge",
    [0x06][0x3] = "MCA Bridge",
    [0x06][0x4] = "PCI-to-PCI Bridge",
    [0x06][0x5] = "PCMCIA Bridge",
    [0x06][0x6] = "NuBus Bridge",
    [0x06][0x7] = "CardBus Bridge",
    [0x06][0x8] = "RACEway Bridge",
    [0x06][0x9] = "PCI-to-PCI Bridge",
    [0x06][0xA] = "InfiniBand-to-PCI Host Bridge",
    [0x06][0x80] = "Other Bridge Device",

    [0x07][0x0] = "Serial Controller",
    [0x07][0x1] = "Parallel Controller",
    [0x07][0x2] = "Multiport Serial Controller",
    [0x07][0x3] = "Modem",
    [0x07][0x4] = "GPIB Controller",
    [0x07][0x5] = "Smart Card",
    [0x07][0x80] = "Other Communication Device",

    [0x08][0x0] = "PIC",
    [0x08][0x1] = "DMA Controller",
    [0x08][0x2] = "Timer",
    [0x08][0x3] = "RTC Controller",
    [0x08][0x4] = "PCI Hot-Plug Controller",
    [0x08][0x5] = "SD Host Controller",
    [0x08][0x6] = "IOMMU",
    [0x08][0x80] = "Other System Peripheral",

    [0x09][0x0] = "Keyboard Controller",
    [0x09][0x1] = "Digitizer Pen",
    [0x09][0x2] = "Mouse Controller",
    [0x09][0x3] = "Scanner Controller",
    [0x09][0x4] = "Gameport Controller",
    [0x09][0x80] = "Other Input Controller",

    [0x0A][0x0] = "Docking Station",
    [0x0A][0x80] = "Other Docking Station",

    [0x0B][0x0] = "386",
    [0x0B][0x1] = "486",
    [0x0B][0x2] = "Pentium",
    [0x0B][0x10] = "Alpha",
    [0x0B][0x20] = "PowerPC",
    [0x0B][0x30] = "MIPS",
    [0x0B][0x40] = "Co-Processor",
    [0x0B][0x80] = "Other Processor",

    [0x0C][0x0] = "FireWire (IEEE 1394)",
    [0x0C][0x1] = "ACCESS Bus",
    [0x0C][0x2] = "SSA",
    [0x0C][0x3] = "USB Controller",
    [0x0C][0x4] = "Fibre Channel",
    [0x0C][0x5] = "SMBus",
    [0x0C][0x6] = "InfiniBand",
    [0x0C][0x7] = "IPMI Interface",
    [0x0C][0x8] = "SERCOS Interface",
    [0x0C][0x9] = "CANbus",
    [0x0C][0x80] = "Other Serial Bus Controller",

    [0x0D][0x0] = "iRDA Compatible Controller",
    [0x0D][0x1] = "Consumer IR Controller",
    [0x0D][0x2] = "RF Controller",
    [0x0D][0x3] = "Bluetooth Controller",
    [0x0D][0x4] = "Broadband Controller",
    [0x0D][0x5] = "Ethernet Controller (802.1a)",
    [0x0D][0x6] = "Ethernet Controller (0x2.1b)",
    [0x0D][0x80] = "Other Wireless Controller",

    [0x0E][0x0] = "I2O Controller",
    [0x0E][0x80] = "Other Intelligent Controller",

    [0x0F][0x1] = "Satellite TV Controller",
    [0x0F][0x2] = "Satellite Audio Controller",
    [0x0F][0x3] = "Satellite Voice Controller",
    [0x0F][0x4] = "Satellite Data Controller",
    [0x0F][0x80] = "Other Satellite Controller",

    [0x10][0x0] = "Network and Computing Encrpytion/Descryption",
    [0x10][0x1] = "Entertainment Encryption/Descryption",
    [0x10][0x80] = "Other Encryption/Descryption",

    [0x11][0x0] = "DPIO Module",
    [0x11][0x1] = "Performance Counters",
    [0x11][0x10] = "Communication Syncronization",
    [0x11][0x20] = "Signal Processing Management",
    [0x11][0x80] = "Other Data Acquisition/Signal Processing Controller"
};

static inline PCIeDevice *_getDevPtr(u64 addrBase, u8 bus, u8 dev, u8 func) {
    return (PCIeDevice *)DMAS_phys2Virt(addrBase | ((u64)bus << 20) | ((u64)dev << 15) | ((u64)func << 12));
}

static inline u16 _getVendor(u64 addrBase, u8 bus, u8 dev, u8 func) {
    return _getDevPtr(addrBase, bus, dev, func)->vendorID;
}
static inline u8 _getHeaderType(u32 addrBase, u8 bus, u8 dev, u8 func) {
    return _getDevPtr(addrBase, bus, dev, func)->headerType;
}

void _chkBus(u64 addrBase, u8 bus);

void _chkFunc(u64 addrBase, u8 bus, u8 dev, u8 func) {
    PCIeDevice *devPtr = _getDevPtr(addrBase, bus, dev, func);
    printk(YELLOW, BLACK, "%d-%d-%d: Class:%#04x-%#04x ", bus, dev, func, devPtr->classCode, devPtr->subClass);
    printk(WHITE, BLACK, "%s\n",
            (PCIeClassDesc[devPtr->classCode][devPtr->subClass] == NULL ? "Unknown" : PCIeClassDesc[devPtr->classCode][devPtr->subClass]));
    if (devPtr->classCode == 0x06 && devPtr->subClass == 0x4) {
        _chkBus(addrBase & (~(255ul << 20)), devPtr->type.type1.subBusNum);
    }
}

void _chkDev(u64 addrBase, u8 bus, u8 dev) {
    u16 vendor = _getVendor(addrBase, bus, dev, 0);
    if (vendor == 0xffff) return ;
    u64 head = _getHeaderType(addrBase, bus, dev, 0);
    if ((head & 0x80) == 0) _chkFunc(addrBase, bus, dev, 0);
    else {
        for (int func = 0; func < 8; func++) _chkFunc(addrBase, bus, dev, func);
    }
}

void _chkBus(u64 addrBase, u8 bus) {
    for (u8 dev = 0; dev < 32; dev++) _chkDev(addrBase | ((u64)dev << 15), bus, dev);
}

void HW_PCIe_init() {
    _desc = NULL;
    printk(RED, BLACK, "HW_PCIe_init()\n");
    XSDTDescriptor *xsdt = HW_UEFI_getXSDT();
    u32 entryCnt = (xsdt->header.length - sizeof(XSDTDescriptor)) / 8;
    for (int i = 0; i < entryCnt; i++) {
        ACPIHeader *header = (ACPIHeader *)DMAS_phys2Virt(xsdt->entry[i]);
        if (!strncmp(header->signature, "MCFG", 4)) {
            _desc = (PCIeDescriptor *)header;
            break;
        }
    }
    if (_desc == NULL) { 
        printk(RED, BLACK, "PCIe no found...");
        while (1) IO_hlt();
    }
    printk(WHITE, BLACK, "find PCIeDescriptor at %#018lx\n", _desc);
    devCnt = (_desc->header.length - sizeof(PCIeDescriptor)) / sizeof(PCIeStruct);
    for (int i = 0; i < devCnt; i++) {
        printk(WHITE, BLACK,
                "PCIe device %d: address: %#018lx, segment: %#04x, start bus: %#02x, end bus: %#02x\n", 
                i, _desc->structs[i].address, _desc->structs[i].segment, _desc->structs[i].stBus, _desc->structs[i].edBus);
        u8 head = _getHeaderType(_desc->structs[i].address, 0, 0, 0);
        if ((head & 0x80) == 0) _chkBus(_desc->structs[i].address, 0);
        else {
            for (int func = 0; func < 8; func++) {
                int vendor = _getVendor(_desc->structs[i].address, 0, 0, func);
                if (vendor == 0xffff) break;
                _chkBus(_desc->structs[i].address | ((u64)func << 20), func);
            }
        }
    }
}