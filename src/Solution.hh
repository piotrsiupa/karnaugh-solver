#pragma once

#include <numeric>
#include <ostream>

#include "GateCost.hh"
#include "Implicants.hh"
#include "Minterm.hh"


class Solution final : public GateCost, public Implicants
{
public:
	using Implicants::Implicants;
	
	Solution& humanSort() { Implicants::humanSort(); return *this; }
	
	std::size_t getNotCount() const final { return std::accumulate<decltype(begin()), std::size_t>(cbegin(), cend(), 0, [](const std::size_t acc, const Implicant &implicant){ return implicant.getFalseBitCount() + acc; }); }
	std::size_t getAndCount() const final { return std::accumulate<decltype(begin()), std::size_t>(cbegin(), cend(), 0, [](const std::size_t acc, const Implicant &implicant){ return implicant.getBitCount() == 0 ? acc : implicant.getBitCount() - 1 + acc; }); }
	std::size_t getOrCount() const final { return empty() ? 0 : size() - 1; }
	
	void printHuman(std::ostream &o) const;
	void printVerilog(std::ostream &o) const;
	void printVhdl(std::ostream &o) const;
	void printCpp(std::ostream &o) const;
	void printMath(std::ostream &o) const;
	
#ifndef NDEBUG
	bool covers(const Minterm minterm) const;
#endif
};