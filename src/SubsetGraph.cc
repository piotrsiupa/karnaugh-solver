#include "./SubsetGraph.hh"

#include <algorithm>
#include <iterator>
#include <set>
#include <utility>


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
				std::ranges::set_difference(groups[foundGroupIndex], setCopy, std::back_inserter(groupRejects));
				if (!groupRejects.empty())
				{
					typename group_t::const_iterator currentRejectIt = groupRejects.cbegin(), endRejectIt = groupRejects.cend();
					const auto eraseBegin = std::remove_if(groups[foundGroupIndex].begin(), groups[foundGroupIndex].end(), [&currentRejectIt, endRejectIt](const VALUE_T x){ if (currentRejectIt == endRejectIt || x != *currentRejectIt) return false; ++currentRejectIt; return true; });
					groups[foundGroupIndex].erase(eraseBegin, groups[foundGroupIndex].end());
					groups.emplace_back(std::move(groupRejects));
					const std::size_t newGroupIndex = groups.size() - 1;
					for (const VALUE_T &value : groups.back())
						groupsMap.at(value) = newGroupIndex;
				}
				typename group_t::const_iterator currentValueIt = groups[foundGroupIndex].cbegin(), endValueIt = groups[foundGroupIndex].cend();
				const auto eraseBegin = std::remove_if(setCopy.begin(), setCopy.end(), [&currentValueIt, endValueIt](const VALUE_T x){ if (currentValueIt == endValueIt || x != *currentValueIt) return false; ++currentValueIt; return true; });
				setCopy.erase(eraseBegin, setCopy.end());
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
void SubsetGraph<VALUE_T, CONTAINER>::makeInitial(const groupedSets_t &groupedSets)
{
	for (std::size_t i = 0; i != groupedSets.size(); ++i)
		if (const auto foundGrouped = std::ranges::find(grouped, groupedSets[i], &GroupedEntry::groupIds); foundGrouped != grouped.end())
			foundGrouped->setIds.push_back(i);
		else
			grouped.push_back(GroupedEntry{groupedSets[i], {}, {i}, 0, true});
	
	for (std::size_t i = 0; i != grouped.size(); ++i)
	{
		const std::size_t sizeBeforeLoop = grouped.size();
		for (std::size_t j = i + 1; j != sizeBeforeLoop; ++j)
		{
			groupedSet_t partialSet;
			std::ranges::set_intersection(grouped[i].groupIds, grouped[j].groupIds, std::back_inserter(partialSet));
			if (partialSet.size() > 1 || (partialSet.size() == 1 && groups[partialSet[0]].size() > 1))
			{
				auto foundGrouped = std::ranges::find(grouped, partialSet, &GroupedEntry::groupIds);
				if (foundGrouped == grouped.end())
				{
					grouped.push_back(GroupedEntry{partialSet, {}, {}, 0, false});
					foundGrouped = std::ranges::next(grouped.begin(), grouped.size() - 1);
				}
				if (foundGrouped != std::ranges::next(grouped.begin(), i))
					foundGrouped->setIds.insert(foundGrouped->setIds.end(), grouped[i].setIds.cbegin(), grouped[i].setIds.cend());
				if (foundGrouped != std::ranges::next(grouped.begin(), j))
					foundGrouped->setIds.insert(foundGrouped->setIds.end(), grouped[j].setIds.cbegin(), grouped[j].setIds.cend());
			}
		}
	}
	
	for (auto &x : grouped)
	{
		std::ranges::sort(x.setIds);
		const auto eraseBegin = std::unique(x.setIds.begin(), x.setIds.end()), eraseEnd = x.setIds.end();
		x.setIds.erase(eraseBegin, eraseEnd);
	}
}

