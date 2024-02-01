#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <vector>


// This container can store unique numbers in range 0..capacity-1 using only about capacity/CHAR_BIT bytes of memory.
// (The memory usage is the same regardles of how many numbers are stored.)
template<std::unsigned_integral T>
class CompactSet
{
	std::vector<bool> bitset;
	std::size_t count = 0;
	
public:
	class const_iterator
	{
		const CompactSet<T> &compactSet;
		std::uint_fast64_t i;
		const_iterator(const CompactSet &compactSet, const std::uint_fast64_t i) : compactSet(compactSet), i(i) { }
		friend class CompactSet<T>;
	public:
		[[nodiscard]] bool operator==(const const_iterator &other) const { return this->i == other.i; }
		[[nodiscard]] bool operator!=(const const_iterator &other) const { return this->i != other.i; }
		inline const_iterator& operator++();
		inline const_iterator& operator--();
		[[nodiscard]] T operator*() const { return static_cast<T>(i); }
	};
	
	explicit inline CompactSet(const std::size_t capacity);
	
	[[nodiscard]] bool operator==(const CompactSet &other) const { return this->bitset == other.bitset; }
	[[nodiscard]] bool operator!=(const CompactSet &other) const { return this->bitset != other.bitset; }
	
	[[nodiscard]] bool empty() const { return count == 0; }
	[[nodiscard]] bool full() const { return count == bitset.size(); }
	[[nodiscard]] std::size_t size() const { return count; }
	[[nodiscard]] std::size_t capacity() const { return bitset.size(); }
	[[nodiscard]] bool check(const T value) const { return bitset[value]; }
	inline bool add(const T value);
	inline void add(const CompactSet &other);
	inline void add(const CompactSet &other, const std::size_t overlappingCount);
	inline bool remove(const T value);
	
	[[nodiscard]] inline const_iterator begin() const;
	[[nodiscard]] const_iterator end() const { return {*this, bitset.size()}; }
	[[nodiscard]] const_iterator cbegin() const { return begin(); }
	[[nodiscard]] const_iterator cend() const { return end(); }
	
#ifndef NDEBUG
	void validate() const;
#endif
};


template<typename T>
typename CompactSet<T>::const_iterator& CompactSet<T>::const_iterator::operator++()
{
	for (++i; i != compactSet.bitset.size() && !compactSet.bitset[i]; ++i) { }
	return *this;
}

template<typename T>
typename CompactSet<T>::const_iterator& CompactSet<T>::const_iterator::operator--()
{
	for (--i; !compactSet.bitset[i]; --i) { }
	return *this;
}

template<typename T>
CompactSet<T>::CompactSet(const std::size_t capacity) :
	bitset(capacity, false)
{
	assert(capacity == 0 || capacity - 1 <= std::numeric_limits<T>::max());
	assert(capacity <= std::numeric_limits<std::uint_fast64_t>::max());  // Otherwise, iterators won't work.
}

template<typename T>
bool CompactSet<T>::add(const T value)
{
	const bool previous = check(value);
	if (!previous)
	{
		bitset[value] = true;
		++count;
	}
	return !previous;
}

template<typename T>
void CompactSet<T>::add(const CompactSet &other)
{
	for (const T value : other)
		add(value);
}

template<typename T>
void CompactSet<T>::add(const CompactSet &other, const std::size_t overlappingCount)
{
	std::transform(
			other.bitset.cbegin(), other.bitset.cend(),
			this->bitset.begin(), this->bitset.begin(),
			std::logical_or<bool>());
	this->count += other.count - overlappingCount;
}

template<typename T>
bool CompactSet<T>::remove(const T value)
{
	const bool previous = check(value);
	if (previous)
	{
		bitset[value] = false;
		--count;
	}
	return previous;
}

template<typename T>
typename CompactSet<T>::const_iterator CompactSet<T>::begin() const
{
	const_iterator iter{*this, 0};
	if (!bitset.empty() && !bitset[0])
		++iter;
	return iter;
}
