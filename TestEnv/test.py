import numpy as np
import time

st = time.time()
f = np.zeros(100000000 + 1, np.int32)
f[0], f[1] = 1, 1
for i in range(2, 100000000 + 1):
    f[i] = f[i - 1] + f[i - 2]
print(f[10000000])
ed = time.time()
print((ed - st))