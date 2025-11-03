#include "functions.h"

#include <stdint.h>

#define COFFEE_LAKE
#define _B(x, pos) (((x) >> (pos)) & 1)

uint64_t eval_chain(uint64_t x, int *chain, int chain_length) {
  uint64_t result = 0;
  for (int i = 0; i < chain_length; i++) {
    result |= _B(x, chain[i]) << i;
  }
  return result;
}

uint64_t a_0(uint64_t x) {
  return _B(x, 6) ^ _B(x, 11) ^ _B(x, 12) ^ _B(x, 16) ^ _B(x, 18) ^ _B(x, 21) ^
         _B(x, 22) ^ _B(x, 23) ^ _B(x, 24) ^ _B(x, 26) ^ _B(x, 30) ^ _B(x, 31) ^
         _B(x, 32) ^ _B(x, 35) ^ _B(x, 38);
}

uint64_t a_1(uint64_t x) {
  return _B(x, 7) ^ _B(x, 12) ^ _B(x, 13) ^ _B(x, 17) ^ _B(x, 19) ^ _B(x, 22) ^
         _B(x, 23) ^ _B(x, 24) ^ _B(x, 25) ^ _B(x, 27) ^ _B(x, 31) ^ _B(x, 32) ^
         _B(x, 33) ^ _B(x, 36);
}

uint64_t a_2(uint64_t x) {
  return _B(x, 8) ^ _B(x, 13) ^ _B(x, 14) ^ _B(x, 18) ^ _B(x, 20) ^ _B(x, 23) ^
         _B(x, 24) ^ _B(x, 25) ^ _B(x, 26) ^ _B(x, 28) ^ _B(x, 32) ^ _B(x, 33) ^
         _B(x, 34) ^ _B(x, 37);
}

uint64_t a_3(uint64_t x) {
  return _B(x, 9) ^ _B(x, 14) ^ _B(x, 15) ^ _B(x, 19) ^ _B(x, 21) ^ _B(x, 24) ^
         _B(x, 25) ^ _B(x, 26) ^ _B(x, 27) ^ _B(x, 29) ^ _B(x, 33) ^ _B(x, 34) ^
         _B(x, 35) ^ _B(x, 38);
}

uint64_t a_4(uint64_t x) {
  return _B(x, 10) ^ _B(x, 15) ^ _B(x, 16) ^ _B(x, 20) ^ _B(x, 22) ^ _B(x, 25) ^
         _B(x, 26) ^ _B(x, 27) ^ _B(x, 28) ^ _B(x, 30) ^ _B(x, 34) ^ _B(x, 35) ^
         _B(x, 36);
}

uint64_t a_5(uint64_t x) {
  return _B(x, 11) ^ _B(x, 16) ^ _B(x, 17) ^ _B(x, 21) ^ _B(x, 23) ^ _B(x, 26) ^
         _B(x, 27) ^ _B(x, 28) ^ _B(x, 29) ^ _B(x, 31) ^ _B(x, 35) ^ _B(x, 36) ^
         _B(x, 37);
}

uint64_t x_0(uint64_t x) {
  return _B(x, 6) ^ _B(x, 8) ^ _B(x, 9) ^ _B(x, 10) ^ _B(x, 14) ^ _B(x, 15) ^
         _B(x, 17) ^ _B(x, 18) ^ _B(x, 20) ^ _B(x, 23) ^ _B(x, 27) ^ _B(x, 30) ^
         _B(x, 31) ^ _B(x, 34) ^ _B(x, 36) ^ _B(x, 38);
}

uint64_t x_1(uint64_t x) {
  return _B(x, 9) ^ _B(x, 11) ^ _B(x, 12) ^ _B(x, 13) ^ _B(x, 14) ^ _B(x, 17) ^
         _B(x, 19) ^ _B(x, 21) ^ _B(x, 22) ^ _B(x, 26) ^ _B(x, 27) ^ _B(x, 29) ^
         _B(x, 32);
}

int gpu_4_chain_0(uint64_t x) {
  int l_0[] = {9, 13, 14, 17, 18, 19, 20, 21, 22, 23, 25, 27, 32, 30, 36, 38};
  int l_0_length = sizeof(l_0) / sizeof(l_0[0]);
  return eval_chain(x, l_0, l_0_length);
}

