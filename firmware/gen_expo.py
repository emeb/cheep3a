#!/bin/sh
# gen_expo.py - generate an expo LUT
# 07-22-23 E. Brombaugh

import math as math
import numpy as np

N_bits = 7  # bits in LUT addr
Fs = 48000 # sample rate
ADC_max = 1023 # Max ADC value in CV range
octaves = 10 # total octaves in CV range
frac_bits = 16 # scaling fraction bits
phs_bits = 32 # bits in NCO phase
low_Hz = 10 # bottom frequency

# expo table
N = 2**N_bits
expo = np.floor(16384 * np.power(2, np.arange(N)/N))

# adjustment factors
oscale = octaves / ADC_max
lin_scale = math.floor(2**frac_bits * oscale + 0.5)
lin_shf = frac_bits - N_bits
low_inc = low_Hz / (Fs / 2**phs_bits)
oct_shf = math.floor(math.log2(low_inc / 16384))
low_val = low_inc / 2**oct_shf
low_idx = 0
while low_val > expo[low_idx]:
    low_idx = low_idx + 1

# print results in C format
print('#define LIN_SCALE', lin_scale)
print('#define LIN_SHF', lin_shf)
print('#define OCT_SHF', oct_shf)
print('#define LOW_IDX', low_idx)
print('#define LUT_MASK', N-1)
print('#define LUT_SHF', N_bits)

print('/*', N, 'step expo table */')
print('int16_t expo_lut[] = {')
for i in range(0, N, 8):
    print('\t', end ="")
    print(''.join('{:5.0f},'.format(item) for item in expo[range(i,i+8)]))
print('};')
