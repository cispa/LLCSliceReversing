#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>

#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>
#include <limits.h>
#include <string.h>

// #include "libsc.h"
#include "utils.h"
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../slice_functions/slice_functions.h"

#define MEMSIZE (1 * 1024ul * 1024ul * 1024ul)
#define ADDRESSES 100

#define BASE_PATH "/sys/bus/event_source/devices/"
#define CBOX_PREFIX "uncore_cbox_"
#define CHA_PREFIX "uncore_cha_"
#define TYPE_FILE "/type"

int get_slice_info(const char *prefix, char *type_value) {
  DIR *dir;
  struct dirent *entry;
  int slices = -1;
  char path[PATH_MAX];

  dir = opendir(BASE_PATH);
  if (!dir) {
    perror("opendir");
    return -1;
  }

  while ((entry = readdir(dir)) != NULL) {
    // printf("%s | %d == %d | %d\n", entry->d_name, entry->d_type, DT_DIR,
    // strncmp(entry->d_name, prefix, strlen(prefix)));
    if (/*entry->d_type == DT_DIR && */ strncmp(entry->d_name, prefix,
                                                strlen(prefix)) == 0) {
      int X = atoi(entry->d_name + strlen(prefix));
      if (X > slices) {
        slices = X;
      }

      if (X == 0) {
        snprintf(path, sizeof(path), "%s%s%d%s", BASE_PATH, prefix, X,
                 TYPE_FILE);
        FILE *type_file = fopen(path, "r");
        if (type_file) {
          if (fgets(type_value, 10, type_file) != NULL) {
            type_value[strcspn(type_value, "\n")] = '\0';
          }
          fclose(type_file);
        } else {
          perror("fopen");
        }
      }
    }
  }
  closedir(dir);
  return slices + 1;
}

// ---------------------------------------------------------------------------
size_t find_slice_perf(void *address, int repeat, int *slice_count,
                       unsigned long *config, int *base) {
#define REP4(x) x x x x
#define REP16(x) REP4(x) REP4(x) REP4(x) REP4(x)
#define REP256(x)                                                              \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x) REP16(x) REP16(x) REP16(x) REP16(x) REP16(x) REP16(x) REP16(x)
#define REP1K(x) REP256(x) REP256(x) REP256(x) REP256(x)
#define REP4K(x) REP1K(x) REP1K(x) REP1K(x) REP1K(x)

  size_t hist[256];
  memset(hist, 0, sizeof(hist));

  int fds[256];
  size_t ev_hist[256];

  char type_value[10] = {0};
  int slices = get_slice_info(CBOX_PREFIX, type_value);
  // printf("%d\n", slices);

  if (slices > 0) {
    printf("Found %d uncore CBoxes | Perf type: %s\n", slices, type_value);
  } else {
    slices = get_slice_info(CHA_PREFIX, type_value);
    if (slices >= 0) {
      printf("Found %d uncore CBoxes | Perf type: %s\n", slices, type_value);
    } else {
      printf("Neither uncore_cbox_0 nor uncore_cha_0 found.\n");
      return 1;
    }
  }

  int did_find = 0;
  void *start_address = address;
  int mask = 0x7;

  while (!did_find && mask < 0xff) {
    address = start_address;
    mask = (mask << 1) | 0x1;
    memset(ev_hist, 0, sizeof(ev_hist));

    for (int i = 0; i < repeat; i++) {
      // printf("Round %d / %d\n", i + 1, repeat);
      for (int ev = 0; ev < 0xff; ev++) {
        unsigned long config = mask * 256 + ev;
        long long sum = 0;
        int qualify = 0;
        for (int i = 0; i < slices; i++) {
          // printf(":Open %d | %zd\n", atoi(type_value) + i, config);
          fds[i] = utils_event_open(atoi(type_value) + i, config, 0, 0, 0, i);
          if (fds[i] < 0)
            break;
          ioctl(fds[i], PERF_EVENT_IOC_ENABLE, 0);
          ioctl(fds[i], PERF_EVENT_IOC_RESET, 0);

          REP4K(asm volatile("mfence; clflush (%0); mfence; \n" : : "r"(
              address) : "memory");
                *(volatile char *)address;)

          ioctl(fds[i], PERF_EVENT_IOC_DISABLE, 0);

          long long result = 0;
          read(fds[i], &result, sizeof(result));
          hist[i] = result;
          close(fds[i]);
          // printf("CPU %d: %zd\n", i, result);
          if (result >= 4096 && result < 8000) {
            // printf("Maybe event %x (%zd)\n", ev, result);
            qualify = 1;
          }
          sum += result;
        }
        if (sum >= 4096 && sum < 8000 && qualify) {
          // printf("-> Could be 0x%zx (%lld)\n", config, sum);
          ev_hist[ev]++;
        }
      }
      address = (void *)((size_t)address + 64);
    }
    int potential = utils_find_index_of_nth_largest_size_t(ev_hist, 256, 0);
    // printf("Check [0x%zx]: %zd ?= %d\n", potential + mask * 256,
    // ev_hist[potential], repeat);
    if (ev_hist[potential] == repeat) {
      did_find = 1;
    }
  }

  *config =
      utils_find_index_of_nth_largest_size_t(ev_hist, 256, 0) + mask * 256;
  *base = atoi(type_value);
  *slice_count = slices;
  return 0;
}

