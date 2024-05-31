#include "./SetOptimizerForSums.hh"

#include <algorithm>
#include <iterator>


SetOptimizerForSums::SubsetFinder::sets_t SetOptimizerForSums::convertSets(const sets_t &sets) const
{
	SubsetFinder::sets_t convertedSets;
	for (const std::set<std::size_t> &set : sets)
		convertedSets.push_back(set);
	return convertedSets;
}

void SetOptimizerForSums::makeGraph(const SubsetFinder::setHierarchy_t &setHierarchy)
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

std::vector<SetOptimizerForSums::setElement_t> SetOptimizerForSums::getAllSetElements(const sets_t &oldSets) const
{
	std::map<std::size_t, std::size_t> allSetElementsMap;
	for (const set_t &set : oldSets)
		for (const std::size_t element : set)
			++allSetElementsMap[element];
	std::vector<SetOptimizerForSums::setElement_t> allSetElements;
	allSetElements.reserve(allSetElementsMap.size());
	for (const auto [element, count] : allSetElementsMap)
		allSetElements.push_back(element);
	std::ranges::sort(allSetElements, std::less(), [allSetElementsMap = std::as_const(allSetElementsMap)](const std::size_t x){ return allSetElementsMap.at(x); });
	return allSetElements;
}

SetOptimizerForSums::gateCount_t SetOptimizerForSums::countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const
{
	gateCount_t gates = 0;
	for (std::size_t i = 0; i != graph.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		gates += subsetSelections[i].size();
		std::set<std::size_t> reducedSet = graph[i].set;
		for (const std::size_t &subset : subsetSelections[i])
		{
			std::set<std::size_t> setDifference;
			std::ranges::set_difference(reducedSet, graph[subset].set, std::inserter(setDifference, setDifference.end()));
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
			std::ranges::set_difference(set, subset, std::inserter(setDifference, setDifference.end()));
			set = std::move(setDifference);
		}
	}
}
