#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <functional>
#include <iterator>
#include <limits>
#include <vector>


// This container can store unique numbers in range 0..capacity-1 using only about capacity/CHAR_BIT bytes of memory.
// (The memory usage is the same regardles of how many numbers are stored.)
template<std::unsigned_integral T>
class CompactSet
{
public:
	using size_type = std::vector<bool>::size_type;
	
private:
	std::vector<bool> bits;
	size_type size_ = 0;
	
public:
	class const_iterator
	{
		const CompactSet<T> *compactSet = nullptr;
		size_type i;
		
		const_iterator(const CompactSet &compactSet, const size_type i) : compactSet(&compactSet), i(i) { }
		friend class CompactSet<T>;
		
	public:
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::bidirectional_iterator_tag;
		
		const_iterator() = default;
		
		[[nodiscard]] bool operator==(const const_iterator &other) const { return this->i == other.i; }
		[[nodiscard]] bool operator!=(const const_iterator &other) const { return this->i != other.i; }
		[[nodiscard]] bool operator<(const const_iterator &other) const { return this->i < other.i; }
		[[nodiscard]] bool operator<=(const const_iterator &other) const { return this->i <= other.i; }
		[[nodiscard]] bool operator>(const const_iterator &other) const { return this->i > other.i; }
		[[nodiscard]] bool operator>=(const const_iterator &other) const { return this->i >= other.i; }
		[[nodiscard]] auto operator<=>(const const_iterator &other) const { return this->i <=> other.i; }
		
		inline const_iterator& operator++();
		[[nodiscard]] const_iterator operator++(int) const { const_iterator copy = *this; ++copy; return copy; }
		inline const_iterator& operator--();
		[[nodiscard]] const_iterator operator--(int) const { const_iterator copy = *this; --copy; return copy; }
		
		[[nodiscard]] T operator*() const { return static_cast<T>(i); }
	};
	static_assert(std::bidirectional_iterator<const_iterator>);
	
	explicit inline CompactSet(const size_type capacity);
	
	[[nodiscard]] bool operator==(const CompactSet &other) const { return this->size_ == other.size_ && this->bits == other.bits; }
	[[nodiscard]] bool operator!=(const CompactSet &other) const { return !(*this == other); }
	
	[[nodiscard]] bool empty() const { return size_ == 0; }
	[[nodiscard]] bool full() const { return size_ == bits.size(); }
	[[nodiscard]] size_type size() const { return size_; }
	[[nodiscard]] size_type max_size() const { return bits.size(); }
	[[nodiscard]] size_type count(const T value) const { return bits[value] ? 1 : 0; }
	
	inline bool add(const T value);
	inline void add(const CompactSet &other);
	inline void add(const CompactSet &other, const size_type overlappingCount);
	inline bool remove(const T value);
	
	[[nodiscard]] inline const_iterator begin() const;
	[[nodiscard]] const_iterator end() const { return {*this, bits.size()}; }
	[[nodiscard]] const_iterator cbegin() const { return begin(); }
	[[nodiscard]] const_iterator cend() const { return end(); }
	
#ifndef NDEBUG
	void validate() const;
#endif
};


template<std::unsigned_integral T>
typename CompactSet<T>::const_iterator& CompactSet<T>::const_iterator::operator++()
{
	for (++i; i != compactSet->bits.size() && !compactSet->bits[i]; ++i) { }
	return *this;
}

template<std::unsigned_integral T>
typename CompactSet<T>::const_iterator& CompactSet<T>::const_iterator::operator--()
{
	for (--i; !compactSet->bits[i]; --i) { }
	return *this;
}

template<std::unsigned_integral T>
CompactSet<T>::CompactSet(const size_type capacity) :
	bits(capacity, false)
{
	assert(capacity == 0 || capacity - 1 <= std::numeric_limits<T>::max());
	assert(capacity <= std::numeric_limits<size_type>::max());  // Otherwise, iterators won't work.
}

template<std::unsigned_integral T>
bool CompactSet<T>::add(const T value)
{
	const bool previous = bits[value];
	if (!previous)
	{
		bits[value] = true;
		++size_;
	}
	return !previous;
}

template<std::unsigned_integral T>
void CompactSet<T>::add(const CompactSet &other)
{
	for (const T value : other)
		add(value);
}

template<std::unsigned_integral T>
void CompactSet<T>::add(const CompactSet &other, const size_type overlappingCount)
{
	std::transform(
			other.bits.cbegin(), other.bits.cend(),
			this->bits.begin(), this->bits.begin(),
			std::logical_or<bool>());
	this->size_ += other.size_ - overlappingCount;
}

template<std::unsigned_integral T>
bool CompactSet<T>::remove(const T value)
{
	const bool previous = bits[value];
	if (previous)
	{
		bits[value] = false;
		--size_;
	}
	return previous;
}

template<std::unsigned_integral T>
typename CompactSet<T>::const_iterator CompactSet<T>::begin() const
{
	const_iterator iter{*this, 0};
	if (!bits.empty() && !bits[0])
		++iter;
	return iter;
}
