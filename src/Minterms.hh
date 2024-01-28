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
	using overlapping_t = std::vector<Minterm>;
	
	class ConstIterator
	{
		const Minterms &minterms;
		std::uint_fast64_t i;
		ConstIterator(const Minterms &minterms, const std::uint_fast64_t i) : minterms(minterms), i(i) {}
		friend class Minterms;
	public:
		[[nodiscard]] bool operator==(const ConstIterator &other) const { return this->i == other.i; }
		[[nodiscard]] bool operator!=(const ConstIterator &other) const { return this->i != other.i; }
		inline ConstIterator& operator++();
		inline ConstIterator& operator--();
		[[nodiscard]] Minterm operator*() const { return static_cast<Minterm>(i); }
	};
	
	Minterms() : bitset(static_cast<std::uint_fast64_t>(::maxMinterm) + 1, false) {}
	
	[[nodiscard]] bool operator==(const Minterms &other) const { return this->bitset == other.bitset; }
	[[nodiscard]] bool operator!=(const Minterms &other) const { return this->bitset != other.bitset; }
	
	[[nodiscard]] bool isEmpty() const { return size == 0; }
	[[nodiscard]] bool isFull() const { return size - 1 == ::maxMinterm; }
	[[nodiscard]] std::size_t getSize() const { return size; }
	[[nodiscard]] std::size_t getCapacity() const { return bitset.size(); }
	[[nodiscard]] bool check(const Minterm minterm) const { return bitset[minterm]; }
	[[nodiscard]] inline overlapping_t findOverlapping(const Minterms &other) const;  // Optimized for when there is little to none of them.
	inline bool add(const Minterm minterm);
	inline void add(const Minterms &other, const std::size_t overlappingCount);
	inline bool remove(const Minterm minterm);
	
	[[nodiscard]] inline ConstIterator begin() const;
	[[nodiscard]] ConstIterator end() const { return {*this, bitset.size()}; }
	[[nodiscard]] ConstIterator cbegin() const { return begin(); }
	[[nodiscard]] ConstIterator cend() const { return end(); }
	
#ifndef NDEBUG
	void validate() const;
#endif
};


Minterms::ConstIterator& Minterms::ConstIterator::operator++()
{
	for (++i; i != minterms.bitset.size() && !minterms.bitset[i]; ++i) { }
	return *this;
}

Minterms::ConstIterator& Minterms::ConstIterator::operator--()
{
	for (--i; !minterms.bitset[i]; --i) { }
	return *this;
}

Minterms::overlapping_t Minterms::findOverlapping(const Minterms &other) const
{
	overlapping_t overlapping;
	std::set_intersection(
			this->cbegin(), this->cend(),
			other.cbegin(), other.cend(),
			std::back_inserter(overlapping));
	return overlapping;
}

bool Minterms::add(const Minterm minterm)
{
	const bool previous = check(minterm);
	if (!previous)
	{
		bitset[minterm] = true;
		++size;
	}
	return !previous;
}

void Minterms::add(const Minterms &other, const std::size_t overlappingCount)
{
	std::transform(
			other.bitset.begin(), other.bitset.end(),
			this->bitset.begin(), this->bitset.begin(),
			std::logical_or<bool>());
	this->size += other.size - overlappingCount;
}

bool Minterms::remove(const Minterm minterm)
{
	const bool previous = check(minterm);
	if (previous)
	{
		bitset[minterm] = false;
		--size;
	}
	return previous;
}

Minterms::ConstIterator Minterms::begin() const
{
	ConstIterator iter{*this, 0};
	if (!bitset.empty() && !bitset[0])
		++iter;
	return iter;
}
