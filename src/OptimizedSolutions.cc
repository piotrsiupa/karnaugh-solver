#include "./OptimizedSolutions.hh"

#include <algorithm>
#include <cassert>
#include <iomanip>

#include "SetOptimizerForProducts.hh"
#include "SetOptimizerForSums.hh"


void OptimizedSolutions::generateHumanIds() const
{
	const id_t idCount = products.size() + sums.size();
	humanIds.resize(idCount);
	id_t currentHumanId = 0;
	for (id_t id = 0; id != idCount; ++id)
		if (isWorthPrinting(id))
			humanIds[id] = currentHumanId++;
}

void OptimizedSolutions::printHumanId(std::ostream &o, const id_t id) const
{
	o << '[' << humanIds[id] << ']';
}

void OptimizedSolutions::printNegatedInputs(std::ostream &o) const
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
			o << ::inputNames[i];
		}
	}
	if (first)
		o << " <none>";
	o << '\n';
}

void OptimizedSolutions::printProductBody(std::ostream &o, const id_t productId) const
{
	const auto &[primeImplicant, ids] = getProduct(productId);
	bool first = primeImplicant == PrimeImplicant::all();
	if (!first || ids.empty())
		primeImplicant.print(o, false);
	for (const auto &id : ids)
	{
		if (first)
			first = false;
		else
			o << " && ";
		printHumanId(o, id);
	}
}

void OptimizedSolutions::printProduct(std::ostream &o, const id_t productId) const
{
	o << '\t';
	printHumanId(o, productId);
	o << " = ";
	printProductBody(o, productId);
	o << '\n';
}

void OptimizedSolutions::printProducts(std::ostream &o) const
{
	o << "Products:\n";
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (!isProductWorthPrinting(makeProductId(i)))
			continue;
		printProduct(o, makeProductId(i));
	}
}

void OptimizedSolutions::printSumBody(std::ostream &o, const id_t sumId) const
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
			getProduct(partId).first.print(o, false);
	}
}

void OptimizedSolutions::printSum(std::ostream &o, const id_t sumId) const
{
	o << '\t';
	printHumanId(o, sumId);
	o << " = ";
	printSumBody(o, sumId);
	o << '\n';
}

void OptimizedSolutions::printSums(std::ostream &o) const
{
	o << "Sums:\n";
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		if (!isSumWorthPrinting(makeSumId(i)))
			continue;
		printSum(o, makeSumId(i));
	}
}

void OptimizedSolutions::printFinalSums(std::ostream &o, const std::vector<std::string> &functionNames) const
{
	for (std::size_t i = 0; i != finalSums.size(); ++i)
	{
		o << "\t\"" << functionNames[i] << "\" = ";
		const id_t sumId = finalSums[i];
		if (isSumWorthPrinting(sumId))
			printHumanId(o, sumId);
		else
			printSumBody(o, sumId);
		o << '\n';
	}
}

void OptimizedSolutions::printGateScores(std::ostream &o) const
{
	o << "Gate scores: NOTs = " << getNotCount() << ", ANDs = " << getAndCount() << ", ORs = " << getOrCount() << '\n';
}

void OptimizedSolutions::createNegatedInputs(const solutions_t &solutions)
{
	for (const PrimeImplicants *const solution : solutions)
		for (const auto &x : *solution)
			negatedInputs |= x.getFalseBits();
}

OptimizedSolutions::finalPrimeImplicants_t OptimizedSolutions::extractCommonProductParts(const solutions_t &solutions, Progress &progress)
{
	std::vector<PrimeImplicant> oldPrimeImplicants;
	for (const PrimeImplicants *const solution : solutions)
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
		for (const PrimeImplicants *const solution : solutions)
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
		PrimeImplicant resultingProduct = PrimeImplicant::all();
		while (!idsToProcess.empty())
		{
			const product_t &product = getProduct(idsToProcess.back());
			assert(product.first.getBitCount() != 0 || (product.first == PrimeImplicant::all() && !product.second.empty()));
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
	
	Progress progress("Validating the optimized solution", solutions.size());
	for (std::size_t i = 0; i != solutions.size(); ++i)
	{
		progress.step();
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

void OptimizedSolutions::print(std::ostream &o, const std::vector<std::string> &functionNames) const
{
	generateHumanIds();
	printNegatedInputs(o);
	printProducts(o);
	printSums(o);
	printFinalSums(o, functionNames);
	printGateScores(o);
}
