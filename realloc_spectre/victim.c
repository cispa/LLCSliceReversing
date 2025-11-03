#include "common.h"

int main(int argc, char **argv) {
  init();

  uint8_t *buf = mmap(NULL, PAGE_SIZE * PAGES2, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_POPULATE | MAP_PRIVATE, -1, 0);
  memset(buf, '2', PAGE_SIZE * PAGES2);
  memset(buf, '1', PAGE_SIZE);

  for (int i = 0; i < PAGES2; i++) {
    shared_mem[i] = virt2phys(&buf[i * PAGE_SIZE]);
  }

  usleep(200000);
}
