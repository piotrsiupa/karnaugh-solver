#pragma once

#include <vector>

#include "Minterm.hh"
#include "Implicant.hh"


class Implicants : public std::vector<Implicant>
{
public:
	using std::vector<Implicant>::vector;
	
#ifndef NDEBUG
	bool covers(const Minterm minterm) const;
#endif
};
