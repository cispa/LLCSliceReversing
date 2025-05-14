## Machine

The case study is built for an Intel Core i7-8700 with 16 GB of memory.
It demonstrates a DRAMA covert channel and uses v2p-oracle for the virtual to physical address translation.

## Building

The case study can be built using CMake:

```sh
mkdir build
cd build
cmake ..
make
```

The output is `build/drama`.

## Usage/Evaluation

`eval.py` provides a simple Python script that creates random data to transfer over the covert channel and ensures that the receiver and sender processes are started at the same time. It also provides the transfer and error rate. It takes one argument, the number of bytes to transfer, and defaults to 1MB if nothing is passed.

NOTE: For the fully unprivileged attack, one or multiple machine reboots might be needed to get a working run. This is due to physical address fragmentation. We observed the best results when running the attack right after a reboot.

``` sh
python eval.py
# For privileged (debugging) variant:
sudo python eval.py
```

## Debugging/Configurations

Different configurations of the case study exist. Select them in `CMakeLists.txt`.
The implemented options are:
1. [slooow, use for evaluation] fully unprivileged attack
2. unprivileged but with debug prints
3. privileged: don't measure slice via timing; use the slice function
4. privileged: check consecutive, but don't use Oracle at all (just directly translate virt to phys)
5. [very fast, use for debugging] privileged, don't use Oracle at all, just directly translate virt to phys, don't even check if consecutive

### Expected output (1, fully unprivileged config)

```
$ python eval.py 1000
TODO
```

### Expected output (5, debug config)

```
$ sudo python eval.py 1000
{0, 1, 2, 3, 4, 5}
{0, 1, 2, 3, 4, 5}
[+] Memory candidate: 0x737a1d000000
virt: 0x737a1d000000, phys: 0x278226000 real_phys: 0x278226000
mapping: 0x737a1d000000, mapping_size: 0x1900000, mapping_physical: 0x278226000
[+] Memory candidate: 0x75ffd5000000
virt: 0x75ffd5000000, phys: 0x27836c000 real_phys: 0x27836c000
mapping: 0x75ffd5000000, mapping_size: 0x1900000, mapping_physical: 0x27836c000
virt 0x75ffd5000180 phys 0x27836c180
base set 0 base_row 40461
virt 0x737a1d022100 phys 0x278248100
base set 0 base_row 40457
start sender
start recver
stop sender
stop receiver
Transmission speed: 1.04 kbit/s
Error rate: 6.48%
```
