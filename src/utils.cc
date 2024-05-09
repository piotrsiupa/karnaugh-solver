#include "utils.hh"


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
	
	assert(invertPermutation(invertPermutation(ordering)) == ordering);
	
	permutation_t values = ordering;
	applyInversePermutation(values, ordering);
	assert(values.size() == ordering.size());
	for (std::size_t i = 0; i != values.size(); ++i)
		assert(values[i] == i);
	
	applyPermutation(values, ordering);
	assert(values == ordering);
	
	values = {12, 8, 3, 0, 15, 7};
	permuteIndexes(values, ordering);
	assert((values == permutation_t{13, 1, 2, 10, 12, 14}));
	
	std::set<std::size_t> set{12, 8, 3, 0, 15, 7};
	permuteIndexes(set, ordering);
	assert((set == std::set<std::size_t>{13, 1, 2, 10, 12, 14}));
}

static void verifyRemovalFunctions()
{
	const std::vector<int> values = {10, 6, 3, 2, 0, 5, 8, 14, 1, 9, 4, 7, 13, 15, 11, 12};
	
	const permutation_t removeEven = makeRemovingPermutation(values, [](const int x){ return x % 2 == 0; });
	const permutation_t removeOdd = makeRemovingPermutation(values, [](const int x){ return x % 2 != 0; });
	assert((removeEven == permutation_t{SIZE_MAX, SIZE_MAX, 0, SIZE_MAX, SIZE_MAX, 1, SIZE_MAX, SIZE_MAX, 2, 3, SIZE_MAX, 4, 5, 6, 7, SIZE_MAX}));
	assert((removeOdd  == permutation_t{0, 1, SIZE_MAX, 2, 3, SIZE_MAX, 4, 5, SIZE_MAX, SIZE_MAX, 6, SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX, 7}));
	
	assert(makeRemovingPermutation(values, [](const int x){ return x == 0; }, [](const int x){ return x % 2; }) == removeEven);
	assert(makeRemovingPermutation(values, [](const int x){ return x != 0; }, [](const int x){ return x % 2; }) == removeOdd);
	
	assert(makeRemovingPermutationByIndex(values, [&values](const std::size_t i){ return values[i] % 2 == 0; }) == removeEven);
	assert(makeRemovingPermutationByIndex(values, [&values](const std::size_t i){ return values[i] % 2 != 0; }) == removeOdd);
	
	assert(makeRemovingPermutationByIndex(values, [](const int x){ return x == 0; }, [&values](const std::size_t i){ return values[i] % 2; }) == removeEven);
	assert(makeRemovingPermutationByIndex(values, [](const int x){ return x != 0; }, [&values](const std::size_t i){ return values[i] % 2; }) == removeOdd);
	
	std::vector<int> data = values;
	removeByPermutation(data, removeEven);
	assert((data == std::vector<int>{3, 5, 1, 9, 7, 13, 15, 11}));
	data = values;
	removeByPermutation(data, removeOdd);
	assert((data == std::vector<int>{10, 6, 2, 0, 8, 14, 4, 12}));
	
	std::vector<std::size_t> indexes{5, 11, 0, 2, 10, 14, 3};
	removeAndPermuteIndexes(indexes, removeEven);
	assert((indexes == std::vector<std::size_t>{1, 4, 0, 7}));
	indexes = {5, 11, 0, 2, 10, 14, 3};
	removeAndPermuteIndexes(indexes, removeOdd);
	assert((indexes == std::vector<std::size_t>{0, 6, 2}));
	
	std::set<std::size_t> indexesInSet{5, 11, 0, 2, 10, 14, 3};
	removeAndPermuteIndexes(indexesInSet, removeEven);
	assert((indexesInSet == std::set<std::size_t>{1, 4, 0, 7}));
	indexesInSet = {5, 11, 0, 2, 10, 14, 3};
	removeAndPermuteIndexes(indexesInSet, removeOdd);
	assert((indexesInSet == std::set<std::size_t>{0, 6, 2}));
}

void verifyUtils()
{
	verifyFirst();
	verifyPermutationFunctions();
	verifyRemovalFunctions();
}
#endif
