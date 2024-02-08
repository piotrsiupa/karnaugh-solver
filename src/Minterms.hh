#pragma once

#include "CompactSet.hh"
#include "global.hh"
#include "Minterm.hh"


class Minterms : public CompactSet<Minterm>
{
public:
	Minterms() : CompactSet(static_cast<std::size_t>(::maxMinterm) + 1) {}
};
