#pragma once

#include <cassert>

#include "CompactSet.hh"
#include "global.hh"
#include "Minterm.hh"


class Minterms : public CompactSet<Minterm>
{
	static size_type calcMaxSize() { return static_cast<size_type>(::maxMinterm) + 1; }
	
	Minterms(CompactSet<Minterm> &&set) : CompactSet<Minterm>(std::move(set)) { assert(max_size() == calcMaxSize()); }
	
public:
	Minterms() : CompactSet(calcMaxSize()) {}
	
	[[nodiscard]] Minterms operator~() const { return CompactSet<Minterm>::operator~(); }
};
