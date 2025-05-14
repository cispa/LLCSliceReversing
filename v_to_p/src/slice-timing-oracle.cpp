#include "slice-timing-oracle.hpp"

#include <algorithm>
#include <numeric>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <cmath>

#include "pretty-print.hpp"
#include "utils.h"

static uint64_t rdtsc()
{
  uint64_t a, d;
  asm volatile("xor %%rax, %%rax\n"
               "cpuid" ::: "rax", "rbx", "rcx", "rdx");
  asm volatile("rdtscp" : "=a"(a), "=d"(d) : : "rcx");
  a = (d << 32) | a;
  return a;
}

inline __attribute__((always_inline)) uint64_t __rdtsc_begin()
{
  uint64_t a, d;
  asm volatile("mfence\n\t"
               "CPUID\n\t"
               "RDTSCP\n\t"
               "mov %%rdx, %0\n\t"
               "mov %%rax, %1\n\t"
               "mfence\n\t"
               : "=r"(d), "=r"(a)
               :
               : "%rax", "%rbx", "%rcx", "%rdx");
  a = (d << 32) | a;
  return a;
}

// ---------------------------------------------------------------------------
inline __attribute__((always_inline)) uint64_t __rdtsc_end()
{
  uint64_t a, d;
  asm volatile("mfence\n\t"
               "RDTSCP\n\t"
               "mov %%rdx, %0\n\t"
               "mov %%rax, %1\n\t"
               "CPUID\n\t"
               "mfence\n\t"
               : "=r"(d), "=r"(a)
               :
               : "%rax", "%rbx", "%rcx", "%rdx");
  a = (d << 32) | a;
  return a;
}

/*
 * Function that prints an array
 */
template <typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &v)
{
  out << "{";
  size_t last = v.size() - 1;
  for (size_t i = 0; i < v.size(); ++i)
  {
    out << v[i];
    if (i != last)
      out << ", ";
  }
  out << "}";
  return out;
}

/*
 * Function that prints a nested array
 */
template <typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<std::vector<T>> &v)
{
  out << "{";
  size_t last = v.size() - 1;
  for (size_t i = 0; i < v.size(); ++i)
  {
    out << v[i];
    if (i != last)
      out << "\n";
  }
  out << "}";
  return out;
}

uint64_t findMedianSorted(std::vector<uint64_t> &v, size_t offset)
{
  if ((v.size() - offset) % 2 == 0)
  {
    return (v[(v.size() + offset) / 2 - 1] + v[(v.size() + offset) / 2]) / 2;
  }
  else
  {
    return v[(v.size() + offset) / 2];
  }
}

uint64_t ostsu_treshold(std::vector<uint64_t> measures)
{

  // Prefilter to remove outlier
  std::sort(measures.begin(), measures.end());
  auto Q1 = findMedianSorted(measures, 0);
  auto Q3 = findMedianSorted(measures, measures.size() / 2);
  auto IQR = Q3 - Q1;

  std::vector<uint64_t>::iterator it = measures.begin();
  while (it != measures.end())
  {
    if (*it < Q1 - 1.5 * IQR || *it > Q3 + 1.5 * IQR)
    {
      it = measures.erase(it);
    }
    else
    {
      ++it;
    }
  }

  // Normalize remaining vector entries
  auto min = *measures.begin();
  auto max = *measures.end();
  for (size_t i = 0; i < measures.size(); i++)
  {
    measures[i] = ((measures[i] - min) * 255) / ((float)(max - min));
  }

  // Build a histogram for the vector
  std::vector<uint64_t> hist(255, 0);
  for (size_t i = 0; i < measures.size(); i++)
  {
    hist[measures[i]]++;
  }

  // Compute threshold
  // Init variables
  float sum = 0;
  float sumB = 0;
  int q1 = 0;
  int q2 = 0;
  float varMax = 0;
  int threshold = 0;

  // Auxiliary value for computing m2
  for (int i = 0; i <= 255; i++)
  {
    sum += i * ((int)hist[i]);
  }

  for (int i = 0; i <= 255; i++)
  {
    // Update q1
    q1 += hist[i];
    if (q1 == 0)
      continue;

    // Update q2
    q2 = measures.size() - q1;
    if (q2 == 0)
      break;

    // Update m1 and m2
    sumB += (float)(i * ((int)hist[i]));
    float m1 = sumB / q1;
    float m2 = (sum - sumB) / q2;

    // Update the between class variance
    float varBetween = (float)q1 * (float)q2 * (m1 - m2) * (m1 - m2);

    // Update the threshold if necessary
    if (varBetween > varMax)
    {
      varMax = varBetween;
      threshold = i;
    }
  }

  // Denormalize treshold to get usable value
  uint64_t denormalized_threshold = ((threshold * (max - min)) / 255) + min;

  return denormalized_threshold;
}

