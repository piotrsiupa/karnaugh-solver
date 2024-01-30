#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <vector>


// This container can store unique numbers in range 0..capacity-1 using only about capacity/CHAR_BIT bytes of memory.
// (The memory usage is the same regardles of how many numbers are stored.)
template<typename T>
class CompactSet
{
	static_assert(!std::numeric_limits<T>::is_signed);
	
	std::vector<bool> bitset;
	std::size_t size = 0;
	
public:
	class ConstIterator
	{
		const CompactSet<T> &compactSet;
		std::uint_fast64_t i;
		ConstIterator(const CompactSet &compactSet, const std::uint_fast64_t i) : compactSet(compactSet), i(i) { }
		friend class CompactSet<T>;
	public:
		[[nodiscard]] bool operator==(const ConstIterator &other) const { return this->i == other.i; }
		[[nodiscard]] bool operator!=(const ConstIterator &other) const { return this->i != other.i; }
		inline ConstIterator& operator++();
		inline ConstIterator& operator--();
		[[nodiscard]] T operator*() const { return static_cast<T>(i); }
	};
	
	explicit inline CompactSet(const std::size_t capacity);
	
	[[nodiscard]] bool operator==(const CompactSet &other) const { return this->bitset == other.bitset; }
	[[nodiscard]] bool operator!=(const CompactSet &other) const { return this->bitset != other.bitset; }
	
	[[nodiscard]] bool isEmpty() const { return size == 0; }
	[[nodiscard]] bool isFull() const { return size == bitset.size(); }
	[[nodiscard]] std::size_t getSize() const { return size; }
	[[nodiscard]] std::size_t getCapacity() const { return bitset.size(); }
	[[nodiscard]] bool check(const T value) const { return bitset[value]; }
	inline bool add(const T value);
	inline void add(const CompactSet &other, const std::size_t overlappingCount);
	inline bool remove(const T value);
	
	[[nodiscard]] inline ConstIterator begin() const;
	[[nodiscard]] ConstIterator end() const { return {*this, bitset.size()}; }
	[[nodiscard]] ConstIterator cbegin() const { return begin(); }
	[[nodiscard]] ConstIterator cend() const { return end(); }
	
#ifndef NDEBUG
	void validate() const;
#endif
};


template<typename T>
CompactSet<T>::ConstIterator& CompactSet<T>::ConstIterator::operator++()
{
	for (++i; i != compactSet.bitset.size() && !compactSet.bitset[i]; ++i) { }
	return *this;
}

template<typename T>
CompactSet<T>::ConstIterator& CompactSet<T>::ConstIterator::operator--()
{
	for (--i; !compactSet.bitset[i]; --i) { }
	return *this;
}

template<typename T>
CompactSet<T>::CompactSet(const std::size_t capacity) :
	bitset(capacity, false)
{
	assert(capacity == 0 || capacity - 1 <= std::numeric_limits<T>::max());
	assert(capacity < std::numeric_limits<T>::max());  // Otherwise, iterators won't work.
}

template<typename T>
bool CompactSet<T>::add(const T value)
{
	const bool previous = check(value);
	if (!previous)
	{
		bitset[value] = true;
		++size;
	}
	return !previous;
}

template<typename T>
void CompactSet<T>::add(const CompactSet &other, const std::size_t overlappingCount)
{
	std::transform(
			other.bitset.begin(), other.bitset.end(),
			this->bitset.begin(), this->bitset.begin(),
			std::logical_or<bool>());
	this->size += other.size - overlappingCount;
}

template<typename T>
bool CompactSet<T>::remove(const T value)
{
	const bool previous = check(value);
	if (previous)
	{
		bitset[value] = false;
		--size;
	}
	return previous;
}

template<typename T>
CompactSet<T>::ConstIterator CompactSet<T>::begin() const
{
	ConstIterator iter{*this, 0};
	if (!bitset.empty() && !bitset[0])
		++iter;
	return iter;
}
