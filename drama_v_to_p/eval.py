#!/usr/bin/env python3
import subprocess
import tempfile
import time
import os
import sys

# n: number of bytes
def run_evaluation(n_bytes):
    data = os.urandom(n_bytes)
    assert(n_bytes == len(data))

    with tempfile.NamedTemporaryFile(delete=False) as temp_file:
        temp_file.write(data)
        temp_file.flush()
        temp_file_path = temp_file.name

    with tempfile.NamedTemporaryFile(delete=False) as output_file:
        output_file_path = output_file.name

    receiver_process = subprocess.Popen(['./build/drama', '--outfile', output_file_path, "--nbytes", str(n_bytes)])
    sender_process = subprocess.Popen(['./build/drama', '--sendfile', temp_file_path, "--nbytes", str(n_bytes)])
    receiver_process.wait()

    with open(output_file_path, 'rb') as f:
        received = f.read()

    # Extract time and received data
    split = received.split(b'\n')
    time_ms = int(split[-1].decode().strip())
    received_data = b'\n'.join(split[:-1])

    # Calculate metrics
    time_seconds = time_ms / 1000.0
    bits = n_bytes * 8
    speed_kbps = (bits / 1024) / time_seconds

    # Bitwise accuracy calculation
    wrong_bits = sum((a^b).bit_count() for a, b in zip(data, received_data))
    error_rate = wrong_bits/bits * 100

    assert(len(received_data) == n_bytes)

    print(f"Transmission speed: {speed_kbps:.2f} kbit/s")
    print(f"Error rate: {error_rate:.2f}%")

    os.remove(temp_file_path)
    os.remove(output_file_path)

if len(sys.argv) > 1:
    run_evaluation(int(sys.argv[1]))
else:
    run_evaluation(1 << 20)