void filter_timings(std::vector<std::vector<uint64_t>> &measurement, int threshold)
{
  // Determine the indices that need to be removed
  std::vector<int> indicesToRemove;
  for (size_t i = 0; i < measurement.front().size(); ++i)
  {
    bool removeAtIndex = false;
    for (const auto &vec : measurement)
    {
      if (vec[i] > threshold)
      {
        removeAtIndex = true;
        break;
      }
    }
    if (removeAtIndex)
    {
      indicesToRemove.push_back(i);
    }
  }

  // Remove elements at the determined indices from all vectors
  for (auto &vec : measurement)
  {
    for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); ++it)
    {
      vec.erase(vec.begin() + *it);
    }
  }
}

typedef struct
{
  void *physical_address;
  void *virtual_address;
} address_t;

/**
 * @brief returns a randomly mapped address
 */
address_t get_random_address(size_t number_of_bytes)
{
  uint64_t *mem = (uint64_t *)mmap(0, number_of_bytes, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, 0, 0);
  // printf("virtual: %p, physical: %p\n", mem, (void *) random);

  address_t address;
  address.virtual_address = mem;
  address.physical_address = (void *)utils_get_physical_address((size_t)address.virtual_address);
  return address;
}

SliceTimingOracle::SliceTimingOracle(CoreConfig core_config)
{
  this->core_config = core_config;
  // std::cout << "Determining Treshold" << std::endl;
  // determine_treshold();
  // this->threshold = 268;
  // std::cout << "Treshold set to " << this->threshold << std::endl;
  // benchmark_cores();
  this->slices = core_config.num_slices;
  std::cout << core_config.slice_cores << std::endl;
}

SliceTimingOracle::~SliceTimingOracle()
{
}

/**
 * Returns the distance of the minumum value in the vector to the second lowest value
 */
uint64_t min_distance(std::vector<uint64_t> &v)
{
  std::vector<uint64_t> sorted = v;
  std::sort(sorted.begin(), sorted.end());
  return sorted[1] - sorted[0];
}

inline __attribute__((always_inline)) uint64_t measure_time(void* addr) {
    uint64_t a, b;
    
    asm volatile(
         "movq (%[addr]), %%rax\n"
         "clflush (%[addr])\n"
         "mfence\n"
         "cpuid\n"
         "rdtscp\n"
         "mov %%rax, %[a]\n"
         "mfence\n"
         "movq (%[addr]), %%rax\n"
         "mfence\n"
         "clflush (%[addr])\n"
         "mfence\n"
         "rdtsc\n"
         "mov %%rax, %[b]\n"
         : [a] "=r" (a), [b] "=r" (b)
         : [addr] "r" (addr)
         : "%rax", "%rbx", "%rcx", "%rdx"
    );
    
    return b - a; // TODO: overflow fix stuff
}

void SliceTimingOracle::record_timings(void *address, std::vector<std::vector<uint64_t>> &measurements, int repetitions)
{
  for (size_t i = 0; i < repetitions; i++)
  {
    for (size_t c = 0; c < this->core_config.num_slices; c++)
    {
      uint64_t measurement = 0;
      utils_pin_to_core(getpid(), this->core_config.get_core_for_slice(c));
      // asm volatile("movq (%0), %%rax\n" : : "r"(address) : "rax");
      // asm volatile(".rept 5000\nmovq (%0), %%rax\n.endr\n" : : "c"(address) : "rax");
      // asm volatile("mfence\n" : : :);
      // // asm volatile("movq (%0), %%rax\n" : : "r"(address) : "rax");
      // size_t start = __rdtsc_begin();
      // // asm volatile("clflush 0(%0)\n" : : "c"(address) : "rax");
      // asm volatile(".rept 1\nmovq (%0), %%rax\nclflush 0(%0)\n.endr\n" : : "c"(address) : "rax");
      // size_t end = __rdtsc_end();
      // measurement = end - start;
      measurement = measure_time(address);

      // measurements[c][i] = measurement;
      // Balance timings on efficiency cores with constant offset
      if (core_config.is_offset_slice(c))
      {
        measurement -= 100;
      }
      measurements[c][i] = measurement;
    }
  }
}

