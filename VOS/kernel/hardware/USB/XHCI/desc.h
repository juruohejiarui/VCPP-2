#ifndef __HW_USB_XHCI_DESC_H__
#define __HW_USB_XHCI_DESC_H__

#include "../../../includes/lib.h"
#include "../../../includes/task.h"
#include "../../PCIe.h"

typedef struct XHCI_GenerTRB {
	u32 data1;
	u32 data2;
	u32 status;
	u32 ctrl;
} __attribute__ ((packed)) XHCI_GenerTRB;

// evaluate next TRB
#define XHCI_TRB_Ctrl_ent		(1u << 1)
// interrupt on short packet
#define XHCI_TRB_Ctrl_isp		(1u << 2)
// no snoop
#define XHCI_TRB_Ctrl_noSnoop	(1u << 3)
#define XHCI_TRB_Ctrl_chain		(1u << 4)
// interrupt on completion
#define XHCI_TRB_Ctrl_ioc		(1u << 5)
// immediate data
#define XHCI_TRB_Ctrl_idt		(1u << 6)
// block event interrupt
#define XHCI_TRB_Ctrl_bei		(1u << 9)

#define XHCI_TRB_Ctrl_allBit		(0x0000027eu)

#define XHCI_TRB_Ctrl_Dir_In		1
#define XHCI_TRB_Ctrl_Dir_Out		0
#define XHCI_TRB_TRT_No		0
#define XHCI_TRB_TRT_Out	2
#define XHCI_TRB_TRT_In		3

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
	XHCI_GenerTRB res;
	XHCI_GenerTRB *trb;
	struct XHCI_Request ***target;
} __attribute__ ((packed)) XHCI_Request;

// because of the limitation of that software can only accessing data in dword / quad style
// all the fields description here in contexts will be provided by bit masking

// XHCI Device Slot Context
typedef struct XHCI_SlotCtx {
	u32 dw0;
	#define XHCI_SlotCtx_routeStr	0x000fffffu
	#define XHCI_SlotCtx_speed		0x00f00000u
	#define XHCI_SlotCtx_mtt		0x02000000u
	#define XHCI_SlotCtx_hub		0x04000000u
	#define XHCI_SlotCtx_ctxEntries	0xf8000000u
	u32 dw1;
	#define XHCI_SlotCtx_maxExitLatency	0x0000ffffu
	#define XHCI_SlotCtx_rootPortNum	0x00ff0000u
	#define XHCI_SlotCtx_numOfPorts		0xff000000u
	u32 dw2;
	#define XHCI_SlotCtx_ttHubSlotId	0x000000ffu
	#define XHCI_SlotCtx_ttPortNum		0x0000ff00u
	#define XHCI_SlotCtx_ttt			0x00030000u
	#define XHCI_SlotCtx_intrTarget		0xffc00000u
	u32 dw3;
	#define XHCI_SlotCtx_devAddr	0x000000ffu
	#define XHCI_SlotCtx_slotState	0xff000000u
	u32 reserved[4];
} __attribute__ ((packed)) XHCI_SlotCtx;

// XHCI Device Endpoint Context
typedef struct XHCI_EpCtx {
	u32 dw0;
	#define XHCI_EpCtx_epState		0x00000007u
	#define XHCI_EpCtx_multi		0x00000300u
	#define XHCI_EpCtx_mxPStreams	0x00007C00u
	#define XHCI_EpCtx_lsa			0x00008000u
	#define XHCI_EpCtx_interval		0x00ff0000u
	#define XHCI_EpCtx_mxESITPayH	0xff000000u
	u32 dw1;
	#define XHCI_EpCtx_CErr			0x00000006u
	#define XHCI_EpCtx_epType		0x00000038u
	#define XHIC_EpCtx_hid			0x00000080u
	#define XHCI_EpCtx_mxBurstSize	0x0000ff00u
	#define XHCI_EpCtx_mxPackSize	0xffff0000u
	union {
		u64 deqPtr;
		struct {
			u32 dw2;
			u32 dw3;
		} __attribute__ ((packed)) dw2_3;
	};
	#define XHCI_EpCtx_dcs	0x00000001u
	u32 dw4;
	#define XHCI_EpCtx_aveTrbLen	0x0000ffffu
	#define XHCI_EpCtx_mxESITPayL	0xffff0000u
	u32 reserved[3]; 
} __attribute__ ((packed)) XHCI_EpCtx;

#define XHCI_EpCtx_epType_IsochOut	1
#define XHCI_EpCtx_epType_BulkOut	2
#define XHCI_EpCtx_epType_IntrOut	3
#define XHCI_EpCtx_epType_Control	4
#define XHCI_EpCtx_epType_IsochIn	5
#define XHCI_EpCtx_epType_BulkIn	6
#define XHCI_EpCtx_epType_IntrIn	7

typedef struct XHCI_DevCtx {
	XHCI_SlotCtx slot;
	XHCI_EpCtx ep[31];
} __attribute__ ((packed)) XHCI_DevCtx;

typedef struct XHCI_InCtrlCtx {
	u32 dropFlags;
	u32 addFlags;
	u32 reserved[5];
	u32 dw7;
	#define XHCI_InCtrlCtx_cfgVal	0x000000ffu
	#define XHCI_InCtrlCtx_inteNum	0x0000ff00u
	#define XHCI_InCtrlCtx_AlterSet	0x00ff0000u
} __attribute__ ((packed)) XHCI_InCtrlCtx;

