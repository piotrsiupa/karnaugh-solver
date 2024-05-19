#include "./Solution.hh"

#include <algorithm>


Solution& Solution::sort()
{
	std::sort(begin(), end());
	return *this;
}

#ifndef NDEBUG
bool Solution::covers(const Minterm minterm) const
{
	for (const Implicant &implicant : *this)
		if (implicant.covers(minterm))
			return true;
	return false;
}
#endif