size_t measure_slice_perf(void *address, int slices, unsigned long config,
                          int type) {
#define REP4(x) x x x x
#define REP16(x) REP4(x) REP4(x) REP4(x) REP4(x)
#define REP256(x)                                                              \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x)                                                                     \
  REP16(x) REP16(x) REP16(x) REP16(x) REP16(x) REP16(x) REP16(x) REP16(x)
#define REP1K(x) REP256(x) REP256(x) REP256(x) REP256(x)
#define REP4K(x) REP1K(x) REP1K(x) REP1K(x) REP1K(x)

  size_t hist[256];
  memset(hist, 0, sizeof(hist));

  int fds[256];
  // printf("%d\n", *(volatile char*)address);

  // printf("\nAddress: 0x%zx\n", address);
  for (int i = 0; i < slices; i++) {
    // printf("Open %d | %zd\n", type + i, config);
    fds[i] = utils_event_open(type + i, config, 0, 0, 0, i);
    if (fds[i] < 0)
      break;
    ioctl(fds[i], PERF_EVENT_IOC_ENABLE, 0);
    ioctl(fds[i], PERF_EVENT_IOC_RESET, 0);

    REP4K(asm volatile(
              "mfence; clflush (%0); mfence; \n" : : "r"(address) : "memory");
          *(volatile char *)address;)
    REP4K(asm volatile(
              "mfence; clflush (%0); mfence; \n" : : "r"(address) : "memory");
          *(volatile char *)address;)

    ioctl(fds[i], PERF_EVENT_IOC_DISABLE, 0);

    long long result = 0;
    read(fds[i], &result, sizeof(result));
    hist[i] = result;
    // printf("Slice %d: %zd\n", i, result);
    close(fds[i]);
  }

  int idx = utils_find_index_of_nth_largest_size_t(hist, slices, 0);
  if (hist[idx] < 8192) {
    printf("Expected >= 8192, got %zd\n", hist[idx]);
    return -1;
  }
  return idx;
}

int log_2(uint64_t x) {
  int result = 0;
  if (x == 0)
    return 0;
  x -= 1;
  while (x > 0) {
    x >>= 1;
    result++;
  }
  return result;
}

char __attribute__((aligned(4096))) data[4096 * 1024];

