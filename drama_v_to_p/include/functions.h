#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int compute_slice(uint64_t x);
int compute_slice_coffee_lake(uint64_t x);
int compute_slice_alder_lake(uint64_t x);
int compute_slice_raptor_lake(uint64_t x);
int compute_slice_4_core(uint64_t x);
int compute_slice_wiowio(uint64_t x);

#ifdef __cplusplus
}
#endif

#endif