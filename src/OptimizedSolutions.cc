#include "./OptimizedSolutions.hh"

#include <algorithm>
#include <cassert>
#include <iomanip>

#include "options.hh"
#include "SetOptimizerForProducts.hh"
#include "SetOptimizerForSums.hh"
#include "utils.hh"


void OptimizedSolutions::printHumanAnd(std::ostream &o) const
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

void OptimizedSolutions::printHumanOr(std::ostream &o) const
{
	switch (options::outputOperators.getValue())
	{
	case options::OutputOperators::FORMAL:
		o << u8" \u2228 ";
		break;
	case options::OutputOperators::ASCII:
		o << " \\/ ";
		break;
	case options::OutputOperators::PROGRAMMING:
		o << " || ";
		break;
	case options::OutputOperators::NAMES:
		o << " OR ";
		break;
	default:
		break;
	}
}

void OptimizedSolutions::printGraphNot(std::ostream &o) const
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

void OptimizedSolutions::printGraphAnd(std::ostream &o) const
{
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
}

void OptimizedSolutions::printGraphOr(std::ostream &o, const bool spaces) const
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

Implicant OptimizedSolutions::flattenProduct(const id_t productId) const
{
	auto [primeImplicant, ids] = getProduct(productId);
	while (!ids.empty())
	{
		const product_t &product = getProduct(ids.back());
		ids.pop_back();
		primeImplicant |= product.first;
		ids.insert(ids.end(), product.second.cbegin(), product.second.cend());
	}
	return primeImplicant;
}

std::vector<OptimizedSolutions::id_t> OptimizedSolutions::flattenSum(const id_t sumId) const
{
	sum_t sum = getSum(sumId);
	std::vector<id_t> productsOfSum;
	while (!sum.empty())
	{
		const id_t id = sum.back();
		sum.pop_back();
		if (isProduct(id))
		{
			productsOfSum.push_back(id);
		}
		else
		{
			const sum_t &otherSum = getSum(id);
			sum.insert(sum.end(), otherSum.cbegin(), otherSum.cend());
		}
	}
	std::reverse(productsOfSum.begin(), productsOfSum.end());
	return productsOfSum;
}

void OptimizedSolutions::generateHumanIds() const
{
	const id_t idCount = products.size() + sums.size();
	normalizedIds.resize(idCount);
	id_t currentHumanId = 0;
	for (id_t id = 0; id != idCount; ++id)
		if (isWorthPrinting(id, false))
			normalizedIds[id] = currentHumanId++;
}

void OptimizedSolutions::generateGraphIds() const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	const id_t idCount = products.size() + sums.size();
	normalizedIds.resize(idCount);
	id_t currentGraphId = 0;
	for (id_t id = 0; id != idCount; ++id)
		if (isWorthPrintingOnGraph(id, isFullGraph))
			normalizedIds[id] = currentGraphId++;
}

std::pair<std::size_t, std::size_t> OptimizedSolutions::generateMathIds() const
{
	const id_t idCount = products.size() + sums.size();
	normalizedIds.resize(idCount);
	id_t currentNormalizedProductId = 0, currentNormalizedSumId = 0;
	for (id_t id = 0; id != idCount; ++id)
	{
		if (isProduct(id))
		{
			if (isProductWorthPrintingForMath(id))
				normalizedIds[id] = currentNormalizedProductId++;
		}
		else
		{
			if (isSumWorthPrintingForMath(id))
				normalizedIds[id] = currentNormalizedSumId++;
		}
	}
	return {currentNormalizedProductId, currentNormalizedSumId};
}

std::pair<std::size_t, std::size_t> OptimizedSolutions::generateNormalizedIds() const
{
	const id_t idCount = products.size() + sums.size();
	normalizedIds.resize(idCount);
	id_t currentNormalizedProductId = 0, currentNormalizedSumId = 0;
	for (id_t id = 0; id != idCount; ++id)
	{
		if (isProduct(id))
		{
			if (isProductWorthPrinting(id))
				normalizedIds[id] = currentNormalizedProductId++;
		}
		else
		{
			if (isSumWorthPrinting(id, true))
				normalizedIds[id] = currentNormalizedSumId++;
		}
	}
	return {currentNormalizedProductId, currentNormalizedSumId};
}

void OptimizedSolutions::printVerilogImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const
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

void OptimizedSolutions::printVhdlImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const
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

void OptimizedSolutions::printCppImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const
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

