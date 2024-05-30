#include "./OutputComposer.hh"

#include <filesystem>
#include <iomanip>
#include <type_traits>

#include "info.hh"


using OF = options::OutputFormat;
using OO = options::OutputOperators;


template<OF ...FORMATS>
bool constexpr OutputComposer::discriminate(const OF format)
{
	if constexpr (sizeof...(FORMATS) == 1)
	{
		return format == (FORMATS, ...);
	}
	else
	{
		constexpr std::underlying_type_t<OF> mask = (static_cast<std::underlying_type_t<OF>>(FORMATS) | ...);
		return (static_cast<std::underlying_type_t<OF>>(format) & mask) != 0;
	}
}

template<OF ...FORMATS>
bool OutputComposer::discriminate() const
{
	return discriminate<FORMATS...>(outputFormat);
}

bool OutputComposer::isHuman() const
{
	return discriminate<OF::HUMAN_SHORT, OF::HUMAN, OF::HUMAN_LONG>();
}

bool OutputComposer::isGraph() const
{
	return discriminate<OF::GRAPH, OF::REDUCED_GRAPH>();
}

bool OutputComposer::isHumanReadable() const
{
	return discriminate<OF::HUMAN_SHORT, OF::HUMAN, OF::HUMAN_LONG, OF::GRAPH, OF::REDUCED_GRAPH, OF::MATHEMATICAL, OF::GATE_COSTS>();
}

bool OutputComposer::isProgramming() const
{
	return discriminate<OF::VERILOG, OF::VHDL, OF::CPP>();
}

template<typename ENUM, ENUM ...CHOICES, typename ...VALUES>
void OutputComposer::printChoiceSpecific(const ENUM choice, const VALUES &...values) const
{
	const bool success = (... || (
			choice <= CHOICES && [values]{ if constexpr (is_nullable<VALUES>) return values != nullptr; else return true; }()
				? ([this, values]{
						if constexpr (std::is_invocable_v<VALUES>)
							std::invoke(values);
						else if constexpr (std::is_member_function_pointer_v<VALUES>)
							std::invoke(reinterpret_cast<void (OutputComposer::*)() const>(values), *this);  // Ugly but I'm not going to make a second set of those funciton for non-const `this`.
						else if constexpr (!std::is_same_v<VALUES, decltype(BLANK)>)
							o << values;
					}(), true)
				: false
		));
	assert(success);
#ifdef NDEBUG
	(void) success;
#endif
}

template<typename GRAPH, typename REDUCED_GRAPH, typename MATHEMATICAL, typename GATE_COST, typename HUMAN_LONG, typename HUMAN, typename HUMAN_SHORT, typename CPP, typename VERILOG, typename VHDL>
void OutputComposer::printFormatSpecific(const OF format, GRAPH graph, REDUCED_GRAPH reducedGraph, MATHEMATICAL mathematical, GATE_COST gateCost, HUMAN_LONG humanLong, HUMAN human, HUMAN_SHORT humanShort, CPP cpp, VERILOG verilog, VHDL vhdl) const
{
	return printChoiceSpecific<OF, OF::GRAPH, OF::REDUCED_GRAPH, OF::MATHEMATICAL, OF::GATE_COSTS, OF::HUMAN_LONG, OF::HUMAN, OF::HUMAN_SHORT, OF::CPP, OF::VERILOG, OF::VHDL>(format, graph, reducedGraph, mathematical, gateCost, humanLong, human, humanShort, cpp, verilog, vhdl);
}

template<typename GRAPH, typename MATHEMATICAL, typename GATE_COST, typename HUMAN, typename CPP, typename VERILOG, typename VHDL>
void OutputComposer::printFormatSpecific(const OF format, GRAPH graph, MATHEMATICAL mathematical, GATE_COST gateCost, HUMAN human, CPP cpp, VERILOG verilog, VHDL vhdl) const
{
	return printFormatSpecific(format, NEXT, graph, mathematical, gateCost, NEXT, NEXT, human, cpp, verilog, vhdl);
}

template<typename GRAPH, typename MATHEMATICAL, typename HUMAN, typename CPP, typename VERILOG, typename VHDL>
void OutputComposer::printFormatSpecific(const OF format, GRAPH graph, MATHEMATICAL mathematical, HUMAN human, CPP cpp, VERILOG verilog, VHDL vhdl) const
{
	return printFormatSpecific(format, graph, mathematical, NEXT, human, cpp, verilog, vhdl);
}

template<typename GRAPH, typename MATHEMATICAL, typename HUMAN, typename PROGRAM>
void OutputComposer::printFormatSpecific(const OF format, GRAPH graph, MATHEMATICAL mathematical, HUMAN human, PROGRAM program) const
{
	return printFormatSpecific(format, graph, mathematical, human, NEXT, NEXT, program);
}

