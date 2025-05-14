# AES T-Tables Perfect Eviction PoC
We demonstrate that we can create perfect eviction sets for the L3 cache by performing an EVICT+RELOAD attack on the well-known example of AES T-tables
from OpenSSL 3.4.0 (released October 2024).

## Test System
- Chip:  Intel Xeon E5-2697 v4 with 18 slices and 512 GB memory
- Slice function: `C2a|C9`
- OS: Ubuntu 24.04 LTS x86_64
## Usage
Automatically install openssl and create binary:
```
make
```
Run PoC:
```
sudo ./spy
```

## Expected Output
As the first key byte is 0, we expect to see a diagonal in the heatmap.