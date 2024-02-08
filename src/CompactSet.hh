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
	using key_type = T;
	using value_type = T;
	using size_type = std::vector<bool>::size_type;
	using difference_type = std::vector<bool>::difference_type;
	using key_compare = std::less<key_type>;
	using value_compare = std::less<value_type>;
	using pointer = T*;
	using reference = T&;
	
private:
	std::vector<bool> bits;
	size_type size_ = 0;
	
public:
	class iterator
	{
		const std::vector<bool> *bits = nullptr;
		size_type i = 0;
		
		iterator(const std::vector<bool> &bits, const size_type i) noexcept : bits(&bits), i(i) { }
		friend class CompactSet<T>;
		
	public:
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::bidirectional_iterator_tag;
		
		iterator() noexcept = default;
		
		[[nodiscard]] bool operator==(const iterator &other) const noexcept { return this->i == other.i; }
		[[nodiscard]] auto operator<=>(const iterator &other) const noexcept { return this->i <=> other.i; }
		
		inline iterator& operator++() noexcept;
		[[nodiscard]] iterator operator++(int) const noexcept { iterator copy = *this; ++copy; return copy; }
		inline iterator& operator--() noexcept;
		[[nodiscard]] iterator operator--(int) const noexcept { iterator copy = *this; --copy; return copy; }
		
		[[nodiscard]] value_type operator*() const noexcept { return static_cast<value_type>(i); }
	};
	static_assert(std::bidirectional_iterator<iterator>);
	using const_iterator = iterator;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = reverse_iterator;
	
	explicit inline CompactSet(const size_type capacity);
	
	[[nodiscard]] inline bool operator==(const CompactSet &other) const noexcept;
	[[nodiscard]] inline auto operator<=>(const CompactSet &other) const noexcept;
	
	[[nodiscard]] CompactSet operator~() const { CompactSet copy = *this; copy.bits.flip(); copy.size_ = max_size() - size_; return copy; }
	
	[[nodiscard]] bool empty() const noexcept { return size_ == 0; }
	[[nodiscard]] bool full() const noexcept { return size_ == bits.size(); }
	[[nodiscard]] size_type size() const noexcept { return size_; }
	[[nodiscard]] size_type max_size() const noexcept { return bits.size(); }
	
	void clear() noexcept { std::fill(bits.begin(), bits.end(), false); size_ = 0; }
	inline std::pair<iterator, bool> insert(const value_type value) noexcept;
	iterator insert(const const_iterator, const value_type value) noexcept { return insert(value).first; }
	template<class InputIt>
	inline void insert(const InputIt first, const InputIt last) noexcept;
	void insert(std::initializer_list<value_type> ilist) noexcept { return insert(ilist.begin(), ilist.end()); }
	inline void unsafe_insert(const CompactSet &other, const size_type overlappingCount) noexcept;  // This is needed to cut off a few seconds at max input size.
	template<class... Args>
	std::pair<iterator, bool> emplace(Args&&... args) noexcept { return insert(value_type(std::forward<Args>(args)...)); }
	template<class... Args>
	iterator emplace_hint(const const_iterator, Args&&... args) noexcept { return insert(value_type(std::forward<Args>(args)...)).first; }
	inline iterator erase(const_iterator pos) noexcept;
	inline iterator erase(const const_iterator first, const const_iterator last) noexcept;
	inline size_type erase(const value_type value) noexcept;
	void swap(CompactSet &other) noexcept { std::ranges::swap(this->bits, other.bits); std::ranges::swap(this->size_, other.size_); }
	
	[[nodiscard]] size_type count(const value_type value) const noexcept { assert(value < bits.size()); return bits[value] ? 1 : 0; }
	[[nodiscard]] const_iterator find(const value_type value) const noexcept { assert(value < bits.size()); return bits[value] ? iterator(bits, value) : end(); }
	[[nodiscard]] bool contains(const value_type value) const noexcept { assert(value < bits.size()); return bits[value]; }
	[[nodiscard]] inline std::pair<const_iterator, const_iterator> equal_range(const value_type value) const noexcept;
	[[nodiscard]] inline const_iterator lower_bound(const value_type value) const noexcept;
	[[nodiscard]] const_iterator upper_bound(const value_type value) const noexcept;
	
	[[nodiscard]] inline const_iterator begin() const noexcept;
	[[nodiscard]] const_iterator end() const noexcept { return {this->bits, bits.size()}; }
	[[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
	[[nodiscard]] const_iterator cend() const noexcept { return end(); }
	[[nodiscard]] reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator(end()); }
	[[nodiscard]] reverse_iterator rend() const noexcept { return std::make_reverse_iterator(begin()); }
	[[nodiscard]] const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	[[nodiscard]] const_reverse_iterator crend() const noexcept { return rend(); }
	
#ifndef NDEBUG
	void validate() const noexcept;
#endif
};


