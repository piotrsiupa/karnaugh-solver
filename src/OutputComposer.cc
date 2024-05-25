#include "./OutputComposer.hh"

#include <filesystem>
#include <iomanip>

#include "info.hh"
#include "options.hh"


bool OutputComposer::isHuman()
{
	const options::OutputFormat outputFormat = options::outputFormat;
	return outputFormat == options::OutputFormat::HUMAN_SHORT || outputFormat == options::OutputFormat::HUMAN || outputFormat == options::OutputFormat::HUMAN_LONG;
}

bool OutputComposer::isGraph()
{
	const options::OutputFormat outputFormat = options::outputFormat;
	return outputFormat == options::OutputFormat::GRAPH || outputFormat == options::OutputFormat::REDUCED_GRAPH;
}

bool OutputComposer::isHumanReadable()
{
	const options::OutputFormat outputFormat = options::outputFormat;
	return isHuman() || isGraph() || outputFormat == options::OutputFormat::MATHEMATICAL || outputFormat == options::OutputFormat::GATE_COSTS;
}

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
	if (options::skipOptimization)
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
		if (std::any_of(solution.cbegin(), solution.cend(), [](const Implicant &implicant){ return implicant.getBitCount() != 0; }))
			return true;
	return false;
}

bool OutputComposer::isOptimizedProductWorthPrintingInGeneral(const OptimizedSolutions::id_t productId) const
{
	const OptimizedSolutions::product_t &product = optimizedSolutions->getProduct(productId);
	return product.implicant.getBitCount() >= 2 || !product.subProducts.empty();
}

bool OutputComposer::isOptimizedProductWorthPrinting(const OptimizedSolutions::id_t productId) const
{
	const bool isWorthPrintingInGeneral = isOptimizedProductWorthPrintingInGeneral(productId);
	switch (options::outputFormat)
	{
	case options::OutputFormat::REDUCED_GRAPH:
		return isWorthPrintingInGeneral || optimizedSolutions->findProductEndNode(productId) != SIZE_MAX;
	case options::OutputFormat::MATHEMATICAL:
		return isWorthPrintingInGeneral && optimizedSolutions->getIdUseCount(productId) >= 2;
	default:
		return isWorthPrintingInGeneral;
	}
}

bool OutputComposer::isOptimizedSumWorthPrintingInGeneral(const OptimizedSolutions::id_t sumId) const
{
	if (isHuman() || isGraph())
	{
		if (std::count(optimizedSolutions->getFinalSums().cbegin(), optimizedSolutions->getFinalSums().cend(), sumId) >= 2 && optimizedSolutions->getSum(sumId).size() >= 2)
			return true;
		for (std::size_t i = 0; i != optimizedSolutions->getSumCount(); ++i)
			for (const OptimizedSolutions::id_t &id : optimizedSolutions->getSum(optimizedSolutions->makeSumId(i)))
				if (id == sumId)
					return true;
		return false;
	}
	else
	{
		return optimizedSolutions->getSum(sumId).size() >= 2;
	}
}

bool OutputComposer::isOptimizedSumWorthPrinting(const OptimizedSolutions::id_t sumId) const
{
	switch (options::outputFormat)
	{
	case options::OutputFormat::REDUCED_GRAPH:
		return optimizedSolutions->getSum(sumId).size() != 1;
	case options::OutputFormat::MATHEMATICAL:
		return isOptimizedSumWorthPrintingInGeneral(sumId) && (optimizedSolutions->getIdUseCount(sumId) + std::count(optimizedSolutions->getFinalSums().cbegin(), optimizedSolutions->getFinalSums().cend(), sumId) >= 2);
	default:
		return isOptimizedSumWorthPrintingInGeneral(sumId);
	}
}

std::string OutputComposer::getName()
{
	if (options::name)
		return *options::name;
	else if (::inputFilePath)
		return std::filesystem::path(*::inputFilePath).stem().string();
	else
		return "Karnaugh";
}

std::pair<std::size_t, std::size_t> OutputComposer::generateOptimizedNormalizedIds()
{
	const OptimizedSolutions::id_t idCount = optimizedSolutions->getProductCount() + optimizedSolutions->getSumCount();
	normalizedOptimizedIds.resize(idCount);
	
	const bool isSingleCounter = isHuman() || isGraph();
	
	OptimizedSolutions::id_t currentNormalizedId0 = 0, currentNormalizedId1 = 0;
	for (OptimizedSolutions::id_t id = 0; id != idCount; ++id)
	{
		const bool isProduct = optimizedSolutions->isProduct(id);
		if (isProduct ? isOptimizedProductWorthPrinting(id) : isOptimizedSumWorthPrinting(id))
			normalizedOptimizedIds[id] = (isProduct || isSingleCounter) ? currentNormalizedId0++ : currentNormalizedId1++;
	}
	return {currentNormalizedId0, currentNormalizedId1};
}

void OutputComposer::printNormalizedId(const OptimizedSolutions::id_t id, const bool useHumanAnyway) const
{
	if (useHumanAnyway)
		goto human;
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
		human:
		o << '[' << normalizedOptimizedIds[id] << ']';
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		o << 's' << normalizedOptimizedIds[id];
		break;
	case options::OutputFormat::VERILOG:
	case options::OutputFormat::CPP:
		if (optimizedSolutions->isProduct(id))
			o << "prods[" << normalizedOptimizedIds[id] << ']';
		else
			o << "sums[" << normalizedOptimizedIds[id] << ']';
		break;
	case options::OutputFormat::VHDL:
		if (optimizedSolutions->isProduct(id))
			o << "prods(" << normalizedOptimizedIds[id] << ')';
		else
			o << "sums(" << normalizedOptimizedIds[id] << ')';
		break;
	case options::OutputFormat::MATHEMATICAL:
		if (optimizedSolutions->isProduct(id))
			o << 'p' << (normalizedOptimizedIds[id] + 1);
		else
			o << 's' << (normalizedOptimizedIds[id] + 1);
		printOptimizedMathArgs(id);
		break;
	case options::OutputFormat::GATE_COSTS:
		// not expected to be needed
		break;
	}
}