void OptimizedSolutions::printHumanId(std::ostream &o, const id_t id) const
{
	o << '[' << normalizedIds[id] << ']';
}

void OptimizedSolutions::printGraphId(std::ostream &o, const id_t id) const
{
	o << 's' << normalizedIds[id];
}

void OptimizedSolutions::printVerilogId(std::ostream &o, const id_t id) const
{
	if (isProduct(id))
		o << "prods[" << normalizedIds[id] << ']';
	else
		o << "sums[" << normalizedIds[id] << ']';
}

void OptimizedSolutions::printVhdlId(std::ostream &o, const id_t id) const
{
	if (isProduct(id))
		o << "prods(" << normalizedIds[id] << ')';
	else
		o << "sums(" << normalizedIds[id] << ')';
}

void OptimizedSolutions::printCppId(std::ostream &o, const id_t id) const
{
	if (isProduct(id))
		o << "prods[" << normalizedIds[id] << ']';
	else
		o << "sums[" << normalizedIds[id] << ']';
}

void OptimizedSolutions::printMathArgs(std::ostream &o, const id_t id) const
{
	Implicant implicant = Implicant::all();
	ids_t ids{id};
	while (!ids.empty())
	{
		id_t subId = ids.back();
		ids.pop_back();
		if (isProduct(subId))
		{
			const auto &[subImplicant, subIds] = getProduct(subId);
			implicant |= subImplicant;
			ids.insert(ids.end(), subIds.cbegin(), subIds.cend());
		}
		else
		{
			const auto &subIds = getSum(subId);
			ids.insert(ids.end(), subIds.cbegin(), subIds.cend());
		}
	}
	o << '(';
	First first;
	for (const auto &[bit, value] : implicant.splitBits())
	{
		if (!first)
			std::cout << ", ";
		::inputNames.printMathName(std::cout, bit);
	}
	o << ')';
}

void OptimizedSolutions::printMathId(std::ostream &o, const id_t id) const
{
	if (isProduct(id))
		o << 'p' << (normalizedIds[id] + 1);
	else
		o << 's' << (normalizedIds[id] + 1);
	printMathArgs(o, id);
}