template<std::unsigned_integral T>
typename CompactSet<T>::iterator& CompactSet<T>::iterator::operator++() noexcept
{
	assert(i < bits->size());
	for (++i; i != bits->size() && !(*bits)[i]; ++i) { }
	return *this;
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator& CompactSet<T>::iterator::operator--() noexcept
{
	assert(i != 0);
	for (--i; !(*bits)[i]; --i)
		assert(i != 0);
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
bool CompactSet<T>::operator==(const CompactSet &other) const noexcept
{
	if (this->size_ != other.size_)
		return false;
	if (this->bits.size() == other.bits.size())
		return this->bits == other.bits;
	else
		return std::lexicographical_compare_three_way(this->cbegin(), this->cend(), other.cbegin(), other.cend()) == 0;
}

template<std::unsigned_integral T>
auto CompactSet<T>::operator<=>(const CompactSet &other) const noexcept
{
	if (this->bits.size() == other.bits.size())
		return this->bits <=> other.bits;
	else
		return std::lexicographical_compare_three_way(this->cbegin(), this->cend(), other.cbegin(), other.cend());
}

template<std::unsigned_integral T>
std::pair<typename CompactSet<T>::iterator, bool> CompactSet<T>::insert(const value_type value) noexcept
{
	assert(value < bits.size());
	const bool previous = bits[value];
	if (!previous)
	{
		bits[value] = true;
		++size_;
	}
	return {{bits, value}, !previous};
}

template<std::unsigned_integral T>
template<class InputIt>
void CompactSet<T>::insert(const InputIt first, const InputIt last) noexcept
{
	for (InputIt current = first; current != last; ++current)
		insert(*current);
}

template<std::unsigned_integral T>
void CompactSet<T>::unsafe_insert(const CompactSet &other, const size_type overlappingCount) noexcept
{
	assert(other.bits.size() <= this->bits.size());
	assert(overlappingCount <= this->size_);
	assert(overlappingCount <= other.size_);
	std::transform(
			other.bits.cbegin(), other.bits.cend(),
			this->bits.begin(), this->bits.begin(),
			std::logical_or<bool>());
	this->size_ += other.size_ - overlappingCount;
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator CompactSet<T>::erase(const_iterator pos) noexcept
{
	bits[*pos] = false;
	--size_;
	++pos;
	return pos;
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator CompactSet<T>::erase(const const_iterator first, const const_iterator last) noexcept
{
	for (const_iterator current = first; current != last; ++current)
	{
		bits[*current] = false;
		--size_;
	}
	return last;
}

template<std::unsigned_integral T>
typename CompactSet<T>::size_type CompactSet<T>::erase(const value_type value) noexcept
{
	assert(value < bits.size());
	const bool previous = bits[value];
	if (previous)
	{
		bits[value] = false;
		--size_;
	}
	return previous ? 1 : 0;
}

template<std::unsigned_integral T>
std::pair<typename CompactSet<T>::iterator, typename CompactSet<T>::iterator> CompactSet<T>::equal_range(const value_type value) const noexcept
{
	assert(value < bits.size());
	iterator iter(bits, value);
	if (bits[value])
		return {iter, std::ranges::next(iter)};
	++iter;
	return {iter, iter};
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator CompactSet<T>::lower_bound(const value_type value) const noexcept
{
	assert(value < bits.size());
	iterator iter(bits, value);
	if (!bits[value])
		++iter;
	return iter;
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator CompactSet<T>::upper_bound(const value_type value) const noexcept
{
	assert(value < bits.size());
	iterator iter(bits, value);
	++iter;
	return iter;
}

template<std::unsigned_integral T>
typename CompactSet<T>::iterator CompactSet<T>::begin() const noexcept
{
	iterator iter(bits, 0);
	if (!bits.empty() && !bits[0])
		++iter;
	return iter;
}
