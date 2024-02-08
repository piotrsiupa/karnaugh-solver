#include "./Implicants.hh"

#include <algorithm>

#include "options.hh"


Implicants& Implicants::humanSort()
{
	std::ranges::sort(*this, [](const Implicant &x, const Implicant &y){ return x.humanLess(y); });
	return *this;
}

void Implicants::printHuman(std::ostream &o) const
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
}

void Implicants::printVerilog(std::ostream &o) const
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

void Implicants::printVhdl(std::ostream &o) const
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

void Implicants::printCpp(std::ostream &o) const
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

void Implicants::printMath(std::ostream &o) const
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
bool Implicants::covers(const Minterm minterm) const
{
	for (const Implicant &implicant : *this)
		if (implicant.covers(minterm))
			return true;
	return false;
}
#endif
