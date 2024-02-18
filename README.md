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
Any compiler conforming to the C++20 standard should be able to handle it.
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
scons --dev test
```

You can see other options by running:
```bash
scons --help
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
It's similar to CSV but less constrained in some ways.

A program input looks like this:
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

Leading and trailing whitespaces are stripped.

Lines stating with a letter or an underscore are considered to contain names.

Empty lines and lines starting with `#` (comments) are ignored.


## Example (and output formats)

This example was chosen to show also various ways in which some other solvers can generate suboptimal solutions.

(The exact outputs may differ from the presented ones depending on options given to the program. You can see more variants [here](tests/tricky_one).)

You can find more "examples" in the directory [`tests`](tests).

### Input

Each part was formatted a little differently to show various styles allowed by the parser.
(If you want, you can even use mix and match them in a single line.)

([file](tests/tricky_one/input))
```
# This tests is used as the example in `README.md`.
# Notice various ways of separating the values.
a, b, c, d

# Solvers tend to get a solution for this one along the lines of a'b'd' + bc'd,
# instead of the optimal one: a'c' + abd. (1 less AND and 2 less NOTs)
 tricky_0    
  0; 5; 13   
  1; 2; 4; 15

# Solvers tend to not be biased against using NOT gates which often results in
# solution a'c' + ac, instead of bd + ab. (2 less NOTs)
# Beside that, this solution can reuse parts of the previous one reducing the
# amount of gates even further. (2 less ANDs)
tricky_1
5 14
0 1 3 4 7 10 11 12 13 15
```

### Human readable output (long variant)

This is the default output format of the program.
It's intended to visualize the solution in form of Karnaugh maps (unless they are too big) as well as show steps to build it out of logic gates in a way that is readable by a human.
(See the [short variant](#human-readable-output-short-variant) for more detailed explanations.)

([file](tests/tricky_one/--format=human-long))
```
--- tricky_0 ---

goal:
   00 01 11 10 
00 T  -  F  -
01 -  T  F  F
11 F  T  -  F
10 F  F  F  F

best fit:
   00 01 11 10 
00 T  T  F  F
01 T  T  F  F
11 F  T  T  F
10 F  F  F  F

solution:
(!a && !c) || (a && b && d)

--- tricky_1 ---

goal:
   00 01 11 10 
00 -  -  -  F
01 -  T  -  F
11 -  -  -  T
10 F  F  -  -

best fit:
   00 01 11 10 
00 T  T  F  F
01 T  T  F  F
11 T  T  T  T
10 F  F  F  F

solution:
(a && b) || (!a && !c)

=== optimized solution ===

Negated inputs: a, c
Products:
	[0] = a && b
	[1] = d && [0]
	[2] = !a && !c
Sums:
	"tricky_0" = [1] || [2]
	"tricky_1" = [0] || [2]
Gate cost: NOTs = 2, ANDs = 3, ORs = 2
```

### Human readable output (medium variant)

This is the same as the long variant except it doesn't print functions represented as Karnaugh maps (which significantly reduces the output length).

([file](tests/tricky_one/--format=human))

### Human readable output (short variant)

This is a shorter version of the human readable format.
It is useful when all you need are instruction on how to build the circuit.

It first lists all the inputs that will need to be negated. (NOT gates)

Then it lists products, each of which containing at least 2 inputs, negated inputs or outputs of other products. (AND gates)

Finally, it lists all the sums, in a format similar to the products but they can also use outputs of other sums. (OR gates)
Sums can be either numbered or named. The named ones are the outputs of the circuit.

([file](tests/tricky_one/--format=human-short))
```
Negated inputs: a, c
Products:
	[0] = a && b
	[1] = d && [0]
	[2] = !a && !c
Sums:
	"tricky_0" = [1] || [2]
	"tricky_1" = [0] || [2]
```

### Mathematical

This is a minimalistic output format that just prints the functions using the formal mathematical notation.

This format cannot be used with common subexpression elimination. (It implies `--no-optimize`.)

It uses Unicode characters for boolean algebra operators, which may interfere with some programs like older terminals.
To bypass that issue, other variants of this output format are available. They replace the Unicode characters with [ASCII art](tests/tricky_one/--format=math-ascii), [names of operations](tests/tricky_one/--format=math-names) or [programming operators](tests/tricky_one/--format=math-prog).

([file](tests/tricky_0/--format=math-formal)
```
tricky_0(a, b, c, d) = (¬a ∧ ¬c) ∨ (a ∧ b ∧ d)
tricky_1(a, b, c, d) = (a ∧ b) ∨ (b ∧ d)
```

### Verilog

([file](tests/tricky_one/--format=verilog))
```
module tricky_one (
	input wire a, b, c, d,
	output wire tricky_0, tricky_1,
);
	
	// Internal signals
	wire [2:0] prods;
	wire [1:0] sums;
	
	// Products
	assign prods[0] = a & b;
	assign prods[1] = d & prods[0];
	assign prods[2] = !a & !c;
	
	// Sums
	assign sums[0] = prods[1] | prods[2];
	assign sums[1] = prods[0] | prods[2];
	
	// Results
	assign tricky_0 = sums[0];
	assign tricky_1 = sums[1];
	
endmodule
```

### VHDL

([file](tests/tricky_one/--format=vhdl))
```
library IEEE;
use IEEE.std_logic_1164.all;

entity tricky_one is
	port(
		a, b, c, d : in std_logic;
		tricky_0, tricky_1 : out std_logic
	);
end tricky_one;

architecture behavioural of tricky_one is
	-- Internal signals
	signal prods : std_logic_vector(2 downto 0);
	signal sums : std_logic_vector(1 downto 0);
	
begin
	
	-- Products
	prods(0) <= a and b;
	prods(1) <= d and prods(0);
	prods(2) <= not a and not c;
	
	-- Sums
	sums(0) <= prods(1) or prods(2);
	sums(1) <= prods(0) or prods(2);
	
	-- Results
	tricky_0 <= sums(0);
	tricky_1 <= sums(1);
	
end behavioural;
```

### Other formats

It is also possible to export the result as [C++](tests/tricky_one/--format=cpp) but this may produce worse results that just hardcoding a lookup table.
It may be beneficial in situations when the boolean function is highly susceptible to optimization.
