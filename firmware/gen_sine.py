#!/bin/sh
# gen_sine.py - generate a quarter-cycle sine LUT
# 07-27-23 E. Brombaugh

import math as math
import numpy as np


# expo table
addr_bits = 10
N = 2**addr_bits
sine = np.floor(32767 * np.sin(0.5 * np.pi * (np.arange(N)+0.5)/N))

# print results in C format
print('#define SINE_LUT_BITS', addr_bits)
print('/* 16-bit quarter-cycle Sine table */')
print('const int16_t Sine16bit[', N, '] = {')
for i in range(0, N, 8):
    print('\t', end ="")
    print(''.join('{:5.0f},'.format(item) for item in sine[range(i,i+8)]))
print('};')
