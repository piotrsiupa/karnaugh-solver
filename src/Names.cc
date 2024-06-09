#include "./Names.hh"

#include "utils.hh"


void Names::printRawNames(IndentedOStream &o, const options::FilterSpec::Filter &filter) const
{
	std::vector<std::string> limitedNames;
	limitedNames.reserve(filter.count(names.size()));
	for (std::size_t i = 0; i != names.size(); ++i)
		if (filter[i])
			limitedNames.push_back(names[i]);
	if (!useInCode || options::outputFormat != options::OutputFormat::VERILOG)
		o << joinStrings(limitedNames);
	else
		o << joinStrings(limitedNames, "", " ", ",");
}

void Names::printName(IndentedOStream &o, const std::size_t i) const
{
	if (!useInCode)
	{
		switch (options::outputFormat)
		{
		case options::OutputFormat::CPP:
		case options::OutputFormat::VERILOG:
			o << replacementName << '[' << i << ']';
			return;
		case options::OutputFormat::VHDL:
			o << replacementName << '(' << i << ')';
			return;
		default:
			break;
		}
	}
	if (options::outputFormat == options::OutputFormat::CPP)
		o << replacementName << '.';
	printPlainName(o, i);
}

void Names::printNames(IndentedOStream &o, const options::FilterSpec::Filter &filter) const
{
	if (!useInCode)
	{
		switch (options::outputFormat)
		{
		case options::OutputFormat::VERILOG:
			o << " [" << (names.size() - 1) << ":0] " << replacementName << ',';
			return;
		case options::OutputFormat::VHDL:
			o << replacementName;
			return;
		default:
			break;
		}
	}
	printRawNames(o, filter);
}

void Names::printType(IndentedOStream &o, const options::FilterSpec::Filter &filter) const
{
	switch (options::outputFormat)
	{
	case options::OutputFormat::VHDL:
		if (useInCode)
			o << "std_logic";
		else
			o << "std_logic_vector(" << (names.size() - 1) << " downto 0)";
		break;
	case options::OutputFormat::CPP:
		if (useInCode)
		{
			o << "struct {";
			if (!names.empty())
			{
				o << " bool ";
				printRawNames(o, filter);
				o << ';';
			}
			o << " }";
		}
		else
		{
			o << "std::array<bool, " << names.size() << '>';
		}
		break;
	default:
		// unused
		break;
	}
}