typedef struct XHCI_InputCtx {
	XHCI_InCtrlCtx ctrl;
	XHCI_SlotCtx slot;
	XHCI_EpCtx ep[31];
} __attribute__ ((packed)) XHCI_InputCtx;

typedef struct XHCI_Ring {
	SpinLock lock;
	XHCI_GenerTRB *ring, *cur;
	u32 curPos;
	u32 cycBit;
	u32 size;
	XHCI_Request **reqSrc;
} XHCI_Ring;

#pragma region Descriptors
typedef struct XHCI_DescHdr {
	u8 len;
	u8 type;
} __attribute__ ((packed)) XHCI_DescHdr;
typedef struct XHCI_DevDesc {
	XHCI_DescHdr hdr;
	u16 bcdUsb;
	u8 bDevClass;
	u8 bDevSubClass;
	u8 bDevProtocol;
	u8 bMaxPackSz0;
	u16 idVendor;
	u16 idProduct;
	u16 bcdDev;
	u8 iManufacturer;
	u8 iProduct;
	u8 iSerialNum;
	u8 bNumCfg;
} __attribute__ ((packed)) XHCI_DevDesc;

typedef struct XHCI_CfgDesc {
	XHCI_DescHdr hdr;
	u16 wtotLen;
	u8 bNumInter;
	u8 bCfgVal;
	u8 iCfg;
	u8 bmAttr;
	u8 bMxPw;
} __attribute__ ((packed)) XHCI_CfgDesc;

typedef struct XHCI_StrDesc {
	XHCI_DescHdr hdr;
	u8 str[0];
} __attribute__ ((packed)) XHCI_StrDesc;

typedef struct XHCI_InterDesc {
	XHCI_DescHdr hdr;
	u8 bInterNum;
	u8 bAlterSet;
	u8 bNumEp;
	u8 bInterClass;
	u8 bInterSubClass;
	u8 bInterProto;
	u8 iInter;
} __attribute__ ((packed)) XHCI_InterDesc;

typedef struct XHCI_EpDesc {
	XHCI_DescHdr hdr;
	u8 bEpAddr;
	u8 bmAttr;
	u16 wMxPackSz;
	u8 interval;
} __attribute__ ((packed)) XHCI_EpDesc;

#define XHCI_Descriptor_Type_Device		0x01
#define XHCI_Descriptor_Type_Cfg		0x02
#define XHCI_Descriptor_Type_Str		0x03
#define XHCI_Descriptor_Type_Inter		0x04
#define XHCI_Descriptor_Type_Endpoint	0x05
#define XHCI_Descriptor_Type_HID		0x21

#pragma endregion

typedef struct XHCI_Device {
	struct XHCI_Host *host;
	int slotId;
	int state;
	#define XHCI_Device_State_Disable	0
	#define XHCI_Device_State_Enable	1
	TaskStruct *mgrTask;
	XHCI_DevCtx *ctx;
	XHCI_InputCtx *inCtx;
	// transfer ring for each endpoint
	XHCI_Ring *trRing[31];

	XHCI_DevDesc *devDesc;
	XHCI_CfgDesc **cfgDesc;

	// suitable driver for this device
	struct XHCI_Driver *driver;
} XHCI_Device;

typedef struct XHCI_Driver {
	// checking if this driver is valid for a specific device
	int (*check)(XHCI_Device *);
	// function called by device management task, as the main process of driver
	void (*process)(XHCI_Device *);
	// process when kernel or application disable this device and before this device gets into Disable status
	void (*disable)(XHCI_Device *);
	// process when kernel or application enable this device from Disable status and before move to Enabled status
	void (*enable)(XHCI_Device *);
	// process after the device is removed from the device
	void (*uninstall)(XHCI_Device *);

	char *name;
	List list;
} XHCI_Driver;

extern SpinLock HW_USB_XHCI_DriverListLock;
extern List HW_USB_XHCI_DriverList;

typedef struct XHCI_EveRing {
	XHCI_GenerTRB **rings;
	u32 curRingId, curPos;
	u32 ringNum, ringSize;
	u32 cycBit;
} XHCI_EveRing;

typedef struct XHCI_Event {
	XHCI_GenerTRB trb;
	List list;
} XHCI_Event;

#define XHCI_Ring_maxSize (Page_4KSize * 16 / sizeof(XHCI_GenerTRB))

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
	XHCI_Device *dev;
} XHCI_PortInfo;

#define XHCI_Port_Speed_Full	1 // USB 2.0
#define XHCI_Port_Speed_Low		2 // USB 1.1
#define XHCI_Port_Speed_High	3 // USB 2.0
#define XHCI_Port_Speed_Super	4 // USB 3.0

#define XHCI_EveHandleTaskNum	0x4

typedef struct XHCI_Host {
	List listEle;
	PCIeManager *pci;
	PCIe_MSICapability *msiCapDesc;
	PCIe_MSIXCapability *msixCapDesc;

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
	
	TaskStruct **eveHandlerTask;

	XHCI_GenerTRB **eveQue;
	#define XHCI_Host_EveQueSize 1024
	int *eveQueHdr, *eveQueTil, *eveQueLen;
	SpinLock *eveLock, addr0Lock;

	XHCI_Device **dev;

	u64 flags;
	#define XHCI_Host_Flag_Initialized	(1 << 0)
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
#define XHCI_PortReg_sc_AllEve		(0xe000000u)
#define XHCI_PortReg_sc_AllChg		(0xfe0000u)
#define XHCI_PortReg_sc_Power		(1u << 9)
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