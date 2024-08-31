#include "ds.h"
#include "../includes/log.h"

u32 Bit_ffs(u64 val) {
	u32 n = 1;
	if (!val) return 0;
	if (!(val & 0x00000000FFFFFFFF)) { val >>= 32; n += 32; }
	if (!(val & 0x000000000000FFFF)) { val >>= 16; n += 16; }
	if (!(val & 0x00000000000000FF)) { val >>=  8; n += 8;  }
	if (!(val & 0x000000000000000F)) { val >>=  4; n += 4;  }
	if (!(val & 0x0000000000000003)) { val >>=  2; n += 2;  }
	if (!(val & 0x0000000000000001)) { val >>=  1; n += 1;  }
	return n;
}