#!/usr/bin/env python3

import sys
from pathlib import Path
import subprocess


def find_tests(test_dir: Path) -> list[str]:
    return [x.stem for x in test_dir.glob('*.input')]


def run_test(test_dir: Path, program: Path, test_name: str) -> bool:
    print(f'Running "{test_name}"...', end=' ', flush=True)
    input_file = test_dir / (test_name + '.input')
    output_file = test_dir / (test_name + '.output')
    with open(input_file, 'r') as f:
        input_data = f.read()
    with open(output_file, 'r') as f:
        expected_output = f.read()
    try:
        actual_output = subprocess.check_output('./' + str(program), text=True, input=input_data)
    except subprocess.CalledProcessError:
        print('FAIL')
        print(f'The program hasn\'t returned 0!', file=sys.stderr)
        return False
    if actual_output != expected_output:
        print('FAIL')
        return False
    print('SUCCESS')
    return True


def main() -> None:
    test_dir = Path(sys.argv[0]).parent
    program = Path(sys.argv[1])
    
    tests = find_tests(test_dir)
    
    success_count = 0
    for test in tests:
        if run_test(test_dir, program, test):
            success_count += 1;
    print(f'Passed {success_count}/{len(tests)} tests.')
    if success_count == len(tests):
        print('=== SUCCESS ===')
    else:
        print('=== FAIL ===')
        sys.exit(1)

if __name__ == '__main__':
    main()