bool SliceTimingOracle::test_quality_timings(std::vector<std::vector<uint64_t>> &measurements)
{
  std::vector<uint64_t> dist_q3_q1;
  for (size_t i = 0; i < this->core_config.num_slices; i++)
  {
    std::vector<uint64_t> measures = measurements[i];
    std::sort(measures.begin(), measures.end());
    uint64_t Q1 = findMedianSorted(measures, -(measures.size() / 2));
    uint64_t Q3 = findMedianSorted(measures, measures.size() / 2);
    dist_q3_q1.push_back(Q3 - Q1);
    if (Q3 - Q1 > 35)
    {
      return false;
    }
  }
  return true;
}

void SliceTimingOracle::dump_timings(std::vector<std::vector<uint64_t>> &measurements, int slice)
{
  std::ofstream outputFile("oracle_data.txt", std::ios::app);
  for (size_t i = 0; i < this->slices; i++)
  {
    std::vector<uint64_t> measures = measurements[i];
    outputFile << measures << std::endl;
  }
  outputFile << slice << std::endl;
  outputFile.close();
}

uint64_t __attribute((aligned(4096))) SliceTimingOracle::oracle(void *addr)
{
  int repetitions = 30;
  std::vector<std::vector<uint64_t>> measurements(this->slices, std::vector<uint64_t>(repetitions, 0));
  bool good_quality = false;
  while (!good_quality)
  {
    record_timings(addr, measurements, repetitions);
    good_quality = test_quality_timings(measurements);
  }
  // filter_timings(measurements, 6000);
  std::vector<uint64_t> medians(this->slices, 0);
  for (size_t i = 0; i < this->slices; i++)
  {
    std::vector<uint64_t> measures = measurements[i];
    std::sort(measures.begin(), measures.end());
    uint64_t Q1 = findMedianSorted(measures, 0);
    medians[i] = Q1;
  }

  // std::cout << "Medians: " << medians << std::endl;
  return std::distance(medians.begin(), std::min_element(medians.begin(), medians.end()));
}

uint64_t SliceTimingOracle::oracle_min(void *addr)
{
  int repetitions = 20;
  std::vector<std::vector<uint64_t>> measurements(this->slices, std::vector<uint64_t>(repetitions, 0));
  bool good_quality = false;
  while (!good_quality)
  {
    record_timings(addr, measurements, repetitions);
    good_quality = true; // test_quality_timings(measurements);
  }
  dump_timings(measurements, 0);
  std::vector<uint64_t> mins(this->slices, 0);
  for (size_t i = 0; i < this->slices; i++)
  {
    std::vector<uint64_t> measures = measurements[i];
    uint64_t min = *std::min_element(measures.begin(), measures.end());
    mins[i] = min;
  }

  uint64_t min_idx = std::distance(mins.begin(), std::min_element(mins.begin(), mins.end()));
  // std::cout << "Mins: " << mins << " " << min_idx << std::endl;
  return min_idx;
}

class LogisticRegression
{
private:
  std::vector<double> coefficients;

public:
  LogisticRegression(const std::vector<double> &coeffs) : coefficients(coeffs) {}

  double sigmoid(double z)
  {
    // if (z >= 0)
    // {
      return 1.0 / (1.0 + exp(-z));
    // } else {
    //   return exp(z) / (1.0 + exp(z));
    // }
  }

  double predict(const std::vector<double> &features)
  {
    double z = 144.68081518;
    for (size_t i = 0; i < features.size(); ++i)
    {
      z += coefficients[i] * features[i];
    }
    return sigmoid(z);
  }
};

