#include <fstream>
#include <ios>
#include <iostream>
#include <istream>
#include <memory>
#include <string>

#include "global.hh"
#include "info.hh"
#include "Input.hh"
#include "Karnaughs.hh"
#include "non-stdlib-stuff.hh"
#include "options.hh"


static bool parseInputBits(Input &input)
{
	if (options::prompt.getValue())
		std::cerr << "Enter a list of input variables or their count:\n";
	if (input.hasError())
		return false;
	if (!input.hasNext())
	{
		std::cerr << "The description of inputs is missing!\n";
		return false;
	}
	if (input.isNextText())
	{
		Names::names_t names;
		do
			names.emplace_back(input.getWord());
		while (input.hasNextInLine());
		if (names.size() > ::maxBits)
		{
			std::cerr << "Too many input variables!\n";
			return false;
		}
		::bits = static_cast<::bits_t>(names.size());
		::inputNames = Names(true, std::move(names), "i");
	}
	else
	{
		const std::string line = input.getLine();
		if (line == "-")
		{
			::bits = 0;
			::inputNames = Names(true, {}, "i");
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
		Names::names_t names;
		names.reserve(::bits);
		for (bits_t i = 0; i != ::bits; ++i)
			names.push_back("i" + std::to_string(i));
		::inputNames = Names(false, std::move(names), "i");
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
		use_stdin:
		std::ios_base::sync_with_stdio(false);
		std::cin.tie(nullptr);
		return {&std::cin, deleteIstream};
	}
	else if (options::freeArgs.size() == 1)
	{
		const std::string path(options::freeArgs.front());
		if (path == "-")
		{
			goto use_stdin;
		}
		else
		{
			::inputFilePath = options::freeArgs.front();
			IstreamUniquePtr ifstream(new std::ifstream(path, std::ios_base::in | std::ios_base::binary), deleteIstream);
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
	try
	{
		Input input(*istream);
		if (!parseInputBits(input))
			return false;
		Input::recomputeMintermSize();
		if (!karnaughs.loadData(input))
			return false;
		if (input.hasError())
			return false;
	}
	catch (Input::Error &)
	{
		return false;
	}
	return true;
}

static bool processInput(IstreamUniquePtr istream)
{
	Karnaughs karnaughs;
	if (!loadInput(std::move(istream), karnaughs))
		return false;
	std::move(karnaughs).solve();
	karnaughs.print();
	return true;
}

int main(const int argc, const char *const *const argv)
{
	Progress::init();
	
	if (!options::parse(argc, argv))
		return 1;
	if (options::outputFormat.getValue() == options::OutputFormat::MATH_FORMAL || options::outputFormat.getValue() == options::OutputFormat::MATH_PROG || options::outputFormat.getValue() == options::OutputFormat::MATH_ASCII || options::outputFormat.getValue() == options::OutputFormat::MATH_NAMES)
		options::skipOptimization.raise();
	
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
	
	enableAnsiSequences();
	
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
