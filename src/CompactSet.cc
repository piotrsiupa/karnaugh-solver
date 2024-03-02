#include "./CompactSet.hh"


#ifndef NDEBUG
template<std::unsigned_integral T>
void CompactSet<T>::validate() const noexcept
{
	const std::size_t actualCount = std::ranges::count(bits, true);
	assert(size_ == actualCount);
}
#endif


#include <cstdint>

template class CompactSet<std::uint8_t>;
template class CompactSet<std::uint16_t>;
template class CompactSet<std::uint32_t>;
template class CompactSet<std::uint64_t>;
