#include "./HasseDiagram.hh"

#include <algorithm>


bool HasseDiagram::containsSubset(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, const Node &currentNode)
{
	for (; currentInSet != endOfSet; ++currentInSet)
		if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
			if (containsSubset(std::next(currentInSet), endOfSet, foundChild->getNode()))
				return true;
	return currentNode.children.find(TOP_NODE) != currentNode.children.cend();
}

HasseDiagram::Node& HasseDiagram::insert(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, Node &currentNode)
{
	++size;
	Node *nextNode;
	if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
		nextNode = &foundChild->getNode();
	else
		nextNode = &currentNode.children.emplace_back(*currentInSet, new Node{{}, *currentInSet, &currentNode}).getNode();
	const set_t::const_iterator nextInSet = std::next(currentInSet);
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

void HasseDiagram::insertSideBranch(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, Node &currentNode, Node &resultNode)
{
	Node *nextNode;
	if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
		nextNode = &foundChild->getNode();
	else
		nextNode = &currentNode.children.emplace_back(*currentInSet, new Node{{}, *currentInSet, &currentNode}).getNode();
	const set_t::const_iterator nextInSet = std::next(currentInSet);
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

void HasseDiagram::removeChildren(Node &node)
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

void HasseDiagram::removeTopNode(Node &topNode)
{
	--size;
	topNode.children.erase(TOP_NODE);
	std::vector<std::size_t> &reversedSet = workingVector;
	reversedSet.clear();
	Node *curr = &topNode;
	while (curr->parent != nullptr)
	{
		if (reversedSet.size() >= 2)
			removeSideBranch(std::next(reversedSet.crbegin()), reversedSet.crend(), *curr, &topNode);
		reversedSet.push_back(curr->value);
		Node *const parent = curr->parent;
		if (curr->children.empty())
			parent->children.erase(reversedSet.back());
		curr = parent;
	}
	if (reversedSet.size() >= 2)
		removeSideBranch(std::next(reversedSet.crbegin()), reversedSet.crend(), *curr, &topNode);
}

void HasseDiagram::removeSideBranch(std::vector<std::size_t>::const_reverse_iterator currentInSet, const std::vector<std::size_t>::const_reverse_iterator &endOfSet, Node &startPoint, const Node *const endNode)
{
	Node &currentNode = startPoint.children.find(*currentInSet)->getNode();
	const std::vector<std::size_t>::const_reverse_iterator nextInSet = std::next(currentInSet);
	if (nextInSet != endOfSet)
	{
		removeSideBranch(nextInSet, endOfSet, currentNode, endNode);
		removeSideBranch(nextInSet, endOfSet, startPoint, endNode);
	}
	else
	{
		std::vector<Node*> &sideEnds = currentNode.children.find(REFERENCES)->getReferences();
		sideEnds.erase(std::find(sideEnds.begin(), sideEnds.end(), endNode));
		if (sideEnds.empty())
			currentNode.children.erase(REFERENCES);
	}
	if (currentNode.children.empty())
		startPoint.children.erase(*currentInSet);
}

void HasseDiagram::getSets(sets_t &sets, const Node &currentNode) const
{
	if (currentNode.children.find(TOP_NODE) != currentNode.children.cend())
	{
		sets.emplace_back(workingVector);
	}
	else
	{
		for (const auto &child : currentNode.children)
		{
			if (child.getKey() != REFERENCES)
			{
				workingVector.push_back(child.getKey());
				getSets(sets, child.getNode());
				workingVector.pop_back();
			}
		}
	}
}

HasseDiagram::sets_t HasseDiagram::getSets() const
{
	workingVector.clear();
	sets_t sets;
	sets.reserve(size);
	getSets(sets, root);
	sets.shrink_to_fit();
	return sets;
}

bool HasseDiagram::insertRemovingSupersets(const set_t &set)
{
	if (containsSubset(set.cbegin(), set.cend(), root))
		return false;
	Node &added = insert(set.cbegin(), set.cend(), root);
	removeChildren(added);
	return true;
}