int gpu_4(uint64_t x) {
  int val1 = (a_1(x) & a_3(x)) & (a_4(x) & a_5(x));
  int val2 = val1 & (gpu_4_chain_0(x) ^ 1);
  return val2;
}

int gpu_extra(uint64_t x) {
  int val1 = (a_1(x) & a_3(x)) & (a_4(x) | a_5(x));
  int val2 = val1 & (gpu_4_chain_0(x) ^ 1);
  return val1 & (val2 ^ 1);
}

int gpu_3_chain_0(uint64_t x) {
  int l_0[] = {6, 9, 11, 17, 18, 19, 24, 25, 27, 29, 30, 35, 36, 37};
  int l_0_length = sizeof(l_0) / sizeof(l_0[0]);
  return eval_chain(x, l_0, l_0_length);
}

int gpu_3(uint64_t x) {
  int val0 = gpu_3_chain_0(x) & gpu_extra(x);
  int val1 = (1 ^ gpu_4(x)) & (a_0(x) ^ val0);
  return val1;
}

int gpu_2_chain_0(uint64_t x) {
  int l_0[] = {12, 14, 16, 18, 19, 20, 22, 24, 25,
               27, 28, 30, 31, 32, 33, 35, 36, 38};
  int l_0_length = sizeof(l_0) / sizeof(l_0[0]);
  return eval_chain(x, l_0, l_0_length);
}

int gpu_2_chain_1(uint64_t x) {
  int l_0[] = {8, 13, 14, 18, 20, 23, 24, 25, 26, 28, 32, 33, 34, 37};
  int l_0_length = sizeof(l_0) / sizeof(l_0[0]);
  return eval_chain(x, l_0, l_0_length);
}

int gpu_2(uint64_t x) {
  int val0 = gpu_2_chain_0(x) & gpu_extra(x);
  int val1 = (1 ^ gpu_4(x)) & (gpu_2_chain_1(x) ^ val0);
  return val1;
}

int gpu_1_chain_0(uint64_t x) {
  int l_0[] = {11, 12, 15, 20, 21, 25, 26, 27, 28, 29, 32, 34, 35, 36, 37, 38};
  int l_0_length = sizeof(l_0) / sizeof(l_0[0]);
  return eval_chain(x, l_0, l_0_length);
}

int gpu_1_chain_1(uint64_t x) {
  int l_0[] = {9, 13, 14, 17, 18, 19, 20, 21, 22, 23, 25, 27, 30, 32, 36, 38};
  int l_0_length = sizeof(l_0) / sizeof(l_0[0]);
  return eval_chain(x, l_0, l_0_length);
}

int gpu_1(uint64_t x) {
  int val0 = gpu_1_chain_0(x) & gpu_extra(x);
  int val1 = (1 ^ gpu_4(x)) & (gpu_1_chain_1(x) ^ val0);
  return val1;
}

int gpu_0_chain_0(uint64_t x) {
  int l_0[] = {8, 12, 14, 16, 19, 21, 22, 23, 27, 28, 29, 31, 32, 38};
  int l_0_length = sizeof(l_0) / sizeof(l_0[0]);
  return eval_chain(x, l_0, l_0_length);
}

int gpu_0_chain_1(uint64_t x) {
  int l_0[] = {6,  7,  8,  9,  10, 12, 13, 14, 15, 18,
               19, 20, 22, 24, 25, 30, 32, 33, 34, 38};
  int l_0_length = sizeof(l_0) / sizeof(l_0[0]);
  return eval_chain(x, l_0, l_0_length);
}

int gpu_0(uint64_t x) {
  int val0 = gpu_1_chain_0(x) & gpu_extra(x);
  int val1 = gpu_1_chain_1(x) ^ val0;
  return val1;
}

int compute_slice_xeon(uint64_t x) {
  return gpu_0(x) | (gpu_1(x) << 1) | (gpu_2(x) << 2) | (gpu_3(x) << 3) |
         (gpu_4(x) << 4);
}

