#include "./OutputComposer.hh"

#include <filesystem>
#include <iomanip>

#include "info.hh"
#include "options.hh"
#include "utils.hh"


std::pair<bool, bool> OutputComposer::checkForUsedConstants(const Solution &solution) const
{
	if (solution.size() >= 2 || solution.front().getBitCount() != 0)
		return {false, false};
	else if (solution.front().isError())
		return {true, false};
	else
		return {false, true};
}

std::pair<bool, bool> OutputComposer::checkForUsedConstants() const
{
	if (options::skipOptimization.isRaised())
	{
		bool anyUsesFalse = false, anyUsesTrue = false;
		for (const Solution &solution : solutions)
		{
			const auto [usesFalse, usesTrue] = checkForUsedConstants(solution);
			anyUsesFalse |= usesFalse;
			anyUsesTrue |= usesTrue;
		}
		return {anyUsesFalse, anyUsesTrue};
	}
	else
	{
		return optimizedSolutions->checkForUsedConstants();
	}
}

bool OutputComposer::areInputsUsed() const
{
	for (const Solution &solution : solutions)
		for (const Implicant &implicant : solution)
			if (implicant.getBitCount() != 0)
				return true;
	return false;
}

bool OutputComposer::isOptimizedSumWorthPrinting(const OptimizedSolutions::id_t sumId, const bool simpleFinalSums) const
{
	if (simpleFinalSums)
		return optimizedSolutions->getSum(sumId).size() >= 2;
	if (std::count(optimizedSolutions->getFinalSums().cbegin(), optimizedSolutions->getFinalSums().cend(), sumId) >= 2 && optimizedSolutions->getSum(sumId).size() >= 2)
		return true;
	for (std::size_t i = 0; i != optimizedSolutions->getSumCount(); ++i)
		for (const OptimizedSolutions::id_t &id : optimizedSolutions->getSum(optimizedSolutions->makeSumId(i)))
			if (id == sumId)
				return true;
	return false;
}

std::string OutputComposer::getName()
{
	if (options::name.getValue())
		return *options::name.getValue();
	else if (::inputFilePath)
		return std::filesystem::path(*::inputFilePath).stem().string();
	else
		return "Karnaugh";
}

void OutputComposer::generateOptimizedHumanIds()
{
	const OptimizedSolutions::id_t idCount = optimizedSolutions->getProductCount() + optimizedSolutions->getSumCount();
	normalizedOptimizedIds.resize(idCount);
	OptimizedSolutions::id_t currentHumanId = 0;
	for (OptimizedSolutions::id_t id = 0; id != idCount; ++id)
		if (isOptimizedPartWorthPrinting(id, false))
			normalizedOptimizedIds[id] = currentHumanId++;
}

void OutputComposer::generateOptimizedGraphIds()
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	const OptimizedSolutions::id_t idCount = optimizedSolutions->getProductCount() + optimizedSolutions->getSumCount();
	normalizedOptimizedIds.resize(idCount);
	OptimizedSolutions::id_t currentGraphId = 0;
	for (OptimizedSolutions::id_t id = 0; id != idCount; ++id)
		if (isOptimizedPartWorthPrintingOnGraph(id, isFullGraph))
			normalizedOptimizedIds[id] = currentGraphId++;
}

std::pair<std::size_t, std::size_t> OutputComposer::generateOptimizedMathIds()
{
	const OptimizedSolutions::id_t idCount = optimizedSolutions->getProductCount() + optimizedSolutions->getSumCount();
	normalizedOptimizedIds.resize(idCount);
	OptimizedSolutions::id_t currentNormalizedProductId = 0, currentNormalizedSumId = 0;
	for (OptimizedSolutions::id_t id = 0; id != idCount; ++id)
	{
		if (optimizedSolutions->isProduct(id))
		{
			if (isOptimizedProductWorthPrintingForMath(id))
				normalizedOptimizedIds[id] = currentNormalizedProductId++;
		}
		else
		{
			if (isOptimizedSumWorthPrintingForMath(id))
				normalizedOptimizedIds[id] = currentNormalizedSumId++;
		}
	}
	return {currentNormalizedProductId, currentNormalizedSumId};
}

std::pair<std::size_t, std::size_t> OutputComposer::generateOptimizedNormalizedIds()
{
	const OptimizedSolutions::id_t idCount = optimizedSolutions->getProductCount() + optimizedSolutions->getSumCount();
	normalizedOptimizedIds.resize(idCount);
	OptimizedSolutions::id_t currentNormalizedProductId = 0, currentNormalizedSumId = 0;
	for (OptimizedSolutions::id_t id = 0; id != idCount; ++id)
	{
		if (optimizedSolutions->isProduct(id))
		{
			if (isOptimizedProductWorthPrinting(id))
				normalizedOptimizedIds[id] = currentNormalizedProductId++;
		}
		else
		{
			if (isOptimizedSumWorthPrinting(id, true))
				normalizedOptimizedIds[id] = currentNormalizedSumId++;
		}
	}
	return {currentNormalizedProductId, currentNormalizedSumId};
}

void OutputComposer::printOptimizedHumanId(std::ostream &o, const OptimizedSolutions::id_t id) const
{
	o << '[' << normalizedOptimizedIds[id] << ']';
}

void OutputComposer::printOptimizedGraphId(std::ostream &o, const OptimizedSolutions::id_t id) const
{
	o << 's' << normalizedOptimizedIds[id];
}

void OutputComposer::printOptimizedVerilogId(std::ostream &o, const OptimizedSolutions::id_t id) const
{
	if (optimizedSolutions->isProduct(id))
		o << "prods[" << normalizedOptimizedIds[id] << ']';
	else
		o << "sums[" << normalizedOptimizedIds[id] << ']';
}

void OutputComposer::printOptimizedVhdlId(std::ostream &o, const OptimizedSolutions::id_t id) const
{
	if (optimizedSolutions->isProduct(id))
		o << "prods(" << normalizedOptimizedIds[id] << ')';
	else
		o << "sums(" << normalizedOptimizedIds[id] << ')';
}

void OutputComposer::printOptimizedCppId(std::ostream &o, const OptimizedSolutions::id_t id) const
{
	if (optimizedSolutions->isProduct(id))
		o << "prods[" << normalizedOptimizedIds[id] << ']';
	else
		o << "sums[" << normalizedOptimizedIds[id] << ']';
}

void OutputComposer::printOptimizedMathId(std::ostream &o, const OptimizedSolutions::id_t id) const
{
	if (optimizedSolutions->isProduct(id))
		o << 'p' << (normalizedOptimizedIds[id] + 1);
	else
		o << 's' << (normalizedOptimizedIds[id] + 1);
	printOptimizedMathArgs(o, id);
}

void OutputComposer::printBanner(std::ostream &o)
{
	o << "Generated with " << getFullName() << " v" << getVersionNumber() << " by " << getAuthor();
	if (std::any_of(options::allOptions.cbegin(), options::allOptions.cend(), [](const options::Option *const option){ return option->isSet(); }))
	{
		o << " (with options:";
		for (const options::Option *const option : options::allOptions)
			if (option->isSet())
				o << ' ' << option->compose();
		o << ')';
	}
	o << "\n\n";
}

OutputComposer::grayCode_t OutputComposer::makeGrayCode(const bits_t bitCount)
{
	grayCode_t grayCode;
	grayCode.reserve(static_cast<unsigned>(1u << bitCount));
	grayCode.push_back(0);
	if (bitCount != 0)
	{
		grayCode.push_back(1);
		for (bits_t i = 1; i != bitCount; ++i)
			for (Minterm j = 0; j != unsigned(1) << i; ++j)
				grayCode.push_back(grayCode[j ^ ((1 << i) - 1)] | (1 << i));
	}
	return grayCode;
}

void OutputComposer::printBits(std::ostream &o, const Minterm minterm, const bits_t bitCount)
{
	for (bits_t i = bitCount; i != 0; --i)
		o << ((minterm & (1 << (i - 1))) != 0 ? '1' : '0');
}

