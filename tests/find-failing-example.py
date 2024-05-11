#!/usr/bin/env python3

import argparse
import os
from pathlib import Path
import subprocess
import sys
import time


file_path = Path(sys.argv[0]).parent / 'failing_example'
random_generator_path = Path(sys.argv[0]).parent / 'gen_random_input.py'
program_args = ['--no-progress', '--format=gate-costs']


def create_all_examples_of_size(bits, functions):
    limit = 2 ** bits if bits != 0 else 0
    bit_limit = 2 ** limit
    minterms = [0] * functions
    dont_cares = [0] * functions
    mask = [bit_limit - 1] * functions
    
    while True:
        with open(file_path, 'w') as file:
            def make_values_(x):
                if x == 0:
                    yield '-'
                else:
                    for y in range(limit):
                        if x >> y & 0b1:
                            yield str(y)
            def make_values(x):
                return ', '.join(make_values_(x))
            file.write(str(bits) + '\n')
            for i in range(functions):
                file.write('\n')
                file.write(make_values(minterms[i]) + '\n')
                file.write(make_values(dont_cares[i]) + '\n')
        yield None
        
        for i in range(functions):
            if minterms[i] != mask[i]:
                minterms[i] = (minterms[i] + bit_limit - mask[i]) & mask[i]
            else:
                dont_cares[i] += 1
                if dont_cares[i] == bit_limit:
                    minterms[i] = 0
                    dont_cares[i] = 0
                    mask[i] = bit_limit - 1
                    continue
                minterms[i] = 0
                mask[i] = ~dont_cares[i] & (bit_limit - 1)
            break
        else:
            break


def create_random_example_of_size(bits, functions):
    subprocess.check_call([sys.executable, random_generator_path, str(bits), str(functions), file_path])


def create_examples():
    to_check_all = [
                (0, 0, None), (0, 1, None), (0, 2, None), (0, 3, None), (0, 4, None),
                (1, 0, None), (1, 1, None), (1, 2, None), (1, 3, None), (1, 4, None),
                (2, 0, None), (2, 1, None), (2, 2, None), (2, 3, 4000), (2, 4, 1000),
                (3, 0, None), (3, 1, None), (3, 2, 1000), (3, 3, 1000), (3, 4, 800),
                (4, 0, None), (4, 1, 1000), (4, 2, 500), (4, 3, 400), (4, 4, 200),
                (5, 0, None), (5, 1, 400), (5, 2, 100),
                (6, 0, None), (6, 1, 100), (6, 2, 10),
                (7, 0, None), (7, 1, 50), (7, 2, 5),
                (8, 0, None), (8, 1, 10), (8, 2, 1),
                (9, 0, None), (9, 1, 3), (9, 2, 1),
                (10, 0, None), (10, 1, 1), (10, 2, 1),
                (12, 0, None), (12, 1, 1), (12, 2, 1),
                (16, 0, None), (16, 1, 1), (16, 2, 1),
                (20, 0, None), (20, 1, 1), (20, 2, 1),
                (24, 0, None), (24, 1, 1), (24, 2, 1),
                (32, 0, None), (32, 1, 1), (32, 2, 1),
            ]
    for current in to_check_all:
        bits, functions, limit = current
        if limit is None:
            for _ in create_all_examples_of_size(bits, functions):
                yield current
        else:
            for _ in range(limit):
                create_random_example_of_size(bits, functions)
                yield current


def run_check(executable_path):
    try:
        subprocess.run([executable_path] + program_args + ['--', file_path], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, check=True)
        return None
    except subprocess.CalledProcessError as err:
        if err.stderr:
            return err.stderr.decode()
        else:
            return '<No error message - likely a segfault>'


def run_checks(executable_path):
    def print_info():
        print(f'\x1B[1K\rTesting {"all" if limit is None else "rnd"} combinations of {bits} input bits and {functions} functions ({max_iters})...  {(current_iter / max_iters) * 100}%', end='', flush=True)
    previous = None
    for current in create_examples():
        if current != previous:
            if previous is not None:
                print_info()
            previous = current
            bits, functions, limit = current
            current_iter = 0
            if limit is None:
                max_iters = (1 if bits == 0 else 3 ** (2 ** bits)) ** functions
            else:
                max_iters = limit
            print(flush=False)
            last_time = 0.0
        if time.time() - last_time >= 1.0:
            print_info()
            last_time = time.time()
        if error_message := run_check(executable_path):
            print()
            print(flush=True)
            print(error_message, file=sys.stderr, flush=True)
            print()
            file_path_for_human = file_path.resolve().parent.name + os.sep + file_path.name
            print(f'Example of input crashing the program if found and saved to "{file_path_for_human}"!')
            print('You can reproduce it by running:')
            print('.' + os.sep + executable_path.name, *program_args, '--', file_path_for_human)
            return True
        current_iter += 1
    print_info()
    
    print()
    print()
    print('It doesn\'t seem like there are any outputs that can crash the program.')
    print('Either that, or this is very rare and specific set of data that is hard to achieve by random.')
    print(f'(or this may be a wrong set of arguments: {program_args})')
    return False


def main():
    parser = argparse.ArgumentParser(
            prog='find-failing-example.py',
            description='This script tries to find the smallest input for which the program crashes.')
    parser.add_argument('PROGRAM', help='path to the karnaugh executable')
    parser.add_argument('PROGRAM_ARGS', nargs='*', help='arguments that will be passed to the karnaugh executable for each run\n(additionally `--no-progress` and `--format=gate-costs` are added at the beginning)')
    args = parser.parse_args()
    global program_args
    program_args += args.PROGRAM_ARGS
    
    sys.exit(0 if run_checks(Path(args.PROGRAM).resolve()) else 1)

if __name__ == '__main__':
    main()
