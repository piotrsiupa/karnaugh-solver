#include "./PetricksMethod.hh"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>


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
PrimeImplicants PetricksMethod<INDEX_T>::extractEssentials()
{
	PrimeImplicants essentials;
	for (typename minterms_t::const_iterator iter = minterms.cbegin(); iter != minterms.cend();)
	{
		const index_t essentialPrimeImplicantIndex = findEssentialPrimeImplicantIndex(*iter);
		if (essentialPrimeImplicantIndex == NO_INDEX)
		{
			++iter;
			continue;
		}
		essentials.emplace_back(std::move(primeImplicants[essentialPrimeImplicantIndex]));
		primeImplicants.erase(primeImplicants.begin() + essentialPrimeImplicantIndex);
		const PrimeImplicant &primeImplicant = essentials.back();
		for (typename minterms_t::const_iterator jiter = minterms.cbegin(); jiter != iter;)
			if (primeImplicant.covers(*jiter))
				jiter = minterms.erase(jiter);
			else
				++jiter;
		for (typename minterms_t::const_iterator jiter = std::next(iter); jiter != minterms.cend();)
			if (primeImplicant.covers(*jiter))
				jiter = minterms.erase(jiter);
			else
				++jiter;
		iter = minterms.erase(iter);
	}
	return essentials;
}

template<typename INDEX_T>
typename PetricksMethod<INDEX_T>::productOfSumsOfProducts_t PetricksMethod<INDEX_T>::createPreliminaryProductOfSums() const
{
	productOfSumsOfProducts_t productOfSums;
	for (const Minterm &minterm : minterms)
	{
		sumOfProducts_t &sum = productOfSums.emplace_back();
		for (index_t i = 0; i != primeImplicants.size(); ++i)
			if (primeImplicants[i].covers(minterm))
				sum.emplace_back().push_back(i);
	}
	return productOfSums;
}

template<typename INDEX_T>
void PetricksMethod<INDEX_T>::removeRedundantSums(productOfSumsOfProducts_t &productOfSums)
{
	for (auto x = productOfSums.begin(); x != productOfSums.end(); ++x)
	{
		if (!x->empty())
		{
			for (auto y = std::next(x); y != productOfSums.end(); ++y)
			{
				if (!y->empty())
				{
					if (std::includes(x->cbegin(), x->cend(), y->cbegin(), y->cend()))
					{
						x->clear();
						break;
					}
					else if (std::includes(y->cbegin(), y->cend(), x->cbegin(), x->cend()))
					{
						y->clear();
					}
				}
			}
		}
	}
	productOfSums.erase(std::remove_if(productOfSums.begin(), productOfSums.end(), [](const auto &x){ return x.empty(); }), productOfSums.end());
}

template<typename INDEX_T>
typename PetricksMethod<INDEX_T>::productOfSumsOfProducts_t PetricksMethod<INDEX_T>::createProductOfSums() const
{
	productOfSumsOfProducts_t productOfSums = createPreliminaryProductOfSums();
	removeRedundantSums(productOfSums);
	return productOfSums;
}

template<typename INDEX_T>
inline typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::multiplySumsOfProducts(const sumOfProducts_t &multiplier0, const sumOfProducts_t &multiplier1, long double &actualOperations, const long double expectedOperations, Progress &progress)
{
	product_t newProduct;
	newProduct.reserve(multiplier0.front().size() + multiplier1.front().size());
	HasseDiagram<index_t> hasseDiagram;
	const auto estimateCompletion = [&actualOperations = std::as_const(actualOperations), expectedOperations](){ return static_cast<Progress::completion_t>(actualOperations / expectedOperations); };
	{
		Progress::SubtaskGuard progressSubtask = progress.enterSubtask("expanding");
		std::size_t operationsThisTime = 0;
		for (const product_t &x : multiplier0)
		{
			for (const product_t &y : multiplier1)
			{
				progress.substep(estimateCompletion, operationsThisTime == 0);
				++operationsThisTime;
				newProduct.clear();
				std::set_union(x.cbegin(), x.cend(), y.cbegin(), y.cend(), std::back_inserter(newProduct));
				hasseDiagram.insertRemovingSupersets(std::move(newProduct));
			}
		}
		actualOperations += operationsThisTime;
	}
	{	
		Progress::SubtaskGuard progressSubtask = progress.enterSubtask("refining");
		progress.substep(estimateCompletion, true);
		return hasseDiagram.getSets();
	}
}

template<typename INDEX_T>
std::string PetricksMethod<INDEX_T>::ld2integerString(const long double value)
{
	std::stringstream ss;
	ss << std::fixed << std::setprecision(0) << value;
	return ss.str();
}

template<typename INDEX_T>
typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::findSumOfProducts(Progress &progress) const
{
	productOfSumsOfProducts_t productOfSumsOfProducts = createProductOfSums();
	if (productOfSumsOfProducts.empty())
		return sumOfProducts_t{};
	
	char progressInfo[128] = ""; // 128 should be enough even if the number is huge.
	long double actualOperations = 0.0, expectedOperations = 0.0, expectedSolutions = 0.0;
	Progress::SubtaskGuard progressSubtask = progress.enterSubtask(progressInfo);
	
	while (productOfSumsOfProducts.size() != 1)
	{
		if (::terminalStderr)
		{
			expectedSolutions = 0.0;
			expectedSolutions = 0.0;
			expectedSolutions = static_cast<long double>(productOfSumsOfProducts.back().size());
			for (auto iter = std::next(productOfSumsOfProducts.crbegin()); iter != productOfSumsOfProducts.crend(); ++iter)
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
		if (::terminalStderr)
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
typename PetricksMethod<INDEX_T>::solutions_t PetricksMethod<INDEX_T>::solve(Progress &progress)
{
	PrimeImplicants essentials = extractEssentials();
	sumOfProducts_t sumOfProducts = findSumOfProducts(progress);
	
	if (sumOfProducts.empty())
		return !essentials.empty()
			? solutions_t{std::move(essentials)}
			: solutions_t{{PrimeImplicant::error()}};
	
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