uint64_t SliceTimingOracle::oracle_lr(void *addr)
{
  // Coefficients obtained from sklearn logistic regression
  std::vector<double> coefficients = {-0.2930982, -0.00051657, -0.11951354};

  // Create the logistic regression model
  LogisticRegression model(coefficients);
  
  int repetitions = 100;
  std::vector<std::vector<uint64_t>> measurements(this->slices, std::vector<uint64_t>(repetitions, 0));
  bool good_quality = false;
  while (!good_quality)
  {
    record_timings(addr, measurements, repetitions);
    good_quality = true; // test_quality_timings(measurements);
  }
  std::vector<uint64_t> scores(this->slices, 0);
  for (size_t i = 0; i < this->slices; i++)
  {
    std::vector<uint64_t> measures = measurements[i];
    std::sort(measures.begin(), measures.end());
    uint64_t Q2 = findMedianSorted(measures, 0);
    uint64_t Q1 = findMedianSorted(measures, -(measures.size() / 2));
    uint64_t Q3 = findMedianSorted(measures, measures.size() / 2);
    std::vector<double> features = {(double) Q1, (double) Q2, (double) Q3};
    scores[i] = model.predict(features);
  }

  int best_index = std::distance(scores.begin(), std::max_element(scores.begin(), scores.end()));
  double best_score = scores[best_index];
  if (best_score < 0.2) 
  {
    return oracle_lr(addr);
  } else {
    return best_index;
  }
}

uint64_t SliceTimingOracle::oracle_dump(void *addr, int correct_slice)
{
  int repetitions = 100;
  std::vector<std::vector<uint64_t>> measurements(this->slices, std::vector<uint64_t>(repetitions, 0));
  bool good_quality = false;
  while (!good_quality)
  {
    record_timings(addr, measurements, repetitions);
    good_quality = test_quality_timings(measurements);
  }
  dump_timings(measurements, correct_slice);
  std::vector<uint64_t> medians(this->slices, 0);
  for (size_t i = 0; i < this->slices; i++)
  {
    std::vector<uint64_t> measures = measurements[i];
    std::sort(measures.begin(), measures.end());
    uint64_t Q1 = findMedianSorted(measures, 0);
    medians[i] = Q1;
  }

  std::cout << "Medians: " << medians << std::endl;
  return std::distance(medians.begin(), std::min_element(medians.begin(), medians.end()));
}

uint64_t SliceTimingOracle::oracle_mean(void *addr, int correct_slice)
{
  int repetitions = 400;
  std::vector<std::vector<uint64_t>> measurements(this->slices, std::vector<uint64_t>(repetitions, 0));
  bool good_quality = false;
  while (!good_quality)
  {
    record_timings(addr, measurements, repetitions);
    good_quality = true; // test_quality_timings(measurements);
  }
  dump_timings(measurements, correct_slice);
  filter_timings(measurements, 500);
  std::vector<float> means(this->slices, 0);
  for (size_t i = 0; i < this->slices; i++)
  {
    float mean = (float)std::accumulate(measurements[i].begin(), measurements[i].end(), 0.0) / (float)measurements[i].size();
    means[i] = mean;
  }

  // std::cout << "Means: " << means << std::endl;
  return std::distance(means.begin(), std::min_element(means.begin(), means.end()));
}

uint64_t SliceTimingOracle::oracle_info(void *addr, int correct_slice)
{
  std::ofstream outputFile("oracle_data.txt", std::ios::app);
  int repetitions = 3000;
  std::vector<std::vector<uint64_t>> measurements(this->slices, std::vector<uint64_t>(3000, 0));
  std::vector<uint64_t> medians(this->slices, 0);
  record_timings(addr, measurements, repetitions);
  for (size_t i = 0; i < this->slices; i++)
  {
    std::vector<uint64_t> measures = measurements[i];
    outputFile << measures << std::endl;
    std::sort(measures.begin(), measures.end());
    uint64_t Q1 = findMedianSorted(measures, 0);
    // uint64_t Q1 = measures[10];
    medians[i] = Q1;
    // std::cout << measures << std::endl;
  }
  outputFile << correct_slice << std::endl;

  int measured_slice = std::distance(medians.begin(), std::min_element(medians.begin(), medians.end()));
  if (measured_slice == correct_slice)
  {
    std::cout << TAG_OK;
  }
  else
  {
    std::cout << TAG_FAIL;
  }
  std::cout << "Medians: " << medians << "->" << measured_slice << "~" << correct_slice << std::endl;

  // Close the file
  outputFile.close();

  return measured_slice;
}

