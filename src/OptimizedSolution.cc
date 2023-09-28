#include "./OptimizedSolution.hh"

#include <algorithm>
#include <iomanip>
#include <stack>

#include "OptimizationHasseDiagram.hh"


void OptimizedSolution::printNegatedInputs(std::ostream &o) const
{
	o << "Negated inputs:";
	bool first = true;
	for (Minterm i = 0; i != ::bits; ++i)
	{
		if ((negatedInputs & (1 << (::bits - i - 1))) != 0)
		{
			if (first)
			{
				first = false;
				o << ' ';
			}
			else
			{
				o << ", ";
			}
			o << ::inputNames[i];
		}
	}
	if (first)
		o << " <none>";
	o << '\n';
}

void OptimizedSolution::printProducts(std::ostream &o) const
{
	o << "Products:";
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (i == 0)
			o << ' ';
		else
			o << ",  ";
		o << '[' << i << "] = ";
		const auto &[primeImplicant, references] = products[i];
		bool first = primeImplicant == PrimeImplicant::all();
		if (!first || references.empty())
			primeImplicant.print(o, false);
		for (const auto &productRef : references)
		{
			if (first)
				first = false;
			else
				o << " && ";
			o << '[' << productRef << ']';
		}
	}
	if (products.empty())
		o << " <none>";
	o << '\n';
}

void OptimizedSolution::printSums(std::ostream &o) const
{
	o << "Sums:";
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		if (i == 0)
			o << ' ';
		else
			o << ",  ";
		o << '[' << i + products.size() << "] = ";
		bool first = true;
		for (const auto &productRef : sums[i])
		{
			if (first)
				first = false;
			else
				o << " || ";
			o << '[' << productRef << ']';
		}
	}
	if (sums.empty())
		o << " <none>";
	o << '\n';
}

void OptimizedSolution::printFinalSums(std::ostream &o, const strings_t &functionNames) const
{
	o << "Final sums:";
	for (std::size_t i = 0; i != finalSums.size(); ++i)
	{
		if (i == 0)
			o << ' ';
		else
			o << ", ";
		o << '"' << functionNames[i] << "\" = [" << finalSums[i] << ']';
	}
	if (finalSums.empty())
		o << " <none>";
	o << '\n';
}

void OptimizedSolution::printGateScores(std::ostream &o) const
{
	o << "Gate scores: NOTs = " << getNotCount() << ", ANDs = " << getAndCount() << ", ORs = " << getOrCount() << '\n';
}

void OptimizedSolution::initializeWips(const solutions_t &solutions, wipProducts_t &wipProducts, wipSums_t &wipSums, wipFinalSums_t &wipFinalSums)
{
	wipFinalSums.reserve(solutions.size());
	for (const PrimeImplicants *const solution : solutions)
	{
		std::set<ref_t> wipSum;
		for (const auto &x : *solution)
			wipSum.insert(&wipProducts[x]);
		wipFinalSums.push_back(&wipSums[std::move(wipSum)]);
	}
}

std::pair<OptimizedSolution::productsGraph_t, OptimizedSolution::endNodes_t> OptimizedSolution::buildGraph(const wipProducts_t &wipProducts)
{
	using HasseDiagram = OptimizationHasseDiagram<std::int8_t>;
	HasseDiagram hasseDiagram;
	for (const auto &x : wipProducts)
	{
		if (x.first.getBitCount() != 0)
		{
			OptimizationHasseDiagram<std::int8_t>::set_t set;
			for (const auto &bit : x.first.splitBits())
				set.push_back(bit.second ? -bit.first - 1 : bit.first);
			hasseDiagram.insert(set);
		}
	}
	HasseDiagram::setHierarchy_t setHierarchy = hasseDiagram.makeSetHierarchy();
	productsGraph_t productsGraph;
	productsGraph.reserve(setHierarchy.size());
	endNodes_t endNodes;
	std::size_t i = 0;
	for (auto &setHierarchyEntry : setHierarchy)
	{
		PrimeImplicant primeImplicant = PrimeImplicant::all();
		for (const auto &value : setHierarchyEntry.values)
			if (value >= 0)
				primeImplicant.setBit(value, false);
			else
				primeImplicant.setBit(-value - 1, true);
		productsGraph.emplace_back(primeImplicant, std::move(setHierarchyEntry.subsets));
		if (setHierarchyEntry.isOriginalSet)
			endNodes.insert(i);
		++i;
	}
	return {productsGraph, endNodes};
}

