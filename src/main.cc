#include <fstream>
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
#include "Progress.hh"


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
		Names::names_t names = input.popParts(progress);
		if (names.size() > ::maxBits)
		{
			Progress::cerr() << "Too many input variables!\n";
			return false;
		}
		::bits = static_cast<::bits_t>(names.size());
		::inputNames = Names(true, std::move(names), "i");
	}
	else
	{
		const std::string line = input.popLine();
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
	Progress::init();
	
	if (!options::parse(argc, argv))
		return 1;
	if (options::outputOperators.isSet() && !options::outputFormat.supportsOperatorStyles())
	{
		std::cerr << "Style of operators cannot be specified for the output format \"" << options::outputFormat.getMappedName() << "\"!\n";
		return 1;
	}
	
	if (options::helpOptions.isRaised())  // `--help-options` is before `--help` because it should be used when both flags are present.
	{
		printShortHelp();
		return 0;
	}
	else if (options::help.isRaised())
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
