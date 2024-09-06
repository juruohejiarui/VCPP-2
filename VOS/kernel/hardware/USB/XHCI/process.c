#include "api.h"
#include "../../../includes/smp.h"
#include "../../../includes/interrupt.h"
#include "../../../includes/log.h"
#include "../../../includes/interrupt.h"

List HW_USB_XHCI_hostList, HW_USB_XHCI_DriverList;
SpinLock HW_USB_XHCI_DriverListLock;

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
	printk(WHITE, BLACK, "\tcapReg:%#018lx opReg:%#018lx rtReg:%#018lx dbReg:%#018lx\n", 
		host->capRegAddr, host->opRegAddr, host->rtRegAddr, host->dbRegAddr);
	if (!(HW_USB_XHCI_readOpReg(host, XHCI_OpReg_pgSize) & 0x1)) {
		printk(RED, BLACK, "XHCI: %#018lx: no support for 4K page\n", host);
		kfree(host, 0);
		return ;
	}
	// stop the host
	HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_cmd, HW_USB_XHCI_readOpReg(host, XHCI_OpReg_cmd) & ~1u);
	int timeout = 30;
	do {
		if (HW_USB_XHCI_readOpReg(host, XHCI_OpReg_status) & (1 << 0)) break;
		Intr_SoftIrq_Timer_mdelay(1);
		timeout--;
	} while (timeout > 0);
	if (!(HW_USB_XHCI_readOpReg(host, XHCI_OpReg_status) & (1 << 0))) {
		printk(RED, BLACK, "XHCI: %#018lx: failed to stop.\n", host);
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
	HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_cfg, 
		(HW_USB_XHCI_readOpReg(host, XHCI_OpReg_cfg) & ((1ul << 10) - 1)) | (1ul << 8) | HW_USB_XHCI_maxSlot(host));

	// allocate the device context base address array
	u64 *dcbaa = kmalloc(2048, Slab_kmalloc_arg_Clear, NULL);
	HW_USB_XHCI_writeDCBAAP(host, DMAS_virt2Phys(dcbaa));

	// allocate the scratchpad array and items
	{
		int maxScrSz = max(64, HW_USB_XHCI_maxScrSz(host));
		u64 *scrArray = kmalloc(0x1000, Slab_kmalloc_arg_Clear, NULL);
		for (int i = 0; i < maxScrSz; i++)
			scrArray[i] = DMAS_virt2Phys(kmalloc(0x1000, 0, NULL));
		dcbaa[0] = DMAS_virt2Phys(scrArray);
	}
	// allocate device context and set dcbaa items
	host->devCtx = kmalloc(sizeof(XHCI_DevCtx *) * (HW_USB_XHCI_maxSlot(host) + 1), Slab_kmalloc_arg_Clear, NULL);
	host->devCtx[0] = (XHCI_DevCtx *)DMAS_phys2Virt(dcbaa[0]);
	for (int i = HW_USB_XHCI_maxSlot(host); i > 0; i--) {
		host->devCtx[i] = kmalloc(0x1000, Slab_kmalloc_arg_Clear, NULL);
		dcbaa[i] = DMAS_virt2Phys(host->devCtx[i]);
	}
	host->dev = kmalloc(sizeof(XHCI_Device *) * (HW_USB_XHCI_maxSlot(host) + 1), Slab_kmalloc_arg_Clear, NULL);
	
	// release this host from BIOS
	for (void *ecp = HW_USB_XHCI_getNxtECP(host, NULL); ecp; ecp = HW_USB_XHCI_getNxtECP(host, ecp)) {
		if (HW_USB_XHCI_ECP_id(ecp) == XHCI_Ext_Id_Legacy) {
			printk(WHITE, BLACK, "XHCI: %#018lx: legacy support previous value: %#010x\n", HW_USB_XHCI_readDword((u64)ecp));
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
	// configure the ports
	for (int i = HW_USB_XHCI_maxPort(host); i > 0; i--)
		HW_USB_XHCI_writePortReg(host, i, XHCI_PortReg_sc, XHCI_PortReg_sc_Power | XHCI_PortReg_sc_AllEve);

	// allocate a command ring
	host->cmdRing = HW_USB_XHCI_allocRing(XHCI_Ring_maxSize);
	// construct a link trb at the end and points to the head
	{
		XHCI_GenerTRB *trb = &host->cmdRing->ring[XHCI_Ring_maxSize - 1];
		HW_USB_XHCI_TRB_setType(trb, XHCI_TRB_Type_Link);
		HW_USB_XHCI_TRB_setToggle(trb, 1);
		HW_USB_XHCI_TRB_setData(trb, DMAS_virt2Phys(&host->cmdRing->ring[0]));
	}
	// set the device notification register
	HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_dnCtrl, (1 << 1) | (HW_USB_XHCI_readOpReg(host, XHCI_OpReg_dnCtrl) & ~0xffffu));

	// allocate a event ring array
	host->eveRing = HW_USB_XHCI_allocEveRing(4, XHCI_Ring_maxSize);
	// make the event ring table
	u64 *eveRingTbl = kmalloc(max(64, sizeof(u64 *) * 8), Slab_kmalloc_arg_Clear, NULL);
	for (int i = 0; i < 4; i++)
		eveRingTbl[(i << 1) + 0] = DMAS_virt2Phys(host->eveRing->rings[i]),
		eveRingTbl[(i << 1) + 1] = XHCI_Ring_maxSize;
	HW_USB_XHCI_writeIntrDword(host, 0, XHCI_IntrReg_IMan, (1 << 1) | (1 << 0) | (HW_USB_XHCI_readIntrDword(host, 0, XHCI_IntrReg_IMan) | ~0x3u));
	HW_USB_XHCI_writeIntrDword(host, 0, XHCI_IntrReg_IMod, 0);
	HW_USB_XHCI_writeIntrDword(host, 0, XHCI_IntrReg_TblSize, 4 | (HW_USB_XHCI_readIntrDword(host, 0, XHCI_IntrReg_TblSize) & ~0xffffu));
	HW_USB_XHCI_writeIntrQuad(host, 0, XHCI_IntrReg_DeqPtr, 
		DMAS_virt2Phys(&host->eveRing->rings[host->eveRing->curRingId][host->eveRing->curPos]) | (1ul << 3));
	HW_USB_XHCI_writeIntrQuad(host, 0, XHCI_IntrReg_TblAddr, DMAS_virt2Phys(eveRingTbl) | (HW_USB_XHCI_readIntrQuad(host, 0, XHCI_IntrReg_TblAddr) & 0x3f));

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
	HW_PCIe_MSI_setMsgData(host->msiCapDesc, vecSt, HW_APIC_DeliveryMode_Fixed, HW_APIC_Level_Assert, HW_APIC_TriggerMode_Edge);
	// allocate the MSI interrupt descriptor
	host->msiDesc = kmalloc(sizeof(PCIe_MSI_Descriptor) * vecNum, Slab_kmalloc_arg_Clear, NULL);
	for (int i = 0; i < vecNum; i++) {
		// the parameter is the address of the host and the id of interrupt
		// We can easily use the OR operation to combine the two parameters, because the address of the host must be 64-aligned.
		HW_PCIe_MSI_initDesc(&host->msiDesc[i], cpuId, vecSt + i, HW_USB_XHCI_msiHandler, (u64)host | i);
		HW_PCIe_MSI_setIntr(&host->msiDesc[i]);
	}
	// initialize the evering handle task

	host->eveHandlerTask = kmalloc(sizeof(TaskStruct *) * XHCI_EveHandleTaskNum, Slab_kmalloc_arg_Clear, NULL);
	host->eveList = kmalloc(sizeof(List) * HW_USB_XHCI_maxIntr(host), Slab_kmalloc_arg_Clear, NULL);
	host->eveLock = kmalloc(sizeof(SpinLock) * HW_USB_XHCI_maxIntr(host), Slab_kmalloc_arg_Clear, NULL);
	for (int i = HW_USB_XHCI_maxIntr(host) - 1; i >= 0; i--) {
		List_init(&host->eveList[i]);
		SpinLock_init(&host->eveLock[i]);
	}
	int evePreTask = HW_USB_XHCI_maxIntr(host) / XHCI_EveHandleTaskNum;
	for (int i = 0; i < XHCI_EveHandleTaskNum; i++)
		host->eveHandlerTask[i] = Task_createTask((Task_Entry)HW_USB_XHCI_evehandleTask, 
			host, ((1ul << evePreTask) - 1) << (evePreTask * i), Task_Flag_Kernel | Task_Flag_Inner);

	// disable the INTx
	pci->cfg->command |= (1 << 10);
	// disable mask
	if (host->msiCapDesc->msgCtrl & (1 << 8)) host->msiCapDesc->mask = 0;
	// enable the interrupt
	host->msiCapDesc->msgCtrl |= (1 << 0);
	// set crcr registers
	HW_USB_XHCI_writeOpRegQuad(host, XHCI_OpReg_crCtrl, DMAS_virt2Phys(host->cmdRing->ring) | 1);
	// restart the host
	HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_cmd, (1 << 0) | (1 << 2) | (1 << 3));

	printk(WHITE, BLACK, "XHCI: %#018lx: command ring control: %#018lx\n", host, DMAS_virt2Phys(host->cmdRing->ring) | 1);

	printk(WHITE, BLACK, "XHCI: %#018lx: finish initialization. host cmd:%#010x\n", host, HW_USB_XHCI_readOpReg(host, XHCI_OpReg_cmd));
}

