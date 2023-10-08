#pragma once

#include <vector>

#include "Minterm.hh"
#include "PrimeImplicant.hh"


class PrimeImplicants : public std::vector<PrimeImplicant>
{
public:
	using std::vector<PrimeImplicant>::vector;
	
#ifndef NDEBUG
	bool covers(const Minterm minterm) const;
#endif
};
