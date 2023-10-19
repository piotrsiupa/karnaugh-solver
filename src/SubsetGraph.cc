#include "./SubsetGraph.hh"

#include <algorithm>
#include <set>


template<typename VALUE_T, template<typename> class CONTAINER>
typename SubsetGraph<VALUE_T, CONTAINER>::groupsMap_t SubsetGraph<VALUE_T, CONTAINER>::createGroups(const sets_t &sets)
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
typename SubsetGraph<VALUE_T, CONTAINER>::groupedSets_t SubsetGraph<VALUE_T, CONTAINER>::groupSets(const sets_t &sets, const groupsMap_t &groupsMap)
{
	groupedSets_t groupedSets;
	for (const set_t &set : sets)
	{
		std::set<groupId_t> groupIds;
		for (const VALUE_T &value : set)
			groupIds.insert(groupsMap.at(value));
		groupedSets.emplace_back(groupIds.begin(), groupIds.end());
	}
	return groupedSets;
}

template<typename VALUE_T, template<typename> class CONTAINER>
void SubsetGraph<VALUE_T, CONTAINER>::makeInitialSetHierarchy(const groupedSets_t &groupedSets)
{
	std::map<groupedSet_t, GroupedSetHierarchyEntry> partialSets;
	for (std::size_t i = 0; i != groupedSets.size(); ++i)
	{
		if (const auto foundPartialSet = partialSets.find(groupedSets[i]); foundPartialSet != partialSets.end())
		{
			foundPartialSet->second.setIds.push_back(i);
			foundPartialSet->second.isOriginalSet = true;
		}
		else
		{
			partialSets[groupedSets[i]] = GroupedSetHierarchyEntry{groupedSets[i], {}, {i}, 0, true};
		}
		for (std::size_t j = i + 1; j != groupedSets.size(); ++j)
		{
			groupedSet_t partialSet;
			std::set_intersection(groupedSets[i].cbegin(), groupedSets[i].cend(), groupedSets[j].cbegin(), groupedSets[j].cend(), std::back_inserter(partialSet));
			if (partialSet.size() > 1 || (partialSet.size() == 1 && groups[partialSet[0]].size() > 1))
			{
				if (const auto foundPartialSet = partialSets.find(partialSet); foundPartialSet != partialSets.end())
				{
					if (groupedSets[i].size() != partialSet.size())
						foundPartialSet->second.setIds.push_back(i);
					if (groupedSets[j].size() != partialSet.size())
						foundPartialSet->second.setIds.push_back(j);
				}
				else
				{
					partialSets[partialSet] = GroupedSetHierarchyEntry{partialSet, {}, {i, j}, 0, false};
				}
			}
		}
	}
	for (auto &partialSet : partialSets)
	{
		std::set<setId_t> setIds(partialSet.second.setIds.begin(), partialSet.second.setIds.end());
		partialSet.second.setIds = {setIds.begin(), setIds.end()};
		groupedSetHierarchy.push_back(std::move(partialSet.second));
	}
}