int compute_slice_alder_lake(uint64_t x) {
  uint64_t val3 = ((a_4(x) | a_5(x)) & (a_1(x) & a_3(x))) & 1;
  uint64_t val2 = (~val3 & a_0(x)) & 1;
  uint64_t val1 = (~val3 & a_2(x)) & 1;
  return x_0(x) | (val1 << 1) | (val2 << 2) | (val3 << 3);
}

int compute_slice_raptor_lake(uint64_t x) {
  uint64_t val2 =
      (((a_2(x)) | (a_4(x)) | (a_5(x))) & ((a_2(x)) | (a_3(x))) & (a_0(x))) & 1;
  uint64_t val1 = (~val2 & a_1(x)) & 1;
  return x_0(x) | (x_1(x) << 1) | (val1 << 2) | (val2 << 3);
}

int compute_slice_coffee_lake(uint64_t x) {
  uint64_t val2 =
      (((a_2(x)) | (a_4(x)) | (a_5(x))) & ((a_2(x)) | (a_3(x))) & (a_0(x))) & 1;
  uint64_t val1 = (~val2 & a_1(x)) & 1;
  return x_0(x) | (val1 << 1) | (val2 << 2);
}

int compute_slice_4_core(uint64_t x) {
  uint64_t val0 = _B(x, 6) ^ _B(x, 10) ^ _B(x, 12) ^ _B(x, 14) ^ _B(x, 16) ^
                  _B(x, 17) ^ _B(x, 18) ^ _B(x, 20) ^ _B(x, 22) ^ _B(x, 24) ^
                  _B(x, 25) ^ _B(x, 26) ^ _B(x, 27) ^ _B(x, 28) ^ _B(x, 30) ^
                  _B(x, 32) ^ _B(x, 33);
  uint64_t val1 = _B(x, 7) ^ _B(x, 11) ^ _B(x, 13) ^ _B(x, 15) ^ _B(x, 17) ^
                  _B(x, 19) ^ _B(x, 20) ^ _B(x, 21) ^ _B(x, 22) ^ _B(x, 23) ^
                  _B(x, 24) ^ _B(x, 26) ^ _B(x, 28) ^ _B(x, 29) ^ _B(x, 31) ^
                  _B(x, 33) ^ _B(x, 34);

  return val1 << 1 | val0;
}

int compute_slice(uint64_t x) {
  int slice = 0;
#ifdef ALDER_LAKE
  slice = compute_slice_alder_lake(x);
#elif defined(COFFEE_LAKE)
  slice = compute_slice_coffee_lake(x);
#elif defined(RAPTOR_LAKE)
  slice = compute_slice_raptor_lake(x);
#elif defined(COMET_LAKE)
  // TODO: add
#elif defined(XEON)
  slice = compute_slice_xeon(x);
#endif
  return slice;
}

int compute_slice_wiowio(uint64_t x) {
  uint64_t val0 = _B(x, 6) ^ _B(x, 10) ^ _B(x, 12) ^ _B(x, 14) ^ _B(x, 16) ^
                  _B(x, 17) ^ _B(x, 18) ^ _B(x, 20) ^ _B(x, 22) ^ _B(x, 24) ^
                  _B(x, 25) ^ _B(x, 26) ^ _B(x, 27) ^ _B(x, 28) ^ _B(x, 30) ^
                  _B(x, 32);
  uint64_t val1 = _B(x, 9) ^ _B(x, 12) ^ _B(x, 16) ^ _B(x, 17) ^ _B(x, 19) ^
                  _B(x, 21) ^ _B(x, 22) ^ _B(x, 23) ^ _B(x, 25) ^ _B(x, 26) ^
                  _B(x, 27) ^ _B(x, 29) ^ _B(x, 31) ^ _B(x, 32);
  uint64_t val2 = _B(x, 10) ^ _B(x, 11) ^ _B(x, 13) ^ _B(x, 16) ^ _B(x, 17) ^
                  _B(x, 18) ^ _B(x, 19) ^ _B(x, 20) ^ _B(x, 21) ^ _B(x, 22) ^
                  _B(x, 27) ^ _B(x, 28) ^ _B(x, 30) ^ _B(x, 31);

  return val0 | val1 << 1 | (val2 << 2);
}
