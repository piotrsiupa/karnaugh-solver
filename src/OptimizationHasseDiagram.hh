#pragma once

#include <cstddef>
#include <map>
#include <set>
#include <vector>


template<typename VALUE_T, template<typename> class CONTAINER>
class OptimizationHasseDiagram
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
	using groupedSet_t = std::set<groupId_t>;
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
	
	struct Node;
	using setIds_t = std::vector<setId_t>;
	class NodeChild
	{
		groupId_t key;
		Node *node;
		
	public:
		NodeChild(const groupId_t key, Node *const node) : key(key), node(node) {}
		NodeChild(const NodeChild &) = delete;
		NodeChild& operator=(const NodeChild &) = delete;
		NodeChild(NodeChild &&other) : key(other.key) { this->node = other.node; other.node = nullptr; }
		NodeChild& operator=(NodeChild &&other) { this->key = other.key; this->node = other.node; other.node = nullptr; return *this; }
		~NodeChild() { delete node; }
		
		groupId_t getKey() const { return key; }
		Node& getNode() { return *node; }
		const Node& getNode() const { return *node; }
	};
	class NodeChildren : public std::vector<NodeChild>
	{
		using super = std::vector<NodeChild>;
	public:
		typename super::iterator find(const groupId_t &key)
		{
			for (typename super::iterator iter = super::begin(); iter != super::end(); ++iter)
				if (iter->getKey() == key)
					return iter;
			return super::end();
		}
		typename super::const_iterator find(const groupId_t &key) const { return const_cast<NodeChildren*>(this)->find(key); }
	};
	struct Node
	{
		NodeChildren children;
		groupId_t value;
		setIds_t setIds;
		bool isOriginalSet;
	};
	Node root{{}, 0, {}, false};
	
	groupsMap_t createGroups(const sets_t &sets);
	bool contains(const groupedSet_t &set) const;
	void insert(typename groupedSet_t::const_iterator currentInSet, const typename groupedSet_t::const_iterator &endOfSet, Node &currentNode, const setId_t setId, const bool primaryBranch);
	void insertGrouped(const groupsMap_t &groupsMap, const sets_t &sets);
	mutable std::vector<groupId_t> currentGroupIds;
	
	void makeSetHierarchy(groupedSetHierarchy_t &setHierarchy, const Node &node, const std::size_t subset) const;
	static void trimSetHierarchy(groupedSetHierarchy_t &setHierarchy);
	static void addMoreEdgesToSetHierarchy(groupedSetHierarchy_t &setHierarchy);
	static void removeRedundantEdgesFromSetHierarchy(groupedSetHierarchy_t &setHierarchy);
	static void sortSetHierarchy(groupedSetHierarchy_t &setHierarchy);
	setHierarchy_t ungroupSetHierarchy(groupedSetHierarchy_t &groupedSetHierarchy) const;
	
	OptimizationHasseDiagram(const sets_t &sets);
	setHierarchy_t makeSetHierarchy() const;

public:
	static setHierarchy_t makeSetHierarchy(const sets_t &sets) { OptimizationHasseDiagram ohd(sets); return ohd.makeSetHierarchy(); }
};
