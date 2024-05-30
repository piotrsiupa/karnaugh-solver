#pragma once

#include <cstddef>


class GateCost
{
public:
	virtual ~GateCost() = default;
	
	[[nodiscard]] virtual std::size_t getNotCount() const = 0;
	[[nodiscard]] virtual std::size_t getAndCount() const = 0;
	[[nodiscard]] virtual std::size_t getOrCount() const = 0;
	
	[[nodiscard]] std::size_t getGateScore() const { return getNotCount() + 2 * getAndCount() + 2 * getOrCount(); }
	[[nodiscard]] std::size_t getCost(const bool inputNots) const { return inputNots ? getCost(getNotCount(), getAndCount(), getOrCount()) : getCost(getAndCount(), getOrCount()); }
	
	[[nodiscard]] static std::size_t getCost(const std::size_t notCount, const std::size_t andCount, const std::size_t orCount) { return notCount + getCost(andCount, orCount); }
	[[nodiscard]] static std::size_t getCost(const std::size_t andCount, const std::size_t orCount) { return andCount * 2 + orCount * 2; }
};