template<typename VALUE_T, template<typename> class CONTAINER>
void SubsetGraph<VALUE_T, CONTAINER>::trim()
{
	std::vector<std::size_t> offsets(grouped.size());
	for (GroupedEntry &entry : grouped)
	{
		for (std::size_t i = 0; i != entry.subsets.size();)
		{
			const std::size_t subsetIndex = entry.subsets[i];
			GroupedEntry &subset = grouped[subsetIndex];
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
	const auto eraseBegin = std::remove_if(grouped.begin(), grouped.end(), [](const GroupedEntry &entry){ return entry.setIds.empty(); });
	grouped.erase(eraseBegin, grouped.end());
	grouped.shrink_to_fit();
	
	std::size_t previousOffset = 0;
	for (std::size_t &offset : offsets)
	{
		offset += previousOffset;
		previousOffset = offset;
	}
	for (GroupedEntry &entry : grouped)
		for (std::size_t &subset : entry.subsets)
			subset -= offsets[subset];
}

template<typename VALUE_T, template<typename> class CONTAINER>
void SubsetGraph<VALUE_T, CONTAINER>::addMoreEdges()
{
	for (GroupedEntry &entry0 : grouped)
	{
		for (std::size_t j = 0; j != grouped.size(); ++j)
		{
			if (entry0.groupIds.size() > grouped[j].groupIds.size())
			{
				if (std::ranges::find(entry0.subsets, j) == entry0.subsets.cend() && std::ranges::includes(entry0.groupIds, grouped[j].groupIds))
				{
					entry0.subsets.push_back(j);
					++grouped[j].supersetCount;
				}
			}
		}
	}
}

template<typename VALUE_T, template<typename> class CONTAINER>
void SubsetGraph<VALUE_T, CONTAINER>::removeRedundantEdges()
{
	for (auto iter = grouped.rbegin(); iter != grouped.rend(); ++iter)
	{
		if (!iter->setIds.empty())
		{
			std::vector<std::size_t> subsetsOfSubsets;
			for (const std::size_t &subset : iter->subsets)
				subsetsOfSubsets.insert(subsetsOfSubsets.end(), grouped[subset].subsets.cbegin(), grouped[subset].subsets.cend());
			const auto eraseBegin = std::remove_if(iter->subsets.begin(), iter->subsets.end(), [&subsetsOfSubsets = std::as_const(subsetsOfSubsets)](const std::size_t x) { return std::ranges::find(subsetsOfSubsets, x) != subsetsOfSubsets.cend(); });
			iter->subsets.erase(eraseBegin, iter->subsets.end());
			iter->subsets.shrink_to_fit();
		}
	}
}

template<typename VALUE_T, template<typename> class CONTAINER>
void SubsetGraph<VALUE_T, CONTAINER>::sort()
{
	std::vector<std::size_t> sortOrder;
	sortOrder.reserve(grouped.size());
	std::vector<std::size_t> entriesToProcess;
	entriesToProcess.reserve(grouped.size() * 2);
	for (std::size_t i = 0; i != grouped.size(); ++i)
		entriesToProcess.push_back(grouped.size() - 1 - i);
	std::vector<bool> processedEntries(grouped.size(), false);
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
			for (const std::size_t &subset : grouped[currentEntry].subsets)
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
	
	grouped_t sorted;
	sorted.reserve(grouped.size());
	for (const std::size_t &index : sortOrder)
	{
		sorted.push_back(std::move(grouped[index]));
		for (std::size_t &subset : sorted.back().subsets)
			subset = reverseSortOrder[subset];
	}
	grouped = std::move(sorted);
}

template<typename VALUE_T, template<typename> class CONTAINER>
typename SubsetGraph<VALUE_T, CONTAINER>::setHierarchy_t SubsetGraph<VALUE_T, CONTAINER>::ungroup() const
{
	setHierarchy_t setHierarchy;
	for (const GroupedEntry &groupedEntry : grouped)
	{
		std::vector<value_t> ungroupedValues;
		for (const groupId_t groupId : groupedEntry.groupIds)
			ungroupedValues.insert(ungroupedValues.end(), groups.at(groupId).cbegin(), groups.at(groupId).cend());
		std::ranges::sort(ungroupedValues);
		setHierarchy.push_back({std::move(ungroupedValues), std::move(groupedEntry.subsets), std::move(groupedEntry.setIds), groupedEntry.supersetCount, groupedEntry.isOriginalSet});
	}
	return setHierarchy;
}

template<typename VALUE_T, template<typename> class CONTAINER>
typename SubsetGraph<VALUE_T, CONTAINER>::setHierarchy_t SubsetGraph<VALUE_T, CONTAINER>::make(const sets_t &sets)
{
	const groupsMap_t groupsMap = createGroups(sets);
	const groupedSets_t groupedSets = groupSets(sets, groupsMap);
	makeInitial(groupedSets);
	addMoreEdges();
	trim();
	removeRedundantEdges();
	sort();
	return ungroup();
}


#include <cstdint>
#include <set>

template class SubsetGraph<std::int8_t, std::vector>;
template class SubsetGraph<std::size_t, std::set>;
