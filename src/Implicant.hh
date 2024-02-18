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
	
	constexpr std::uint_fast64_t makeComparisonValue() const;
	
public:
	constexpr Implicant(const mask_t bits, const mask_t mask) : bits(bits), mask(mask) {}
	explicit Implicant(const Minterm minterm) : bits(minterm), mask(maxMinterm) {}
	constexpr Implicant(const Implicant &) = default;
	constexpr Implicant& operator=(const Implicant &) = default;
	
	constexpr bool operator==(const Implicant &other) const = default;
	constexpr auto operator<=>(const Implicant &other) const { return this->makeComparisonValue() <=> other.makeComparisonValue(); }
	bool humanLess(const Implicant &other) const;
	constexpr bool covers(const Minterm minterm) const { return (minterm & mask) == bits; }
	
	constexpr bool empty() const { return mask == 0; }
	constexpr mask_t getBits() const { return bits; }
	constexpr mask_t getMask() const { return mask; }
	constexpr mask_t getTrueBits() const { return bits & mask; }
	constexpr mask_t getFalseBits() const { return ~bits & mask; }
	bits_t getBitCount() const { return static_cast<bits_t>(std::bitset<::maxBits>(mask).count()); } // constexpr since C++23
	bits_t getTrueBitCount() const { return static_cast<bits_t>(std::bitset<::maxBits>(getTrueBits()).count()); } // constexpr since C++23
	bits_t getFalseBitCount() const { return static_cast<bits_t>(std::bitset<::maxBits>(getFalseBits()).count()); } // constexpr since C++23
	splitBits_t splitBits() const;
	
	void add(const Implicant &other) { this->bits |= other.bits; this->mask |= other.mask; }
	void substract(const Implicant &other) { this->bits &= ~other.bits; this->mask &= ~other.mask; }
	void setBit(const bits_t bit, const bool value) { const mask_t bitMask = 1 << (::bits - bit - 1); if (value) bits |= bitMask; mask |= bitMask; }
	void unsetBit(const bits_t bit) { const mask_t bitMask = ~(1 << (::bits - bit - 1)); bits &= bitMask; mask &= bitMask; }
	void applyMask(const mask_t maskToApply) { bits &= maskToApply; mask &= maskToApply; }
	
	bool isAnyInMinterms(const Minterms &minterms) const;
	bool areAllInMinterms(const Minterms &minterms) const;
	void addToMinterms(Minterms &minterms) const;
	void removeFromMinterms(Minterms &minterms) const;
	
	static inline Implicant findBiggestInUnion(const Implicant &x, const Implicant &y);
	bool contains(const Implicant &other) const { return (this->mask & other.mask) == this->mask && (this->mask & other.bits) == this->bits; }
	
	void printHuman(std::ostream &o, const bool parentheses) const;
	void printVerilog(std::ostream &o, const bool parentheses) const;
	void printVhdl(std::ostream &o, const bool parentheses) const;
	void printCpp(std::ostream &o, const bool parentheses) const;
	void printMath(std::ostream &o, const bool parentheses) const;
};

constexpr std::uint_fast64_t Implicant::makeComparisonValue() const
{
	static_assert(::maxBits == 32);
	return (static_cast<std::uint_fast64_t>(this->mask) << 32) | this->bits;
}

Implicant Implicant::findBiggestInUnion(const Implicant &x, const Implicant &y)
{
	const mask_t conflicts = (x.bits ^ y.bits) & (x.mask & y.mask);
	const bits_t conflictCount = static_cast<bits_t>(std::bitset<::maxBits>(conflicts).count());
	return conflictCount == 1  // Implicants are "touching". ("1" means touching; "0" means intersecting.)
		? Implicant((x.bits | y.bits) & ~conflicts, (x.mask | y.mask) & ~conflicts)
		: none();
}
