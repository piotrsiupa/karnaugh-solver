#include "./HasseDiagram.hh"

#include <algorithm>


HasseDiagram::Node& HasseDiagram::findAnyTopNode(Node &startPoint)
{
	Node *curr = &startPoint;
	while (!curr->children.empty())
	{
		auto &firstChild = *curr->children.begin();
		if (firstChild.first == TOP_NODE)
			return *std::get<1>(firstChild.second).front();
		else
			curr = &std::get<Node>(firstChild.second);
	}
	return *curr;
}

bool HasseDiagram::containsSubset(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, const Node &currentNode)
{
	for (; currentInSet != endOfSet; ++currentInSet)
		if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
			if (containsSubset(std::next(currentInSet), endOfSet, std::get<Node>(foundChild->second)))
				return true;
	return currentNode.children.find(TOP_NODE) != currentNode.children.cend();
}

HasseDiagram::Node& HasseDiagram::insert(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, Node &currentNode)
{
	Node *nextNode;
	if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
		nextNode = &std::get<Node>(foundChild->second);
	else
		nextNode = &std::get<Node>(currentNode.children.emplace(*currentInSet, Node{{}, *currentInSet, &currentNode}).first->second);
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
		nextNode->children.emplace(TOP_NODE, std::monostate{});
	}
	return *resultNode;
}

void HasseDiagram::insertSideBranch(set_t::const_iterator currentInSet, const set_t::const_iterator &endOfSet, Node &currentNode, Node &resultNode)
{
	Node *nextNode;
	if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
		nextNode = &std::get<Node>(foundChild->second);
	else
		nextNode = &std::get<Node>(currentNode.children.emplace(*currentInSet, Node{{}, *currentInSet, &currentNode}).first->second);
	const set_t::const_iterator nextInSet = std::next(currentInSet);
	if (nextInSet != endOfSet)
	{
		insertSideBranch(nextInSet, endOfSet, *nextNode, resultNode);
		insertSideBranch(nextInSet, endOfSet, currentNode, resultNode);
	}
	else
	{
		if (const auto foundChild = nextNode->children.find(REFERENCES); foundChild != nextNode->children.end())
			std::get<1>(foundChild->second).emplace_back(&resultNode);
		else
			nextNode->children.emplace(REFERENCES, std::vector<Node*>{&resultNode});
	}
}

void HasseDiagram::removeChildren(Node &node)
{
	while (!node.children.empty() && node.children.find(TOP_NODE) == node.children.cend())
	{
		Node &topNode = findAnyTopNode(node);
		removeTopNode(topNode);
	}
}

void HasseDiagram::removeTopNode(Node &topNode)
{
	topNode.children.erase(TOP_NODE);
	std::vector<std::size_t> reversedSet;
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
	Node &currentNode = std::get<Node>(startPoint.children.at(*currentInSet));
	const std::vector<std::size_t>::const_reverse_iterator nextInSet = std::next(currentInSet);
	if (nextInSet != endOfSet)
	{
		removeSideBranch(nextInSet, endOfSet, currentNode, endNode);
		removeSideBranch(nextInSet, endOfSet, startPoint, endNode);
	}
	else
	{
		std::vector<Node*> sideEnds = std::get<1>(currentNode.children.at(REFERENCES));
		sideEnds.erase(std::find(sideEnds.begin(), sideEnds.end(), endNode));
		if (sideEnds.empty())
			currentNode.children.erase(REFERENCES);
	}
	if (currentNode.children.empty())
		startPoint.children.erase(*currentInSet);
}

void HasseDiagram::getSets(sets_t &sets, set_t &currentSet, const Node &currentNode)
{
	if (!currentNode.children.empty() && currentNode.children.cbegin()->first == TOP_NODE)
	{
		sets.emplace_back(currentSet);
	}
	else
	{
		for (const auto &child : currentNode.children)
		{
			if (child.first != REFERENCES)
			{
				currentSet.insert(child.first);
				getSets(sets, currentSet, std::get<Node>(child.second));
				currentSet.erase(child.first);
			}
		}
	}
}

bool HasseDiagram::insertRemovingSupersets(const set_t &set)
{
	if (containsSubset(set.cbegin(), set.cend(), root))
		return false;
	Node &added = insert(set.cbegin(), set.cend(), root);
	removeChildren(added);
	return true;
}

HasseDiagram::sets_t HasseDiagram::getSets() const
{
	sets_t sets;
	set_t set;
	getSets(sets, set, root);
	return sets;
}
