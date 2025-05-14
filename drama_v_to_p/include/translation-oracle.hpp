#ifndef _TRANSLATION_ORACLE_H_
#define _TRANSLATION_ORACLE_H_

#include <cstdint>

#include "slice-timing-oracle.hpp"
#include "core-config.hpp"

class TranslationOracle
{
private:
    /**
     * The number of physical slices in the system
    */
    CoreConfig core_config;

    /**
     * The total amount of memory in the system
     */
    uint64_t memory_size;

    /**
     * The slice timing oracle
     */
    SliceTimingOracle slice_oracle;

    /**
     * Search the page frame number (pfn) with the given pattern in the physical address space
     *
     * @param pattern The slice access pattern to search for
     * @param pattern_length The length of the pattern in cache lines (64 bytes)
     * @return The found pfn
     */
    void *search_pfn(int *pattern, int pattern_length);

    /**
     * Count the number of faults for the given pfn and pattern
     */
    uint64_t count_faults(size_t pfn, int *pattern, int pattern_length);

    /**
     * Record a slice access pattern for the given address
     *
     * @param address The address to record the pattern for
     * @param pattern The pattern memory
     * @param pattern_length The length of the pattern in cache lines (64 bytes)
     */
    void record_pattern(void *address, int *pattern, int pattern_length);

public:
    /**
     * @brief Construct a new Translation Oracle object
     *
     * @param slices The number of slices in the L3 cache
     * @param memory_size The total amount of memory in the system
     */
    TranslationOracle(CoreConfig core_config, uint64_t memory_size);

    /**
     * @brief Destroy the Translation Oracle object
     */
    ~TranslationOracle();

    /**
     * @brief Provides a timing-based oracle for the translation of virtual to physical addresses
     *
     * @param address The virtual address to translate
     * @param pattern_length_bytes The length of the pattern to use in the oracle in bytes
     */
    void *oracle(void *address, uint64_t pattern_length_bytes);
};

#endif
