#pragma once

#include <climits>
#include <cstdint>
#include <ostream>
#include <vector>
#include <utility>

#include "global.hh"
#include "Minterm.hh"


class PrimeImplicant
{
public:
	using mask_t = std::uint32_t;
	static_assert(sizeof(mask_t) * CHAR_BIT >= maxBits);
	using minterms_t = std::vector<Minterm>;
	
	static constexpr PrimeImplicant all() { return {0, 0, 0}; }
	static constexpr PrimeImplicant error() { return {~mask_t(0), ~mask_t(0), 0}; }
	
private:
	using splitBits_t = std::vector<std::pair<bits_t, bool>>;
	
	mask_t trueBits, falseBits;
	bits_t bitCount;
	
	explicit PrimeImplicant(const mask_t trueBits, const mask_t falseBits) : trueBits(trueBits), falseBits(falseBits) { recalculateBits(); }
	constexpr PrimeImplicant(const mask_t trueBits, const mask_t falseBits, const bits_t bitCount) : trueBits(trueBits), falseBits(falseBits), bitCount(bitCount) {}
	
	void recalculateBits();
	
	splitBits_t splitBits(const bits_t bits) const;
	
public:
	explicit constexpr PrimeImplicant(const Minterm minterm, const bits_t bits) : trueBits(minterm), falseBits(minterm ^ ((1u << bits) - 1)), bitCount(bits) {}
	PrimeImplicant(const PrimeImplicant &) = default;
	
	constexpr bool operator==(const PrimeImplicant &other) const { return this->trueBits == other.trueBits && this->falseBits == other.falseBits && this->bitCount == other.bitCount; }
	bool operator<(const PrimeImplicant &other) const;
	constexpr bool covers(const Minterm minterm) const { return (trueBits & minterm) == trueBits && (falseBits & ~minterm) == falseBits; }
	
	PrimeImplicant& operator&=(const PrimeImplicant &other) { this->trueBits &= other.trueBits; this->falseBits &= other.falseBits; recalculateBits(); if (bitCount == 0) *this = error(); return *this; }
	PrimeImplicant operator&(const PrimeImplicant &other) { PrimeImplicant copy = *this; copy &= other; return copy; }
	PrimeImplicant& operator-=(const PrimeImplicant &other) { this->trueBits &= ~other.trueBits; this->falseBits &= ~other.falseBits; recalculateBits(); return *this; }
	PrimeImplicant operator-(const PrimeImplicant &other) { PrimeImplicant copy = *this; copy -= other; return copy; }
	
	constexpr bool isError() const { return falseBits != 0 && bitCount == 0; }
	constexpr mask_t getTrueBits() const { return isError() ? 0 : trueBits; }
	constexpr mask_t getFalseBits() const { return isError() ? 0 : falseBits; }
	constexpr bits_t getBitCount() const { return bitCount; }
	minterms_t findMinterms(const bits_t bits) const;
	
	static bool areMergeable(const PrimeImplicant &x, const PrimeImplicant &y);
	static PrimeImplicant merge(const PrimeImplicant &x, const PrimeImplicant &y);
	
	void print(std::ostream &o, const bits_t bits, const names_t &inputNames, const bool parentheses) const;
};
