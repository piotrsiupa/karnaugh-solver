#include "./Solution.hh"

#include <algorithm>

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
		for (Minterm i = 0; i != ::bits; ++i)
		{
			for (const std::size_t j : negatedInputs[i])
			{
				o << "\t\t\tf" << functionNum << "_ni" << i << '_' << j << " [label=\"!";
				::inputNames.printGraphName(o, i);
				o << "\"];\n";
				o << "\t\t\ti" << i << " -> f" << functionNum << "_ni" << i << '_' << j << ";\n";
			}
		}
		o << "\t\t}\n";
	}
}

inline void Solution::printGraphParentBit(std::ostream &o, const std::size_t functionNum, const Implicant::splitBit_t &splitBit, const std::size_t i)
{
	const auto &[bit, negated] = splitBit;
	if (negated)
		o << 'f' << functionNum << "_ni" << static_cast<unsigned>(bit) << '_' << i;
	else
		o << 'i' << static_cast<unsigned>(bit);
}

std::size_t Solution::printGraphProducts(std::ostream &o, const std::size_t functionNum, std::size_t idShift) const
{
	if (std::any_of(cbegin(), cend(), [](const Implicant &x){ return x.getBitCount() >= 2; }))
	{
		const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
		const bool isVerbose = options::verboseGraph.isRaised();
		o << "\t\tsubgraph products\n";
		o << "\t\t{\n";
		o << "\t\t\tnode [shape=ellipse];\n";
		for (std::size_t i = 0; i != size(); ++i)
		{
			if ((*this)[i].getBitCount() < 2)
				continue;
			o << "\t\t\tf" << functionNum << "_s" << i << " [label=\"";
			if (isFullGraph)
				o << "&&\\n";
			o << "[" << idShift++ << "]";
			if (!isFullGraph || isVerbose)
			{
				o << " = ";
				(*this)[i].printHuman(o, false);
			}
			o << "\"];\n";
			if (isFullGraph)
			{
				o << "\t\t\t";
				First first;
				for (const auto &splitBit : (*this)[i].splitBits())
				{
					if (!first)
						o << ", ";
					printGraphParentBit(o, functionNum, splitBit, i);
				}
				o << " -> f" << functionNum << "_s" << i << ";\n";
			}
		}
		o << "\t\t}\n";
	}
	return idShift;
}

void Solution::printGraphSum(std::ostream &o, const std::size_t functionNum, const std::string_view functionName) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	const bool isVerbose = options::verboseGraph.isRaised();
	o << "\t\tsubgraph sum\n";
	o << "\t\t{\n";
	o << "\t\t\tnode [shape=rectangle, style=filled];\n";
	o << "\t\t\tf" << functionNum << " [label=\"";
	const bool hasParents = isFullGraph || (size() >= 2 && std::any_of(cbegin(), cend(), [](const Implicant &x){ return x.getBitCount() >= 2; }));
	if (hasParents && size() >= 2)
		o << "||\\n";
	o << functionName;
	if (!isFullGraph || isVerbose)
	{
		First first;
		for (const Implicant &product : *this)
		{
			if (!isVerbose && size() >= 2 && product.getBitCount() >= 2)
				continue;
			if (first)
				o << " = ";
			else
				o << " || ";
			product.printHuman(o, this->size() != 1);
		}
	}
	o << "\"];\n";
	if (hasParents)
	{
		o << "\t\t\t";
		First first;
		for (std::size_t i = 0; i != size(); ++i)
		{
			if (!isFullGraph && (*this)[i].getBitCount() <= 1)
				continue;
			if (!first)
				o << ", ";
			switch ((*this)[i].getBitCount())
			{
			case 0:
				o << ((*this)[i].isError() ? "false" : "true");
				break;
			case 1:
				printGraphParentBit(o, functionNum, (*this)[i].splitBits().front(), i);
				break;
			default:
				o << 'f' << functionNum << "_s" << i;
				break;
			}
		}
		o << " -> f" << functionNum << ";\n";
	}
	o << "\t\t}\n";
}

Solution& Solution::sort()
{
	std::sort(begin(), end());
	return *this;
}

std::pair<bool, bool> Solution::checkForUsedConstants() const
{
	if (size() >= 2 || front().getBitCount() != 0)
		return {false, false};
	else if (front().isError())
		return {true, false};
	else
		return {false, true};
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
					o << u8" \u2228 ";
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
