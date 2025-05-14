#include "translation-oracle.hpp"

#include <iostream>
#include <fstream>
#include <map>
#include <chrono>
#include <unordered_map>

#include "functions.h"
#include "slice-timing-oracle.hpp"
#include "pretty-print.hpp"
#include "utils.h"

typedef struct
{
    void *physical_address;
    void *virtual_address;
} address_t;

std::map<uint64_t, uint64_t>
    hist_map;

/**
 * Checks if the slice that was measured for an address is correct.
 *
 * @param address The address
 * @param measured_slice The measured slice to check
 * @return True if the slice is correct, false otherwise.
 */
bool is_slice_correct(address_t address, int measured_slice)
{
    // int correct_slice = compute_slice_4_core((uint64_t)address.physical_address);
    int correct_slice = compute_slice((uint64_t)address.physical_address);
    // printf("%p aka %p at slice %d (correct slice %d)", address.virtual_address, address.physical_address, measured_slice, correct_slice);
    if (measured_slice != correct_slice)
    {
        // std::cout << " === Slice incorrect ===\n");
        return false;
    }
    else
    {
        // std::cout << "\n");
        return true;
    }
}

int verify_pattern(size_t pfn, std::vector<int> &pattern, int fault_tolerance)
{
    uint64_t physical_address = pfn << 12;
    uint64_t current_address;
    uint64_t fault_counter = 0;

    for (int i = 0; i < pattern.size(); i++)
    {
        current_address = physical_address + i * (0x1 << 6);
        // int current_slice = compute_slice_coffee_lake(current_address);
        int current_slice = compute_slice(current_address);
        int is_equal = pattern[i] == current_slice;
        // std::cout << "Physical addr: %p, expected slice: %d, measured slice %d, i: %d\n", current_address, current_slice, pattern[i], i);
        if (!is_equal)
        {
            hist_map[i]++;
            fault_counter++;
            if (fault_counter > fault_tolerance)
            {
                return 0;
            }
        }
    }

    return 1;
}

int in_suspected_range(void *address)
{
    uint64_t address_n = (uint64_t)address;
    return address_n > 0x100000000;
}

TranslationOracle::TranslationOracle(CoreConfig core_config, uint64_t memory_size) : slice_oracle(core_config)
{
    this->core_config = core_config;
    this->memory_size = memory_size;
}

TranslationOracle::~TranslationOracle()
{
}

uint64_t TranslationOracle::count_faults(size_t pfn, std::vector<int> &pattern)
{
    uint64_t physical_address = pfn << 12;
    uint64_t current_address;
    uint64_t fault_counter = 0;

    for (int i = 0; i < pattern.size(); i++)
    {
        current_address = physical_address + i * (0x1 << 6);
        // int current_slice = compute_slice_coffee_lake(current_address);
        int current_slice = compute_slice(current_address);
        int is_equal = pattern[i] == current_slice;
        // std::cout << "Physical addr: %p, expected slice: %d, measured slice %d, i: %d\n", current_address, current_slice, pattern[i], i);
        if (!is_equal)
        {
            fault_counter++;
        }
    }

    return fault_counter;
}

void __attribute__((aligned(4096))) TranslationOracle::record_pattern(void *address, std::vector<int> &pattern, int pattern_length)
{
    // Get the start time
    auto start = std::chrono::high_resolution_clock::now();

    std::ofstream outputFile("slice_data.txt");
    // Check if the file is opened successfully
    if (!outputFile.is_open())
    {
        throw std::runtime_error("Error opening the file for writing!");
    }

    int last_perc = 0;
    bool error = false;
    int fault_ctr = 0;
    printProgress(0);
    for (int i = 0; i < pattern_length; i++)
    {
        int perc = (100 * i) / pattern_length;
        if (last_perc != perc)
        {
            printProgress(perc);
            last_perc = perc;
            error = false;
        }
        address_t current_address;
        current_address.virtual_address = address + i * (0x1 << 6);
        current_address.physical_address = (void *)(utils_get_physical_address((size_t)current_address.virtual_address));
        int correct_slice = compute_slice((uint64_t)current_address.physical_address);
        int current_slice = this->slice_oracle.oracle(current_address.virtual_address);
        if (current_slice != correct_slice)
        {
            fault_ctr++;
            error = true;
        }
        pattern[i] = current_slice;
    }
    printProgress(100);

    // Close the file
    outputFile.close();

    // Get the end time
    auto end = std::chrono::high_resolution_clock::now();
    // Calculate the duration
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    std::cout << TAG_OK << "Took " << duration << " seconds, " << fault_ctr << "faults" << std::endl;
}