void OutputComposer::prettyPrintTable(std::ostream &o, const Minterms &target, const Minterms &allowed)
{
	const bits_t vBits = (::bits + 1) / 2;
	const bits_t hBits = ::bits / 2;
	const grayCode_t vGrayCode = makeGrayCode(vBits);
	const grayCode_t hGrayCode = makeGrayCode(hBits);
	for (int i = 0; i != vBits; ++i)
		o << ' ';
	o << ' ';
	for (const Minterm y : hGrayCode)
	{
		printBits(o, y, hBits);
		o << ' ';
	}
	o << '\n';
	for (const Minterm x : vGrayCode)
	{
		printBits(o, x, std::max(bits_t(1), vBits));
		o << ' ';
		First first;
		for (int i = 0; i != (hBits - 1) / 2; ++i)
			o << ' ';
		for (const Minterm y : hGrayCode)
		{
			if (!first)
				for (int i = 0; i != hBits; ++i)
					o << ' ';
			const Minterm minterm = (x << hBits) | y;
			switch (options::outputOperators.getValue())
			{
			case options::OutputOperators::FORMAL:
			case options::OutputOperators::ASCII:
				o << (target.find(minterm) != target.cend() ? '1' : (allowed.find(minterm) != allowed.cend() ? 'X' : '0'));
				break;
			case options::OutputOperators::PROGRAMMING:
				o << (target.find(minterm) != target.cend() ? 'T' : (allowed.find(minterm) != allowed.cend() ? '-' : 'F'));
				break;
			case options::OutputOperators::NAMES:
				o << (target.find(minterm) != target.cend() ? 'T' : (allowed.find(minterm) != allowed.cend() ? '?' : 'F'));
				break;
			}
		}
		o << '\n';
	}
	o << std::endl;
}

void OutputComposer::prettyPrintTable(std::ostream &o, const std::size_t i) const
{
	const Karnaugh &karnaugh = karnaughs[i];
	return prettyPrintTable(o, karnaugh.getTargetMinterms(), karnaugh.getAllowedMinterms());
}

void OutputComposer::prettyPrintSolution(std::ostream &o, const Solution &solution)
{
	Minterms minterms;
	for (const auto &implicant : solution)
	{
		const auto newMinterms = implicant.findMinterms();
		minterms.insert(newMinterms.cbegin(), newMinterms.end());
	}
	prettyPrintTable(o, minterms);
}

void OutputComposer::printHumanBool(std::ostream &o, const bool value) const
{
	switch (options::outputOperators.getValue())
	{
	case options::OutputOperators::FORMAL:
		o << (value ? u8"\u22A4" : u8"\u22A5");
		break;
	case options::OutputOperators::ASCII:
		o << (value ? 'T' : 'F');
		break;
	case options::OutputOperators::PROGRAMMING:
		o << (value ? "true" : "false");
		break;
	case options::OutputOperators::NAMES:
		o << (value ? "TRUE" : "FALSE");
		break;
	default:
		break;
	}
}

void OutputComposer::printHumanNot(std::ostream &o) const
{
	switch (options::outputOperators.getValue())
	{
	case options::OutputOperators::FORMAL:
		o << u8"\u00AC";
		break;
	case options::OutputOperators::ASCII:
		o << '~';
		break;
	case options::OutputOperators::PROGRAMMING:
		o << '!';
		break;
	case options::OutputOperators::NAMES:
		o << "NOT ";
		break;
	default:
		break;
	}
}

void OutputComposer::printGraphNot(std::ostream &o) const
{
	switch (options::outputOperators.getValue())
	{
	case options::OutputOperators::FORMAL:
		o << u8"\u00AC";
		break;
	case options::OutputOperators::ASCII:
		o << '~';
		break;
	case options::OutputOperators::PROGRAMMING:
		o << '!';
		break;
	case options::OutputOperators::NAMES:
		o << "NOT ";
		break;
	default:
		break;
	}
}

void OutputComposer::printHumanAnd(std::ostream &o) const
{
	switch (options::outputOperators.getValue())
	{
	case options::OutputOperators::FORMAL:
		o << u8" \u2227 ";
		break;
	case options::OutputOperators::ASCII:
		o << " /\\ ";
		break;
	case options::OutputOperators::PROGRAMMING:
		o << " && ";
		break;
	case options::OutputOperators::NAMES:
		o << " AND ";
		break;
	default:
		break;
	}
}

void OutputComposer::printGraphAnd(std::ostream &o, const bool spaces) const
{
	if (spaces)
		o << ' ';
	switch (options::outputOperators.getValue())
	{
	case options::OutputOperators::FORMAL:
		o << u8"\u2227";
		break;
	case options::OutputOperators::ASCII:
		o << "/\\\\";
		break;
	case options::OutputOperators::PROGRAMMING:
		o << "&&";
		break;
	case options::OutputOperators::NAMES:
		o << "AND";
		break;
	default:
		break;
	}
	if (spaces)
		o << ' ';
}

void OutputComposer::printHumanOr(std::ostream &o, const bool spaces) const
{
	if (spaces)
		o << ' ';
	switch (options::outputOperators.getValue())
	{
	case options::OutputOperators::FORMAL:
		o << u8"\u2228";
		break;
	case options::OutputOperators::ASCII:
		o << "\\/";
		break;
	case options::OutputOperators::PROGRAMMING:
		o << "||";
		break;
	case options::OutputOperators::NAMES:
		o << "OR";
		break;
	default:
		break;
	}
	if (spaces)
		o << ' ';
}

void OutputComposer::printGraphOr(std::ostream &o, const bool spaces) const
{
	if (spaces)
		o << ' ';
	switch (options::outputOperators.getValue())
	{
	case options::OutputOperators::FORMAL:
		o << u8"\u2228";
		break;
	case options::OutputOperators::ASCII:
		o << "\\\\/";
		break;
	case options::OutputOperators::PROGRAMMING:
		o << "||";
		break;
	case options::OutputOperators::NAMES:
		o << "OR";
		break;
	default:
		break;
	}
	if (spaces)
		o << ' ';
}