void OptimizedSolutions::printHumanNegatedInputs(std::ostream &o) const
{
	o << "Negated inputs:";
	First first;
	for (Minterm i = 0; i != ::bits; ++i)
	{
		if ((negatedInputs & (1 << (::bits - i - 1))) != 0)
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

void OptimizedSolutions::printGraphNegatedInputs(std::ostream &o) const
{
	if (negatedInputs == 0)
		return;
	o << "\tsubgraph negated_inputs\n";
	o << "\t{\n";
	o << "\t\tnode [shape=diamond];\n";
	for (Minterm i = 0; i != ::bits; ++i)
	{
		if ((negatedInputs & (1 << (::bits - i - 1))) != 0)
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

void OptimizedSolutions::printHumanProductBody(std::ostream &o, const id_t productId) const
{
	const auto &[primeImplicant, ids] = getProduct(productId);
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		primeImplicant.printHuman(o, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			printHumanAnd(o);
		else
			needsAnd = true;
		printHumanId(o, id);
	}
}

void OptimizedSolutions::printHumanProduct(std::ostream &o, const id_t productId) const
{
	o << '\t';
	printHumanId(o, productId);
	o << " = ";
	printHumanProductBody(o, productId);
	o << '\n';
}

void OptimizedSolutions::printHumanProducts(std::ostream &o) const
{
	o << "Products:\n";
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (!isProductWorthPrinting(makeProductId(i)))
			continue;
		printHumanProduct(o, makeProductId(i));
	}
}

std::size_t OptimizedSolutions::findProductEndNode(const id_t productId, std::size_t startFunctionNum) const
{
	for (std::size_t i = startFunctionNum; i != finalSums.size(); ++i)
	{
		const sum_t &sum = getSum(finalSums[i]);
		if (sum.size() == 1 && sum.front() == productId)
			return i;
	}
	return SIZE_MAX;
}

void OptimizedSolutions::printGraphProductImplicant(std::ostream &o, const id_t productId) const
{
	const bool isVerboseGraph = options::verboseGraph.isRaised();
	Implicant primeImplicant = Implicant::error();
	if (isVerboseGraph)
	{
		primeImplicant = flattenProduct(productId);
	}
	else
	{
		const product_t &product = getProduct(productId);
		if (product.first.getBitCount() == 0 && !product.second.empty())
			return;
		primeImplicant = product.first;
	}
	o << " = ";
	primeImplicant.printGraph(o, false);
}

void OptimizedSolutions::printGraphProductParents(std::ostream &o, const id_t productId) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	const auto &[primeImplicant, ids] = getProduct(productId);
	bool needsComma = isFullGraph && primeImplicant != Implicant::all();
	if (needsComma || ids.empty())
		primeImplicant.printGraph(o);
	for (const auto &id : ids)
	{
		if (needsComma)
			o << ", ";
		else
			needsComma = true;
		printGraphId(o, id);
	}
}

void OptimizedSolutions::printGraphProduct(std::ostream &o, const Names &functionNames, const id_t productId) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	const bool isVerboseGraph = options::verboseGraph.isRaised();
	std::size_t functionNum = isFullGraph ? SIZE_MAX : findProductEndNode(productId);
	o << "\t\t";
	printGraphId(o, productId);
	o << " [label=\"";
	const bool hasParents = isFullGraph || !getProduct(productId).second.empty();
	if (hasParents)
	{
		printGraphAnd(o);
		o << "\\n";
	}
	if (functionNum == SIZE_MAX)
	{
		printHumanId(o, productId);
	}
	else
	{
		while (true)
		{
			functionNames.printGraphName(o, functionNum);
			const std::size_t additionalFunNum = findProductEndNode(productId, functionNum + 1);
			if (additionalFunNum == SIZE_MAX)
				break;
			o << ", ";
			functionNum = additionalFunNum;
		}
	}
	if (!isFullGraph || isVerboseGraph)
		printGraphProductImplicant(o, productId);
	o << '"';
	if (functionNum != SIZE_MAX)
		o << ", style=filled";
	o << "];";
	o << '\n';
	if (hasParents)
	{
		o << "\t\t";
		printGraphProductParents(o, productId);
		o << " -> ";
		printGraphId(o, productId);
		o << ";\n";
	}
}

void OptimizedSolutions::printGraphProducts(std::ostream &o, const Names &functionNames) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	First first;
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (!isProductWorthPrintingOnGraph(makeProductId(i), isFullGraph))
			continue;
		if (first)
		{
			o << "\tsubgraph products\n";
			o << "\t{\n";
			o << "\t\tnode [shape=ellipse];\n";
		}
		printGraphProduct(o, functionNames, makeProductId(i));
	}
	if (!first)
		o << "\t}\n";
}

void OptimizedSolutions::printVerilogProductBody(std::ostream &o, const id_t productId) const
{
	const auto &[primeImplicant, ids] = getProduct(productId);
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		primeImplicant.printVerilog(o, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			o << " & ";
		else
			needsAnd = true;
		printVerilogId(o, id);
	}
}

void OptimizedSolutions::printVerilogProduct(std::ostream &o, const id_t productId) const
{
	o << "\tassign ";
	printVerilogId(o, productId);
	o << " = ";
	printVerilogProductBody(o, productId);
	o << ";\n";
}

void OptimizedSolutions::printVerilogProducts(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (!isProductWorthPrinting(makeProductId(i)))
			continue;
		if (first)
			o << "\t// Products\n";
		printVerilogProduct(o, makeProductId(i));
	}
	if (!first)
		o << "\t\n";
}

void OptimizedSolutions::printVhdlProductBody(std::ostream &o, const id_t productId) const
{
	const auto &[primeImplicant, ids] = getProduct(productId);
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		primeImplicant.printVhdl(o, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			o << " and ";
		else
			needsAnd = true;
		printVhdlId(o, id);
	}
}

void OptimizedSolutions::printVhdlProduct(std::ostream &o, const id_t productId) const
{
	o << '\t';
	printVhdlId(o, productId);
	o << " <= ";
	printVhdlProductBody(o, productId);
	o << ";\n";
}

void OptimizedSolutions::printVhdlProducts(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (!isProductWorthPrinting(makeProductId(i)))
			continue;
		if (first)
			o << "\t\n\t-- Products\n";
		printVhdlProduct(o, makeProductId(i));
	}
}

void OptimizedSolutions::printCppProductBody(std::ostream &o, const id_t productId) const
{
	const auto &[primeImplicant, ids] = getProduct(productId);
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		primeImplicant.printCpp(o, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			o << " && ";
		else
			needsAnd = true;
		printCppId(o, id);
	}
}

void OptimizedSolutions::printCppProduct(std::ostream &o, const id_t productId) const
{
	o << '\t';
	printCppId(o, productId);
	o << " = ";
	printCppProductBody(o, productId);
	o << ";\n";
}

