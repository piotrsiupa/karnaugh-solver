#include "./OptimizedSolutions.hh"

#include <algorithm>
#include <cassert>

#include "SetOptimizerForProducts.hh"
#include "SetOptimizerForSums.hh"


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
		products.push_back(product_t{newPrimeImplicants[i], subsetSelections[i]});
	
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
		if (product.implicant.getBitCount() == 0 && product.subProducts.empty())
		{
			normalizedSolution.insert(product.implicant);
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
			assert(product.implicant.getBitCount() != 0 || (product.implicant == Implicant::all() && !product.subProducts.empty()));
			idsToProcess.pop_back();
			resultingProduct |= product.implicant;
			idsToProcess.insert(idsToProcess.end(), product.subProducts.cbegin(), product.subProducts.cend());
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

std::size_t OptimizedSolutions::findSumEndNode(const id_t sumId, const std::size_t startFunctionNum) const
{
	for (std::size_t i = startFunctionNum; i != finalSums.size(); ++i)
		if (finalSums[i] == sumId)
			return i;
	return SIZE_MAX;
}

Implicant OptimizedSolutions::flattenProduct(const id_t productId) const
{
	auto [primeImplicant, ids] = getProduct(productId);
	while (!ids.empty())
	{
		const product_t &product = getProduct(ids.back());
		ids.pop_back();
		primeImplicant |= product.implicant;
		ids.insert(ids.end(), product.subProducts.cbegin(), product.subProducts.cend());
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
		if (product.implicant.getBitCount() != 0)
			continue;
		if (product.implicant.isError())
			usesFalse = true;
		else
			usesTrue = true;
	}
	return {usesFalse, usesTrue};
}