void OutputComposer::printHumanImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const
{
	if (implicant.getBitCount() == 0)
	{
		printHumanBool(o, !implicant.isError());
		return;
	}
	const bool needsParentheses = parentheses && implicant.getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : implicant.splitBits())
	{
		if (!first)
			printHumanAnd(o);
		if (negated)
			printHumanNot(o);
		::inputNames.printHumanName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void OutputComposer::printGraphImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const
{
	if (implicant.getBitCount() == 0)
	{
		printHumanBool(o, !implicant.isError());
		return;
	}
	const bool needsParentheses = parentheses && implicant.getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : implicant.splitBits())
	{
		if (!first)
			printGraphAnd(o, true);
		if (negated)
			printGraphNot(o);
		::inputNames.printHumanName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void OutputComposer::printGraphImplicant(std::ostream &o, const Implicant &implicant) const
{
	if (implicant.getBitCount() == 0)
	{
		if (implicant.isError())
			o << "false";
		else
			o << "true";
		return;
	}
	First first;
	for (const auto &[bitIndex, negated] : implicant.splitBits())
	{
		if (!first)
			o << ", ";
		if (negated)
			o << 'n';
		o << 'i' << static_cast<unsigned>(bitIndex);
	}
}

void OutputComposer::printVerilogImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const
{
	if (implicant.getBitCount() == 0)
	{
		if (implicant.isError())
			o << '0';
		else
			o << '1';
		return;
	}
	const bool needsParentheses = parentheses && implicant.getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : implicant.splitBits())
	{
		if (!first)
			o << " & ";
		if (negated)
			o << '!';
		::inputNames.printVerilogName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void OutputComposer::printVhdlImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const
{
	if (implicant.getBitCount() == 0)
	{
		if (implicant.isError())
			o << "'0'";
		else
			o << "'1'";
		return;
	}
	const bool needsParentheses = parentheses && implicant.getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : implicant.splitBits())
	{
		if (!first)
			o << " and ";
		if (negated)
			o << "not ";
		::inputNames.printVhdlName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void OutputComposer::printCppImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const
{
	if (implicant.getBitCount() == 0)
	{
		if (implicant.isError())
			o << "false";
		else
			o << "true";
		return;
	}
	const bool needsParentheses = parentheses && implicant.getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : implicant.splitBits())
	{
		if (!first)
			o << " && ";
		if (negated)
			o << "!";
		::inputNames.printCppName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void OutputComposer::printMathImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const
{
	if (implicant.getBitCount() == 0)
	{
		printHumanBool(o, !implicant.isError());
		return;
	}
	const bool needsParentheses = parentheses && implicant.getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : implicant.splitBits())
	{
		if (!first)
			printHumanAnd(o);
		if (negated)
			printHumanNot(o);
		::inputNames.printMathName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void OutputComposer::printGateCost(std::ostream &o, const GateCost &gateCost, const bool full) const
{
	const auto notCount = gateCost.getNotCount();
	const auto andCount = gateCost.getAndCount();
	const auto orCount = gateCost.getOrCount();
	o << "Gate cost: NOTs = " << notCount << ", ANDs = " << andCount << ", ORs = " << orCount;
	if (full)
		o << ", overall cost = " << GateCost::getCost(notCount, andCount, orCount)
				<< ", cost without input NOTs = " << GateCost::getCost(andCount, orCount);
	o << '\n';
}

void OutputComposer::printGraphConstants(std::ostream &o) const
{
	const auto [usesFalse, usesTrue] = checkForUsedConstants();
	if (!usesFalse && !usesTrue)
		return;
	o << "\tsubgraph constants\n";
	o << "\t{\n";
	o << "\t\tnode [shape=octagon, style=dashed];\n";
	if (usesTrue)
	{
		o << "\t\ttrue [label=\"";
		switch (options::outputOperators.getValue())
		{
		case options::OutputOperators::FORMAL:
			o << u8"\u22A4";
			break;
		case options::OutputOperators::ASCII:
			o << 'T';
			break;
		case options::OutputOperators::PROGRAMMING:
			o << "true";
			break;
		case options::OutputOperators::NAMES:
			o << "TRUE";
			break;
		default:
			break;
		}
		o << "\"];\n";
	}
	if (usesFalse)
	{
		o << "\t\tfalse [label=\"";
		switch (options::outputOperators.getValue())
		{
		case options::OutputOperators::FORMAL:
			o << u8"\u22A5";
			break;
		case options::OutputOperators::ASCII:
			o << 'F';
			break;
		case options::OutputOperators::PROGRAMMING:
			o << "false";
			break;
		case options::OutputOperators::NAMES:
			o << "FALSE";
			break;
		default:
			break;
		}
		o << "\"];\n";
	}
	o << "\t}\n";
}

void OutputComposer::printGraphRoots(std::ostream &o) const
{
	printGraphInputs(o);
	printGraphConstants(o);
}

void OutputComposer::printGraphInputs(std::ostream &o) const
{
	if (::bits == 0)
		return;
	o << "\tsubgraph inputs\n";
	o << "\t{\n";
	o << "\t\trank=same;\n";
	o << "\t\tnode [shape=invhouse];\n";
	for (Minterm i = 0; i != ::bits; ++i)
	{
		o << "\t\ti" << i << " [label=\"";
		::inputNames.printGraphName(o, i);
		o << "\"];\n";
	}
	o << "\t}\n";
}

void OutputComposer::printGraphNegatedInputs(std::ostream &o, const Solution &solution, const std::size_t functionNum) const
{
	std::vector<std::vector<std::size_t>> negatedInputs(::bits);
	for (std::size_t i = 0; i != solution.size(); ++i)
	{
		const auto falseBits = solution[i].getFalseBits();
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
				o << "\t\t\tf" << functionNum << "_ni" << i << '_' << j << " [label=\"";
				printGraphNot(o);
				::inputNames.printGraphName(o, i);
				o << "\"];\n";
				o << "\t\t\ti" << i << " -> f" << functionNum << "_ni" << i << '_' << j << ";\n";
			}
		}
		o << "\t\t}\n";
	}
}

inline void OutputComposer::printGraphParentBit(std::ostream &o, const std::size_t functionNum, const Implicant::splitBit_t &splitBit, const std::size_t i)
{
	const auto &[bit, negated] = splitBit;
	if (negated)
		o << 'f' << functionNum << "_ni" << static_cast<unsigned>(bit) << '_' << i;
	else
		o << 'i' << static_cast<unsigned>(bit);
}

std::size_t OutputComposer::printGraphProducts(std::ostream &o, const Solution &solution, const std::size_t functionNum, std::size_t idShift) const
{
	if (std::any_of(solution.cbegin(), solution.cend(), [](const Implicant &x){ return x.getBitCount() >= 2; }))
	{
		const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
		const bool isVerbose = options::verboseGraph.isRaised();
		o << "\t\tsubgraph products\n";
		o << "\t\t{\n";
		o << "\t\t\tnode [shape=ellipse];\n";
		for (std::size_t i = 0; i != solution.size(); ++i)
		{
			if (solution[i].getBitCount() < 2)
				continue;
			o << "\t\t\tf" << functionNum << "_s" << i << " [label=\"";
			if (isFullGraph)
			{
				printGraphAnd(o, false);
				o << "\\n";
			}
			o << "[" << idShift++ << "]";
			if (!isFullGraph || isVerbose)
			{
				o << " = ";
				printGraphImplicant(o, solution[i], false);
			}
			o << "\"];\n";
			if (isFullGraph)
			{
				o << "\t\t\t";
				First first;
				for (const auto &splitBit : solution[i].splitBits())
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

void OutputComposer::printGraphSum(std::ostream &o, const Solution &solution, const std::size_t functionNum) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	const bool isVerbose = options::verboseGraph.isRaised();
	o << "\t\tsubgraph sum\n";
	o << "\t\t{\n";
	o << "\t\t\tnode [shape=rectangle, style=filled];\n";
	o << "\t\t\tf" << functionNum << " [label=\"";
	const bool hasParents = isFullGraph || (solution.size() >= 2 && std::any_of(solution.cbegin(), solution.cend(), [](const Implicant &x){ return x.getBitCount() >= 2; }));
	if (hasParents && solution.size() >= 2)
	{
		printGraphOr(o, false);
		o << "\\n";
	}
	functionNames.printGraphName(o, functionNum);
	if (!isFullGraph || isVerbose)
	{
		First first;
		for (const Implicant &product : solution)
		{
			if (!isVerbose && solution.size() >= 2 && product.getBitCount() >= 2)
				continue;
			if (first)
				o << " = ";
			else
				printGraphOr(o, true);
			printGraphImplicant(o, product, solution.size() != 1);
		}
	}
	o << "\"];\n";
	if (hasParents)
	{
		o << "\t\t\t";
		First first;
		for (std::size_t i = 0; i != solution.size(); ++i)
		{
			if (!isFullGraph && solution[i].getBitCount() <= 1)
				continue;
			if (!first)
				o << ", ";
			switch (solution[i].getBitCount())
			{
			case 0:
				o << (solution[i].isError() ? "false" : "true");
				break;
			case 1:
				printGraphParentBit(o, functionNum, solution[i].splitBits().front(), i);
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

void OutputComposer::printHumanSolution_(std::ostream &o, const Solution &solution) const
{
	if (solution.size() == 1)
	{
		printHumanImplicant(o, solution.front(), false);
	}
	else
	{
		for (const Implicant &implicant : solution)
		{
			if (&implicant != &solution.front())
				printHumanOr(o, true);
			printHumanImplicant(o, implicant, true);
		}
	}
	o << '\n';
	if (options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT)
	{
		o << '\n';
		printGateCost(o, solution, false);
	}
}

std::size_t OutputComposer::printGraphSolution_(std::ostream &o, const Solution &solution, const std::size_t functionNum, std::size_t idShift) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	if (isFullGraph)
		printGraphNegatedInputs(o, solution, functionNum);
	if (isFullGraph || solution.size() >= 2)
		idShift = printGraphProducts(o, solution, functionNum, idShift);
	printGraphSum(o, solution, functionNum);
	return idShift;
}

void OutputComposer::printVerilogSolution_(std::ostream &o, const Solution &solution) const
{
	if (solution.size() == 1)
	{
		printVerilogImplicant(o, solution.front(), false);
	}
	else
	{
		for (const Implicant &implicant : solution)
		{
			if (&implicant != &solution.front())
				o << " | ";
			printVerilogImplicant(o, implicant, true);
		}
	}
}

void OutputComposer::printVhdlSolution_(std::ostream &o, const Solution &solution) const
{
	if (solution.size() == 1)
	{
		printVhdlImplicant(o, solution.front(), false);
	}
	else
	{
		for (const Implicant &implicant : solution)
		{
			if (&implicant != &solution.front())
				o << " or ";
			printVhdlImplicant(o, implicant, true);
		}
	}
}

void OutputComposer::printCppSolution_(std::ostream &o, const Solution &solution) const
{
	if (solution.size() == 1)
	{
		printCppImplicant(o, solution.front(), false);
	}
	else
	{
		for (const Implicant &implicant : solution)
		{
			if (&implicant != &solution.front())
				o << " || ";
			printCppImplicant(o, implicant, true);
		}
	}
}

void OutputComposer::printMathSolution_(std::ostream &o, const Solution &solution) const
{
	if (solution.size() == 1)
	{
		printMathImplicant(o, solution.front(), false);
	}
	else
	{
		for (const Implicant &implicant : solution)
		{
			if (&implicant != &solution.front())
				printHumanOr(o, true);
			printMathImplicant(o, implicant, true);
		}
	}
}

void OutputComposer::printHumanSolution(std::ostream &o, const std::size_t i) const
{
	const Solution &solution = solutions[i];
	const Karnaugh &karnaugh = karnaughs[i];
	if (options::outputFormat.getValue() == options::OutputFormat::HUMAN_LONG)
	{
		if (::bits <= 8)
		{
			o << "goal:\n";
			prettyPrintTable(o, i);
			
			if (karnaugh.getTargetMinterms().size() != karnaugh.getAllowedMinterms().size())
			{
				o << "best fit:\n";
				prettyPrintSolution(o, solution);
			}
		}
		else
		{
			o << "The Karnaugh map is too big to be displayed.\n\n";
		}
		o << "solution:\n";
	}
	printHumanSolution_(o, Solution(solution).sort());
}

std::size_t OutputComposer::printGraphSolution(std::ostream &o, const std::size_t i, std::size_t idShift) const
{
	o << "\tsubgraph function_" << i << '\n';
	o << "\t{\n";
	idShift = printGraphSolution_(o, Solution(solutions[i]).sort(), i, idShift);
	o << "\t}\n";
	return idShift;
}

void OutputComposer::printVerilogSolution(std::ostream &o, const Solution &solution) const
{
	printVerilogSolution_(o, Solution(solution).sort());
}

void OutputComposer::printVhdlSolution(std::ostream &o, const Solution &solution) const
{
	printVhdlSolution_(o, Solution(solution).sort());
}

void OutputComposer::printCppSolution(std::ostream &o, const Solution &solution) const
{
	printCppSolution_(o, Solution(solution).sort());
}

void OutputComposer::printMathSolution(std::ostream &o, const Solution &solution) const
{
	printMathSolution_(o, Solution(solution).sort());
}

void OutputComposer::printHumanSolutions(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != functionNames.size(); ++i)
	{
		if (!first && options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT)
			o << '\n' << '\n';
		o << "--- ";
		functionNames.printHumanName(o, i);
		o << " ---\n";
		if (options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT)
			o << '\n';
		printHumanSolution(o, i);
	}
}

void OutputComposer::printGraphSolutions(std::ostream &o) const
{
	std::size_t idShift = 0;
	for (std::size_t i = 0; i != karnaughs.size(); ++i)
		idShift = printGraphSolution(o, i, idShift);
}

void OutputComposer::printVerilogSolutions(std::ostream &o) const
{
	if (!karnaughs.empty())
	{
		for (std::size_t i = 0; i != karnaughs.size(); ++i)
		{
			o << "\tassign ";
			functionNames.printVerilogName(o, i);
			o << " = ";
			printVerilogSolution(o, solutions[i]);
			o << ";\n";
		}
		o << "\t\n";
	}
}

void OutputComposer::printVhdlSolutions(std::ostream &o, const Names &functionNames) const
{
	o << "begin\n";
	o << "\t\n";
	if (!karnaughs.empty())
	{
		for (std::size_t i = 0; i != karnaughs.size(); ++i)
		{
			o << '\t';
			functionNames.printVhdlName(o, i);
			o << " <= ";
			printVhdlSolution(o, solutions[i]);
			o << ";\n";
		}
		o << "\t\n";
	}
}

void OutputComposer::printCppSolutions(std::ostream &o, const Names &functionNames) const
{
	if (karnaughs.empty())
	{
		o << "\treturn {};\n";
	}
	else
	{
		o << "\toutput_t o = {};\n";
		for (std::size_t i = 0; i != karnaughs.size(); ++i)
		{
			o << '\t';
			functionNames.printCppName(o, i);
			o << " = ";
			printCppSolution(o, solutions[i]);
			o << ";\n";
		}
		o << "\treturn o;\n";
	}
}

void OutputComposer::printMathSolutions(std::ostream &o, const Names &functionNames) const
{
	for (std::size_t i = 0; i != karnaughs.size(); ++i)
	{
		functionNames.printMathName(o, i);
		o << '(';
		::inputNames.printMathNames(o);
		o << ") = ";
		printMathSolution(o, solutions[i]);
		o << "\n";
	}
}

void OutputComposer::printOptimizedHumanSums(std::ostream &o) const
{
	o << "Sums:\n";
	for (std::size_t i = 0; i != optimizedSolutions->getSumCount(); ++i)
	{
		if (!isOptimizedSumWorthPrinting(optimizedSolutions->makeSumId(i), false))
			continue;
		printOptimizedHumanSum(o, optimizedSolutions->makeSumId(i));
	}
}

void OutputComposer::printOptimizedGraphSums(std::ostream &o, const Names &functionNames) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	First first;
	for (std::size_t i = 0; i != optimizedSolutions->getSumCount(); ++i)
	{
		const OptimizedSolutions::id_t sumId = optimizedSolutions->makeSumId(i);
		if (!isOptimizedSumWorthPrintingOnGraph(sumId, isFullGraph))
			continue;
		if (first)
		{
			o << "\tsubgraph sums\n";
			o << "\t{\n";
			o << "\t\tnode [shape=rectangle];\n";
		}
		printOptimizedGraphSum(o, functionNames, sumId);
	}
	if (!first)
		o << "\t}\n";
}

void OutputComposer::printOptimizedVerilogSums(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != optimizedSolutions->getSumCount(); ++i)
	{
		if (!isOptimizedSumWorthPrinting(optimizedSolutions->makeSumId(i), true))
			continue;
		if (first)
			o << "\t// Sums\n";
		printOptimizedVerilogSum(o, optimizedSolutions->makeSumId(i));
	}
	if (!first)
		o << "\t\n";
}

void OutputComposer::printOptimizedVhdlSums(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != optimizedSolutions->getSumCount(); ++i)
	{
		if (!isOptimizedSumWorthPrinting(optimizedSolutions->makeSumId(i), true))
			continue;
		if (first)
			o << "\t\n\t-- Sums\n";
		printOptimizedVhdlSum(o, optimizedSolutions->makeSumId(i));
	}
}

void OutputComposer::printOptimizedCppSums(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != optimizedSolutions->getSumCount(); ++i)
	{
		if (!isOptimizedSumWorthPrinting(optimizedSolutions->makeSumId(i), true))
			continue;
		if (first)
			o << "\t// Sums\n";
		printOptimizedCppSum(o, optimizedSolutions->makeSumId(i));
	}
	if (!first)
		o << "\t\n";
}

void OutputComposer::printOptimizedMathSums(std::ostream &o) const
{
	for (std::size_t i = 0; i != optimizedSolutions->getSumCount(); ++i)
	{
		const OptimizedSolutions::id_t sumId = optimizedSolutions->makeSumId(i);
		if (!isOptimizedSumWorthPrintingForMath(sumId))
			continue;
		printOptimizedMathSum(o, sumId);
	}
}

void OutputComposer::printOptimizedVerilogImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const
{
	if (immediateProductCount != 0 || immediateSumCount != 0)
	{
		o << "\t// Internal signals\n";
		if (immediateProductCount != 0)
			o << "\twire [" << (immediateProductCount - 1) << ":0] prods;\n";
		if (immediateSumCount != 0)
			o << "\twire [" << (immediateSumCount - 1) << ":0] sums;\n";
		o << "\t\n";
	}
}

void OutputComposer::printOptimizedVhdlImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const
{
	if (immediateProductCount != 0 || immediateSumCount != 0)
	{
		o << "\t-- Internal signals\n";
		if (immediateProductCount != 0)
			o << "\tsignal prods : std_logic_vector(" << (immediateProductCount - 1) << " downto 0);\n";
		if (immediateSumCount != 0)
			o << "\tsignal sums : std_logic_vector(" << (immediateSumCount - 1) << " downto 0);\n";
		o << "\t\n";
	}
}

void OutputComposer::printOptimizedCppImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const
{
	if (immediateProductCount != 0 || immediateSumCount != 0)
	{
		o << "\t// Intermediary values\n";
		if (immediateProductCount != 0)
			o << "\tbool prods[" << immediateProductCount << "] = {};\n";
		if (immediateSumCount != 0)
			o << "\tbool sums[" << immediateSumCount << "] = {};\n";
		o << "\t\n";
	}
}

void OutputComposer::printOptimizedMathArgs(std::ostream &o, const OptimizedSolutions::id_t id) const
{
	Implicant implicant = Implicant::all();
	OptimizedSolutions::ids_t ids{id};
	while (!ids.empty())
	{
		OptimizedSolutions::id_t subId = ids.back();
		ids.pop_back();
		if (optimizedSolutions->isProduct(subId))
		{
			const auto &[subImplicant, subIds] = optimizedSolutions->getProduct(subId);
			implicant |= subImplicant;
			ids.insert(ids.end(), subIds.cbegin(), subIds.cend());
		}
		else
		{
			const auto &subIds = optimizedSolutions->getSum(subId);
			ids.insert(ids.end(), subIds.cbegin(), subIds.cend());
		}
	}
	o << '(';
	First first;
	for (const auto &[bit, value] : implicant.splitBits())
	{
		if (!first)
			o << ", ";
		::inputNames.printMathName(o, bit);
	}
	o << ')';
}

void OutputComposer::printOptimizedGraphProductImplicant(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	const bool isVerboseGraph = options::verboseGraph.isRaised();
	Implicant primeImplicant = Implicant::error();
	if (isVerboseGraph)
	{
		primeImplicant = optimizedSolutions->flattenProduct(productId);
	}
	else
	{
		const OptimizedSolutions::product_t &product = optimizedSolutions->getProduct(productId);
		if (product.implicant.getBitCount() == 0 && !product.subProducts.empty())
			return;
		primeImplicant = product.implicant;
	}
	o << " = ";
	printGraphImplicant(o, primeImplicant, false);
}

void OutputComposer::printOptimizedGraphProductParents(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	const auto &[primeImplicant, ids] = optimizedSolutions->getProduct(productId);
	bool needsComma = isFullGraph && primeImplicant != Implicant::all();
	if (needsComma || ids.empty())
		printGraphImplicant(o, primeImplicant);
	for (const auto &id : ids)
	{
		if (needsComma)
			o << ", ";
		else
			needsComma = true;
		printOptimizedGraphId(o, id);
	}
}

void OutputComposer::printOptimizedHumanProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	o << '\t';
	printOptimizedHumanId(o, productId);
	o << " = ";
	printOptimizedHumanProductBody(o, productId);
	o << '\n';
}

void OutputComposer::printOptimizedGraphProduct(std::ostream &o, const Names &functionNames, const OptimizedSolutions::id_t productId) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	const bool isVerboseGraph = options::verboseGraph.isRaised();
	std::size_t functionNum = isFullGraph ? SIZE_MAX : optimizedSolutions->findProductEndNode(productId);
	o << "\t\t";
	printOptimizedGraphId(o, productId);
	o << " [label=\"";
	const bool hasParents = isFullGraph || !optimizedSolutions->getProduct(productId).subProducts.empty();
	if (hasParents)
	{
		printGraphAnd(o, false);
		o << "\\n";
	}
	if (functionNum == SIZE_MAX)
	{
		printOptimizedHumanId(o, productId);
	}
	else
	{
		while (true)
		{
			functionNames.printGraphName(o, functionNum);
			const std::size_t additionalFunNum = optimizedSolutions->findProductEndNode(productId, functionNum + 1);
			if (additionalFunNum == SIZE_MAX)
				break;
			o << ", ";
			functionNum = additionalFunNum;
		}
	}
	if (!isFullGraph || isVerboseGraph)
		printOptimizedGraphProductImplicant(o, productId);
	o << '"';
	if (functionNum != SIZE_MAX)
		o << ", style=filled";
	o << "];";
	o << '\n';
	if (hasParents)
	{
		o << "\t\t";
		printOptimizedGraphProductParents(o, productId);
		o << " -> ";
		printOptimizedGraphId(o, productId);
		o << ";\n";
	}
}

void OutputComposer::printOptimizedVerilogProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	o << "\tassign ";
	printOptimizedVerilogId(o, productId);
	o << " = ";
	printOptimizedVerilogProductBody(o, productId);
	o << ";\n";
}

void OutputComposer::printOptimizedVhdlProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	o << '\t';
	printOptimizedVhdlId(o, productId);
	o << " <= ";
	printOptimizedVhdlProductBody(o, productId);
	o << ";\n";
}

void OutputComposer::printOptimizedCppProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	o << '\t';
	printOptimizedCppId(o, productId);
	o << " = ";
	printOptimizedCppProductBody(o, productId);
	o << ";\n";
}

