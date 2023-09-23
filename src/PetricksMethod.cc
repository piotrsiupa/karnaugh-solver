#include "./PetricksMethod.hh"

#include <algorithm>
#include <cstdint>
#include <iostream>


template<typename MINTERM, typename PRIME_IMPLICANT, typename INDEX_T>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::index_t PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::findEssentialPrimeImplicantIndex(const minterm_t minterm)
{
	std::uint_fast8_t count = 0;
	index_t index;
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

template<typename MINTERM, typename PRIME_IMPLICANT, typename INDEX_T>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::primeImplicants_t PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::extractEssentials()
{
	primeImplicants_t essentials;
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
		for (const minterm_t &coveredMinterm : essentials.back().findMinterms())
		{
			const typename minterms_t::iterator iterToRemove = minterms.find(coveredMinterm);
			if (iterToRemove == minterms.end())
				continue;
			if (*iter == coveredMinterm)
				iter = minterms.erase(iterToRemove);
			else
				minterms.erase(iterToRemove);
		}
	}
	return essentials;
}

template<typename MINTERM, typename PRIME_IMPLICANT, typename INDEX_T>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::productOfSumsOfProducts_t PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::createPreliminaryProductOfSums() const
{
	productOfSumsOfProducts_t productOfSums;
	for (const minterm_t &minterm : minterms)
	{
		sumOfProducts_t &sum = productOfSums.emplace_back();
		for (index_t i = 0; i != primeImplicants.size(); ++i)
			if (primeImplicants[i].covers(minterm))
				sum.emplace_back().push_back(i);
	}
	return productOfSums;
}

template<typename MINTERM, typename PRIME_IMPLICANT, typename INDEX_T>
void PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::removeRedundantSums(productOfSumsOfProducts_t &productOfSums)
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

template<typename MINTERM, typename PRIME_IMPLICANT, typename INDEX_T>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::productOfSumsOfProducts_t PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::createProductOfSums() const
{
	productOfSumsOfProducts_t productOfSums = createPreliminaryProductOfSums();
	removeRedundantSums(productOfSums);
	return productOfSums;
}

template<typename MINTERM, typename PRIME_IMPLICANT, typename INDEX_T>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::sumOfProducts_t PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::multiplySumsOfProducts(const sumOfProducts_t &multiplier0, const sumOfProducts_t &multiplier1)
{
	product_t newProduct;
	newProduct.reserve(multiplier0.front().size() + multiplier1.front().size());
	PetricksHasseDiagram<index_t> hasseDiagram;
	for (const product_t &x : multiplier0)
	{
		for (const product_t &y : multiplier1)
		{
			newProduct.clear();
			std::set_union(x.cbegin(), x.cend(), y.cbegin(), y.cend(), std::back_inserter(newProduct));
			hasseDiagram.insertRemovingSupersets(std::move(newProduct));
		}
	}
	return hasseDiagram.getSets();
}

template<typename MINTERM, typename PRIME_IMPLICANT, typename INDEX_T>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::sumOfProducts_t PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::findSumOfProducts() const
{
	productOfSumsOfProducts_t productOfSumsOfProducts = createProductOfSums();
	if (productOfSumsOfProducts.empty())
		return sumOfProducts_t{};
	
	while (productOfSumsOfProducts.size() != 1)
	{
		sumOfProducts_t multiplier0 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		sumOfProducts_t multiplier1 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		productOfSumsOfProducts.emplace_back(multiplySumsOfProducts(std::move(multiplier0), std::move(multiplier1)));
	}
	
	return std::move(productOfSumsOfProducts.front());
}

template<typename MINTERM, typename PRIME_IMPLICANT, typename INDEX_T>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::solutions_t PetricksMethod<MINTERM, PRIME_IMPLICANT, INDEX_T>::solve()
{
	primeImplicants_t essentials = extractEssentials();
	sumOfProducts_t sumOfProducts = findSumOfProducts();
	
	if (sumOfProducts.empty())
		return !essentials.empty()
			? solutions_t{std::move(essentials)}
			: solutions_t{{primeImplicant_t::error()}};
	
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


#include "Minterm.hh"
#include "PrimeImplicant.hh"

template class PetricksMethod<Minterm, PrimeImplicant, std::uint8_t>;
template class PetricksMethod<Minterm, PrimeImplicant, std::uint16_t>;
template class PetricksMethod<Minterm, PrimeImplicant, std::uint32_t>;
template class PetricksMethod<Minterm, PrimeImplicant, std::uint64_t>;
