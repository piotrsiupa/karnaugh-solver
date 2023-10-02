#pragma once

#include <cstddef>
#include <set>
#include <utility>
#include <vector>


template<typename SET>
class SetOptimizer
{
	using graph_t = std::vector<std::pair<SET, std::vector<std::size_t>>>;
	using endNodes_t = std::set<std::size_t>;
	using subsetSelection_t = std::vector<std::size_t>;
	using subsetSelections_t = std::vector<subsetSelection_t>;
	using usageCounts_t = std::vector<std::size_t>;
	using gateCount_t = std::size_t;
	
	graph_t graph;
	endNodes_t endNodes;
	
public:
	using sets_t = std::vector<SET>;
	using result_t = std::pair<sets_t, subsetSelections_t>;
	
	SetOptimizer(const sets_t &sets);
	
	result_t extractCommonParts();
	
private:
	bool chooseNextSubsets(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const;
	void removeRedundantNodes(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const;
	gateCount_t countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const;
	std::pair<subsetSelections_t, usageCounts_t> findBestSubsets() const;
	void removeUnusedSubsets(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts);
	sets_t makeSets() const;
};
