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
inline typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::multiplySumsOfProducts(const sumOfProducts_t &multiplier0, const sumOfProducts_t &multiplier1, Progress &progress)
{
	product_t newProduct;
	HasseDiagram<index_t> hasseDiagram;
	{
		const auto infoGuard = progress.addInfo("expanding");
		progress.step();
		const std::size_t expectedOperations = multiplier0.size() * multiplier1.size();
		std::size_t operations = 0;
		const auto estimateCompletion = [expectedOperations, &operations = std::as_const(operations)](){ return static_cast<Progress::completion_t>(operations) / static_cast<Progress::completion_t>(expectedOperations); };
		newProduct.reserve(multiplier0.front().size() + multiplier1.front().size());
		for (const product_t &x : multiplier0)
		{
			for (const product_t &y : multiplier1)
			{
				progress.substep(estimateCompletion);
				++operations;
				newProduct.clear();
				std::ranges::set_union(x, y, std::back_inserter(newProduct));
				hasseDiagram.insertRemovingSupersets(std::move(newProduct));
			}
		}
	}
	{	
		const auto infoGuard = progress.addInfo("refining");
		progress.step();
		progress.substep(-0.0, true);
		return hasseDiagram.getSets();
	}
}

template<std::unsigned_integral INDEX_T>
std::string PetricksMethod<INDEX_T>::solutionCountEstimation(const long double value)
{
	if (value >= 1'000'000'000'000'000'000'000.0)
		return "infinity (or close enough)";
	std::stringstream ss;
	ss << "up to " << std::fixed << std::setprecision(0) << value;
	return ss.str();
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::sumOfProducts_t PetricksMethod<INDEX_T>::findSumOfProducts(const std::string &functionName)
{
	productOfSumsOfProducts_t productOfSumsOfProducts = createProductOfSums(functionName);
	if (productOfSumsOfProducts.empty())
		return sumOfProducts_t{};
	
	const std::string progressName = "Solving \"" + functionName + '"';
	char progressInfo[128] = ""; // 128 should be enough even if the number is huge.
	std::strcpy(progressInfo, progressName.c_str());
	Progress progress(Progress::Stage::SOLVING, progressInfo, (productOfSumsOfProducts.size() - 1) * 2, false);
	long double expectedSolutions = 0.0;
	
	if (progress.isVisible())
	{
		expectedSolutions = 0.0;
		expectedSolutions = 0.0;
		expectedSolutions = static_cast<long double>(productOfSumsOfProducts.back().size());
		for (auto iter = std::ranges::next(productOfSumsOfProducts.crbegin()); iter != productOfSumsOfProducts.crend(); ++iter)
			expectedSolutions *= iter->size();
	}
	
	while (productOfSumsOfProducts.size() != 1)
	{
		if (progress.isVisible())
		{
			std::strcpy(progressInfo, progressName.c_str());
			std::strcat(progressInfo, " - ");
			std::strcat(progressInfo, solutionCountEstimation(expectedSolutions).c_str());
			std::strcat(progressInfo, " solutions");
		}
		sumOfProducts_t multiplier0 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		sumOfProducts_t multiplier1 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		if (progress.isVisible())
		{
			const std::uintmax_t expectedResultSize = static_cast<std::uintmax_t>(multiplier0.size()) * static_cast<std::uintmax_t>(multiplier1.size());
			productOfSumsOfProducts.emplace_back(multiplySumsOfProducts(std::move(multiplier0), std::move(multiplier1), progress));
			const std::size_t actualResultSize = productOfSumsOfProducts.back().size();
			expectedSolutions = expectedSolutions / expectedResultSize * actualResultSize;
		}
		else
		{
			productOfSumsOfProducts.emplace_back(multiplySumsOfProducts(std::move(multiplier0), std::move(multiplier1), progress));
		}
	}
	
	return std::move(productOfSumsOfProducts.front());
}

template<std::unsigned_integral INDEX_T>
typename PetricksMethod<INDEX_T>::solutions_t PetricksMethod<INDEX_T>::solve(const std::string &functionName) &&
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
