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

void OptimizedSolution::initializeWips(const solutions_t &solutions, wipProducts_t &wipProducts, wipSums_t &wipSums, wipFinalSums_t &wipFinalSums)
{
	wipFinalSums.reserve(solutions.size());
	for (const PrimeImplicants *const solution : solutions)
	{
		std::set<ref_t> wipSum;
		for (const auto &x : *solution)
			wipSum.insert(&wipProducts[x]);
		wipFinalSums.push_back(&wipSums[std::move(wipSum)]);
	}
}

void OptimizedSolution::extractCommonParts(wipProducts_t &wipProducts)
{
	std::vector<PrimeImplicant> oldPrimeImplicants;
	oldPrimeImplicants.reserve(wipProducts.size());
	for (const auto &wipProduct : wipProducts)
		oldPrimeImplicants.push_back(wipProduct.first);
	const auto [newPrimeImplicants, chosenSubsets] = SetOptimizerForProducts().extractCommonParts(oldPrimeImplicants);
	
	std::vector<ref_t> refsrefs(chosenSubsets.size());
	for (std::size_t i = 0; i != chosenSubsets.size(); ++i)
	{
		auto &refs = wipProducts[newPrimeImplicants[i]];
		refs.reserve(chosenSubsets[i].size());
		for (const std::size_t &subset : chosenSubsets[i])
			refs.push_back(refsrefs[subset]);
		refsrefs[i] = &refs;
	}
}

void OptimizedSolution::extractCommonParts(wipSums_t &wipSums)
{
	std::vector<std::set<const void*>> oldPointerSets;
	oldPointerSets.reserve(wipSums.size());
	for (const auto &wipSum : wipSums)
		oldPointerSets.push_back(wipSum.first);
	const auto [newPointerSets, chosenSubsets] = SetOptimizerForSums().extractCommonParts(oldPointerSets);
	
	std::vector<ref_t> refsrefs(chosenSubsets.size());
	for (std::size_t i = 0; i != chosenSubsets.size(); ++i)
	{
		auto &refs = wipSums[newPointerSets[i]];
		refs.reserve(chosenSubsets[i].size());
		for (const std::size_t &subset : chosenSubsets[i])
			refs.push_back(refsrefs[subset]);
		refsrefs[i] = &refs;
	}
}

void OptimizedSolution::createNegatedInputs(const solutions_t &solutions)
{
	for (const PrimeImplicants *const solution : solutions)
		for (const auto &x : *solution)
			negatedInputs |= x.getFalseBits();
}

OptimizedSolution::id_t OptimizedSolution::findWipProductId(const wipProducts_t &wipProducts, const ref_t wipProductRef)
{
	std::size_t i = 0;
	for (const auto &wipProduct : wipProducts)
	{
		if (&wipProduct.second == wipProductRef)
			return makeProductId(i);
		++i;
	}
	__builtin_unreachable();
}

OptimizedSolution::id_t OptimizedSolution::findWipSumId(const wipSums_t &wipSums, const ref_t wipSumRef) const
{
	std::size_t i = 0;
	for (const auto &wipSum : wipSums)
	{
		if (&wipSum.second == wipSumRef)
			return makeSumId(i);
		++i;
	}
	__builtin_unreachable();
}

void OptimizedSolution::insertWipProducts(const wipProducts_t &wipProducts)
{
	products.reserve(wipProducts.size());
	for (const auto &wipProduct : wipProducts)
		products.push_back({wipProduct.first, {}});
	std::size_t i = 0;
	for (const auto &wipProduct : wipProducts)
	{
		auto &ids = products[i++].second;
		for (auto wipProductRefIter = wipProduct.second.crbegin(); wipProductRefIter != wipProduct.second.crend(); ++wipProductRefIter)
			ids.push_back(findWipProductId(wipProducts, *wipProductRefIter));
	}
}

void OptimizedSolution::insertWipSums(const wipProducts_t &wipProducts, const wipSums_t &wipSums)
{
	sums.reserve(wipSums.size());
	for (const auto &wipSum : wipSums)
	{
		sums.emplace_back();
		auto &sum = sums.back();
		sum.reserve(wipSum.first.size() + wipSum.second.size());
		for (const auto &wipProductRef : wipSum.first)
			sum.push_back(findWipProductId(wipProducts, wipProductRef));
		for (auto wipSumRefIter = wipSum.second.crbegin(); wipSumRefIter != wipSum.second.crend(); ++wipSumRefIter)
			sum.push_back(findWipSumId(wipSums, *wipSumRefIter));
	}
}

void OptimizedSolution::insertWipFinalSums(const wipSums_t &wipSums, const wipFinalSums_t &wipFinalSums)
{
	finalSums.reserve(wipFinalSums.size());
	for (ref_t wipFinalSum : wipFinalSums)
		finalSums.push_back(findWipSumId(wipSums, wipFinalSum));
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
	wipProducts_t wipProducts;
	wipSums_t wipSums(wipSumsLess);
	wipFinalSums_t wipFinalSums;
	initializeWips(solutions, wipProducts, wipSums, wipFinalSums);
	
	extractCommonParts(wipProducts);
	extractCommonParts(wipSums);
	
	OptimizedSolution os;
	
	os.createNegatedInputs(solutions);
	os.insertWipProducts(wipProducts);
	os.insertWipSums(wipProducts, wipSums);
	os.insertWipFinalSums(wipSums, wipFinalSums);
	
	os.cleanupProducts();
	os.cleanupSums();
	
#ifndef NDEBUG
	os.validate(solutions);
#endif
	
	return os;
}
