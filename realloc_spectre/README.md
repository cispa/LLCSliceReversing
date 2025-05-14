# Spectre Reallocation Case Study

To run the case study, run `make` in this directory followed by `taskset -c 0 sudo ./poc` (or `make run`).
Root is required to insert the kernel module containing the spectre gadget.
Pinning to a CPU core makes the reallocation pattern more consistent.

## Expected Results
At the start of execution, the binary should output the physical address of the target kernel buffer:

```
kernel buffer: 0x1100dc000
```

In some cases, the kernel module fails to properly create, setup, and report the buffer to the userspace attacker.
In those cases `0` is printed as address.
If this is the case, the `spectre_module` needs to be removed and re-inserted.
However, this is done automatically when re-executing the proof-of-concept.


During execution, the binary should output lines like:
```
a = 99560, b = 0: 0x18d780000 0x18d780000
```

`a` and `b` are page offsets in the attacker (`a`) and victim (`b`) buffer.
The hexadecimal number is the physical address.

The default offset which worked on multiple devices is `99560`.
Thus, we expect a lot of outputs in the form:

```
a = 99560, b = 0: 0x... 0x... 
a = 99561, b = 1: 0x... 0x...
a = 99562, b = 2: 0x... 0x...
a = 99563, b = 3: 0x... 0x...
a = 99564, b = 4: 0x... 0x...
a = 99565, b = 5: 0x... 0x...
...
```

If the offset differs for your kernel version, you can adjust the `VICTIM_OFFSET` macro in `attacker.c`. 

Once the attack is done, the amount of working and total attempts will be printed:

```
working: 81 / 100
```

We expect a high ratio of working attempts on a machine with little noise.
In any case, the amount should not be 0.

## Configuration
You can adjust the amount of times the experiment is repeated (`REPEATS` in `common.h`).
For the evaluation in the paper, we used a value of `10000`.
However, the default value in this artifact is `100` to reduce execution time.

You can adjust the expected offset of the re-allocated pages within the attacker (`VICTIM_OFFSET`) in `attacker.c`.
By default this offset is set to `99560` which works on multiple machines we tested.

You can also adjust the amount of pages allocated by the attacker (`PAGES` in `attacker.c`) and victim (`PAGES2` in `common.h`).
Note that this likely requires a modification to `VICTIM_OFFSET`.

By default, the PoC uses `/proc/self/pagemap` instead of the physical address oracle for faster evaluation.
You can change this by adjusting the `virt2phys_attack` in `common.h`.

You can adjust the amount iterations that are done for leaking one bit with spectre (`SPECTRE_ITERS` in `attacker.c`).
The first `SPECTRE_ITERS - 1` iterations will be in-bound accesses mistraining the branch predictor.
By default, this value is set to 80.
However, depending on the CPU, a value of 5 often suffices.
