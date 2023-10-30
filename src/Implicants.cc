#include "./Implicants.hh"

#include <algorithm>


Implicants& Implicants::sort()
{
	std::sort(begin(), end());
	return *this;
}

void Implicants::print(std::ostream &o) const
{
	if (size() == 1)
	{
		front().print(o, false);
	}
	else
	{
		for (const Implicant &implicant : *this)
		{
			if (&implicant != &front())
				o << " || ";
			implicant.print(o, true);
		}
	}
}

#ifndef NDEBUG
bool Implicants::covers(const Minterm minterm) const
{
	for (const Implicant &implicant : *this)
		if (implicant.covers(minterm))
			return true;
	return false;
}
#endif
