#pragma once

#include <algorithm>
#include <cstdint>
#include <set>

#include "Progress.hh"
#include "SetOptimizer.hh"


class SetOptimizerForSums : public SetOptimizer<false>
{
public:
	static Result optimizeSet(sets_t &&sets, Progress &progress) { const auto infoGuard = progress.addInfo("Sums"); return SetOptimizerForSums().extractCommonParts(std::move(sets), progress); }
	
protected:
	SubsetFinder::sets_t convertSets(const sets_t &sets) const final;
	void makeGraph(const SubsetFinder::setHierarchy_t &setHierarchy) final;
	std::vector<setElement_t> getAllSetElements(const sets_t &oldSets) const final;
	set_t addSetElement(const set_t &set, const setElement_t &setElement) const final { set_t newSet = set; newSet.insert(setElement); return newSet; }
	gateCount_t countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const final;
	void substractSubsets(sets_t &sets, const subsetSelections_t &subsetSelections) final;
	void substractSet(set_t &set, const set_t &otherSet) const final { for (const std::size_t x : otherSet) set.erase(x); }
	bool setContainsSet(const set_t &x, const set_t &y) const final { return std::ranges::includes(x, y); }
	set_t getSetIntersection(const set_t &set0, const set_t &set1) const final { set_t intersection; std::ranges::set_intersection(set0, set1, std::inserter(intersection, intersection.begin())); return intersection; }
	bool isSubsetWorthy(const set_t &subset) const final { return subset.size() >= 2; }
};
