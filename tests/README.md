# Tests

Each test is composed of two files, one with extension `.input` and one with extension `.output`.
This is pretty straightforward really. The input file is passed to the program and the program's output is compared with the output file.
You can read those files to see some examples of how the input and output of the program look.

Some test files are disabled. (They have `.disabled` added to their name.)
It's because the current version of the program is too slow to handle them (because it uses a brute-force algorithm without heuristics).


# `tests/run.py`

This script runs all the enabled tests and prints the result.
It takes one argument which is the path to the program which will be tested.
(The suggested way to run the tests is through `scons`.)


# `tests/benchmark.sh`

It's just some script I use for benchmarking.
You can run it with the option `--help` if you're interested.


# `tests/gen-random-input.py`

It's a simple script for generating random inputs for the program.
You can run it with the option `--help` if you're interested.
