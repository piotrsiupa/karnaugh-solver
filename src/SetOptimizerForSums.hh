#pragma once

#include <cstdint>
#include <set>

#include "SetOptimizer.hh"


class SetOptimizerForSums : public SetOptimizer<std::set<std::size_t>, std::size_t, std::set>
{
protected:
	HasseDiagram makeHasseDiagram(const sets_t &sets) const final;
	void makeGraph(const HasseDiagram::setHierarchy_t &setHierarchy) final;
	gateCount_t countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const final;
	void substractSubsets(sets_t &sets, const subsetSelections_t &subsetSelections) final;
};
