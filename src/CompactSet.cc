#include "./CompactSet.hh"


#ifndef NDEBUG
template<std::unsigned_integral T>
void CompactSet<T>::validate() const
{
	const std::size_t actualCount = std::ranges::count(bits, true);
	assert(size_ == actualCount);
}
#endif


#include "Minterm.hh"

template class CompactSet<Minterm>;
