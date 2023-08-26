#include "./PetricksMethod.hh"

#include <algorithm>
#include <cstdint>

#include "HasseDiagram.hh"


template<typename MINTERM, typename PRIME_IMPLICANT>
std::size_t PetricksMethod<MINTERM, PRIME_IMPLICANT>::findEssentialPrimeImplicantIndex(const minterm_t minterm)
{
	std::uint_fast8_t count = 0;
	std::size_t index;
	for (std::size_t i = 0; i != primeImplicants.size(); ++i)
	{
		if (primeImplicants[i].covers(minterm))
		{
			if (++count == 2)
				return SIZE_MAX;
			index = i;
		}
	}
	return count != 0 ? index : SIZE_MAX;
}

template<typename MINTERM, typename PRIME_IMPLICANT>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT>::primeImplicants_t PetricksMethod<MINTERM, PRIME_IMPLICANT>::extractEssentials()
{
	primeImplicants_t essentials;
	for (typename minterms_t::const_iterator iter = minterms.cbegin(); iter != minterms.cend();)
	{
		const std::size_t essentialPrimeImplicantIndex = findEssentialPrimeImplicantIndex(*iter);
		if (essentialPrimeImplicantIndex == SIZE_MAX)
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

template<typename MINTERM, typename PRIME_IMPLICANT>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT>::productOfSumsOfProducts_t PetricksMethod<MINTERM, PRIME_IMPLICANT>::createPreliminaryProductOfSums() const
{
	productOfSumsOfProducts_t productOfSums;
	for (const minterm_t &minterm : minterms)
	{
		sumOfProducts_t &sum = productOfSums.emplace_back();
		for (std::size_t i = 0; i != primeImplicants.size(); ++i)
			if (primeImplicants[i].covers(minterm))
				sum.emplace_back().insert(i);
	}
	return productOfSums;
}

template<typename MINTERM, typename PRIME_IMPLICANT>
void PetricksMethod<MINTERM, PRIME_IMPLICANT>::removeRedundantSums(productOfSumsOfProducts_t &productOfSums)
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

template<typename MINTERM, typename PRIME_IMPLICANT>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT>::productOfSumsOfProducts_t PetricksMethod<MINTERM, PRIME_IMPLICANT>::createProductOfSums() const
{
	productOfSumsOfProducts_t productOfSums = createPreliminaryProductOfSums();
	removeRedundantSums(productOfSums);
	return productOfSums;
}

template<typename MINTERM, typename PRIME_IMPLICANT>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT>::sumOfProducts_t PetricksMethod<MINTERM, PRIME_IMPLICANT>::multiplySumsOfProducts(sumOfProducts_t multiplier0, sumOfProducts_t multiplier1)
{
	HasseDiagram hasseDiagram;
	for (const product_t &x : multiplier0)
	{
		for (const product_t &y : multiplier1)
		{
			product_t newProduct = x;
			newProduct.insert(y.cbegin(), y.cend());
			hasseDiagram.insertRemovingSupersets(std::move(newProduct));
		}
	}
	return hasseDiagram.getSets();
}

template<typename MINTERM, typename PRIME_IMPLICANT>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT>::sumOfProducts_t PetricksMethod<MINTERM, PRIME_IMPLICANT>::findSumOfProducts() const
{
	productOfSumsOfProducts_t productOfSumsOfProducts = createProductOfSums();
	if (productOfSumsOfProducts.empty())
		return sumOfProducts_t{};
	
	while (productOfSumsOfProducts.size() != 1)
	{
		productOfSumsOfProducts_t newProductOfSumsOfProducts;
		while (productOfSumsOfProducts.size() >= 2)
		{
			sumOfProducts_t multiplier0 = std::move(productOfSumsOfProducts.back());
			productOfSumsOfProducts.pop_back();
			sumOfProducts_t multiplier1 = std::move(productOfSumsOfProducts.back());
			productOfSumsOfProducts.pop_back();
			newProductOfSumsOfProducts.emplace_back(multiplySumsOfProducts(std::move(multiplier0), std::move(multiplier1)));
		}
		if (!productOfSumsOfProducts.empty())
			newProductOfSumsOfProducts.push_back(std::move(productOfSumsOfProducts.front()));
		productOfSumsOfProducts = std::move(newProductOfSumsOfProducts);
	}
	
	return std::move(productOfSumsOfProducts.front());
}

template<typename MINTERM, typename PRIME_IMPLICANT>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT>::solutions_t PetricksMethod<MINTERM, PRIME_IMPLICANT>::solve()
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
		for (const std::size_t &index : x)
			solutions.back().push_back(primeImplicants[index]);
	}
	return solutions;
}


#include "Minterm.hh"
#include "PrimeImplicant.hh"

template class PetricksMethod<Minterm, PrimeImplicant>;
