#pragma once

#include "PrimeImplicant.hh"
#include "SetOptimizer.hh"


class SetOptimizerForPrimeImplicants : public SetOptimizer<PrimeImplicant>
{
protected:
	HasseDiagram makeHasseDiagram(const sets_t &sets) const final;
	void makeGraph(const HasseDiagram::setHierarchy_t &setHierarchy) final;
	sets_t makeSets() const final;
};