void OutputComposer::printBanner() const
{
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_LONG:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::MATHEMATICAL:
	case options::OutputFormat::GATE_COSTS:
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
	case options::OutputFormat::VERILOG:
	case options::OutputFormat::CPP:
		o << "// ";
		break;
	case options::OutputFormat::VHDL:
		o << "-- ";
		break;
	}
	
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

void OutputComposer::printShortBool(const Trilean value) const
{
	switch (value.get())
	{
	case Trilean::Value::FALSE:
		switch (options::outputOperators)
		{
		case options::OutputOperators::FORMAL:
		case options::OutputOperators::ASCII:
			o << '0';
			break;
		case options::OutputOperators::PROGRAMMING:
		case options::OutputOperators::NAMES:
			o << 'F';
			break;
		}
		break;
	case Trilean::Value::TRUE:
		switch (options::outputOperators)
		{
		case options::OutputOperators::FORMAL:
		case options::OutputOperators::ASCII:
			o << '1';
			break;
		case options::OutputOperators::PROGRAMMING:
		case options::OutputOperators::NAMES:
			o << 'T';
			break;
		}
		break;
	case Trilean::Value::UNDEFINED:
		switch (options::outputOperators)
		{
		case options::OutputOperators::FORMAL:
		case options::OutputOperators::ASCII:
			o << 'X';
			break;
		case options::OutputOperators::PROGRAMMING:
			o << '-';
			break;
		case options::OutputOperators::NAMES:
			o << '?';
			break;
		}
		break;
	}
}

void OutputComposer::printBool(const bool value, const bool strictlyForCode) const
{
	if (strictlyForCode)
		goto programming;
	switch (options::outputFormat)
	{
	case options::OutputFormat::VERILOG:
		o << (value ? '1' : '0');
		break;
	case options::OutputFormat::VHDL:
		o << (value ? "'1'" : "'0'");
		break;
	case options::OutputFormat::CPP:
		o << (value ? "true" : "false");
		break;
	default:
		switch (options::outputOperators)
		{
		case options::OutputOperators::FORMAL:
			o << (value ? u8"\u22A4" : u8"\u22A5");
			break;
		case options::OutputOperators::ASCII:
			o << (value ? 'T' : 'F');
			break;
		case options::OutputOperators::PROGRAMMING:
			programming:
			o << (value ? "true" : "false");
			break;
		case options::OutputOperators::NAMES:
			o << (value ? "TRUE" : "FALSE");
			break;
		default:
			break;
		}
		break;
	}
}

void OutputComposer::printNot() const
{
	switch (options::outputFormat)
	{
	case options::OutputFormat::VERILOG:
	case options::OutputFormat::CPP:
		o << '!';
		break;
	case options::OutputFormat::VHDL:
		o << "not ";
		break;
	default:
		switch (options::outputOperators)
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
}

void OutputComposer::printAnd(const bool spaces) const
{
	if (spaces)
		o << ' ';
	switch (options::outputFormat)
	{
	case options::OutputFormat::VERILOG:
		o << '&';
		break;
	case options::OutputFormat::VHDL:
		o << "and";
		break;
	case options::OutputFormat::CPP:
		o << "&&";
		break;
	default:
		switch (options::outputOperators)
		{
		case options::OutputOperators::FORMAL:
			o << u8"\u2227";
			break;
		case options::OutputOperators::ASCII:
			if (options::outputFormat == options::OutputFormat::GRAPH || options::outputFormat == options::OutputFormat::REDUCED_GRAPH)
				o << "/\\\\";
			else
				o << "/\\";
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
	}
	if (spaces)
		o << ' ';
}

void OutputComposer::printOr(const bool spaces) const
{
	if (spaces)
		o << ' ';
	switch (options::outputFormat)
	{
	case options::OutputFormat::VERILOG:
		o << '|';
		break;
	case options::OutputFormat::VHDL:
		o << "or";
		break;
	case options::OutputFormat::CPP:
		o << "||";
		break;
	default:
		switch (options::outputOperators)
		{
		case options::OutputOperators::FORMAL:
			o << u8"\u2228";
			break;
		case options::OutputOperators::ASCII:
			if (options::outputFormat == options::OutputFormat::GRAPH || options::outputFormat == options::OutputFormat::REDUCED_GRAPH)
				o << "\\\\/";
			else
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
	}
	if (spaces)
		o << ' ';
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

void OutputComposer::printBits(const Minterm minterm, const bits_t bitCount) const
{
	for (bits_t i = bitCount; i != 0; --i)
		o << ((minterm & (1 << (i - 1))) != 0 ? '1' : '0');
}

void OutputComposer::prettyPrintTable(const Minterms &target, const Minterms &allowed) const
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
		printBits(y, hBits);
		o << ' ';
	}
	o << '\n';
	for (const Minterm x : vGrayCode)
	{
		printBits(x, std::max(bits_t(1), vBits));
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
			const Trilean value = Trilean::fromTrueAndFalse(target.find(minterm) != target.cend(), allowed.find(minterm) == allowed.cend());
			printShortBool(value);
		}
		o << '\n';
	}
	o << std::endl;
}

void OutputComposer::prettyPrintTable(const std::size_t i) const
{
	const Karnaugh &karnaugh = karnaughs[i];
	return prettyPrintTable(karnaugh.getTargetMinterms(), karnaugh.getAllowedMinterms());
}

void OutputComposer::prettyPrintSolution(const Solution &solution) const
{
	Minterms minterms;
	for (const auto &implicant : solution)
	{
		const auto newMinterms = implicant.findMinterms();
		minterms.insert(newMinterms.cbegin(), newMinterms.end());
	}
	prettyPrintTable(minterms);
}

void OutputComposer::printImplicant(const Implicant &implicant, const bool parentheses, const bool useHumanAnyway) const
{
	const bool isGraphParents = isGraph() && !useHumanAnyway;
	
	if (implicant.getBitCount() == 0)
	{
		printBool(!implicant.isError(), isGraphParents);
		return;
	}
	
	const bool needsParentheses = parentheses && implicant.getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	
	First first;
	for (const auto &[bitIndex, negated] : implicant.splitBits())
	{
		if (isGraphParents)
		{
			if (!first)
				o << ", ";
			if (negated)
				o << 'n';
			o << 'i' << static_cast<unsigned>(bitIndex);
		}
		else
		{
			if (!first)
				printAnd(true);
			if (negated)
				printNot();
			::inputNames.printName(o, bitIndex);
		}
	}
	
	if (needsParentheses)
		o << ')';
}

void OutputComposer::printGateCost(const GateCost &gateCost, const bool full) const
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

void OutputComposer::printGraphNegatedInputs(const Solution &solution, const std::size_t functionNum) const
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
				printNot();
				::inputNames.printName(o, i);
				o << "\"];\n";
				o << "\t\t\ti" << i << " -> f" << functionNum << "_ni" << i << '_' << j << ";\n";
			}
		}
		o << "\t\t}\n";
	}
}

