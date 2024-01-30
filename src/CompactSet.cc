#include "./CompactSet.hh"


#ifndef NDEBUG
template<typename T>
void CompactSet<T>::validate() const
{
	const std::size_t actualCount = std::ranges::count(bitset.cbegin(), bitset.cend(), true);
	assert(size == actualCount);
}
#endif


#include "Minterm.hh"

template class CompactSet<Minterm>;
