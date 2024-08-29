#ifndef __HW_USB_XHCI_DESC_H__
#define __HW_USB_XHCI_DESC_H__

#include "../../../includes/lib.h"
#include "../../PCIe.h"

typedef struct XHCI_GenerTRB {
	u32 data1;
	u32 data2;
	u32 status;
	u32 ctrl;
} __attribute__ ((packed)) XHCI_GenerTRB;

enum XHCI_TRB_Type {
	XHCI_TRB_Type_Normal = 1, 	XHCI_TRB_Type_SetupStage, 	XHCI_TRB_Type_DataStage,	XHCI_TRB_Type_StatusStage,
	XHCI_TRB_Type_Isoch, 		XHCI_TRB_Type_Link, 		XHCI_TRB_Type_EventData,	XHCI_TRB_Type_NoOp,
	XHCI_TRB_Type_EnblSlot,		XHCI_TRB_Type_DisblSlot,	XHCI_TRB_Type_AddrDev,		XHCI_TRB_Type_CfgEp,
	XHCI_TRB_Type_EvalCtx,		XHCI_TRB_Type_ResetEp,		XHCI_TRB_Type_StopEp,		XHCI_TRB_Type_SetTrDeq,
	XHCI_TRB_Type_ResetDev,		XHCI_TRB_Type_ForceEvent,	XHCI_TRB_Type_DegBand,		XHCI_TRB_Type_SetLatToler,
	XHCI_TRB_Type_GetPortBand,	XHCI_TRB_Type_ForceHdr,		XHCI_TRB_Type_NoOpCmd,
	XHCI_TRB_Type_TransEve = 32,XHCI_TRB_Type_CmdCmpl,		XHCI_TRB_Type_PortStChg,	XHCI_TRB_Type_BandReq,
	XHCI_TRB_Type_DbEve,		XHCI_TRB_Type_HostCtrlEve,	XHCI_TRB_Type_DevNotfi,		XHCI_TRB_Type_MfidxWrap
};

enum XHCI_TRB_CmplCode {
	XHCI_TRB_CmplCode_Succ = 1,			XHCI_TRB_CmplCode_DtBufErr,			XHCI_TRB_CmplCode_BadDetect,	XHCI_TRB_CmplCode_TransErr,
	XHCI_TRB_CmplCode_TrbErr,			XHCI_TRB_CmplCode_StallErr,			XHCI_TRB_CmplCode_ResrcErr,		XHCI_TRB_CmplCode_BandErr,
	XHCI_TRB_CmplCode_NoSlotErr,		XHCI_TRB_CmplCode_InvalidStreamT,	XHCI_TRB_CmplCode_SlotNotEnbl,	XHCI_TRB_CmplCode_EpNotEnbl,
	XHCI_TRB_CmplCode_ShortPkg = 13,	XHCI_TRB_CmplCode_RingUnderRun,		XHCI_TRB_CmplCode_RingOverRun,	XHCI_TRB_CmplCode_VFEveRingFull,
	XHCI_TRB_CmplCode_ParamErr,			XHCI_TRB_CmplCode_BandOverRun,		XHCI_TRB_CmplCode_CtxStsErr,	XHCI_TRB_CmplCode_NoPingResp,
	XHCI_TRB_CmplCode_EveRingFull,		XHCI_TRB_CmplCode_IncompDev,		XHCI_TRB_CmplCode_MissServ,		XHCI_TRB_CmplCode_CmdRingStop,
	XHCI_TRB_CmplCode_CmdAborted,		XHCI_TRB_CmplCode_Stop,				XHCI_TRB_CmplCode_StopLenErr,	XHCI_TRB_CmplCode_Reserved,
	XHCI_TRB_CmplCode_IsochBufOverRun,	XHCI_TRB_CmplCode_EvernLost = 32,	XHCI_TRB_CmplCode_Undefined,	XHCI_TRB_CmplCode_InvalidStreamId,
	XHCI_TRB_CmplCode_SecBand,			XHCI_TRB_CmplCode_SplitTrans
};

