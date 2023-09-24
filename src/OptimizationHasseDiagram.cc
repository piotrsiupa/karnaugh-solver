#include "./OptimizationHasseDiagram.hh"

#include <algorithm>
#include <cstdint>


template<typename VALUE_T>
void OptimizationHasseDiagram<VALUE_T>::insert(typename set_t::const_iterator currentInSet, const typename set_t::const_iterator &endOfSet, Node &currentNode, const setId_t setId)
{
	Node *nextNode;
	if (const auto foundChild = currentNode.children.find(*currentInSet); foundChild != currentNode.children.end())
	{
		nextNode = &foundChild->getNode();
		if (std::find(nextNode->setIds.cbegin(), nextNode->setIds.cend(), setId) == nextNode->setIds.cend())
			nextNode->setIds.emplace_back(setId);
	}
	else
	{
		nextNode = &currentNode.children.emplace_back(*currentInSet, new Node{{}, *currentInSet, {setId}}).getNode();
	}
	const typename set_t::const_iterator nextInSet = std::next(currentInSet);
	if (nextInSet != endOfSet)
	{
		insert(nextInSet, endOfSet, *nextNode, setId);
		insert(nextInSet, endOfSet, currentNode, setId);
	}
}

template<typename VALUE_T>
void OptimizationHasseDiagram<VALUE_T>::makeSetHierarchy(setHierarchy_t &setHierarchy, const Node &node, const std::size_t subset) const
{
	if (node.setIds.size() == 1)
		return;
	currentValues.push_back(node.value);
	if (currentValues.size() == 1)
	{
		for (const NodeChild &child : node.children)
			makeSetHierarchy(setHierarchy, child.getNode(), SIZE_MAX);
	}
	else
	{
		const std::size_t addedSet = setHierarchy.size();
		if (subset == SIZE_MAX)
		{
			setHierarchy.push_back({currentValues, {}, node.setIds, 0});
		}
		else
		{
			setHierarchy.push_back({currentValues, {subset}, node.setIds, 0});
			++setHierarchy[subset].supersetCount;
		}
		for (const NodeChild &child : node.children)
			makeSetHierarchy(setHierarchy, child.getNode(), addedSet);
	}
	currentValues.pop_back();
}

template<typename VALUE_T>
void OptimizationHasseDiagram<VALUE_T>::trimSetHierarchy(setHierarchy_t &setHierarchy)
{
	std::vector<std::size_t> offsets(setHierarchy.size());
	for (SetHierarchyEntry &entry : setHierarchy)
	{
		if (!entry.subsets.empty())
		{
			SetHierarchyEntry &subset = setHierarchy[entry.subsets[0]];
			if (subset.supersetCount == 1)
			{
				offsets[entry.subsets[0]] = 1;
				entry.subsets = std::move(subset.subsets);
				subset.setIds.clear();
			}
		}
	}
	std::size_t previousOffset = 0;
	for (std::size_t &offset : offsets)
	{
		offset += previousOffset;
		previousOffset = offset;
	}
	setHierarchy.erase(std::remove_if(setHierarchy.begin(), setHierarchy.end(), [](const SetHierarchyEntry &entry){ return entry.setIds.empty(); }), setHierarchy.end());
	setHierarchy.shrink_to_fit();
	for (SetHierarchyEntry &entry : setHierarchy)
		for (std::size_t &subset : entry.subsets)
			subset -= offsets[subset];
}

template<typename VALUE_T>
void OptimizationHasseDiagram<VALUE_T>::addMoreEdgesToSetHierarchy(setHierarchy_t &setHierarchy)
{
	for (SetHierarchyEntry &entry0 : setHierarchy)
		if (!entry0.setIds.empty())
			for (std::size_t j = 0; j != setHierarchy.size(); ++j)
				if (!setHierarchy[j].setIds.empty())
					if (entry0.values.size() > setHierarchy[j].values.size())
						if (std::find(entry0.subsets.cbegin(), entry0.subsets.cend(), j) == entry0.subsets.cend() && std::includes(entry0.values.cbegin(), entry0.values.cend(), setHierarchy[j].values.cbegin(), setHierarchy[j].values.cend()))
							entry0.subsets.push_back(j);
}

template<typename VALUE_T>
void OptimizationHasseDiagram<VALUE_T>::removeRedundantEdgesFromSetHierarchy(setHierarchy_t &setHierarchy)
{
	for (auto iter = setHierarchy.rbegin(); iter != setHierarchy.rend(); ++iter)
	{
		if (!iter->setIds.empty())
		{
			std::vector<std::size_t> subsetsOfSubsets;
			for (const std::size_t &subset : iter->subsets)
				subsetsOfSubsets.insert(subsetsOfSubsets.end(), setHierarchy[subset].subsets.cbegin(), setHierarchy[subset].subsets.cend());
			iter->subsets.erase(std::remove_if(iter->subsets.begin(), iter->subsets.end(), [&subsetsOfSubsets = std::as_const(subsetsOfSubsets)](const std::size_t x) { return std::find(subsetsOfSubsets.cbegin(), subsetsOfSubsets.cend(), x) != subsetsOfSubsets.cend(); }), iter->subsets.end());
			iter->subsets.shrink_to_fit();
		}
	}
}

template<typename VALUE_T>
typename OptimizationHasseDiagram<VALUE_T>::setHierarchy_t OptimizationHasseDiagram<VALUE_T>::makeSetHierarchy() const
{
	currentValues.clear();
	setHierarchy_t setHierarchy;
	for (const NodeChild &child : root.children)
		makeSetHierarchy(setHierarchy, child.getNode(), SIZE_MAX);
	trimSetHierarchy(setHierarchy);
	addMoreEdgesToSetHierarchy(setHierarchy);
	removeRedundantEdgesFromSetHierarchy(setHierarchy);
	return setHierarchy;
}


template class OptimizationHasseDiagram<std::int8_t>;
