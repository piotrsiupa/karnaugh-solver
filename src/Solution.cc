#include "./Solution.hh"

#include "options.hh"
#include "utils.hh"


void Solution::printGraphNegatedInputs(std::ostream &o, const std::size_t functionNum) const
{
	std::vector<std::vector<std::size_t>> negatedInputs(::bits);
	for (std::size_t i = 0; i != size(); ++i)
	{
		const auto falseBits = (*this)[i].getFalseBits();
		for (Minterm j = 0; j != ::bits; ++j)
			if ((falseBits & (1 << (::bits - j - 1))) != 0)
				negatedInputs[j].push_back(i);
	}
	if (std::any_of(negatedInputs.cbegin(), negatedInputs.cend(), [](const std::vector<std::size_t> &x){ return !x.empty(); }))
	{
		o << "\t\tsubgraph negated_inputs\n";
		o << "\t\t{\n";
		o << "\t\t\tnode [shape=diamond];\n";
		o << "\t\t\tedge [taillabel=\"!\"];\n";
		for (Minterm i = 0; i != ::bits; ++i)
		{
			for (const std::size_t j : negatedInputs[i])
			{
				o << "\t\t\tf" << functionNum << "_ni" << i << '_' << j << " [label=\"!";
				::inputNames.printGraphName(o, i);
				o << "\"];\n";
				o << "\t\t\tf" << functionNum << "_ni" << i << '_' << j << " -> i" << i << ";\n";
			}
		}
		o << "\t\t}\n";
	}
}

std::size_t Solution::printGraphProducts(std::ostream &o, const std::size_t functionNum, std::size_t idShift) const
{
	if (std::any_of(cbegin(), cend(), [](const Implicant &x){ return !x.empty(); }))
	{
		o << "\t\tsubgraph products\n";
		o << "\t\t{\n";
		o << "\t\t\tnode [shape=ellipse];\n";
		o << "\t\t\tedge [taillabel=\"&&\"];\n";
		for (std::size_t i = 0; i != size(); ++i)
		{
			if (options::outputFormat.getValue() == options::OutputFormat::GRAPH)
			{
				o << "\t\t\tf" << functionNum << "_s" << i << " [label=\"[" << idShift++ << "]\"];\n";
				o << "\t\t\tf" << functionNum << "_s" << i << " -> ";
				First first;
				for (const auto &[bit, value] : (*this)[i].splitBits())
				{
					if (!first)
						o << ", ";
					if (!value)
						o << 'f' << functionNum << "_ni" << static_cast<unsigned>(bit) << '_' << i;
					else
						o << 'i' << static_cast<unsigned>(bit);
				}
				o << ";\n";
			}
			else
			{
				o << "\t\t\tf" << functionNum << "_s" << i << " [label=\"[" << idShift++ << "] = ";
				(*this)[i].printHuman(o, false);
				o << "\"];\n";
			}
		}
		o << "\t\t}\n";
	}
	return idShift;
}

void Solution::printGraphSum(std::ostream &o, const std::size_t functionNum, const std::string_view functionName) const
{
	o << "\t\tsubgraph sum\n";
	o << "\t\t{\n";
	o << "\t\t\tnode [shape=rectangle, style=filled];\n";
	o << "\t\t\tedge [taillabel=\"||\"];\n";
	if (options::outputFormat.getValue() == options::OutputFormat::GRAPH || size() >= 2)
	{
		o << "\t\t\tf" << functionNum << " [label=\"" << functionName << "\"];\n";
		o << "\t\t\tf" << functionNum << " -> ";
		First first;
		for (std::size_t i = 0; i != size(); ++i)
		{
			if (!first)
				o << ", ";
			if ((*this)[i].empty())
				(*this)[i].printGraph(o);
			else
				o << 'f' << functionNum << "_s" << i;
		}
		o << ";\n";
	}
	else
	{
		o << "\t\t\tf" << functionNum << " [label=\"" << functionName << " = ";
		front().printHuman(o, false);
		o << "\"];\n";
	}
	o << "\t\t}\n";
}

Minterms Solution::getMinterms() const
{
	Minterms minterms;
	for (const Implicant &implicant : *this)
		implicant.addToMinterms(minterms);
	return minterms;
}

std::pair<bool, bool> Solution::checkForUsedConstants() const
{
	if (size() < 2 && front().empty())
		return front().checkForUsedConstants();
	else
		return {false, false};
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

std::size_t Solution::printGraph(std::ostream &o, const std::size_t functionNum, const std::string_view functionName, std::size_t idShift) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	if (isFullGraph)
		printGraphNegatedInputs(o, functionNum);
	if (isFullGraph || size() >= 2)
		idShift = printGraphProducts(o, functionNum, idShift);
	printGraphSum(o, functionNum, functionName);
	return idShift;
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
