#include "utils.hh"

#include <cassert>


#ifndef NDEBUG
static void verifyFirst()
{
	First first;
	assert(first);
	assert(!first);
	assert(!first);
	assert(!first);
	assert(!first);
}

static void verifyPermutationFunctions()
{
	const permutation_t ordering = {10, 6, 3, 2, 0, 5, 8, 14, 1, 9, 4, 7, 13, 15, 11, 12};
	assert(makeSortingPermutation(ordering) == invertPermutation(ordering));
	permutation_t values = ordering;
	applyInversePermutation(values, ordering);
	for (std::size_t i = 0; i != values.size(); ++i)
		assert(values[i] == i);
	applyPermutation(values, ordering);
	assert(values == ordering);
}

void verifyUtils()
{
	verifyFirst();
	verifyPermutationFunctions();
}
#endif
