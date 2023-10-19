#pragma once

#include <cstddef>
#include <vector>


template<typename VALUE_T>
class HasseDiagram
{
public:
	using value_t = VALUE_T;
	static constexpr value_t MAX_VALUE = ~value_t(0) - 2;
	using set_t = std::vector<value_t>;
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
		
		inline void moveValue(NodeChild &&other);
		inline void deconstructValue();
		
	public:
		NodeChild(const value_t key, Node *const node) : key(key), node(node) {}
		NodeChild() : key(TOP_NODE) {}
		NodeChild(std::vector<Node*> &&references) : key(REFERENCES) { new(&this->references) references_t(std::move(references)); }
		NodeChild(const NodeChild &) = delete;
		NodeChild& operator=(const NodeChild &) = delete;
		inline NodeChild(NodeChild &&other);
		inline NodeChild& operator=(NodeChild &&other);
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
		typename super::iterator find(const value_t &key) { for (typename super::iterator iter = super::begin(); iter != super::end(); ++iter) if (iter->getKey() == key) return iter; return super::end(); }
		typename super::const_iterator find(const value_t &key) const { return const_cast<NodeChildren*>(this)->find(key); }
		void erase(const value_t &key) { super::erase(find(key)); }
	};
	struct Node
	{
		NodeChildren children;
		value_t value;
		Node *parent;
	};
	Node root{{}, 0, nullptr};
	
	std::size_t size = 0;
	
	mutable std::vector<value_t> workingVector;
	
	static bool containsSubset(typename set_t::const_iterator currentInSet, const typename set_t::const_iterator &endOfSet, const Node &currentNode);
	
	Node& insert(typename set_t::const_iterator currentInSet, const typename set_t::const_iterator &endOfSet, Node &currentNode);
	void insertSideBranch(typename set_t::const_iterator currentInSet, const typename set_t::const_iterator &endOfSet, Node &currentNode, Node &result);
	
	void removeChildren(Node &node);
	void removeTopNode(Node &topNode);
	static void removeSideBranch(typename std::vector<value_t>::const_reverse_iterator currentInSet, const typename std::vector<value_t>::const_reverse_iterator &endOfSet, Node &startPoint, const Node *const endNode);
	
	void getSets(sets_t &sets, const Node &currentNode) const;
	
public:
	std::size_t getSize() const { return size; }
	sets_t getSets() const;
	
	bool insertRemovingSupersets(const set_t &set);
};


template<typename VALUE_T>
void HasseDiagram<VALUE_T>::NodeChild::moveValue(NodeChild &&other)
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

template<typename VALUE_T>
void HasseDiagram<VALUE_T>::NodeChild::deconstructValue()
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

template<typename VALUE_T>
HasseDiagram<VALUE_T>::NodeChild::NodeChild(NodeChild &&other) :
	key(other.key)
{
	moveValue(std::move(other));
	other.key = TOP_NODE;
}

template<typename VALUE_T>
typename HasseDiagram<VALUE_T>::NodeChild& HasseDiagram<VALUE_T>::NodeChild::operator=(NodeChild &&other)
{
	deconstructValue();
	this->key = other.key;
	moveValue(std::move(other));
	other.key = TOP_NODE;
	return *this;
}
