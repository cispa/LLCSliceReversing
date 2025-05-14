import click
from subprocess import run, PIPE
import tempfile

from slice_functions import XorChain

FILE_NAME = "pattern_lab73/pattern_0.txt"


def _b(x, pos):
    """
    Extracts the bit at the specified position from an integer.

    Args:
        x (int): The integer from which to extract the bit.
        pos (int): The position of the bit to extract (0-based).

    Returns:
        int: The bit at the specified position (0 or 1).
    """
    return (x >> pos) & 1


def read_pattern(file_name: str = FILE_NAME):
    """
    Reads a pattern from a file and returns a list of tuples containing address and slice index.

    Args:
        file_name (str): The name of the file to read from. Defaults to FILE_NAME.

    Returns:
        list of tuples: A list where each tuple contains an address (int) and a slice index (int).
    """
    data = []
    with open(file_name, "r") as f:
        for line in f.readlines():
            tokens = line.split(",")
            address = int(tokens[0][2:], base=16)
            slice_idx = int(tokens[1].strip())
            data.append((address, slice_idx))
    return data


def gen_base_pla(pattern, bit_idx, chains):
    """
    Generates a PLA (Programmable Logic Array) representation based on the given pattern, bit index, and chains.

    A line in the PLA contains the bit results of the chains for a given address as an input and the bit index of the slice index as an output.

    Args:
        pattern (list of tuples): A list of tuples where each tuple contains an address and a slice index.
        bit_idx (int): The bit index of the slice index bit that we are interested in.
        chains (list of callables): A list of functions (chains) that take an address as input and return a bit result.

    Returns:
        str: A string representing the generated PLA.
    """
    base_pla = ""
    base_pla += f".i {len(chains)}\n"
    base_pla += f".o 1\n"
    for address, slice_idx in pattern:
        for i in range(len(chains)):
            bit_res = chains[i](address)
            base_pla += f"{bit_res}"
        base_pla += f" {_b(slice_idx, bit_idx)}\n"
    base_pla += ".e\n"
    return base_pla


def check_pla(pla):
    """
    Checks the given PLA (Programmable Logic Array) content for duplicate chains with different results.

    Args:
        pla (str): A string representation of the PLA content, where each line contains a chain and its result.

    Returns:
        bool: True if no duplicates with different results are found, False otherwise.
    """
    known = {}
    for index, line in enumerate(pla.split("\n")[2:-2]):
        # print(line)
        chains, res = line.split()
        if chains in known and known[chains] != res:
            # print(f"Duplicate! {chains} res {res} {index}")
            return False
        else:
            known[chains] = res
    return True


def reduce_chains(pattern, bit_idx, chains):
    """
    Reduces the given chains by indetifying chains that do lead to contradicting input output pairs in the PLA when removed. This indicates that the corresponding bit does not affect the output, and the chain can be removed.

    Args:
        pattern (str): The pattern used to generate the base PLA.
        bit_idx (int): The bit index used in the generation of the base PLA.
        chains (list): A list of chains to be reduced.

    Returns:
        list: A list of relevant chains.
    """
    relevant_chains = []
    reduced_chains_idx = []
    for i in range(len(chains)):
        # drop the chain
        new_chains = chains[:i] + chains[i + 1 :]
        current_pla = gen_base_pla(pattern, bit_idx, new_chains)
        if check_pla(current_pla):
            reduced_chains_idx.append(i)
        else:
            relevant_chains.append(chains[i])
    print(f"[+] Reduced chains {reduced_chains_idx}")
    return relevant_chains


def solve_pla(pla, optimize, nr_bits):
    # Check if the pla contains contradictions,
    # i.e. the same input produces different outputs
    if not check_pla(pla):
        print("Not out of the box")
        exit(0)

    if optimize:
        print(f"[+] Optimizing PLA")
        p = run(
            ["./espresso-logic/bin/espresso"],
            stdout=PIPE,
            input=pla,
            encoding="ascii",
        )
        pla = p.stdout

    print(f"[+] Synthetizing nonlinear fraction for {nr_bits - 6} bits")

    with tempfile.NamedTemporaryFile(mode="w", delete=False) as f:
        f.write(pla)
        f.flush()
        p = run(
            ["python3", "./synth/boolfunc.py", "--ops", "and2,or2,xor2,not1", "op:pla", "--op.file", f.name],
            stdout=PIPE,
            encoding="ascii",
        )
        print(p.stdout)

    print("[+] Done")


@click.command()
@click.option("--nr-bits", "-n", help="Number of bits", type=int)
@click.option("--bit-idx", "-i", help="Bit idx into slice function", type=int)
@click.option("--file", "-f", help="The path of the measurement data", type=str)
@click.option("--optimize", is_flag=True, help="Optimize output with espresso")
def main(nr_bits, bit_idx, optimize, file):
    assert nr_bits > 6
    pattern = read_pattern(file)
    number_samples = 1 << (nr_bits - 6 + 1)
    print(f"[+] Using {number_samples} samples")
    # base_pla = produce_data(pattern[: 1 << 8], lab_33_1_test)

    pure_0 = XorChain([6]).evaluate
    pure_1 = XorChain([7]).evaluate
    pure_2 = XorChain([8]).evaluate
    pure_3 = XorChain([9]).evaluate
    pure_4 = XorChain([10]).evaluate
    pure_5 = XorChain([11]).evaluate
    pure_bits = [pure_0, pure_1, pure_2, pure_3, pure_4, pure_5]
    """
    input_bits is a list of functions that take an address as input and return
    a bit result. Each element in this list corresponds to an input bit in the
    PLA. In the default case, each returns a bit from the address (6, 7, 8, 9,
    10, 11). However, this can be changed to any other function that takes an
    address as input and returns a bit result. This allows to take the output
    of a circuit that we know to be part of the slice function as input.
    """
    input_bits = pure_bits

    # Reduce chains by only keeping chains that would cause contradictions
    # in the PLA when dropped
    # This is only a heuristic and might not work for all cases
    #reduced_chains = reduce_chains(pattern[:number_samples], bit_idx, input_bits)
    reduced_chains = pure_bits
    print("[+] Generating PLA")
    base_pla = gen_base_pla(pattern[:number_samples], bit_idx, reduced_chains)
    print(base_pla)

    # Use boolean formula synthesis to find a funtion that relates the input bits to the output bit
    solve_pla(base_pla, optimize, nr_bits)


if __name__ == "__main__":
    main()
