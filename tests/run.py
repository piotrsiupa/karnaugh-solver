#!/usr/bin/env python3

import sys
from pathlib import Path
import signal
import subprocess
import time


def find_tests(test_dir: Path) -> list[str]:
    return sorted(x.stem for x in test_dir.glob('*.input'))


def run_test(test_dir: Path, program: Path, test_name: str) -> bool:
    print(f'Running "{test_name}"...', end=' ', flush=True)
    input_file = test_dir / (test_name + '.input')
    output_file = test_dir / (test_name + '.output')
    process = subprocess.Popen(['./' + str(program), input_file], text=True, stdout=subprocess.PIPE)
    starttime = time.perf_counter()
    while True:
        try:
            process.wait()
            break
        except KeyboardInterrupt:
            process.send_signal(signal.SIGINT)
    endtime = time.perf_counter()
    if process.returncode != 0:
        print(f'FAIL ({endtime-starttime:.2f}s, return code is {process.returncode})')
        return False
    with open(output_file, 'r') as f:
        expected_output = f.read()
    if process.stdout.read() != expected_output:
        print(f'FAIL ({endtime-starttime:.2f}s)')
        return False
    print(f'SUCCESS ({endtime-starttime:.2f}s)')
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
