#pragma once

#include <cstdint>
#include <vector>

#include "Implicant.hh"
#include "Progress.hh"
#include "SetOptimizer.hh"


class SetOptimizerForProducts : public SetOptimizer<Implicant, std::int_fast8_t, std::vector>
{
public:
	static Result optimizeSet(sets_t &&sets, Progress &progress) { const auto infoGuard = progress.addInfo("Products"); return SetOptimizerForProducts().extractCommonParts(std::move(sets), progress); }
	
protected:
	SubsetFinder::sets_t convertSets(const sets_t &sets) const final;
	void makeGraph(const SubsetFinder::setHierarchy_t &setHierarchy) final;
	gateCount_t countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const final;
	void substractSubsets(sets_t &sets, const subsetSelections_t &subsetSelections) final;
	void substractSet(set_t &set, const set_t &otherSet) const final { set.substract(otherSet); }
	bool isSubsetWorthy(const set_t &subset) const final { return subset.getBitCount() >= 2; }
};