void OutputComposer::printOptimizedMathProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	printOptimizedMathId(o, productId);
	o << " = ";
	printOptimizedMathProductBody(o, productId, false);
	o << '\n';
}

void OutputComposer::printOptimizedHumanProducts(std::ostream &o) const
{
	o << "Products:\n";
	for (std::size_t i = 0; i != optimizedSolutions->getProductCount(); ++i)
	{
		if (!isOptimizedProductWorthPrinting(optimizedSolutions->makeProductId(i)))
			continue;
		printOptimizedHumanProduct(o, optimizedSolutions->makeProductId(i));
	}
}

void OutputComposer::printOptimizedGraphProducts(std::ostream &o, const Names &functionNames) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	First first;
	for (std::size_t i = 0; i != optimizedSolutions->getProductCount(); ++i)
	{
		if (!isOptimizedProductWorthPrintingOnGraph(optimizedSolutions->makeProductId(i), isFullGraph))
			continue;
		if (first)
		{
			o << "\tsubgraph products\n";
			o << "\t{\n";
			o << "\t\tnode [shape=ellipse];\n";
		}
		printOptimizedGraphProduct(o, functionNames, optimizedSolutions->makeProductId(i));
	}
	if (!first)
		o << "\t}\n";
}

void OutputComposer::printOptimizedVerilogProducts(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != optimizedSolutions->getProductCount(); ++i)
	{
		if (!isOptimizedProductWorthPrinting(optimizedSolutions->makeProductId(i)))
			continue;
		if (first)
			o << "\t// Products\n";
		printOptimizedVerilogProduct(o, optimizedSolutions->makeProductId(i));
	}
	if (!first)
		o << "\t\n";
}

