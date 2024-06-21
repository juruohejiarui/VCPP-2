#include "algorithm.h"

i32 log2(u64 n) {
	i32 res = 0;
	if (n & 0xFFFFFFFF00000000ul) res += 32, n >>= 32;
	if (n & 0x00000000FFFF0000ul) res += 16, n >>= 16;
	if (n & 0x000000000000FF00ul) res += 8, n >>= 8;
	if (n & 0x00000000000000F0ul) res += 4, n >>= 4;
	if (n & 0x000000000000000Cul) res += 2, n >>= 2;
	if (n & 0x0000000000000002ul) res += 1, n >>= 1;
	return res;
}
i32 log2Ceil(u64 n) {
	i32 res = ((n & (n - 1)) ? 1 : 0) + log2(n);
	return res;
}