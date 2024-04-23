#include "APIC.h"
#include "../includes/lib.h"

inline u64 APIC_getReg_IA32_APIC_BASE() { return IO_readMSR(0x1b); }

inline void APIC_setReg_IA32_APIC_BASE(u64 value) { IO_writeMSR(0x1b, value); }

void Init_APIC() {
    u64 apicBase = APIC_getReg_IA32_APIC_BASE();
}
