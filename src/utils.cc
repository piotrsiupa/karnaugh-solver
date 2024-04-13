#include "utils.hh"

#include <cassert>


#ifndef NDEBUG
void verifyUtils()
{
	const std::vector<std::size_t> ordering = {10, 6, 3, 2, 0, 5, 8, 14, 1, 9, 4, 7, 13, 15, 11, 12};
	std::vector<std::size_t> values = ordering;
	applyOrdering(values, ordering);
	for (std::size_t i = 0; i != ordering.size(); ++i)
		assert(values[i] == i);
	const std::vector<std::size_t> reverseOrdering = makeReverseOrdering(ordering);
	applyOrdering(values, reverseOrdering);
	assert(values == ordering);
}
#endif
