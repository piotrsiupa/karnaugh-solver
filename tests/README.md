# Tests

These are simple tests composed from an input files, each of which is linked with one or more output files.
Each input file is named "input" and is in a separate directory which name is the name of the test. The remaining files in the directory are output files and their names are the options that will be passed to the program, separated by underscores. (A few additional options are added at the beginning to make the tests go smoothly: `--no-status` and `--name TEST_NAME`.)
This is pretty straightforward really. The input file is passed to the program along with the options and the program's output is compared with the appropriate output file.
You can read those files to see some examples of how the input and output of the program look.

Some test files are disabled. (The input name is `input.disabled`.)
It's because the current version of the program is too slow to handle them (because it uses a brute-force algorithm without heuristics).


# `tests/run.py`

This script runs all the enabled tests and prints the result.
It takes one argument which is the path to the program which will be tested.
(The suggested way to run the tests is through `scons` but it works well as a standalone script too.)
You can run it with the option `--help` if you're interested in learning more.


# `tests/gen-random-input.py`

It's a simple script for generating random inputs for the program.
You can run it with the option `--help` if you're interested in learning more.


# `tests/find-failing-example.py`

It runs the program with variety of inputs, trying to find the smallest one for which it crashes.
It's intended to find the smallest offending example when the program crashes, for easier debugging.
You can run it with the option `--help` if you're interested in learning more.
