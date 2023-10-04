#pragma once

#include <cstdint>
#include <set>

#include "SetOptimizer.hh"


class SetOptimizerForSums : public SetOptimizer<std::set<const void*>, std::uintptr_t>
{
protected:
	HasseDiagram makeHasseDiagram(const sets_t &sets) const final;
	void makeGraph(const HasseDiagram::setHierarchy_t &setHierarchy) final;
	gateCount_t countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const final;
	sets_t makeSets() const final;
};
