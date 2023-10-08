#include "./OptimizedSolution.hh"

#include <algorithm>
#include <cassert>
#include <iomanip>

#include "OptimizationHasseDiagram.hh"
#include "SetOptimizerForProducts.hh"
#include "SetOptimizerForSums.hh"


void OptimizedSolution::printNegatedInputs(std::ostream &o) const
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

void OptimizedSolution::printProducts(std::ostream &o) const
{
	o << "Products:";
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (i == 0)
			o << ' ';
		else
			o << ",  ";
		o << '[' << i << "] = ";
		const auto &[primeImplicant, ids] = products[i];
		bool first = primeImplicant == PrimeImplicant::all();
		if (!first || ids.empty())
			primeImplicant.print(o, false);
		for (const auto &id : ids)
		{
			if (first)
				first = false;
			else
				o << " && ";
			o << '[' << id << ']';
		}
	}
	if (products.empty())
		o << " <none>";
	o << '\n';
}

void OptimizedSolution::printSums(std::ostream &o) const
{
	o << "Sums:";
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		if (i == 0)
			o << ' ';
		else
			o << ",  ";
		o << '[' << makeSumId(i) << "] = ";
		bool first = true;
		for (const auto &id : sums[i])
		{
			if (first)
				first = false;
			else
				o << " || ";
			o << '[' << id << ']';
		}
	}
	if (sums.empty())
		o << " <none>";
	o << '\n';
}

void OptimizedSolution::printFinalSums(std::ostream &o, const strings_t &functionNames) const
{
	o << "Final sums:";
	for (std::size_t i = 0; i != finalSums.size(); ++i)
	{
		if (i == 0)
			o << ' ';
		else
			o << ", ";
		o << '"' << functionNames[i] << "\" = [" << finalSums[i] << ']';
	}
	if (finalSums.empty())
		o << " <none>";
	o << '\n';
}

void OptimizedSolution::printGateScores(std::ostream &o) const
{
	o << "Gate scores: NOTs = " << getNotCount() << ", ANDs = " << getAndCount() << ", ORs = " << getOrCount() << '\n';
}

void OptimizedSolution::createNegatedInputs(const solutions_t &solutions)
{
	for (const PrimeImplicants *const solution : solutions)
		for (const auto &x : *solution)
			negatedInputs |= x.getFalseBits();
}

OptimizedSolution::finalPrimeImplicants_t OptimizedSolution::extractCommonParts(const solutions_t &solutions)
{
	std::vector<PrimeImplicant> oldPrimeImplicants;
	for (const PrimeImplicants *const solution : solutions)
		for (const auto &product: *solution)
			oldPrimeImplicants.push_back(product);
	const auto [newPrimeImplicants, finalPrimeImplicants, subsetSelections] = SetOptimizerForProducts().extractCommonParts(oldPrimeImplicants);
	
	products.reserve(subsetSelections.size());
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
		products.emplace_back(newPrimeImplicants[i], subsetSelections[i]);
	
	return finalPrimeImplicants;
}

void OptimizedSolution::extractCommonParts(const solutions_t &solutions, const finalPrimeImplicants_t &finalPrimeImplicants)
{
	std::vector<std::set<std::size_t>> oldIdSets;
	std::size_t i = 0;
	for (const PrimeImplicants *const solution : solutions)
	{
		oldIdSets.emplace_back();
		auto &oldIdSet = oldIdSets.back();
		for (std::size_t j = 0; j != solution->size(); ++j)
			oldIdSet.insert(finalPrimeImplicants[i++]);
	}
	const auto [newIdSets, finalIdSets, subsetSelections] = SetOptimizerForSums().extractCommonParts(oldIdSets);
	
	sums.reserve(subsetSelections.size());
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
	{
		const auto &newIdSet = newIdSets[i];
		const auto &subsetSelection = subsetSelections[i];
		sums.emplace_back();
		auto &sum = sums.back();
		sum.insert(sum.end(), newIdSet.begin(), newIdSet.end());
		sum.reserve(newIdSet.size() + subsetSelection.size());
		for (const std::size_t &subsetSelections : subsetSelection)
			sum.push_back(makeSumId(subsetSelections));
	}
	
	finalSums.reserve(finalIdSets.size());
	for (const std::size_t &finalIdSet : finalIdSets)
		finalSums.push_back(makeSumId(finalIdSet));
}

void OptimizedSolution::cleanupProducts()
{
	for (auto productIter = products.rbegin(); productIter != products.rend(); ++productIter)
		for (const id_t productId : productIter->second)
			productIter->first -= getProduct(productId).first;
}

void OptimizedSolution::cleanupSums()
{
	for (auto sumIter = sums.rbegin(); sumIter != sums.rend(); ++sumIter)
	{
		for (const id_t id : *sumIter)
		{
			if (!isProduct(id))
			{
				const auto &referedSum = getSum(id);
				sumIter->erase(std::remove_if(sumIter->begin(), sumIter->end(), [&referedSum](const id_t x){ return std::find(referedSum.cbegin(), referedSum.cend(), x) != referedSum.cend(); }), sumIter->end());
			}
		}
		std::sort(sumIter->begin(), sumIter->end());
	}
}

#ifndef NDEBUG
OptimizedSolution::normalizedSolution_t OptimizedSolution::normalizeSolution(const id_t finalSumId) const
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

void OptimizedSolution::validate(const solutions_t &solutions) const
{
	assert(solutions.size() == finalSums.size());
	
	for (std::size_t i = 0; i != solutions.size(); ++i)
	{
		const normalizedSolution_t expectedSolution(solutions[i]->cbegin(), solutions[i]->cend());
		const normalizedSolution_t actualSolution = normalizeSolution(finalSums[i]);
		assert(actualSolution == expectedSolution);
	}
}
#endif

void OptimizedSolution::print(std::ostream &o, const strings_t &functionNames) const
{
	printNegatedInputs(o);
	printProducts(o);
	printSums(o);
	printFinalSums(o, functionNames);
	o << '\n';
	printGateScores(o);
}

OptimizedSolution OptimizedSolution::create(const solutions_t &solutions)
{
	OptimizedSolution os;
	os.createNegatedInputs(solutions);
	const finalPrimeImplicants_t finalPrimeImplicants = os.extractCommonParts(solutions);
	os.extractCommonParts(solutions, finalPrimeImplicants);
	
	os.cleanupProducts();
	os.cleanupSums();
	
#ifndef NDEBUG
	os.validate(solutions);
#endif
	
	return os;
}
