#include "./Minterms.hh"

#include <cassert>


#ifndef NDEBUG
void Minterms::validate() const
{
	const std::size_t actualCount = std::count(bitset.cbegin(), bitset.cend(), true);
	assert(size == actualCount);
}
#endif
