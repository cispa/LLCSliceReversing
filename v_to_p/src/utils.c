#include "utils.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
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
