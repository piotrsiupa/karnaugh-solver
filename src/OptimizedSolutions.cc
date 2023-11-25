#include "./OptimizedSolutions.hh"

#include <algorithm>
#include <cassert>
#include <iomanip>

#include "options.hh"
#include "SetOptimizerForProducts.hh"
#include "SetOptimizerForSums.hh"


std::size_t OptimizedSolutions::generateHumanIds() const
{
	const id_t idCount = products.size() + sums.size();
	normalizedIds.resize(idCount);
	id_t currentHumanId = 0;
	for (id_t id = 0; id != idCount; ++id)
		if (isWorthPrinting(id, false))
			normalizedIds[id] = currentHumanId++;
	return currentHumanId;
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
			o << "\twire [" << (immediateProductCount - 1) << ":0] int_prods;\n";
		if (immediateSumCount != 0)
			o << "\twire [" << (immediateSumCount - 1) << ":0] int_sums;\n";
		o << "\t\n";
	}
}

void OptimizedSolutions::printVhdlImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const
{
	if (immediateProductCount != 0 || immediateSumCount != 0)
	{
		o << "\t-- Internal signals\n";
		if (immediateProductCount != 0)
			o << "\tsignal int_prods : std_logic_vector(" << (immediateProductCount - 1) << " downto 0);\n";
		if (immediateSumCount != 0)
			o << "\tsignal int_sums : std_logic_vector(" << (immediateSumCount - 1) << " downto 0);\n";
		o << "\t\n";
	}
}

void OptimizedSolutions::printHumanId(std::ostream &o, const id_t id) const
{
	o << '[' << normalizedIds[id] << ']';
}

void OptimizedSolutions::printVerilogId(std::ostream &o, const id_t id) const
{
	if (isProduct(id))
		o << "int_prods[" << normalizedIds[id] << ']';
	else
		o << "int_sums[" << normalizedIds[id] << ']';
}

void OptimizedSolutions::printVhdlId(std::ostream &o, const id_t id) const
{
	if (isProduct(id))
		o << "int_prods(" << normalizedIds[id] << ')';
	else
		o << "int_sums(" << normalizedIds[id] << ')';
}

void OptimizedSolutions::printHumanNegatedInputs(std::ostream &o) const
{
	o << "Negated inputs:";
	bool first = true;
	for (Minterm i = 0; i != ::bits; ++i)
	{
		if ((negatedInputs & (1 << (::bits - i - 1))) != 0)
		{
			if (first)
			{
				first = false;
				o << ' ';
			}
			else
			{
				o << ", ";
			}
			::inputNames.printHumanName(o, i);
		}
	}
	if (first)
		o << " <none>";
	o << '\n';
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
			o << " && ";
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
	bool anyIsPrinted = false;
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (!isProductWorthPrinting(makeProductId(i)))
			continue;
		if (!anyIsPrinted)
		{
			anyIsPrinted = true;
			o << "\t// Products\n";
		}
		printVerilogProduct(o, makeProductId(i));
	}
	if (anyIsPrinted)
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
	bool anyIsPrinted = false;
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (!isProductWorthPrinting(makeProductId(i)))
			continue;
		if (!anyIsPrinted)
		{
			anyIsPrinted = true;
			o << "\t\n"
				"\t-- Products\n";
		}
		printVhdlProduct(o, makeProductId(i));
	}
}

void OptimizedSolutions::printHumanSumBody(std::ostream &o, const id_t sumId) const
{
	bool first = true;
	for (const auto &partId : getSum(sumId))
	{
		if (first)
			first = false;
		else
			o << " || ";
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

void OptimizedSolutions::printVerilogSumBody(std::ostream &o, const id_t sumId) const
{
	bool first = true;
	for (const auto &partId : getSum(sumId))
	{
		if (first)
			first = false;
		else
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
	bool anyIsPrinted = false;
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		if (!isSumWorthPrinting(makeSumId(i), true))
			continue;
		if (!anyIsPrinted)
		{
			anyIsPrinted = true;
			o << "\t// Sums\n";
		}
		printVerilogSum(o, makeSumId(i));
	}
	if (anyIsPrinted)
		o << "\t\n";
}

void OptimizedSolutions::printVhdlSumBody(std::ostream &o, const id_t sumId) const
{
	bool first = true;
	for (const auto &partId : getSum(sumId))
	{
		if (first)
			first = false;
		else
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
	bool anyIsPrinted = false;
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		if (!isSumWorthPrinting(makeSumId(i), true))
			continue;
		if (!anyIsPrinted)
		{
			anyIsPrinted = true;
			o << "\t\n"
				"\t-- Sums\n";
		}
		printVhdlSum(o, makeSumId(i));
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
		o << "\t\n";
	}
}

void OptimizedSolutions::printGateScores(std::ostream &o) const
{
	o << "Gate scores: NOTs = " << getNotCount() << ", ANDs = " << getAndCount() << ", ORs = " << getOrCount() << '\n';
}

void OptimizedSolutions::createNegatedInputs(const solutions_t &solutions)
{
	for (const Implicants *const solution : solutions)
		for (const auto &x : *solution)
			negatedInputs |= x.getFalseBits();
}

OptimizedSolutions::finalPrimeImplicants_t OptimizedSolutions::extractCommonProductParts(const solutions_t &solutions, Progress &progress)
{
	std::vector<Implicant> oldPrimeImplicants;
	for (const Implicants *const solution : solutions)
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
		for (const Implicants *const solution : solutions)
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

void OptimizedSolutions::validate(const solutions_t &solutions) const
{
	assert(solutions.size() == finalSums.size());
	
	Progress progress(Progress::Stage::OPTIMIZING, "Validating the optimized solution", solutions.size());
	progress.step();
	std::size_t i;
	const Progress::calcSubstepCompletion_t calcSubstepCompletion = [&i = std::as_const(i), n = solutions.size()](){ return static_cast<Progress::completion_t>(i) / static_cast<Progress::completion_t>(n); };
	for (i = 0; i != solutions.size(); ++i)
	{
		progress.substep(calcSubstepCompletion);
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
	validate(solutions);
#endif
}

void OptimizedSolutions::printHuman(std::ostream &o, const Names &functionNames) const
{
	generateHumanIds();
	printHumanNegatedInputs(o);
	printHumanProducts(o);
	printHumanSums(o);
	printHumanFinalSums(o, functionNames);
	if (options::outputFormat.getValue() != options::OutputFormat::SHORT_HUMAN)
		printGateScores(o);
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
}
