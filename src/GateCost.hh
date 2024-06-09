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


class StandaloneGateCost : public GateCost
{
	std::size_t notCount = 0, andCount = 0, orCount = 0;
	
public:
	StandaloneGateCost& operator+=(const GateCost &gateCost) { notCount += gateCost.getNotCount(); andCount += gateCost.getAndCount(); orCount += gateCost.getOrCount(); return *this; }
	StandaloneGateCost& addToNotCount(const std::size_t n) { notCount += n; return *this; }
	StandaloneGateCost& addToAndCount(const std::size_t n) { andCount += n; return *this; }
	StandaloneGateCost& addToOrCount(const std::size_t n) { orCount += n; return *this; }
	
	[[nodiscard]] std::size_t getNotCount() const final { return notCount; }
	[[nodiscard]] std::size_t getAndCount() const final { return andCount; }
	[[nodiscard]] std::size_t getOrCount() const final { return orCount; }
};
