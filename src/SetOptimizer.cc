#include "./SetOptimizer.hh"

#include <algorithm>
#include <cstdint>


template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::Result SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::extractCommonParts(const sets_t &oldSets)
{
	const typename SubsetFinder::setHierarchy_t setHierarchy = SubsetFinder::makeSetHierarchy(convertSets(oldSets));
	makeGraph(setHierarchy);
	auto [subsetSelections, usageCounts] = findBestSubsets();
	removeUnusedSubsets(subsetSelections, usageCounts);
	sets_t newSets = makeSets();
	const finalSets_t finalSets = makeFinalSets(oldSets, newSets);
	substractSubsets(newSets, subsetSelections);
	return {newSets, finalSets, subsetSelections};
}

template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
bool SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::chooseNextSubsets(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const
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

template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::removeRedundantNodes(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const
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

template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
std::pair<typename SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::subsetSelections_t, typename SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::usageCounts_t> SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::findBestSubsets() const
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

template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::removeUnusedSubsets(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts)
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
	
	endNodes_t newEndNodes;
	for (const std::size_t &oldEndNode : endNodes)
		newEndNodes.insert(indexMap[oldEndNode]);
	endNodes = std::move(newEndNodes);
}

template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::sets_t SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::makeSets() const
{
	sets_t sets;
	sets.reserve(graph.size());
	for (const auto &graphNode : graph)
		sets.push_back(graphNode.first);
	return sets;
}

template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::finalSets_t SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::makeFinalSets(const sets_t &oldSets, const sets_t &newSets)
{
	finalSets_t finalSets(oldSets.size());
	for (const std::size_t endNode : endNodes)
	{
		const auto &newSet = newSets[endNode];
		auto foundOldSet = std::find(oldSets.cbegin(), oldSets.cend(), newSet);
		do
		{
			finalSets[foundOldSet - oldSets.cbegin()] = endNode;
			foundOldSet = std::find(foundOldSet + 1, oldSets.cend(), newSet);
		} while (foundOldSet != oldSets.cend());
	}
	return finalSets;
}


#include "PrimeImplicant.hh"

template class SetOptimizer<PrimeImplicant, std::int_fast8_t, std::vector>;
template class SetOptimizer<std::set<std::size_t>, std::size_t, std::set>;