int main(int argc, char *argv[]) {
  memset(data, 1, sizeof(data));

  unsigned long config;
  int perf_base, slices;
  printf(
      "Looking for performance counter to use, this might take a while...\n");
  // slices = 36;
  // perf_base = 24;
  // config = 0x1bc10000ff34; // NOTE: ice lake server can't be brute forced at
  // the moment, as they have the 0x1bc1 extended umask. use this config for ice
  // lake
  int perf = find_slice_perf(data, 3, &slices, &config, &perf_base);

  printf("%d slices, base 0x%x, event 0x%zx\n", slices, perf_base, config);

  char *test = mmap(0, MEMSIZE, PROT_READ | PROT_WRITE,
                    MAP_ANON | MAP_PRIVATE | MAP_POPULATE, -1, 0);

  int chains = sizeof(linear_functions) / sizeof(linear_functions[0]);
  int bits = log_2(slices);
  printf("Expect %d bits\n", bits);
  printf("%d known XOR chains\n", chains);
  int mixer = sizeof(mixers) / sizeof(mixers[0]);
  printf("%d known chain mixer\n", mixer);
  // for(int i = 0; i < chains; i++) {
  //     libslice_print_chain(linear_functions[i]);
  // }

  int *correct = malloc(chains * bits * sizeof(int));
  memset(correct, 0, chains * bits * sizeof(int));
  int *correct_m = malloc(mixer * bits * sizeof(int));
  memset(correct_m, 0, mixer * bits * sizeof(int));

  printf("Measuring %d addresses...\n", ADDRESSES);
  double last_perc = 0;
  for (int i = 0; i < ADDRESSES; i++) {
    double perc = i * 100.0 / ADDRESSES;
    if (perc != last_perc) {
      last_perc = perc;
      printf("\r%3.2f%%  ", perc);
    }
    int slice, slice_c;
    int tries = 0;
    size_t ra, pa;
    do {
      ra = (size_t)test + (rand() % (MEMSIZE / 64)) * 64;
      pa = utils_get_physical_address(ra);
      slice = (int)measure_slice_perf((void *)ra, slices, config, perf_base);
      slice_c =
          (int)measure_slice_perf((void *)ra + 32, slices, config, perf_base);
      tries++;
    } while ((slice != slice_c || pa == 0 || slice == -1) && tries < 10);
    if (tries == 10) {
      printf("Something is weird, exiting...\n");
      exit(1);
    }
    for (int c = 0; c < chains; c++) {
      int x = linear_functions[c].func(pa);
      for (int b = 0; b < bits; b++) {
        if (x == ((slice >> b) & 1))
          correct[c * bits + b]++;
      }
    }
    for (int m = 0; m < mixer; m++) {
      int x = mixers[m].function(pa);
      for (int b = 0; b < bits; b++) {
        if (x == ((slice >> b) & ((1 << mixers[m].bits) - 1))) {
          correct_m[m * bits + b]++;
        }
      }
    }
  }
  printf("Done!\n");

  int next_is_mixer = 0, mixer_idx = 0;
  for (int b = 0; b < bits; b++) {
    printf("bit %d: ", b);
    double highest_perc = 0;
    int idx = 0;
    if (!next_is_mixer) {
      for (int c = 0; c < chains; c++) {
        double perc =
            correct[c * bits + b] * 100.0 /
            ADDRESSES; /// (correct[c * bits + b] + wrong[c * bits + b]);
        if (perc < 50)
          perc = 100 - perc;
        // printf("%3.1f  ", perc);
        if (perc > highest_perc) {
          highest_perc = perc;
          idx = c;
        }
      }
    }
    printf(" [%5.1f%%] ", highest_perc);
    if (highest_perc > 97) {
      printf("  --> %s\n", linear_functions[idx].name);
    } else {
      //             printf("  --> %s (?)\n", linear_functions[idx].name);
      highest_perc = 0;
      idx = 0;
      if (!next_is_mixer) {
        for (int m = 0; m < mixer; m++) {
          double perc =
              correct_m[m * bits + b] * 100.0 /
              ADDRESSES; /// (correct[c * bits + b] + wrong[c * bits + b]);
          if (perc < 50)
            perc = 100 - perc;
          // printf("%3.1f  ", perc);
          if (perc > highest_perc) {
            highest_perc = perc;
            idx = m;
          }
        }
      }
      if (next_is_mixer) {
        idx = mixer_idx;
        highest_perc = 100;
      }
      printf(" Mixer [%5.1f%%] ", highest_perc);
      if (highest_perc > 90) {
        printf("  --> %s\n", mixers[idx].name);
        next_is_mixer = mixers[idx].bits;
        mixer_idx = idx;
      } else {
        printf("  --> %s (?)\n", mixers[idx].name);
      }
    }
    if (next_is_mixer)
      next_is_mixer--;
  }

  return 0;
}
