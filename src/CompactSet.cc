#include "./CompactSet.hh"


#ifndef NDEBUG
template<typename T>
void CompactSet<T>::validate() const
{
	const std::size_t actualCount = std::ranges::count(bits.cbegin(), bits.cend(), true);
	assert(size_ == actualCount);
}
#endif


#include "Minterm.hh"

template class CompactSet<Minterm>;
