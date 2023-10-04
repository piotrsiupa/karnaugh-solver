#include "./SetOptimizerForSums.hh"

#include <algorithm>


SetOptimizerForSums::HasseDiagram SetOptimizerForSums::makeHasseDiagram(const sets_t &sets) const
{
	HasseDiagram hasseDiagram;
	for (const std::set<const void*> &set : sets)
	{
		if (!set.empty())
		{
			HasseDiagram::set_t convertedSet;
			for (const auto &product : set)
				convertedSet.push_back(reinterpret_cast<std::uintptr_t>(product));
			std::sort(convertedSet.begin(), convertedSet.end());
			hasseDiagram.insert(convertedSet);
		}
	}
	return hasseDiagram;
}

void SetOptimizerForSums::makeGraph(const HasseDiagram::setHierarchy_t &setHierarchy)
{
	graph.reserve(setHierarchy.size());
	std::size_t i = 0;
	for (auto &setHierarchyEntry : setHierarchy)
	{
		std::set<const void*> set;
		for (const auto &value : setHierarchyEntry.values)
			set.insert(reinterpret_cast<const void*>(value));
		graph.emplace_back(set, std::move(setHierarchyEntry.subsets));
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
		std::set<const void*> reducedSet = graph[i].first;
		for (const std::size_t &subset : subsetSelections[i])
		{
			std::set<const void*> setDifference;
			std::set_difference(reducedSet.cbegin(), reducedSet.cend(), graph[subset].first.cbegin(), graph[subset].first.cend(), std::inserter(setDifference, setDifference.end()));
			reducedSet = std::move(setDifference);
		}
		gates += reducedSet.size();
		if (subsetSelections[i].size() != 0 || !reducedSet.empty())
			--gates;
	}
	return gates;
}

SetOptimizerForSums::sets_t SetOptimizerForSums::makeSets() const
{
	sets_t sets;
	sets.reserve(graph.size());
	for (const auto &graphNode : graph)
		sets.push_back(graphNode.first);
	return sets;
}