template<typename CPP, typename VERILOG, typename VHDL>
void OutputComposer::printFormatSpecific(const OF format, CPP cpp, VERILOG verilog, VHDL vhdl) const
{
	return printFormatSpecific(format, NEXT, NEXT, NEXT, cpp, verilog, vhdl);
}

template<typename GRAPH, typename HUMAN>
void OutputComposer::printFormatSpecific(const OF format, GRAPH graph, HUMAN human) const
{
	return printFormatSpecific(format, graph, NEXT, human, NEXT, NEXT, NEXT);
}

template<typename GRAPH, typename REDUCED_GRAPH, typename MATHEMATICAL, typename GATE_COST, typename HUMAN_LONG, typename HUMAN, typename HUMAN_SHORT, typename CPP, typename VERILOG, typename VHDL>
std::enable_if_t<!std::is_same_v<GRAPH, options::MappedOutputFormats>, void>
OutputComposer::printFormatSpecific(GRAPH graph, REDUCED_GRAPH reducedGraph, MATHEMATICAL mathematical, GATE_COST gateCost, HUMAN_LONG humanLong, HUMAN human, HUMAN_SHORT humanShort, CPP cpp, VERILOG verilog, VHDL vhdl) const
{
	return printFormatSpecific(outputFormat, graph, reducedGraph, mathematical, gateCost, humanLong, human, humanShort, cpp, verilog, vhdl);
}

template<typename GRAPH, typename MATHEMATICAL, typename GATE_COSTS, typename HUMAN, typename CPP, typename VERILOG, typename VHDL>
std::enable_if_t<!std::is_same_v<GRAPH, options::MappedOutputFormats>, void>
OutputComposer::printFormatSpecific(GRAPH graph, MATHEMATICAL mathematical, GATE_COSTS gateCost, HUMAN human, CPP cpp, VERILOG verilog, VHDL vhdl) const
{
	return printFormatSpecific(outputFormat, graph, mathematical, gateCost, human, cpp, verilog, vhdl);
}

template<typename GRAPH, typename MATHEMATICAL, typename HUMAN, typename CPP, typename VERILOG, typename VHDL>
std::enable_if_t<!std::is_same_v<GRAPH, options::MappedOutputFormats>, void>
OutputComposer::printFormatSpecific(GRAPH graph, MATHEMATICAL mathematical, HUMAN human, CPP cpp, VERILOG verilog, VHDL vhdl) const
{
	return printFormatSpecific(outputFormat, graph, mathematical, human, cpp, verilog, vhdl);
}

template<typename GRAPH, typename MATHEMATICAL, typename HUMAN, typename PROGRAM>
std::enable_if_t<!std::is_same_v<GRAPH, options::MappedOutputFormats>, void>
OutputComposer::printFormatSpecific(GRAPH graph, MATHEMATICAL mathematical, HUMAN human, PROGRAM program) const
{
	return printFormatSpecific(outputFormat, graph, mathematical, human, program);
}

template<typename CPP, typename VERILOG, typename VHDL>
std::enable_if_t<!std::is_same_v<CPP, options::MappedOutputFormats>, void>
OutputComposer::printFormatSpecific(CPP cpp, VERILOG verilog, VHDL vhdl) const
{
	return printFormatSpecific(outputFormat, cpp, verilog, vhdl);
}

