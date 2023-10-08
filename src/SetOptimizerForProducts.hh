#pragma once

#include <cstdint>
#include <vector>

#include "PrimeImplicant.hh"
#include "SetOptimizer.hh"


class SetOptimizerForProducts : public SetOptimizer<PrimeImplicant, std::int_fast8_t, std::vector>
{
protected:
	HasseDiagram makeHasseDiagram(const sets_t &sets) const final;
	void makeGraph(const HasseDiagram::setHierarchy_t &setHierarchy) final;
	gateCount_t countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const final;
	sets_t makeSets() const final;
};