void OptimizedSolutions::printCppProducts(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (!isProductWorthPrinting(makeProductId(i)))
			continue;
		if (first)
			o << "\t// Products\n";
		printCppProduct(o, makeProductId(i));
	}
	if (!first)
		o << "\t\n";
}

void OptimizedSolutions::printMathProductBody(std::ostream &o, const id_t productId, const bool parentheses) const
{
	const auto &[primeImplicant, ids] = getProduct(productId);
	const bool parenthesesNeeded = parentheses && primeImplicant.getBitCount() + ids.size() >= 2;
	if (parenthesesNeeded)
		o << '(';
	bool needsAnd = primeImplicant != Implicant::all();
	if (needsAnd || ids.empty())
		primeImplicant.printHuman(o, false);
	for (const auto &id : ids)
	{
		if (needsAnd)
			printHumanAnd(o);
		else
			needsAnd = true;
		printMathId(o, id);
	}
	if (parenthesesNeeded)
		o << ')';
}

void OptimizedSolutions::printMathProduct(std::ostream &o, const id_t productId) const
{
	printMathId(o, productId);
	o << " = ";
	printMathProductBody(o, productId, false);
	o << '\n';
}

void OptimizedSolutions::printMathProducts(std::ostream &o) const
{
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		const id_t productId = makeProductId(i);
		if (!isProductWorthPrintingForMath(productId))
			continue;
		printMathProduct(o, productId);
	}
}

bool OptimizedSolutions::isSumWorthPrinting(const id_t sumId, const bool simpleFinalSums) const
{
	if (simpleFinalSums)
		return getSum(sumId).size() >= 2;
	if (std::count(finalSums.cbegin(), finalSums.cend(), sumId) >= 2 && getSum(sumId).size() >= 2)
		return true;
	for (const sum_t &sum : sums)
		for (const id_t &id : sum)
			if (id == sumId)
				return true;
	return false;
}

void OptimizedSolutions::printHumanSumBody(std::ostream &o, const id_t sumId) const
{
	First first;
	for (const auto &partId : getSum(sumId))
	{
		if (!first)
			printHumanOr(o);
		if (!isProduct(partId) || isProductWorthPrinting(partId))
			printHumanId(o, partId);
		else
			getProduct(partId).first.printHuman(o, false);
	}
}

void OptimizedSolutions::printHumanSum(std::ostream &o, const id_t sumId) const
{
	o << '\t';
	printHumanId(o, sumId);
	o << " = ";
	printHumanSumBody(o, sumId);
	o << '\n';
}

void OptimizedSolutions::printHumanSums(std::ostream &o) const
{
	o << "Sums:\n";
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		if (!isSumWorthPrinting(makeSumId(i), false))
			continue;
		printHumanSum(o, makeSumId(i));
	}
}

std::size_t OptimizedSolutions::findSumEndNode(const id_t sumId, const std::size_t startFunctionNum) const
{
	for (std::size_t i = startFunctionNum; i != finalSums.size(); ++i)
		if (finalSums[i] == sumId)
			return i;
	return SIZE_MAX;
}

void OptimizedSolutions::printGraphSumProducts(std::ostream &o, const id_t sumId) const
{
	const bool isVerboseGraph = options::verboseGraph.isRaised();
	sum_t sum;
	if (isVerboseGraph)
	{
		sum = flattenSum(sumId);
	}
	else
	{
		sum = getSum(sumId);
		sum.erase(std::remove_if(sum.begin(), sum.end(), [this](const id_t id){ return !isProduct(id) || isProductWorthPrinting(id); }), sum.end());
	}
	First first;
	for (const auto &productId : sum)
	{
		if (first)
			o << " = ";
		else
			printGraphOr(o, true);
		if (isVerboseGraph)
			flattenProduct(productId).printGraph(o, sum.size() != 1);
		else
			getProduct(productId).first.printHuman(o, false);
	}
}

void OptimizedSolutions::printGraphSumParents(std::ostream &o, const id_t sumId) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	First first;
	for (const auto &partId : getSum(sumId))
	{
		if (!isProduct(partId) || isProductWorthPrinting(partId))
		{
			if (!first)
				o << ", ";
			printGraphId(o, partId);
		}
		else if (isFullGraph)
		{
			if (!first)
				o << ", ";
			getProduct(partId).first.printGraph(o);
		}
	}
}

