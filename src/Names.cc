#include "./Names.hh"


void Names::printVerilogNames(std::ostream &o) const
{
	if (useInCode)
	{
		for (const std::string &name : names)
			o << ' ' << name << ',';
	}
	else
	{
		o << " [" << (names.size() - 1) << ":0] " << replacementName << ',';
	}
}

void Names::printVhdlNames(std::ostream &o) const
{
	if (useInCode)
	{
		bool first = true;
		for (const std::string &name : names)
		{
			if (first)
				first = false;
			else
				o << ", ";
			o << name;
		}
	}
	else
	{
		o << replacementName;
	}
}

void Names::printVhdlType(std::ostream &o) const
{
	if (useInCode)
		o << "std_logic";
	else
		o << "std_logic_vector(" << (names.size() - 1) << " downto 0)";
}
