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
	for (index_t i = 0; i != primeImplicants.size(); ++i)
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
std::size_t PetricksMethod<INDEX_T>::calcMaxSums() const
{
	if (options::solutionsLimit.getValue())
		return options::solutionsLimit.getValue();
	else if (::bits > 25)
		return 1;
	else
		return 10000000 / primeImplicants.size();
}

template<std::unsigned_integral INDEX_T>
inline typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::multiplySumsOfProducts(sumOfProducts_t &&multiplier0, const sumOfProducts_t &multiplier1, Progress &progress)
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
		std::ranges::sort(sumOfProducts);
		const auto eraseBegin = std::unique(sumOfProducts.begin(), sumOfProducts.end());
		sumOfProducts.erase(eraseBegin, sumOfProducts.end());
	}
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
	return sumOfProducts;
}

template<std::unsigned_integral INDEX_T>
inline void PetricksMethod<INDEX_T>::multiplySumsOfProducts_maxN(std::vector<CompactSet<index_t>> &multiplier0, const sumOfProducts_t &multiplier1, const std::size_t maxSums)
{
	assert(!multiplier0.empty());
	assert(!multiplier1.empty());
	
	struct SizeInfo
	{
		std::size_t i, j;
		index_t size;
	};
	std::vector<SizeInfo> sizeInfos;
	sizeInfos.reserve(multiplier0.size() * multiplier1.size());
	for (std::size_t i = 0; i != multiplier0.size(); ++i)
	{
		for (std::size_t j = 0; j != multiplier1.size(); ++j)
		{
			const CompactSet<index_t> &product0 = multiplier0[i];
			const product_t &product1 = multiplier1[j];
			sizeInfos.emplace_back(SizeInfo{i, j, static_cast<index_t>(product0.size() + std::ranges::count_if(product1, [product0](const index_t index){ return !product0.contains(index); }))});
		}
	}
	if (sizeInfos.size() > maxSums)
	{
		std::ranges::sort(sizeInfos, std::ranges::less{}, &SizeInfo::size);
		sizeInfos.resize(maxSums);
		std::ranges::sort(sizeInfos, std::ranges::less{}, &SizeInfo::i);
	}
	// `sizeInfos` must be sorted by `i` from this point.
	
	std::vector<std::size_t> mapping;
	{
		mapping.reserve(multiplier0.size());
		for (const SizeInfo &sizeInfo : sizeInfos)
			mapping.push_back(sizeInfo.i);
		const auto eraseBegin = std::unique(mapping.begin(), mapping.end());
		mapping.erase(eraseBegin, mapping.end());
	}
	
	std::vector<std::size_t> reverseMapping(multiplier0.size());
	for (std::size_t i = 0; i != mapping.size(); ++i)
	{
		reverseMapping[mapping[i]] = i;
		if (mapping[i] != i)
			std::ranges::swap(multiplier0[i], multiplier0[mapping[i]]);
	}
	
	assert(multiplier0.size() <= sizeInfos.size());
	while (multiplier0.size() < sizeInfos.size())
		multiplier0.emplace_back(primeImplicants.size());
	
	assert(!sizeInfos.empty());
	for (std::size_t i = sizeInfos.size(); i-- != 0;)
	{
		if (std::size_t mappedPos = reverseMapping[sizeInfos[i].i]; mappedPos != i)
			multiplier0[i] = multiplier0[mappedPos];
		CompactSet<index_t> &product0 = multiplier0[i];
		const product_t &product1 = multiplier1[sizeInfos[i].j];
		assert(!product0.empty());
		assert(!product1.empty());
		for (const index_t index : product1)
			product0.insert(index);
	}
}

