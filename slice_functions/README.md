# Non-Linear Slice Function Library
`slice_functions.h` is a header-only library that can be used to compute the slice index for a given physical address. The library contains `C` implementations of all XOR chains that we observed and different mixer circuits.

Our experiments indicate that the LLC slice function only depends on the number of cores and is independent of the microarchitecture. However, we can not guarantee that this is always the case.

## Usage

To compute the index of an address and a specified number of slices use:
```C
int compute_slice(uint64_t x, int num_slices);
```