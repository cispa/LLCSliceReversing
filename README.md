# LLCSliceReversing
This repository contains the artifact for the IEEE S&P 2025 [paper](https://www.computer.org/csdl/proceedings-article/sp/2025/223600d238/26hiVoj9hU4): *Rapid Reversing of Non-Linear CPU Cache Slice Functions: Unlocking Physical Address Leakage*

## Overview
The artifact is split into sub-folders, each containing documentation on how to setup and run the code.

### Attacks
In our 3 case studies we show that physical address information is useful to perform side-channel attacks.
- `aes_t_table`: Perfect eviction on AES t-tables using the reverse engineered function.
- `drama_v_to_p`: End-to-end DRAMA attack using the v2p-oracle.
- `prime_probe`: Prime&Probe attack with perfect eviction sets on LLC using cache slice function and physical address information
- `realloc_spectre`: Spectre attack using physical address information. Shows that we can get the physical address of victim memory.

### Reverse Engineering
We show how to reverse enigneer non-linear CPU chache-slice functions in an efficient manner. The key idea here is to first focus on one part of the function, the non-linear mixer. After the mixer is recovered it is easy to extend the function to be valid for the complete physical address space.
- `detect_function`: Detect the non-linear cache-slice function used by the current processor
- `measure_pattern`: Measure a slice-access pattern for a given memory region using performance counters.
- `non_linear_solve`: Tools to reverse-engineer non-linear cache-slice functions
- `slice_functions`: Slice function library with all obtained functions.

### Oracle
Besed on the observations we make about non-linear chache slice functions, we build an oracle that can translate virtual to physical addresses from userspace.
- `v_to_p`: V2P oracle, translating virtual to physical memory from userspace using a timing side channel.

## Contact
If there are questions regarding these experiments, please send an email to `mikka.rainer (AT) cispa.de`.

## Citing Paper and Artifacts
If you use our results in your research, please cite our paper as:

```bibtex
@inproceedings{Rainer2025Rapid,
 author={Rainer, Mikka and Hetterich, Lorenz and Thomas, Fabian and Hornetz, Tristan and Trampert, Leon and Gerlach, Lukas and Schwarz, Michael},
 booktitle = {S\&P},
 title={Rapid Reversing of Non-Linear CPU Cache Slice Functions: Unlocking Physical Address Leakage},
 year = {2025}
}

```

And our artifacts as:

```bibtex
@misc{Rainer2025RapidArtifacts,
 author={Rainer, Mikka and Hetterich, Lorenz and Thomas, Fabian and Hornetz, Tristan and Trampert, Leon and Gerlach, Lukas and Schwarz, Michael},
 url = {https://github.com/cispa/TODO},
 title = {Rapid Reversing of Non-Linear CPU Cache Slice Functions: Unlocking Physical Address Leakage Artifact Repository},
 year = {2025}
}
