#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#include "global.hh"
#include "Minterm.hh"


class Minterms
{
	std::vector<bool> bitset;
	std::size_t size = 0;
	
public:
	using duplicates_t = std::vector<Minterm>;
	
	class ConstIterator
	{
		const Minterms &minterms;
		std::uint_fast64_t i;
		ConstIterator(const Minterms &minterms, const std::uint_fast64_t i) : minterms(minterms), i(i) {}
		friend class Minterms;
	public:
		bool operator==(const ConstIterator &other) { return this->i == other.i; }
		bool operator!=(const ConstIterator &other) { return this->i != other.i; }
		ConstIterator& operator++() { for (++i; i != minterms.bitset.size() && !minterms.bitset[i]; ++i) {} return *this; }
		ConstIterator& operator--() { for (--i; i != 0 && !minterms.bitset[i]; --i) {} return *this; }
		Minterm operator*() const { return i; }
	};
	
	Minterms() : bitset(static_cast<std::uint_fast64_t>(::maxMinterm) + 1, false) {}
	
	[[nodiscard]] bool operator==(const Minterms &other) const { return this->bitset == other.bitset; }
	[[nodiscard]] bool operator!=(const Minterms &other) const { return this->bitset != other.bitset; }
	duplicates_t findDuplicates(const Minterms &other) const { duplicates_t duplicates; std::set_intersection(this->cbegin(), this->cend(), other.cbegin(), other.cend(), std::back_inserter(duplicates)); return duplicates; }
	Minterms& operator|=(const Minterms &other) { std::transform(other.bitset.begin(), other.bitset.end(), this->bitset.begin(), this->bitset.begin(), std::logical_or<bool>()); return *this; }
	
	[[nodiscard]] std::size_t getSize() const { return size; }
	[[nodiscard]] bool check(const Minterm minterm) const { return bitset[minterm]; }
	bool add(const Minterm minterm) { const bool previous = check(minterm); bitset[minterm] = true; if (!previous) ++size; return !previous; }
	bool remove(const Minterm minterm) { const bool previous = check(minterm); bitset[minterm] = false; if (previous) --size; return previous; }
	
	ConstIterator begin() const { ConstIterator iter{*this, 0}; if (!bitset.empty() && !bitset[0]) ++iter; return iter; }
	ConstIterator end() const { return {*this, bitset.size()}; }
	ConstIterator cbegin() const { return begin(); }
	ConstIterator cend() const { return end(); }
	Minterm front() const { return *begin(); }
	Minterm back() const { ConstIterator iter = end(); --iter; return *iter; }
};
