#include "stats.h"

#include <math.h>
#include <stdint.h>

uint64_t calculate_mean_uint64(uint64_t *data, int n) {
  uint64_t sum = 0;
  for (int i = 0; i < n; i++) {
    sum += data[i];
  }
  return sum / n;
}

double calculate_mean_double(double *data, int n) {
  double sum = 0.0;
  for (int i = 0; i < n; i++) {
    sum += data[i];
  }
  return sum / n;
}

uint64_t calculate_standard_deviation_uint64(uint64_t data[], int n) {
  uint64_t mean = calculate_mean_uint64(data, n);
  uint64_t ssd = 0;

  for (int i = 0; i < n; i++) {
    uint64_t difference = data[i] - mean;
    ssd += difference * difference;
  }

  uint64_t variance = ssd / n;
  return (uint64_t)sqrt((double)variance);
}

double calculate_standard_deviation_double(double data[], int n) {
  double mean = calculate_mean_double(data, n);
  double ssd = 0.0;

  for (int i = 0; i < n; i++) {
    double difference = data[i] - mean;
    ssd += difference * difference;
  }

  double variance = ssd / n;
  return sqrt(variance);
}