IntrHandlerDeclare(HW_USB_XHCI_msiHandler) {
	XHCI_Host *host = (XHCI_Host *)(arg & ~0x40);
	int intrId = arg & 0x40;
	HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_status, (1 << 3));
	for (int i = HW_USB_XHCI_maxIntr(host) - 1; i >= 0; i--) {
		// check if this interrupt is enabled
		if (!(HW_USB_XHCI_readIntrDword(host, i, XHCI_IntrReg_IMan) & 2)) continue;
		// check if the event handler busy bit of DepPtr is set
		if (!(HW_USB_XHCI_readIntrQuad(host, i, XHCI_IntrReg_DeqPtr) & (1 << 3))) continue;
		// clear the busy bit
		XHCI_GenerTRB *trb;
		while (HW_USB_XHCI_EveRing_getNxt(host->eveRing, &trb)) {
			XHCI_Event *eve = kmalloc(sizeof(XHCI_Event), Slab_kmalloc_arg_Clear, NULL);
			HW_USB_XHCI_TRB_copy(trb, &eve->trb);
			List_init(&eve->list);
			SpinLock_lock(&host->eveLock[i]);
			List_insBefore(&eve->list, &host->eveList[i]);
			SpinLock_unlock(&host->eveLock[i]);
		}
		
		// write the dequeue pointer
		HW_USB_XHCI_writeIntrQuad(host, i, XHCI_IntrReg_DeqPtr, DMAS_virt2Phys(trb) | (1 << 3));
		// printk(WHITE, BLACK, "\tintr %d depPtr:%#018lx\n", i, HW_USB_XHCI_readIntrQuad(host, i, XHCI_IntrReg_DeqPtr));
	}
}

