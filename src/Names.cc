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

void Names::printCppType(std::ostream &o) const
{
	if (useInCode)
	{
		o << "struct { ";
		if (!names.empty())
		{
			o << "bool ";
			bool first = true;
			for (const std::string &name : names)
			{
				if (first)
					first = false;
				else
					o << ", ";
				o << name;
			}
			o << "; ";
		}
		o << '}';
	}
	else
	{
		o << "std::array<bool, " << names.size() << '>';
	}
}

void Names::printMathNames(std::ostream &o) const
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
