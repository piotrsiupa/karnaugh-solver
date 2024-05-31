#include "./SetOptimizerForProducts.hh"

#include <algorithm>
#include <limits>


SetOptimizerForProducts::SubsetFinder::sets_t SetOptimizerForProducts::convertSets(const sets_t &sets) const
{
	SubsetFinder::sets_t convertedSets;
	for (const Implicant &set : sets)
	{
		SubsetFinder::set_t convertedSet;
		if (set == Implicant::all())
		{
			convertedSet.push_back(std::numeric_limits<valueId_t>::max());
		}
		else if (set == Implicant::none())
		{
			convertedSet.push_back(std::numeric_limits<valueId_t>::min());
		}
		else
		{
			for (const auto &bit : set.splitBits())
				convertedSet.push_back(bit.second ? bit.first : -bit.first - 1);
			std::ranges::sort(convertedSet);
		}
		convertedSets.push_back(std::move(convertedSet));
	}
	return convertedSets;
}

void SetOptimizerForProducts::makeGraph(const SubsetFinder::setHierarchy_t &setHierarchy)
{
	graph.reserve(setHierarchy.size());
	std::size_t i = 0;
	for (auto &setHierarchyEntry : setHierarchy)
	{
		Implicant set = Implicant::all();
		const auto &values = setHierarchyEntry.values;
		if (values.size() == 1 && values[0] == std::numeric_limits<valueId_t>::max())
		{
			set = Implicant::all();
		}
		else if (values.size() == 1 && values[0] == std::numeric_limits<valueId_t>::min())
		{
			set = Implicant::none();
		}
		else
		{
			for (const auto &value : values)
				if (value >= 0)
					set.setBit(value, true);
				else
					set.setBit(-value - 1, false);
		}
		graph.emplace_back(set, std::move(setHierarchyEntry.subsets));
		if (setHierarchyEntry.isOriginalSet)
			endNodes.insert(i);
		++i;
	}
}

std::vector<SetOptimizerForProducts::setElement_t> SetOptimizerForProducts::getAllSetElements(const sets_t &oldSets) const
{
	std::map<setElement_t, std::size_t> allSetElementsMap;
	for (const set_t &set : oldSets)
		for (const Implicant::splitBit_t &bit : set.splitBits())
			++allSetElementsMap[bit];
	std::vector<setElement_t> allSetElements;
	allSetElements.reserve(allSetElementsMap.size());
	for (const auto &[element, count] : allSetElementsMap)
		allSetElements.push_back(element);
	std::ranges::sort(allSetElements, std::less(), [allSetElementsMap = std::as_const(allSetElementsMap)](const setElement_t &x){ return allSetElementsMap.at(x); });
	return allSetElements;
}

SetOptimizerForProducts::gateCount_t SetOptimizerForProducts::countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const
{
	gateCount_t gates = 0;
	for (std::size_t i = 0; i != graph.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		gates += subsetSelections[i].size();
		Implicant reducedProduct = graph[i].set;
		for (const std::size_t &subset : subsetSelections[i])
			reducedProduct.substract(graph[subset].set);
		gates += reducedProduct.size();
		if (subsetSelections[i].size() != 0 || !reducedProduct.empty())
			--gates;
	}
	return gates;
}

void SetOptimizerForProducts::substractSubsets(sets_t &sets, const subsetSelections_t &subsetSelections)
{
	for (std::size_t i = sets.size(); i --> 0;)
	{
		for (const std::size_t subsetIndex : subsetSelections[i])
		{
			sets[i].substract(sets[subsetIndex]);
			assert((sets[i].getBits() & ~sets[i].getMask()) == 0);
		}
	}
}
