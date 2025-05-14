import itertools
import pprint


def _b(x, pos):
    # print(f"x:{hex(x)} pos:{pos} {(x >> pos) & 1}")
    return (x >> pos) & 1


class XorChain:
    def __init__(self, relevant_bits: list):
        self.relevant_bits = relevant_bits

    def evaluate(self, address: int) -> int:
        """
        Evaluate the output bit for a given address by XOR-ing the relevant bits.

        Args:
            address (int): The address to evaluate.

        Returns:
            int: The resulting output bit after XOR-ing the relevant bits.
        """
        output_bit = 0
        for relevant_bit in self.relevant_bits:
            output_bit ^= _b(address, relevant_bit)
        return output_bit


class SliceFunction:
    def __init__(self, bit_functions: list):
        self.bit_functions = bit_functions

    def evaluate(self, address: int) -> int:
        """
        Computes the slice index for a given address.

        Args:
            address (int): The address.

        Returns:
            int: The computed slice index.
        """
        slice_idx = 0
        for idx, bit_function in enumerate(self.bit_functions):
            slice_idx |= bit_function(address) << idx
        return slice_idx

    def evaluate_bit(self, address: int, bit: int) -> int:
        """
        Computes a specific bit in the output of the slice function for a given address.

        Args:
            address (int): The address.
            bit (int): The index of the ouput bit to compute.

        Returns:
            int: The value of the output bit of the slice function.
        """
        assert bit < len(self.bit_functions)
        res = self.bit_functions[bit](address)
        # print(res)
        return res

    def verify(self, pattern) -> bool:
        pattern_correct = True
        fault_ctr = 0
        print(f"[+] Testing with {len(pattern)} samples")
        for address, slice_idx in pattern:
            # print(f"[+] Slice {bin(slice_idx)}, Address: {hex(address)}")
            # address = (address >> 6) << 6
            out_str = f"\t[-] {hex(address)}: "
            current_correct = True
            for j in range(len(self.bit_functions)):
                measured_bit = _b(slice_idx, j)
                function_bit = self.evaluate_bit(address, j)
                # print(f"function bit: {function_bit}, measured bit: {measured_bit}")
                if function_bit != measured_bit:
                    out_str += f"\033[31m{function_bit}\033[0m"
                    pattern_correct = False
                    current_correct = False
                else:
                    out_str += f"{measured_bit}"
            out_str += " "
            if not current_correct:
                fault_ctr += 1
                print(out_str)
        if pattern_correct:
            print(f"[+] Correct")
        else:
            print(f"[-] Incorrect: {fault_ctr}/{len(pattern)} faults")
        return pattern_correct

    def verify_bit(self, pattern, idx, debug=False) -> bool:
        pattern_correct = True
        fault_ctr = 0
        # print(f"[+] Testing with {len(pattern)} samples")
        for address, slice_idx in pattern:
            address = (address >> 6) << 6
            out_str = f"\t[-] {address:0{16}b}:"
            current_correct = True
            measured_bit = _b(slice_idx, idx)
            function_bit = self.evaluate_bit(address, idx)
            if function_bit != measured_bit:
                out_str += f" {measured_bit}\033[31m{function_bit}\033[0m"
                pattern_correct = False
                current_correct = False
            else:
                out_str += f" {measured_bit}{function_bit}"
            if not current_correct:
                fault_ctr += 1
            out_str += " "
            if not current_correct and debug:
                print(out_str)
        if pattern_correct:
            if debug:
                print(f"[+] Correct")
            pass
        else:
            if debug:
                print(f"[-] Incorrect: {fault_ctr}/{len(pattern)} faults")
            pass
        return pattern_correct

    def extend_bit(self, pattern, out_bit, pos, upper_bound, raw_chains):
        """
        Extends the XOR chains by trying all combinations of extensions of a given bit and verifying their correctness.

        Args:
            pattern (list): The data pattern to be used for verification.
            out_bit (int): The output bit to be verified.
            pos (int): The bit to use for extension.
            upper_bound (int): The upper bound for the pattern.

        Returns:
            list: A list of valid bit extensions.
        """
        # print(f"Extending bit {pos}")
        variants = []
        # Try all combinations of extensions
        for x in itertools.product([0, 1], repeat=len(raw_chains)):
            # Append new bit combination to the chain ends
            for idx, bit in enumerate(x):
                if bit == 1:
                    raw_chains[idx].append(pos)
            # Verify the correctness of the results
            if self.verify_bit(pattern[: (1 << (upper_bound + 1 - 6))], out_bit):
                # print(f"Success {x}")
                # Store a valid extension upon success
                variants.append(x)
            # Remove the extensions from the chain ends
            for idx, bit in enumerate(x):
                if bit == 1:
                    raw_chains[idx].pop()
        return variants


