#include "./PetricksMethod.hh"

#include <algorithm>
#include <cstdint>


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
				sum.emplace_back().first.insert(i);
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
	sumOfProducts_t result;
	
	// Because of the absorptive law of boolean algebra "(A.B+C).(A+D)=A.B+A.C+C.D" (the result "A.B+A.B.D+A.C+C.D" can be reduced).
	// Instead of calculating the long version and later reducing it, the "A.B" is extracted straight into final result in this loop.
	// (This approach doesn't always remove all redundancy but it would be too costly to do it during every multiplication.)
	for (auto &x : multiplier0)
	{
		for (auto &y : multiplier1)
		{
			if (std::includes(x.first.begin(), x.first.end(), y.first.begin(), y.first.end()))
			{
				result.emplace_back(x.first, false);
				x.second = true;
				y.second = true;
			}
			else if (std::includes(y.first.begin(), y.first.end(), x.first.begin(), x.first.end()))
			{
				result.emplace_back(y.first, false);
				x.second = true;
				y.second = true;
			}
		}
	}
	multiplier0.erase(std::remove_if(multiplier0.begin(), multiplier0.end(), [](auto &x){ return x.second; }), multiplier0.end());
	multiplier1.erase(std::remove_if(multiplier1.begin(), multiplier1.end(), [](auto &x){ return x.second; }), multiplier1.end());
	
	for (const auto &x : multiplier0)
	{
		for (const auto &y : multiplier1)
		{
			result.push_back(x);
			result.back().first.insert(y.first.cbegin(), y.first.cend());
		}
	}
	
	return result;
}

template<typename MINTERM, typename PRIME_IMPLICANT>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT>::sumOfProducts_t PetricksMethod<MINTERM, PRIME_IMPLICANT>::findPreliminarySumOfProducts() const
{
	productOfSumsOfProducts_t productOfSumsOfProducts = createProductOfSums();
	while (productOfSumsOfProducts.size() >= 2)
	{
		sumOfProducts_t multiplier0 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		sumOfProducts_t multiplier1 = std::move(productOfSumsOfProducts.back());
		productOfSumsOfProducts.pop_back();
		productOfSumsOfProducts.emplace_back(multiplySumsOfProducts(std::move(multiplier0), std::move(multiplier1)));
	}
	return productOfSumsOfProducts.empty() ? sumOfProducts_t{} : std::move(productOfSumsOfProducts.front());
}

template<typename MINTERM, typename PRIME_IMPLICANT>
void PetricksMethod<MINTERM, PRIME_IMPLICANT>::removeRedundantSums(sumOfProducts_t &sumOfProducts)
{
	for (auto x = sumOfProducts.begin(); x != sumOfProducts.end(); ++x)
	{
		if (!x->first.empty())
		{
			for (auto y = std::next(x); y != sumOfProducts.end(); ++y)
			{
				if (!y->first.empty())
				{
					if (std::includes(x->first.cbegin(), x->first.cend(), y->first.cbegin(), y->first.cend()))
					{
						x->first.clear();
						break;
					}
					else if (std::includes(y->first.cbegin(), y->first.cend(), x->first.cbegin(), x->first.cend()))
					{
						y->first.clear();
					}
				}
			}
		}
	}
	sumOfProducts.erase(std::remove_if(sumOfProducts.begin(), sumOfProducts.end(), [](auto &x){ return x.first.empty(); }), sumOfProducts.end());
}

template<typename MINTERM, typename PRIME_IMPLICANT>
typename PetricksMethod<MINTERM, PRIME_IMPLICANT>::sumOfProducts_t PetricksMethod<MINTERM, PRIME_IMPLICANT>::findSumOfProducts() const
{
	sumOfProducts_t sumOfProducts = findPreliminarySumOfProducts();
	removeRedundantSums(sumOfProducts);
	sumOfProducts.shrink_to_fit();
	return sumOfProducts;
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
		solutions.back().reserve(essentials.size() + x.first.size());
		solutions.back().insert(solutions.back().end(), essentials.begin(), essentials.end());
		for (const std::size_t &index : x.first)
			solutions.back().push_back(primeImplicants[index]);
	}
	return solutions;
}


#include "Minterm.hh"
#include "PrimeImplicant.hh"

template class PetricksMethod<Minterm, PrimeImplicant>;
