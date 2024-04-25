#!/usr/bin/env python

from pathlib import Path
import random
import sys


def print_help() -> None:
    print('This is a simple script that generates a valid random input for the program.')
    print('The default proportions of 0s to 1s to don\'t-cares in the table are 1 to 2 to 1.')
    print()
    print(f'Usage:\t{Path(sys.argv[0]).name} --help')
    print(f'\t{Path(sys.argv[0]).name} INPUT_SIZE FUNCTION_COUNT [0S_PROP 1S_PROP DCS_PROP]')
    print()
    print('INPUT_SIZE is the number of inputs to the functions (between 0 and 32).')
    print()
    print('FUNCTION_COUNT is the number of functions to generate randomly.')
    print()
    print('0S_PROP, 1S_PROP, DCS_PROP are proportions of more or less how many 0s, 1s and')
    print('don\'t-cares will be in the requested funtion.')
    print('(The amount won\'t be exact because it\'s all randomly generated.')


def print_inputs(input_size: int) -> None:
    print(input_size)


def print_function(proportions: (int, int, int), input_size: int) -> None:
    minterms = []
    dont_cares = []
    
    rand_max = sum(proportions)
    max_for_1 = proportions[1]
    max_for_dc = proportions[1] + proportions[2]
    for i in range(1 << input_size):
        r = random.randint(1, rand_max)
        if r <= max_for_1:
            minterms.append(i)
        elif r <= max_for_dc:
            dont_cares.append(i)
    
    def format_values(values: [int]) -> str:
        if values:
            return ' '.join(str(x) for x in values)
        else:
            return '-'
    print(format_values(minterms))
    print(format_values(dont_cares))


def main() -> None:
    if len(sys.argv) > 1 and sys.argv[1] == '--help':
        print_help()
        return
    elif len(sys.argv) in (3, 6):
        input_size = int(sys.argv[1])
        if input_size not in range(33):
            print('INPUT_SIZE is out of range!', file=sys.stderr)
            sys.exit(1)
        function_count = int(sys.argv[2])
        if function_count < 0:
            print('FUNCTION_COUNT cannot be negative!', file=sys.stderr)
            sys.exit(1)
        if len(sys.argv) == 6:
            proportions = (int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]))
            if any(prop < 0 for prop in proportions):
                print('Proportions cannot be negative!', file=sys.stderr)
                sys.exit(1)
            elif sum(proportions) == 0:
                print('At least one proportion number has to be bigger than 0!', file=sys.stderr)
                sys.exit(1)
        else:
            proportions = (1, 2, 1)
    else:
        print('Expected 2 or 5 arguments!', file=sys.stderr)
        sys.exit(1)
    
    print_inputs(input_size)
    for _ in range(function_count):
        print()
        print_function(proportions, input_size)


if __name__ == '__main__':
    main()
