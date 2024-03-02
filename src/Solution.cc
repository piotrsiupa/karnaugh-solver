#include "./Solution.hh"

#include "options.hh"


Minterms Solution::getMinterms() const
{
	Minterms minterms;
	for (const Implicant &implicant : *this)
		implicant.addToMinterms(minterms);
	return minterms;
}

void Solution::printHuman(std::ostream &o) const
{
	if (size() == 1)
	{
		front().printHuman(o, false);
	}
	else
	{
		for (const Implicant &implicant : *this)
		{
			if (&implicant != &front())
				o << " || ";
			implicant.printHuman(o, true);
		}
	}
	o << '\n';
	if (options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT)
	{
		o << '\n';
		printGateCost(o, false);
	}
}

void Solution::printVerilog(std::ostream &o) const
{
	if (size() == 1)
	{
		front().printVerilog(o, false);
	}
	else
	{
		for (const Implicant &implicant : *this)
		{
			if (&implicant != &front())
				o << " | ";
			implicant.printVerilog(o, true);
		}
	}
}

void Solution::printVhdl(std::ostream &o) const
{
	if (size() == 1)
	{
		front().printVhdl(o, false);
	}
	else
	{
		for (const Implicant &implicant : *this)
		{
			if (&implicant != &front())
				o << " or ";
			implicant.printVhdl(o, true);
		}
	}
}

void Solution::printCpp(std::ostream &o) const
{
	if (size() == 1)
	{
		front().printCpp(o, false);
	}
	else
	{
		for (const Implicant &implicant : *this)
		{
			if (&implicant != &front())
				o << " || ";
			implicant.printCpp(o, true);
		}
	}
}

void Solution::printMath(std::ostream &o) const
{
	if (size() == 1)
	{
		front().printMath(o, false);
	}
	else
	{
		for (const Implicant &implicant : *this)
		{
			if (&implicant != &front())
			{
				switch (options::outputFormat.getValue())
				{
				case options::OutputFormat::MATH_FORMAL:
					o << " \u2228 ";
					break;
				case options::OutputFormat::MATH_ASCII:
					o << " \\/ ";
					break;
				case options::OutputFormat::MATH_PROG:
					o << " || ";
					break;
				case options::OutputFormat::MATH_NAMES:
					o << " OR ";
					break;
				default:
					break;
				}
			}
			implicant.printMath(o, true);
		}
	}
}

#ifndef NDEBUG
bool Solution::covers(const Minterm minterm) const
{
	for (const Implicant &implicant : *this)
		if (implicant.covers(minterm))
			return true;
	return false;
}
#endif