void OutputComposer::printOptimizedVhdlProducts(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != optimizedSolutions->getProductCount(); ++i)
	{
		if (!isOptimizedProductWorthPrinting(optimizedSolutions->makeProductId(i)))
			continue;
		if (first)
			o << "\t\n\t-- Products\n";
		printOptimizedVhdlProduct(o, optimizedSolutions->makeProductId(i));
	}
}

void OutputComposer::printOptimizedCppProducts(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != optimizedSolutions->getProductCount(); ++i)
	{
		if (!isOptimizedProductWorthPrinting(optimizedSolutions->makeProductId(i)))
			continue;
		if (first)
			o << "\t// Products\n";
		printOptimizedCppProduct(o, optimizedSolutions->makeProductId(i));
	}
	if (!first)
		o << "\t\n";
}

void OutputComposer::printOptimizedMathProducts(std::ostream &o) const
{
	for (std::size_t i = 0; i != optimizedSolutions->getProductCount(); ++i)
	{
		const OptimizedSolutions::id_t productId = optimizedSolutions->makeProductId(i);
		if (!isOptimizedProductWorthPrintingForMath(productId))
			continue;
		printOptimizedMathProduct(o, productId);
	}
}

void OutputComposer::printOptimizedHumanSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	First first;
	for (const auto &partId : optimizedSolutions->getSum(sumId))
	{
		if (!first)
			printHumanOr(o, true);
		if (!optimizedSolutions->isProduct(partId) || isOptimizedProductWorthPrinting(partId))
			printOptimizedHumanId(o, partId);
		else
			printHumanImplicant(o, optimizedSolutions->getProduct(partId).implicant, false);
	}
}