template<typename VALUE_T, template<typename> class CONTAINER>
void SubsetGraph<VALUE_T, CONTAINER>::trimSetHierarchy()
{
	std::vector<std::size_t> offsets(groupedSetHierarchy.size());
	for (GroupedSetHierarchyEntry &entry : groupedSetHierarchy)
	{
		for (std::size_t i = 0; i != entry.subsets.size();)
		{
			const std::size_t subsetIndex = entry.subsets[i];
			GroupedSetHierarchyEntry &subset = groupedSetHierarchy[subsetIndex];
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
	groupedSetHierarchy.erase(std::remove_if(groupedSetHierarchy.begin(), groupedSetHierarchy.end(), [](const GroupedSetHierarchyEntry &entry){ return entry.setIds.empty(); }), groupedSetHierarchy.end());
	groupedSetHierarchy.shrink_to_fit();
	
	std::size_t previousOffset = 0;
	for (std::size_t &offset : offsets)
	{
		offset += previousOffset;
		previousOffset = offset;
	}
	for (GroupedSetHierarchyEntry &entry : groupedSetHierarchy)
		for (std::size_t &subset : entry.subsets)
			subset -= offsets[subset];
}

template<typename VALUE_T, template<typename> class CONTAINER>
void SubsetGraph<VALUE_T, CONTAINER>::addMoreEdgesToSetHierarchy()
{
	for (GroupedSetHierarchyEntry &entry0 : groupedSetHierarchy)
	{
		for (std::size_t j = 0; j != groupedSetHierarchy.size(); ++j)
		{
			if (entry0.groupIds.size() > groupedSetHierarchy[j].groupIds.size())
			{
				if (std::find(entry0.subsets.cbegin(), entry0.subsets.cend(), j) == entry0.subsets.cend() && std::includes(entry0.groupIds.cbegin(), entry0.groupIds.cend(), groupedSetHierarchy[j].groupIds.cbegin(), groupedSetHierarchy[j].groupIds.cend()))
				{
					entry0.subsets.push_back(j);
					++groupedSetHierarchy[j].supersetCount;
				}
			}
		}
	}
}

template<typename VALUE_T, template<typename> class CONTAINER>
void SubsetGraph<VALUE_T, CONTAINER>::removeRedundantEdgesFromSetHierarchy()
{
	for (auto iter = groupedSetHierarchy.rbegin(); iter != groupedSetHierarchy.rend(); ++iter)
	{
		if (!iter->setIds.empty())
		{
			std::vector<std::size_t> subsetsOfSubsets;
			for (const std::size_t &subset : iter->subsets)
				subsetsOfSubsets.insert(subsetsOfSubsets.end(), groupedSetHierarchy[subset].subsets.cbegin(), groupedSetHierarchy[subset].subsets.cend());
			iter->subsets.erase(std::remove_if(iter->subsets.begin(), iter->subsets.end(), [&subsetsOfSubsets = std::as_const(subsetsOfSubsets)](const std::size_t x) { return std::find(subsetsOfSubsets.cbegin(), subsetsOfSubsets.cend(), x) != subsetsOfSubsets.cend(); }), iter->subsets.end());
			iter->subsets.shrink_to_fit();
		}
	}
}

template<typename VALUE_T, template<typename> class CONTAINER>
void SubsetGraph<VALUE_T, CONTAINER>::sortSetHierarchy()
{
	std::vector<std::size_t> sortOrder;
	sortOrder.reserve(groupedSetHierarchy.size());
	std::vector<std::size_t> entriesToProcess;
	entriesToProcess.reserve(groupedSetHierarchy.size() * 2);
	for (std::size_t i = 0; i != groupedSetHierarchy.size(); ++i)
		entriesToProcess.push_back(groupedSetHierarchy.size() - 1 - i);
	std::vector<bool> processedEntries(groupedSetHierarchy.size(), false);
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
			for (const std::size_t &subset : groupedSetHierarchy[currentEntry].subsets)
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
	sortedSetHierarchy.reserve(groupedSetHierarchy.size());
	for (const std::size_t &index : sortOrder)
	{
		sortedSetHierarchy.push_back(std::move(groupedSetHierarchy[index]));
		for (std::size_t &subset : sortedSetHierarchy.back().subsets)
			subset = reverseSortOrder[subset];
	}
	groupedSetHierarchy = std::move(sortedSetHierarchy);
}

template<typename VALUE_T, template<typename> class CONTAINER>
typename SubsetGraph<VALUE_T, CONTAINER>::setHierarchy_t SubsetGraph<VALUE_T, CONTAINER>::ungroupSetHierarchy() const
{
	setHierarchy_t setHierarchy;
	for (const GroupedSetHierarchyEntry &groupedSetHierarchyEntry : groupedSetHierarchy)
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
typename SubsetGraph<VALUE_T, CONTAINER>::setHierarchy_t SubsetGraph<VALUE_T, CONTAINER>::makeSetHierarchy_(const sets_t &sets)
{
	const groupsMap_t groupsMap = createGroups(sets);
	const groupedSets_t groupedSets = groupSets(sets, groupsMap);
	makeInitialSetHierarchy(groupedSets);
	addMoreEdgesToSetHierarchy();
	trimSetHierarchy();
	removeRedundantEdgesFromSetHierarchy();
	sortSetHierarchy();
	return ungroupSetHierarchy();
}


#include <cstdint>
#include <set>

template class SubsetGraph<std::int8_t, std::vector>;
template class SubsetGraph<std::size_t, std::set>;
