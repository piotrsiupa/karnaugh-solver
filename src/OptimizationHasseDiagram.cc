#include "./OptimizationHasseDiagram.hh"

#include <algorithm>
#include <cstdint>


template<typename VALUE_T, template<typename> class CONTAINER>
typename OptimizationHasseDiagram<VALUE_T, CONTAINER>::groupsMap_t OptimizationHasseDiagram<VALUE_T, CONTAINER>::createGroups(const sets_t &sets)
{
	groupsMap_t groupsMap;
	for (const set_t &set : sets)
	{
		std::vector<VALUE_T> setCopy(set.cbegin(), set.cend());
		for (std::size_t i = 0; i != setCopy.size();)
		{
			const auto foundGroup = groupsMap.find(setCopy[i]);
			if (foundGroup == groupsMap.cend())
			{
				++i;
			}
			else
			{
				const std::size_t foundGroupIndex = foundGroup->second;
				group_t groupRejects;
				std::set_difference(groups[foundGroupIndex].cbegin(), groups[foundGroupIndex].cend(), setCopy.cbegin(), setCopy.cend(), std::back_inserter(groupRejects));
				if (!groupRejects.empty())
				{
					typename group_t::const_iterator currentRejectIt = groupRejects.cbegin(), endRejectIt = groupRejects.cend();
					groups[foundGroupIndex].erase(std::remove_if(groups[foundGroupIndex].begin(), groups[foundGroupIndex].end(), [&currentRejectIt, endRejectIt](const VALUE_T x){ if (currentRejectIt == endRejectIt || x != *currentRejectIt) return false; ++currentRejectIt; return true; }), groups[foundGroupIndex].end());
					groups.emplace_back(std::move(groupRejects));
					const std::size_t newGroupIndex = groups.size() - 1;
					for (const VALUE_T &value : groups.back())
						groupsMap.at(value) = newGroupIndex;
				}
				typename group_t::const_iterator currentValueIt = groups[foundGroupIndex].cbegin(), endValueIt = groups[foundGroupIndex].cend();
				setCopy.erase(std::remove_if(setCopy.begin(), setCopy.end(), [&currentValueIt, endValueIt](const VALUE_T x){ if (currentValueIt == endValueIt || x != *currentValueIt) return false; ++currentValueIt; return true; }), setCopy.end());
			}
		}
		if (!setCopy.empty())
		{
			groups.emplace_back(std::move(setCopy));
			const std::size_t newGroupIndex = groups.size() - 1;
			for (const VALUE_T &value : groups.back())
				groupsMap[value] = newGroupIndex;
		}
	}
	return groupsMap;
}

template<typename VALUE_T, template<typename> class CONTAINER>
bool OptimizationHasseDiagram<VALUE_T, CONTAINER>::contains(const groupedSet_t &set) const
{
	const Node *node = &root;
	for (const groupId_t &group : set)
	{
		const auto foundChild = node->children.find(group);
		if (foundChild == node->children.cend())
			return false;
		node = &foundChild->getNode();
	}
	return node->isOriginalSet;
}

template<typename VALUE_T, template<typename> class CONTAINER>
void OptimizationHasseDiagram<VALUE_T, CONTAINER>::insert(typename groupedSet_t::const_iterator currentInSet, const typename groupedSet_t::const_iterator &endOfSet, Node &currentNode, const setId_t setId, const bool primaryBranch)
{
	Node *nextNode;
	if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
	{
		nextNode = &foundChild->getNode();
		if (std::find(nextNode->setIds.cbegin(), nextNode->setIds.cend(), setId) == nextNode->setIds.cend())
			nextNode->setIds.emplace_back(setId);
	}
	else
	{
		nextNode = &currentNode.children.emplace_back(*currentInSet, new Node{{}, *currentInSet, {setId}, false}).getNode();
	}
	const typename groupedSet_t::const_iterator nextInSet = std::next(currentInSet);
	if (nextInSet == endOfSet)
	{
		if (primaryBranch)
			nextNode->isOriginalSet = true;
	}
	else
	{
		insert(nextInSet, endOfSet, *nextNode, setId, primaryBranch);
		insert(nextInSet, endOfSet, currentNode, setId, false);
	}
}

template<typename VALUE_T, template<typename> class CONTAINER>
void OptimizationHasseDiagram<VALUE_T, CONTAINER>::insertGrouped(const groupsMap_t &groupsMap, const sets_t &sets)
{
	setId_t currentSetId = 0;
	for (const set_t &set : sets)
	{
		groupedSet_t groupedSet;
		for (const VALUE_T &value : set)
			groupedSet.insert(groupsMap.at(value));
		if (!contains(groupedSet))
			insert(groupedSet.cbegin(), groupedSet.cend(), root, currentSetId++, true);
	}
}

