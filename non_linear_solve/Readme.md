# Reverse Engineering of Nonlinear Hash Functions
This is an approach that allows to solce nonlinear hash functions.
We observed that non-linear slice addressing functions consist of a non-linear mixer that takes XOR chains as an input for the lower bits, and linear XOR chains for the upper bits.

We split out reverse engineering of the non-linear mixer into two parts:
- solving the non-linear mixer for a fixed number of address bits (e.g input bits 6-11)
- solving the upper bits of the Xor chains that are used as an input for the mixer (e.g. input bits 12-36)

## Obtaining Input Data
This tool requires some input data of the form: (physical address, slice index).
You can obtain this data by using the `measure_pattern` tool. For the first part of the reverse engineering it suffices to have data for the first 1024 cache lines. To measure from base address 0x0 call it with:
```bash
sudo ./slice --bit 0
```

For the remaining part the tool requires a pattern for each bit of interest above bit 15.
```bash
sudo ./slice --bit 16
...
```

The data for the examples is provided in the `pattern_lab73` folder.

## Solving the Non-Linear Mixer

### How to use
```bash
python rev_mixer.py -n 11 -i 4 --optimize -f "pattern_lab73/pattern_0.txt"
```

```
Usage: rev_mixer.py [OPTIONS]

Options:
  -n, --nr-bits INTEGER  Number of bits
  -i, --bit-idx INTEGER  Bit idx into slice function
  -f, --file TEXT        The path of the measurement data
  --optimize             Optimize output with espresso
  --help                 Show this message and exit.
```
This example uses the raw bits 6,7,8,9,10,11 as an input for the solver to solve the last ouput bit of the slice function. The following bits should use the circuit of the last bit as an input bit for the solver, as this is the typical structure of the circuit.
For more complex functions it might be required to start with raw bits and interatively build the circuit by changing bits of the input to the solver to circuits that you found to be part of the function.

### Example Output
```bash
[+] Using 64 samples
[+] Reduced chains [0, 2]
[+] Generating PLA
[+] Optimizing PLA
[+] Synthetizing nonlinear fraction for 5 bits
/tmp/tmpzzftimo2:
x4 = or2  (x2, x3)
x5 = and2 (x1, x4)
y0 = and2 (x0, x5)
synthesis time: 0.323s

[+] Done
```

## Solving the Upper Bits
In this part, we fix the non-linear part of the function that we aquired for a small part in the beginning of the address space, e.g. all adresses with 12 bits (up to 0x800). The goal of this process is to obtain the extended xor chains that are used as in input for the mixer. This then gives us the complete function for the whole address space, e.g. for 128GB of RAM all adresses with 37 bits (up to 0x1000000000).

We split this process into two parts:
- Extending bits up to bit 15, to get a function for the complete base pattern
- Extending bits up to bit 36, using patterns at the beginning of ranges where a new bit is introduced.

### How to Use
Solving the upper bits of the nonlinear has function, requires you to specify the circuit for the mixer that you extracted in the first step as a python function, such that it can be used for pattern generation.
```bash
python3 upper_bits.py
```
### Example Output
```bash
[*] Extending to 15 starting from 12
[+] chain 0: [7, 12, 13]
[+] chain 1: [9, 14, 15]
[+] chain 2: [10, 15]
[+] chain 3: [11]
[+] Correct

[*] Extending upper bits up to 36
[+] chain 0: [7, 12, 13, 17, 19, 22, 23, 24, 25, 27, 32, 33, 36]
[+] chain 1: [9, 14, 15, 19, 21, 24, 25, 26, 27, 29, 33, 34, 35]
[+] chain 2: [10, 15, 16, 20, 22, 25, 26, 27, 28, 30, 34, 35, 36]
[+] chain 3: [11, 16, 17, 21, 23, 26, 27, 28, 29, 31, 35, 36]
```