uint64_t TranslationOracle::search_pfn(std::vector<int> &pattern)
{
    // Get the start time
    auto start = std::chrono::high_resolution_clock::now();

    // Search the pfn in the physical address space
    uint64_t number_of_candidates = 0;
    uint64_t number_of_good_candidates = 0;
    uint64_t upper_limit = this->memory_size >> 12;
    int last_perc = 0;
    printProgress(0);
    for (uint64_t current_pfn = 0; current_pfn < upper_limit; ++current_pfn)
    {
        int perc = (100 * current_pfn) / upper_limit;
        if (last_perc != perc)
        {
            printProgress(perc);
            last_perc = perc;
        }
        int current_pattern_correct = verify_pattern(current_pfn, pattern, 0);
        if (current_pattern_correct)
        {
            void *physical_address = (void *)(current_pfn << 12);
            number_of_candidates += 1;
            if (in_suspected_range(physical_address))
            {
                number_of_good_candidates += 1;
                // std::cout << TAG_OK << "Pattern correct for physical address: " << physical_address << std::endl;
            }
        }
    }
    printProgress(100);
    std::cout << TAG_OK << "Number of candidates: " << number_of_candidates << std::endl;
    std::cout << TAG_OK << "Number of good candidates: " << number_of_good_candidates << std::endl;

    // Get the end time
    auto end = std::chrono::high_resolution_clock::now();
    // Calculate the duration
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    std::cout << TAG_OK << "Took " << duration << " seconds" << std::endl;
    return 0;
}

int best_of_three(int a, int b, int c)
{
    if (a == b || a == c) {
        return a;
    } else {
        return b;
    }
}



void *TranslationOracle::oracle(void *address, uint64_t pattern_length_bytes)
{
    address_t random_address;
    random_address.virtual_address = address;
    random_address.physical_address = (void *)utils_get_physical_address((size_t)random_address.virtual_address);
    uint64_t pattern_length = pattern_length_bytes >> 6; // Number of cache lines in the pattern region

    int correct_slice = compute_slice((uint64_t)random_address.physical_address);

    // Test the slice addressing function and oracle
    center_print("Test function");

    // Warmup
    for (int i = 0; i < 100; i++)
    {
        this->slice_oracle.oracle(address);
    }

    // Get the start time
    auto start = std::chrono::high_resolution_clock::now();

    int slice = this->slice_oracle.oracle(random_address.virtual_address);

    // Get the end time
    auto end = std::chrono::high_resolution_clock::now();
    // Calculate the duration
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (slice == correct_slice)
    {
        std::cout << TAG_OK << "virtual: " << random_address.virtual_address << ", physical: " << random_address.physical_address;
        std::cout << ", slice: " << slice << "~" << correct_slice;
        std::cout << " correct" << std::endl;
    }
    else
    {
        std::cout << TAG_FAIL << "virtual: " << random_address.virtual_address << ", physical: " << random_address.physical_address;
        std::cout << ", slice: " << slice << "~" << correct_slice;
        std::cout << " wrong" << std::endl;
    }
    std::cout << TAG_OK << "Took " << duration << " ms" << std::endl;
    auto estimated_duration = std::chrono::duration_cast<std::chrono::minutes>((end - start) * pattern_length);
    // std::cout << TAG_OK << "Estimated waiting time: " << estimated_duration << std::endl;

    // Record the slice mapping pattern
    center_print("Record slice access pattern");
    std::vector<std::vector<int>> patterns;
    for (int i = 0; i < 5; i++)
    {
        std::vector<int> pattern(pattern_length);
        this->record_pattern(address, pattern, pattern_length);
        patterns.push_back(pattern);
    }

    std::vector<int> noise_free_pattern(pattern_length);
    for (size_t i = 0; i < pattern_length; ++i) {
        std::unordered_map<int, int> count_map;
        for (const auto& pattern : patterns) {
            count_map[pattern[i]]++;
        }

        // Find the majority value
        int majority_value = patterns[0][i];
        int max_count = 0;
        for (const auto& pair : count_map) {
            if (pair.second > max_count) {
                max_count = pair.second;
                majority_value = pair.first;
            }
        }
        noise_free_pattern[i] = majority_value;
    }


    // Verify the measurements
    center_print("Verify slice access pattern");
    uint64_t pfn = ((uint64_t)random_address.physical_address) >> 12;
    uint64_t fault_counter = count_faults(pfn, noise_free_pattern);
    int fault_percentage = (100 * fault_counter) / pattern_length;
    std::cout << TAG_OK << "Faults: " << fault_counter << "/" << pattern_length << " (" << ORANGE << fault_percentage << "%" << RESET << ")" << std::endl;
    int pattern_correct = verify_pattern(pfn, noise_free_pattern, 0);
    if (pattern_correct)
    {
        std::cout << TAG_OK << "Pattern suits for physical address " << random_address.physical_address << std::endl;
    }
    else
    {
        std::cout << TAG_FAIL << "Pattern does not suit for physical address " << random_address.physical_address << std::endl;
    }

    // Search the pfn in the physical address space
    center_print("Search pfn");
    uint64_t pfn_estimate = this->search_pfn(noise_free_pattern);

    // Print the locations where the pattern started to differ
    //  for (const auto &pair : hist_map)
    //  {
    //      std::cout << TAG_OK << pair.first << "\t| " << pair.second << std::endl;
    //  }
    return address;
}
