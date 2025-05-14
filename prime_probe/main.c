#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <string.h>
#include <sched.h>
#include "multihist.h"

#include "cacheutils.h"

#define MAX_HIST 10000
#define EV_BUFFER 256 * 1024 * 1024
#define SLICES 8

#define XEON
#include "functions.h"

size_t measure(size_t addr) {
    size_t start = rdtsc();
    maccess(addr);
    return rdtsc() - start;
}


char __attribute__((aligned(4096))) target[4096];

int get_set(size_t paddr) {
    return (paddr >> 6) & 2047;
}

void evict(size_t* set, int len) {
    for(size_t i = 0; i < len - 2; i ++) {
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
}

int main(int argc, char* argv[]) {
    memset(target, 1, sizeof(target));

    multihist_t* hist = multihist_init(10000, 3);
    multihist_set_noise_threshold(hist, 0.005);
    // multihist_set_binsize(hist, 2);
    multihist_set_label(hist, 0, "Evicted");
    multihist_set_label(hist, 1, "Cached");
    multihist_set_label(hist, 2, "Flushed");
    
    
    size_t phys_target = get_physical_address(target);
    char* eviction = malloc(EV_BUFFER);
    memset(eviction, 1, EV_BUFFER);
    
    int target_set = get_set(phys_target);
    int target_slice = compute_slice(phys_target);
    printf("Set: %d, Slice: %d\n", target_set, target_slice);
    size_t evset[64];
    int evcnt = 0;
    size_t candidate = (size_t)eviction;
    while(evcnt < sizeof(evset) / sizeof(evset[0]) && candidate < (size_t)eviction + EV_BUFFER) {
        size_t p = get_physical_address(candidate);
        if(get_set(p) == target_set && compute_slice(p) == target_slice) {
            evset[evcnt++] = candidate;
        }
        candidate += 64;
    }
    printf("Found %d addresses for the eviction set\n", evcnt);
    

    for(int i = 0; i < 5000; i++) {
        asm volatile("mfence");
        evict(evset, evcnt);
        size_t t1 = measure((size_t)target);
        multihist_inc(hist, 0, t1);

        size_t t2 = measure((size_t)target);
        multihist_inc(hist, 1, t2);

        flush((size_t)target);
        size_t t3 = measure((size_t)target);
        multihist_inc(hist, 2, t3);
    }
    // print_histogram(hist, MAX_HIST, 60);
    multihist_print(hist, 120);
    multihist_export_csv(hist, "hist.csv");

    // size_t start, end;
    // cap_hist(hist, MAX_HIST, &start, &end, 0.05);
    // printf("%zd vs %zd\n", hist[0][end], hist[1][end]);


    return 0;
}
