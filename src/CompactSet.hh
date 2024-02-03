#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <functional>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>


// This container can store unique numbers in range 0..capacity-1 using only about capacity/CHAR_BIT bytes of memory.
// (The memory usage is the same regardles of how many numbers are stored.)
template<std::unsigned_integral T>
class CompactSet
{
public:
	using value_type = T;
	using size_type = std::vector<bool>::size_type;
	
private:
	std::vector<bool> bits;
	size_type size_ = 0;
	
public:
	class iterator
	{
		const std::vector<bool> *bits = nullptr;
		size_type i;
		
		iterator(const std::vector<bool> &bits, const size_type i) : bits(&bits), i(i) { }
		friend class CompactSet<T>;
		
	public:
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::bidirectional_iterator_tag;
		
		iterator() = default;
		
		[[nodiscard]] bool operator==(const iterator &other) const { return this->i == other.i; }
		[[nodiscard]] bool operator!=(const iterator &other) const { return this->i != other.i; }
		[[nodiscard]] bool operator<(const iterator &other) const { return this->i < other.i; }
		[[nodiscard]] bool operator<=(const iterator &other) const { return this->i <= other.i; }
		[[nodiscard]] bool operator>(const iterator &other) const { return this->i > other.i; }
		[[nodiscard]] bool operator>=(const iterator &other) const { return this->i >= other.i; }
		[[nodiscard]] auto operator<=>(const iterator &other) const { return this->i <=> other.i; }
		
		inline iterator& operator++();
		[[nodiscard]] iterator operator++(int) const { iterator copy = *this; ++copy; return copy; }
		inline iterator& operator--();
		[[nodiscard]] iterator operator--(int) const { iterator copy = *this; --copy; return copy; }
		
		[[nodiscard]] value_type operator*() const { return static_cast<value_type>(i); }
	};
	static_assert(std::bidirectional_iterator<iterator>);
	using const_iterator = iterator;
	
	explicit inline CompactSet(const size_type capacity);
	
	[[nodiscard]] bool operator==(const CompactSet &other) const { return this->size_ == other.size_ && this->bits == other.bits; }
	[[nodiscard]] bool operator!=(const CompactSet &other) const { return !(*this == other); }
	
	[[nodiscard]] bool empty() const { return size_ == 0; }
	[[nodiscard]] bool full() const { return size_ == bits.size(); }
	[[nodiscard]] size_type size() const { return size_; }
	[[nodiscard]] size_type max_size() const { return bits.size(); }
	
	void clear() { std::fill(bits.begin(), bits.end(), false); size_ = 0; }
	inline std::pair<iterator, bool> insert(const value_type value);
	iterator insert(const const_iterator, const value_type value) { return insert(value).first; }
	template<class InputIt>
	inline void insert(const InputIt first, const InputIt last);
	void insert(std::initializer_list<value_type> ilist) { return insert(ilist.begin(), ilist.end()); }
	inline void insert(const CompactSet &other);
	inline void unsafe_insert(const CompactSet &other, const size_type overlappingCount);  // This is needed to cut off a few seconds at max input size.
	template<class... Args>
	std::pair<iterator, bool> emplace(Args&&... args) { return insert(value_type(std::forward<Args>(args)...)); }
	template<class... Args>
	iterator emplace_hint(const const_iterator, Args&&... args) { return insert(value_type(std::forward<Args>(args)...)).first; }
	inline iterator erase(const_iterator pos);
	inline iterator erase(const const_iterator first, const const_iterator last);
	inline size_type erase(const value_type value);
	void swap(CompactSet &other) { std::ranges::swap(this->bits, other.bits); std::ranges::swap(this->size_, other.size_); }
	
	[[nodiscard]] size_type count(const value_type value) const { return bits[value] ? 1 : 0; }
	[[nodiscard]] const_iterator find(const value_type value) const { return bits[value] ? iterator(bits, value) : end(); }
	[[nodiscard]] bool contains(const value_type value) const { return bits[value]; }
	[[nodiscard]] inline std::pair<const_iterator, const_iterator> equal_range(const value_type value) const;
	[[nodiscard]] inline const_iterator lower_bound(const value_type value) const;
	[[nodiscard]] const_iterator upper_bound(const value_type value) const;
	
	[[nodiscard]] inline iterator begin() const;
	[[nodiscard]] iterator end() const { return {this->bits, bits.size()}; }
	[[nodiscard]] const_iterator cbegin() const { return begin(); }
	[[nodiscard]] const_iterator cend() const { return end(); }
	
#ifndef NDEBUG
	void validate() const;
#endif
};


template<std::unsigned_integral T>
typename CompactSet<T>::iterator& CompactSet<T>::iterator::operator++()
{
	for (++i; i != bits->size() && !(*bits)[i]; ++i) { }
	return *this;
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator& CompactSet<T>::iterator::operator--()
{
	for (--i; !(*bits)[i]; --i) { }
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
std::pair<typename CompactSet<T>::iterator, bool> CompactSet<T>::insert(const value_type value)
{
	const bool previous = bits[value];
	if (!previous)
	{
		bits[value] = true;
		++size_;
	}
	return {{this->bits, value}, !previous};
}

template<std::unsigned_integral T>
template<class InputIt>
void CompactSet<T>::insert(const InputIt first, const InputIt last)
{
	for (InputIt current = first; current != last; ++current)
		insert(*current);
}

template<std::unsigned_integral T>
void CompactSet<T>::unsafe_insert(const CompactSet &other, const size_type overlappingCount)
{
	std::transform(
			other.bits.cbegin(), other.bits.cend(),
			this->bits.begin(), this->bits.begin(),
			std::logical_or<bool>());
	this->size_ += other.size_ - overlappingCount;
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator CompactSet<T>::erase(const_iterator pos)
{
	bits[*pos];
	--size_;
	++pos;
	return pos;
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator CompactSet<T>::erase(const const_iterator first, const const_iterator last)
{
	for (const_iterator current = first; current != last; ++current)
	{
		bits[*current];
		--size_;
	}
	return last;
}

template<std::unsigned_integral T>
typename CompactSet<T>::size_type CompactSet<T>::erase(const value_type value)
{
	const bool previous = bits[value];
	if (previous)
	{
		bits[value] = false;
		--size_;
	}
	return previous ? 1 : 0;
}

template<std::unsigned_integral T>
std::pair<typename CompactSet<T>::iterator, typename CompactSet<T>::iterator> CompactSet<T>::equal_range(const value_type value) const
{
	iterator iter(bits, value);
	if (bits[value])
		return {iter, std::ranges::next(iter)};
	++iter;
	return {iter, iter};
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator CompactSet<T>::lower_bound(const value_type value) const
{
	iterator iter(bits, value);
	if (!bits[value])
		++iter;
	return iter;
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator CompactSet<T>::upper_bound(const value_type value) const
{
	iterator iter(bits, value);
	return ++iter;
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator CompactSet<T>::begin() const
{
	iterator iter{this->bits, 0};
	if (!bits.empty() && !bits[0])
		++iter;
	return iter;
}
