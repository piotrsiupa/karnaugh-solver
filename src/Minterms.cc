#include "./Minterms.hh"

#include <cassert>


#ifndef NDEBUG
void Minterms::validate() const
{
	std::size_t actualCount = 0;
	for ([[maybe_unused]] const Minterm minterm : *this)
		++actualCount;
	assert(size == actualCount);
}
#endif
