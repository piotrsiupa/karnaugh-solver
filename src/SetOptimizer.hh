#pragma once

#include <cstddef>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "Implicant.hh"
#include "Progress.hh"
#include "SubsetGraph.hh"


template<bool IS_IMPLICANT>
class SetOptimizer
{
public:
	using set_t = std::conditional_t<IS_IMPLICANT, Implicant, std::set<std::size_t>>;
	
protected:
	using valueId_t = std::conditional_t<IS_IMPLICANT, std::int_fast8_t, std::size_t>;
	using setElement_t = std::conditional_t<IS_IMPLICANT, Implicant::splitBit_t, std::size_t>;
	using SubsetFinder = std::conditional_t<IS_IMPLICANT, SubsetGraph<valueId_t, std::vector>, SubsetGraph<valueId_t, std::set>>;
	using possibleSubsets_t = std::vector<std::size_t>;
	using graphNode_t = struct { set_t set; possibleSubsets_t possibleSubsets; };
	using graph_t = std::vector<graphNode_t>;
	using endNodes_t = std::set<std::size_t>;
	using gateCount_t = std::size_t;
	using usageCounts_t = std::vector<std::size_t>;
	
	graph_t graph;
	endNodes_t endNodes;
	
public:
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
	
	Result extractCommonParts(sets_t &&sets, Progress &progress);
	
	virtual typename SubsetFinder::sets_t convertSets(const sets_t &sets) const = 0;
	virtual void makeGraph(const typename SubsetFinder::setHierarchy_t &setHierarchy) = 0;
	virtual std::vector<setElement_t> getAllSetElements(const sets_t &oldSets) const = 0;
	virtual set_t addSetElement(const set_t &set, const setElement_t &setElement) const = 0;
	virtual gateCount_t countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const = 0;
	virtual void substractSubsets(sets_t &sets, const subsetSelections_t &subsetSelections) = 0;
	virtual void substractSet(set_t &set, const set_t &otherSet) const = 0;
	virtual bool setContainsSet(const set_t &x, const set_t &y) const = 0;
	virtual set_t getSetIntersection(const set_t &set0, const set_t &set1) const = 0;
	virtual bool isSubsetWorthy(const set_t &subset) const = 0;
	
private:
	void makeFullGraph(const sets_t &oldSets);
	
	void makeGreedyGraph(const sets_t &oldSets, Progress &progress);
	
	static std::size_t getMaxRoughDepth();
	static std::size_t getMaxRoughWidth();
	void makeRoughGraph(const sets_t &oldSets, Progress &progress);
	
	void makeGraph(const sets_t &oldSets, Progress &progress);
	
	void switchToParentNodesIfAllowed(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const;
	void removeSingleUseNonFinalNodes(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const;
	void removeUnusedNodes(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts);
	void removeUnnecessaryParents(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const;
	void removeRedundantNodes(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts, const bool switchToParents);
	
	static std::pair<Progress::completion_t, Progress::completion_t> estimateBruteForceCompletion(const subsetSelection_t &subsetSelection, const possibleSubsets_t &possibleSubsets);
	Progress::completion_t estimateBruteForceCompletion(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const;
	bool chooseNextSubsetsForBruteForce(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const;
	subsetSelections_t processGraph_bruteForce(Progress &progress);
	
	bool chooseNextSubsetsForExhaustive(usageCounts_t &usageCounts) const;
	void cleanupResultOfExhaustive(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const;
	subsetSelections_t processGraph_exhaustive(Progress &progress);
	
	subsetSelections_t processGraph_cursory(Progress &progress);
	
	subsetSelections_t processGraph_greedy(Progress &progress);
	
	subsetSelections_t processGraph_rough(Progress &progress);
	
	subsetSelections_t processGraph(Progress &progress);
	subsetSelections_t findBestSubsets(const sets_t &oldSets, Progress &progress);
	
	sets_t makeSets() const;
	
	finalSets_t makeFinalSets(sets_t &&oldSets, const sets_t &newSets);
};