inline void OutputComposer::printGraphParentBit(const std::size_t functionNum, const Implicant::splitBit_t &splitBit, const std::size_t i) const
{
	const auto &[bit, negated] = splitBit;
	if (negated)
		o << 'f' << functionNum << "_ni" << static_cast<unsigned>(bit) << '_' << i;
	else
		o << 'i' << static_cast<unsigned>(bit);
}

std::size_t OutputComposer::printGraphProducts(const Solution &solution, const std::size_t functionNum, std::size_t idShift) const
{
	if (std::any_of(solution.cbegin(), solution.cend(), [](const Implicant &x){ return x.getBitCount() >= 2; }))
	{
		const bool isFullGraph = options::outputFormat == options::OutputFormat::GRAPH;
		const bool isVerbose = options::verboseGraph;
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
				printAnd(false);
				o << "\\n";
			}
			o << "[" << idShift++ << "]";
			if (!isFullGraph || isVerbose)
			{
				o << " = ";
				printImplicant(solution[i], false, true);
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
					printGraphParentBit(functionNum, splitBit, i);
				}
				o << " -> f" << functionNum << "_s" << i << ";\n";
			}
		}
		o << "\t\t}\n";
	}
	return idShift;
}

void OutputComposer::printGraphSum(const Solution &solution, const std::size_t functionNum) const
{
	const bool isFullGraph = options::outputFormat == options::OutputFormat::GRAPH;
	const bool isVerbose = options::verboseGraph;
	o << "\t\tsubgraph sum\n";
	o << "\t\t{\n";
	o << "\t\t\tnode [shape=rectangle, style=filled];\n";
	o << "\t\t\tf" << functionNum << " [label=\"";
	const bool hasParents = isFullGraph || (solution.size() >= 2 && std::any_of(solution.cbegin(), solution.cend(), [](const Implicant &x){ return x.getBitCount() >= 2; }));
	if (hasParents && solution.size() >= 2)
	{
		printOr(false);
		o << "\\n";
	}
	functionNames.printName(o, functionNum);
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
				printOr(true);
			printImplicant(product, solution.size() != 1, true);
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
				printBool(!solution[i].isError(), true);
				break;
			case 1:
				printGraphParentBit(functionNum, solution[i].splitBits().front(), i);
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

std::size_t OutputComposer::printSolution(const std::size_t i, std::size_t idShift) const
{
	const Solution solution = Solution(solutions[i]).sort();
	
	if (isGraph())
	{
		const bool isFullGraph = options::outputFormat == options::OutputFormat::GRAPH;
		if (isFullGraph)
			printGraphNegatedInputs(solution, i);
		if (isFullGraph || solution.size() >= 2)
			idShift = printGraphProducts(solution, i, idShift);
		printGraphSum(solution, i);
		return idShift;
	}
	else if (options::outputFormat == options::OutputFormat::GATE_COSTS)
	{
		printGateCost(solution, true);
		return 0;
	}
	
	if (isHuman())
	{
		const Karnaugh &karnaugh = karnaughs[i];
		if (options::outputFormat == options::OutputFormat::HUMAN_LONG)
		{
			if (::bits <= 8)
			{
				o << "goal:\n";
				prettyPrintTable(i);
				
				if (karnaugh.getTargetMinterms().size() != karnaugh.getAllowedMinterms().size())
				{
					o << "best fit:\n";
					prettyPrintSolution(solution);
				}
			}
			else
			{
				o << "The Karnaugh map is too big to be displayed.\n\n";
			}
			o << "solution:\n";
		}
	}
	
	if (solution.size() == 1)
	{
		printImplicant(solution.front(), false);
	}
	else
	{
		First first;
		for (const Implicant &implicant : solution)
		{
			if (!first)
				printOr(true);
			printImplicant(implicant, true);
		}
	}
	
	if (isHuman())
	{
		o << '\n';
		if (options::outputFormat != options::OutputFormat::HUMAN_SHORT)
		{
			o << '\n';
			printGateCost(solution, false);
		}
	}
	
	return 0;
}

void OutputComposer::printSolutions() const
{
	switch (options::outputFormat)
	{
	case options::OutputFormat::VHDL:
		o << "begin\n" << "\t\n";
		break;
	default:
		break;
	}
	
	if (karnaughs.empty())
	{
		if (options::outputFormat == options::OutputFormat::CPP)
			o << "\treturn {};\n";
	}
	else
	{
		switch (options::outputFormat)
		{
		case options::OutputFormat::CPP:
			o << "\toutput_t o = {};\n";
			break;
		default:
			break;
		}
		
		First first;
		std::size_t idShift = 0;
		for (std::size_t i = 0; i != functionNames.size(); ++i)
		{
			switch (options::outputFormat)
			{
			case options::OutputFormat::HUMAN_LONG:
			case options::OutputFormat::HUMAN:
				if (!first)
					o << '\n' << '\n';
				[[fallthrough]];
			case options::OutputFormat::HUMAN_SHORT:
				o << "--- ";
				break;
			case options::OutputFormat::GRAPH:
			case options::OutputFormat::REDUCED_GRAPH:
				o << "\tsubgraph function_" << i << '\n';
				o << "\t{\n";
				break;
			case options::OutputFormat::VERILOG:
				o << "\tassign ";
				break;
			case options::OutputFormat::VHDL:
			case options::OutputFormat::CPP:
				o << '\t';
				break;
			default:
				break;
			}
			
			if (!isGraph() && options::outputFormat != options::OutputFormat::GATE_COSTS)
				functionNames.printName(o, i);
			
			switch (options::outputFormat)
			{
			case options::OutputFormat::HUMAN_LONG:
			case options::OutputFormat::HUMAN:
			case options::OutputFormat::HUMAN_SHORT:
				o << " ---\n";
				if (options::outputFormat != options::OutputFormat::HUMAN_SHORT)
					o << '\n';
				break;
			case options::OutputFormat::MATHEMATICAL:
				o << '(';
				::inputNames.printNames(o);
				o << ')';
				[[fallthrough]];
			case options::OutputFormat::VERILOG:
			case options::OutputFormat::CPP:
				o << " = ";
				break;
			case options::OutputFormat::VHDL:
				o << " <= ";
				break;
			default:
				break;
			}
			
			idShift = printSolution(i, idShift);
			
			switch (options::outputFormat)
			{
			case options::OutputFormat::GRAPH:
			case options::OutputFormat::REDUCED_GRAPH:
				o << "\t}\n";
				break;
			case options::OutputFormat::VERILOG:
			case options::OutputFormat::VHDL:
			case options::OutputFormat::CPP:
				o << ";\n";
				break;
			case options::OutputFormat::MATHEMATICAL:
				o << '\n';
				break;
			default:
				break;
			}
		}
		
		switch (options::outputFormat)
		{
		case options::OutputFormat::VERILOG:
		case options::OutputFormat::VHDL:
			o << "\t\n";
			break;
		case options::OutputFormat::CPP:
			o << "\treturn o;\n";
			break;
		default:
			break;
		}
	}
}

void OutputComposer::printOptimizedImmediates(const std::size_t immediateProductCount, const std::size_t immediateSumCount) const
{
	if (immediateProductCount != 0 || immediateSumCount != 0)
	{
		switch (options::outputFormat)
		{
		case options::OutputFormat::VERILOG:
			o << "\t// Internal signals\n";
			break;
		case options::OutputFormat::VHDL:
			o << "\t-- Internal signals\n";
			break;
		case options::OutputFormat::CPP:
			o << "\t// Intermediary values\n";
			break;
		default:
			break;
		}
		if (immediateProductCount != 0)
		{
			switch (options::outputFormat)
			{
			case options::OutputFormat::VERILOG:
				o << "\twire [" << (immediateProductCount - 1) << ":0] prods;\n";
				break;
			case options::OutputFormat::VHDL:
				o << "\tsignal prods : std_logic_vector(" << (immediateProductCount - 1) << " downto 0);\n";
				break;
			case options::OutputFormat::CPP:
				o << "\tbool prods[" << immediateProductCount << "] = {};\n";
				break;
			default:
				break;
			}
		}
		if (immediateSumCount != 0)
		{
			switch (options::outputFormat)
			{
			case options::OutputFormat::VERILOG:
				o << "\twire [" << (immediateSumCount - 1) << ":0] sums;\n";
				break;
			case options::OutputFormat::VHDL:
				o << "\tsignal sums : std_logic_vector(" << (immediateSumCount - 1) << " downto 0);\n";
				break;
			case options::OutputFormat::CPP:
				o << "\tbool sums[" << immediateSumCount << "] = {};\n";
				break;
			default:
				break;
			}
		}
		o << "\t\n";
	}
}

void OutputComposer::printOptimizedMathArgs(const OptimizedSolutions::id_t id) const
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
		::inputNames.printName(o, bit);
	}
	o << ')';
}

