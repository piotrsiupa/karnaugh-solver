#include "./math.hh"

#include <cstdint>
#include <map>
#include <utility>


double calcBinomialCoefficient(const std::size_t n, std::size_t k)
{
	if (k > n / 2)
		k = n - k;
	
	static std::map<std::pair<std::size_t, std::size_t>, double> cache;
	const auto cacheHit = cache.find({n, k});
	if (cacheHit != cache.cend())
		return cacheHit->second;
	
	double coefficient = 1.0;
	for (std::size_t i = 1, j = n; i != k + 1; ++i, --j)
	{
		coefficient *= j;
		coefficient /= i;
	}
	
	cache[{n, k}] = coefficient;
	return coefficient;
}

double calcBinomialCoefficientSum(const std::size_t n, const std::size_t k)
{
	static std::map<std::pair<std::size_t, std::size_t>, double> cache;
	const auto cacheHit = cache.find({n, k});
	if (cacheHit != cache.cend())
		return cacheHit->second;
	
	double coefficientSum = 0.0;
	for (std::size_t i = 0; i != k + 1; ++i)
		coefficientSum += calcBinomialCoefficient(n, i);
	
	cache[{n, k}] = coefficientSum;
	return coefficientSum;
}
