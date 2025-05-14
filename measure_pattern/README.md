# Measure Pattern
This program records the slice-sequence for a given physical memory region, using uncore performance counters.

## Usage
The program requires sudo privileges to read and write uncore performance counters and [PTEditor](https://github.com/misc0110/PTEditor) to map specific physical memory regions.
```
sudo ./slice [--bit <int>] [--base <hex> --event <hex> --slices <int>]
```

## Expected Output
```
% sudo ./slice --bit 32
Bit: 32
Looking for performance counter to use, this might take a while...
Found 6 uncore CBoxes | Perf type: 14
6 slices, base 0xe, event 0xf37
Physical address: 0x100000000
Physical check: 0x100000000
Virtual address: 0x71488bae1000
100.00%  Done!
```
The program creates a file `pattern_<bit>.txt` that contains pairs of physical address and slice for 1024 cache lines:
```csv
0x100000000, 4
0x100000040, 3
0x100000080, 4
0x1000000c0, 1
0x100000100, 3
0x100000140, 2
0x100000180, 1
0x1000001c0, 0
...
```
