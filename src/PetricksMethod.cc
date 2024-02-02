#include "./PetricksMethod.hh"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>

#include "options.hh"


template<typename INDEX_T>
typename PetricksMethod<INDEX_T>::index_t PetricksMethod<INDEX_T>::findEssentialPrimeImplicantIndex(const Minterm minterm)
{
	std::uint_fast8_t count = 0;
	assert(!primeImplicants.empty());
	index_t index = 0;  // Initialized only to avoid a compiler warning.
	for (index_t i = 0; i != primeImplicants.size(); ++i)
	{
		if (primeImplicants[i].covers(minterm))
		{
			if (++count == 2)
				return NO_INDEX;
			index = i;
		}
	}
	return count != 0 ? index : NO_INDEX;
}

template<typename INDEX_T>
Implicants PetricksMethod<INDEX_T>::extractEssentials(const std::string &functionName)
{
	const std::string progressName = "Extracting essentials of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 1);
	progress.step();
	auto progressStep = progress.makeCountingStepHelper(minterms->size());
	
	Implicants essentials;
	for (const Minterm minterm : *minterms)
	{
		progressStep.substep();
		const index_t essentialPrimeImplicantIndex = findEssentialPrimeImplicantIndex(minterm);
		if (essentialPrimeImplicantIndex == NO_INDEX)
			continue;
		essentials.emplace_back(std::move(primeImplicants[essentialPrimeImplicantIndex]));
		primeImplicants.erase(primeImplicants.begin() + essentialPrimeImplicantIndex);
		const Implicant &primeImplicant = essentials.back();
		for (const Minterm minterm1 : *minterms)
			if (primeImplicant.covers(minterm1))
				minterms->remove(minterm1);
	}
#ifndef NDEBUG
	minterms->validate();
#endif
	return essentials;
}

template<typename INDEX_T>
typename PetricksMethod<INDEX_T>::productOfSumsOfProducts_t PetricksMethod<INDEX_T>::createPreliminaryProductOfSums(const std::string &functionName) const
{
	const std::string progressName = "Creating initial solution space for \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 1);
	progress.step();
	auto progressStep = progress.makeCountingStepHelper(minterms->size());
	productOfSumsOfProducts_t productOfSums;
	for (const Minterm &minterm : *minterms)
	{
		progressStep.substep();
		sumOfProducts_t &sum = productOfSums.emplace_back();
		for (index_t i = 0; i != primeImplicants.size(); ++i)
			if (primeImplicants[i].covers(minterm))
				sum.emplace_back().push_back(i);
	}
	return productOfSums;
}

template<typename INDEX_T>
void PetricksMethod<INDEX_T>::removeRedundantSums(productOfSumsOfProducts_t &productOfSums, const std::string &functionName)
{
	const std::string progressName = "Cleaning up solution space for \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 1);
	progress.step();
	auto progressStep = progress.makeCountingStepHelper(productOfSums.size());
	for (auto x = productOfSums.begin(); x != productOfSums.end(); ++x)
	{
		progressStep.substep();
		if (!x->empty())
		{
			for (auto y = std::ranges::next(x); y != productOfSums.end(); ++y)
			{
				if (!y->empty())
				{
					if (std::ranges::includes(*x, *y))
					{
						x->clear();
						break;
					}
					else if (std::ranges::includes(*y, *x))
					{
						y->clear();
					}
				}
			}
		}
	}
	const auto eraseBegin = std::remove_if(productOfSums.begin(), productOfSums.end(), [](const auto &x){ return x.empty(); });
	productOfSums.erase(eraseBegin, productOfSums.end());
}

template<typename INDEX_T>
typename PetricksMethod<INDEX_T>::productOfSumsOfProducts_t PetricksMethod<INDEX_T>::createProductOfSums(const std::string &functionName)
{
	productOfSumsOfProducts_t productOfSums = createPreliminaryProductOfSums(functionName);
	minterms.reset();
	removeRedundantSums(productOfSums, functionName);
	return productOfSums;
}

