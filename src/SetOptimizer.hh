#pragma once

#include <cstddef>
#include <set>
#include <utility>
#include <vector>

#include "SubsetGraph.hh"


template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
class SetOptimizer
{
protected:
	using valueId_t = VALUE_ID;
	using SubsetFinder = SubsetGraph<valueId_t, FINDER_CONTAINER>;
	using graph_t = std::vector<std::pair<SET, std::vector<std::size_t>>>;
	using endNodes_t = std::set<std::size_t>;
	using gateCount_t = std::size_t;
	using usageCounts_t = std::vector<std::size_t>;
	
	graph_t graph;
	endNodes_t endNodes;
	
public:
	using set_t = SET;
	using sets_t = std::vector<set_t>;
	using finalSets_t = std::vector<std::size_t>;
	using subsetSelection_t = std::vector<std::size_t>;
	using subsetSelections_t = std::vector<subsetSelection_t>;
	struct Result
	{
		sets_t sets;
		finalSets_t finalSets;
		subsetSelections_t subsets;
	};
	
protected:
	SetOptimizer() = default;
	
	Result extractCommonParts(const sets_t &sets);
	
	virtual typename SubsetFinder::sets_t convertSets(const sets_t &sets) const = 0;
	virtual void makeGraph(const typename SubsetFinder::setHierarchy_t &setHierarchy) = 0;
	virtual gateCount_t countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const = 0;
	virtual void substractSubsets(sets_t &sets, const subsetSelections_t &subsetSelections) = 0;
	
private:
	bool chooseNextSubsets(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const;
	void removeRedundantNodes(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const;
	std::pair<subsetSelections_t, usageCounts_t> findBestSubsets() const;
	void removeUnusedSubsets(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts);
	sets_t makeSets() const;
	finalSets_t makeFinalSets(const sets_t &oldSets, const sets_t &newSets);
};
