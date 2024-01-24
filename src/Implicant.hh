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
	class ConstIterator
	{
		const Minterm bits;
		const Minterm inversedMask;
		Minterm unmaskedPart = 0;
		bool end;
	protected:
		ConstIterator(const Minterm bits, const Minterm mask) : bits(bits), inversedMask(~mask & ::maxMinterm), end(false) {}
		constexpr ConstIterator() : bits(0), inversedMask(0), end(true) {}
		friend class Implicant;
	public:
		[[nodiscard]] constexpr bool operator==(const ConstIterator &other) const { return this->end == other.end && this->unmaskedPart == other.unmaskedPart; }
		[[nodiscard]] constexpr bool operator!=(const ConstIterator &other) const { return !(*this == other); }
		[[nodiscard]] constexpr Minterm operator*() const { return bits | unmaskedPart; }
		ConstIterator& operator++() { unmaskedPart = (unmaskedPart - inversedMask) & inversedMask; if (unmaskedPart == 0) [[unlikely]] end = true; return *this; }
	};
	
	constexpr Implicant(const mask_t bits, const mask_t mask) : bits(bits), mask(mask) {}
	explicit Implicant(const Minterm minterm) : bits(minterm), mask(maxMinterm) {}
	constexpr Implicant(const Implicant &) = default;
	constexpr Implicant& operator=(const Implicant &) = default;
	
	[[nodiscard]] constexpr bool operator==(const Implicant &other) const = default;
	[[nodiscard]] constexpr bool operator!=(const Implicant &other) const = default;
	[[nodiscard]] inline bool operator<(const Implicant &other) const;
	[[nodiscard]] bool humanLess(const Implicant &other) const;
	[[nodiscard]] constexpr bool covers(const Minterm minterm) const { return (minterm & mask) == bits; }
	
	[[nodiscard]] constexpr bool isEmpty() const { return mask == 0; }
	[[nodiscard]] constexpr bool isEmptyTrue() const { return bits == 0; }
	[[nodiscard]] constexpr mask_t getBits() const { return bits; }
	[[nodiscard]] constexpr mask_t getMask() const { return mask; }
	[[nodiscard]] constexpr mask_t getTrueBits() const { return bits & mask; }
	[[nodiscard]] constexpr mask_t getFalseBits() const { return ~bits & mask; }
	[[nodiscard]] bits_t getBitCount() const { return static_cast<bits_t>(std::bitset<::maxBits>(mask).count()); } // constexpr since C++23
	[[nodiscard]] splitBits_t splitBits() const;
	
	void add(const Implicant &other) { this->bits |= other.bits; this->mask |= other.mask; }
	void substract(const Implicant &other) { this->bits &= ~other.bits; this->mask &= ~other.mask; }
	void setBit(const bits_t bit, const bool value) { const mask_t bitMask = 1 << (::bits - bit - 1); if (value) bits |= bitMask; mask |= bitMask; }
	void unsetBit(const bits_t bit) { const mask_t bitMask = ~(1 << (::bits - bit - 1)); bits &= bitMask; mask &= bitMask; }
	void applyMask(const mask_t maskToApply) { bits &= maskToApply; mask &= maskToApply; }
	
	[[nodiscard]] bool isAnyInMinterms(const Minterms &minterms) const;
	[[nodiscard]] bool areAllInMinterms(const Minterms &minterms) const;
	void addToMinterms(Minterms &minterms) const;
	void removeFromMinterms(Minterms &minterms) const;
	
	static inline Implicant findBiggestInUnion(const Implicant &x, const Implicant &y);
	[[nodiscard]] bool contains(const Implicant &other) const { return (this->mask & other.mask) == this->mask && (this->mask & other.bits) == this->bits; }
	
	[[nodiscard]] ConstIterator begin() const { if (isEmpty() && !isEmptyTrue()) [[unlikely]] return {}; else return {bits, mask}; }
	[[nodiscard]] constexpr ConstIterator end() const { return {}; }
	[[nodiscard]] ConstIterator cbegin() const { return begin(); }
	[[nodiscard]] constexpr ConstIterator cend() const { return end(); }
	
	void printHuman(std::ostream &o, const bool parentheses) const;
	void printVerilog(std::ostream &o, const bool parentheses) const;
	void printVhdl(std::ostream &o, const bool parentheses) const;
	void printCpp(std::ostream &o, const bool parentheses) const;
	void printMath(std::ostream &o, const bool parentheses) const;
};

bool Implicant::operator<(const Implicant &other) const
{
	static_assert(::maxBits == 32);
	using comp_t = std::uint_fast64_t;
	const comp_t x = (static_cast<comp_t>(this->mask) << 32) | this->bits;
	const comp_t y = (static_cast<comp_t>(other.mask) << 32) | other.bits;
	return x < y;
}

Implicant Implicant::findBiggestInUnion(const Implicant &x, const Implicant &y)
{
	const mask_t conflicts = (x.bits ^ y.bits) & (x.mask & y.mask);
	const bits_t conflictCount = static_cast<bits_t>(std::bitset<::maxBits>(conflicts).count());
	return conflictCount == 1  // Implicants are "touching". ("1" means touching; "0" means intersecting.)
		? Implicant((x.bits | y.bits) & ~conflicts, (x.mask | y.mask) & ~conflicts)
		: none();
}
