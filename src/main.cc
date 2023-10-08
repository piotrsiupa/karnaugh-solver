#include <cstring>
#include <iostream>
#include <string>

#include "global.hh"
#include "input.hh"
#include "Karnaughs.hh"


static void printHelp()
{
	std::cout << "This program solves Karnough maps using a brute-force approach, searching for a solution that uses a minimal number of logic gates. ";
	std::cout << "(This includes searching for repeating parts of the resulting circuit to reuse them in multiple places.)\n";
	std::cout << "It can accept many functions at once to make the most of the deduplication optimization.\n";
	std::cout << "The input format is highly flexible and easy to generate by a script. ";
	std::cout << "The solution is printed in a human redable format.\n";
	
	std::cout << '\n';
	
	std::cout << "Usage:\tkarnough [OPTIONS...]\n";
	std::cout << "Options:\n";
	std::cout << "\t--help\t\t- Print this help text.\n";
	std::cout << "\t--version\t- Print version information.\n";
	
	std::cout << '\n';
	
	std::cout << "Input:\n";
	std::cout << "The input is read from the stdin and has the following format: INPUT_NAMES <line-break> LIST_OF_FUNCTIONS\n";
	std::cout << "The functions are separated by line breaks and have the following format: [NAME <line-break>] LIST_OF_MINTERMS <line-break> LIST_OF_DONT_CARES\n";
	std::cout << "Both minterms and don't-cares are lists of numbers separated by whitespaces and/or punctation charaters. A single dash may be used to indicate an empty list.\n";
	std::cout << "Names have to start with a non-digit character.\n";
	std::cout << "Leading and traling whitespaces are stripped.\n";
	std::cout << "Empty lines and lines starting with \"#\" are ignored.\n";
	
	std::cout << '\n';
	
	std::cout << "An example of input:\n";
	std::cout << '\n';
	std::cout << "\ta, b, c, d\n";
	std::cout << "\t\n";
	std::cout << "\tName of this function\n";
	std::cout << "\t# This function is quite tricky and many solvers fail to find the best solution.\n";
	std::cout << "\t5, 11, 15\n";
	std::cout << "\t4, 7, 10, 13\n";
	std::cout << "\t\n";
	std::cout << "\tName of another function\n";
	std::cout << "\t2, 3, 5, 15\n";
	std::cout << "\t4, 7, 11, 13\n";
}

static void printVersion()
{
	std::cout << "karnaugh (Karnaugh Map Solver) version 0.0.0\n";
	std::cout << "Author: Piotr Siupa\n";
#ifndef NDEBUG
	std::cout << "This is a development build which contains additional assertions. This may slow down the execution.\n";
#endif
}

static bool solveInput(std::istream &input)
{
	lines_t lines;
	if (!loadLines(input, lines))
		return false;
		
	if (lines.empty())
	{
		std::cerr << "Input names are missing!\n";
		return false;
	}
	::inputNames = readStrings(lines.front());
	if (::inputNames.size() > ::maxBits)
	{
		std::cerr << "Too many variables!\n";
		return false;
	}
	lines.pop_front();
	
	::bits = ::inputNames.size();
	
	if (!Karnaughs::solve(lines))
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
	
	if (!solveInput(std::cin))
		return 1;
	
	return 0;
}
