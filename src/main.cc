#include <cstring>
#include <iostream>
#include <string>

#include "global.hh"
#include "Input.hh"
#include "Karnaughs.hh"
#include "non-stdlib-stuff.hh"
#include "Progress.hh"


static void printHelp()
{
	std::cout <<
			"This performs logic function minimization of a function described as a list of\nminterms and don't-cares to a SOP form followed by common subexpression\nelimination, using a brute-force approach.\n"
			"It searches for a solution that uses a minimal number of logic gates.\n"
			"It can accept many functions at once to make the most of the CSE.\n"
			"The input format is flexible and easy to generate by a script.\n"
			"The solution is printed in a human readable format.\n"
			"\n"
			"Usage:\tkarnough [OPTIONS...]\n"
			"Options:\n"
			"\t--help\t\t- Print this help text.\n"
			"\t--version\t- Print version information.\n"
			"\n"
			"Input:\n"
			"The input is read from the stdin and has the following format:\nINPUTS_DESCRIPTION <line-break> LIST_OF_FUNCTIONS\n"
			"The description of inputs is either a list of their names or just their count.\n"
			"The functions are separated by line breaks and have the following format:\n[NAME <line-break>] LIST_OF_MINTERMS <line-break> LIST_OF_DONT_CARES\n"
			"Input names, minterms and don't-cares are lists of numbers separated by\nwhitespaces and/or and punctuation characters except \"-\" and \"_\".\n(A single dash may be used to indicate an empty list.)\n"
			"Lines with any letters in them are considered to contain names.\n"
			"Leading and trailing whitespaces are stripped.\n"
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

static void printVersion()
{
	std::cout <<
			"karnaugh (Karnaugh Map Solver) version 0.1.2\n"
			"Author: Piotr Siupa\n"
#ifndef NDEBUG
			"This is a development build which contains additional assertions. This may slow down the execution.\n"
#endif
			;
}

static bool parseInputBits(Input &input)
{
	if (::terminalStdin)
		std::cerr << "Enter a list of input variables or their count:\n";
	if (input.hasError())
		return false;
	if (input.isEmpty())
	{
		std::cerr << "The description of inputs is missing!\n";
		return false;
	}
	if (input.isName())
	{
		Progress progress(Progress::Stage::LOADING, "Loading input names", 1);
		progress.step();
		::inputNames = input.popParts(progress);
		if (::inputNames.size() > ::maxBits)
		{
			std::cerr << "Too many input variables!\n";
			return false;
		}
		::bits = static_cast<::bits_t>(::inputNames.size());
	}
	else
	{
		const std::string line = input.popLine();
		if (line == "-")
		{
			::bits = 0;
			return true;
		}
		try
		{
			const int n = std::stoi(line);
			if (n < 0)
			{
				std::cerr << '"' << line << "\" is negative!\n";
				return false;
			}
			else if (n > ::maxBits)
			{
				std::cerr << "Too many input variables!\n";
				return false;
			}
			::bits = static_cast<::bits_t>(n);
		}
		catch (std::invalid_argument &)
		{
			std::cerr << '"' << line << "\" is not a number!\n";
			return false;
		}
		catch (std::out_of_range &)
		{
			std::cerr << '"' << line << "\" is out of range!\n";
			return false;
		}
		for (bits_t i = 0; i != ::bits; ++i)
			::inputNames.push_back("i" + std::to_string(i));
	}
	::maxMinterm = ::bits == 0 ? 0 : ((Minterm(1) << (::bits - 1)) - 1) * 2 + 1;
	return true;
}

static bool solveInput(std::istream &istream)
{
	Input input(istream);
	if (!parseInputBits(input))
		return false;
	if (!Karnaughs::solve(input))
		return false;
	return true;
}

int main(const int argc, const char *const *const argv)
{
	if (argc > 1 && std::strcmp(argv[1], "--help") == 0)
	{
		printHelp();
		return 0;
	}
	else if (argc > 1 && std::strcmp(argv[1], "--version") == 0)
	{
		printVersion();
		return 0;
	}
	
	::terminalStdin = isStdinTerminal();
	::terminalStderr = isStderrTerminal();
	
	if (!solveInput(std::cin))
		return 1;
	
	return 0;
}
