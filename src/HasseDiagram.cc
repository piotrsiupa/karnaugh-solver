#include "./HasseDiagram.hh"

#include <algorithm>


template<typename VALUE_T>
bool HasseDiagram<VALUE_T>::containsSubset(typename set_t::const_iterator currentInSet, const typename set_t::const_iterator &endOfSet, const Node &currentNode)
{
	for (; currentInSet != endOfSet; ++currentInSet)
		if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
			if (containsSubset(std::ranges::next(currentInSet), endOfSet, foundChild->getNode()))
				return true;
	return currentNode.children.find(TOP_NODE) != currentNode.children.cend();
}

template<typename VALUE_T>
typename HasseDiagram<VALUE_T>::Node& HasseDiagram<VALUE_T>::insert(typename set_t::const_iterator currentInSet, const typename set_t::const_iterator &endOfSet, Node &currentNode)
{
	++count;
	Node *nextNode;
	if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
		nextNode = &foundChild->getNode();
	else
		nextNode = &currentNode.children.emplace_back(*currentInSet, new Node{{}, *currentInSet, &currentNode}).getNode();
	const typename set_t::const_iterator nextInSet = std::ranges::next(currentInSet);
	Node *resultNode;
	if (nextInSet != endOfSet)
	{
		resultNode = &insert(nextInSet, endOfSet, *nextNode);
		insertSideBranch(nextInSet, endOfSet, currentNode, *resultNode);
	}
	else
	{
		resultNode = nextNode;
		nextNode->children.emplace_back();
	}
	return *resultNode;
}

template<typename VALUE_T>
void HasseDiagram<VALUE_T>::insertSideBranch(typename set_t::const_iterator currentInSet, const typename set_t::const_iterator &endOfSet, Node &currentNode, Node &resultNode)
{
	Node *nextNode;
	if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
		nextNode = &foundChild->getNode();
	else
		nextNode = &currentNode.children.emplace_back(*currentInSet, new Node{{}, *currentInSet, &currentNode}).getNode();
	const typename set_t::const_iterator nextInSet = std::ranges::next(currentInSet);
	if (nextInSet != endOfSet)
	{
		insertSideBranch(nextInSet, endOfSet, *nextNode, resultNode);
		insertSideBranch(nextInSet, endOfSet, currentNode, resultNode);
	}
	else
	{
		if (const auto foundChild = nextNode->children.find(REFERENCES); foundChild != nextNode->children.end())
			foundChild->getReferences().emplace_back(&resultNode);
		else
			nextNode->children.emplace_back(std::vector<Node*>{&resultNode});
	}
}

template<typename VALUE_T>
void HasseDiagram<VALUE_T>::removeChildren(Node &node)
{
	while (true)
	{
		Node *curr = &node;
		while (true)
		{
			auto child = curr->children.begin();
			fuck_go_back:
			if (child == curr->children.end())
				return;
			switch (child->getKey())
			{
			case TOP_NODE:
				if (curr == &node)
				{
					++child;
					goto fuck_go_back;
				}
				goto end_loop;
			case REFERENCES:
				curr = child->getReferences().front();
				goto end_loop;
			default:
				curr = &child->getNode();
			}
		}
		end_loop:
		removeTopNode(*curr);
	}
}

template<typename VALUE_T>
void HasseDiagram<VALUE_T>::removeTopNode(Node &topNode)
{
	--count;
	topNode.children.erase(TOP_NODE);
	std::vector<value_t> &valuesBackwards = workingVector;
	valuesBackwards.clear();
	Node *curr = &topNode;
	while (curr->parent != nullptr)
	{
		if (valuesBackwards.size() >= 2)
			removeSideBranch(std::ranges::next(valuesBackwards.crbegin()), valuesBackwards.crend(), *curr, &topNode);
		valuesBackwards.push_back(curr->value);
		Node *const parent = curr->parent;
		if (curr->children.empty())
			parent->children.erase(valuesBackwards.back());
		curr = parent;
	}
	if (valuesBackwards.size() >= 2)
		removeSideBranch(std::ranges::next(valuesBackwards.crbegin()), valuesBackwards.crend(), *curr, &topNode);
}

template<typename VALUE_T>
void HasseDiagram<VALUE_T>::removeSideBranch(typename std::vector<value_t>::const_reverse_iterator currentValue, const typename std::vector<value_t>::const_reverse_iterator &endOfValues, Node &startPoint, const Node *const endNode)
{
	Node &currentNode = startPoint.children.find(*currentValue)->getNode();
	const typename std::vector<value_t>::const_reverse_iterator nextValue = std::ranges::next(currentValue);
	if (nextValue != endOfValues)
	{
		removeSideBranch(nextValue, endOfValues, currentNode, endNode);
		removeSideBranch(nextValue, endOfValues, startPoint, endNode);
	}
	else
	{
		std::vector<Node*> &references = currentNode.children.find(REFERENCES)->getReferences();
		references.erase(std::ranges::find(references, endNode));
		if (references.empty())
			currentNode.children.erase(REFERENCES);
	}
	if (currentNode.children.empty())
		startPoint.children.erase(*currentValue);
}

template<typename VALUE_T>
void HasseDiagram<VALUE_T>::getSets(sets_t &sets, const Node &currentNode) const
{
	set_t &currentSet = workingVector;
	if (currentNode.children.find(TOP_NODE) != currentNode.children.cend())
	{
		sets.emplace_back(currentSet);
	}
	else
	{
		for (const auto &child : currentNode.children)
		{
			if (child.getKey() != REFERENCES)
			{
				currentSet.push_back(child.getKey());
				getSets(sets, child.getNode());
				currentSet.pop_back();
			}
		}
	}
}

template<typename VALUE_T>
typename HasseDiagram<VALUE_T>::sets_t HasseDiagram<VALUE_T>::getSets() const
{
	workingVector.clear();
	sets_t sets;
	sets.reserve(count);
	getSets(sets, root);
	sets.shrink_to_fit();
	return sets;
}

template<typename VALUE_T>
bool HasseDiagram<VALUE_T>::insertRemovingSupersets(const set_t &set)
{
	if (containsSubset(set.cbegin(), set.cend(), root))
		return false;
	Node &added = insert(set.cbegin(), set.cend(), root);
	removeChildren(added);
	return true;
}


#include <cstdint>

template class HasseDiagram<std::uint8_t>;
template class HasseDiagram<std::uint16_t>;
template class HasseDiagram<std::uint32_t>;
template class HasseDiagram<std::uint64_t>;
