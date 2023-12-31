#include "./info.hh"

#include <iostream>


void printHelp()
{
	std::cout <<
			"This program performs a logic function minimization of a function described as\na list of minterms and don't-cares to a SOP form and after that it performs\na common subexpression elimination.\n"
			"It searches for a solution that uses a minimal number of logic gates.\n"
			"\n"
			"Usage:\tkarnaugh [OPTIONS...]\n"
			"Options:\n"
			"    -h, --help\t\t- Print this help text.\n"
			"    -v, --version\t- Print version information.\n"
			"    -p, --prompt[=X]\t- Set whether hints are shown when the input is read.\n\t\t\t  Valid values are \"never\", \"always\" and \"default\".\n\t\t\t  (No value means \"always\".) By default, hints are shown\n\t\t\t  only when input is read from a TTY.\n"
			"    -P, --no-prompt\t- Same as `--prompt=never`.\n"
			"    -s, --status[=X]\t- Set whether things like the current operation,\n\t\t\t  progress bar, ET, ETA and so on are shown.\n\t\t\t  Valid values are \"never\", \"always\" and \"default\".\n\t\t\t  (No value means \"always\".) By default, they are shown\n\t\t\t  only when the stderr is a TTY.\n"
			"    -S, --no-status\t- Same as `--status=never`.\n"
			"    -O, --no-optimize\t- Skip the common subexpression elimination optimization\n\t\t\t  and show only a raw solution for each function.\n"
			"    -f, --format=X\t- Set the output format. (See \"Output formats\".)\n\t\t\t  (Mathematical formats imply `--no-optimize`.)\n"
			"    -n, --name=X\t- Set module name for Verilog output or entity name for\n\t\t\t  VHDL output or class name for C++ output.\n\t\t\t  (By default, the name of the input file is used,\n\t\t\t  or \"Karnaugh\" if input is read from stdin.)\n"
			"\n"
			"Output formats:\n"
			"\thuman-long\t- The default format which displays all the information\n\t\t\t  in a human-readable way.\n"
			"\thuman\t\t- A format similar to \"human-long\" but it without the\n\t\t\t  graphical representations of Karnaugh maps which\n\t\t\t  normally take most of the vertical space.\n"
			"\thuman-short\t- A minimalistic result of the program without any\n\t\t\t  additional fluff, in a human-readable way.\n"
			"\tverilog\t\t- A Verilog module.\n"
			"\tvhdl\t\t- A VHDL entity.\n"
			"\tcpp\t\t- A C++ class (both a functor and static functions).\n"
			"\tmath-formal\t- The formal mathematical notation. (It uses Unicode.)\n"
			"\tmath-ascii\t- A notation like \"math-formal\" but it uses only ASCII.\n"
			"\tmath-prog\t- A mathematical notation with programming operators.\n"
			"\tmath-names\t- A mathematical notation that uses names of operators.\n"
			"\n"
			"Input:\n"
			"The input is read from the stdin and has the following format:\nINPUTS_DESCRIPTION <line-break> LIST_OF_FUNCTIONS\n"
			"The description of inputs is either a list of their names or just their count.\n"
			"The functions are separated by line breaks and have the following format:\n[NAME <line-break>] LIST_OF_MINTERMS <line-break> LIST_OF_DONT_CARES\n"
			"Input names, minterms and don't-cares are lists of numbers separated by\nwhitespaces and/or and punctuation characters except \"-\" and \"_\".\n(A single dash may be used to indicate an empty list.)\n"
			"Leading and trailing whitespaces are stripped.\n"
			"Lines stating with a letter or an underscore are considered to contain names.\n"
			"Empty lines and lines starting with \"#\" are ignored.\n"
			"\n"
			"An example of input:\n"
			"\n"
			"\ta, b, c, d\n"
			"\t\n"
			"\tName of this function\n"
			"\t# This is quite tricky and many solvers fail to find the best solution.\n"
			"\t5, 11, 15\n"
			"\t4, 7, 10, 13\n"
			"\t\n"
			"\tName of another function\n"
			"\t2, 3, 5, 15\n"
			"\t4, 7, 11, 13\n"
			;
}

void printVersion()
{
	std::cout <<
			"karnaugh (Karnaugh Map Solver) version 0.2.1\n"
			"Author: Piotr Siupa\n"
#ifndef NDEBUG
			"This is a development build which contains additional assertions. This may slow down the execution.\n"
#endif
			;
}