void OutputComposer::printOptimizedNegatedInputs() const
{
	if (optimizedSolutions->getNegatedInputs() == 0 && !isHuman())
		return;
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
		o << "Negated inputs:";
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		o << "\tsubgraph negated_inputs\n";
		o << "\t{\n";
		o << "\t\tnode [shape=diamond];\n";
		break;
	default:
		break;
	}
	First first;
	for (Minterm i = 0; i != ::bits; ++i)
	{
		if ((optimizedSolutions->getNegatedInputs() & (1 << (::bits - i - 1))) != 0)
		{
			if (!first)
			{
				switch (options::outputFormat)
				{
				case options::OutputFormat::HUMAN_SHORT:
				case options::OutputFormat::HUMAN:
				case options::OutputFormat::HUMAN_LONG:
					o << ',';
					break;
				default:
					break;
				}
			}
			switch (options::outputFormat)
			{
			case options::OutputFormat::HUMAN_SHORT:
			case options::OutputFormat::HUMAN:
			case options::OutputFormat::HUMAN_LONG:
				o << ' ';
				break;
			case options::OutputFormat::GRAPH:
			case options::OutputFormat::REDUCED_GRAPH:
				o << "\t\tni" << i << " [label=\"";
				printNot();
				break;
			default:
				break;
			}
			::inputNames.printName(o, i);
			switch (options::outputFormat)
			{
			case options::OutputFormat::GRAPH:
			case options::OutputFormat::REDUCED_GRAPH:
				o << "\"];\n";
				o << "\t\ti" << i << " -> ni" << i << ";\n";
				break;
			default:
				break;
			}
		}
	}
	if (first)
	{
		switch (options::outputFormat)
		{
		case options::OutputFormat::HUMAN_SHORT:
		case options::OutputFormat::HUMAN:
		case options::OutputFormat::HUMAN_LONG:
			o << " <none>";
			break;
		default:
			break;
		}
	}
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
		o << '\n';
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		o << "\t}\n";
		break;
	default:
		break;
	}
}

void OutputComposer::printOptimizedProductBody(const OptimizedSolutions::id_t productId, const bool parentheses) const
{
	const auto &[primeImplicant, ids] = optimizedSolutions->getProduct(productId);
	const bool parenthesesNeeded = parentheses && primeImplicant.getBitCount() + ids.size() >= 2;
	if (parenthesesNeeded)
		o << '(';
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		printImplicant(primeImplicant, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			printAnd(true);
		else
			needsAnd = true;
		printNormalizedId(id);
	}
	if (parenthesesNeeded)
		o << ')';
}

void OutputComposer::printOptimizedGraphProductImplicant(const OptimizedSolutions::id_t productId) const
{
	const bool isVerboseGraph = options::verboseGraph;
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
	printImplicant(primeImplicant, false, true);
}

