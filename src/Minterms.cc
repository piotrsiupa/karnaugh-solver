#include "./Minterms.hh"

#include <cassert>


Minterms Minterms::operator~() const
{
	std::vector<bool> negatedBitset;
	negatedBitset.resize(bitset.size());
	std::transform(bitset.cbegin(), bitset.cend(), negatedBitset.begin(), std::logical_not<>{});
	return {std::move(negatedBitset), bitset.size() - size};
}

#ifndef NDEBUG
void Minterms::validate() const
{
	std::size_t actualCount = 0;
	for ([[maybe_unused]] const Minterm minterm : *this)
		++actualCount;
	assert(size == actualCount);
}
#endif
