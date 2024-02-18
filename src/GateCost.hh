#pragma once

#include <cstddef>
#include <ostream>


class GateCost
{
public:
	virtual ~GateCost() = default;
	
	[[nodiscard]] virtual std::size_t getNotCount() const = 0;
	[[nodiscard]] virtual std::size_t getAndCount() const = 0;
	[[nodiscard]] virtual std::size_t getOrCount() const = 0;
	[[nodiscard]] std::size_t getGateScore() const { return getNotCount() + 2 * getAndCount() + 2 * getOrCount(); }
	[[nodiscard]] std::size_t getCost() const { return getCost(getNotCount(), getAndCount(), getOrCount()); }
	[[nodiscard]] static std::size_t getCost(const std::size_t notCount, const std::size_t andCount, const std::size_t orCount) { return notCount + andCount * 2 + orCount * 2; }
	
	void printGateCost(std::ostream &o, const bool full) const;
};
