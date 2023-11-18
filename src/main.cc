#include <fstream>
#include <iostream>
#include <istream>
#include <memory>
#include <string>

#include "global.hh"
#include "Input.hh"
#include "Karnaughs.hh"
#include "non-stdlib-stuff.hh"
#include "options.hh"
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
			"Usage:\tkarnaugh [OPTIONS...]\n"
			"Options:\n"
			"    -h, --help\t\t- Print this help text.\n"
			"    -v, --version\t- Print version information.\n"
			"    -p, --prompt[=X]\t- Set whether hints are shown when the input is read.\n\t\t\t  Valid values are \"never\", \"always\" and \"default\".\n\t\t\t  (No value means \"always\".) By default, hints are shown\n\t\t\t  only when input is read from a TTY.\n"
			"    -P, --no-prompt\t- Same as `--prompt=never`.\n"
			"    -s, --status[=X]\t- Set whether things like the current operation,\n\t\t\t  progress bar, ET, ETA and so on are shown.\n\t\t\t  Valid values are \"never\", \"always\" and \"default\".\n\t\t\t  (No value means \"always\".) By default, they are shown\n\t\t\t  only when the stderr is a TTY.\n"
			"    -S, --no-status\t- Same as `--status=never`.\n"
			"    -O, --no-optimize\t- Skip the common subexpression elimination optimization\n\t\t\t  and only show a raw solution for each function.\n"
			"    -f, --format=X\t- Set the output format. (See \"Output formats\".)\n"
			"    -n, --name=X\t- Set module name for Verilog output.\n\t\t\t  (By default, the name of the input file is used,\n\t\t\t  or \"Karnaugh\" if input is read from stdin.)\n"
			"\n"
			"Output formats:\n"
			"\thuman-long\t- The default format which displays all the information\n\t\t\t  in a human-readable way.\n"
			"\thuman\t\t- It's similar to \"human-long\" but it doesn't print\n\t\t\t  graphical representations of Karnaugh maps which\n\t\t\t  normally take most of the vertical space.\n"
			"\thuman-short\t- Only the result of the program without any additional\n\t\t\t  fluff, in a human-readable way.\n"
			"\tverilog\t\t- A Verilog module.\n"
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
			"karnaugh (Karnaugh Map Solver) version 0.1.3\n"
			"Author: Piotr Siupa\n"
#ifndef NDEBUG
			"This is a development build which contains additional assertions. This may slow down the execution.\n"
#endif
			;
}

static bool parseInputBits(Input &input)
{
	if (options::prompt.getValue())
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
			progress.cerr() << "Too many input variables!\n";
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

static void deleteIstream(const std::istream *const stream) {
	if (stream != &std::cin)
		delete stream;
}
using IstreamUniquePtr = std::unique_ptr<std::istream, decltype(&deleteIstream)>;
static IstreamUniquePtr prepareIstream()
{
	if (options::freeArgs.empty())
	{
		return {&std::cin, deleteIstream};
	}
	else if (options::freeArgs.size() == 1)
	{
		const std::string path(options::freeArgs.front());
		if (path == "-")
		{
			return {&std::cin, deleteIstream};
		}
		else
		{
			::inputFilePath = options::freeArgs.front();
			IstreamUniquePtr ifstream(new std::ifstream(path), deleteIstream);
			if (!*ifstream)
			{
				std::cerr << "Cannot open \"" << path << "\"!\n";
				return {nullptr, deleteIstream};
			}
			return ifstream;
		}
	}
	else
	{
		std::cerr << "Too many arguments!\n";
		return {nullptr, deleteIstream};
	}
}

static bool loadInput(IstreamUniquePtr istream, Karnaughs &karnaughs)
{
	Input input(*istream);
	if (!parseInputBits(input))
		return false;
	if (!karnaughs.loadData(input))
		return false;
	return true;
}

static bool processInput(IstreamUniquePtr istream)
{
	Karnaughs karnaughs;
	if (!loadInput(std::move(istream), karnaughs))
		return false;
	karnaughs.solve();
	karnaughs.print();
	return true;
}

int main(const int argc, const char *const *const argv)
{
	if (!options::parse(argc, argv))
		return 1;
	
	if (options::help.isRaised())
	{
		printHelp();
		return 0;
	}
	else if (options::version.isRaised())
	{
		printVersion();
		return 0;
	}
	
	IstreamUniquePtr istream = prepareIstream();
	if (!istream)
		return 1;
	
	::terminalStdin = isStdinTerminal();
	::terminalInput = ::terminalStdin && istream.get() == &std::cin;
	::terminalStderr = isStderrTerminal();
	
	if (!processInput(std::move(istream)))
		return 1;
	
	return 0;
}
