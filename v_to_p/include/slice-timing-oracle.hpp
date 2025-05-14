#ifndef _SLICE_TIMING_ORACLE_H_
#define _SLICE_TIMING_ORACLE_H_

#include <cstdint>
#include <vector>

#include "core-config.hpp"

class SliceTimingOracle
{
private:
    /**
     * The threshold for the timing-based oracle
    */
    size_t threshold;

    /**
     * The slice timing oracle
     */
    CoreConfig core_config;

    int slices;

    std::vector<int> slice_cores;
    std::vector<int> offset_cores;

    /**
     * @brief Determines the threshold for the timing-based oracle
    */
    void determine_treshold();

    void dump_timings(std::vector<std::vector<uint64_t>> &measurements, int slice);

public:
    /**
     * @brief Construct a new Slice Timing Oracle object
    */
    SliceTimingOracle(CoreConfig core_config);

    /**
     * @brief Destroy the Slice Timing Oracle object
    */
    ~SliceTimingOracle();
    
    void record_timings(void * address, std::vector<std::vector<uint64_t>> &measurements, int repetitions);
    bool test_quality_timings(std::vector<std::vector<uint64_t>> &measurements);

    /**
     * @brief Provides a timing-based oracle to get the cache slice that a virtual address maps to
     * 
     * @param addr The virtual address
     * @return The cache slice that the virtual address maps to
    */
    uint64_t oracle(void *addr);

    uint64_t oracle_mean(void *addr, int correct_slice);
    
    uint64_t oracle_info(void *addr, int correct_slice);
    
    uint64_t oracle_dump(void *addr, int correct_slice);

    uint64_t oracle_min(void *addr);

    void benchmark_cores();

    uint64_t oracle_lr(void *addr);

    uint64_t confident_oracle (void* addr);
};

#endif
