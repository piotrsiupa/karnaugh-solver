#pragma once

#include <numeric>
#include <vector>

#include "GateScore.hh"
#include "Solution.hh"


class Solutions : public GateScore, public std::vector<Solution>
{
public:
	using std::vector<Solution>::vector;
	Solutions(std::vector<Solution> &&solutions) : std::vector<Solution>(std::move(solutions)) {}
	
	std::size_t getNotCount() const final { return std::accumulate(cbegin(), cend(), 0, [](const std::size_t acc, const Solution &solution){ return solution.getNotCount() + acc; }); }
	std::size_t getAndCount() const final { return std::accumulate(cbegin(), cend(), 0, [](const std::size_t acc, const Solution &solution){ return solution.getAndCount() + acc; }); }
	std::size_t getOrCount() const final { return std::accumulate(cbegin(), cend(), 0, [](const std::size_t acc, const Solution &solution){ return solution.getOrCount() + acc; }); }
};