typedef struct XHCI_Request {
	u64 flags;
	u64 trbCnt;
	u32 slot, ep;
	XHCI_GenerTRB res;
	XHCI_GenerTRB *trb;
	struct XHCI_Request ***target;
	List list;
} __attribute__ ((packed)) XHCI_Request;

typedef struct XHCI_Ring {
	SpinLock lock;
	XHCI_GenerTRB *ring, *cur;
	u32 curPos;
	u32 cycBit;
	XHCI_Request **reqSrc;
} XHCI_Ring;

typedef struct XHCI_EveRing {
	XHCI_GenerTRB **rings;
	u32 curRingId, curPos;
	u32 ringNum, ringSize;
	u32 cycBit;
} XHCI_EveRing;

#define XHCI_Ring_maxSize (Page_4KSize / sizeof(XHCI_GenerTRB))

#define XHCI_Request_Flag_IsCommand	(1 << 0)
#define XHCI_Request_Flag_IsInRing	(1 << 1)
#define XHCI_Request_Flag_Finished	(1 << 2)

#define XHCI_PortInfo_Flag_Protocol	(1 << 0)
#define XHCI_PortInfo_Flag_isUSB3	(1 << 0)
#define XHCI_PortInfo_Flag_Paired	(1 << 1)
#define XHCI_PortInfo_Flag_Active	(1 << 2)

typedef struct XHCI_PortInfo {
	u8 flags;
	u8 pairOffset; // one based offset to the other speed port, zero means there is no pair
	u8 offset; // one based offset in the specific protocol, zero means this port is invalid
	u8 portId;
} XHCI_PortInfo;

typedef struct XHCI_SlotCtx {

} __attribute__ ((packed)) XHCI_SlotCtx;
typedef struct XHCI_EpCtx {

} __attribute__ ((packed)) XHCI_EpCtx;

typedef struct XHCI_DevCtx {
	XHCI_SlotCtx slot;
	XHCI_EpCtx ep[31];
} __attribute__ ((packed)) XHCI_DevCtx;

typedef struct XHCI_Host {
	List listEle;
	PCIeManager *pci;
	PCIe_MSICapability *msiCapDesc;

	// this address is the virtual address
	u64 capRegAddr;
	u64 opRegAddr;
	u64 rtRegAddr;
	u64 dbRegAddr;
	
	XHCI_PortInfo *port;
	XHCI_Ring *cmdRing;
	XHCI_EveRing *eveRing;

	PCIe_MSI_Descriptor *msiDesc;

	XHCI_DevCtx **devCtx;
} XHCI_Host;

#define XHCI_CapReg_capLen 0x0
#define XHCI_CapReg_hciVer 0x2
#define XHCI_CapReg_hcsParam1 0x4
#define XHCI_CapReg_hcsParam2 0x8
#define XHCI_CapReg_hcsParam3 0xc
#define XHCI_CapReg_hccParam1 0x10
#define XHCI_CapReg_hccParam2 0x1c
#define XHCI_CapReg_dbOff	0x14
#define XHCI_CapReg_rtsOff	0x18

#define XHCI_OpReg_cmd		0x0
#define XHCI_OpReg_status	0x4
#define XHCI_OpReg_pgSize	0x8
// device notification control
#define XHCI_OpReg_dnCtrl	0x14
// command ring control
#define XHCI_OpReg_crCtrl	0x18
// device context base address array pointer
#define XHCI_OpReg_dcbaa	0x30
#define XHCI_OpReg_cfg		0x38

#define XHCI_Ext_Id_Legacy		0x1
#define XHCI_Ext_Id_Protocol	0x2

#define XHCI_PortReg_baseOffset	0x400
#define XHCI_PortReg_offset	0x10
// port status and control
#define XHCI_PortReg_sc		0x00
// port power management staatus and control
#define XHCI_PortReg_pwsc	0x04
// port link info
#define XHCI_PortReg_lk		0x08

#define XHCI_IntrReg_IMan	0x00
#define XHCI_IntrReg_IMod	0x04
#define XHCI_IntrReg_TblSize	0x08
#define XHCI_IntrReg_TblAddr	0x10
#define XHCI_IntrReg_DeqPtr		0x18

extern List HW_USB_XHCI_hostList;
#endif