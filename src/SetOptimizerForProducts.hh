#pragma once

#include <cstdint>
#include <vector>

#include "PrimeImplicant.hh"
#include "SetOptimizer.hh"


class SetOptimizerForProducts : public SetOptimizer<PrimeImplicant, std::int_fast8_t, std::vector>
{
public:
	static Result optimizeSet(const sets_t &sets) { return SetOptimizerForProducts().extractCommonParts(sets); }
	
protected:
	SubsetFinder::sets_t convertSets(const sets_t &sets) const final;
	void makeGraph(const SubsetFinder::setHierarchy_t &setHierarchy) final;
	gateCount_t countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const final;
	void substractSubsets(sets_t &sets, const subsetSelections_t &subsetSelections) final;
};