void OutputComposer::printOptimizedGraphSumProducts(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	const bool isVerboseGraph = options::verboseGraph.isRaised();
	OptimizedSolutions::sum_t sum;
	if (isVerboseGraph)
	{
		sum = optimizedSolutions->flattenSum(sumId);
	}
	else
	{
		sum = optimizedSolutions->getSum(sumId);
		sum.erase(std::remove_if(sum.begin(), sum.end(), [this](const OptimizedSolutions::id_t id){ return !optimizedSolutions->isProduct(id) || isOptimizedProductWorthPrinting(id); }), sum.end());
	}
	First first;
	for (const auto &productId : sum)
	{
		if (first)
			o << " = ";
		else
			printGraphOr(o, true);
		if (isVerboseGraph)
			printGraphImplicant(o, optimizedSolutions->flattenProduct(productId), sum.size() != 1);
		else
			printHumanImplicant(o, optimizedSolutions->getProduct(productId).implicant, false);
	}
}

void OutputComposer::printOptimizedGraphSumParents(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	First first;
	for (const auto &partId : optimizedSolutions->getSum(sumId))
	{
		if (!optimizedSolutions->isProduct(partId) || isOptimizedProductWorthPrinting(partId))
		{
			if (!first)
				o << ", ";
			printOptimizedGraphId(o, partId);
		}
		else if (isFullGraph)
		{
			if (!first)
				o << ", ";
			printGraphImplicant(o, optimizedSolutions->getProduct(partId).implicant);
		}
	}
}

void OutputComposer::printOptimizedVerilogSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	First first;
	for (const auto &partId : optimizedSolutions->getSum(sumId))
	{
		if (!first)
			o << " | ";
		if (!optimizedSolutions->isProduct(partId) || isOptimizedProductWorthPrinting(partId))
			printOptimizedVerilogId(o, partId);
		else
			printVerilogImplicant(o, optimizedSolutions->getProduct(partId).implicant, false);
	}
}

void OutputComposer::printOptimizedVhdlSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	First first;
	for (const auto &partId : optimizedSolutions->getSum(sumId))
	{
		if (!first)
			o << " or ";
		if (!optimizedSolutions->isProduct(partId) || isOptimizedProductWorthPrinting(partId))
			printOptimizedVhdlId(o, partId);
		else
			printVhdlImplicant(o, optimizedSolutions->getProduct(partId).implicant, false);
	}
}

void OutputComposer::printOptimizedCppSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	First first;
	for (const auto &partId : optimizedSolutions->getSum(sumId))
	{
		if (!first)
			o << " || ";
		if (!optimizedSolutions->isProduct(partId) || isOptimizedProductWorthPrinting(partId))
			printOptimizedCppId(o, partId);
		else
			printCppImplicant(o, optimizedSolutions->getProduct(partId).implicant, false);
	}
}

void OutputComposer::printOptimizedMathSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	First first;
	const OptimizedSolutions::sum_t &sum = optimizedSolutions->getSum(sumId);
	for (const auto &partId : sum)
	{
		if (!first)
			printHumanOr(o, true);
		if (!optimizedSolutions->isProduct(partId) || isOptimizedProductWorthPrintingForMath(partId))
			printOptimizedMathId(o, partId);
		else
			printOptimizedMathProductBody(o, partId, sum.size() != 1);
	}
}

void OutputComposer::printOptimizedHumanSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	o << '\t';
	printOptimizedHumanId(o, sumId);
	o << " = ";
	printOptimizedHumanSumBody(o, sumId);
	o << '\n';
}

void OutputComposer::printOptimizedGraphSum(std::ostream &o, const Names &functionNames, const OptimizedSolutions::id_t sumId) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	const bool isVerboseGraph = options::verboseGraph.isRaised();
	const OptimizedSolutions::sum_t &sum = optimizedSolutions->getSum(sumId);
	std::size_t functionNum = isFullGraph ? SIZE_MAX : optimizedSolutions->findSumEndNode(sumId);
	o << "\t\t";
	printOptimizedGraphId(o, sumId);
	o << " [label=\"";
	const bool hasParents = isFullGraph || std::any_of(sum.cbegin(), sum.cend(), [this](const OptimizedSolutions::id_t id){ return !optimizedSolutions->isProduct(id) || isOptimizedProductWorthPrinting(id); });
	if (hasParents)
	{
		printGraphOr(o, false);
		o << "\\n";
	}
	if (functionNum == SIZE_MAX)
	{
		printOptimizedHumanId(o, sumId);
	}
	else
	{
		while (true)
		{
			functionNames.printGraphName(o, functionNum);
			const std::size_t additionalFunNum = optimizedSolutions->findSumEndNode(sumId, functionNum + 1);
			if (additionalFunNum == SIZE_MAX)
				break;
			o << ", ";
			functionNum = additionalFunNum;
		}
	}
	if (!isFullGraph || isVerboseGraph)
		printOptimizedGraphSumProducts(o, sumId);
	o << '"';
	if (functionNum != SIZE_MAX)
		o << ", style=filled";
	o << "];\n";
	if (hasParents)
	{
		o << "\t\t";
		printOptimizedGraphSumParents(o, sumId);
		o << " -> ";
		printOptimizedGraphId(o, sumId);
		o << ";\n";
	}
}

void OutputComposer::printOptimizedVerilogSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	o << "\tassign ";
	printOptimizedVerilogId(o, sumId);
	o << " = ";
	printOptimizedVerilogSumBody(o, sumId);
	o << ";\n";
}

void OutputComposer::printOptimizedVhdlSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	o << '\t';
	printOptimizedVhdlId(o, sumId);
	o << " <= ";
	printOptimizedVhdlSumBody(o, sumId);
	o << ";\n";
}

void OutputComposer::printOptimizedCppSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	o << '\t';
	printOptimizedCppId(o, sumId);
	o << " = ";
	printOptimizedCppSumBody(o, sumId);
	o << ";\n";
}

void OutputComposer::printOptimizedMathSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const
{
	printOptimizedMathId(o, sumId);
	o << " = ";
	printOptimizedMathSumBody(o, sumId);
	o << '\n';
}

void OutputComposer::printOptimizedGraphFinalSum(std::ostream &o, const Names &functionNames, const std::size_t i) const
{
	const bool isVerboseGraph = options::verboseGraph.isRaised();
	const OptimizedSolutions::id_t sumId = optimizedSolutions->getFinalSums()[i];
	o << "\t\tf" << i << " [label=\"";
	if (!isOptimizedSumWorthPrinting(sumId, false) && optimizedSolutions->getSum(sumId).size() > 1)
	{
		printGraphOr(o, false);
		o << "\\n";
	}
	functionNames.printGraphName(o, i);
	if (isVerboseGraph)
		printOptimizedGraphSumProducts(o, sumId);
	o << "\"];\n";
	o << "\t\t";
	if (isOptimizedSumWorthPrinting(sumId, false))
		printOptimizedGraphId(o, sumId);
	else
		printOptimizedGraphSumParents(o, sumId);
	o << " -> f" << i << ";\n";
}

void OutputComposer::printOptimizedHumanFinalSums(std::ostream &o, const Names &functionNames) const
{
	for (std::size_t i = 0; i != optimizedSolutions->getFinalSums().size(); ++i)
	{
		o << "\t\"";
		functionNames.printHumanName(o, i);
		o << "\" = ";
		const OptimizedSolutions::id_t sumId = optimizedSolutions->getFinalSums()[i];
		if (isOptimizedSumWorthPrinting(sumId, false))
			printOptimizedHumanId(o, sumId);
		else
			printOptimizedHumanSumBody(o, sumId);
		o << '\n';
	}
}

void OutputComposer::printOptimizedGraphFinalSums(std::ostream &o, const Names &functionNames) const
{
	if (optimizedSolutions->getFinalSums().empty())
		return;
	o << "\tsubgraph final_sums\n";
	o << "\t{\n";
	o << "\t\tnode [shape=rectangle, style=filled];\n";
	for (std::size_t i = 0; i != optimizedSolutions->getFinalSums().size(); ++i)
		printOptimizedGraphFinalSum(o, functionNames, i);
	o << "\t}\n";
}

