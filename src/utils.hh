#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>
#include <set>


// An object of this class casts to `true` the first time and to `false` every other time.
class First
{
	bool first = true;
public:
	[[nodiscard]] operator bool() { const bool firstCopy = first; first = false; return firstCopy; }
};


// Permutation is a list of indexes describing order of a colllection.
// Applying the permutation will move each element `i` of the collection to the position `permutation[i]`.
// Applying permutation to itself will produce the same result as `std::iota(perm.begin(), perm.end())`.
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

// Inverted permutation is a permutation that will restore the permuted collection to its original order.
// You can also think about it as indexes of from where an element should be moved to the current position.
// Inverted inverted permutation is equal to the original permutation.
[[nodiscard]] inline permutation_t invertPermutation(const permutation_t &permutation)
{
	permutation_t inversePermutation(permutation.size());
	for (std::size_t i = 0; i != permutation.size(); ++i)
	{
		assert(permutation[i] != SIZE_MAX);
		inversePermutation[permutation[i]] = i;
	}
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
		assert(permutation[i] != SIZE_MAX);
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


template<typename T>
std::vector<T>::const_iterator removeIfIndex(std::vector<T> &v, const std::function<bool(std::size_t)> &p)
{
	std::size_t i;
	for (i = 0; i != v.size(); ++i)
	{
		if (p(i))
		{
			for (std::size_t j = i; ++j != v.size();)
				if (!p(j))
					v[i++] = std::move(v[j]);
			break;
		}
	}
	return std::ranges::next(v.cbegin(), i);
}

inline void permuteIndexes(std::vector<std::size_t> &indexes, const permutation_t &permutation)
{
	for (std::size_t &index : indexes)
	{
		assert(permutation[index] != SIZE_MAX);
		index = permutation[index];
	}
}

inline void permuteIndexes(std::set<std::size_t> &indexes, const permutation_t &permutation)
{
	std::set<std::size_t> newIndexes;
	for (const std::size_t &index : indexes)
	{
		assert(permutation[index] != SIZE_MAX);
		newIndexes.insert(permutation[index]);
	}
	indexes = std::move(newIndexes);
}

// This function doesn't create a fully legal `permutation_t` because elements to be removed have index set to `SIZE_MAX`.
// It cannot be applied and inverted but it can be used in `permuteIndexes` as long as the indexes don't contain the removed ones.
template<std::ranges::random_access_range R, class Cond, class Proj = std::identity>
[[nodiscard]] permutation_t makeRemovingPermutation(const R& r, Cond cond, Proj proj = {})
{
	permutation_t permutation(r.size());
	std::size_t nextIndex = 0;
	for (std::size_t i = 0; i != r.size(); ++i)
		permutation[i] = std::invoke(cond, std::invoke(proj, r[i])) ? SIZE_MAX : nextIndex++;
	return permutation;
}

// This function doesn't create a fully legal `permutation_t` because elements to be removed have index set to `SIZE_MAX`.
// It cannot be applied and inverted but it can be used in `permuteIndexes` as long as the indexes don't contain the removed ones.
template<std::ranges::random_access_range R, class Cond, class Proj = std::identity>
[[nodiscard]] permutation_t makeRemovingPermutationByIndex(const R& r, Cond cond, Proj proj = {})
{
	permutation_t permutation(r.size());
	std::size_t nextIndex = 0;
	for (std::size_t i = 0; i != r.size(); ++i)
		permutation[i] = std::invoke(cond, std::invoke(proj, i)) ? SIZE_MAX : nextIndex++;
	return permutation;
}

template<typename T>
inline void removeByPermutation(std::vector<T> &v, const permutation_t permutation)
{
	const auto eraseBegin = removeIfIndex(v, [&permutation](const std::size_t i){ return permutation[i] == SIZE_MAX; });
	v.erase(eraseBegin, v.end());
}

inline void removeAndPermuteIndexes(std::vector<std::size_t> &indexes, const permutation_t &permutation)
{
	const auto [eraseBegin, eraseEnd] = std::ranges::remove_if(indexes, [&permutation](const std::size_t i){ return permutation[i] == SIZE_MAX; });
	indexes.erase(eraseBegin, eraseEnd);
	permuteIndexes(indexes, permutation);
}

inline void removeAndPermuteIndexes(std::set<std::size_t> &indexes, const permutation_t &permutation)
{
	std::set<std::size_t> newIndexes;
	for (const std::size_t &index : indexes)
		if (permutation[index] != SIZE_MAX)
			newIndexes.insert(permutation[index]);
	indexes = std::move(newIndexes);
}


#ifndef NDEBUG
void verifyUtils();
#endif