template<typename VALUE_T, template<typename> class CONTAINER>
void OptimizationHasseDiagram<VALUE_T, CONTAINER>::makeSetHierarchy(groupedSetHierarchy_t &setHierarchy, const Node &node, const std::size_t subset) const
{
	currentGroupIds.push_back(node.value);
	if (!node.isOriginalSet && (node.setIds.size() == 1 || (currentGroupIds.size() == 1 && groups[currentGroupIds[0]].size() == 1)))
	{
		for (const NodeChild &child : node.children)
			makeSetHierarchy(setHierarchy, child.getNode(), subset);
	}
	else
	{
		const std::size_t addedSet = setHierarchy.size();
		if (subset == SIZE_MAX)
		{
			setHierarchy.push_back({currentGroupIds, {}, node.setIds, 0, node.isOriginalSet});
		}
		else
		{
			setHierarchy.push_back({currentGroupIds, {subset}, node.setIds, 0, node.isOriginalSet});
			++setHierarchy[subset].supersetCount;
		}
		for (const NodeChild &child : node.children)
			makeSetHierarchy(setHierarchy, child.getNode(), addedSet);
	}
	currentGroupIds.pop_back();
}

template<typename VALUE_T, template<typename> class CONTAINER>
void OptimizationHasseDiagram<VALUE_T, CONTAINER>::trimSetHierarchy(groupedSetHierarchy_t &setHierarchy)
{
	std::vector<std::size_t> offsets(setHierarchy.size());
	for (GroupedSetHierarchyEntry &entry : setHierarchy)
	{
		for (std::size_t i = 0; i != entry.subsets.size();)
		{
			const std::size_t subsetIndex = entry.subsets[i];
			GroupedSetHierarchyEntry &subset = setHierarchy[subsetIndex];
			if (subset.supersetCount == 1 && !subset.isOriginalSet)
			{
				offsets[subsetIndex] = 1;
				entry.subsets.erase(entry.subsets.begin() + i);
				entry.subsets.insert(entry.subsets.end(), subset.subsets.begin(), subset.subsets.end());
				subset.subsets.clear();
				subset.setIds.clear();
			}
			else
			{
				++i;
			}
		}
	}
	setHierarchy.erase(std::remove_if(setHierarchy.begin(), setHierarchy.end(), [](const GroupedSetHierarchyEntry &entry){ return entry.setIds.empty(); }), setHierarchy.end());
	setHierarchy.shrink_to_fit();
	
	std::size_t previousOffset = 0;
	for (std::size_t &offset : offsets)
	{
		offset += previousOffset;
		previousOffset = offset;
	}
	for (GroupedSetHierarchyEntry &entry : setHierarchy)
		for (std::size_t &subset : entry.subsets)
			subset -= offsets[subset];
}

template<typename VALUE_T, template<typename> class CONTAINER>
void OptimizationHasseDiagram<VALUE_T, CONTAINER>::addMoreEdgesToSetHierarchy(groupedSetHierarchy_t &setHierarchy)
{
	for (GroupedSetHierarchyEntry &entry0 : setHierarchy)
	{
		for (std::size_t j = 0; j != setHierarchy.size(); ++j)
		{
			if (entry0.groupIds.size() > setHierarchy[j].groupIds.size())
			{
				if (std::find(entry0.subsets.cbegin(), entry0.subsets.cend(), j) == entry0.subsets.cend() && std::includes(entry0.groupIds.cbegin(), entry0.groupIds.cend(), setHierarchy[j].groupIds.cbegin(), setHierarchy[j].groupIds.cend()))
				{
					entry0.subsets.push_back(j);
					++setHierarchy[j].supersetCount;
				}
			}
		}
	}
}

template<typename VALUE_T, template<typename> class CONTAINER>
void OptimizationHasseDiagram<VALUE_T, CONTAINER>::removeRedundantEdgesFromSetHierarchy(groupedSetHierarchy_t &setHierarchy)
{
	for (auto iter = setHierarchy.rbegin(); iter != setHierarchy.rend(); ++iter)
	{
		if (!iter->setIds.empty())
		{
			std::vector<std::size_t> subsetsOfSubsets;
			for (const std::size_t &subset : iter->subsets)
				subsetsOfSubsets.insert(subsetsOfSubsets.end(), setHierarchy[subset].subsets.cbegin(), setHierarchy[subset].subsets.cend());
			iter->subsets.erase(std::remove_if(iter->subsets.begin(), iter->subsets.end(), [&subsetsOfSubsets = std::as_const(subsetsOfSubsets)](const std::size_t x) { return std::find(subsetsOfSubsets.cbegin(), subsetsOfSubsets.cend(), x) != subsetsOfSubsets.cend(); }), iter->subsets.end());
			iter->subsets.shrink_to_fit();
		}
	}
}

