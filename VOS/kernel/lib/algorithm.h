#ifndef __LIB_ALGORITHM_H__
#define __LIB_ALGORITHM_H__
#define upAlignTo(x, bs) (((x) + (bs) - 1) / (bs) * (bs))
#define downAlignTo(x, bs) (((x) / (bs) * (bs)))

#define max(a, b) ({__typeof__(a) ta = (a); __typeof__(b) tb = (b); (ta) > (tb) ? (ta) : (tb); })
#define min(a, b) ({__typeof__(a) ta = (a); __typeof__(b) tb = (b); (ta) > (tb) ? (tb) : (ta); })

#include "ds.h"

// get floor(log2(n))
i32 log2(u64 n);
// get ceil(log2(n))
i32 log2Ceil(u64 n);

// get the lowest bit of x
#define lowbit(x) ({__typeof__(x) tx = (x); tx & -tx; })
#endif