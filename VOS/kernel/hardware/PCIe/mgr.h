#ifndef __HAREWARE_PCIE_MGR_H__
#define __HAREWARE_PCIE_MGR_H__

#include "../PCIe.h"

typedef struct {
    List listEle;
    PCIeConfig *device;
    u8 bus, dev, func;
} PCIeManager;

#endif