std::size_t OutputComposer::printOptimizedGraphProductLabel(const OptimizedSolutions::id_t productId, std::size_t functionNum) const
{
	const bool isFullGraph = options::outputFormat == options::OutputFormat::GRAPH;
	const bool isVerboseGraph = options::verboseGraph;
	const bool hasParents = isFullGraph || !optimizedSolutions->getProduct(productId).subProducts.empty();
	if (hasParents)
	{
		printAnd(false);
		o << "\\n";
	}
	if (functionNum == SIZE_MAX)
	{
		printNormalizedId(productId, true);
	}
	else
	{
		while (true)
		{
			functionNames.printName(o, functionNum);
			const std::size_t additionalFunNum = optimizedSolutions->findProductEndNode(productId, functionNum + 1);
			if (additionalFunNum == SIZE_MAX)
				break;
			o << ", ";
			functionNum = additionalFunNum;
		}
	}
	if (!isFullGraph || isVerboseGraph)
		printOptimizedGraphProductImplicant(productId);
	return functionNum;
}

void OutputComposer::printOptimizedGraphProductParents(const OptimizedSolutions::id_t productId) const
{
	const bool isFullGraph = options::outputFormat == options::OutputFormat::GRAPH;
	const auto &[primeImplicant, ids] = optimizedSolutions->getProduct(productId);
	bool needsComma = isFullGraph && primeImplicant != Implicant::all();
	if (needsComma || ids.empty())
		printImplicant(primeImplicant, false);
	for (const auto &id : ids)
	{
		if (needsComma)
			o << ", ";
		else
			needsComma = true;
		printNormalizedId(id);
	}
}

void OutputComposer::printOptimizedProduct(const OptimizedSolutions::id_t productId) const
{
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
	case options::OutputFormat::VHDL:
	case options::OutputFormat::CPP:
		o << '\t';
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		o << "\t\t";
		break;
	case options::OutputFormat::VERILOG:
		o << "\tassign ";
		break;
	case options::OutputFormat::MATHEMATICAL:
		break;
	default:
		break;
	}
	
	printNormalizedId(productId);
	
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
	case options::OutputFormat::VERILOG:
	case options::OutputFormat::CPP:
	case options::OutputFormat::MATHEMATICAL:
		o << " = ";
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		o << " [label=\"";
		break;
	case options::OutputFormat::VHDL:
		o << " <= ";
		break;
	default:
		break;
	}
	
	
	if (!isGraph())
	{
		printOptimizedProductBody(productId, false);
	}
	else
	{
		const bool isFullGraph = options::outputFormat == options::OutputFormat::GRAPH;
		const bool hasParents = isFullGraph || !optimizedSolutions->getProduct(productId).subProducts.empty();
		std::size_t functionNum = isFullGraph ? SIZE_MAX : optimizedSolutions->findProductEndNode(productId);
		functionNum = printOptimizedGraphProductLabel(productId, functionNum);
		o << '"';
		if (functionNum != SIZE_MAX)
			o << ", style=filled";
		o << "]";
		if (hasParents)
		{
			o << ";\n";
			o << "\t\t";
			printOptimizedGraphProductParents(productId);
			o << " -> ";
			printNormalizedId(productId);
		}
	}
	
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
	case options::OutputFormat::MATHEMATICAL:
		o << '\n';
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
	case options::OutputFormat::VERILOG:
	case options::OutputFormat::VHDL:
	case options::OutputFormat::CPP:
		o << ";\n";
		break;
	default:
		break;
	}
}

void OutputComposer::printOptimizedProducts() const
{
	if (isHuman())
		o << "Products:\n";
	First first;
	for (std::size_t i = 0; i != optimizedSolutions->getProductCount(); ++i)
	{
		if (!isOptimizedProductWorthPrinting(optimizedSolutions->makeProductId(i)))
			continue;
		if (first)
		{
			switch (options::outputFormat)
			{
			case options::OutputFormat::GRAPH:
			case options::OutputFormat::REDUCED_GRAPH:
				o << "\tsubgraph products\n";
				o << "\t{\n";
				o << "\t\tnode [shape=ellipse];\n";
				break;
			case options::OutputFormat::VERILOG:
				o << "\t// Products\n";
				break;
			case options::OutputFormat::VHDL:
				o << "\t\n\t-- Products\n";
				break;
			case options::OutputFormat::CPP:
				o << "\t// Products\n";
				break;
			default:
				break;
			}
		}
		printOptimizedProduct(optimizedSolutions->makeProductId(i));
	}
	if (!first)
	{
		switch (options::outputFormat)
		{
		case options::OutputFormat::GRAPH:
		case options::OutputFormat::REDUCED_GRAPH:
			o << "\t}\n";
			break;
		case options::OutputFormat::VERILOG:
		case options::OutputFormat::CPP:
			o << "\t\n";
			break;
		default:
			break;
		}
	}
}

void OutputComposer::printOptimizedSumBody(const OptimizedSolutions::id_t sumId) const
{
	First first;
	const OptimizedSolutions::sum_t &sum = optimizedSolutions->getSum(sumId);
	for (const auto &partId : sum)
	{
		if (!first)
			printOr(true);
		if (!optimizedSolutions->isProduct(partId) || isOptimizedProductWorthPrinting(partId))
			printNormalizedId(partId);
		else
			printOptimizedProductBody(partId, options::outputFormat == options::OutputFormat::MATHEMATICAL && sum.size() != 1);
	}
}

std::size_t OutputComposer::printOptimizedGraphSumLabel(const OptimizedSolutions::id_t sumId, std::size_t functionNum) const
{
	const OptimizedSolutions::sum_t &sum = optimizedSolutions->getSum(sumId);
	const bool isFullGraph = options::outputFormat == options::OutputFormat::GRAPH;
	const bool isVerboseGraph = options::verboseGraph;
	const bool hasParents = isFullGraph || std::any_of(sum.cbegin(), sum.cend(), [this](const OptimizedSolutions::id_t id){ return !optimizedSolutions->isProduct(id) || isOptimizedProductWorthPrintingInGeneral(id); });
	if (hasParents)
	{
		printOr(false);
		o << "\\n";
	}
	if (functionNum == SIZE_MAX)
	{
		printNormalizedId(sumId, true);
	}
	else
	{
		while (true)
		{
			functionNames.printName(o, functionNum);
			const std::size_t additionalFunNum = optimizedSolutions->findSumEndNode(sumId, functionNum + 1);
			if (additionalFunNum == SIZE_MAX)
				break;
			o << ", ";
			functionNum = additionalFunNum;
		}
	}
	if (!isFullGraph || isVerboseGraph)
		printOptimizedGraphSumProducts(sumId);
	return functionNum;
}