void OptimizedSolutions::printGraphSum(std::ostream &o, const Names &functionNames, const id_t sumId) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	const bool isVerboseGraph = options::verboseGraph.isRaised();
	const sum_t &sum = getSum(sumId);
	std::size_t functionNum = isFullGraph ? SIZE_MAX : findSumEndNode(sumId);
	o << "\t\t";
	printGraphId(o, sumId);
	o << " [label=\"";
	const bool hasParents = isFullGraph || std::any_of(sum.cbegin(), sum.cend(), [this](const id_t id){ return !isProduct(id) || isProductWorthPrinting(id); });
	if (hasParents)
	{
		printGraphOr(o, false);
		o << "\\n";
	}
	if (functionNum == SIZE_MAX)
	{
		printHumanId(o, sumId);
	}
	else
	{
		while (true)
		{
			functionNames.printGraphName(o, functionNum);
			const std::size_t additionalFunNum = findSumEndNode(sumId, functionNum + 1);
			if (additionalFunNum == SIZE_MAX)
				break;
			o << ", ";
			functionNum = additionalFunNum;
		}
	}
	if (!isFullGraph || isVerboseGraph)
		printGraphSumProducts(o, sumId);
	o << '"';
	if (functionNum != SIZE_MAX)
		o << ", style=filled";
	o << "];\n";
	if (hasParents)
	{
		o << "\t\t";
		printGraphSumParents(o, sumId);
		o << " -> ";
		printGraphId(o, sumId);
		o << ";\n";
	}
}

void OptimizedSolutions::printGraphSums(std::ostream &o, const Names &functionNames) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	First first;
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		const id_t sumId = makeSumId(i);
		if (!isSumWorthPrintingOnGraph(sumId, isFullGraph))
			continue;
		if (first)
		{
			o << "\tsubgraph sums\n";
			o << "\t{\n";
			o << "\t\tnode [shape=rectangle];\n";
		}
		printGraphSum(o, functionNames, sumId);
	}
	if (!first)
		o << "\t}\n";
}

void OptimizedSolutions::printVerilogSumBody(std::ostream &o, const id_t sumId) const
{
	First first;
	for (const auto &partId : getSum(sumId))
	{
		if (!first)
			o << " | ";
		if (!isProduct(partId) || isProductWorthPrinting(partId))
			printVerilogId(o, partId);
		else
			getProduct(partId).first.printVerilog(o, false);
	}
}

void OptimizedSolutions::printVerilogSum(std::ostream &o, const id_t sumId) const
{
	o << "\tassign ";
	printVerilogId(o, sumId);
	o << " = ";
	printVerilogSumBody(o, sumId);
	o << ";\n";
}

void OptimizedSolutions::printVerilogSums(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		if (!isSumWorthPrinting(makeSumId(i), true))
			continue;
		if (first)
			o << "\t// Sums\n";
		printVerilogSum(o, makeSumId(i));
	}
	if (!first)
		o << "\t\n";
}

void OptimizedSolutions::printVhdlSumBody(std::ostream &o, const id_t sumId) const
{
	First first;
	for (const auto &partId : getSum(sumId))
	{
		if (!first)
			o << " or ";
		if (!isProduct(partId) || isProductWorthPrinting(partId))
			printVhdlId(o, partId);
		else
			getProduct(partId).first.printVhdl(o, false);
	}
}

void OptimizedSolutions::printVhdlSum(std::ostream &o, const id_t sumId) const
{
	o << '\t';
	printVhdlId(o, sumId);
	o << " <= ";
	printVhdlSumBody(o, sumId);
	o << ";\n";
}

void OptimizedSolutions::printVhdlSums(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		if (!isSumWorthPrinting(makeSumId(i), true))
			continue;
		if (first)
			o << "\t\n\t-- Sums\n";
		printVhdlSum(o, makeSumId(i));
	}
}

void OptimizedSolutions::printCppSumBody(std::ostream &o, const id_t sumId) const
{
	First first;
	for (const auto &partId : getSum(sumId))
	{
		if (!first)
			o << " || ";
		if (!isProduct(partId) || isProductWorthPrinting(partId))
			printCppId(o, partId);
		else
			getProduct(partId).first.printCpp(o, false);
	}
}

void OptimizedSolutions::printCppSum(std::ostream &o, const id_t sumId) const
{
	o << '\t';
	printCppId(o, sumId);
	o << " = ";
	printCppSumBody(o, sumId);
	o << ";\n";
}