void OutputComposer::printOptimizedVerilogFinalSums(std::ostream &o, const Names &functionNames) const
{
	if (!optimizedSolutions->getFinalSums().empty())
	{
		o << "\t// Results\n";
		for (std::size_t i = 0; i != optimizedSolutions->getFinalSums().size(); ++i)
		{
			o << "\tassign ";
			functionNames.printVerilogName(o, i);
			o << " = ";
			const OptimizedSolutions::id_t sumId = optimizedSolutions->getFinalSums()[i];
			if (isOptimizedSumWorthPrinting(sumId, true))
				printOptimizedVerilogId(o, optimizedSolutions->getFinalSums()[i]);
			else
				printOptimizedVerilogSumBody(o, sumId);
			o << ";\n";
		}
		o << "\t\n";
	}
}

void OutputComposer::printOptimizedVhdlFinalSums(std::ostream &o, const Names &functionNames) const
{
	if (!optimizedSolutions->getFinalSums().empty())
	{
		o << "\t\n";
		o << "\t-- Results\n";
		for (std::size_t i = 0; i != optimizedSolutions->getFinalSums().size(); ++i)
		{
			o << '\t';
			functionNames.printVhdlName(o, i);
			o << " <= ";
			const OptimizedSolutions::id_t sumId = optimizedSolutions->getFinalSums()[i];
			if (isOptimizedSumWorthPrinting(sumId, true))
				printOptimizedVhdlId(o, optimizedSolutions->getFinalSums()[i]);
			else
				printOptimizedVhdlSumBody(o, sumId);
			o << ";\n";
		}
	}
}

void OutputComposer::printOptimizedCppFinalSums(std::ostream &o, const Names &functionNames) const
{
	o << "\t// Results\n";
	if (optimizedSolutions->getFinalSums().empty())
	{
		o << "\treturn {};\n";
	}
	else
	{
		o << "\toutput_t o = {};\n";
		for (std::size_t i = 0; i != optimizedSolutions->getFinalSums().size(); ++i)
		{
			o << '\t';
			functionNames.printCppName(o, i);
			o << " = ";
			const OptimizedSolutions::id_t sumId = optimizedSolutions->getFinalSums()[i];
			if (isOptimizedSumWorthPrinting(sumId, true))
				printOptimizedCppId(o, optimizedSolutions->getFinalSums()[i]);
			else
				printOptimizedCppSumBody(o, sumId);
			o << ";\n";
		}
		o << "\treturn o;\n";
	}
}

void OutputComposer::printOptimizedMathFinalSums(std::ostream &o, const Names &functionNames) const
{
	if (!optimizedSolutions->getFinalSums().empty())
	{
		for (std::size_t i = 0; i != optimizedSolutions->getFinalSums().size(); ++i)
		{
			functionNames.printMathName(o, i);
			o << '(';
			::inputNames.printMathNames(o);
			o << ") = ";
			const OptimizedSolutions::id_t sumId = optimizedSolutions->getFinalSums()[i];
			if (isOptimizedSumWorthPrintingForMath(sumId))
				printOptimizedMathId(o, optimizedSolutions->getFinalSums()[i]);
			else
				printOptimizedMathSumBody(o, sumId);
			o << '\n';
		}
	}
}

void OutputComposer::printOptimizedHumanNegatedInputs(std::ostream &o) const
{
	o << "Negated inputs:";
	First first;
	for (Minterm i = 0; i != ::bits; ++i)
	{
		if ((optimizedSolutions->getNegatedInputs() & (1 << (::bits - i - 1))) != 0)
		{
			if (!first)
				o << ',';
			o << ' ';
			::inputNames.printHumanName(o, i);
		}
	}
	if (first)
		o << " <none>";
	o << '\n';
}

void OutputComposer::printOptimizedGraphNegatedInputs(std::ostream &o) const
{
	if (optimizedSolutions->getNegatedInputs() == 0)
		return;
	o << "\tsubgraph negated_inputs\n";
	o << "\t{\n";
	o << "\t\tnode [shape=diamond];\n";
	for (Minterm i = 0; i != ::bits; ++i)
	{
		if ((optimizedSolutions->getNegatedInputs() & (1 << (::bits - i - 1))) != 0)
		{
			o << "\t\tni" << i << " [label=\"";
			printGraphNot(o);
			::inputNames.printGraphName(o, i);
			o << "\"];\n";
			o << "\t\ti" << i << " -> ni" << i << ";\n";
		}
	}
	o << "\t}\n";
}

void OutputComposer::printOptimizedHumanProductBody(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	const auto &[primeImplicant, ids] = optimizedSolutions->getProduct(productId);
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		printHumanImplicant(o, primeImplicant, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			printHumanAnd(o);
		else
			needsAnd = true;
		printOptimizedHumanId(o, id);
	}
}

void OutputComposer::printOptimizedVerilogProductBody(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	const auto &[primeImplicant, ids] = optimizedSolutions->getProduct(productId);
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		printVerilogImplicant(o, primeImplicant, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			o << " & ";
		else
			needsAnd = true;
		printOptimizedVerilogId(o, id);
	}
}

void OutputComposer::printOptimizedVhdlProductBody(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	const auto &[primeImplicant, ids] = optimizedSolutions->getProduct(productId);
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		printVhdlImplicant(o, primeImplicant, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			o << " and ";
		else
			needsAnd = true;
		printOptimizedVhdlId(o, id);
	}
}

void OutputComposer::printOptimizedCppProductBody(std::ostream &o, const OptimizedSolutions::id_t productId) const
{
	const auto &[primeImplicant, ids] = optimizedSolutions->getProduct(productId);
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		printCppImplicant(o, primeImplicant, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			o << " && ";
		else
			needsAnd = true;
		printOptimizedCppId(o, id);
	}
}

void OutputComposer::printOptimizedMathProductBody(std::ostream &o, const OptimizedSolutions::id_t productId, const bool parentheses) const
{
	const auto &[primeImplicant, ids] = optimizedSolutions->getProduct(productId);
	const bool parenthesesNeeded = parentheses && primeImplicant.getBitCount() + ids.size() >= 2;
	if (parenthesesNeeded)
		o << '(';
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		printHumanImplicant(o, primeImplicant, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			printHumanAnd(o);
		else
			needsAnd = true;
		printOptimizedMathId(o, id);
	}
	if (parenthesesNeeded)
		o << ')';
}

void OutputComposer::printOptimizedHumanSolution_(std::ostream &o, const Names &functionNames)
{
	generateOptimizedHumanIds();
	printOptimizedHumanNegatedInputs(o);
	printOptimizedHumanProducts(o);
	printOptimizedHumanSums(o);
	printOptimizedHumanFinalSums(o, functionNames);
	if (options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT)
	{
		o << '\n';
		printGateCost(o, *optimizedSolutions, false);
	}
}

void OutputComposer::printOptimizedGraphSolution_(std::ostream &o, const Names &functionNames)
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	generateOptimizedGraphIds();
	if (isFullGraph)
		printOptimizedGraphNegatedInputs(o);
	printOptimizedGraphProducts(o, functionNames);
	printOptimizedGraphSums(o, functionNames);
	if (isFullGraph)
		printOptimizedGraphFinalSums(o, functionNames);
}

void OutputComposer::printOptimizedVerilogSolution_(std::ostream &o, const Names &functionNames)
{
	const auto [immediateProductCount, immediateSumCount] = generateOptimizedNormalizedIds();
	printOptimizedVerilogImmediates(o, immediateProductCount, immediateSumCount);
	printOptimizedVerilogProducts(o);
	printOptimizedVerilogSums(o);
	printOptimizedVerilogFinalSums(o, functionNames);
}

void OutputComposer::printOptimizedVhdlSolution_(std::ostream &o, const Names &functionNames)
{
	const auto [immediateProductCount, immediateSumCount] = generateOptimizedNormalizedIds();
	printOptimizedVhdlImmediates(o, immediateProductCount, immediateSumCount);
	o << "begin\n";
	printOptimizedVhdlProducts(o);
	printOptimizedVhdlSums(o);
	printOptimizedVhdlFinalSums(o, functionNames);
	o << "\t\n";
}

void OutputComposer::printOptimizedCppSolution_(std::ostream &o, const Names &functionNames)
{
	const auto [immediateProductCount, immediateSumCount] = generateOptimizedNormalizedIds();
	printOptimizedCppImmediates(o, immediateProductCount, immediateSumCount);
	printOptimizedCppProducts(o);
	printOptimizedCppSums(o);
	printOptimizedCppFinalSums(o, functionNames);
}

