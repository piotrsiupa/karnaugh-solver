#include "./SetOptimizerForPrimeImplicants.hh"

#include <algorithm>


SetOptimizerForPrimeImplicants::HasseDiagram SetOptimizerForPrimeImplicants::makeHasseDiagram(const sets_t &sets) const
{
	HasseDiagram hasseDiagram;
	for (const PrimeImplicant &set : sets)
	{
		if (set.getBitCount() != 0)
		{
			OptimizationHasseDiagram<std::int8_t>::set_t convertedSet;
			for (const auto &bit : set.splitBits())
				convertedSet.push_back(bit.second ? -bit.first - 1 : bit.first);
			std::sort(convertedSet.begin(), convertedSet.end());
			hasseDiagram.insert(convertedSet);
		}
	}
	return hasseDiagram;
}

void SetOptimizerForPrimeImplicants::makeGraph(const HasseDiagram::setHierarchy_t &setHierarchy)
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

SetOptimizerForPrimeImplicants::sets_t SetOptimizerForPrimeImplicants::makeSets() const
{
	sets_t sets;
	sets.reserve(graph.size());
	for (const auto &graphNode : graph)
		sets.push_back(graphNode.first);
	return sets;
}