void OptimizedSolutions::printCppSums(std::ostream &o) const
{
	First first;
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		if (!isSumWorthPrinting(makeSumId(i), true))
			continue;
		if (first)
			o << "\t// Sums\n";
		printCppSum(o, makeSumId(i));
	}
	if (!first)
		o << "\t\n";
}

void OptimizedSolutions::printMathSumBody(std::ostream &o, const id_t sumId) const
{
	First first;
	const sum_t &sum = getSum(sumId);
	for (const auto &partId : sum)
	{
		if (!first)
			printHumanOr(o);
		if (!isProduct(partId) || isProductWorthPrintingForMath(partId))
			printMathId(o, partId);
		else
			printMathProductBody(o, partId, sum.size() != 1);
	}
}

void OptimizedSolutions::printMathSum(std::ostream &o, const id_t sumId) const
{
	printMathId(o, sumId);
	o << " = ";
	printMathSumBody(o, sumId);
	o << '\n';
}

void OptimizedSolutions::printMathSums(std::ostream &o) const
{
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		const id_t sumId = makeSumId(i);
		if (!isSumWorthPrintingForMath(sumId))
			continue;
		printMathSum(o, sumId);
	}
}

void OptimizedSolutions::printHumanFinalSums(std::ostream &o, const Names &functionNames) const
{
	for (std::size_t i = 0; i != finalSums.size(); ++i)
	{
		o << "\t\"";
		functionNames.printHumanName(o, i);
		o << "\" = ";
		const id_t sumId = finalSums[i];
		if (isSumWorthPrinting(sumId, false))
			printHumanId(o, sumId);
		else
			printHumanSumBody(o, sumId);
		o << '\n';
	}
}

void OptimizedSolutions::printGraphFinalSum(std::ostream &o, const Names &functionNames, const std::size_t i) const
{
	const bool isVerboseGraph = options::verboseGraph.isRaised();
	const id_t sumId = finalSums[i];
	o << "\t\tf" << i << " [label=\"";
	if (!isSumWorthPrinting(sumId, false) && getSum(sumId).size() > 1)
	{
		printGraphOr(o, false);
		o << "\\n";
	}
	functionNames.printGraphName(o, i);
	if (isVerboseGraph)
		printGraphSumProducts(o, sumId);
	o << "\"];\n";
	o << "\t\t";
	if (isSumWorthPrinting(sumId, false))
		printGraphId(o, sumId);
	else
		printGraphSumParents(o, sumId);
	o << " -> f" << i << ";\n";
}

void OptimizedSolutions::printGraphFinalSums(std::ostream &o, const Names &functionNames) const
{
	if (finalSums.empty())
		return;
	o << "\tsubgraph final_sums\n";
	o << "\t{\n";
	o << "\t\tnode [shape=rectangle, style=filled];\n";
	for (std::size_t i = 0; i != finalSums.size(); ++i)
		printGraphFinalSum(o, functionNames, i);
	o << "\t}\n";
}

void OptimizedSolutions::printVerilogFinalSums(std::ostream &o, const Names &functionNames) const
{
	if (!finalSums.empty())
	{
		o << "\t// Results\n";
		for (std::size_t i = 0; i != finalSums.size(); ++i)
		{
			o << "\tassign ";
			functionNames.printVerilogName(o, i);
			o << " = ";
			const id_t sumId = finalSums[i];
			if (isSumWorthPrinting(sumId, true))
				printVerilogId(o, finalSums[i]);
			else
				printVerilogSumBody(o, sumId);
			o << ";\n";
		}
		o << "\t\n";
	}
}

void OptimizedSolutions::printVhdlFinalSums(std::ostream &o, const Names &functionNames) const
{
	if (!finalSums.empty())
	{
		o << "\t\n";
		o << "\t-- Results\n";
		for (std::size_t i = 0; i != finalSums.size(); ++i)
		{
			o << '\t';
			functionNames.printVhdlName(o, i);
			o << " <= ";
			const id_t sumId = finalSums[i];
			if (isSumWorthPrinting(sumId, true))
				printVhdlId(o, finalSums[i]);
			else
				printVhdlSumBody(o, sumId);
			o << ";\n";
		}
	}
}

