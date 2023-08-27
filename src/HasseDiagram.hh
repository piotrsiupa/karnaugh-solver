#pragma once

#include <cstddef>
#include <memory>
#include <set>
#include <utility>
#include <variant>
#include <vector>


class HasseDiagram
{
public:
	using value_t = std::size_t;
	using set_t = std::set<std::size_t>;
	using sets_t = std::vector<set_t>;
	
private:
	static constexpr value_t TOP_NODE = ~value_t(0) - 1, REFERENCES = ~value_t(0);
	
	struct Node;
	using NodeChild = std::variant<std::unique_ptr<Node>, std::vector<Node*>, std::monostate>;
	class NodeChildren : public std::vector<std::pair<value_t, NodeChild>>
	{
		using super = std::vector<std::pair<value_t, NodeChild>>;
	public:
		iterator find(const value_t &key) { for (iterator iter = begin(); iter != end(); ++iter) if (iter->first == key) return iter; return end(); }
		const_iterator find(const value_t &key) const { return const_cast<NodeChildren*>(this)->find(key); }
		void erase(const value_t &key) { super::erase(find(key)); }
	};
	struct Node
	{
		NodeChildren children;
		std::size_t value;
		Node *parent;
	};
	Node root{{}, 0, nullptr};
	
	static bool containsSubset(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, const Node &currentNode);
	
	Node& insert(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, Node &currentNode);
	void insertSideBranch(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, Node &currentNode, Node &result);
	
	void removeChildren(Node &node);
	static void removeTopNode(Node &topNode);
	static void removeSideBranch(std::vector<std::size_t>::const_reverse_iterator currentInSet, const std::vector<std::size_t>::const_reverse_iterator &endOfSet, Node &startPoint, const Node *const endNode);
	
	static void getSets(sets_t &sets, set_t &currentSet, const Node &currentNode);
	
public:
	bool insertRemovingSupersets(const set_t &set);
	
	sets_t getSets() const;
};