void OutputComposer::printOptimizedGraphSumProducts(const OptimizedSolutions::id_t sumId) const
{
	const bool isVerboseGraph = options::verboseGraph;
	OptimizedSolutions::sum_t sum;
	if (isVerboseGraph)
	{
		sum = optimizedSolutions->flattenSum(sumId);
	}
	else
	{
		sum = optimizedSolutions->getSum(sumId);
		sum.erase(std::remove_if(sum.begin(), sum.end(), [this](const OptimizedSolutions::id_t id){ return !optimizedSolutions->isProduct(id) || isOptimizedProductWorthPrintingInGeneral(id); }), sum.end());
	}
	First first;
	for (const auto &productId : sum)
	{
		if (first)
			o << " = ";
		else
			printOr(true);
		if (isVerboseGraph)
			printImplicant(optimizedSolutions->flattenProduct(productId), sum.size() != 1, true);
		else
			printImplicant(optimizedSolutions->getProduct(productId).implicant, false, true);
	}
}

void OutputComposer::printOptimizedGraphSumParents(const OptimizedSolutions::id_t sumId) const
{
	const bool isFullGraph = options::outputFormat == options::OutputFormat::GRAPH;
	First first;
	for (const auto &partId : optimizedSolutions->getSum(sumId))
	{
		if (!optimizedSolutions->isProduct(partId) || isOptimizedProductWorthPrintingInGeneral(partId))
		{
			if (!first)
				o << ", ";
			printNormalizedId(partId);
		}
		else if (isFullGraph)
		{
			if (!first)
				o << ", ";
			printImplicant(optimizedSolutions->getProduct(partId).implicant, false);
		}
	}
}

void OutputComposer::printOptimizedSum(const OptimizedSolutions::id_t sumId) const
{
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
	case options::OutputFormat::VHDL:
	case options::OutputFormat::CPP:
		o << '\t';
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		o << "\t\t";
		break;
	case options::OutputFormat::VERILOG:
		o << "\tassign ";
		break;
	case options::OutputFormat::MATHEMATICAL:
	default:
		break;
	}
	
	printNormalizedId(sumId);
	
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
	case options::OutputFormat::VERILOG:
	case options::OutputFormat::CPP:
	case options::OutputFormat::MATHEMATICAL:
		o << " = ";
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		o << " [label=\"";
		break;
	case options::OutputFormat::VHDL:
		o << " <= ";
		break;
	default:
		break;
	}
	
	if (!isGraph())
	{
		printOptimizedSumBody(sumId);
	}
	else
	{
		const OptimizedSolutions::sum_t &sum = optimizedSolutions->getSum(sumId);
		const bool isFullGraph = options::outputFormat == options::OutputFormat::GRAPH;
		const bool hasParents = isFullGraph || std::any_of(sum.cbegin(), sum.cend(), [this](const OptimizedSolutions::id_t id){ return !optimizedSolutions->isProduct(id) || isOptimizedProductWorthPrintingInGeneral(id); });
		std::size_t functionNum = isFullGraph ? SIZE_MAX : optimizedSolutions->findSumEndNode(sumId);
		functionNum = printOptimizedGraphSumLabel(sumId, functionNum);
		o << '"';
		if (functionNum != SIZE_MAX)
			o << ", style=filled";
		o << ']';
		if (hasParents)
		{
			o << ";\n";
			o << "\t\t";
			printOptimizedGraphSumParents(sumId);
			o << " -> ";
			printNormalizedId(sumId);
		}
	}
	
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
	case options::OutputFormat::MATHEMATICAL:
		o << '\n';
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
	case options::OutputFormat::VERILOG:
	case options::OutputFormat::VHDL:
	case options::OutputFormat::CPP:
		o << ";\n";
		break;
	default:
		break;
	}
}

void OutputComposer::printOptimizedSums() const
{
	if (isHuman())
		o << "Sums:\n";
	
	First first;
	for (std::size_t i = 0; i != optimizedSolutions->getSumCount(); ++i)
	{
		if (!isOptimizedSumWorthPrinting(optimizedSolutions->makeSumId(i)))
			continue;
		if (first)
		{
			switch (options::outputFormat)
			{
			case options::OutputFormat::HUMAN_SHORT:
			case options::OutputFormat::HUMAN:
			case options::OutputFormat::HUMAN_LONG:
				break;
			case options::OutputFormat::GRAPH:
			case options::OutputFormat::REDUCED_GRAPH:
				o << "\tsubgraph sums\n";
				o << "\t{\n";
				o << "\t\tnode [shape=rectangle];\n";
				break;
			case options::OutputFormat::VERILOG:
			case options::OutputFormat::CPP:
				o << "\t// Sums\n";
				break;
			case options::OutputFormat::VHDL:
				o << "\t\n\t-- Sums\n";
				break;
			case options::OutputFormat::MATHEMATICAL:
			default:
				break;
			}
		}
		printOptimizedSum(optimizedSolutions->makeSumId(i));
	}
	
	if (!first)
	{
		switch (options::outputFormat)
		{
		case options::OutputFormat::HUMAN_SHORT:
		case options::OutputFormat::HUMAN:
		case options::OutputFormat::HUMAN_LONG:
			break;
		case options::OutputFormat::GRAPH:
		case options::OutputFormat::REDUCED_GRAPH:
			o << "\t}\n";
			break;
		case options::OutputFormat::VERILOG:
		case options::OutputFormat::CPP:
			o << "\t\n";
			break;
		case options::OutputFormat::VHDL:
		case options::OutputFormat::MATHEMATICAL:
		default:
			break;
		}
	}
}

