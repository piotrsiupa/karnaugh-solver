#pragma once

#include <bitset>
#include <climits>
#include <cstdint>
#include <ostream>
#include <vector>
#include <utility>

#include "global.hh"
#include "Minterm.hh"
#include "Minterms.hh"


class Implicant
{
public:
	using mask_t = std::uint32_t;
	static_assert(sizeof(mask_t) * CHAR_BIT >= ::maxBits);
	using splitBits_t = std::vector<std::pair<bits_t, bool>>;
	
	static constexpr Implicant all() { return {0, 0}; }
	static constexpr Implicant none() { return {~mask_t(0), 0}; }
	
private:
	mask_t bits, mask;
	
public:
	constexpr Implicant(const mask_t bits, const mask_t mask) : bits(bits), mask(mask) {}
	explicit Implicant(const Minterm minterm) : bits(minterm), mask(maxMinterm) {}
	constexpr Implicant(const Implicant &) = default;
	constexpr Implicant& operator=(const Implicant &) = default;
	
	constexpr bool operator==(const Implicant &other) const = default;
	constexpr bool operator!=(const Implicant &other) const = default;
	inline bool operator<(const Implicant &other) const;
	bool humanLess(const Implicant &other) const;
	constexpr bool covers(const Minterm minterm) const { return (minterm & mask) == bits; }
	
	constexpr bool isEmpty() const { return mask == 0; }
	constexpr bool isEmptyTrue() const { return bits == 0; }
	constexpr mask_t getRawBits() const { return bits; }
	constexpr mask_t getRawMask() const { return mask; }
	constexpr mask_t getTrueBits() const { return bits & mask; }
	constexpr mask_t getFalseBits() const { return ~bits & mask; }
	bits_t getBitCount() const { return static_cast<bits_t>(std::bitset<::maxBits>(mask).count()); } // constexpr since C++23
	splitBits_t splitBits() const;
	
	void add(const Implicant &other) { this->bits |= other.bits; this->mask |= other.mask; }
	void substract(const Implicant &other) { this->bits &= ~other.bits; this->mask &= ~other.mask; }
	void setBit(const bits_t bit, const bool value) { const mask_t bitMask = 1 << (::bits - bit - 1); if (value) bits |= bitMask; mask |= bitMask; }
	void unsetBit(const bits_t bit) { const mask_t bitMask = ~(1 << (::bits - bit - 1)); bits &= bitMask; mask &= bitMask; }
	void applyMask(const mask_t maskToApply) { bits &= maskToApply; mask &= maskToApply; }
	
	constexpr Minterm firstMinterm() const { return bits; }
	Minterm nextMinterm(const Minterm minterm) const;
	Minterm lastMinterm() const { return bits | (~mask & ::maxMinterm); }
	void addToMinterms(Minterms &minterms) const;
	void removeFromMinterms(Minterms &minterms) const;
	
	static Implicant findBiggestInUnion(const Implicant &x, const Implicant &y);
	bool contains(const Implicant &other) const { return (this->mask & other.mask) == this->mask && (this->mask & other.bits) == this->bits; }
	
	void printHuman(std::ostream &o, const bool parentheses) const;
	void printVerilog(std::ostream &o, const bool parentheses) const;
	void printVhdl(std::ostream &o, const bool parentheses) const;
	void printCpp(std::ostream &o, const bool parentheses) const;
	void printMath(std::ostream &o, const bool parentheses) const;
};

bool Implicant::operator<(const Implicant &other) const
{
	static_assert(sizeof(mask_t) == sizeof(std::uint32_t));
	using comp_t = std::uint_fast64_t;
	const comp_t x = (static_cast<comp_t>(this->mask) << 32) | this->bits;
	const comp_t y = (static_cast<comp_t>(other.mask) << 32) | other.bits;
	return x < y;
}
