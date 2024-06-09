#include "./info.hh"

#include <iostream>


static constexpr std::string_view internalName = "karnaugh";
std::string_view getInternalName() { return internalName; }

static constexpr std::string_view fullName = "Karnaugh Map Solver";
std::string_view getFullName() { return fullName; }

static constexpr std::string_view versionNumber = "0.3.2";
std::string_view getVersionNumber() { return versionNumber; }

static constexpr std::string_view author = "Piotr Siupa";
std::string_view getAuthor() { return author; }

void printShortHelp()
{
	std::cout <<
			"This program performs a logic function minimization of a function described as\na list of minterms and don't-cares to a SOP form and after that it performs\na common subexpression elimination.\n"
			"It searches for a solution that uses a minimal number of logic gates.\n"
			"\n"
			"Usage:\t" << internalName << " [OPTIONS...] [--] [INPUT_FILE]\n"
			"Options:\n"
			" general:\n"
			"    -h, --help\t\t- Print full help text, including lists of option\n\t\t\t  arguments and description of the input format.\n"
			"    -H, --help-options\t- Print shortened help text, truncated at options.\n"
			"    -v, --version\t- Print version information.\n"
			" user interface:\n"
			"    -p, --prompt[=X]\t- Set whether hints are shown when the input is read.\n\t\t\t  Valid values are \"always\", \"never\" and \"default\".\n\t\t\t  (No value means \"always\".) By default, hints are shown\n\t\t\t  only when input is read from a TTY.\n"
			"    -P, --no-prompt\t- Same as `--prompt=never`.\n"
			"    -s, --status[=X]\t- Set whether things like the current operation,\n\t\t\t  progress bar, ET, ETA and so on are shown.\n\t\t\t  Valid values are \"always\", \"never\" and \"default\".\n\t\t\t  (No value means \"always\".) By default, they are shown\n\t\t\t  only when the stderr is a TTY.\n"
			"    -S, --no-status\t- Same as `--status=never`.\n"
			" output:\n"
			"    -b, --banner\t- Enable printing a line with version info and a list of\n\t\t\t  options on top of the output. (Enabled by default.)\n\t\t\t  (It's useful for reproducing errors.)\n"
			"    -B, --no-banner\t- Disable the `--banner`.\n"
			"    -f, --format=X\t- Set the output format. (See \"Output formats\".)\n\t\t\t  (Mathematical formats imply `--no-optimize`.)\n"
			"    -o, --output-ops=X\t- Set style of operators to be used in output (if the\n\t\t\t  output format allows this). (See \"Output operators\".)\n"
			"    -I, --indent=X\t- Set number of spaces used to indent output. `0` means\n\t\t\t  \"use tabs\" and it's chosen by default. Adding prefix\n\t\t\t  \"-\" to the number turns off indenting empty lines.\n"
			"    -n, --name=X\t- Set module name for Verilog output or entity name for\n\t\t\t  VHDL output or class name for C++ output.\n\t\t\t  (By default, the name of the input file is used,\n\t\t\t  or \"Karnaugh\" if input is read from stdin.)\n"
			"    -G, --verbose-graph\t- Show shows all inputs in every node of a graph output,\n\t\t\t  instead only the ones added by that node. (It's more\n\t\t\t  readable but less useful for building the circuit.)\n"
			"    -l, --limit\t\t- Select functions that will be present in the output\n\t\t\t  (all by default). This only affects outputs and it's\n\t\t\t  meant to aid humans in reading them (mainly graphs).\n\t\t\t  Outputs with different \"limits\" are consistent.\n"
			" 3rd stage - common subexpression elimination:\n"
			"    -O, --no-optimize\t- Skip the common subexpression elimination optimization\n\t\t\t  and show only a raw solution for each function.\n"
		;
}

void printHelp()
{
	printShortHelp();
	std::cout <<
			"\n"
			"\n"
			"Output formats:\n"
			"\thuman-long\t- The default format which displays all the information\n\t\t\t  in a human-readable way.\n"
			"\thuman\t\t- A format similar to \"human-long\" but it without the\n\t\t\t  graphical representations of Karnaugh maps which\n\t\t\t  normally take most of the vertical space.\n"
			"\thuman-short\t- A minimalistic result of the program without any\n\t\t\t  additional fluff, in a human-readable way.\n"
			"\tgraph\t\t- `human-short` as a graph in the DOT language.\n"
			"\treduced-graph\t- Similar `graph` but input nodes are omitted for\n\t\t\t  readability. (Instead, they are shown as labels.)\n"
			"\tverilog\t\t- A Verilog module.\n"
			"\tvhdl\t\t- A VHDL entity.\n"
			"\tcpp\t\t- A C++ class (both a functor and static functions).\n"
			"\tmathematical\t- The formal mathematical notation.\n"
			"\tgate-costs\t- Only gate costs. (Useful mostly for development.)\n"
			"\n"
			"\n"
			"Output operators:\n"
			"\tformal\t\t- The proper mathematical symbols. (Unicode)\n"
			"\tascii\t\t- Similar to the \"formal\" but made out of exclusively\n\t\t\t  ASCII characters. (compatibility mode)\n"
			"\tprogramming\t- Programming operators (like in C or Java).\n"
			"\tnames\t\t- Names of operators (uppercase).\n"
			"\n"
			"\n"
			"Input:\n"
			"The input format is similar to CSV but less constrained in some ways.\n"
			"It has the following format:\nINPUTS_DESCRIPTION <line-break> LIST_OF_FUNCTIONS\n"
			"- The description of inputs is either a list of their names or just their count.\n"
			"- The functions are separated by line breaks and are defined like this:\n  [NAME <line-break>] LIST_OF_MINTERMS <line-break> LIST_OF_DONT_CARES\n"
			"- Input names, minterms and don't-cares are lists of numbers separated by\n  whitespaces and/or and punctuation characters except \"-\", \"_\" and \"#\".\n  (A single dash may be used to indicate an empty list.)\n"
			"- Lines with any letters in them are considered to contain names.\n"
			"- Leading and trailing whitespaces are stripped as well as everything after the\n  \"#\" character (comments).\n"
			"- Blank lines and comment-only lines are ignored.\n"
			"\n"
			"An example of an input:\n"
			"\n"
			"\t# Notice various ways of separating the values.\n"
			"\t# (This is a comment btw; it will be ignored.)\n"
			"\ta, b, c, d\n"
			"\t\n"
			"\t Name of this function  # A valid name but bad name for generating code.\n"
			"\t  0; 5; 13  # Leading and training whitespaces are ignored.\n"
			"\t1; 2; 4; 15\n"
			"\t\n"
			"\t# Name is skipped for this one\n"
			"\t5 14\n"
			"\t0 1 3 4 7, 10;11 12 ,13 15  # You can mix different separators.\n"
		;
}

void printVersion()
{
	std::cout <<
			internalName << " (" << fullName << ") version " << versionNumber << "\n"
			"Author: " << author << "\n"
#ifndef NDEBUG
			"This is a development build which contains additional assertions. This may slow down the execution.\n"
#endif
			;
}
