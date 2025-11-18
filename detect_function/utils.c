
#include "utils.h"

#include <asm/unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask);

void utils_pin_to_core(pid_t pid, int core) {
  cpu_set_t mask;
  mask.__bits[0] = 1 << core;
  sched_setaffinity(pid, sizeof(cpu_set_t), &mask);
}

size_t utils_get_physical_address_pid(pid_t pid, size_t vaddr) {
  int fd;
  if (pid == 0) {
    fd = open("/proc/self/pagemap", O_RDONLY);
  } else {
    char fname[256];
    sprintf(fname, "/proc/%d/pagemap", pid);
    fd = open(fname, O_RDONLY);
  }
  if (fd != -1) {
    uint64_t virtual_addr = (uint64_t)vaddr;
    size_t value = 0;
    off_t offset = (virtual_addr / getpagesize()) * sizeof(value);
    int got = pread(fd, &value, sizeof(value), offset);
    close(fd);
    //         printf("%d, %zd\n", got, value);
    if (got < 8 || value == 0)
      goto err;
    return (value * getpagesize()) | ((size_t)vaddr % getpagesize());
  }
err:
  return 0;
}

size_t utils_get_physical_address(size_t vaddr) {
  return utils_get_physical_address_pid(0, vaddr);
}

int utils_event_open(enum perf_type_id type, __u64 config, __u64 exclude_kernel,
                     __u64 exclude_hv, __u64 exclude_callchain_kernel,
                     int cpu) {
  static struct perf_event_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.type = type;
  attr.config = config;
  attr.size = sizeof(attr);
  attr.exclude_kernel = exclude_kernel;
  attr.exclude_hv = exclude_hv;
  attr.exclude_callchain_kernel = exclude_callchain_kernel;
  attr.sample_type = PERF_SAMPLE_IDENTIFIER;
  attr.inherit = 1;
  attr.disabled = 1;

  int fd = syscall(__NR_perf_event_open, &attr, -1, 0, -1, 0);
  assert(fd >= 0 && "perf_event_open failed: you forgot sudo or you have no "
                    "perf event interface available for the userspace.");

  return fd;
}

int utils_find_index_of_nth_largest_size_t(size_t *list, size_t nmemb,
                                           size_t skip) {
  size_t sorted[nmemb];
  size_t idx[nmemb];
  size_t i, j;
  size_t tmp;
  memset(sorted, 0, sizeof(sorted));
  for (i = 0; i < nmemb; i++) {
    sorted[i] = list[i];
    idx[i] = i;
  }
  for (i = 0; i < nmemb; i++) {
    int swaps = 0;
    for (j = 0; j < nmemb - 1; j++) {
      if (sorted[j] < sorted[j + 1]) {
        tmp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = tmp;
        tmp = idx[j];
        idx[j] = idx[j + 1];
        idx[j + 1] = tmp;
        swaps++;
      }
    }
    if (!swaps)
      break;
  }

  return idx[skip];
}
