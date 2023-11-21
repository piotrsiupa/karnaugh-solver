#include "./Names.hh"


void Names::printVerilogNames(std::ostream &o, const std::string_view replacementName) const
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