template<typename GRAPH, typename HUMAN>
std::enable_if_t<!std::is_same_v<GRAPH, options::MappedOutputFormats>, void>
OutputComposer::printFormatSpecific(GRAPH graph, HUMAN human) const
{
	return printFormatSpecific(outputFormat, graph, human);
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
	if (optimizedSolutions == nullptr)
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
	switch (outputFormat)
	{
	case OF::REDUCED_GRAPH:
		return isWorthPrintingInGeneral || optimizedSolutions->findProductEndNode(productId) != SIZE_MAX;
	case OF::MATHEMATICAL:
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
	switch (outputFormat)
	{
	case OF::REDUCED_GRAPH:
		return optimizedSolutions->getSum(sumId).size() != 1;
	case OF::MATHEMATICAL:
		return isOptimizedSumWorthPrintingInGeneral(sumId) && (optimizedSolutions->getIdUseCount(sumId) + std::count(optimizedSolutions->getFinalSums().cbegin(), optimizedSolutions->getFinalSums().cend(), sumId) >= 2);
	default:
		return isOptimizedSumWorthPrintingInGeneral(sumId);
	}
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
	const OF format = useHumanAnyway ? OF::HUMAN : outputFormat;
	if (optimizedSolutions->isProduct(id))
		printFormatSpecific(format, 's', 'p', BLANK, "prods");
	else
		printFormatSpecific(format, NEXT, 's', BLANK, "sums");
	printFormatSpecific(format, NEXT, BLANK, NEXT, NEXT, '[', '(');
	if (format != OF::MATHEMATICAL)
	{
		o << normalizedOptimizedIds[id];
	}
	else
	{
		o << (normalizedOptimizedIds[id] + 1);
		printOptimizedMathArgs(id);
	}
	printFormatSpecific(format, NEXT, BLANK, NEXT, NEXT, ']', ')');
}

void OutputComposer::printCommentStart() const
{
	printFormatSpecific("// ", NEXT, NEXT, BLANK, NEXT, "// ", "-- ");
}

void OutputComposer::printAssignmentStart() const
{
	if (discriminate<OF::VERILOG>())
		o << "assign ";
}

void OutputComposer::printAssignmentOp() const
{
	printFormatSpecific(BLANK, NEXT, NEXT, NEXT, " = ", " <= ");
}

void OutputComposer::printShortBool(const Trilean value) const
{
	const auto print = [this](const char formal, const char ascii, const char programming, const char names){ return printChoiceSpecific<OO, OO::FORMAL, OO::ASCII, OO::PROGRAMMING, OO::NAMES>(outputOperators, formal, ascii, programming, names); };
	switch (value.get())
	{
	case Trilean::Value::FALSE:
		print('0', '0', 'F', 'F');
		break;
	case Trilean::Value::TRUE:
		print('1', '1', 'T', 'T');
		break;
	case Trilean::Value::UNDEFINED:
		print('X', 'X', '-', '?');
		break;
	}
}

constexpr OF OutputComposer::mapOutputOperators(const OO outputOperators)
{
	switch (outputOperators)
	{
	case OO::FORMAL:
		return OF::MATHEMATICAL;
	case OO::ASCII:
		return OF::GRAPH;
	case OO::PROGRAMMING:
		return OF::CPP;
	case OO::NAMES:
		return OF::HUMAN;
	}
	assert(false);
	return OF::GATE_COSTS;
}

OF OutputComposer::getOperatorsStyle() const
{
	return isProgramming()
			? outputFormat
			: mapOutputOperators(outputOperators);
}

void OutputComposer::printBool(const bool value, const bool strictlyForCode) const
{
	const OF format = strictlyForCode ? OF::CPP : getOperatorsStyle();
	if (value)
		printFormatSpecific(format, 'T', u8"\u22A4", "TRUE", "true", '1', "'1'");
	else
		printFormatSpecific(format, 'F', u8"\u22A5", "FALSE", "false", '0', "'0'");
}

void OutputComposer::printNot() const
{
	printFormatSpecific(getOperatorsStyle(), '~', u8"\u00AC", "NOT ", NEXT, '!', "not ");
}

void OutputComposer::printAnd(const bool spaces) const
{
	if (spaces)
		o << ' ';
	printFormatSpecific(getOperatorsStyle(), "/\\", u8"\u2227", "AND", "&&", '&', "and");
	if (spaces)
		o << ' ';
}

void OutputComposer::printOr(const bool spaces) const
{
	if (spaces)
		o << ' ';
	printFormatSpecific(getOperatorsStyle(), "\\/", u8"\u2228", "OR", "||", '|', "or");
	if (spaces)
		o << ' ';
}

void OutputComposer::printBanner() const
{
	printCommentStart();
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
	o << '\n';
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
		o << "subgraph negated_inputs\n";
		o << "{\n";
		{
			const auto indentGuard = o.startIndent();
			o << "node [shape=diamond];\n";
			for (Minterm i = 0; i != ::bits; ++i)
			{
				for (const std::size_t j : negatedInputs[i])
				{
					o << "f" << functionNum << "_ni" << i << '_' << j << " [label=\"";
					{
						const auto sanitizeGuard = o.startSanitize();
						printNot();
						::inputNames.printName(o, i);
					}
					o << "\"];\n";
					o << "i" << i << " -> f" << functionNum << "_ni" << i << '_' << j << ";\n";
				}
			}
		}
		o << "}\n";
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
		const bool isFullGraph = outputFormat == OF::GRAPH;
		o << "subgraph products\n";
		o << "{\n";
		{
			const auto indentGuard = o.startIndent();
			o << "node [shape=ellipse];\n";
			for (std::size_t i = 0; i != solution.size(); ++i)
			{
				if (solution[i].getBitCount() < 2)
					continue;
				o << "f" << functionNum << "_s" << i << " [label=\"";
				{
					const auto sanitizeGuard = o.startSanitize();
					if (isFullGraph)
					{
						printAnd(false);
						const auto sanitizeGuard = o.startSanitize(false);
						o << "\\n";
					}
					o << "[" << idShift++ << "]";
					if (!isFullGraph || isGraphVerbose)
					{
						o << " = ";
						printImplicant(solution[i], false, true);
					}
				}
				o << "\"];\n";
				if (isFullGraph)
				{
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
		}
		o << "}\n";
	}
	return idShift;
}

void OutputComposer::printGraphSum(const Solution &solution, const std::size_t functionNum) const
{
	const bool isFullGraph = discriminate<OF::GRAPH>();
	o << "subgraph sum\n";
	o << "{\n";
	{
		const auto indentGuard = o.startIndent();
		o << "node [shape=rectangle, style=filled];\n";
		o << "f" << functionNum << " [label=\"";
		const bool hasParents = isFullGraph || (solution.size() >= 2 && std::any_of(solution.cbegin(), solution.cend(), [](const Implicant &x){ return x.getBitCount() >= 2; }));
		{
			const auto sanitizeGuard = o.startSanitize();
			if (hasParents && solution.size() >= 2)
			{
				printOr(false);
				const auto sanitizeGuard = o.startSanitize(false);
				o << "\\n";
			}
			functionNames.printName(o, functionNum);
			if (!isFullGraph || isGraphVerbose)
			{
				First first;
				for (const Implicant &product : solution)
				{
					if (!isGraphVerbose && solution.size() >= 2 && product.getBitCount() >= 2)
						continue;
					if (first)
						o << " = ";
					else
						printOr(true);
					printImplicant(product, solution.size() != 1, true);
				}
			}
		}
		o << "\"];\n";
		if (hasParents)
		{
			o << "";
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
	}
	o << "}\n";
}

std::size_t OutputComposer::printSolution(const std::size_t i, std::size_t idShift) const
{
	const Solution solution = Solution(solutions[i]).sort();
	
	if (isGraph())
	{
		const bool isFullGraph = discriminate<OF::GRAPH>();
		if (isFullGraph)
			printGraphNegatedInputs(solution, i);
		if (isFullGraph || solution.size() >= 2)
			idShift = printGraphProducts(solution, i, idShift);
		printGraphSum(solution, i);
		return idShift;
	}
	else if (discriminate<OF::GATE_COSTS>())
	{
		printGateCost(solution, true);
		return 0;
	}
	
	if (isHuman())
	{
		const Karnaugh &karnaugh = karnaughs[i];
		if (discriminate<OF::HUMAN_LONG>())
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
		if (!discriminate<OF::HUMAN_SHORT>())
		{
			o << '\n';
			printGateCost(solution, false);
		}
	}
	
	return 0;
}

void OutputComposer::printSolutions() const
{
	if (discriminate<OF::VHDL>())
	{
		{
			const auto indentGuard = o.startIndent(-1);
			o << "begin\n";
		}
		o << '\n';
	}
	
	if (karnaughs.empty())
	{
		if (discriminate<OF::CPP>())
			o << "return {};\n";
		return;
	}
	
	if (discriminate<OF::CPP>())
		o << "output_t o = {};\n";
	
	First first;
	std::size_t idShift = 0;
	for (std::size_t i = 0; i != functionNames.size(); ++i)
	{
		if (discriminate<OF::HUMAN_LONG, OF::HUMAN>() && !first)
			o << "\n\n";
		
		printFormatSpecific("subgraph function_", NEXT, BLANK, "--- ", NEXT, NEXT, &OutputComposer::printAssignmentStart);
		if (isGraph())
			o << i;
		else if (!discriminate<OF::GATE_COSTS>())
			functionNames.printName(o, i);
		printFormatSpecific("\n{\n", [this]{ o << '('; ::inputNames.printNames(o); o << ')'; printAssignmentOp(); }, BLANK, " ---\n", NEXT, NEXT, &OutputComposer::printAssignmentOp);
		if (discriminate<OF::HUMAN_LONG, OF::HUMAN>())
			o << '\n';
		if (isGraph())
		{
			const auto indentGuard = o.startIndent();
			idShift = printSolution(i, idShift);
		}
		else
		{
			printSolution(i);
		}
		printFormatSpecific("}\n", '\n', NEXT, BLANK, NEXT, NEXT, ";\n");
	}
	
	if (isProgramming())
		printFormatSpecific("return o;\n", NEXT, '\n');
}

void OutputComposer::printOptimizedImmediates(const std::size_t immediateProductCount, const std::size_t immediateSumCount) const
{
	if (immediateProductCount + immediateSumCount == 0)
		return;
	
	printCommentStart();
	printFormatSpecific("Intermediary values\n", NEXT, "Internal signals\n");
	const auto printImmediates = [this](const std::string &name, const std::size_t count)
		{
			if (count == 0)
				return;
			printFormatSpecific("bool " + name + '[', "wire [", "signal " + name + " : std_logic_vector(");
			o << (discriminate<OF::CPP>() ? count : count - 1);
			printFormatSpecific("] = {};\n", ":0] " + name + ";\n", " downto 0);\n");
		};
	printImmediates("prods", immediateProductCount);
	printImmediates("sums", immediateSumCount);
	o << '\n';
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
	
	{
		const auto indentGuard = o.startIndent(0);
		if (isGraph())
		{
			o << "subgraph negated_inputs\n{\n";
			o.indent();
			o << "node [shape=diamond];\n";
		}
		else
		{
			o << "Negated inputs:";
		}
		First first;
		for (Minterm i = 0; i != ::bits; ++i)
		{
			if ((optimizedSolutions->getNegatedInputs() & (1 << (::bits - i - 1))) != 0)
			{
				if (isHuman() && !first)
					o << ',';
				{
					const auto sanitizeGuard = o.startSanitize(false);
					printFormatSpecific(
							[this, i]{
								o << "ni" << i << " [label=\"";
								o.sanitize(true);
								printNot();
							},
							' '
						);
					::inputNames.printName(o, i);
				}
				if (isGraph())
				{
					o << "\"];\n";
					o << "i" << i << " -> ni" << i << ";\n";
				}
			}
		}
		if (isHuman() && first)
			o << " <none>";
	}
	printFormatSpecific("}\n", '\n');
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
	Implicant primeImplicant = Implicant::error();
	if (isGraphVerbose)
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
	const bool isFullGraph = discriminate<OF::GRAPH>();
	const bool hasParents = isFullGraph || !optimizedSolutions->getProduct(productId).subProducts.empty();
	if (hasParents)
	{
		printAnd(false);
		const auto sanitizeGuard = o.startSanitize(false);
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
	if (!isFullGraph || isGraphVerbose)
		printOptimizedGraphProductImplicant(productId);
	return functionNum;
}

void OutputComposer::printOptimizedGraphProductParents(const OptimizedSolutions::id_t productId) const
{
	const bool isFullGraph = discriminate<OF::GRAPH>();
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
	printAssignmentStart();
	printNormalizedId(productId);
	printFormatSpecific(BLANK, NEXT, NEXT, &OutputComposer::printAssignmentOp);
	if (!isGraph())
	{
		printOptimizedProductBody(productId, false);
	}
	else
	{
		o << " [label=\"";
		const bool isFullGraph = discriminate<OF::GRAPH>();
		const bool hasParents = isFullGraph || !optimizedSolutions->getProduct(productId).subProducts.empty();
		std::size_t functionNum = isFullGraph ? SIZE_MAX : optimizedSolutions->findProductEndNode(productId);
		{
			const auto sanitizeGuard = o.startSanitize();
			functionNum = printOptimizedGraphProductLabel(productId, functionNum);
		}
		o << '"';
		if (functionNum != SIZE_MAX)
			o << ", style=filled";
		o << "]";
		if (hasParents)
		{
			o << ";\n";
			printOptimizedGraphProductParents(productId);
			o << " -> ";
			printNormalizedId(productId);
		}
	}
	printFormatSpecific(";\n", NEXT, '\n', ";\n");
}

void OutputComposer::printOptimizedProducts() const
{
	if (isHuman())
		o << "Products:\n";
	First first;
	const auto indentGuard = o.startIndent(0);
	for (std::size_t i = 0; i != optimizedSolutions->getProductCount(); ++i)
	{
		if (!isOptimizedProductWorthPrinting(optimizedSolutions->makeProductId(i)))
			continue;
		if (first)
		{
			if (discriminate<OF::VHDL>())
				o << '\n';
			if (!isHumanReadable())
				printCommentStart();
			if (isGraph())
				o << "subgraph products\n" << "{\n";
			if (isHuman() || isGraph())
				o.indent();
			printFormatSpecific("node [shape=ellipse];\n", NEXT, BLANK, "Products\n");
		}
		printOptimizedProduct(optimizedSolutions->makeProductId(i));
	}
	if (!first)
	{
		indentGuard.reset();
		printFormatSpecific("}\n", NEXT, BLANK, NEXT, '\n', BLANK);
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
			printOptimizedProductBody(partId, discriminate<OF::MATHEMATICAL>() && sum.size() != 1);
	}
}

std::size_t OutputComposer::printOptimizedGraphSumLabel(const OptimizedSolutions::id_t sumId, std::size_t functionNum) const
{
	const OptimizedSolutions::sum_t &sum = optimizedSolutions->getSum(sumId);
	const bool isFullGraph = discriminate<OF::GRAPH>();
	const bool hasParents = isFullGraph || std::any_of(sum.cbegin(), sum.cend(), [this](const OptimizedSolutions::id_t id){ return !optimizedSolutions->isProduct(id) || isOptimizedProductWorthPrintingInGeneral(id); });
	if (hasParents)
	{
		printOr(false);
		const auto sanitizeGuard = o.startSanitize(false);
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
	if (!isFullGraph || isGraphVerbose)
		printOptimizedGraphSumProducts(sumId);
	return functionNum;
}

void OutputComposer::printOptimizedGraphSumProducts(const OptimizedSolutions::id_t sumId) const
{
	OptimizedSolutions::sum_t sum;
	if (isGraphVerbose)
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
		if (isGraphVerbose)
			printImplicant(optimizedSolutions->flattenProduct(productId), sum.size() != 1, true);
		else
			printImplicant(optimizedSolutions->getProduct(productId).implicant, false, true);
	}
}

void OutputComposer::printOptimizedGraphSumParents(const OptimizedSolutions::id_t sumId) const
{
	const bool isFullGraph = discriminate<OF::GRAPH>();
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
	printAssignmentStart();
	printNormalizedId(sumId);
	printFormatSpecific(BLANK, NEXT, NEXT, &OutputComposer::printAssignmentOp);
	if (!isGraph())
	{
		printOptimizedSumBody(sumId);
	}
	else
	{
		o << " [label=\"";
		const OptimizedSolutions::sum_t &sum = optimizedSolutions->getSum(sumId);
		const bool isFullGraph = discriminate<OF::GRAPH>();
		const bool hasParents = isFullGraph || std::any_of(sum.cbegin(), sum.cend(), [this](const OptimizedSolutions::id_t id){ return !optimizedSolutions->isProduct(id) || isOptimizedProductWorthPrintingInGeneral(id); });
		std::size_t functionNum = isFullGraph ? SIZE_MAX : optimizedSolutions->findSumEndNode(sumId);
		{
			const auto sanitizeGuard = o.startSanitize();
			functionNum = printOptimizedGraphSumLabel(sumId, functionNum);
		}
		o << '"';
		if (functionNum != SIZE_MAX)
			o << ", style=filled";
		o << ']';
		if (hasParents)
		{
			o << ";\n";
			printOptimizedGraphSumParents(sumId);
			o << " -> ";
			printNormalizedId(sumId);
		}
	}
	printFormatSpecific(';', NEXT, BLANK, ';');
	o << '\n';
}

void OutputComposer::printOptimizedSums() const
{
	if (isHuman())
		o << "Sums:\n";
	First first;
	const auto indentGuard = o.startIndent(0);
	for (std::size_t i = 0; i != optimizedSolutions->getSumCount(); ++i)
	{
		if (!isOptimizedSumWorthPrinting(optimizedSolutions->makeSumId(i)))
			continue;
		if (first)
		{
			if (discriminate<OF::VHDL>())
				o << '\n';
			if (!isHumanReadable())
				printCommentStart();
			if (isGraph())
				o << "subgraph sums\n" << "{\n";
			if (isHuman() || isGraph())
				o.indent();
			printFormatSpecific("node [shape=rectangle];\n", NEXT, BLANK, "Sums\n");
		}
		printOptimizedSum(optimizedSolutions->makeSumId(i));
	}
	if (!first)
	{
		indentGuard.reset();
		printFormatSpecific("}\n", NEXT, BLANK, NEXT, '\n', BLANK);
	}
}

void OutputComposer::printOptimizedGraphFinalSumLabel(const std::size_t i) const
{
	const OptimizedSolutions::id_t sumId = optimizedSolutions->getFinalSums()[i];
	if (!isOptimizedSumWorthPrintingInGeneral(sumId) && optimizedSolutions->getSum(sumId).size() > 1)
	{
		printOr(false);
		const auto sanitizeGuard = o.startSanitize(false);
		o << "\\n";
	}
	functionNames.printName(o, i);
	if (isGraphVerbose)
		printOptimizedGraphSumProducts(sumId);
}

void OutputComposer::printOptimizedFinalSums() const
{
	if (discriminate<OF::CPP>())
	{
		printCommentStart();
		o << "Results\n";
	}
	
	if (optimizedSolutions->getFinalSums().empty() && !isHuman())
	{
		if (discriminate<OF::CPP>())
			o << "return {};\n";
		return;
	}
	
	if (discriminate<OF::VHDL>())
		o << '\n';
	if (discriminate<OF::VHDL, OF::VERILOG>())
		printCommentStart();
	if (isGraph())
		o << "subgraph final_sums\n" << "{\n";
	{
		const auto indentGuard = o.startIndent(isHuman() || isGraph() ? 1 : 0);
		printFormatSpecific("node [shape=rectangle, style=filled];\n", NEXT, BLANK, "output_t o = {};\n", NEXT, "Results\n");
		for (std::size_t i = 0; i != optimizedSolutions->getFinalSums().size(); ++i)
		{
			{
				const auto sanitizeGuard = o.startSanitize(false);
				printFormatSpecific([this, i]{ o << "f" << i << " [label=\""; o.sanitize(true); }, BLANK, '"', &OutputComposer::printAssignmentStart);
				if (isGraph())
					printOptimizedGraphFinalSumLabel(i);
				else
					functionNames.printName(o, i);
				if (isHuman())
					o << '"';
				if (discriminate<OF::MATHEMATICAL>())
				{
					o << '(';
					::inputNames.printNames(o);
					o << ")";
				}
			}
			printFormatSpecific("\"];\n", NEXT, NEXT, &OutputComposer::printAssignmentOp);
			
			const OptimizedSolutions::id_t sumId = optimizedSolutions->getFinalSums()[i];
			if (isOptimizedSumWorthPrinting(sumId))
				printNormalizedId(sumId);
			else if (isGraph())
				printOptimizedGraphSumParents(sumId);
			else
				printOptimizedSumBody(sumId);
			printFormatSpecific([this, i]{ o << " -> f" << i << ';'; }, NEXT, BLANK, ';');
			o << '\n';
		}
	}
	printFormatSpecific("}\n", NEXT, BLANK, "return o;\n", '\n', BLANK);
}

void OutputComposer::printOptimizedSolution()
{
	const auto [immediateProductCount, immediateSumCount] = generateOptimizedNormalizedIds();
	
	if (isHuman() || discriminate<OF::GRAPH>())
		printOptimizedNegatedInputs();
	if (!isHumanReadable())
		printOptimizedImmediates(immediateProductCount, immediateSumCount);
	
	if (discriminate<OF::VHDL>())
	{
		const auto indentGuard = o.startIndent(-1);
		o << "begin\n";
	}
	
	if (discriminate<OF::MATHEMATICAL>() && immediateProductCount + immediateSumCount != 0)
		o << "Let:\n";
	printOptimizedProducts();
	printOptimizedSums();
	
	if (discriminate<OF::MATHEMATICAL>() && immediateProductCount + immediateSumCount != 0)
		o << "\nThen:\n";
	if (!discriminate<OF::REDUCED_GRAPH>())
		printOptimizedFinalSums();
	if (discriminate<OF::HUMAN, OF::HUMAN_LONG>())
	{
		o << '\n';
		printGateCost(*optimizedSolutions, false);
	}
	
	if (discriminate<OF::VHDL>())
		o << '\n';
}

void OutputComposer::printGraphConstants() const
{
	const auto [usesFalse, usesTrue] = checkForUsedConstants();
	if (!usesFalse && !usesTrue)
		return;
	o << "subgraph constants\n";
	o << "{\n";
	{
		const auto indentGuard = o.startIndent();
		o << "node [shape=octagon, style=dashed];\n";
		if (usesTrue)
		{
			o << "true [label=\"";
			{
				const auto sanitizeGuard = o.startSanitize();
				printBool(true);
			}
			o << "\"];\n";
		}
		if (usesFalse)
		{
			o << "false [label=\"";
			{
				const auto sanitizeGuard = o.startSanitize();
				printBool(false);
			}
			o << "\"];\n";
		}
	}
	o << "}\n";
}

void OutputComposer::printGraphInputs() const
{
	if (::bits == 0)
		return;
	o << "subgraph inputs\n";
	o << "{\n";
	{
		const auto indentGuard = o.startIndent();
		o << "rank=same;\n";
		o << "node [shape=invhouse];\n";
		for (Minterm i = 0; i != ::bits; ++i)
		{
			o << "i" << i << " [label=\"";
			{
				const auto sanitizeGuard = o.startSanitize();
				::inputNames.printName(o, i);
			}
			o << "\"];\n";
		}
	}
	o << "}\n";
}

void OutputComposer::printGraphRoots() const
{
	printGraphInputs();
	printGraphConstants();
}

void OutputComposer::printHuman()
{
	const bool solutionsVisible = optimizedSolutions == nullptr || !discriminate<OF::HUMAN_SHORT>();
	if (solutionsVisible)
	{
		printSolutions();
		if (!discriminate<OF::HUMAN_SHORT>())
		{
			if (solutions.size() != 1 && optimizedSolutions == nullptr)
			{
				if (!solutions.empty())
					o << "\n\n=== summary ===\n" << '\n';
				printGateCost(solutions, false);
			}
		}
	}
	if (optimizedSolutions != nullptr)
	{
		if (!discriminate<OF::HUMAN_SHORT>())
		{
			if (!solutions.empty())
				o << "\n\n";
			o << "=== optimized solution ===\n" << '\n';
		}
		printOptimizedSolution();
	}
}

void OutputComposer::printGraph()
{
	o << "digraph " << name << '\n';
	o << "{\n";
	{
		const auto indentGuard = o.startIndent();
		if (discriminate<OF::GRAPH>())
			printGraphRoots();
		if (optimizedSolutions == nullptr)
			printSolutions();
		else
			printOptimizedSolution();
	}
	o << "}\n";
}

void OutputComposer::printVerilog()
{
	o << "module " << name << " (\n";
	{
		const auto indentGuard = o.startIndent();
		if (!::inputNames.empty())
		{
			o << "input wire";
			::inputNames.printNames(o);
			o << '\n';
		}
		if (!karnaughs.empty())
		{
			o << "output wire";
			functionNames.printNames(o);
			o << '\n';
		}
	}
	o << ");\n";
	{
		const auto indentGuard = o.startIndent();
		o << '\n';
		if (optimizedSolutions == nullptr)
			printSolutions();
		else
			printOptimizedSolution();
	}
	o << "endmodule\n";
}

void OutputComposer::printVhdl()
{
	o << "library IEEE;\n"
			"use IEEE.std_logic_1164.all;\n";
	o << '\n';
	o << "entity " << name << " is\n";
	if (!::inputNames.empty() || !karnaughs.empty())
	{
		const auto indentGuard = o.startIndent();
		o << "port(\n";
		{
			const auto indentGuard = o.startIndent();
			if (!::inputNames.empty())
			{
				::inputNames.printNames(o);
				o << " : in ";
				::inputNames.printType(o);
				if (!karnaughs.empty())
					o << ';';
				o << '\n';
			}
			if (!karnaughs.empty())
			{
				functionNames.printNames(o);
				o << " : out ";
				functionNames.printType(o);
				o << '\n';
			}
		}
		o << ");\n";
	}
	o << "end " << name << ";\n";
	o << '\n';
	o << "architecture behavioural of " << name << " is\n";
	{
		const auto indentGuard = o.startIndent();
		if (optimizedSolutions == nullptr)
			printSolutions();
		else
			printOptimizedSolution();
	}
	o << "end behavioural;\n";
}

void OutputComposer::printCpp()
{
	if (!::inputNames.areNamesUsedInCode() || !functionNames.areNamesUsedInCode())
		o << "#include <array>\n"
				"\n";
	o << "class " << name << "\n"
			"{\n"
			"public:\n";
	{
		const auto indentGuard = o.startIndent();
		o << "using input_t = ";
		::inputNames.printType(o);
		o << ";\n";
		o << "using output_t = ";
		functionNames.printType(o);
		o << ";\n";
		o << '\n';
		o << "[[nodiscard]] constexpr output_t operator()(";
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
		o << "[[nodiscard]] constexpr output_t operator()(const input_t &i) const { return calc(i); }\n";
		o << '\n';
		o << "[[nodiscard]] static constexpr output_t calc(";
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
		o << "[[nodiscard]] static constexpr output_t calc(const input_t &i);\n";
	}
	o << "};\n";
	o << '\n';
	o << "constexpr " << name << "::output_t " << name << "::calc(const input_t &" << (areInputsUsed() ? "i" : "") << ")\n";
	o << "{\n";
	{
		const auto indentGuard = o.startIndent();
		if (optimizedSolutions == nullptr)
			printSolutions();
		else
			printOptimizedSolution();
	}
	o << "}\n";
}

void OutputComposer::printMath()
{
	if (optimizedSolutions == nullptr)
		printSolutions();
	else
		printOptimizedSolution();
}

void OutputComposer::printGateCost()
{
	printSolutions();
	o << "=== summary ===\n";
	printGateCost(solutions, true);
	if (optimizedSolutions != nullptr)
	{
		o << "=== optimized solution ===\n";
		printGateCost(*optimizedSolutions, true);
	}
}

std::string OutputComposer::getStandardName()
{
	if (options::name)
		return *options::name;
	else if (::inputFilePath)
		return std::filesystem::path(*::inputFilePath).stem().string();
	else
		return "Karnaugh";
}

OutputComposer::OutputComposer(Names &&functionNames, std::vector<Karnaugh> &karnaughs, const Solutions &solutions, const OptimizedSolutions *const optimizedSolutions) :
	functionNames(std::move(functionNames)),
	karnaughs(karnaughs),
	solutions(solutions),
	optimizedSolutions(optimizedSolutions)
{
}

void OutputComposer::print(std::ostream &o, const OF outputFormat, const OO outputOperators, const bool isGraphVerbose, const bool includeBanner, std::string &&name)
{
	this->o.setStream(o);
	this->outputFormat = outputFormat;
	this->outputOperators = outputOperators;
	this->isGraphVerbose = isGraphVerbose;
	this->name = std::move(name);
	
	if (includeBanner)
		printBanner();
	printFormatSpecific(&OutputComposer::printGraph, &OutputComposer::printMath, static_cast<void (OutputComposer::*)()>(&OutputComposer::printGateCost), &OutputComposer::printHuman, &OutputComposer::printCpp, &OutputComposer::printVerilog, &OutputComposer::printVhdl);
}
