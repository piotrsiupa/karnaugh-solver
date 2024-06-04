#include "./Names.hh"

#include "options.hh"
#include "utils.hh"


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

void Names::printNames(IndentedOStream &o) const
{
	if (useInCode)
	{
		if (options::outputFormat == options::OutputFormat::VERILOG)
		{
			o << joinStrings(names, "", " ", ",");
			return;
		}
	}
	else
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
	o << joinStrings(names);
}

void Names::printType(IndentedOStream &o) const
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
				o << " bool " << joinStrings(names) << ';';
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
