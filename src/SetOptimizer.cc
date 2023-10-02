#include "./SetOptimizer.hh"

#include <algorithm>
#include <cstdint>

#include "OptimizationHasseDiagram.hh"


template<typename SET>
SetOptimizer<SET>::SetOptimizer(const sets_t &sets)
{
	using HasseDiagram = OptimizationHasseDiagram<std::int8_t>;
	HasseDiagram hasseDiagram;
	for (const SET &set : sets)
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
	HasseDiagram::setHierarchy_t setHierarchy = hasseDiagram.makeSetHierarchy();
	graph.reserve(setHierarchy.size());
	std::size_t i = 0;
	for (auto &setHierarchyEntry : setHierarchy)
	{
		SET set = SET::all();
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

template<typename SET>
typename SetOptimizer<SET>::result_t SetOptimizer<SET>::extractCommonParts()
{
	auto [subsetSelections, usageCounts] = findBestSubsets();
	removeUnusedSubsets(subsetSelections, usageCounts);
	return {makeSets(), subsetSelections};
}

template<typename SET>
bool SetOptimizer<SET>::chooseNextSubsets(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const
{
	try_again:
	// Don't let the familiar sight of the loop make an impression that control flow of this function is in any way understandable without analyzing specific paths.
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		const std::vector<std::size_t> &possibleSubsets = graph[i].second;
		if (possibleSubsets.empty())
			continue;
		subsetSelection_t &subsetSelection = subsetSelections[i];
		if (subsetSelection.empty())
		{
			subsetSelection.push_back(0);
			++usageCounts[possibleSubsets[0]];
		}
		else if (subsetSelection.back() != possibleSubsets.size() - 1)
		{
			subsetSelection.push_back(subsetSelection.back() + 1);
			++usageCounts[possibleSubsets[subsetSelection.back()]];
		}
		else if (subsetSelection.size() == 1)
		{
			--usageCounts[possibleSubsets.back()];
			subsetSelection.pop_back();
			continue;
		}
		else
		{
			subsetSelection.pop_back();
			--usageCounts[possibleSubsets[subsetSelection.back()]];
			++subsetSelection.back();
			--usageCounts[possibleSubsets.back()];
			++usageCounts[possibleSubsets[subsetSelection.back()]];
		}
		for (std::size_t j = 0; j != subsetSelections.size(); ++j)
			if (usageCounts[j] == 1 && subsetSelections[j].empty())
				goto try_again;
		return true;
	}
	return false;
}

template<typename SET>
void SetOptimizer<SET>::removeRedundantNodes(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const
{
	for (std::size_t nodeIndex = 0; nodeIndex != subsetSelections.size(); ++nodeIndex)
	{
		if (usageCounts[nodeIndex] == 0)
			continue;
		for (std::size_t i = 0; i != subsetSelections[nodeIndex].size();)
		{
			const std::size_t subset = subsetSelections[nodeIndex][i];
			if (usageCounts[subset] == 1)
			{
				subsetSelections[nodeIndex].erase(subsetSelections[nodeIndex].begin() + i);
				usageCounts[subset] = 0;
				subsetSelections[nodeIndex].insert(subsetSelections[nodeIndex].end(), subsetSelections[subset].cbegin(), subsetSelections[subset].cend());
			}
			else
			{
				++i;
			}
		}
	}
}

template<typename SET>
typename SetOptimizer<SET>::gateCount_t SetOptimizer<SET>::countGates(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const
{
	gateCount_t gates = 0;
	for (std::size_t i = 0; i != graph.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		gates += subsetSelections[i].size();
		SET reducedPrimeImplicant = graph[i].first;
		for (const std::size_t &subset : subsetSelections[i])
			reducedPrimeImplicant -= graph[subset].first;
		gates += reducedPrimeImplicant.getBitCount();
		if (subsetSelections[i].size() != 0 || reducedPrimeImplicant.getBitCount() != 0)
			--gates;
	}
	return gates;
}

template<typename SET>
std::pair<typename SetOptimizer<SET>::subsetSelections_t, typename SetOptimizer<SET>::usageCounts_t> SetOptimizer<SET>::findBestSubsets() const
{
	subsetSelections_t subsetSelections(graph.size()), bestSubsetSelections(graph.size());
	usageCounts_t usageCounts(graph.size()), bestUsageCounts(graph.size());
	for (const std::size_t endNode : endNodes)
		usageCounts[endNode] = SIZE_MAX - graph.size();
	std::size_t bestGates = SIZE_MAX;
	while (true)
	{
		subsetSelections_t candidateSubsetSelections(subsetSelections.size());
		for (std::size_t i = 0; i != subsetSelections.size(); ++i)
		{
			candidateSubsetSelections[i].reserve(subsetSelections[i].size());
			for (const std::size_t &subset : subsetSelections[i])
				candidateSubsetSelections[i].push_back(graph[i].second[subset]);
		}
		usageCounts_t candidateUsageCounts = usageCounts;
		removeRedundantNodes(candidateSubsetSelections, candidateUsageCounts);
		
		const gateCount_t gates = countGates(candidateSubsetSelections, candidateUsageCounts);
		if (gates < bestGates)
		{
			bestSubsetSelections = std::move(candidateSubsetSelections);
			bestUsageCounts = std::move(candidateUsageCounts);
			bestGates = gates;
		}
		if (!chooseNextSubsets(subsetSelections, usageCounts))
			break;
	}
	
	return {bestSubsetSelections, bestUsageCounts};
}

template<typename SET>
void SetOptimizer<SET>::removeUnusedSubsets(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts)
{
	std::size_t i = 0;
	for (i = 0; i != graph.size(); ++i)
		if (usageCounts[i] == 0)
			break;
	if (i == graph.size())
		return;
	
	std::vector<std::size_t> indexMap(graph.size());
	std::size_t current;
	for (current = 0; current != i; ++current)
		indexMap[current] = current;
	for (++i; i != graph.size(); ++i)
	{
		if (usageCounts[i] != 0)
		{
			graph[current] = std::move(graph[i]);
			subsetSelections[current] = std::move(subsetSelections[i]);
			indexMap[i] = current;
			++current;
		}
	}
	graph.erase(graph.begin() + current, graph.end());
	subsetSelections.resize(current);
	
	for (std::vector<std::size_t> &x : subsetSelections)
		for (std::size_t &subset : x)
			subset = indexMap[subset];
}

template<typename SET>
typename SetOptimizer<SET>::sets_t SetOptimizer<SET>::makeSets() const
{
	sets_t sets;
	sets.reserve(graph.size());
	for (const auto &graphNode : graph)
		sets.push_back(graphNode.first);
	return sets;
}


#include "PrimeImplicant.hh"

template class SetOptimizer<PrimeImplicant>;
