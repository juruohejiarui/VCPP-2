#include "api.h"
#include "../../../includes/smp.h"
#include "../../../includes/interrupt.h"
#include "../../../includes/log.h"
#include "../../../includes/interrupt.h"

List HW_USB_XHCI_hostList;

IntrHandlerDeclare(XHCI_intrHandler) {

}

void HW_USB_XHCI_init(PCIeManager *pci) {
	// check the capability list
	if (!(pci->cfg->status & (1 << 4))) {
		printk(RED, BLACK, "XHCI: PCI %#018lx has no capability list\n", pci);
		return ;
	}
	XHCI_Host *host = kmalloc(sizeof(XHCI_Host), Slab_kmalloc_arg_Clear, NULL);
	List_init(&host->listEle);
	printk(WHITE, BLACK, "capPtr:%x\n", pci->cfg->type.type0.capPtr);
	for (PCIe_CapabilityHeader *hdr = HW_PCIe_getNxtCapHdr(pci->cfg, NULL); hdr; hdr = HW_PCIe_getNxtCapHdr(pci->cfg, hdr)) {
		printk(WHITE, BLACK, "\tCapId:%x nxtPtr:%x\n", hdr->capId, hdr->nxtPtr);
		if (hdr->capId == PCIe_CapId_MSI) {
			host->msiCapDesc = container(hdr, PCIe_MSICapability, hdr);
			break;
		}
	}
	printk(WHITE, BLACK, "vendor:%04x device:%04x revision:%04x\n", pci->cfg->vendorID, pci->cfg->devID, pci->cfg->revID);
	if (pci->cfg->vendorID == 0x8086 && pci->cfg->devID == 0x1e31 && pci->cfg->revID == 4) {
		*(u32 *)((u64)pci->cfg + 0xd8) = 0xffffffff;
		*(u32 *)((u64)pci->cfg + 0xd0) = 0xffffffff;
	}
	host->capRegAddr = (u64)DMAS_phys2Virt((pci->cfg->type.type0.bar[0] | (((u64)pci->cfg->type.type0.bar[1]) << 32)) & ~0xffful);
	host->opRegAddr = host->capRegAddr + HW_USB_XHCI_CapReg_capLen(host);
	host->rtRegAddr = host->capRegAddr + HW_USB_XHCI_CapReg_rtsOff(host);
	host->dbRegAddr = host->capRegAddr + HW_USB_XHCI_CapReg_dbOffset(host);
	printk(WHITE, BLACK, "XHCI: %#018lx: maxSlot:%d maxIntr:%d maxPort:%d maxScrSz:%d\n", 
		host, HW_USB_XHCI_maxSlot(host), HW_USB_XHCI_maxIntr(host), HW_USB_XHCI_maxPort(host), HW_USB_XHCI_maxScrSz(host));
	if (!(HW_USB_XHCI_readOpReg(host, XHCI_OpReg_pgSize) & 0x1)) {
		printk(RED, BLACK, "XHCI: %#018lx: no support for 4K page\n", host);
		kfree(host, 0);
		return ;
	}
	// reset the host
	if (!HW_USB_XHCI_reset(host)) {
		printk(RED, BLACK, "XHCI: %#018lx: failed to reset.\n", host);
		kfree(host, 0);
		return ;
	}
	HW_USB_XHCI_waiForHostIsReady(host);

	// set max slot field of config register
	HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_cfg, HW_USB_XHCI_readOpReg(host, XHCI_OpReg_cfg) | HW_USB_XHCI_maxSlot(host));

	// allocate the device context base address array
	u64 *dcbaa = kmalloc(0x1000, Slab_kmalloc_arg_Clear, NULL);
	HW_USB_XHCI_writeDCBAAP(host, DMAS_virt2Phys(dcbaa));

	// allocate the scratchpad array and items
	{
		int maxScrSz = HW_USB_XHCI_maxScrSz(host);
		u64 *scrArray = kmalloc(sizeof(u64) * maxScrSz, Slab_kmalloc_arg_Clear, NULL);
		for (int i = 0; i < maxScrSz; i++)
			scrArray[i] = DMAS_virt2Phys(kmalloc(0x1000, 0, NULL));
		dcbaa[0] = DMAS_virt2Phys(scrArray);
	}
	// allocate device context and set dcbaa items
	host->devCtx = kmalloc(sizeof(XHCI_DevCtx *) * (HW_USB_XHCI_maxSlot(host) + 1), Slab_kmalloc_arg_Clear, NULL);
	for (int i = HW_USB_XHCI_maxSlot(host); i > 0; i--) {
		host->devCtx[i] = kmalloc(0x1000, Slab_kmalloc_arg_Clear, NULL);
		dcbaa[i] = DMAS_virt2Phys(host->devCtx[i]);
	}
	
	// release this host from BIOS
	for (void *ecp = HW_USB_XHCI_getNxtECP(host, NULL); ecp; ecp = HW_USB_XHCI_getNxtECP(host, ecp)) {
		if (HW_USB_XHCI_ECP_id(ecp) == XHCI_Ext_Id_Legacy) {
			HW_USB_XHCI_writeDword((u64)ecp, HW_USB_XHCI_readDword((u64)ecp) | (1u << 24));
			int timeout = 10;
			while (timeout--) {
				u32 cur = HW_USB_XHCI_readDword((u64)ecp) & ((1u << 24) | (1u << 16));
				if (cur == (1u << 24)) break;
				Intr_SoftIrq_Timer_mdelay(1);
			}
			break;
		}
	}

	// set up the port pairs from the protocol description
	u32 usb3C = 0, usb2C = 0;
	host->port = kmalloc(sizeof(XHCI_PortInfo) * (HW_USB_XHCI_maxPort(host)), Slab_kmalloc_arg_Clear, NULL);
	for (void *ecp = HW_USB_XHCI_getNxtECP(host, NULL); ecp; ecp = HW_USB_XHCI_getNxtECP(host, ecp)) {
		if (HW_USB_XHCI_ECP_id(ecp) == XHCI_Ext_Id_Protocol) {
			int isUSB3 = (HW_USB_XHCI_readByte((u64)ecp + 3) == 3);
			u8 offset, count;
			{
				u32 desc = HW_USB_XHCI_readDword((u64)ecp + 0x08);
				offset = desc & 0xff, count = (desc >> 8) & 0xff;
			}
			u32 *protoC = (isUSB3 ? &usb3C : &usb2C);
			for (int i = offset; i < offset + count; i++) {
				host->port[i].offset = ++*protoC;
				host->port[i].flags |= XHCI_PortInfo_Flag_Active;
				if (isUSB3)
					host->port[i].flags |= XHCI_PortInfo_Flag_isUSB3;
			}
		}
	}
	for (int i = 1; i <= HW_USB_XHCI_maxPort(host); i++) {
		for (int j = i + 1; j <= HW_USB_XHCI_maxPort(host); j++) {
			if (host->port[i].offset == host->port[j].offset && host->port[i].offset) {
				host->port[i].flags |= XHCI_PortInfo_Flag_Paired;
				host->port[j].flags |= XHCI_PortInfo_Flag_Paired;
				if ((host->port[i].flags & XHCI_PortInfo_Flag_Protocol) == XHCI_PortInfo_Flag_isUSB3)
					host->port[j].flags &= ~XHCI_PortInfo_Flag_Active;
				else host->port[i].flags &= ~XHCI_PortInfo_Flag_Active;
				host->port[i].pairOffset = j;
				host->port[j].pairOffset = i;
			}
		}
	}

	// allocate a command ring
	host->cmdRing = HW_USB_XHCI_allocRing(XHCI_Ring_maxSize);
	// construct a link trb at the end and points to the head
	{
		XHCI_GenerTRB *trb = &host->cmdRing->ring[XHCI_Ring_maxSize - 1];
		HW_USB_XHCI_TRB_setType(trb, XHCI_TRB_Type_Link);
		HW_USB_XHCI_TRB_setToggle(trb, 1);
		HW_USB_XHCI_TRB_setData(trb, DMAS_virt2Phys(host->cmdRing));
	}
	// set the CRCR register
	HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_crCtrl, DMAS_virt2Phys(host->cmdRing->ring) | 1);
	printk(WHITE, BLACK, "XHCI: %#018lx: command ring control: %#018lx\n", host, DMAS_virt2Phys(host->cmdRing->ring) | 1);
	HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_dnCtrl, (1 << 1));

	// allocate a event ring array
	host->eveRing = HW_USB_XHCI_allocEveRing(4, XHCI_Ring_maxSize);
	// make the event ring table, be
	u64 *eveRingTbl = kmalloc(sizeof(u64 *) * 8, Slab_kmalloc_arg_Clear, NULL);
	for (int i = 0; i < 4; i++)
		eveRingTbl[(i << 1) + 0] = DMAS_virt2Phys(host->eveRing->rings[i]),
		eveRingTbl[(i << 1) + 1] = XHCI_Ring_maxSize;
	HW_USB_XHCI_writeIntrDword(host, 0, XHCI_IntrReg_IMan, (1 << 1) | (1 << 0));
	HW_USB_XHCI_writeIntrDword(host, 0, XHCI_IntrReg_IMod, 0);
	HW_USB_XHCI_writeIntrDword(host, 0, XHCI_IntrReg_TblSize, 4);
	HW_USB_XHCI_writeIntrDword(host, 0, XHCI_IntrReg_DeqPtr, 
		DMAS_virt2Phys(&host->eveRing->rings[host->eveRing->curRingId][host->eveRing->curPos]) | (1ul << 3));
	HW_USB_XHCI_writeIntrDword(host, 0, XHCI_IntrReg_TblAddr, DMAS_virt2Phys(eveRingTbl));

	// configure the ports
	for (int i = HW_USB_XHCI_maxPort(host); i > 0; i--)
		HW_USB_XHCI_writePortReg(host, i, XHCI_PortReg_sc, (1 << 9) | (1 << 25) | (1 << 26) | (1 << 27));
	// register MSI register
	int cpuId; u8 vecSt;
	int vecNum = (1 << ((host->msiCapDesc->msgCtrl >> 1) & 0x7));
	SMP_allocIntrVec(vecNum, &cpuId, &vecSt);
	if (cpuId == -1) {
		printk(RED, BLACK, "XHCI: %#0118lx: fail to allocate interrupt for MSI\n");
		kfree(host, 0);
	}
	printk(WHITE, BLACK, "XHCI: %#018lx: msiCtrl:%#06x -> get interrupt vector: %#04x~%#04x on processor %d\n", host, host->msiCapDesc->msgCtrl, vecSt, vecSt + vecNum - 1, cpuId);
	HW_PCIe_MSI_setMsgAddr(host->msiCapDesc, SMP_getCPUInfoPkg(cpuId)->cpuId, 0, HW_APIC_DestMode_Physical);
	HW_PCIe_MSI_setMsgData(host->msiCapDesc, vecSt, HW_APIC_DeliveryMode_Fixed, HW_APIC_Level_Deassert, HW_APIC_TriggerMode_Edge);
	// allocate the MSI interrupt descriptor
	host->msiDesc = kmalloc(sizeof(PCIe_MSI_Descriptor) * vecNum, Slab_kmalloc_arg_Clear, NULL);
	for (int i = 0; i < vecNum; i++) {
		// the parameter is the address of the host and the id of interrupt
		// We can easily use the OR operation to combine the two parameters, because the address of the host must be 64-aligned.
		HW_PCIe_MSI_initDesc(&host->msiDesc[i], cpuId, vecSt + i, HW_USB_XHCI_msiHandler, (u64)host | i);
		HW_PCIe_MSI_setIntr(&host->msiDesc[i]);
	}
	// enable the interrupt
	host->msiCapDesc->msgCtrl |= (1 << 0);
	// restart the host
	HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_cmd, (1 << 0));
	printk(WHITE, BLACK, "XHCI: %#018lx: finish initialization.\n", host);
}