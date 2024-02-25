#include "./PetricksMethod.hh"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

#include "Implicant.hh"
#include "options.hh"


template<std::unsigned_integral INDEX_T>
Implicants PetricksMethod<INDEX_T>::extractEssentials(const std::string &functionName)
{
	const std::string progressName = "Extracting essentials of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 1);
	progress.step();
	auto progressStep = progress.makeCountingStepHelper(primeImplicants.size() * 2);
	
	Minterms multiple;
	{
		Minterms single = ~*minterms;
		for (const Implicant &primeImplicant : primeImplicants)
		{
			progressStep.substep();
			for (const Minterm minterm : primeImplicant)
				if (single.contains(minterm))
					multiple.insert(minterm);
				else
					single.insert(minterm);
		}
#ifndef NDEBUG
		single.validate();
		multiple.validate();
#endif
	}
	
	Implicants essentials;
	{
		auto iter = primeImplicants.begin();
		for (; iter != primeImplicants.cend(); ++iter)
		{
			progressStep.substep();
			if (!iter->areAllInMinterms(multiple))
				break;
		}
		auto jiter = iter;
		for (; iter != primeImplicants.cend(); ++iter)
		{
			progressStep.substep();
			if (!iter->areAllInMinterms(multiple))
			{
				iter->removeFromMinterms(*minterms);
				essentials.emplace_back(std::move(*iter));
				continue;
			}
			*(jiter++) = std::move(*iter);
		}
		primeImplicants.erase(jiter, primeImplicants.end());
		primeImplicants.shrink_to_fit();
		essentials.shrink_to_fit();
	}
	
#ifndef NDEBUG
	minterms->validate();
#endif
	return essentials;
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::productOfSums_t PetricksMethod<INDEX_T>::createPreliminaryProductOfSums(const std::string &functionName) const
{
	const std::string progressName = "Creating initial solution space for \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 2, false);
	
	progress.step();
	progress.substep(-0.0, true);
	
	std::vector<Minterm> mintermMap;
	mintermMap.reserve(minterms->size());
	for (const Minterm minterm : *minterms)
		mintermMap.push_back(minterm);
	
	progress.step();
	auto progressStep = progress.makeCountingStepHelper(primeImplicants.size());
	
	productOfSums_t productOfSums(mintermMap.size());
	for (std::size_t i = 0; i != primeImplicants.size(); ++i)
	{
		progressStep.substep();
		const Implicant &primeImplicant = primeImplicants[i];
		for (const Minterm minterm : primeImplicant)
		{
			if (minterms->contains(minterm))
			{
				const auto foundMinterm = std::lower_bound(mintermMap.cbegin(), mintermMap.cend(), minterm);
				const std::size_t index = std::distance(mintermMap.cbegin(), foundMinterm);
				productOfSums[index].push_back(i);
			}
		}
	}
	
#ifndef NDEBUG
	for (const sum_t &sum : productOfSums)
		assert(sum.size() >= 2);
#endif
	
	return productOfSums;
}

template<std::unsigned_integral INDEX_T>
void PetricksMethod<INDEX_T>::removeRedundantSums(productOfSums_t &productOfSums, const std::string &functionName)
{
	const std::string progressName = "Cleaning up solution space for \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 2, false);
	
	{
		const auto infoGuard = progress.addInfo("sorting and deduplicating solutions");
		progress.step();
		progress.substep(-0.0, true);
		std::sort(productOfSums.begin(), productOfSums.end());
		progress.substep(-0.5, true);
		productOfSums.erase(std::unique(productOfSums.begin(), productOfSums.end()), productOfSums.end());
	}
	
	progress.step();
	auto progressStep = progress.makeCountingStepHelper(productOfSums.size());
	
	for (auto x = productOfSums.begin(); x != productOfSums.end(); ++x)
	{
		progressStep.substep();
		for (auto subX = x->cbegin(); subX != std::prev(x->cend()); ++subX)
		{
			const auto rangeBegin = std::lower_bound(productOfSums.cbegin(), productOfSums.cend(), *subX, [](const sum_t &sum, const INDEX_T value){ return sum.front() < value; });
			const auto rangeEnd = std::upper_bound(rangeBegin, productOfSums.cend(), *subX, [](const INDEX_T value, const sum_t &sum){ return sum.front() != value; });
			for (auto y = rangeBegin; y != rangeEnd; ++y)
			{
				if (x == y || y->size() == 1)
					continue;
				if (std::ranges::includes(*x, *y))
				{
					x->resize(1);
					goto next_x;
				}
			}
		}
		next_x:;
	}
	const auto eraseBegin = std::remove_if(productOfSums.begin(), productOfSums.end(), [](const auto &x){ return x.size() == 1; });
	productOfSums.erase(eraseBegin, productOfSums.end());
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::productOfSumsOfProducts_t PetricksMethod<INDEX_T>::createProductOfSums(const std::string &functionName)
{
	productOfSums_t productOfSums = createPreliminaryProductOfSums(functionName);
	minterms.reset();
	removeRedundantSums(productOfSums, functionName);
	productOfSumsOfProducts_t productOfSumsOfProducts;
	productOfSumsOfProducts.reserve(productOfSums.size());
	for (const sum_t &sum : productOfSums)
	{
		sumOfProducts_t &sumOfProducts = productOfSumsOfProducts.emplace_back();
		for (const index_t &index : sum)
			sumOfProducts.emplace_back().push_back(index);
	}
	return productOfSumsOfProducts;
}

template<std::unsigned_integral INDEX_T>
std::size_t PetricksMethod<INDEX_T>::calcMaxSums()
{
	switch (options::solutionsHeuristics.getValue())
	{
	case options::SolutionsHeuristic::PETRICK:
		return 0;
	case options::SolutionsHeuristic::LIMITED_PETRICK:
		return options::solutionsLimit.getValue() == 0 ? 256 : static_cast<std::size_t>(options::solutionsLimit.getValue());
	}
	// Unreachable
	return 0;
}

template<std::unsigned_integral INDEX_T>
inline typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::multiplySumsOfProducts(sumOfProducts_t &&multiplier0, sumOfProducts_t &&multiplier1, const std::size_t maxSums, Progress &progress)
{
	sumOfProducts_t sumOfProducts;
	sumOfProducts.reserve(multiplier0.size() * multiplier1.size());
	{
		const auto infoGuard = progress.addInfo("expanding");
		progress.step();
		auto progressStep = progress.makeCountingStepHelper(static_cast<std::uintmax_t>(multiplier0.size()) * static_cast<std::uintmax_t>(multiplier1.size())); // If this overflows, there is no hope on finishing the calculations in reasonable time anyway.
		for (const product_t &x : multiplier0)
		{
			for (const product_t &y : multiplier1)
			{
				progressStep.substep();
				product_t &newProduct = sumOfProducts.emplace_back();
				newProduct.reserve(x.size() * y.size());
				std::ranges::set_union(x, y, std::back_inserter(newProduct));
			}
		}
	}
	{
		const auto infoGuard = progress.addInfo("sorting");
		progress.step();
		progress.substep(-0.0);
		if (maxSums == 0)
			std::ranges::sort(sumOfProducts);
		else
			std::ranges::sort(sumOfProducts, [](const product_t &x, const product_t &y){ return x.size() < y.size() || (x.size() == y.size() && x < y); });
		const auto eraseBegin = std::unique(sumOfProducts.begin(), sumOfProducts.end());
		sumOfProducts.erase(eraseBegin, sumOfProducts.end());
	}
	if (maxSums == 0)
	{
		const auto infoGuard = progress.addInfo("reducing");
		progress.step();
		auto progressStep = progress.makeCountingStepHelper(sumOfProducts.size());
		for (auto x = sumOfProducts.begin(); x != sumOfProducts.end(); ++x)
		{
			progressStep.substep();
			if (x->size() <= 2)
				continue;
			for (auto value = x->cbegin(); value != x->cend(); ++value)
			{
				const auto rangeBegin = std::lower_bound(sumOfProducts.cbegin(), sumOfProducts.cend(), *value, [](const sum_t &sum, const INDEX_T value){ return sum.front() < value; });
				const auto rangeEnd = std::upper_bound(rangeBegin, sumOfProducts.cend(), *value, [](const INDEX_T value, const sum_t &sum){ return sum.front() != value; });
				for (auto y = rangeBegin; y != rangeEnd; ++y)
				{
					if (y->size() == 1)
						continue;
					if (x->size() > y->size() && std::ranges::includes(*x, *y))
					{
						x->resize(1);
						goto next_x;
					}
				}
			}
			next_x:;
		}
		sumOfProducts.erase(std::remove_if(sumOfProducts.begin(), sumOfProducts.end(), [](const auto &x){ return x.size() == 1; }), sumOfProducts.end());
	}
	else
	{
		const auto infoGuard = progress.addInfo("limiting");
		progress.step();
		progress.substep(-0.0);
		if (sumOfProducts.size() > maxSums)
			sumOfProducts.resize(maxSums);
	}
	return sumOfProducts;
}

template<std::unsigned_integral INDEX_T>
inline typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::multiplySumsOfProducts_max1(sumOfProducts_t &&multiplier0, sumOfProducts_t &&multiplier1)
{
	assert(!multiplier0.empty());
	assert(!multiplier1.empty());
	
	std::size_t minAdditionalSize = SIZE_MAX, minAdditionalSizeIndex = 0;
	product_t &product0 = multiplier0.front();
	for (std::size_t i = 0; i != multiplier1.size(); ++i)
	{
		const product_t &product1 = multiplier1[i];
		const std::size_t additionalSize = std::ranges::count_if(product1, [product0 = std::as_const(product0)](const index_t index){ return !std::ranges::binary_search(product0, index); });
		if (additionalSize < minAdditionalSize)
		{
			if (additionalSize == 0)
				return std::move(multiplier0);
			minAdditionalSize = additionalSize;
			minAdditionalSizeIndex = i;
		}
	}
	assert(minAdditionalSize != SIZE_MAX);
	if (minAdditionalSize != 0)
	{
		const product_t &product1 = multiplier1[minAdditionalSizeIndex];
		assert(!product0.empty());
		assert(!product1.empty());
		assert(minAdditionalSize <= product1.size());
		std::size_t i = product0.size() - 1;
		std::size_t j = product1.size() - 1;
		product0.resize(product0.size() + minAdditionalSize);
		index_t val = product1[j];
		for (std::size_t k = product0.size() - 1;; --k)
		{
			if (product0[i] < val)
			{
				product0[k] = val;
				if (j == 0)
					break;
				if (product0[i] == val)
					if (--i == SIZE_MAX)
						break;
				val = product1[--j];
			}
			else
			{
				product0[k] = product0[i];
				if (--i == SIZE_MAX)
					break;
			}
		}
		if (i == SIZE_MAX)
			do
				product0[j] = product1[j];
			while (j-- != 0);
	}
	return std::move(multiplier0);
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::findSumOfProducts(const std::string &functionName)
{
	productOfSumsOfProducts_t productOfSumsOfProducts = createProductOfSums(functionName);
	if (productOfSumsOfProducts.empty())
		return sumOfProducts_t{};
	
	
	const std::size_t maxSums = calcMaxSums();
	if (maxSums == 1)
	{
		Progress progress(Progress::Stage::SOLVING, "Merging solutions", productOfSumsOfProducts.size() - 1, false);
		{
			sumOfProducts_t &sumOfProducts = productOfSumsOfProducts.back();
			const typename sumOfProducts_t::iterator smallestProduct = std::ranges::min_element(sumOfProducts, [](const product_t &x, const product_t &y){ return x.size() < y.size(); });
			if (smallestProduct != sumOfProducts.begin())
				sumOfProducts.front() = std::move(*smallestProduct);
			sumOfProducts.resize(1);
			sumOfProducts.shrink_to_fit();
			sumOfProducts.front().reserve(primeImplicants.size());
		}
		while (productOfSumsOfProducts.size() != 1)
		{
			progress.step();
			progress.substep(0.0);
			sumOfProducts_t multiplier0 = std::move(productOfSumsOfProducts.back());
			productOfSumsOfProducts.pop_back();
			sumOfProducts_t multiplier1 = std::move(productOfSumsOfProducts.back());
			productOfSumsOfProducts.pop_back();
			productOfSumsOfProducts.emplace_back(multiplySumsOfProducts_max1(std::move(multiplier0), std::move(multiplier1)));
		}
	}
	else
	{
		Progress progress(Progress::Stage::SOLVING, "Merging solutions", (productOfSumsOfProducts.size() - 1) * 3, false);
		if (maxSums != 0 && productOfSumsOfProducts.back().size() > maxSums)
		{
			std::ranges::sort(productOfSumsOfProducts.back(), [](const product_t &x, const product_t &y){ return x.size() < y.size() || (x.size() == y.size() && x < y); });
			productOfSumsOfProducts.back().resize(maxSums);
		}
		while (productOfSumsOfProducts.size() != 1)
		{
			sumOfProducts_t multiplier0 = std::move(productOfSumsOfProducts.back());
			productOfSumsOfProducts.pop_back();
			sumOfProducts_t multiplier1 = std::move(productOfSumsOfProducts.back());
			productOfSumsOfProducts.pop_back();
			productOfSumsOfProducts.emplace_back(multiplySumsOfProducts(std::move(multiplier0), std::move(multiplier1), maxSums, progress));
		}
	}
	
	return std::move(productOfSumsOfProducts.front());
}

template<std::unsigned_integral INDEX_T>
Solutions PetricksMethod<INDEX_T>::solve(const std::string &functionName) &&
{
	Implicants essentials = extractEssentials(functionName);
	sumOfProducts_t sumOfProducts = findSumOfProducts(functionName);
	
	if (sumOfProducts.empty())
		return !essentials.empty()
			? Solutions{std::move(essentials)}
			: Solutions{{Implicant::none()}};
	
	Solutions solutions;
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