std::pair<OptimizedSolution::chosenSubsets_t, OptimizedSolution::usageCounts_t> OptimizedSolution::findBestSubsets(const productsGraph_t &graph, const endNodes_t &endNodes)
{
	std::vector<std::size_t> heuristics(graph.size(), 0);
	std::stack<std::size_t, std::vector<std::size_t>> nodesToProcess;
	for (const std::size_t &endNode : endNodes)
	{
		nodesToProcess.push(endNode);
		std::vector<bool> processedNodes(graph.size(), false);
		while (!nodesToProcess.empty())
		{
			const std::size_t nodeIndex = nodesToProcess.top();
			nodesToProcess.pop();
			if (processedNodes[nodeIndex])
				continue;
			processedNodes[nodeIndex] = true;
			++heuristics[nodeIndex];
			const auto &node = graph[nodeIndex];
			for (const std::size_t &subset : node.second)
				nodesToProcess.push(subset);
		}
	}
	{
		for (const std::size_t &endNode : endNodes)
			nodesToProcess.push(endNode);
		std::vector<bool> processedNodes(graph.size(), false);
		while (!nodesToProcess.empty())
		{
			const std::size_t nodeIndex = nodesToProcess.top();
			if (processedNodes[nodeIndex])
			{
				nodesToProcess.pop();
				continue;
			}
			const auto &node = graph[nodeIndex];
			bool subsetsProcessed = true;
			for (const std::size_t subset : node.second)
			{
				if (!processedNodes[subset])
				{
					nodesToProcess.push(subset);
					subsetsProcessed = false;
				}
				else
				{
					if (heuristics[subset] > heuristics[nodeIndex])
						heuristics[nodeIndex] = heuristics[subset];
				}
			}
			if (subsetsProcessed)
			{
				nodesToProcess.pop();
				processedNodes[nodeIndex] = true;
			}
		}
	}
	
	chosenSubsets_t chosenSubsets(graph.size());
	usageCounts_t usageCounts(graph.size());
	for (const std::size_t endNode : endNodes)
		++usageCounts[endNode];
	for (std::size_t nodeIndex = 0; nodeIndex != graph.size(); ++nodeIndex)
	{
		auto [remainingInputs, subnodes] = graph[nodeIndex];
		while (!subnodes.empty())
		{
			std::stable_sort(subnodes.begin(), subnodes.end(), [&graph = std::as_const(graph), &heuristics = std::as_const(heuristics)](const std::size_t x, const std::size_t y){ return heuristics[x] > heuristics[y] || (heuristics[x] == heuristics[y] && graph[x].first.getBitCount() > graph[y].first.getBitCount()); });
			chosenSubsets[nodeIndex].push_back(subnodes[0]);
			++usageCounts[subnodes[0]];
			remainingInputs -= graph[subnodes[0]].first;
			subnodes.erase(std::remove_if(subnodes.begin(), subnodes.end(), [&graph = std::as_const(graph), &remainingInputs = std::as_const(remainingInputs)](const std::size_t x){ return (remainingInputs & graph[x].first).getBitCount() < 2; }), subnodes.end());
		}
	}
	
	for (std::size_t nodeIndex = 0; nodeIndex != graph.size(); ++nodeIndex)
		if (usageCounts[nodeIndex] == 0)
			nodesToProcess.push(nodeIndex);
	while (!nodesToProcess.empty())
	{
		const std::size_t nodeIndex = nodesToProcess.top();
		nodesToProcess.pop();
		for (const std::size_t subnode : chosenSubsets[nodeIndex])
			if (--usageCounts[subnode] == 0)
				nodesToProcess.push(subnode);
	}
	
	for (std::size_t nodeIndex = 0; nodeIndex != graph.size(); ++nodeIndex)
	{
		if (usageCounts[nodeIndex] == 0)
			continue;
		for (std::size_t i = 0; i != chosenSubsets[nodeIndex].size();)
		{
			const std::size_t subset = chosenSubsets[nodeIndex][i];
			if (usageCounts[subset] == 1 && endNodes.find(subset) == endNodes.cend())
			{
				chosenSubsets[nodeIndex].erase(chosenSubsets[nodeIndex].begin() + i);
				usageCounts[subset] = 0;
				chosenSubsets[nodeIndex].insert(chosenSubsets[nodeIndex].end(), chosenSubsets[subset].cbegin(), chosenSubsets[subset].cend());
			}
			else
			{
				++i;
			}
		}
	}
	
	return {chosenSubsets, usageCounts};
}