void HW_USB_XHCI_portConnect(XHCI_Host *host, int portId) {
	// create the device management structure and task
	XHCI_Device *dev = kmalloc(sizeof(XHCI_Device), Slab_kmalloc_arg_Clear, NULL);
	dev->host = host;
	Task_createTask((Task_Entry)HW_USB_XHCI_devMgrTask, dev, portId, Task_Flag_Kernel | Task_Flag_Inner);
	host->port[portId].dev = dev;
}
void HW_USB_XHCI_portDisconnect(XHCI_Host *host, int portId) {
	// no management structure for this device
	XHCI_Device *dev = host->port[portId].dev;
	if (!dev) return ;
	host->port[portId].dev = NULL;
	if (dev->mgrTask)
		Task_setSignal(dev->mgrTask, Task_Signal_Int);
}

void HW_USB_XHCI_evehandleTask(XHCI_Host *host, u64 intrMap) {
	Task_kernelEntryHeader();
	while (1) {
		List penList;
		for (int i = 0; i < HW_USB_XHCI_maxIntr(host); i++) {
			if (!(intrMap & (1ul << i))) continue;
			List_init(&penList);
			if (host->msiDesc[0].cpuId == Task_current->cpuId) IO_cli();
			SpinLock_lock(&host->eveLock[i]);
			for (List *eveList = host->eveList[i].next; eveList != &host->eveList[i]; eveList = host->eveList[i].next) {
				List_del(eveList);
				List_insBefore(eveList, &penList);
			}
			SpinLock_unlock(&host->eveLock[i]);
			if (host->msiDesc[0].cpuId == Task_current->cpuId) IO_sti();
			for (List *eveList = penList.next; eveList != &penList; eveList = penList.next) {
				XHCI_Event *eve = container(eveList, XHCI_Event, list);
				switch (HW_USB_XHCI_TRB_getType(&eve->trb)) {
					case XHCI_TRB_Type_PortStChg : {
						int portId = eve->trb.data1 >> 24;
						// printk(WHITE, BLACK, "\tport %d status:%#010x\n", portId, HW_USB_XHCI_readPortReg(host, portId, XHCI_PortReg_sc));
						HW_USB_XHCI_writePortReg(host, portId, XHCI_PortReg_sc, XHCI_PortReg_sc_Power | XHCI_PortReg_sc_AllChg | XHCI_PortReg_sc_AllEve);
						HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_status, (1 << 4));
						u32 sc = HW_USB_XHCI_readPortReg(host, portId, XHCI_PortReg_sc);
						if (sc & 1) {
							// this port has been enabled
							if (sc & (1u << 1)) HW_USB_XHCI_portConnect(host, portId);
							else // this port is not enabled, then reset this port for attempting to enable it.
								HW_USB_XHCI_writePortReg(host, portId, XHCI_PortReg_sc, (1u << 4) | XHCI_PortReg_sc_Power |  XHCI_PortReg_sc_AllChg | XHCI_PortReg_sc_AllEve);
						} else HW_USB_XHCI_portDisconnect(host, portId);
						break;
					}
					case XHCI_TRB_Type_CmdCmpl : {
						XHCI_GenerTRB *cmd = DMAS_phys2Virt(*(u64 *)&eve->trb.data1);
						int pos = HW_USB_XHCI_TRB_getPos(cmd);
						// printk(YELLOW, BLACK, "\t\t\t\tEvent: data:%#018lx status:%#010x ctrl:%#010x pos:%d\r", *(u64 *)&eve->trb.data1, eve->trb.status, eve->trb.ctrl, pos);
						// clear the reqSrc
						SpinLock_lock(&host->cmdRing->lock);
						XHCI_Request *req = host->cmdRing->reqSrc[pos];
						for (int i = 0; i < req->trbCnt; i++) *req->target[i] = NULL;
						SpinLock_unlock(&host->cmdRing->lock);

						HW_USB_XHCI_TRB_copy(&eve->trb, &req->res);
						req->flags |= XHCI_Request_Flag_Finished;
						
						break;
					}
					case XHCI_TRB_Type_TransEve : {
						XHCI_GenerTRB *tr = DMAS_phys2Virt(*(u64 *)&eve->trb.data1);
						int pos = HW_USB_XHCI_TRB_getPos(tr), slot = HW_USB_XHCI_TRB_getSlot(&eve->trb), 
							ep = (HW_USB_XHCI_readDword((u64)&eve->trb.ctrl) >> 16) & 0x1f;
						// printk(YELLOW, BLACK, "\t\t\t\tEvent: data:%#018lx status:%#010x ctrl:%#010x pos:%d\r", *(u64 *)&eve->trb.data1, eve->trb.status, eve->trb.ctrl, pos);
						XHCI_Device *dev = host->dev[slot];
						XHCI_Ring *trRing = dev->trRing[ep - 1];
						SpinLock_lock(&trRing->lock);
						XHCI_Request *req = trRing->reqSrc[pos];
						for (int i = 0; i < req->trbCnt; i++) *req->target[i] = NULL;
						SpinLock_unlock(&trRing->lock);
						HW_USB_XHCI_TRB_copy(&eve->trb, &req->res);
						req->flags |= XHCI_Request_Flag_Finished;
						
						break;
					}
				}
				List_del(eveList);
				kfree(eve, 0);
			}
		}
		IO_hlt();
	}
}