void OptimizedSolutions::printCppFinalSums(std::ostream &o, const Names &functionNames) const
{
	o << "\t// Results\n";
	if (finalSums.empty())
	{
		o << "\treturn {};\n";
	}
	else
	{
		o << "\toutput_t o = {};\n";
		for (std::size_t i = 0; i != finalSums.size(); ++i)
		{
			o << '\t';
			functionNames.printCppName(o, i);
			o << " = ";
			const id_t sumId = finalSums[i];
			if (isSumWorthPrinting(sumId, true))
				printCppId(o, finalSums[i]);
			else
				printCppSumBody(o, sumId);
			o << ";\n";
		}
		o << "\treturn o;\n";
	}
}

void OptimizedSolutions::printMathFinalSums(std::ostream &o, const Names &functionNames) const
{
	if (!finalSums.empty())
	{
		for (std::size_t i = 0; i != finalSums.size(); ++i)
		{
			functionNames.printMathName(o, i);
			std::cout << '(';
			::inputNames.printMathNames(std::cout);
			std::cout << ") = ";
			const id_t sumId = finalSums[i];
			if (isSumWorthPrintingForMath(sumId))
				printMathId(o, finalSums[i]);
			else
				printMathSumBody(o, sumId);
			o << '\n';
		}
	}
}

void OptimizedSolutions::createNegatedInputs(const solutions_t &solutions)
{
	for (const Solution *const solution : solutions)
		for (const auto &x : *solution)
			negatedInputs |= x.getFalseBits();
}

OptimizedSolutions::finalPrimeImplicants_t OptimizedSolutions::extractCommonProductParts(const solutions_t &solutions, Progress &progress)
{
	std::vector<Implicant> oldPrimeImplicants;
	for (const Solution *const solution : solutions)
		for (const auto &product: *solution)
			oldPrimeImplicants.push_back(product);
	const auto [newPrimeImplicants, finalPrimeImplicants, subsetSelections] = SetOptimizerForProducts::optimizeSet(oldPrimeImplicants, progress);
	
	products.reserve(subsetSelections.size());
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
		products.emplace_back(newPrimeImplicants[i], subsetSelections[i]);
	
	return finalPrimeImplicants;
}

void OptimizedSolutions::extractCommonSumParts(const solutions_t &solutions, const finalPrimeImplicants_t &finalPrimeImplicants, Progress &progress)
{
	std::vector<std::set<std::size_t>> oldIdSets;
	{
		std::size_t i = 0;
		for (const Solution *const solution : solutions)
		{
			oldIdSets.emplace_back();
			auto &oldIdSet = oldIdSets.back();
			for (std::size_t j = 0; j != solution->size(); ++j)
				oldIdSet.insert(finalPrimeImplicants[i++]);
		}
	}
	const auto [newIdSets, finalIdSets, subsetSelections] = SetOptimizerForSums::optimizeSet(oldIdSets, progress);
	
	sums.reserve(subsetSelections.size());
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
	{
		const auto &newIdSet = newIdSets[i];
		const auto &subsetSelection = subsetSelections[i];
		sums.emplace_back();
		auto &sum = sums.back();
		sum.insert(sum.end(), newIdSet.begin(), newIdSet.end());
		sum.reserve(newIdSet.size() + subsetSelection.size());
		for (const std::size_t &subset : subsetSelection)
			sum.push_back(makeSumId(subset));
	}
	
	finalSums.reserve(finalIdSets.size());
	for (const std::size_t &finalIdSet : finalIdSets)
		finalSums.push_back(makeSumId(finalIdSet));
}

#ifndef NDEBUG
OptimizedSolutions::normalizedSolution_t OptimizedSolutions::normalizeSolution(const id_t finalSumId) const
{
	ids_t rootProductIds, idsToProcess(1, finalSumId);
	while (!idsToProcess.empty())
	{
		const sum_t &sum = getSum(idsToProcess.back());
		idsToProcess.pop_back();
		for (const id_t &id : sum)
			if (isProduct(id))
				rootProductIds.push_back(id);
			else
				idsToProcess.push_back(id);
	}
	assert(!rootProductIds.empty());
	
	normalizedSolution_t normalizedSolution;
	if (rootProductIds.size() == 1)
	{
		const product_t &product = getProduct(rootProductIds.front());
		if (product.first.getBitCount() == 0 && product.second.empty())
		{
			normalizedSolution.insert(product.first);
			return normalizedSolution;
		}
	}
	for (const id_t &rootProductId : rootProductIds)
	{
		idsToProcess.push_back(rootProductId);
		Implicant resultingProduct = Implicant::all();
		while (!idsToProcess.empty())
		{
			const product_t &product = getProduct(idsToProcess.back());
			assert(product.first.getBitCount() != 0 || (product.first == Implicant::all() && !product.second.empty()));
			idsToProcess.pop_back();
			resultingProduct |= product.first;
			idsToProcess.insert(idsToProcess.end(), product.second.cbegin(), product.second.cend());
		}
		normalizedSolution.insert(std::move(resultingProduct));
	}
	return normalizedSolution;
}

