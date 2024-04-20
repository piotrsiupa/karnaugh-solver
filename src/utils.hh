#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <numeric>
#include <vector>


// An object of this class casts to `true` the first time and to `false` every other time.
class First
{
	bool first = true;
public:
	[[nodiscard]] operator bool() { const bool firstCopy = first; first = false; return firstCopy; }
};


using permutation_t = std::vector<std::size_t>;

template<std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
		requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
[[nodiscard]] permutation_t makeSortingPermutation(const R& r, Comp comp = {}, Proj proj = {})
{
	permutation_t permutation(r.size());
	std::iota(permutation.begin(), permutation.end(), 0);
	if constexpr (std::is_same_v<Proj, std::identity>)
		std::ranges::sort(permutation, comp, [&r = std::as_const(r)](const std::size_t i){ return r[i]; });
	else
		std::ranges::sort(permutation, comp, [&r = std::as_const(r), &proj](const std::size_t i){ return std::invoke(proj, r[i]); });
	return permutation;
}

[[nodiscard]] inline permutation_t invertPermutation(const permutation_t &permutation)
{
	permutation_t inversePermutation(permutation.size());
	for (std::size_t i = 0; i != permutation.size(); ++i)
		inversePermutation[permutation[i]] = i;
	return inversePermutation;
}

template<typename T>
void applyInversePermutation(std::vector<T> &data, permutation_t &&permutation)
{
	assert(data.size() == permutation.size());
	if (data.size() < 2)
		return;
	for (std::size_t i = 1; i != permutation.size(); ++i)
	{
		while (permutation[i] != i)
		{
			const std::size_t j = permutation[i];
			std::ranges::swap(permutation[i], permutation[j]);
			std::ranges::swap(data[i], data[j]);
		}
	}
}

template<typename T>
void applyInversePermutation(std::vector<T> &data, const permutation_t &permutation)
{
	return applyInversePermutation(data, permutation_t(permutation));
}

template<typename T>
void applyPermutation(std::vector<T> &data, const permutation_t &permutation)
{
	return applyInversePermutation(data, invertPermutation(permutation));
}


#ifndef NDEBUG
void verifyUtils();
#endif