void HW_USB_XHCI_devMgrTask_int(u64 signal, XHCI_Device *dev) {
	if (dev->slotId) {
		if (dev->driver) {
			if (dev->driver->disable) dev->driver->disable(dev);
			if (dev->driver->uninstall) dev->driver->uninstall(dev);
		}
		XHCI_Request *req = HW_USB_XHCI_allocReq(1);
		HW_USB_XHCI_TRB_setType(&req->trb[0], XHCI_TRB_Type_DisblSlot);
		HW_USB_XHCI_TRB_setSlot(&req->trb[0], dev->slotId);
		req->flags |= XHCI_Request_Flag_IsCommand;
		HW_USB_XHCI_Ring_insReq(dev->host->cmdRing, req);
		HW_USB_XHCI_writeDbReg(dev->host, 0, 0, 0);
		if (HW_USB_XHCI_Req_wait(req) != XHCI_TRB_CmplCode_Succ) {
			printk(RED, BLACK, "Unable to disable the slot %d of device:%#018lx\n", dev->slotId, dev);
		}
		dev->host->dev[dev->slotId] = NULL;
	}
	kfree(dev, 0);
	Task_kernelThreadExit(-1);
}

void HW_USB_XHCI_devMgrTask(XHCI_Device *dev, u64 rootPort) {
	Task_kernelEntryHeader();
	Task_setSignalHandler(Task_current, Task_Signal_Int, (Task_SignalHandler)HW_USB_XHCI_devMgrTask_int, (u64)dev);
	dev->mgrTask = Task_currentDMAS();
	int speed = (HW_USB_XHCI_readPortReg(dev->host, rootPort, XHCI_PortReg_sc) >> 10) & ((1 << 4) - 1);
	printk(YELLOW, BLACK, "dev:%#018lx port:%d speed:%d\n", dev, rootPort, speed);
	// enable a slot for this device
	XHCI_Request *req0;
	{
		req0 = HW_USB_XHCI_allocReq(1);
		HW_USB_XHCI_TRB_setType(&req0->trb[0], XHCI_TRB_Type_EnblSlot);
		HW_USB_XHCI_Ring_insReq(dev->host->cmdRing, req0);
		if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, 0, 0, 0, req0) != XHCI_TRB_CmplCode_Succ) {
			printk(RED, BLACK, "dev:%#018lx failed to allocate slot, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req0->res));
			dev->mgrTask = NULL;
			Task_setSignal(Task_current, Task_Signal_Int);
			while (1) IO_hlt();
		}
		dev->slotId = HW_USB_XHCI_TRB_getSlot(&req0->res);
		dev->host->dev[dev->slotId] = dev;
		dev->ctx = dev->host->devCtx[dev->slotId];
		printk(GREEN, BLACK, "dev %#018lx on slot %d\n", dev, dev->slotId);
	}	 
	// create input context structure
	{
		req0->flags = 0;
		HW_USB_XHCI_TRB_setType(&req0->trb[0], XHCI_TRB_Type_AddrDev);
		HW_USB_XHCI_TRB_setSlot(&req0->trb[0], dev->slotId);
		
		dev->inCtx = kmalloc(sizeof(XHCI_InputCtx), Slab_kmalloc_arg_Clear | Slab_kmalloc_arg_Private, NULL);
		dev->inCtx->ctrl.addFlags |= (1 << 1) | (1 << 0);
		{
			XHCI_SlotCtx *slot = &dev->inCtx->slot;
			HW_USB_XHCI_writeCtx(slot, 0, XHCI_SlotCtx_ctxEntries, 1);
			HW_USB_XHCI_writeCtx(slot, 0, XHCI_SlotCtx_speed, speed);
			HW_USB_XHCI_writeCtx(slot, 1, XHCI_SlotCtx_rootPortNum, rootPort);
		}
		{
			XHCI_EpCtx *ep0 = &dev->inCtx->ep[0];
			HW_USB_XHCI_writeCtx(ep0, 1, XHCI_EpCtx_epType, XHCI_EpCtx_epType_Control);
			HW_USB_XHCI_writeCtx(ep0, 1, XHCI_EpCtx_CErr, 3);
			HW_USB_XHCI_writeCtx(ep0, 1, XHCI_EpCtx_mxPackSize, HW_USB_XHCI_EpCtx_getMxPackSize0(speed));


			dev->trRing[0] = HW_USB_XHCI_allocRing(XHCI_Ring_maxSize);
			XHCI_GenerTRB *lk = &dev->trRing[0]->ring[XHCI_Ring_maxSize - 1];
			HW_USB_XHCI_TRB_setType(lk, XHCI_TRB_Type_Link);
			HW_USB_XHCI_TRB_setToggle(lk, 1);
			HW_USB_XHCI_TRB_setData(lk, DMAS_virt2Phys(&dev->trRing[0]->ring[0]));

			HW_USB_XHCI_writeQuad((u64)&ep0->deqPtr, DMAS_virt2Phys(dev->trRing[0]->cur));
			HW_USB_XHCI_writeCtx(ep0, 2, XHCI_EpCtx_dcs, 1);
			HW_USB_XHCI_writeCtx(ep0, 4, XHCI_EpCtx_aveTrbLen, 8);
		}
		HW_USB_XHCI_TRB_setData(&req0->trb[0], DMAS_virt2Phys(dev->inCtx));
		HW_USB_XHCI_Ring_insReq(dev->host->cmdRing, req0);
		if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, 0, 0, 0, req0) != XHCI_TRB_CmplCode_Succ) {
			printk(RED, BLACK, "dev:%#018lx failed to address device, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req0->res));
			dev->mgrTask = NULL;
			Task_setSignal(Task_current, Task_Signal_Int);
			while (1) IO_hlt();
		}
		printk(GREEN, BLACK, "dev %#018lx address deivce successfully\n", dev);
	}
	XHCI_Request *req1 = HW_USB_XHCI_allocReq(3);
	// get the first 8 bytes of the device descriptor and modify the maxPacketSize0
	{
		XHCI_GenerTRB *setup = &req1->trb[0];
		HW_USB_XHCI_TRB_setData(setup, 		HW_USB_XHCI_TRB_mkSetup(0x80, 0x6, 0x0100, 0x0, 0x8));
		HW_USB_XHCI_TRB_setStatus(setup, 	HW_USB_XHCI_TRB_mkStatus(8, 0, 0));
		HW_USB_XHCI_TRB_setType(setup, 		XHCI_TRB_Type_SetupStage);
		HW_USB_XHCI_TRB_setCtrlBit(setup, 	XHCI_TRB_Ctrl_idt);
		HW_USB_XHCI_TRB_setTRT(setup, 		XHCI_TRB_TRT_In);
	}
	{
		XHCI_GenerTRB *data = &req1->trb[1];
		dev->devDesc = kmalloc(0xff, Slab_kmalloc_arg_Private, NULL);
		memset(dev->devDesc, 0, 0xff);
		HW_USB_XHCI_TRB_setData(data,	DMAS_virt2Phys(dev->devDesc));
		HW_USB_XHCI_TRB_setStatus(data, HW_USB_XHCI_TRB_mkStatus(8, 0, 0));
		HW_USB_XHCI_TRB_setType(data, 	XHCI_TRB_Type_DataStage);
		HW_USB_XHCI_TRB_setDir(data, 	XHCI_TRB_Ctrl_Dir_In);
	}
	{
		XHCI_GenerTRB *status = &req1->trb[2];
		HW_USB_XHCI_TRB_setType(status, 	XHCI_TRB_Type_StatusStage);
		HW_USB_XHCI_TRB_setCtrlBit(status,	XHCI_TRB_Ctrl_ioc);
	}
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
	if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to get device descriptor, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
		dev->mgrTask = NULL;
		Task_setSignal(Task_current, Task_Signal_Int);
		while (1) IO_hlt();
	}
	printk(GREEN, BLACK, "dev %#018lx device descriptor %016lx\n", dev, *(u64 *)dev->devDesc);
	u32 val = (speed >= 4 ? (1u << (dev->devDesc->bMaxPackSz0)) : dev->devDesc->bMaxPackSz0);
	// the max packet size for control endpoint is not correct
	if (val != HW_USB_XHCI_EpCtx_getMxPackSize0(speed)) {
		// modify the control endpoint context and update evaluate context command to update
		HW_USB_XHCI_writeCtx(&dev->inCtx->ep[0], 1, XHCI_EpCtx_mxPackSize, val);
		dev->inCtx->ctrl.addFlags = (1 << 1);
		
		HW_USB_XHCI_TRB_setType(&req0->trb[0], XHCI_TRB_Type_EvalCtx);
		HW_USB_XHCI_Ring_insReq(dev->host->cmdRing, req0);
		if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, 0, 0, 0, req0) != XHCI_TRB_CmplCode_Succ) {
			printk(RED, BLACK, "dev %#018lx: failed to modify max packet size 0, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req0->res));
			dev->mgrTask = NULL;
			Task_setSignal(Task_current, Task_Signal_Int);
			while (1) IO_hlt();
		}
		printk(GREEN, BLACK, "dev %#018lx modify max packet size 0 to %d\n", dev, val);
	}
	
	// get the full device descriptor
	HW_USB_XHCI_TRB_setData(&req1->trb[0], 	HW_USB_XHCI_TRB_mkSetup(0x80, 0x6, 0x0100, 0x0, 0xff));
	HW_USB_XHCI_TRB_setStatus(&req1->trb[1], HW_USB_XHCI_TRB_mkStatus(0xff, 0, 0));
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
	if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to get device descriptor, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
		dev->mgrTask = NULL;
		Task_setSignal(Task_current, Task_Signal_Int);
		while (1) IO_hlt();
	}
	printk(GREEN, BLACK, "dev %#018lx device descriptor %016lx %016lx %04lx\n", dev, 
		*(u64 *)dev->devDesc, *((u64 *)dev->devDesc + 1), *((u64 *)dev->devDesc + 2));
	
	dev->cfgDesc = kmalloc(sizeof(void *) * dev->devDesc->bNumCfg, Slab_kmalloc_arg_Clear | Slab_kmalloc_arg_Private, NULL);
	for (int i = 0; i < dev->devDesc->bNumCfg; i++) {
		// get configuration descriptor
		dev->cfgDesc[i] = kmalloc(0xff, Slab_kmalloc_arg_Private, NULL);
		HW_USB_XHCI_TRB_setData(&req1->trb[0], HW_USB_XHCI_TRB_mkSetup(0x80, 0x6, 0x0200 | i, 0x0, 0xff));
		HW_USB_XHCI_TRB_setData(&req1->trb[1], DMAS_virt2Phys(dev->cfgDesc[i]));
		HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
		if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
			printk(RED, BLACK, "dev %#018lx: failed to get configuration descriptor #%ld, code=%d\n", dev, i, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
			dev->mgrTask = NULL;
			Task_setSignal(Task_current, Task_Signal_Int);
			while (1) IO_hlt();
		}
		printk(GREEN, BLACK, "dev %#018lx: configuration descriptor #%ld: %#018lx\n", dev, i, *(u64 *)dev->cfgDesc[i]);
	}
	kfree(req0, Slab_kmalloc_arg_Private);
	kfree(req1, Slab_kmalloc_arg_Private);
	printk(YELLOW, BLACK, "dev %#018lx: search for driver\n");
	while (1) {
		SpinLock_lock(&HW_USB_XHCI_DriverListLock);
		// search for a compatible driver
		for (List *drvList = HW_USB_XHCI_DriverList.next; drvList != &HW_USB_XHCI_DriverList; drvList = drvList->next) {
			XHCI_Driver *driver = container(drvList, XHCI_Driver, list);
			if (driver->check(dev)) {
				SpinLock_unlock(&HW_USB_XHCI_DriverListLock);
				dev->driver = driver;
				// enable this device
				if (driver->enable) driver->enable(dev);
				dev->state = XHCI_Device_State_Enable;
				// move to the specific process in the driver for this device
				driver->process(dev);
				dev->driver = NULL;
				goto End;
			}
		}
		SpinLock_unlock(&HW_USB_XHCI_DriverListLock);
		Intr_SoftIrq_Timer_mdelay(dev->slotId * 10);
	}
	End:
	while (1) IO_hlt();
	Task_kernelThreadExit(0);
}
void HW_USB_XHCI_test(XHCI_Host *host) {
}