void OutputComposer::printOptimizedMathSolution_(std::ostream &o, const Names &functionNames)
{
	const auto [immediateProductCount, immediateSumCount] = generateOptimizedMathIds();
	if (immediateProductCount != 0 || immediateSumCount != 0)
	{
		o << "Let:\n";
		printOptimizedMathProducts(o);
		printOptimizedMathSums(o);
		o << "\nThen:\n";
	}
	printOptimizedMathFinalSums(o, functionNames);
}

void OutputComposer::printOptimizedHumanSolution(std::ostream &o)
{
	printOptimizedHumanSolution_(o, functionNames);
}

void OutputComposer::printOptimizedGraphSolution(std::ostream &o)
{
	printOptimizedGraphSolution_(o, functionNames);
}

void OutputComposer::printOptimizedVerilogSolution(std::ostream &o)
{
	printOptimizedVerilogSolution_(o, functionNames);
}

void OutputComposer::printOptimizedVhdlSolution(std::ostream &o, const Names &functionNames)
{
	printOptimizedVhdlSolution_(o, functionNames);
}

void OutputComposer::printOptimizedCppSolution(std::ostream &o, const Names &functionNames)
{
	printOptimizedCppSolution_(o, functionNames);
}

void OutputComposer::printOptimizedMathSolution(std::ostream &o, const Names &functionNames)
{
	printOptimizedMathSolution_(o, functionNames);
}

void OutputComposer::printHuman(std::ostream &o)
{
	if (options::outputBanner.getValue())
		printBanner(o);
	const bool solutionsVisible = options::skipOptimization.isRaised() || options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT;
	if (solutionsVisible)
	{
		printHumanSolutions(o);
		if (options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT)
		{
			if (solutions.size() != 1 && options::skipOptimization.isRaised())
			{
				if (!solutions.empty())
					o << "\n\n=== summary ===\n" << '\n';
				printGateCost(o, solutions, false);
			}
		}
	}
	if (!options::skipOptimization.isRaised())
	{
		if (options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT)
		{
			if (!solutions.empty())
				o << "\n\n";
			o << "=== optimized solution ===\n" << '\n';
		}
		printOptimizedHumanSolution(o);
		o << std::flush;
	}
}

void OutputComposer::printGraph(std::ostream &o)
{
	if (options::outputBanner.getValue())
	{
		o << "// ";
		printBanner(o);
	}
	o << "digraph " << getName() << '\n';
	o << "{\n";
	if (options::outputFormat.getValue() == options::OutputFormat::GRAPH)
		printGraphRoots(o);
	if (options::skipOptimization.isRaised())
		printGraphSolutions(o);
	else
		printOptimizedGraphSolution(o);
	o << "}\n";
}

void OutputComposer::printVerilog(std::ostream &o)
{
	if (options::outputBanner.getValue())
	{
		o << "// ";
		printBanner(o);
	}
	o << "module " << getName() << " (\n";
	if (!::inputNames.empty())
	{
		o << "\tinput wire";
		::inputNames.printVerilogNames(o);
		o << '\n';
	}
	if (!karnaughs.empty())
	{
		o << "\toutput wire";
		functionNames.printVerilogNames(o);
		o << '\n';
	}
	o << ");\n";
	o << "\t\n";
	if (options::skipOptimization.isRaised())
		printVerilogSolutions(o);
	else
		printOptimizedVerilogSolution(o);
	o << "endmodule" << std::endl;
}

void OutputComposer::printVhdl(std::ostream &o)
{
	if (options::outputBanner.getValue())
	{
		o << "-- ";
		printBanner(o);
	}
	o << "library IEEE;\n"
			"use IEEE.std_logic_1164.all;\n";
	o << '\n';
	o << "entity " << getName() << " is\n";
	if (!::inputNames.empty() || !karnaughs.empty())
	{
		o << "\tport(\n";
		if (!::inputNames.empty())
		{
			o << "\t\t";
			::inputNames.printVhdlNames(o);
			o << " : in ";
			::inputNames.printVhdlType(o);
			if (!karnaughs.empty())
				o << ';';
			o << '\n';
		}
		if (!karnaughs.empty())
		{
			o << "\t\t";
			functionNames.printVhdlNames(o);
			o << " : out ";
			functionNames.printVhdlType(o);
			o << '\n';
		}
		o << "\t);\n";
	}
	o << "end " << getName() << ";\n";
	o << '\n';
	o << "architecture behavioural of " << getName() << " is\n";
	if (options::skipOptimization.isRaised())
		printVhdlSolutions(o, functionNames);
	else
		printOptimizedVhdlSolution(o, functionNames);
	o << "end behavioural;\n";
}

void OutputComposer::printCpp(std::ostream &o)
{
	if (options::outputBanner.getValue())
	{
		o << "// ";
		printBanner(o);
	}
	if (!::inputNames.areNamesUsedInCode() || !functionNames.areNamesUsedInCode())
		o << "#include <array>\n"
				"\n";
	o << "class " << getName() << "\n"
			"{\n"
			"public:\n";
	o << "\tusing input_t = ";
	::inputNames.printCppType(o);
	o << ";\n";
	o << "\tusing output_t = ";
	functionNames.printCppType(o);
	o << ";\n";
	o << "\t\n";
	o << "\t[[nodiscard]] constexpr output_t operator()(";
	{
		First first;
		for (std::size_t i = 0; i != ::inputNames.size(); ++i)
		{
			if (!first)
				o << ", ";
			o << "const bool ";
			::inputNames.printCppRawName(o, i);
		}
	}
	o << ") const { return (*this)({";
	{
		First first;
		for (std::size_t i = 0; i != ::inputNames.size(); ++i)
		{
			if (!first)
				o << ", ";
			::inputNames.printCppRawName(o, i);
		}
	}
	o << "}); }\n";
	o << "\t[[nodiscard]] constexpr output_t operator()(const input_t &i) const { return calc(i); }\n";
	o << "\t\n";
	o << "\t[[nodiscard]] static constexpr output_t calc(";
	{
		First first;
		for (std::size_t i = 0; i != ::inputNames.size(); ++i)
		{
			if (!first)
				o << ", ";
			o << "const bool ";
			::inputNames.printCppRawName(o, i);
		}
	}
	o << ") { return calc({";
	{
		First first;
		for (std::size_t i = 0; i != ::inputNames.size(); ++i)
		{
			if (!first)
				o << ", ";
			::inputNames.printCppRawName(o, i);
		}
	}
	o << "}); }\n";
	o << "\t[[nodiscard]] static constexpr output_t calc(const input_t &i);\n";
	o << "};" << std::endl;
	o << '\n';
	o << "constexpr " << getName() << "::output_t " << getName() << "::calc(const input_t &" << (areInputsUsed() ? "i" : "") << ")\n";
	o << "{\n";
	if (options::skipOptimization.isRaised())
		printCppSolutions(o, functionNames);
	else
		printOptimizedCppSolution(o, functionNames);
	o << "}\n";
}

void OutputComposer::printMath(std::ostream &o)
{
	if (options::outputBanner.getValue())
		printBanner(o);
	if (options::skipOptimization.isRaised())
		printMathSolutions(o, functionNames);
	else
		printOptimizedMathSolution(o, functionNames);
}

void OutputComposer::printGateCost(std::ostream &o)
{
	if (options::outputBanner.getValue())
		printBanner(o);
	for (const Solution &solution : solutions)
		printGateCost(o, solution, true);
	o << "=== summary ===\n";
	printGateCost(o, solutions, true);
	if (!options::skipOptimization.isRaised())
	{
		o << "=== optimized solution ===\n";
		printGateCost(o, *optimizedSolutions, true);
	}
}

void OutputComposer::compose(std::ostream &o)
{
	switch (options::outputFormat.getValue())
	{
	case options::OutputFormat::HUMAN_LONG:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_SHORT:
		printHuman(o);
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		printGraph(o);
		break;
	case options::OutputFormat::VERILOG:
		printVerilog(o);
		break;
	case options::OutputFormat::VHDL:
		printVhdl(o);
		break;
	case options::OutputFormat::CPP:
		printCpp(o);
		break;
	case options::OutputFormat::MATHEMATICAL:
		printMath(o);
		break;
	case options::OutputFormat::GATE_COSTS:
		printGateCost(o);
		break;
	}
}
