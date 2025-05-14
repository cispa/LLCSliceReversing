#ifndef STATS_H_
#define STATS_H_

#include <stdint.h>

uint64_t calculate_mean_uint64(uint64_t *data, int n);
double calculate_mean_double(double *data, int n);

uint64_t calculate_standard_deviation_uint64(uint64_t data[], int n);
double calculate_standard_deviation_double(double data[], int n);

#endif