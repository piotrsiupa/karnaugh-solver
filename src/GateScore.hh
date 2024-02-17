#pragma once

#include <cstddef>
#include <ostream>


class GateScore
{
public:
	virtual std::size_t getNotCount() const = 0;
	virtual std::size_t getAndCount() const = 0;
	virtual std::size_t getOrCount() const = 0;
	std::size_t getGateScore() const { return getNotCount() + 2 * getAndCount() + 2 * getOrCount(); }
	
	void printGateScores(std::ostream &o) const;
};
