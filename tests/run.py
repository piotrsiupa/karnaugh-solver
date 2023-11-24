#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path
import signal
import subprocess
import time


def parse_args() -> tuple[Path, Path, bool]:
    parser = argparse.ArgumentParser(
            prog='run.py',
            description='This script runs all the tests and checks whether they pass.')
    parser.add_argument('program')
    parser.add_argument('-v', '--verbose', action='store_true', help='show all tests even when they pass')
    args = parser.parse_args()
    return Path(args.program), args.verbose


def find_test_dirs(test_dir: Path) -> list[str]:
    return sorted(child for child in test_dir.iterdir() if child.is_dir() and (child / 'input').is_file())


def time_process(process: subprocess.Popen):
    starttime = time.perf_counter()
    while True:
        try:
            process.wait()
            break
        except KeyboardInterrupt:
            process.send_signal(signal.SIGINT)
    endtime = time.perf_counter()
    return endtime - starttime


def run_test(test_name: str, program: Path, input_file: Path, output_file: Path, show_all: bool):
    options = output_file.name.split('_')
    if show_all:
        print(f'Running "{test_name}" ({" ".join(options)})...', end=' ', flush=True)
    process = subprocess.Popen(['./' + str(program), '--no-status', '--name', test_name] + options + [input_file], text=True, stdout=subprocess.PIPE)
    elapsed_time = time_process(process)
    if process.returncode != 0:
        if not show_all:
            print(f'Test "{test_name}" ({" ".join(options)})', end=' ')
        print(f'FAIL ({elapsed_time:.2f}s, return code is {process.returncode})')
        return False
    with open(output_file, 'r') as f:
        expected_output = f.read()
    if process.stdout.read() != expected_output:
        if not show_all:
            print(f'Test "{test_name}" ({" ".join(options)})', end=' ')
        print(f'FAIL ({elapsed_time:.2f}s)')
        return False
    if show_all:
        print(f'SUCCESS ({elapsed_time:.2f}s)')
    return True


def run_tests(main_test_dir: Path, program: Path, test_dir: Path, show_all: bool) -> bool:
    input_file = test_dir / 'input'
    output_files = sorted(output_file for output_file in test_dir.iterdir() if output_file != input_file)
    success_count = 0
    for output_file in output_files:
        if run_test(test_dir.name, program, input_file, output_file, show_all):
            success_count += 1
    return success_count, len(output_files)


def main() -> None:
    main_test_dir = Path(sys.argv[0]).parent
    program, show_all = parse_args()
    
    test_dirs = find_test_dirs(main_test_dir)
    
    success_count = 0
    test_count = 0
    for test_dir in test_dirs:
        this_success_count, this_test_count = run_tests(main_test_dir, program, test_dir, show_all)
        success_count += this_success_count
        test_count += this_test_count
    print(f'Passed {success_count}/{test_count} tests.')
    if success_count == test_count:
        print('=== SUCCESS ===')
    else:
        print('=== FAIL ===')
        sys.exit(1)

if __name__ == '__main__':
    main()
