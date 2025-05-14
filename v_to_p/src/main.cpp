#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> // mmap
#include <unistd.h>

#include "utils.h"
#include "translation-oracle.hpp"
#include "pretty-print.hpp"
#include "core-config.hpp"

/**
 * Tests if two virtual addresses are physically consecutive
 */
int is_physically_consecutive(void *virtual_1, void *virtual_2,
                              uint64_t page_size) {
  uint64_t physical_1 = (uint64_t)utils_get_physical_address((size_t)virtual_1);
  uint64_t physical_2 = (uint64_t)utils_get_physical_address((size_t)virtual_2);

  // printf("virt1: %p, virt2: %p\n", virtual_1, virtual_2);
  // printf("phys1: %p, phys2: %p\n", physical_1, physical_2);

  return physical_2 == (physical_1 + page_size);
}

/**
 * @brief Tests if the memory region is consecutive
 *
 * @param mem
 * @param mapping_length
 * @param page_size
 * @return True if all consecutive or False otherwise

*/
int is_consecutive(void *mem, uint64_t mapping_length, uint64_t page_size) {
  int consecutive_ctr = 1;
  int required_pages = (mapping_length) / page_size;
  for (size_t i = 0; i < required_pages - 1; ++i) {
    void *current_address = mem + i * page_size;
    int has_consecutive_successor = is_physically_consecutive(
        current_address, current_address + page_size, page_size);
    if (!has_consecutive_successor) {
      // printf("Address: %p\n", current_address);
      std::cout << TAG_FAIL << "Found " << consecutive_ctr << "/"
                << required_pages << " consecutive pages" << std::endl;
      return 0;
    }
    consecutive_ctr++;
  }
  std::cout << TAG_OK << "Found " << std::setfill(' ') << std::setw(6)
            << consecutive_ctr << " consecutive pages" << std::endl;
  return 1;
}

/**
 * @brief Searches for consecutive memory regions and prints the result
 *
 * @param mem
 * @param mapping_length
 * @param page_size
 */
void search_consecutive(void *mem, uint64_t mapping_length,
                        uint64_t page_size) {
  int consecutive_ctr = 1;
  void *start = mem;

  for (size_t i = 0; i < (mapping_length - 1) / page_size; ++i) {
    void *current_address = mem + i * page_size;
    int has_consecutive_successor = is_physically_consecutive(
        current_address, current_address + page_size, page_size);
    if (!has_consecutive_successor) {
      if (consecutive_ctr > 100) {
        std::cout << TAG_OK << "Found " << std::setfill(' ') << std::setw(6)
                  << consecutive_ctr << " cons. pages: " << start << " - "
                  << current_address << std::endl;
      }
      start = current_address + page_size;
      consecutive_ctr = 1;
    } else {
      consecutive_ctr++;
    }
  }
  std::cout << TAG_OK << "Found " << std::setfill(' ') << std::setw(6)
            << consecutive_ctr << " cons. pages: " << start << std::endl;
}

/**
 * @brief Heuristic to find a memory region that is likely to be consecutive
 *
 * @param mem The start of the memory region
 * @param mapping_length The length of the memory region in bytes
 * @param page_size The page size in bytes
 * @param needed_memory Needed amout of memory in bytes
 * @return Pointer to the start of the memory region
 */
void *memory_heuristic(void *mem, uint64_t mapping_length, uint64_t page_size,
                       uint64_t needed_memory) {
  return mem + mapping_length - (0x1 << 8) * page_size - needed_memory;
}

void *memory_heuristic2(void *mem, uint64_t mapping_length, uint64_t page_size,
                        uint64_t needed_memory) {
  u_int64_t aligned_end =
      ((uint64_t)mem + mapping_length - needed_memory) >> 24 << 24;
  return (void *)aligned_end;
}

int main() {
  if (geteuid()) {
    std::cout << TAG_FAIL << "Running as root?\n";
    exit(1);
  }

  // Define the parameters
  uint64_t mapping_length = 0x1UL << 30;   // 1GB
  uint64_t page_size = 0x1UL << 12;        // 4KB
  uint64_t pattern_length = 0x1UL << 16;   // in bytes
  uint64_t available_memory = 0x1UL << 33; // 8GB

  CoreConfig core_config = CoreConfig(6);

  // Create the oracle to translate virtual to physical addresses
  TranslationOracle oracle(core_config, available_memory);

  // Map the test memory
  void *mem = mmap(0, mapping_length, PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, 0, 0);

  // Search for consecutive memory region
  search_consecutive(mem, mapping_length, page_size);
  void *memory_candidate =
      memory_heuristic2(mem, mapping_length, page_size, pattern_length);
  std::cout << TAG_OK << "Memory candidate: " << memory_candidate << std::endl;

  // Test if the memory region is consecutive
  if (is_consecutive(memory_candidate, pattern_length, page_size)) {
    // Translate the virtual address to a physical address
    void *physical_address = oracle.oracle(memory_candidate, pattern_length);
  } else {
    std::cout << TAG_FAIL << "Not enough consecutive memory :/" << std::endl;
  }

  // Unmap the test memory
  int unmap_success = munmap(mem, mapping_length);
  if (unmap_success == 0) {
    std::cout << TAG_OK << "Unmapping successful" << std::endl;
  } else {
    std::cout << TAG_FAIL << "Unmapping Error" << std::endl;
  }

  return 0;
}
