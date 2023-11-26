#pragma once

#include <bitset>
#include <climits>
#include <cstdint>
#include <ostream>
#include <vector>
#include <utility>

#include "global.hh"
#include "Minterm.hh"


class Implicant
{
public:
	using mask_t = std::uint32_t;
	static_assert(sizeof(mask_t) * CHAR_BIT >= ::maxBits);
	using minterms_t = std::vector<Minterm>;
	using splitBits_t = std::vector<std::pair<bits_t, bool>>;
	
	static constexpr Implicant all() { return {0, 0, 0}; }
	static constexpr Implicant error() { return {~mask_t(0), ~mask_t(0), 0}; }
	
private:
	mask_t trueBits, falseBits;
	bits_t bitCount;
	
	explicit Implicant(const mask_t trueBits, const mask_t falseBits) : trueBits(trueBits), falseBits(falseBits) { recalculateBits(); }
	constexpr Implicant(const mask_t trueBits, const mask_t falseBits, const bits_t bitCount) : trueBits(trueBits), falseBits(falseBits), bitCount(bitCount) {}
	
	void recalculateBits() { bitCount = static_cast<bits_t>(std::bitset<32>(trueBits | falseBits).count()); }
	
public:
	explicit Implicant(const Minterm minterm) : trueBits(minterm), falseBits(minterm ^ maxMinterm), bitCount(::bits) {}
	Implicant(const Implicant &) = default;
	Implicant& operator=(const Implicant &) = default;
	
	constexpr bool operator==(const Implicant &other) const { return this->trueBits == other.trueBits && this->falseBits == other.falseBits && this->bitCount == other.bitCount; }
	constexpr bool operator!=(const Implicant &other) const { return !operator==(other); }
	bool operator<(const Implicant &other) const;
	constexpr bool covers(const Minterm minterm) const { return (trueBits & minterm) == trueBits && (falseBits & ~minterm) == falseBits; }
	
	Implicant& operator&=(const Implicant &other) { this->trueBits &= other.trueBits; this->falseBits &= other.falseBits; recalculateBits(); if (bitCount == 0) *this = error(); return *this; }
	Implicant operator&(const Implicant &other) const { Implicant copy = *this; copy &= other; return copy; }
	Implicant& operator|=(const Implicant &other) { this->trueBits |= other.trueBits; this->falseBits |= other.falseBits; recalculateBits(); return *this; }
	Implicant operator|(const Implicant &other) const { Implicant copy = *this; copy |= other; return copy; }
	Implicant& operator-=(const Implicant &other) { this->trueBits &= ~other.trueBits; this->falseBits &= ~other.falseBits; recalculateBits(); return *this; }
	Implicant operator-(const Implicant &other) const { Implicant copy = *this; copy -= other; return copy; }
	Implicant& setBit(const bits_t bit, const bool negated) { const mask_t mask = 1 << (::bits - bit - 1); if (negated) falseBits |= mask; else trueBits |= mask; ++bitCount; return *this; }
	
	constexpr bool isError() const { return falseBits != 0 && bitCount == 0; }
	constexpr mask_t getTrueBits() const { return isError() ? 0 : trueBits; }
	constexpr mask_t getFalseBits() const { return isError() ? 0 : falseBits; }
	constexpr bits_t getBitCount() const { return bitCount; }
	splitBits_t splitBits() const;
	minterms_t findMinterms() const;
	
	static bool areMergeable(const Implicant &x, const Implicant &y);
	static Implicant merge(const Implicant &x, const Implicant &y);
	
	void printHuman(std::ostream &o, const bool parentheses) const;
	void printVerilog(std::ostream &o, const bool parentheses) const;
	void printVhdl(std::ostream &o, const bool parentheses) const;
	void printCpp(std::ostream &o, const bool parentheses) const;
	void printMath(std::ostream &o, const bool parentheses) const;
};