template<typename INDEX_T>
inline typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::multiplySumsOfProducts(const sumOfProducts_t &multiplier0, const sumOfProducts_t &multiplier1, long double &actualOperations, const long double expectedOperations, Progress &progress)
{
	product_t newProduct;
	newProduct.reserve(multiplier0.front().size() + multiplier1.front().size());
	HasseDiagram<index_t> hasseDiagram;
	{
		std::size_t operationsThisTime = 0;
		const auto estimateCompletion = [actualOperations, &operationsThisTime = std::as_const(operationsThisTime), expectedOperations](){ return static_cast<Progress::completion_t>((actualOperations + operationsThisTime) / expectedOperations); };
		const auto infoGuard = progress.addInfo("expanding");
		for (const product_t &x : multiplier0)
		{
			for (const product_t &y : multiplier1)
			{
				progress.substep(estimateCompletion, operationsThisTime == 0);
				++operationsThisTime;
				newProduct.clear();
				std::ranges::set_union(x, y, std::back_inserter(newProduct));
				hasseDiagram.insertRemovingSupersets(std::move(newProduct));
			}
		}
		actualOperations += operationsThisTime;
	}
	{	
		const auto estimateCompletion = [&actualOperations = std::as_const(actualOperations), expectedOperations](){ return static_cast<Progress::completion_t>(actualOperations / expectedOperations); };
		const auto infoGuard = progress.addInfo("refining");
		progress.substep(estimateCompletion, true);
		return hasseDiagram.getSets();
	}
}

template<typename INDEX_T>
std::string PetricksMethod<INDEX_T>::ld2integerString(const long double value)
{
	if (value >= 1'000'000'000'000'000'000'000.0)
		return "infinity (or close enough)";
	std::stringstream ss;
	ss << std::fixed << std::setprecision(0) << value;
	return ss.str();
}

template<typename INDEX_T>
typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::findSumOfProducts(const std::string &functionName)
{
	productOfSumsOfProducts_t productOfSumsOfProducts = createProductOfSums(functionName);
	if (productOfSumsOfProducts.empty())
		return sumOfProducts_t{};
	
	const std::string progressName = "Solving \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 1);
	char progressInfo[128] = ""; // 128 should be enough even if the number is huge.
	long double actualOperations = 0.0, expectedOperations = 0.0, expectedSolutions = 0.0;
	const auto infoGuard = progress.addInfo(progressInfo);
	progress.step();
	
	while (productOfSumsOfProducts.size() != 1)
	{
		if (progress.isVisible())
		{
			expectedSolutions = 0.0;
			expectedSolutions = 0.0;
			expectedSolutions = static_cast<long double>(productOfSumsOfProducts.back().size());
			for (auto iter = std::ranges::next(productOfSumsOfProducts.crbegin()); iter != productOfSumsOfProducts.crend(); ++iter)
			{
				expectedSolutions *= iter->size();
				expectedOperations += expectedSolutions;
			}
			std::strcpy(progressInfo, ld2integerString(expectedSolutions).c_str());
			std::strcat(progressInfo, " solutions");
		}
		sumOfProducts_t multiplier0 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		sumOfProducts_t multiplier1 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		if (progress.isVisible())
		{
			const std::uintmax_t expectedResultSize = static_cast<std::uintmax_t>(multiplier0.size()) * static_cast<std::uintmax_t>(multiplier1.size());
			productOfSumsOfProducts.emplace_back(multiplySumsOfProducts(std::move(multiplier0), std::move(multiplier1), actualOperations, expectedOperations, progress));
			const std::size_t actualResultSize = productOfSumsOfProducts.back().size();
			expectedOperations = (expectedOperations - actualOperations) * (static_cast<long double>(actualResultSize) / static_cast<long double>(expectedResultSize)) + actualOperations;
			expectedSolutions = expectedSolutions / expectedResultSize * actualResultSize;
		}
		else
		{
			productOfSumsOfProducts.emplace_back(multiplySumsOfProducts(std::move(multiplier0), std::move(multiplier1), actualOperations, expectedOperations, progress));
		}
	}
	
	return std::move(productOfSumsOfProducts.front());
}

template<typename INDEX_T>
typename PetricksMethod<INDEX_T>::solutions_t PetricksMethod<INDEX_T>::solve(const std::string &functionName)
{
	Implicants essentials = extractEssentials(functionName);
	sumOfProducts_t sumOfProducts = findSumOfProducts(functionName);
	
	if (sumOfProducts.empty())
		return !essentials.empty()
			? solutions_t{std::move(essentials)}
			: solutions_t{{Implicant::none()}};
	
	solutions_t solutions;
	solutions.reserve(sumOfProducts.size());
	for (const auto &x : sumOfProducts)
	{
		solutions.emplace_back();
		solutions.back().reserve(essentials.size() + x.size());
		solutions.back().insert(solutions.back().end(), essentials.begin(), essentials.end());
		for (const index_t &index : x)
			solutions.back().push_back(primeImplicants[index]);
	}
	return solutions;
}


template class PetricksMethod<std::uint8_t>;
template class PetricksMethod<std::uint16_t>;
template class PetricksMethod<std::uint32_t>;
template class PetricksMethod<std::uint64_t>;
