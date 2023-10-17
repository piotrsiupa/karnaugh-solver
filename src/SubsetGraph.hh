#pragma once

#include <cstddef>
#include <map>
#include <vector>


template<typename VALUE_T, template<typename> class CONTAINER>
class SubsetGraph
{
public:
	using value_t = VALUE_T;
	using setId_t = std::size_t;
	static constexpr value_t MAX_VALUE = ~value_t(0);
	using set_t = CONTAINER<value_t>;
	using sets_t = std::vector<set_t>;
	struct SetHierarchyEntry
	{
		std::vector<value_t> values;
		std::vector<std::size_t> subsets;
		std::vector<setId_t> setIds;
		std::size_t supersetCount;
		bool isOriginalSet;
	};
	using setHierarchy_t = std::vector<SetHierarchyEntry>;
	
private:
	using group_t = std::vector<VALUE_T>;
	using groupId_t = std::size_t;
	using groupsMap_t = std::map<VALUE_T, std::size_t>;
	using groupedSet_t = std::vector<groupId_t>;
	using groupedSets_t = std::vector<groupedSet_t>;
	std::vector<group_t> groups;
	struct GroupedSetHierarchyEntry
	{
		std::vector<groupId_t> groupIds;
		std::vector<std::size_t> subsets;
		std::vector<setId_t> setIds;
		std::size_t supersetCount;
		bool isOriginalSet;
	};
	using groupedSetHierarchy_t = std::vector<GroupedSetHierarchyEntry>;
	
	groupsMap_t createGroups(const sets_t &sets);
	groupedSets_t groupSets(const sets_t &sets, const groupsMap_t &groupsMap) const;
	groupedSetHierarchy_t makeInitialSetHierarchy(const groupedSets_t &groupedSets) const;
	static void trimSetHierarchy(groupedSetHierarchy_t &setHierarchy);
	static void addMoreEdgesToSetHierarchy(groupedSetHierarchy_t &setHierarchy);
	static void removeRedundantEdgesFromSetHierarchy(groupedSetHierarchy_t &setHierarchy);
	static void sortSetHierarchy(groupedSetHierarchy_t &setHierarchy);
	setHierarchy_t ungroupSetHierarchy(groupedSetHierarchy_t &groupedSetHierarchy) const;
	
	setHierarchy_t _makeSetHierarchy(const sets_t &sets);

public:
	static setHierarchy_t makeSetHierarchy(const sets_t &sets) { return SubsetGraph()._makeSetHierarchy(sets); }
};
