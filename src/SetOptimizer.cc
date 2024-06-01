#include "./SetOptimizer.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <ranges>

#include "Implicant.hh"
#include "options.hh"
#include "utils.hh"


template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::Result SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::extractCommonParts(sets_t &&oldSets, Progress &progress)
{
	subsetSelections_t subsetSelections = findBestSubsets(oldSets, progress);
	sets_t newSets = makeSets();
	const finalSets_t finalSets = makeFinalSets(std::move(oldSets), newSets);
	substractSubsets(newSets, subsetSelections);
	assert(newSets.size() == subsetSelections.size());
	return {newSets, finalSets, subsetSelections};
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::makeFullGraph(const sets_t &oldSets)
{
	const typename SubsetFinder::setHierarchy_t setHierarchy = SubsetFinder::makeSetHierarchy(convertSets(oldSets));
	makeGraph(setHierarchy);
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::makeGreedyGraph(const sets_t &oldSets, Progress &progress)
{
	static constexpr auto lessForSorting = [](const set_t &x, const set_t &y) -> bool { return x.size() != y.size() ? x.size() > y.size() : x > y; };
	
	graph.reserve(oldSets.size());
	for (const set_t &oldSet : oldSets)
		graph.emplace_back(oldSet, possibleSubsets_t{});
	std::ranges::sort(graph, [](const graphNode_t &x, const graphNode_t &y){ return lessForSorting(x.set, y.set); });
	const auto [first, last] = std::ranges::unique(graph, {}, [](const graphNode_t &graphNode){ return graphNode.set; });
	graph.erase(first, last);
	for (std::size_t i = 0; i != graph.size(); ++i)
		endNodes.insert(endNodes.end(), i);
	
	std::size_t i = 0;
	const auto estimateCompletion = [&i = std::as_const(i), &graph = std::as_const(graph)](){ return static_cast<Progress::completion_t>(i) / static_cast<Progress::completion_t>(graph.size()); };
	for (; i != graph.size(); ++i)
	{
		progress.substep(estimateCompletion);
		
		set_t remainingElements = graph[i].set;
		for (std::size_t j : graph[i].possibleSubsets)
			substractSet(remainingElements, graph[j].set);
		for (std::size_t j = i + 1; j != graph.size(); ++j)
		{
			const set_t otherSet = graph[j].set;
			set_t commonElements = getSetIntersection(remainingElements, otherSet);
			if (isSubsetWorthy(commonElements))
			{
				substractSet(remainingElements, commonElements);
				if (commonElements == otherSet)  // The other set doesn't need to be checked because they are unique and sorted by size.
				{
					graph[i].possibleSubsets.push_back(j);
					continue;
				}
				const typename graph_t::const_iterator insertionPoint = std::lower_bound(graph.cbegin(), graph.cend(), commonElements, [](const graphNode_t &node, const set_t &newSet){ return lessForSorting(node.set, newSet); });
				const std::size_t newElemIndex = std::ranges::distance(graph.cbegin(), insertionPoint);
				if (insertionPoint != graph.cend() && insertionPoint->set == commonElements)
				{
					graph[i].possibleSubsets.push_back(newElemIndex);
					continue;
				}
				graph.emplace(insertionPoint, std::move(commonElements), possibleSubsets_t{});
				for (typename endNodes_t::reverse_iterator iter = endNodes.rbegin(); iter != endNodes.rend(); ++iter)
				{
					const std::size_t endNode = *iter;
					if (endNode < newElemIndex)
						break;
					endNodes.insert(endNode + 1);
					endNodes.erase(endNode);
				}
				for (auto &graphElement : graph)
					std::ranges::for_each(graphElement.possibleSubsets, [newElemIndex](std::size_t &x){ if (x >= newElemIndex) ++x; });
				graph[i].possibleSubsets.push_back(newElemIndex);
				graph[j].possibleSubsets.push_back(newElemIndex);
			}
		}
	}
	
	std::ranges::reverse(graph);
	for (auto &graphElement : graph)
		std::ranges::for_each(graphElement.possibleSubsets, [difference = graph.size() - 1](std::size_t &x){ x = difference - x; });
	std::set<std::size_t> newEndNodes;
	std::ranges::transform(endNodes, std::inserter(newEndNodes, newEndNodes.end()), [difference = graph.size() - 1](const std::size_t endNode){ return difference - endNode; });
	endNodes = std::move(newEndNodes);
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
std::size_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::getMaxRoughDepth()
{
	switch (options::maxRoughDepth.getValue())
	{
	case 0:
		return SIZE_MAX;
	case SIZE_MAX:
		return 3;
	default:
		return options::maxRoughDepth.getValue();
	}
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
std::size_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::getMaxRoughWidth()
{
	switch (options::maxRoughWidth.getValue())
	{
	case 0:
		return SIZE_MAX;
	case SIZE_MAX:
		return 16;
	default:
		return options::maxRoughWidth.getValue();
	}
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::makeRoughGraph(const sets_t &oldSets, Progress &progress)
{
	std::vector<setElement_t> allSetElements = getAllSetElements(oldSets);
	if (const std::size_t maxWidth = getMaxRoughWidth(); allSetElements.size() > maxWidth)
		allSetElements.resize(maxWidth);
	
	graph.emplace_back(set_t(), possibleSubsets_t{});
	if (std::ranges::find(oldSets, set_t()) != oldSets.cend()) [[unlikely]]
		endNodes.insert(0);
	std::vector<typename decltype(allSetElements)::const_iterator> nextElements{allSetElements.cbegin()};
	if constexpr (std::is_same_v<SET, Implicant>)
	{
		if (const auto foundSet = std::ranges::find(oldSets, Implicant::none()); foundSet != oldSets.cend()) [[unlikely]]
		{
			graph.emplace_back(*foundSet, possibleSubsets_t{});
			endNodes.insert(1);
			nextElements.push_back(allSetElements.cbegin());
		}
	}
	std::size_t prevLevelStart = 0, prevLevelEnd = 1;
	std::size_t i;
	typename decltype(allSetElements)::const_iterator elementToInsert;
	
	const std::size_t maxSetSize = std::min(getMaxRoughDepth(), static_cast<std::size_t>(std::ranges::max(oldSets, std::ranges::less(), [](const set_t &set)->std::size_t{ return set.size(); }).size()));
	const Progress::calcStepCompletion_t calcStepCompletion = [
			maxSetSize,
			&graph = std::as_const(graph),
			&prevLevelStart = std::as_const(prevLevelStart),
			&prevLevelEnd = std::as_const(prevLevelEnd),
			&i = std::as_const(i),
			&allSetElements = std::as_const(allSetElements),
			&elementToInsert = std::as_const(elementToInsert)
		](){
			return	(
						static_cast<Progress::completion_t>(graph.back().set.size())
						+ (
							(
								static_cast<Progress::completion_t>(i - prevLevelStart)
								+ (
									static_cast<Progress::completion_t>(std::ranges::distance(allSetElements.cbegin(), elementToInsert))
									/ static_cast<Progress::completion_t>(allSetElements.size())
								)
							)
							/ static_cast<Progress::completion_t>(prevLevelEnd - prevLevelStart)
						)
					)
					/ static_cast<Progress::completion_t>(maxSetSize + 1);
		};
	
	while (prevLevelStart != prevLevelEnd && graph.back().set.size() != maxSetSize)
	{
		for (i = prevLevelStart; i != prevLevelEnd; ++i)
		{
			for (elementToInsert = nextElements[i]; elementToInsert != allSetElements.cend(); ++elementToInsert)
			{
				progress.substep(calcStepCompletion);
				set_t newSet = addSetElement(graph[i].set, *elementToInsert);
				if (newSet.empty())
					continue;
				graphNode_t &graphNode = graph.emplace_back(std::move(newSet), possibleSubsets_t{});
				bool isUsed = false;
				for (std::size_t i = 0; i != oldSets.size(); ++i)
				{
					if (setContainsSet(oldSets[i], graphNode.set))
					{
						isUsed = true;
						if (oldSets[i].size() == graphNode.set.size())
							endNodes.insert(graph.size() - 1);
					}
				}
				if (!isUsed)
				{
					graph.pop_back();
					continue;
				}
				const auto &nextElement = nextElements.emplace_back(std::ranges::next(elementToInsert));
				if (prevLevelStart != 0) [[likely]]
				{
					for (auto subsetElement = allSetElements.cbegin(); subsetElement != nextElement; ++subsetElement)
					{
						const set_t parentDiff = addSetElement(set_t(), *subsetElement);
						if (!setContainsSet(graphNode.set, parentDiff))
							continue;
						set_t subsetParent = graphNode.set;
						substractSet(subsetParent, parentDiff);
						const auto foundParent = std::ranges::find(std::ranges::next(graph.cbegin(), prevLevelStart), std::ranges::next(graph.cbegin(), prevLevelEnd), subsetParent, [](const graphNode_t &graphNode){ return graphNode.set; });
						assert(foundParent != std::ranges::next(graph.begin(), prevLevelEnd));
						graphNode.possibleSubsets.push_back(std::ranges::distance(graph.cbegin(), foundParent));
					}
				}
			}
		}
		prevLevelStart = prevLevelEnd;
		if constexpr (std::is_same_v<SET, Implicant>)
			if (prevLevelStart == 1 && graph.size() > 1 && graph[1].set.size() == 0) [[unlikely]]
				prevLevelStart = 2;
		prevLevelEnd = graph.size();
	}
	sets_t remainingSets;
	std::ranges::copy_if(oldSets, std::back_inserter(remainingSets), [maxSetSize](const set_t &set){ return set.size() > maxSetSize; });
	std::ranges::sort(remainingSets);
	for (set_t &remainingSet : remainingSets)
	{
		if (remainingSet != graph.back().set)
		{
			endNodes.insert(graph.size());
			graph.emplace_back(std::move(remainingSet), possibleSubsets_t{});
			if (prevLevelStart != 0)
				for (std::size_t i = prevLevelStart; i != prevLevelEnd; ++i)
					if (setContainsSet(graph.back().set, graph[i].set))
						graph.back().possibleSubsets.push_back(i);
		}
	}
	
	const permutation_t newIndexes = makeRemovingPermutationByIndex(graph, [this](const std::size_t i){ return !isSubsetWorthy(graph[i].set) && !endNodes.contains(i); });
	removeByPermutation(graph, newIndexes);
	for (graphNode_t &graphNode : graph)
		removeAndPermuteIndexes(graphNode.possibleSubsets, newIndexes);
	permuteIndexes(endNodes, newIndexes);
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::makeGraph(const sets_t &oldSets, Progress &progress)
{
	switch (options::optimizationHeuristics.getValue())
	{
	case options::OptimizationHeuristic::BRUTE_FORCE:
	case options::OptimizationHeuristic::EXHAUSTIVE:
	case options::OptimizationHeuristic::CURSORY:
		makeFullGraph(oldSets);
		break;
	case options::OptimizationHeuristic::GREEDY:
		makeGreedyGraph(oldSets, progress);
		break;
	case options::OptimizationHeuristic::ROUGH:
		makeRoughGraph(oldSets, progress);
		break;
	}
	assert(endNodes.size() <= oldSets.size());
	assert(graph.size() >= endNodes.size());
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::switchToParentNodesIfAllowed(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const
{
	for (std::size_t nodeIndex = 0; nodeIndex != subsetSelections.size(); ++nodeIndex)
	{
		if (usageCounts[nodeIndex] == 0)
			continue;
		bool changed = false;
		subsetSelection_t &subsetSelection = subsetSelections[nodeIndex];
		for (std::size_t targetSubsetIndex = 0; targetSubsetIndex != subsetSelection.size(); ++targetSubsetIndex)
		{
			if (subsetSelections[subsetSelection[targetSubsetIndex]].empty())  // There is no parents to switch to.
				continue;
			set_t missingElementsWithoutTarget = graph[nodeIndex].set;
			for (std::size_t i = 0; i != subsetSelection.size(); ++i)
				if (i != targetSubsetIndex)
					substractSet(missingElementsWithoutTarget, graph[subsetSelection[i]].set);
			set_t correctMissingElements = missingElementsWithoutTarget;
			substractSet(correctMissingElements, graph[subsetSelection[targetSubsetIndex]].set);
			repeat_subnode_index:
			for (std::size_t subSubNode : subsetSelections[subsetSelection[targetSubsetIndex]])
			{
				set_t missingElementsWithParentInstead = missingElementsWithoutTarget;
				substractSet(missingElementsWithParentInstead, graph[subSubNode].set);
				if (missingElementsWithParentInstead == correctMissingElements)
				{
					assert(usageCounts[subsetSelection[targetSubsetIndex]] != 0);
					--usageCounts[subsetSelection[targetSubsetIndex]];
					assert(usageCounts[subSubNode] != SIZE_MAX);
					++usageCounts[subSubNode];
					subsetSelection[targetSubsetIndex] = subSubNode;
					changed = true;
					if (subsetSelections[subSubNode].empty())  // There is no parent to switch to.
						break;
					else
						goto repeat_subnode_index;
				}
			}
		}
		if (changed)
		{
			assert(!subsetSelection.empty());
			std::ranges::sort(subsetSelection);
			std::size_t prev = subsetSelection.front();
			for (auto iter = std::ranges::next(subsetSelection.cbegin()); iter != subsetSelection.cend(); ++iter)
				if (*iter == prev)
					--usageCounts[*iter];
				else
					prev = *iter;
			const auto [eraseBegin, eraseEnd] = std::ranges::unique(subsetSelection);
			subsetSelection.erase(eraseBegin, eraseEnd);
		}
	}
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::removeSingleUseNonFinalNodes(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const
{
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		subsetSelection_t &subsetSelection = subsetSelections[i];
		bool changed = false;
		for (std::size_t j = 0; j != subsetSelection.size();)
		{
			const std::size_t subset = subsetSelection[j];
			if (usageCounts[subset] == 1)
			{
				subsetSelection.erase(std::ranges::next(subsetSelection.begin(), j));
				subsetSelection.insert(subsetSelection.end(), subsetSelections[subset].cbegin(), subsetSelections[subset].cend());
				usageCounts[subset] = 0;
				subsetSelections[subset].clear();
				changed = true;
				continue;
			}
			++j;
		}
		if (changed && !subsetSelection.empty())
		{
			std::ranges::sort(subsetSelection);
			std::size_t prev = subsetSelection.front();
			for (auto iter = std::ranges::next(subsetSelection.cbegin()); iter != subsetSelection.cend(); ++iter)
				if (*iter == prev)
					--usageCounts[*iter];
				else
					prev = *iter;
			const auto [eraseBegin, eraseEnd] = std::ranges::unique(subsetSelection);
			subsetSelection.erase(eraseBegin, eraseEnd);
		}
	}
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::removeUnnecessaryParents(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const
{
	for (std::size_t nodeIndex = 0; nodeIndex != subsetSelections.size(); ++nodeIndex)
	{
		if (usageCounts[nodeIndex] == 0)
			continue;
		subsetSelection_t &subsetSelection = subsetSelections[nodeIndex];
		set_t missingElements = graph[nodeIndex].set;
		for (std::size_t i = 0; i != subsetSelection.size(); ++i)
			substractSet(missingElements, graph[subsetSelection[i]].set);
		for (std::size_t targetSubsetIndex = 0; targetSubsetIndex != subsetSelection.size();)
		{
			set_t newMissingElements = graph[nodeIndex].set;
			for (std::size_t i = 0; i != subsetSelection.size(); ++i)
				if (i != targetSubsetIndex)
					substractSet(newMissingElements, graph[subsetSelection[i]].set);
			if (newMissingElements.size() - missingElements.size() <= 1)
			{
				--usageCounts[subsetSelection[targetSubsetIndex]];
				subsetSelection.erase(std::ranges::next(subsetSelection.begin(), targetSubsetIndex));
			}
			else
			{
				++targetSubsetIndex;
			}
		}
	}
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::removeUnusedNodes(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts)
{
	std::size_t moveTarget = SIZE_MAX;
	for (std::size_t i = graph.size(); i --> 0;)
	{
		if (usageCounts[i] == 0)
		{
			moveTarget = i;
			for (const std::size_t subnode : subsetSelections[i])
				--usageCounts[subnode];
		}
	}
	if (moveTarget == SIZE_MAX)
		return;
	
	std::vector<std::size_t> indexMap(graph.size());
	std::size_t i;
	for (i = 0; i != moveTarget; ++i)
		indexMap[i] = i;
	for (++i; i != graph.size(); ++i)
	{
		if (usageCounts[i] != 0)
		{
			graph[moveTarget] = std::move(graph[i]);
			subsetSelections[moveTarget] = std::move(subsetSelections[i]);
			usageCounts[moveTarget] = std::move(usageCounts[i]);
			indexMap[i] = moveTarget;
			++moveTarget;
		}
	}
	graph.resize(moveTarget);
	subsetSelections.resize(moveTarget);
	usageCounts.resize(moveTarget);
	
	for (std::vector<std::size_t> &x : subsetSelections)
		for (std::size_t &subset : x)
			subset = indexMap[subset];
	
	endNodes_t newEndNodes;
	for (const std::size_t &oldEndNode : endNodes)
		newEndNodes.insert(indexMap[oldEndNode]);
	endNodes = std::move(newEndNodes);
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::removeRedundantNodes(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts, const bool switchToParents)
{
	if (switchToParents)
		switchToParentNodesIfAllowed(subsetSelections, usageCounts);
	removeUnusedNodes(subsetSelections, usageCounts);
	removeSingleUseNonFinalNodes(subsetSelections, usageCounts);
	removeUnnecessaryParents(subsetSelections, usageCounts);
	removeUnusedNodes(subsetSelections, usageCounts);
	assert(graph.size() == subsetSelections.size() && subsetSelections.size() == usageCounts.size());
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
std::pair<Progress::completion_t, Progress::completion_t> SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::estimateBruteForceCompletion(const subsetSelection_t &subsetSelection, const possibleSubsets_t &possibleSubsets)
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

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
Progress::completion_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::estimateBruteForceCompletion(const subsetSelections_t &subsetSelections, const usageCounts_t &usageCounts) const
{
	Progress::completion_t completion = 0.0;
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		const auto [partialStepCompletion, partialCompletion] = estimateBruteForceCompletion(subsetSelections[i], graph[i].possibleSubsets);
		completion *= partialStepCompletion;
		completion += partialCompletion;
	}
	return completion;
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
bool SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::chooseNextSubsetsForBruteForce(subsetSelections_t &subsetSelectionsIndexes, usageCounts_t &usageCounts) const
{
	try_again:
	// Don't let the familiar sight of the loop make an impression that control flow of this function is in any way understandable without analyzing specific paths.
	for (std::size_t i = 0; i != subsetSelectionsIndexes.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		const possibleSubsets_t &possibleSubsets = graph[i].possibleSubsets;
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

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::subsetSelections_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::processGraph_bruteForce(Progress &progress)
{
	subsetSelections_t subsetSelectionIndexes(graph.size()), bestSubsetSelections(graph.size());
	usageCounts_t usageCounts(graph.size()), bestUsageCounts(graph.size());
	for (const std::size_t endNode : endNodes)
		usageCounts[endNode] = SIZE_MAX - graph.size();
	gateCount_t bestGates = SIZE_MAX;
	const auto estimateCompletion = [this, &subsetSelectionIndexes = std::as_const(subsetSelectionIndexes), &usageCounts = std::as_const(usageCounts)](){ return SetOptimizer::estimateBruteForceCompletion(subsetSelectionIndexes, usageCounts); };
	while (true)
	{
		progress.substep(estimateCompletion);
		subsetSelections_t subsetSelections(graph.size());
		for (std::size_t i = 0; i != subsetSelectionIndexes.size(); ++i)
		{
			subsetSelections[i].reserve(subsetSelectionIndexes[i].size());
			for (const std::size_t &subsetIndex : subsetSelectionIndexes[i])
				subsetSelections[i].push_back(graph[i].possibleSubsets[subsetIndex]);
		}
		
		const gateCount_t gates = countGates(subsetSelections, usageCounts);
		if (gates < bestGates)
		{
			bestSubsetSelections = std::move(subsetSelections);
			bestUsageCounts = usageCounts;
			bestGates = gates;
		}
		if (!chooseNextSubsetsForBruteForce(subsetSelectionIndexes, usageCounts))
			break;
	}
	
	removeRedundantNodes(bestSubsetSelections, bestUsageCounts, true);
	
	return bestSubsetSelections;
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
bool SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::chooseNextSubsetsForExhaustive(usageCounts_t &usageCounts) const
{
	for (std::size_t i = 0; i != graph.size(); ++i)
	{
		if (usageCounts[i] == SIZE_MAX)
			continue;
		if (usageCounts[i] == 0)
		{
			usageCounts[i] = 1;
			return true;
		}
		else
		{
			usageCounts[i] = 0;
			continue;
		}
	}
	return false;
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
void SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::cleanupResultOfExhaustive(subsetSelections_t &subsetSelections, usageCounts_t &usageCounts) const
{
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		for (std::size_t subset : subsetSelections[i])
			if (usageCounts[subset] != SIZE_MAX)
				++usageCounts[subset];
	}
	assert(std::ranges::none_of(usageCounts, [](std::size_t x){ return x == 1; }));
	
	for (std::size_t i = 0; i != graph.size(); ++i)
		if (usageCounts[i] == SIZE_MAX)
			usageCounts[i] -= graph.size();
		else if (usageCounts[i] != 0)
			--usageCounts[i];
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::subsetSelections_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::processGraph_exhaustive(Progress &progress)
{
	auto progressStep = progress.makeCountingStepHelper(std::pow(Progress::completion_t(2), graph.size() - endNodes.size()));
	
	subsetSelections_t subsetSelections(graph.size()), bestSubsetSelections(graph.size());
	
	usageCounts_t usageCounts(graph.size()), bestUsageCounts(graph.size());
	for (const std::size_t endNode : endNodes)
		usageCounts[endNode] = SIZE_MAX;
	
	gateCount_t bestGates = SIZE_MAX;
	
	while (true)
	{
		progressStep.substep();
		
		for (std::size_t i = 0; i != graph.size(); ++i)
		{
			subsetSelection_t &subsetSelection = subsetSelections[i];
			subsetSelection.clear();
			for (const std::size_t &possibleSubset : graph[i].possibleSubsets)
				if (usageCounts[possibleSubset] != 0)
					subsetSelection.push_back(possibleSubset);
		}
		
		const gateCount_t gates = countGates(subsetSelections, usageCounts);
		if (gates < bestGates)
		{
			bestSubsetSelections = subsetSelections;
			bestUsageCounts = usageCounts;
			bestGates = gates;
		}
		
		if (!chooseNextSubsetsForExhaustive(usageCounts))
			break;
	}
	
	cleanupResultOfExhaustive(bestSubsetSelections, bestUsageCounts);
	removeRedundantNodes(bestSubsetSelections, bestUsageCounts, true);
	
	return bestSubsetSelections;
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::subsetSelections_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::processGraph_cursory(Progress &progress)
{
	progress.substep(0.0);
	
	subsetSelections_t subsetSelections(graph.size());
	usageCounts_t usageCounts(graph.size());
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
	{
		subsetSelections[i] = graph[i].possibleSubsets;
		for (std::size_t subset : subsetSelections[i])
			++usageCounts[subset];
	}
	for (const std::size_t endNode : endNodes)
		usageCounts[endNode] = SIZE_MAX - graph.size();
	std::vector<bool> subsetsToRemove(subsetSelections.size());
	for (std::size_t i = 0; i != subsetSelections.size(); ++i)
	{
		subsetSelection_t &subsetSelection = subsetSelections[i];
		set_t correctMissingElements = graph[i].set;
		for (const std::size_t subset : subsetSelection)
			substractSet(correctMissingElements, graph[subset].set);
		std::fill(subsetsToRemove.begin(), subsetsToRemove.end(), false);
		for (const std::size_t j : makeSortingPermutation(subsetSelection, std::ranges::less{}, [&subsetSelections](const std::size_t subset){ return subsetSelections[subset].size(); }))
		{
			set_t missingElementsWithoutSubset = graph[i].set;
			for (std::size_t k = 0; k != subsetSelection.size(); ++k)
				if (k != j && !subsetsToRemove[subsetSelection[k]])
					substractSet(missingElementsWithoutSubset, graph[subsetSelection[k]].set);
			if (missingElementsWithoutSubset == correctMissingElements)
			{
				--usageCounts[subsetSelection[j]];
				subsetsToRemove[subsetSelection[j]] = true;
			}
		}
		const auto [eraseBegin, eraseEnd] = std::ranges::remove_if(subsetSelection, [subsetsToRemove](const std::size_t x){ return subsetsToRemove[x]; });
		subsetSelection.erase(eraseBegin, eraseEnd);
	}
	removeRedundantNodes(subsetSelections, usageCounts, true);
	
	return subsetSelections;
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::subsetSelections_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::processGraph_greedy(Progress &progress)
{
	progress.substep(0.0);
	
	subsetSelections_t subsetSelections(graph.size());
	for (std::size_t i = 0; i != graph.size(); ++i)
		subsetSelections[i] = graph[i].possibleSubsets;
	
	return subsetSelections;
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::subsetSelections_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::processGraph_rough(Progress &progress)
{
	progress.substep(0.0);
	
	usageCounts_t usageCounts(graph.size(), 0);
	subsetSelections_t subsetSelections(graph.size());
	for (std::size_t i = 0; i != graph.size(); ++i)
	{
		subsetSelections[i] = graph[i].possibleSubsets;
		for (const std::size_t &subset : graph[i].possibleSubsets)
			++usageCounts[subset];
	}
	for (const std::size_t &endNode : endNodes)
		++usageCounts[endNode];
	
	removeRedundantNodes(subsetSelections, usageCounts, false);
	
	return subsetSelections;
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::subsetSelections_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::processGraph(Progress &progress)
{
	switch (options::optimizationHeuristics.getValue())
	{
	case options::OptimizationHeuristic::BRUTE_FORCE:
		return processGraph_bruteForce(progress);
	case options::OptimizationHeuristic::EXHAUSTIVE:
		return processGraph_exhaustive(progress);
	case options::OptimizationHeuristic::CURSORY:
		return processGraph_cursory(progress);
	case options::OptimizationHeuristic::GREEDY:
		return processGraph_greedy(progress);
	case options::OptimizationHeuristic::ROUGH:
		return processGraph_rough(progress);
	}
	
	// unreachable
	return {};
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::subsetSelections_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::findBestSubsets(const sets_t &oldSets, Progress &progress)
{
	makeGraph(oldSets, progress);
	return processGraph(progress);
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::sets_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::makeSets() const
{
	sets_t sets;
	sets.reserve(graph.size());
	for (const auto &graphNode : graph)
		sets.push_back(graphNode.set);
	return sets;
}

template<typename SET, typename SET_ELEMENT, typename VALUE_ID, template<typename> class FINDER_CONTAINER>
typename SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::finalSets_t SetOptimizer<SET, SET_ELEMENT, VALUE_ID, FINDER_CONTAINER>::makeFinalSets(sets_t &&oldSets, const sets_t &newSets)
{
	const permutation_t oldSetsPermutation = makeSortingPermutation(oldSets);
	applyPermutation(oldSets, oldSetsPermutation);
	
	finalSets_t finalSets(oldSets.size());
	for (const std::size_t endNode : endNodes)
	{
		const auto [first, last] = std::ranges::equal_range(oldSets, newSets[endNode]);
		for (auto current = first; current != last; ++current)
			finalSets[oldSetsPermutation[std::ranges::distance(oldSets.cbegin(), current)]] = endNode;
	}
	
	return finalSets;
}


template class SetOptimizer<Implicant, Implicant::splitBit_t, std::int_fast8_t, std::vector>;
template class SetOptimizer<std::set<std::size_t>, std::size_t, std::size_t, std::set>;
