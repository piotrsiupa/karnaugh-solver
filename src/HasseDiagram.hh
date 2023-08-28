#pragma once

#include <cstddef>
#include <set>
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
	using references_t = std::vector<Node*>;
	class NodeChild
	{
		value_t key = TOP_NODE;
		union
		{
			Node *node;
			references_t references;
		};
		
		void moveValue(NodeChild &&other)
		{
			switch (other.key)
			{
			case TOP_NODE:
				break;
			case REFERENCES:
				new (&references) references_t(std::move(other.references));
				other.references.~references_t();
				break;
			default:
				node = other.node;
			}
		}
		void deconstructValue()
		{
			switch (this->key)
			{
			case TOP_NODE:
				break;
			case REFERENCES:
				references.~references_t();
				break;
			default:
				delete node;
			}
		}
		
	public:
		NodeChild(const value_t key, Node *const node) : key(key), node(node) {}
		NodeChild() : key(TOP_NODE) {}
		NodeChild(std::vector<Node*> &&references) : key(REFERENCES) { new(&this->references) references_t(std::move(references)); }
		NodeChild(const NodeChild &) = delete;
		NodeChild& operator=(const NodeChild &) = delete;
		NodeChild(NodeChild &&other) :
			key(other.key)
		{
			moveValue(std::move(other));
			other.key = TOP_NODE;
		}
		NodeChild& operator=(NodeChild &&other)
		{
			deconstructValue();
			this->key = other.key;
			moveValue(std::move(other));
			other.key = TOP_NODE;
			return *this;
		}
		~NodeChild() { deconstructValue(); }
		
		value_t getKey() const { return key; }
		Node& getNode() { return *node; }
		const Node& getNode() const { return *node; }
		references_t& getReferences() { return references; }
		const references_t& getReferences() const { return references; }
	};
	class NodeChildren : public std::vector<NodeChild>
	{
		using super = std::vector<NodeChild>;
	public:
		iterator find(const value_t &key) { for (iterator iter = begin(); iter != end(); ++iter) if (iter->getKey() == key) return iter; return end(); }
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
	
	std::size_t size = 0;
	
	mutable std::vector<std::size_t> workingVector;
	
	static bool containsSubset(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, const Node &currentNode);
	
	Node& insert(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, Node &currentNode);
	void insertSideBranch(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, Node &currentNode, Node &result);
	
	void removeChildren(Node &node);
	void removeTopNode(Node &topNode);
	static void removeSideBranch(std::vector<std::size_t>::const_reverse_iterator currentInSet, const std::vector<std::size_t>::const_reverse_iterator &endOfSet, Node &startPoint, const Node *const endNode);
	
	void getSets(sets_t &sets, const Node &currentNode) const;
	
public:
	std::size_t getSize() const { return size; }
	sets_t getSets() const;
	
	bool insertRemovingSupersets(const set_t &set);
};
