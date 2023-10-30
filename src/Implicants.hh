#pragma once

#include <ostream>
#include <vector>

#include "Minterm.hh"
#include "Implicant.hh"


class Implicants : public std::vector<Implicant>
{
public:
	using std::vector<Implicant>::vector;
	
	Implicants& sort();
	
	void print(std::ostream &o) const;
	
#ifndef NDEBUG
	bool covers(const Minterm minterm) const;
#endif
};
