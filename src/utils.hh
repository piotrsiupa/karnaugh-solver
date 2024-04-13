#pragma once

#include <algorithm>
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


template<std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
		requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
[[nodiscard]] std::vector<std::size_t> sort_indexes(R&& r, Comp comp = {}, Proj proj = {})
{
	std::vector<std::size_t> indexes(r.size());
	std::iota(indexes.begin(), indexes.end(), 0);
	if constexpr (std::is_same_v<Proj, std::identity>)
		std::ranges::sort(indexes, comp, [&r = std::as_const(r)](const std::size_t i){ return r[i]; });
	else
		std::ranges::sort(indexes, comp, [&r = std::as_const(r), &proj](const std::size_t i){ return std::invoke(proj, r[i]); });
	return indexes;
}


using ordering_t = std::vector<std::size_t>;

[[nodiscard]] inline ordering_t makeReverseOrdering(const ordering_t ordering)
{
	ordering_t reverseOrdering(ordering.size());
	for (std::size_t i = 0; i != ordering.size(); ++i)
		reverseOrdering[ordering[i]] = i;
	return reverseOrdering;
}

template<typename T>
void applyOrdering(std::vector<T> &data, ordering_t &&ordering)
{
	for (std::size_t i = 0; i != ordering.size(); ++i)
	{
		const std::size_t j = ordering[i];
		std::ranges::swap(ordering[i], ordering[j]);
		std::ranges::swap(data[i], data[j]);
	}
}

template<typename T>
void applyOrdering(std::vector<T> &data, const ordering_t &ordering)
{
	return applyOrdering(data, ordering_t(ordering));
}


#ifndef NDEBUG
void verifyUtils();
#endif
