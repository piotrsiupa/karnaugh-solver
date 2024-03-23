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
