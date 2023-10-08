#pragma once

#include <cstddef>
#include <vector>


template<typename VALUE_T, template<typename> class CONTAINER>
class OptimizationHasseDiagram
{
public:
	using value_t = VALUE_T;
	using setId_t = int;
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
	std::size_t currentSetId = 0;
	struct Node;
	using setIds_t = std::vector<setId_t>;
	class NodeChild
	{
		value_t key;
		Node *node;
		
	public:
		NodeChild(const value_t key, Node *const node) : key(key), node(node) {}
		NodeChild(const NodeChild &) = delete;
		NodeChild& operator=(const NodeChild &) = delete;
		NodeChild(NodeChild &&other) : key(other.key) { this->node = other.node; other.node = nullptr; }
		NodeChild& operator=(NodeChild &&other) { this->key = other.key; this->node = other.node; other.node = nullptr; return *this; }
		~NodeChild() { delete node; }
		
		value_t getKey() const { return key; }
		Node& getNode() { return *node; }
		const Node& getNode() const { return *node; }
	};
	class NodeChildren : public std::vector<NodeChild>
	{
		using super = std::vector<NodeChild>;
	public:
		typename super::iterator find(const value_t &key)
		{
			for (typename super::iterator iter = super::begin(); iter != super::end(); ++iter)
				if (iter->getKey() == key)
					return iter;
			return super::end();
		}
		typename super::const_iterator find(const value_t &key) const { return const_cast<NodeChildren*>(this)->find(key); }
	};
	struct Node
	{
		NodeChildren children;
		value_t value;
		setIds_t setIds;
		bool isOriginalSet;
	};
	Node root{{}, 0, {}, false};
	
	bool contains(const set_t &set) const;
	void insert(typename set_t::const_iterator currentInSet, const typename set_t::const_iterator &endOfSet, Node &currentNode, const setId_t setId, const bool primaryBranch);
	mutable std::vector<value_t> currentValues;
	
	void makeSetHierarchy(setHierarchy_t &setHierarchy, const Node &node, const std::size_t subset) const;
	static void trimSetHierarchy(setHierarchy_t &setHierarchy);
	static void addMoreEdgesToSetHierarchy(setHierarchy_t &setHierarchy);
	static void removeRedundantEdgesFromSetHierarchy(setHierarchy_t &setHierarchy);
	static void sortSetHierarchy(setHierarchy_t &setHierarchy);
	
public:
	void insert(const set_t &set) { if (!contains(set)) insert(set.cbegin(), set.cend(), root, currentSetId++, true); }
	
	setHierarchy_t makeSetHierarchy() const;
};
