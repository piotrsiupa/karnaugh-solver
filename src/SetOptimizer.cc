#include "./SetOptimizer.hh"

#include <algorithm>
#include <cmath>
#include <cstdint>


template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::Result SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::extractCommonParts(const sets_t &oldSets, Progress &progress)
{
	const typename SubsetFinder::setHierarchy_t setHierarchy = SubsetFinder::makeSetHierarchy(convertSets(oldSets));
	makeGraph(setHierarchy);
	auto [subsetSelections, usageCounts] = findBestSubsets(progress);
	removeUnusedSubsets(subsetSelections, usageCounts);
	sets_t newSets = makeSets();
	const finalSets_t finalSets = makeFinalSets(oldSets, newSets);
	substractSubsets(newSets, subsetSelections);
	return {newSets, finalSets, subsetSelections};
}

template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
std::pair<Progress::completion_t, Progress::completion_t> SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::estimateCompletion(const subsetSelection_t &subsetSelection, const possibleSubsets_t &possibleSubsets)
{
	const Progress::completion_t stepCompletion = 1.0 / std::pow(2.0, possibleSubsets.size());
	Progress::completion_t completion = 0.0, currentPositionWeight = 1.0;
	std::size_t lastSubset = 0;
	for (const std::size_t &subset : subsetSelection)
	{
		currentPositionWeight /= 2.0;
		while (++lastSubset != subset + 1)
		{
			completion += currentPositionWeight;
			currentPositionWeight /= 2.0;
		}
		completion += stepCompletion;
	}
	return {stepCompletion, completion};
}

template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
Progress::completion_t SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::estimateCompletion(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const
{
	Progress::completion_t completion = 0.0;
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		const auto [partialStepCompletion, partialCompletion] = estimateCompletion(subsetSelections[i], graph[i].second);
		completion *= partialStepCompletion;
		completion += partialCompletion;
	}
	return completion;
}

template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
bool SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::chooseNextSubsets(subsetSelections_t &subsetSelectionsIndexes, usageCounts_t &usageCounts) const
{
	try_again:
	// Don't let the familiar sight of the loop make an impression that control flow of this function is in any way understandable without analyzing specific paths.
	for (std::size_t i = 0; i != subsetSelectionsIndexes.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		const possibleSubsets_t &possibleSubsets = graph[i].second;
		if (possibleSubsets.empty())
			continue;
		subsetSelection_t &subsetSelectionIndex = subsetSelectionsIndexes[i];
		if (subsetSelectionIndex.empty())
		{
			subsetSelectionIndex.push_back(0);
			++usageCounts[possibleSubsets[0]];
		}
		else if (subsetSelectionIndex.back() != possibleSubsets.size() - 1)
		{
			const std::size_t nextSubset = subsetSelectionIndex.back() + 1;
			subsetSelectionIndex.push_back(nextSubset);
			++usageCounts[possibleSubsets[nextSubset]];
		}
		else if (subsetSelectionIndex.size() == 1)
		{
			--usageCounts[possibleSubsets.back()];
			subsetSelectionIndex.clear();
			continue;
		}
		else
		{
			subsetSelectionIndex.pop_back();
			--usageCounts[possibleSubsets[subsetSelectionIndex.back()]];
			++subsetSelectionIndex.back();
			--usageCounts[possibleSubsets.back()];
			++usageCounts[possibleSubsets[subsetSelectionIndex.back()]];
		}
		for (std::size_t j = 0; j != subsetSelectionsIndexes.size(); ++j)
			if (usageCounts[j] == 1 && subsetSelectionsIndexes[j].empty())
				goto try_again;
		return true;
	}
	return false;
}

template<typename SET, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
std::pair<typename SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::subsetSelections_t, typename SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::usageCounts_t> SetOptimizer<SET, VALUE_ID, FINDER_CONTAINER>::findBestSubsets(Progress &progress) const
{
	subsetSelections_t subsetSelectionIndexes(graph.size()), bestSubsetSelections(graph.size());
	usageCounts_t usageCounts(graph.size()), bestUsageCounts(graph.size());
	for (const std::size_t endNode : endNodes)
		usageCounts[endNode] = SIZE_MAX - graph.size();
	std::size_t bestGates = SIZE_MAX;
	const auto estimateCompletion = [this, &subsetSelectionIndexes = std::as_const(subsetSelectionIndexes), &usageCounts = std::as_const(usageCounts)](){ return SetOptimizer::estimateCompletion(subsetSelectionIndexes, usageCounts); };
	while (true)
	{
		progress.substep(estimateCompletion);
		subsetSelections_t subsetSelections(graph.size());
		for (std::size_t i = 0; i != subsetSelectionIndexes.size(); ++i)
		{
			subsetSelections[i].reserve(subsetSelectionIndexes[i].size());
			for (const std::size_t &subsetIndex : subsetSelectionIndexes[i])
				subsetSelections[i].push_back(graph[i].second[subsetIndex]);
		}
		
		const gateCount_t gates = countGates(subsetSelections, usageCounts);
		if (gates < bestGates)
		{
			bestSubsetSelections = std::move(subsetSelections);
			bestUsageCounts = usageCounts;
			bestGates = gates;
		}
		if (!chooseNextSubsets(subsetSelectionIndexes, usageCounts))
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
		auto foundOldSet = std::ranges::find(oldSets, newSet);
		do
		{
			finalSets[foundOldSet - oldSets.cbegin()] = endNode;
			foundOldSet = std::ranges::find(foundOldSet + 1, oldSets.cend(), newSet);
		} while (foundOldSet != oldSets.cend());
	}
	return finalSets;
}


#include "Implicant.hh"

template class SetOptimizer<Implicant, std::int_fast8_t, std::vector>;
template class SetOptimizer<std::set<std::size_t>, std::size_t, std::set>;