void OutputComposer::printOptimizedGraphFinalSumLabel(const std::size_t i) const
{
	const bool isVerboseGraph = options::verboseGraph;
	const OptimizedSolutions::id_t sumId = optimizedSolutions->getFinalSums()[i];
	if (!isOptimizedSumWorthPrintingInGeneral(sumId) && optimizedSolutions->getSum(sumId).size() > 1)
	{
		printOr(false);
		o << "\\n";
	}
	functionNames.printName(o, i);
	if (isVerboseGraph)
		printOptimizedGraphSumProducts(sumId);
}

void OutputComposer::printOptimizedFinalSums() const
{
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		break;
	case options::OutputFormat::VERILOG:
		break;
	case options::OutputFormat::VHDL:
		break;
	case options::OutputFormat::CPP:
		o << "\t// Results\n";
		break;
	case options::OutputFormat::MATHEMATICAL:
	default:
		break;
	}
	
	if (optimizedSolutions->getFinalSums().empty() && !isHuman())
	{
		if (options::outputFormat == options::OutputFormat::CPP)
			o << "\treturn {};\n";
		return;
	}
	
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		o << "\tsubgraph final_sums\n";
		o << "\t{\n";
		o << "\t\tnode [shape=rectangle, style=filled];\n";
		break;
	case options::OutputFormat::VERILOG:
		o << "\t// Results\n";
		break;
	case options::OutputFormat::VHDL:
		o << "\t\n";
		o << "\t-- Results\n";
		break;
	case options::OutputFormat::CPP:
		o << "\toutput_t o = {};\n";
		break;
	case options::OutputFormat::MATHEMATICAL:
	default:
		break;
	}
	
	for (std::size_t i = 0; i != optimizedSolutions->getFinalSums().size(); ++i)
	{
		switch (options::outputFormat)
		{
		case options::OutputFormat::HUMAN_SHORT:
		case options::OutputFormat::HUMAN:
		case options::OutputFormat::HUMAN_LONG:
			o << "\t\"";
			break;
		case options::OutputFormat::GRAPH:
		case options::OutputFormat::REDUCED_GRAPH:
			o << "\t\tf" << i << " [label=\"";
			printOptimizedGraphFinalSumLabel(i);
			o << "\"];\n";
			o << "\t\t";
			break;
		case options::OutputFormat::VERILOG:
			o << "\tassign ";
			break;
		case options::OutputFormat::VHDL:
		case options::OutputFormat::CPP:
			o << '\t';
			break;
		case options::OutputFormat::MATHEMATICAL:
		default:
			break;
		}
		
		if (!isGraph())
			functionNames.printName(o, i);
		
		switch (options::outputFormat)
		{
		case options::OutputFormat::HUMAN_SHORT:
		case options::OutputFormat::HUMAN:
		case options::OutputFormat::HUMAN_LONG:
			o << "\" = ";
			break;
		case options::OutputFormat::GRAPH:
		case options::OutputFormat::REDUCED_GRAPH:
			break;
		case options::OutputFormat::VERILOG:
		case options::OutputFormat::CPP:
			o << " = ";
			break;
		case options::OutputFormat::VHDL:
			o << " <= ";
			break;
		case options::OutputFormat::MATHEMATICAL:
			o << '(';
			::inputNames.printNames(o);
			o << ") = ";
			break;
		default:
			break;
		}
		
		const OptimizedSolutions::id_t sumId = optimizedSolutions->getFinalSums()[i];
		if (isOptimizedSumWorthPrinting(sumId))
			printNormalizedId(sumId);
		else if (!isGraph())
			printOptimizedSumBody(sumId);
		else
			printOptimizedGraphSumParents(sumId);
		
		switch (options::outputFormat)
		{
		case options::OutputFormat::HUMAN_SHORT:
		case options::OutputFormat::HUMAN:
		case options::OutputFormat::HUMAN_LONG:
		case options::OutputFormat::MATHEMATICAL:
			o << '\n';
			break;
		case options::OutputFormat::GRAPH:
		case options::OutputFormat::REDUCED_GRAPH:
			o << " -> f" << i << ";\n";
			break;
		case options::OutputFormat::VERILOG:
		case options::OutputFormat::VHDL:
		case options::OutputFormat::CPP:
			o << ";\n";
			break;
		default:
			break;
		}
	}
	
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		o << "\t}\n";
		break;
	case options::OutputFormat::VERILOG:
		o << "\t\n";
		break;
	case options::OutputFormat::VHDL:
		break;
	case options::OutputFormat::CPP:
		o << "\treturn o;\n";
		break;
	case options::OutputFormat::MATHEMATICAL:
	default:
		break;
	}
}

void OutputComposer::printOptimizedSolution()
{
	const auto [immediateProductCount, immediateSumCount] = generateOptimizedNormalizedIds();
	if (isHuman() || options::outputFormat == options::OutputFormat::GRAPH)
		printOptimizedNegatedInputs();
	if (!isHumanReadable())
		printOptimizedImmediates(immediateProductCount, immediateSumCount);
	
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		break;
	case options::OutputFormat::VERILOG:
		break;
	case options::OutputFormat::VHDL:
		o << "begin\n";
		break;
	case options::OutputFormat::CPP:
		break;
	case options::OutputFormat::MATHEMATICAL:
		if (immediateProductCount != 0 || immediateSumCount != 0)
			o << "Let:\n";
		break;
	default:
		break;
	}
	
	printOptimizedProducts();
	printOptimizedSums();
	
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		break;
	case options::OutputFormat::VERILOG:
		break;
	case options::OutputFormat::VHDL:
		break;
	case options::OutputFormat::CPP:
		break;
	case options::OutputFormat::MATHEMATICAL:
		if (immediateProductCount != 0 || immediateSumCount != 0)
			o << "\nThen:\n";
		break;
	default:
		break;
	}
	
	if (options::outputFormat != options::OutputFormat::REDUCED_GRAPH)
		printOptimizedFinalSums();
	if (options::outputFormat == options::OutputFormat::HUMAN || options::outputFormat == options::OutputFormat::HUMAN_LONG)
	{
		o << '\n';
		printGateCost(*optimizedSolutions, false);
	}
	
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_SHORT:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_LONG:
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		break;
	case options::OutputFormat::VERILOG:
		break;
	case options::OutputFormat::VHDL:
		o << "\t\n";
		break;
	case options::OutputFormat::CPP:
		break;
	case options::OutputFormat::MATHEMATICAL:
	default:
		break;
	}
}

