#include "./SetOptimizerForProducts.hh"

#include <algorithm>
#include <limits>


SetOptimizerForProducts::HasseDiagram SetOptimizerForProducts::makeHasseDiagram(const sets_t &sets) const
{
	HasseDiagram hasseDiagram;
	for (const PrimeImplicant &set : sets)
	{
		HasseDiagram::set_t convertedSet;
		if (set == PrimeImplicant::all())
		{
			convertedSet.push_back(std::numeric_limits<hasseValue_t>::max());
		}
		else if (set == PrimeImplicant::error())
		{
			convertedSet.push_back(std::numeric_limits<hasseValue_t>::min());
		}
		else
		{
			for (const auto &bit : set.splitBits())
				convertedSet.push_back(bit.second ? -bit.first - 1 : bit.first);
			std::sort(convertedSet.begin(), convertedSet.end());
		}
		hasseDiagram.insert(convertedSet);
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
		const auto &values = setHierarchyEntry.values;
		if (values.size() == 1 && values[0] == std::numeric_limits<hasseValue_t>::max())
		{
			set = PrimeImplicant::all();
		}
		else if (values.size() == 1 && values[0] == std::numeric_limits<hasseValue_t>::min())
		{
			set = PrimeImplicant::error();
		}
		else
		{
			for (const auto &value : values)
				if (value >= 0)
					set.setBit(value, false);
				else
					set.setBit(-value - 1, true);
		}
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

void SetOptimizerForProducts::substractSubsets(sets_t &sets, const subsetSelections_t &subsetSelections)
{
	for (std::size_t i = sets.size(); i --> 0;)
		for (const std::size_t subsetIndex : subsetSelections[i])
			sets[i] -= sets[subsetIndex];
}
