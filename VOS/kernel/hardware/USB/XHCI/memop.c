#include "inner.h"
#include "../../../includes/log.h"

// allocate memory for the controller，use DMAS_virt2Phys to get the physical address
void *HW_USB_XHCI_alloc(USB_XHCIController *ctrl, u64 size) {
	if (size == 0) return NULL;
	void *addr; Page *page;
	// 1MB
	if (size > MM_Slab_maxSize) {
		page = MM_Buddy_alloc(log2Ceil(size) - Page_4KShift, Page_Flag_Kernel | Page_Flag_Active | Page_Flag_KernelShare);
		if (page == NULL) goto _alloc_Fail;
		addr = DMAS_phys2Virt(page->phyAddr);
	} else {
		addr = kmalloc(size, 0);
		if (addr == NULL) goto _alloc_Fail;
	}
	USB_XHCI_MemUsage *usage = (USB_XHCI_MemUsage *)kmalloc(sizeof(USB_XHCI_MemUsage), 0);
	List_init(&usage->listEle);
	List_insBefore(&usage->listEle, &ctrl->memList);
	if (size > MM_Slab_maxSize) usage->addr = (u64)addr;
	else usage->addr = ((u64)page) | 1;
	memset(addr, 0, size);
	return addr;
	_alloc_Fail:
	printk(RED, BLACK, "XHCI: alloc memory fail");
	printk(YELLOW, BLACK, "(ctrl:%#018lx,size:%#018lx)", ctrl, size);
	return NULL;
}

void HW_USB_XHCI_free(USB_XHCIController *ctrl, void *addr) {
	USB_XHCI_MemUsage *usage;
	for (List *list = ctrl->memList.next; list != &ctrl->memList; list = list->next) {
		usage = container(ctrl->memList.next, USB_XHCI_MemUsage, listEle);
		if ((usage->addr & 0x1ul ? DMAS_phys2Virt(((Page *)(usage->addr & ~0x1ul))->phyAddr) : (void *)usage->addr) == addr) {
			List_del(list);
			if (usage->addr & 1) MM_Buddy_free((Page *)(usage->addr ^ 1));
			else kfree((void *)usage->addr, 0);
			kfree(usage, 0);
			break;
		}
	}
}

void HW_USB_XHCI_freeAll(USB_XHCIController *ctrl) {
	// free all pages
	USB_XHCI_MemUsage *usage;
	while (!List_isEmpty(&ctrl->memList)) {
		usage = container(ctrl->memList.next, USB_XHCI_MemUsage, listEle);
		List_del(&usage->listEle);
		if (usage->addr & 1) MM_Buddy_free((Page *)(usage->addr ^ 1));
		else kfree((void *)usage->addr, 0);
		kfree(usage, 0);
	}
}