void OutputComposer::printGraphConstants() const
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
		printBool(true);
		o << "\"];\n";
	}
	if (usesFalse)
	{
		o << "\t\tfalse [label=\"";
		printBool(false);
		o << "\"];\n";
	}
	o << "\t}\n";
}

void OutputComposer::printGraphInputs() const
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
		::inputNames.printName(o, i);
		o << "\"];\n";
	}
	o << "\t}\n";
}

void OutputComposer::printGraphRoots() const
{
	printGraphInputs();
	printGraphConstants();
}

void OutputComposer::printHuman()
{
	const bool solutionsVisible = options::skipOptimization || options::outputFormat != options::OutputFormat::HUMAN_SHORT;
	if (solutionsVisible)
	{
		printSolutions();
		if (options::outputFormat != options::OutputFormat::HUMAN_SHORT)
		{
			if (solutions.size() != 1 && options::skipOptimization)
			{
				if (!solutions.empty())
					o << "\n\n=== summary ===\n" << '\n';
				printGateCost(solutions, false);
			}
		}
	}
	if (!options::skipOptimization)
	{
		if (options::outputFormat != options::OutputFormat::HUMAN_SHORT)
		{
			if (!solutions.empty())
				o << "\n\n";
			o << "=== optimized solution ===\n" << '\n';
		}
		printOptimizedSolution();
		o << std::flush;
	}
}

void OutputComposer::printGraph()
{
	o << "digraph " << getName() << '\n';
	o << "{\n";
	if (options::outputFormat == options::OutputFormat::GRAPH)
		printGraphRoots();
	if (options::skipOptimization)
		printSolutions();
	else
		printOptimizedSolution();
	o << "}\n";
}

void OutputComposer::printVerilog()
{
	o << "module " << getName() << " (\n";
	if (!::inputNames.empty())
	{
		o << "\tinput wire";
		::inputNames.printNames(o);
		o << '\n';
	}
	if (!karnaughs.empty())
	{
		o << "\toutput wire";
		functionNames.printNames(o);
		o << '\n';
	}
	o << ");\n";
	o << "\t\n";
	if (options::skipOptimization)
		printSolutions();
	else
		printOptimizedSolution();
	o << "endmodule" << std::endl;
}

void OutputComposer::printVhdl()
{
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
			::inputNames.printNames(o);
			o << " : in ";
			::inputNames.printType(o);
			if (!karnaughs.empty())
				o << ';';
			o << '\n';
		}
		if (!karnaughs.empty())
		{
			o << "\t\t";
			functionNames.printNames(o);
			o << " : out ";
			functionNames.printType(o);
			o << '\n';
		}
		o << "\t);\n";
	}
	o << "end " << getName() << ";\n";
	o << '\n';
	o << "architecture behavioural of " << getName() << " is\n";
	if (options::skipOptimization)
		printSolutions();
	else
		printOptimizedSolution();
	o << "end behavioural;\n";
}

void OutputComposer::printCpp()
{
	if (!::inputNames.areNamesUsedInCode() || !functionNames.areNamesUsedInCode())
		o << "#include <array>\n"
				"\n";
	o << "class " << getName() << "\n"
			"{\n"
			"public:\n";
	o << "\tusing input_t = ";
	::inputNames.printType(o);
	o << ";\n";
	o << "\tusing output_t = ";
	functionNames.printType(o);
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
			::inputNames.printPlainName(o, i);
		}
	}
	o << ") const { return (*this)({";
	{
		First first;
		for (std::size_t i = 0; i != ::inputNames.size(); ++i)
		{
			if (!first)
				o << ", ";
			::inputNames.printPlainName(o, i);
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
			::inputNames.printPlainName(o, i);
		}
	}
	o << ") { return calc({";
	{
		First first;
		for (std::size_t i = 0; i != ::inputNames.size(); ++i)
		{
			if (!first)
				o << ", ";
			::inputNames.printPlainName(o, i);
		}
	}
	o << "}); }\n";
	o << "\t[[nodiscard]] static constexpr output_t calc(const input_t &i);\n";
	o << "};" << std::endl;
	o << '\n';
	o << "constexpr " << getName() << "::output_t " << getName() << "::calc(const input_t &" << (areInputsUsed() ? "i" : "") << ")\n";
	o << "{\n";
	if (options::skipOptimization)
		printSolutions();
	else
		printOptimizedSolution();
	o << "}\n";
}

void OutputComposer::printMath()
{
	if (options::skipOptimization)
		printSolutions();
	else
		printOptimizedSolution();
}

void OutputComposer::printGateCost()
{
	printSolutions();
	o << "=== summary ===\n";
	printGateCost(solutions, true);
	if (!options::skipOptimization)
	{
		o << "=== optimized solution ===\n";
		printGateCost(*optimizedSolutions, true);
	}
}

OutputComposer::OutputComposer(const Names &functionNames, std::vector<Karnaugh> &karnaughs, const Solutions &solutions, const OptimizedSolutions *const optimizedSolutions, std::ostream &o) :
	functionNames(functionNames),
	karnaughs(karnaughs),
	solutions(solutions),
	optimizedSolutions(optimizedSolutions),
	o(o)
{
}

void OutputComposer::compose()
{
	if (options::outputBanner)
		printBanner();
	
	switch (options::outputFormat)
	{
	case options::OutputFormat::HUMAN_LONG:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::HUMAN_SHORT:
		printHuman();
		break;
	case options::OutputFormat::GRAPH:
	case options::OutputFormat::REDUCED_GRAPH:
		printGraph();
		break;
	case options::OutputFormat::VERILOG:
		printVerilog();
		break;
	case options::OutputFormat::VHDL:
		printVhdl();
		break;
	case options::OutputFormat::CPP:
		printCpp();
		break;
	case options::OutputFormat::MATHEMATICAL:
		printMath();
		break;
	case options::OutputFormat::GATE_COSTS:
		printGateCost();
		break;
	}
}
