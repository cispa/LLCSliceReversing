#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// amount of pages the victim allocated
#define PAGES2 30

// amount of times the experiment is repeated
#define REPEATS 100

#define PAGE_SIZE 4096

// size of shared memory. This is just there to provide additional information.
#define SHMEM_SIZE 4096

static int self_maps_fd;
static int shmem_fd;

static uint64_t *shared_mem;

static void init() {
  self_maps_fd = open("/proc/self/pagemap", 0, O_RDONLY);
  shmem_fd = open("shmem", O_RDWR);
  shared_mem =
      mmap(NULL, SHMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmem_fd, 0);
}

static uint64_t virt2phys(void *virt) {
  uintptr_t pfn;
  lseek(self_maps_fd, 8ull * (((uintptr_t)virt) >> 12), SEEK_SET);
  read(self_maps_fd, &pfn, 8);
  return ((pfn & 0x7FFFFFFFFFFFFFull) << 12ull) |
         (((uintptr_t)virt) & 0xfffull);
}

// adjust this if you want to use the oracle instead
#define virt2phys_attack virt2phys

_Static_assert(SHMEM_SIZE >= (PAGES2 + 4095) / 256,
               "shared memory is too small to store the results");

#endif /* COMMON_H */
