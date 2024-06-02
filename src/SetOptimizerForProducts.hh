#pragma once

#include <cstdint>
#include <vector>

#include "Implicant.hh"
#include "Progress.hh"
#include "SetOptimizer.hh"


class SetOptimizerForProducts : public SetOptimizer<true>
{
public:
	static Result optimizeSet(sets_t &&sets, Progress &progress) { const auto infoGuard = progress.addInfo("Products"); return SetOptimizerForProducts().extractCommonParts(std::move(sets), progress); }
	
protected:
	SubsetFinder::sets_t convertSets(const sets_t &sets) const final;
	void makeGraph(const SubsetFinder::setHierarchy_t &setHierarchy) final;
	std::vector<setElement_t> getAllSetElements(const sets_t &oldSets) const final;
	set_t addSetElement(const set_t &set, const setElement_t &setElement) const final { if (set.hasOppositeBit(setElement)) return set_t(); set_t newSet = set; newSet.setBit(setElement); return newSet; }
	gateCount_t countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const final;
	void substractSubsets(sets_t &sets, const subsetSelections_t &subsetSelections) final;
	void substractSet(set_t &set, const set_t &otherSet) const final { set.substract(otherSet); }
	bool setContainsSet(const set_t &x, const set_t &y) const final { return x.contains(y); }
	set_t getSetIntersection(const set_t &set0, const set_t &set1) const final { set_t intersection = set0; intersection.intersect(set1); return intersection; }
	bool isSubsetWorthy(const set_t &subset) const final { return subset.size() >= 2; }
};
