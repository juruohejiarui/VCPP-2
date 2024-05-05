#ifndef __LIB_ALGORITHM_H__
#define __LIB_ALGORITHM_H__
#define upAlignTo(x, bs) (((x) + (bs) - 1) / (bs) * (bs))
#define downAlignTo(x, bs) (((x) / (bs) * (bs)))

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) > (b) ? (b) : (a))
#endif