void OptimizedSolution::putChosenSubsetsBackToWips(wipProducts_t &wipProducts, const productsGraph_t &graph, const chosenSubsets_t &chosenSubsets, const usageCounts_t &usageCounts)
{
	std::vector<ref_t> refsrefs(graph.size());
	for (std::size_t i = 0; i != graph.size(); ++i)
		if (usageCounts[i] != 0)
			refsrefs[i] = &wipProducts[graph[i].first];
	for (std::size_t i = 0; i != graph.size(); ++i)
	{
		if (usageCounts[i] != 0)
		{
			auto &refs = wipProducts[graph[i].first];
			refs.reserve(graph[i].second.size());
			for (const std::size_t &subset : chosenSubsets[i])
				refs.push_back(refsrefs[subset]);
		}
	}
}

void OptimizedSolution::extractCommonParts(wipProducts_t &wipProducts)
{
	const auto [graph, endNodes] = buildGraph(wipProducts);
	const auto [chosenSubsets, usageCounts] = findBestSubsets(graph, endNodes);
	putChosenSubsetsBackToWips(wipProducts, graph, chosenSubsets, usageCounts);
}

void OptimizedSolution::extractCommonParts(wipSums_t &wipSums)
{
	for (auto iter = wipSums.rbegin(); iter != wipSums.rend(); ++iter)
	{
		//TODO This approach does not strictly guarantee to find the best solution. Petrick's method could be used here.
		std::set<ref_t> remainingProducts = iter->first;
		for (auto jiter = std::next(iter); jiter != wipSums.rend(); ++jiter)
		{
			std::set<ref_t> commonProducts;
			std::set_intersection(remainingProducts.cbegin(), remainingProducts.cend(), jiter->first.cbegin(), jiter->first.cend(), std::inserter(commonProducts, commonProducts.begin()));
			if (commonProducts.size() > 1)
			{
				for (const ref_t commonProduct : commonProducts)
					remainingProducts.erase(commonProduct);
				const auto &commonSum = wipSums[commonProducts];
				if (&commonSum != &iter->second)
					iter->second.push_back(&commonSum);
			}
		}
	}
}

void OptimizedSolution::createNegatedInputs(const solutions_t &solutions)
{
	for (const PrimeImplicants *const solution : solutions)
		for (const auto &x : *solution)
			negatedInputs |= x.getFalseBits();
}

std::size_t OptimizedSolution::findWipProductRefIndex(const wipProducts_t &wipProducts, const ref_t wipProductRef)
{
	std::size_t i = 0;
	for (const auto &wipProduct : wipProducts)
	{
		if (&wipProduct.second == wipProductRef)
			return i;
		++i;
	}
	__builtin_unreachable();
}