uint64_t SliceTimingOracle::confident_oracle (void* addr)
{
  int samples = 10;
  std::vector<int> hist(this->slices, 0);
  for(size_t i = 0; i < samples; i++)
  {
    int current_slice = this->oracle_lr(addr);
    hist[current_slice]++;
  }
  int max_index = std::distance(hist.begin(), std::max_element(hist.begin(), hist.end()));
  if (hist[max_index] >= samples)
  {
    return max_index;
  } else {
    std::cout << "#" << std::flush;
    return confident_oracle(addr);
  }
}

void SliceTimingOracle::determine_treshold()
{
  std::vector<uint64_t> measurements;
  for (size_t i = 0; i < 100000; i++)
  {
    address_t rand_page = get_random_address(4096);
    void *addr = (void *)(rand_page.virtual_address);
    for (size_t c = 0; c < this->slices; c++)
    {
      utils_pin_to_core(getpid(), c);
      asm volatile("movq (%0), %%rax\n" : : "r"(addr) : "rax");
      asm volatile("clflush 0(%0)\n" : : "c"(addr) : "rax");
      size_t start = __rdtsc_begin();
      asm volatile("movq (%0), %%rax\n" : : "r"(addr) : "rax");
      asm volatile("clflush 0(%0)\n" : : "c"(addr) : "rax");
      size_t end = __rdtsc_end();
      measurements.push_back(end - start);
    }
    munmap(rand_page.virtual_address, 4096);
  }
  this->threshold = ostsu_treshold(measurements);
}

double calculate_mean(const std::vector<uint64_t> &vec)
{
  if (vec.empty())
    return 0.0; // handle empty vector case
  uint64_t sum = std::accumulate(vec.begin(), vec.end(), uint64_t(0));
  return static_cast<double>(sum) / vec.size();
}

void SliceTimingOracle::benchmark_cores()
{
  int repetitions = 1000;
  std::vector<std::vector<uint64_t>> measurements(20, std::vector<uint64_t>(repetitions, 0));
  for (size_t i = 0; i < repetitions; i++)
  {
    address_t rand_page = get_random_address(4096);
    void *addr = (void *)(rand_page.virtual_address);
    for (size_t c = 0; c < 20; c++)
    {
      utils_pin_to_core(getpid(), c);
      asm volatile("movq (%0), %%rax\n" : : "r"(addr) : "rax");
      asm volatile("clflush 0(%0)\n" : : "c"(addr) : "rax");
      size_t start = __rdtsc_begin();
      asm volatile("movq (%0), %%rax\n" : : "r"(addr) : "rax");
      asm volatile("clflush 0(%0)\n" : : "c"(addr) : "rax");
      size_t end = __rdtsc_end();
      measurements[c][i] = end - start;
    }
    munmap(rand_page.virtual_address, 4096);
  }
  std::vector<uint64_t> p_cores;
  std::vector<uint64_t> e_cores;
  for (size_t i = 0; i < 20; i++)
  {
    std::vector<uint64_t> measures = measurements[i];
    std::sort(measures.begin(), measures.end());
    uint64_t Q1 = findMedianSorted(measures, 0);
    uint64_t mean = calculate_mean(measures);
    std::cout << "Core " << i << "\tMedian: " << Q1 << " Mean: " << mean << std::endl;
    if (i < 12)
    {
      p_cores.push_back(mean);
    }
    else
    {
      e_cores.push_back(mean);
    }
  }
  uint64_t p_mean = calculate_mean(p_cores);
  uint64_t e_mean = calculate_mean(e_cores);
  std::cout << "Physical Cores Mean: " << p_mean << std::endl;
  std::cout << "Efficiency Cores Mean: " << e_mean << std::endl;
}