template<std::unsigned_integral INDEX_T>
inline void PetricksMethod<INDEX_T>::multiplySumsOfProducts_max1(CompactSet<index_t> &product0, const sumOfProducts_t &multiplier1)
{
	assert(!multiplier1.empty());
	
	std::size_t minAdditionalSize = SIZE_MAX, minAdditionalSizeIndex = 0;
	for (std::size_t i = 0; i != multiplier1.size(); ++i)
	{
		const product_t &product1 = multiplier1[i];
		const std::size_t additionalSize = std::ranges::count_if(product1, [product0 = std::as_const(product0)](const index_t index){ return !product0.contains(index); });
		if (additionalSize < minAdditionalSize)
		{
			minAdditionalSize = additionalSize;
			minAdditionalSizeIndex = i;
			if (additionalSize == 0)
				break;
		}
	}
	
	assert(minAdditionalSize != SIZE_MAX);
	if (minAdditionalSize != 0)
	{
		const product_t &product1 = multiplier1[minAdditionalSizeIndex];
		assert(!product0.empty());
		assert(!product1.empty());
		assert(minAdditionalSize <= product1.size());
		for (const index_t index : product1)
			product0.insert(index);
	}
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::findSumOfProducts_petrick(productOfSumsOfProducts_t productOfSumsOfProducts)
{
	Progress progress(Progress::Stage::SOLVING, "Merging solutions", (productOfSumsOfProducts.size() - 1) * 3, false);
	while (productOfSumsOfProducts.size() != 1)
	{
		sumOfProducts_t multiplier0 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		sumOfProducts_t multiplier1 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		productOfSumsOfProducts.emplace_back(multiplySumsOfProducts(std::move(multiplier0), multiplier1, progress));
	}
	productOfSumsOfProducts.front().shrink_to_fit();
	return std::move(productOfSumsOfProducts.front());
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::findSumOfProducts_limitedPetrick_n(productOfSumsOfProducts_t productOfSumsOfProducts, const std::size_t maxSums)
{
	Progress progress(Progress::Stage::SOLVING, "Merging solutions", productOfSumsOfProducts.size(), false);
	std::vector<CompactSet<index_t>> multiplier0;
	multiplier0.reserve(maxSums);
	{
		progress.step();
		progress.substep(0.0);
		sumOfProducts_t sumOfProducts = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		std::ranges::sort(sumOfProducts, std::ranges::less{}, &product_t::size);
		if (sumOfProducts.size() > maxSums)
			sumOfProducts.resize(maxSums);
		for (const product_t &product : sumOfProducts)
		{
			CompactSet<index_t> &product0 = multiplier0.emplace_back(primeImplicants.size());
			for (const index_t index : product)
				product0.insert(index);
		}
	}
	while (!productOfSumsOfProducts.empty())
	{
		progress.step();
		progress.substep(0.0);
		sumOfProducts_t multiplier1 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		multiplySumsOfProducts_maxN(multiplier0, multiplier1, maxSums);
	}
	sumOfProducts_t sumOfProducts;
	for (const CompactSet<index_t> &product0 : multiplier0)
	{
#ifndef NDEBUG
		product0.validate();
#endif
		product_t &product = sumOfProducts.emplace_back();
		product.reserve(product0.size());
		for (const index_t index : product0)
			product.push_back(index);
	}
	return sumOfProducts;
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::findSumOfProducts_limitedPetrick_1(productOfSumsOfProducts_t productOfSumsOfProducts)
{
	Progress progress(Progress::Stage::SOLVING, "Merging solutions", productOfSumsOfProducts.size(), false);
	CompactSet<index_t> product0(primeImplicants.size());
	{
		progress.step();
		progress.substep(0.0);
		const sumOfProducts_t sumOfProducts = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		const typename sumOfProducts_t::const_iterator smallestProduct = std::ranges::min_element(sumOfProducts, std::ranges::less{}, &product_t::size);
		for (const index_t index : *smallestProduct)
			product0.insert(index);
	}
	while (!productOfSumsOfProducts.empty())
	{
		progress.step();
		progress.substep(0.0);
		sumOfProducts_t multiplier1 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		multiplySumsOfProducts_max1(product0, multiplier1);
	}
	sumOfProducts_t sumOfProducts;
	product_t &product = sumOfProducts.emplace_back();
	product.reserve(product0.size());
#ifndef NDEBUG
	product0.validate();
#endif
	for (const index_t index : product0)
		product.push_back(index);
	return sumOfProducts;
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::findSumOfProducts_limitedPetrick(productOfSumsOfProducts_t productOfSumsOfProducts)
{
	const std::size_t maxSums = calcMaxSums();
	if (maxSums != 1)
		return findSumOfProducts_limitedPetrick_n(productOfSumsOfProducts, maxSums);
	else
		return findSumOfProducts_limitedPetrick_1(productOfSumsOfProducts);
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::findSumOfProducts_greedy(productOfSumsOfProducts_t productOfSumsOfProducts)
{
	Progress progress(Progress::Stage::SOLVING, "Merging solutions (greedy)", 1, false);
	progress.step();
	progress.substep(-0.0, true);
	product_t newProduct;
	newProduct.resize(productOfSumsOfProducts.size());
	for (const sumOfProducts_t &sumOfProducts : productOfSumsOfProducts)
		newProduct.push_back(sumOfProducts.front().front());
	progress.substep(-0.5, true);
	std::ranges::sort(newProduct);
	const auto eraseBegin = std::unique(newProduct.begin(), newProduct.end());
	newProduct.erase(eraseBegin, newProduct.end());
	newProduct.shrink_to_fit();
	productOfSumsOfProducts.clear();
	sumOfProducts_t sumOfProducts{std::move(newProduct)};
	return sumOfProducts;
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::findSumOfProducts(const std::string &functionName)
{
	productOfSumsOfProducts_t productOfSumsOfProducts = createProductOfSums(functionName);
	if (productOfSumsOfProducts.empty())
		return {};
	
	
	switch (options::solutionsHeuristics.getValue())
	{
	case options::SolutionsHeuristic::AUTO:
		{
			const std::size_t magnitudeEstimation = std::accumulate<decltype(productOfSumsOfProducts.cbegin()), std::size_t>(productOfSumsOfProducts.cbegin(), productOfSumsOfProducts.cend(), 0, [](std::size_t x, const sumOfProducts_t &y){ return x + y.size(); }) - productOfSumsOfProducts.size();
			if (magnitudeEstimation <= 63)
				return findSumOfProducts_petrick(std::move(productOfSumsOfProducts));
			else if (::bits <= 27)
				return findSumOfProducts_limitedPetrick(std::move(productOfSumsOfProducts));
			else
				return findSumOfProducts_greedy(std::move(productOfSumsOfProducts));
		}
	case options::SolutionsHeuristic::PETRICK:
		return findSumOfProducts_petrick(std::move(productOfSumsOfProducts));
	case options::SolutionsHeuristic::LIMITED_PETRICK:
		return findSumOfProducts_limitedPetrick(std::move(productOfSumsOfProducts));
	case options::SolutionsHeuristic::GREEDY:
		return findSumOfProducts_greedy(std::move(productOfSumsOfProducts));
	}
	
	// Unreachable
	return {};
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
