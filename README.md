# Karnaugh Solver

A CLI aplication to minimize big (up to 32 variables) logic functions, that can handle multiple mappings at once and eliminates common subexpressions.


## What does this do?

This program takes a description of multiple logic functions (up to 32 input variables) in the form of lists of minterms and don't-cares.
It performs [logic optimization](https://en.wikipedia.org/wiki/Logic_optimization) on them to find the minimal matching [SOPs](https://en.wikipedia.org/wiki/Canonical_normal_form#SOP). (Using the [Petrick's method](https://en.wikipedia.org/wiki/Petrick%27s_method).)

After that, it (optionally) performs a [common subexpression elimination](https://en.wikipedia.org/wiki/Common_subexpression_elimination) to further reduce the number of logic gates required to build the circuit.
(This is the main reason for which the program has ability to solve multiple functions in a single run.)

If there are multiple solutions, the one with the smallest cost in logic gates is chosen.
(The cost calculation takes the CSE into account.)

The result is printed in a human readable format (default) or in variety of other formats, including Verilog, VHDL, C++ and more.

### I haven't understood any of that; speak human!

OK, basically you tell it that you want an electronic circuit that e.g. have input pins `a`, `b`, `c` and output pins `x`, `y`, `z`.
Then for each output you list all the combinations of input values for which the output pin should be 1. (You can also list combinations where output doesn't matter to you because e.g. they will never happen.)
The program then looks for a smallest logic circuit that does that.
(It looks only for the results in the SOP form which means that it always first uses AND gates to calculate a bunch of immediate values and then OR gates to make the final results.)

The popular way to do it by hand for smaller circuits are [Karnaugh maps](https://en.wikipedia.org/wiki/Karnaugh_map).
This program doesn't really do that under the hood. It uses a different (faster) method that gives the same result.
However, the result is displayed (by default) as a Karnaugh map and hence the program name.
There are also other output formats, including Verilog and VHDL (popular formats of hardware description) or even C++ code.

Additionally, the program can find common parts of the resulting circuits and reuse them in multiple places to not waste logic gates for doing the same thing multiple times.
(This is one of the main selling points.)


## Limitations

Currently there is quite a few limitations both because the program is still in development and because of its main purpose which is to be used for the game *Turing Complete*.
 - The search for the solution is done using a **brute force** method which returns the best possible result but often takes too much time and memory to be realistically useful. (Heuristics will be added in the future.)
 - The theoretical limit for the number of input variables is 32, however, the real **input limit is 5 (6 without CSE)** because of how much time the brute-force method takes (unless the logic function is trivial).
 - The number of underlying transistors are not taken into account. Instead **NOT gate, 2-input AND gate and 2-input OR gate** are treated as basic building blocks with respective costs 1, 2 and 2. (Although, this shouldn't change the result dramatically.)
 - **Delay** of the resulting circuit is not taken under account in the deduplication phase (CSE).
 - **Hazards** in the resulting circuit are not taken into account at any stage.
 - The logic function is solved to the **SOP** form. (The POS form is planned for the future.)


## Compilation

The program is written in pure C++, without use of any external libraries, which makes the compilation pretty straightforward.
Any compiler conforming to the C++17 standard should be able to handle it.
(Clang will also work because it can be made conforming by using command line flags.)

You can built it in two ways:

### *SCons* Build script

A script for the build system [*SCons*](https://scons.org/) is provided with the repository.
It requires *Python* and *SCons* to be installed in the system.
(The later one can be installed through *pip* - `pip install scons`.)

To build the program with default settings, just go to the main directory of the project and run the command:
```bash
scons
```

To build it and run the tests, execute the command:
```bash
scons test
```

### Manual build

If you don't want to install any additional software, you can just build all `*.cc` files in the directory `src` and link them together, using your favorite toolchain.
Default options (with optimization turned on) should work fine; there is no libraries to be linked or anything like that.
However, defining the preprocessor macro `NDEBUG` for all files is strongly recommended because assertions can significantly increase the runtime of the resulting program, especially with high number of input variables.
(In most compilers, this is done with the flag `-DNDEBUG`.)


## License

This program is distributed under the MIT license. A full copy of it is available in the `LICENSE` file.


## Input format

The input format is designed to be relatively flexible and easily generated by scripts.

It looks like this:
```
DESCRIPTION_OF_INPUTS

NAME_OF_THE_FIRST_FUNCTION (optional)
MINTERMS_OF_THE_FIRST_FUNCTION
DONT_CARES_OF_THE_FIRST_FUNCTION

NAME_OF_THE_SECOND_FUNCTION (optional)
MINTERMS_OF_THE_SECOND_FUNCTION
DONT_CARES_OF_THE_SECOND_FUNCTION

...
```

The description of inputs is either a list of their names or just their count.

Input names, minterms and don't-cares are lists of numbers separated by whitespaces and/or and punctuation characters except `-` and `_`. (A single dash may be used to indicate an empty list.)

Lines with any letters in them are considered to contain names.

Leading and trailing whitespaces are stripped.

Empty lines and lines starting with `#` (comments) are ignored.

(See [examples](#examples).)

## Examples

### SR latch

Input:
```
set, reset, output
# Note that `output` is considered by the solver to be a normal input.
# IDN, maybe I should pick a clearer first example, whatever.

1, 4, 5
6, 7
```

Output:
```
--- f0 ---

goal:
   0 1 
00 F T
01 F F
11 - -
10 T T

best fit:
   0 1 
00 F T
01 F F
11 T T
10 T T

solution:
set || (!reset && output)

=== optimized solution ===

Negated inputs: reset
Products:
	[0] = !reset && output
Sums:
	"f0" = [0] || set
Gate scores: NOTs = 1, ANDs = 1, ORs = 1
```

### Full adder

Input:
```
# Implementing a full adder as SOP is not the best way but it makes for a good, readable example.

in0 in1 carry-in

sum
1 2 4 7
-

carry-out
3 5 6 7
-
```

Output:
```
--- sum ---

goal:
   0 1 
00 F T
01 T F
11 F T
10 T F

solution:
(in0 && in1 && carry-in) || (in0 && !in1 && !carry-in) || (!in0 && in1 && !carry-in) || (!in0 && !in1 && carry-in)

--- carry-out ---

goal:
   0 1 
00 F F
01 F T
11 T T
10 F T

solution:
(in0 && in1) || (in0 && carry-in) || (in1 && carry-in)

=== optimized solution ===

Negated inputs: in0, in1, carry-in
Products:
	[0] = !in0 && !in1 && carry-in
	[1] = !in0 && in1 && !carry-in
	[2] = in0 && !in1 && !carry-in
	[3] = in1 && carry-in
	[4] = in0 && carry-in
	[5] = in0 && in1
	[6] = in0 && [3]
Sums:
	"sum" = [0] || [1] || [2] || [6]
	"carry-out" = [3] || [4] || [5]
Gate scores: NOTs = 3, ANDs = 10, ORs = 5
```

### Tricky example

Input:
```
# I would need to dive deep into the implementation to try to explain why,
# but this example seems to be particularly hard for solvers to get right.
4
5;11;15
4;7;10;13
```

Output:
```
--- f0 ---

goal:
   00 01 11 10 
00 F  F  F  F
01 -  T  -  F
11 F  -  T  F
10 F  F  T  -

best fit:
   00 01 11 10 
00 F  F  F  F
01 F  T  T  F
11 F  T  T  F
10 F  F  T  F

solution:
(i1 && i3) || (i0 && i2 && i3)

=== optimized solution ===

Negated inputs: <none>
Products:
	[0] = i0 && i2 && i3
	[1] = i1 && i3
Sums:
	"f0" = [0] || [1]
Gate scores: NOTs = 0, ANDs = 3, ORs = 1
```

### More examples...

You can find more examples in the directory `tests`.
