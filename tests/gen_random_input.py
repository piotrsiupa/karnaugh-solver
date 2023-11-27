#!/usr/bin/env python

import os
from pathlib import Path
import random
import sys


def print_help() -> None:
    print('This is a simple script that generates a valid random input for the program.')
    print('The default proportions of 0s to 1s to don\'t-cares in the table are 1 to 2 to 1.')
    print()
    print(f'Usage:\t{Path(sys.argv[0]).name} --help')
    print(f'\t{Path(sys.argv[0]).name} INPUT_SIZE FUNCTION_COUNT [0S_PROP 1S_PROP DCS_PROP] OUTPUT_FILE')
    print()
    print('INPUT_SIZE is the number of inputs to the functions (between 0 and 32).')
    print()
    print('FUNCTION_COUNT is the number of functions to generate randomly.')
    print()
    print('0S_PROP, 1S_PROP, DCS_PROP are proportions of more or less how many 0s, 1s and\ndon\'t-cares will be in the requested funtion.')
    print('(The amount won\'t be exact because it\'s all randomly generated.')


def print_inputs(output, input_size: int) -> None:
    output.write(str(input_size).encode('utf-8') + b'\n')


def reread_number(reread):
    number = reread.read(1)
    if number in (b'\n', b'-'):
        return None
    while (x := reread.read(1)) not in (b' ', b'\n'):
        number += x
    if x == b'\n':
        reread.seek(-1, os.SEEK_CUR)
    return int(number)


def print_function(output, output_path: str, proportions: (int, int, int), input_size: int) -> None:
    start_pos = output.tell()
    
    rand_range = sum(proportions)
    rand_threshold = proportions[1]
    for i in range(1 << input_size):
        r = random.randint(1, rand_range)
        if r <= rand_threshold:
            output.write(str(i).encode('utf-8') + b' ')
    output.seek(-1, os.SEEK_CUR)
    if output.read(1) == b'\n':
        output.write(b'-')
    else:
        output.seek(-1, os.SEEK_CUR)
    output.write(b'\n')
    
    output.flush()
    with open(output_path, 'rb') as reread:
        reread.seek(start_pos, os.SEEK_SET)
        rand_range = proportions[0] + proportions[2]
        rand_threshold = proportions[2]
        next_one = reread_number(reread)
        for i in range(1 << input_size):
            if i == next_one:
                next_one = reread_number(reread)
                continue
            r = random.randint(1, rand_range)
            if r <= rand_threshold:
                output.write(str(i).encode('utf-8') + b' ')
        output.seek(-1, os.SEEK_CUR)
        if output.read(1) == b'\n':
            output.write(b'-')
        else:
            output.seek(-1, os.SEEK_CUR)
        output.write(b'\n')


def main() -> None:
    if len(sys.argv) > 1 and sys.argv[1] == '--help':
        print_help()
        return
    elif len(sys.argv) in (4, 7):
        input_size = int(sys.argv[1])
        if input_size not in range(33):
            print('INPUT_SIZE is out of range!', file=sys.stderr)
            sys.exit(1)
        function_count = int(sys.argv[2])
        if function_count < 0:
            print('FUNCTION_COUNT cannot be negative!', file=sys.stderr)
            sys.exit(1)
        if len(sys.argv) == 7:
            proportions = (int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]))
            if any(prop < 0 for prop in proportions):
                print('Proportions cannot be negative!', file=sys.stderr)
                sys.exit(1)
            elif sum(proportions) == 0:
                print('At least one proportion number has to be bigger than 0!', file=sys.stderr)
                sys.exit(1)
        else:
            proportions = (1, 2, 1)
        output_path = sys.argv[-1];
    else:
        print('Expected 3 or 6 arguments!', file=sys.stderr)
        sys.exit(1)
    
    with open(output_path, 'w+b') as output:
        print_inputs(output, input_size)
        for _ in range(function_count):
            output.write(b'\n')
            print_function(output, output_path, proportions, input_size)

if __name__ == '__main__':
    main()
