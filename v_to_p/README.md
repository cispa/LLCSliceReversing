# Proof of Concept of Virtual to Physical Address Oracle

## Build
```bash
mkdir build
cd build
cmake ..
make
```

## Parameters
The program parameters are currently hard-coded in the source files.

## Execution
You have to execute the POC with sudo such that we can verify the result of the virtual to physical translation. However the same attack works without privileges.
```bash
sudo ./magic
```
Expected output:
```
[+] Found   2048 consecutive pages
============================ Test function =============================
[+] virtual: 0x7f697e000000, physical: 0x1e1bc6000, slice: 4~4 correct
[+] Took 0 ms
===================== Record slice access pattern ======================
[+] 100% [|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]
[+] Took 247 seconds
[+] 100% [|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]
[+] Took 285 seconds
[+] 100% [|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]
[+] Took 235 seconds
[+] 100% [|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]
[+] Took 286 seconds
[+] 100% [|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]
[+] Took 284 seconds
===================== Verify slice access pattern ======================
[+] Faults: 0/131072 (0%)
[+] Pattern suits for physical address 0x1e1bc6000
============================== Search pfn ==============================
[+] 100% [|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]
[+] Number of candidates: 1
[+] Number of good candidates: 1
[+] Took 0 seconds
[+] Unmapping successful
```
The POC was tested on an Intel Core i7-10710U CPU with Ubuntu 22.04.5 LTS (GNU/Linux 5.15.0-124-generic x86_64).

## Explanation of Execution Stages
1. Consecutive memory: the program uses a heuristic to get a physically contigous memory region
2. The program tests if the cache slice function for this processor determines the same cache slice as the timing side channel
3. Recording of the cache slice pattern for the whole region, 5 times to correct errors
4. Count errors in the recorded pattern
5. Search through the cache slice function and search for occurrences of the pattern and output the number of physical base addresses that would lead to the same pattern.
