#include "./Implicants.hh"

#include <algorithm>


Implicants& Implicants::humanSort()
{
	std::ranges::sort(*this, [](const Implicant &x, const Implicant &y){ return x.humanLess(y); });
	return *this;
}
