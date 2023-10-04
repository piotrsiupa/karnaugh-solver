#include "./SetOptimizerForProducts.hh"

#include <algorithm>


SetOptimizerForProducts::HasseDiagram SetOptimizerForProducts::makeHasseDiagram(const sets_t &sets) const
{
	HasseDiagram hasseDiagram;
	for (const PrimeImplicant &set : sets)
	{
		if (set.getBitCount() != 0)
		{
			HasseDiagram::set_t convertedSet;
			for (const auto &bit : set.splitBits())
				convertedSet.push_back(bit.second ? -bit.first - 1 : bit.first);
			std::sort(convertedSet.begin(), convertedSet.end());
			hasseDiagram.insert(convertedSet);
		}
	}
	return hasseDiagram;
}

void SetOptimizerForProducts::makeGraph(const HasseDiagram::setHierarchy_t &setHierarchy)
{
	graph.reserve(setHierarchy.size());
	std::size_t i = 0;
	for (auto &setHierarchyEntry : setHierarchy)
	{
		PrimeImplicant set = PrimeImplicant::all();
		for (const auto &value : setHierarchyEntry.values)
			if (value >= 0)
				set.setBit(value, false);
			else
				set.setBit(-value - 1, true);
		graph.emplace_back(set, std::move(setHierarchyEntry.subsets));
		if (setHierarchyEntry.isOriginalSet)
			endNodes.insert(i);
		++i;
	}
}

SetOptimizerForProducts::gateCount_t SetOptimizerForProducts::countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const
{
	gateCount_t gates = 0;
	for (std::size_t i = 0; i != graph.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		gates += subsetSelections[i].size();
		PrimeImplicant reducedProduct = graph[i].first;
		for (const std::size_t &subset : subsetSelections[i])
			reducedProduct -= graph[subset].first;
		gates += reducedProduct.getBitCount();
		if (subsetSelections[i].size() != 0 || reducedProduct.getBitCount() != 0)
			--gates;
	}
	return gates;
}

SetOptimizerForProducts::sets_t SetOptimizerForProducts::makeSets() const
{
	sets_t sets;
	sets.reserve(graph.size());
	for (const auto &graphNode : graph)
		sets.push_back(graphNode.first);
	return sets;
}