std::size_t OptimizedSolution::findWipSumRefIndex(const wipSums_t &wipSums, const ref_t wipSumRef) const
{
	std::size_t i = 0;
	for (const auto &wipSum : wipSums)
	{
		if (&wipSum.second == wipSumRef)
			return i + products.size();
		++i;
	}
	__builtin_unreachable();
}

void OptimizedSolution::insertWipProducts(const wipProducts_t &wipProducts)
{
	products.reserve(wipProducts.size());
	for (const auto &wipProduct : wipProducts)
		products.push_back({wipProduct.first, {}});
	std::size_t i = 0;
	for (const auto &wipProduct : wipProducts)
	{
		auto &references = products[i++].second;
		for (auto wipProductRefIter = wipProduct.second.crbegin(); wipProductRefIter != wipProduct.second.crend(); ++wipProductRefIter)
			references.push_back(findWipProductRefIndex(wipProducts, *wipProductRefIter));
	}
}

void OptimizedSolution::insertWipSums(const wipProducts_t &wipProducts, const wipSums_t &wipSums)
{
	sums.reserve(wipSums.size());
	for (const auto &wipSum : wipSums)
	{
		sums.emplace_back();
		auto &sum = sums.back();
		sum.reserve(wipSum.first.size() + wipSum.second.size());
		for (const auto &wipProductRef : wipSum.first)
			sum.push_back(findWipProductRefIndex(wipProducts, wipProductRef));
		for (auto wipSumRefIter = wipSum.second.crbegin(); wipSumRefIter != wipSum.second.crend(); ++wipSumRefIter)
			sum.push_back(findWipSumRefIndex(wipSums, *wipSumRefIter));
	}
}

void OptimizedSolution::insertWipFinalSums(const wipSums_t &wipSums, const wipFinalSums_t &wipFinalSums)
{
	finalSums.reserve(wipFinalSums.size());
	for (ref_t wipFinalSum : wipFinalSums)
		finalSums.push_back(findWipSumRefIndex(wipSums, wipFinalSum));
}

void OptimizedSolution::cleanupProducts()
{
	for (auto productIter = products.rbegin(); productIter != products.rend(); ++productIter)
		for (const std::size_t productRef : productIter->second)
			productIter->first -= products[productRef].first;
}

void OptimizedSolution::cleanupSums()
{
	for (auto sumIter = sums.rbegin(); sumIter != sums.rend(); ++sumIter)
	{
		for (const std::size_t sumRef : *sumIter)
		{
			if (sumRef >= products.size())
			{
				const auto &referedSum = sums[sumRef - products.size()];
				sumIter->erase(std::remove_if(sumIter->begin(), sumIter->end(), [&referedSum](const std::size_t x){ return std::find(referedSum.cbegin(), referedSum.cend(), x) != referedSum.cend(); }), sumIter->end());
			}
		}
		std::sort(sumIter->begin(), sumIter->end());
	}
}

void OptimizedSolution::print(std::ostream &o, const strings_t &functionNames) const
{
	printNegatedInputs(o);
	printProducts(o);
	printSums(o);
	printFinalSums(o, functionNames);
	o << '\n';
	printGateScores(o);
}

OptimizedSolution OptimizedSolution::create(const solutions_t &solutions)
{
	wipProducts_t wipProducts;
	wipSums_t wipSums(wipSumsLess);
	wipFinalSums_t wipFinalSums;
	initializeWips(solutions, wipProducts, wipSums, wipFinalSums);
	
	extractCommonParts(wipProducts);
	extractCommonParts(wipSums);
	
	OptimizedSolution os;
	
	os.createNegatedInputs(solutions);
	os.insertWipProducts(wipProducts);
	os.insertWipSums(wipProducts, wipSums);
	os.insertWipFinalSums(wipSums, wipFinalSums);
	
	os.cleanupProducts();
	os.cleanupSums();
	
	return os;
}
