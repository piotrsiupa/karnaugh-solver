#include "./PrimeImplicants.hh"


#ifndef NDEBUG
bool PrimeImplicants::covers(const Minterm minterm) const
{
	for (const PrimeImplicant &primeImplicant : *this)
		if (primeImplicant.covers(minterm))
			return true;
	return false;
}
#endif