template<typename VALUE_T, template<typename> class CONTAINER>
void OptimizationHasseDiagram<VALUE_T, CONTAINER>::sortSetHierarchy(groupedSetHierarchy_t &setHierarchy)
{
	std::vector<std::size_t> sortOrder;
	sortOrder.reserve(setHierarchy.size());
	std::vector<std::size_t> entriesToProcess;
	entriesToProcess.reserve(setHierarchy.size() * 2);
	for (std::size_t i = 0; i != setHierarchy.size(); ++i)
		entriesToProcess.push_back(setHierarchy.size() - 1 - i);
	std::vector<bool> processedEntries(setHierarchy.size(), false);
	while (!entriesToProcess.empty())
	{
		const std::size_t currentEntry = entriesToProcess.back();
		if (processedEntries[currentEntry])
		{
			entriesToProcess.pop_back();
		}
		else
		{
			bool allSubsetsReady = true;
			for (const std::size_t &subset : setHierarchy[currentEntry].subsets)
			{
				if (!processedEntries[subset])
				{
					allSubsetsReady = false;
					entriesToProcess.push_back(subset);
				}
			}
			if (allSubsetsReady)
			{
				sortOrder.push_back(currentEntry);
				entriesToProcess.pop_back();
				processedEntries[currentEntry] = true;
			}
		}
	}
	
	std::vector<std::size_t> reverseSortOrder(sortOrder.size());
	for (std::size_t i = 0; i != sortOrder.size(); ++i)
		reverseSortOrder[sortOrder[i]] = i;
	
	groupedSetHierarchy_t sortedSetHierarchy;
	sortedSetHierarchy.reserve(setHierarchy.size());
	for (const std::size_t &index : sortOrder)
	{
		sortedSetHierarchy.push_back(std::move(setHierarchy[index]));
		for (std::size_t &subset : sortedSetHierarchy.back().subsets)
			subset = reverseSortOrder[subset];
	}
	setHierarchy = std::move(sortedSetHierarchy);
}

template<typename VALUE_T, template<typename> class CONTAINER>
typename OptimizationHasseDiagram<VALUE_T, CONTAINER>::setHierarchy_t OptimizationHasseDiagram<VALUE_T, CONTAINER>::ungroupSetHierarchy(groupedSetHierarchy_t &groupedSetHierarchy) const
{
	setHierarchy_t setHierarchy;
	for (GroupedSetHierarchyEntry &groupedSetHierarchyEntry : groupedSetHierarchy)
	{
		std::vector<value_t> ungroupedValues;
		for (const groupId_t groupId : groupedSetHierarchyEntry.groupIds)
			ungroupedValues.insert(ungroupedValues.end(), groups.at(groupId).cbegin(), groups.at(groupId).cend());
		std::sort(ungroupedValues.begin(), ungroupedValues.end());
		setHierarchy.push_back({std::move(ungroupedValues), std::move(groupedSetHierarchyEntry.subsets), std::move(groupedSetHierarchyEntry.setIds), groupedSetHierarchyEntry.supersetCount, groupedSetHierarchyEntry.isOriginalSet});
	}
	return setHierarchy;
}

template<typename VALUE_T, template<typename> class CONTAINER>
OptimizationHasseDiagram<VALUE_T, CONTAINER>::OptimizationHasseDiagram(const sets_t &sets)
{
	const groupsMap_t groupsMap = createGroups(sets);
	insertGrouped(groupsMap, sets);
}

template<typename VALUE_T, template<typename> class CONTAINER>
typename OptimizationHasseDiagram<VALUE_T, CONTAINER>::setHierarchy_t OptimizationHasseDiagram<VALUE_T, CONTAINER>::makeSetHierarchy() const
{
	currentGroupIds.clear();
	groupedSetHierarchy_t setHierarchy;
	for (const NodeChild &child : root.children)
		makeSetHierarchy(setHierarchy, child.getNode(), SIZE_MAX);
	addMoreEdgesToSetHierarchy(setHierarchy);
	trimSetHierarchy(setHierarchy);
	removeRedundantEdgesFromSetHierarchy(setHierarchy);
	sortSetHierarchy(setHierarchy);
	return ungroupSetHierarchy(setHierarchy);
}


#include <set>

template class OptimizationHasseDiagram<std::int8_t, std::vector>;
template class OptimizationHasseDiagram<std::size_t, std::set>;