# Definition of XOR chains
l6b = XorChain(
    [6, 8, 9, 10, 14, 15, 17, 18, 20, 23, 27, 30, 31, 34, 36, 38]
).evaluate
l6c = XorChain(
    [6, 11, 12, 16, 18, 21, 22, 23, 24, 26, 30, 31, 32, 35, 38]
).evaluate
l6f = XorChain(
    [6, 7, 8, 12, 16, 17, 20, 21, 22, 23, 24, 25, 26, 28, 30, 33, 35]
).evaluate
l7b = XorChain(
    [7, 12, 13, 17, 19, 22, 23, 24, 25, 27, 31, 32, 33, 36]
).evaluate
l8b = XorChain(
    [8, 13, 14, 18, 20, 23, 24, 25, 26, 28, 32, 33, 34, 37]
).evaluate
l9c = XorChain(
    [9, 14, 15, 19, 21, 24, 25, 26, 27, 29, 33, 34, 35, 38]
).evaluate
l10b = XorChain(
    [10, 15, 16, 20, 22, 25, 26, 27, 28, 30, 34, 35, 36]
).evaluate
l11a = XorChain(
    [11, 16, 17, 21, 23, 26, 27, 28, 29, 31, 35, 36, 37]
).evaluate


# Definition of C3 mixer bits
def bit3(x):
    return (l10b(x) | l11a(x)) & l7b(x) & l9c(x) & 1


def bit2(x):
    return ((~bit3(x)) & l6c(x)) & 1


def bit1(x):
    return ((~bit3(x)) & l8b(x)) & 1


lab_73 = SliceFunction([l6b, l6f, bit1, bit2, bit3])


def print_hist(pattern):
    pattern_hist = {}
    bit_hist = [0, 0, 0, 0, 0]
    number_samples = len(pattern)
    for address, slice_idx in pattern:
        if slice_idx not in pattern_hist:
            pattern_hist[slice_idx] = 0
        else:
            pattern_hist[slice_idx] += 1
        for i in range(5):
            if _b(slice_idx, i) == 1:
                bit_hist[i] += 1
    print("Number of accesses per slice")
    for slice_idx in sorted(pattern_hist.keys()):
        print(f"{slice_idx:0{2}d}: {pattern_hist[slice_idx]}")
    print()
    print("Number of activations per output bit")
    for idx, bit_ctr in enumerate(bit_hist):
        print(f"{idx}: {bit_ctr} {int((bit_ctr/number_samples)*100)}%")
    print()


def read_pattern(file_name: str):
    data = []
    with open(file_name, "r") as f:
        for line in f.readlines():
            tokens = line.split(",")
            address = int(tokens[0][2:], base=16)
            slice_idx = int(tokens[1].strip())
            data.append((address, slice_idx))
    return data


def main():
    pattern = read_pattern(f"pattern_lab73/pattern_0.txt")
    print_hist(pattern)
    lab_73.verify(pattern)


if __name__ == "__main__":
    main()
