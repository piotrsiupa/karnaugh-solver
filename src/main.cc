#include <cstring>
#include <iostream>
#include <string>

#include <unistd.h>

#include "global.hh"
#include "Input.hh"
#include "Karnaughs.hh"


static void printHelp()
{
	std::cout << "This program solves Karnough maps using a brute-force approach, searching for a\nsolution that uses a minimal number of logic gates. (This includes searching\nfor repeating parts of the resulting circuit to reuse them in multiple places.)\n";
	std::cout << "It can accept many functions at once to make the most of the deduplication.\n";
	std::cout << "The input format is flexible and easy to generate by a script.\n";
	std::cout << "The solution is printed in a human readable format.\n";
	
	std::cout << '\n';
	
	std::cout << "Usage:\tkarnough [OPTIONS...]\n";
	std::cout << "Options:\n";
	std::cout << "\t--help\t\t- Print this help text.\n";
	std::cout << "\t--version\t- Print version information.\n";
	
	std::cout << '\n';
	
	std::cout << "Input:\n";
	std::cout << "The input is read from the stdin and has the following format:\nINPUTS_DESCRIPTION <line-break> LIST_OF_FUNCTIONS\n";
	std::cout << "The description of inputs is either a list of their names or just their count.\n";
	std::cout << "The functions are separated by line breaks and have the following format:\n[NAME <line-break>] LIST_OF_MINTERMS <line-break> LIST_OF_DONT_CARES\n";
	std::cout << "Input names, minterms and don't-cares are lists of numbers separated by\nwhitespaces and/or and punctation charaters except \"-\" and \"_\".\n(A single dash may be used to indicate an empty list.)\n";
	std::cout << "Lines with any letters in them are considered to contain names.\n";
	std::cout << "Leading and traling whitespaces are stripped.\n";
	std::cout << "Empty lines and lines starting with \"#\" are ignored.\n";
	
	std::cout << '\n';
	
	std::cout << "An example of input:\n";
	std::cout << '\n';
	std::cout << "\ta, b, c, d\n";
	std::cout << "\t\n";
	std::cout << "\tName of this function\n";
	std::cout << "\t# This is quite tricky and many solvers fail to find the best solution.\n";
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

static bool isInputTerminal()
{
	return isatty(STDIN_FILENO);
}

static bool parseInputBits(Input &input)
{
	if (::inputTerminal)
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
		::inputNames = input.popParts();
		if (::inputNames.size() > ::maxBits)
		{
			std::cerr << "Too many input variables!\n";
			return false;
		}
		::bits = ::inputNames.size();
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
			::bits = n;
		}
		catch (std::invalid_argument &e)
		{
			std::cerr << '"' << line << "\" is not a number!\n";
			return false;
		}
		for (bits_t i = 0; i != ::bits; ++i)
			::inputNames.push_back("i" + std::to_string(i));
	}
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
	
	::inputTerminal = isInputTerminal();
	
	if (!solveInput(std::cin))
		return 1;
	
	return 0;
}
