#include "./SetOptimizerForSums.hh"

#include <algorithm>


SetOptimizerForSums::HasseDiagram SetOptimizerForSums::makeHasseDiagram(const sets_t &sets) const
{
	HasseDiagram hasseDiagram;
	for (const std::set<std::size_t> &set : sets)
		hasseDiagram.insert(set);
	return hasseDiagram;
}

void SetOptimizerForSums::makeGraph(const HasseDiagram::setHierarchy_t &setHierarchy)
{
	graph.reserve(setHierarchy.size());
	std::size_t i = 0;
	for (auto &setHierarchyEntry : setHierarchy)
	{
		std::set<std::size_t> set(setHierarchyEntry.values.begin(), setHierarchyEntry.values.end());
		graph.emplace_back(std::move(set), std::move(setHierarchyEntry.subsets));
		if (setHierarchyEntry.isOriginalSet)
			endNodes.insert(i);
		++i;
	}
}

SetOptimizerForSums::gateCount_t SetOptimizerForSums::countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const
{
	gateCount_t gates = 0;
	for (std::size_t i = 0; i != graph.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		gates += subsetSelections[i].size();
		std::set<std::size_t> reducedSet = graph[i].first;
		for (const std::size_t &subset : subsetSelections[i])
		{
			std::set<std::size_t> setDifference;
			std::set_difference(reducedSet.cbegin(), reducedSet.cend(), graph[subset].first.cbegin(), graph[subset].first.cend(), std::inserter(setDifference, setDifference.end()));
			reducedSet = std::move(setDifference);
		}
		gates += reducedSet.size();
		if (subsetSelections[i].size() != 0 || !reducedSet.empty())
			--gates;
	}
	return gates;
}

void SetOptimizerForSums::substractSubsets(sets_t &sets, const subsetSelections_t &subsetSelections)
{
	for (std::size_t i = sets.size(); i --> 0;)
	{
		set_t &set = sets[i];
		for (const std::size_t subsetIndex : subsetSelections[i])
		{
			const set_t &subset = sets[subsetIndex];
			std::set<std::size_t> setDifference;
			std::set_difference(set.cbegin(), set.cend(), subset.cbegin(), subset.cend(), std::inserter(setDifference, setDifference.end()));
			set = std::move(setDifference);
		}
	}
}