void OptimizedSolutions::validate(const solutions_t &solutions, Progress &progress) const
{
	assert(solutions.size() == finalSums.size());
	
	const auto infoGuard = progress.addInfo("validating");
	progress.step();
	auto progressStep = progress.makeCountingStepHelper(static_cast<Progress::completion_t>(solutions.size()));
	for (std::size_t i = 0; i != solutions.size(); ++i)
	{
		progressStep.substep();
		const normalizedSolution_t expectedSolution(solutions[i]->cbegin(), solutions[i]->cend());
		const normalizedSolution_t actualSolution = normalizeSolution(finalSums[i]);
		assert(actualSolution == expectedSolution);
	}
}
#endif

OptimizedSolutions::OptimizedSolutions(const solutions_t &solutions, Progress &progress)
{
	createNegatedInputs(solutions);
	const finalPrimeImplicants_t finalPrimeImplicants = extractCommonProductParts(solutions, progress);
	extractCommonSumParts(solutions, finalPrimeImplicants, progress);
#ifndef NDEBUG
	validate(solutions, progress);
#endif
}

std::pair<bool, bool> OptimizedSolutions::checkForUsedConstants() const
{
	bool usesFalse = false, usesTrue = false;
	for (const id_t sumId : finalSums)
	{
		const sum_t &sum = getSum(sumId);
		if (sum.size() >= 2)
			continue;
		const id_t productId = sum.front();
		if (!isProduct(productId))
			continue;
		const product_t &product = getProduct(productId);
		if (product.first.getBitCount() != 0)
			continue;
		if (product.first.isError())
			usesFalse = true;
		else
			usesTrue = true;
	}
	return {usesFalse, usesTrue};
}

void OptimizedSolutions::printHuman(std::ostream &o, const Names &functionNames) const
{
	generateHumanIds();
	printHumanNegatedInputs(o);
	printHumanProducts(o);
	printHumanSums(o);
	printHumanFinalSums(o, functionNames);
	if (options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT)
	{
		o << '\n';
		printGateCost(o, false);
	}
}

void OptimizedSolutions::printGraph(std::ostream &o, const Names &functionNames) const
{
	const bool isFullGraph = options::outputFormat.getValue() == options::OutputFormat::GRAPH;
	generateGraphIds();
	if (isFullGraph)
		printGraphNegatedInputs(o);
	printGraphProducts(o, functionNames);
	printGraphSums(o, functionNames);
	if (isFullGraph)
		printGraphFinalSums(o, functionNames);
}

void OptimizedSolutions::printVerilog(std::ostream &o, const Names &functionNames) const
{
	const auto [immediateProductCount, immediateSumCount] = generateNormalizedIds();
	printVerilogImmediates(o, immediateProductCount, immediateSumCount);
	printVerilogProducts(o);
	printVerilogSums(o);
	printVerilogFinalSums(o, functionNames);
}

void OptimizedSolutions::printVhdl(std::ostream &o, const Names &functionNames) const
{
	const auto [immediateProductCount, immediateSumCount] = generateNormalizedIds();
	printVhdlImmediates(o, immediateProductCount, immediateSumCount);
	o << "begin\n";
	printVhdlProducts(o);
	printVhdlSums(o);
	printVhdlFinalSums(o, functionNames);
	o << "\t\n";
}

void OptimizedSolutions::printCpp(std::ostream &o, const Names &functionNames) const
{
	const auto [immediateProductCount, immediateSumCount] = generateNormalizedIds();
	printCppImmediates(o, immediateProductCount, immediateSumCount);
	printCppProducts(o);
	printCppSums(o);
	printCppFinalSums(o, functionNames);
}

void OptimizedSolutions::printMath(std::ostream &o, const Names &functionNames) const
{
	const auto [immediateProductCount, immediateSumCount] = generateMathIds();
	if (immediateProductCount != 0 || immediateSumCount != 0)
	{
		std::cout << "Let:\n";
		printMathProducts(o);
		printMathSums(o);
		std::cout << "\nThen:\n";
	}
	printMathFinalSums(o, functionNames);
}
