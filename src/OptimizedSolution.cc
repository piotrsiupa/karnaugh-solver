#include "./OptimizedSolution.hh"

#include <algorithm>
#include <iomanip>

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

std::pair<OptimizedSolution::productsGraph_t, OptimizedSolution::possibleSubsets_t> OptimizedSolution::buildGraph(const wipProducts_t &wipProducts)
{
	using HasseDiagram = OptimizationHasseDiagram<std::int8_t>;
	HasseDiagram hasseDiagram;
	int i = 0;
	for (const auto &x : wipProducts)
	{
		if (x.first.getBitCount() != 0)
		{
			OptimizationHasseDiagram<std::int8_t>::set_t set;
			for (const auto &bit : x.first.splitBits())
				set.push_back(bit.second ? -bit.first - 1 : bit.first);
			hasseDiagram.insert(set, i++);
		}
	}
	HasseDiagram::setHierarchy_t setHierarchy = hasseDiagram.makeSetHierarchy();
	productsGraph_t productsGraph;
	productsGraph.reserve(setHierarchy.size());
	for (auto &setHierarchyEntry : setHierarchy)
	{
		PrimeImplicant primeImplicant = PrimeImplicant::all();
		for (const auto &value : setHierarchyEntry.values)
			if (value >= 0)
				primeImplicant.setBit(value, false);
			else
				primeImplicant.setBit(-value - 1, true);
		productsGraph.emplace_back(primeImplicant, std::move(setHierarchyEntry.subsets));
	}
	possibleSubsets_t possibleSubsets(wipProducts.size());
	for (std::size_t i = 0; i != setHierarchy.size(); ++i)
		for (const auto &setId : setHierarchy[i].setIds)
			possibleSubsets[setId].push_back(i);
	return {productsGraph, possibleSubsets};
}

bool OptimizedSolution::chooseNextSubsets(const possibleSubsets_t &possibleSubsets, chosenSubsets_t &chosenSubsets, usageCounts_t &usageCounts)
{
	try_again:
	for (std::size_t i = 0; i != chosenSubsets.size(); ++i)
	{
		if (chosenSubsets[i].empty())
		{
			if (possibleSubsets[i].empty())
				goto next;
			chosenSubsets[i].push_back(0);
			++usageCounts[possibleSubsets[i][0]];
		}
		else
		{
			chosenSubsets[i].push_back(chosenSubsets[i].back() + 1);
			if (chosenSubsets[i].back() == possibleSubsets[i].size())
			{
				chosenSubsets[i].pop_back();
				--usageCounts[possibleSubsets[i][chosenSubsets[i].back()]];
				chosenSubsets[i].pop_back();
				if (chosenSubsets[i].empty())
					goto next;
				--usageCounts[possibleSubsets[i][chosenSubsets[i].back()]];
				++chosenSubsets[i].back();
				++usageCounts[possibleSubsets[i][chosenSubsets[i].back()]];
			}
			else
			{
				++usageCounts[possibleSubsets[i][chosenSubsets[i].back()]];
			}
		}
		for (const std::size_t &usageCount : usageCounts)
			if (usageCount == 1)
				goto try_again;
		break;
		next:;
		if (i == chosenSubsets.size() - 1)
			return false;
	}
	return true;
}

OptimizedSolution::gateCount_t OptimizedSolution::countGates(const wipProducts_t &wipProducts, const productsGraph_t &graph, const possibleSubsets_t &possibleSubsets, const chosenSubsets_t &chosenSubsets, const usageCounts_t &usageCounts)
{
	gateCount_t gates = 0;
	int i = 0;
	for (const auto &product : wipProducts)
	{
		gates += chosenSubsets[i].size();
		PrimeImplicant reducedPrimeImplicant = product.first;
		for (const std::size_t &subset : chosenSubsets[i])
			reducedPrimeImplicant -= graph[possibleSubsets[i][subset]].first;
		gates += reducedPrimeImplicant.getBitCount();
		if (chosenSubsets[i].size() != 0 || reducedPrimeImplicant.getBitCount() != 0)
			--gates;
		++i;
	}
	for (std::size_t i = 0; i != graph.size(); ++i)
	{
		if (usageCounts[i] == 0)
			continue;
		gates += graph[i].second.size();
		PrimeImplicant reducedPrimeImplicant = graph[i].first;
		for (const std::size_t &subset : graph[i].second)
			reducedPrimeImplicant -= graph[subset].first;
		gates += reducedPrimeImplicant.getBitCount();
		--gates;
	}
	return gates;
}

std::pair<OptimizedSolution::chosenSubsets_t, OptimizedSolution::usageCounts_t> OptimizedSolution::findBestSubsets(const wipProducts_t &wipProducts, const productsGraph_t &graph, const possibleSubsets_t &possibleSubsets)
{
	chosenSubsets_t chosenSubsets(wipProducts.size()), bestChosenSubsets(wipProducts.size());
	usageCounts_t usageCounts(graph.size()), bestUsageCounts(graph.size());
	std::size_t bestGates = SIZE_MAX;
	while (true)
	{
		usageCounts_t candidateUsageCounts = usageCounts;
		std::vector<std::size_t> usageCountsToPropagate; //TODO this may also need brute-force approach?
		for (std::size_t i = 0; i != candidateUsageCounts.size(); ++i)
			if (candidateUsageCounts[i] != 0)
				usageCountsToPropagate.push_back(i);
		while (!usageCountsToPropagate.empty())
		{
			const std::size_t i = usageCountsToPropagate.back();
			usageCountsToPropagate.pop_back();
			for (const std::size_t &subset : graph[i].second)
				if (candidateUsageCounts[subset]++ == 0)
					usageCountsToPropagate.push_back(subset);
		}
		const gateCount_t gates = countGates(wipProducts, graph, possibleSubsets, chosenSubsets, usageCounts);
		if (gates < bestGates)
		{
			bestChosenSubsets = chosenSubsets;
			bestUsageCounts = std::move(candidateUsageCounts);
			bestGates = gates;
		}
		if (!chooseNextSubsets(possibleSubsets, chosenSubsets, usageCounts))
			break;
	}
	return {bestChosenSubsets, bestUsageCounts};
}

void OptimizedSolution::putChosenSubsetsBackToWips(wipProducts_t &wipProducts, const productsGraph_t &graph, const possibleSubsets_t &possibleSubsets, const chosenSubsets_t &chosenSubsets, const usageCounts_t &usageCounts)
{
	std::vector<PrimeImplicant> mainProducts;
	mainProducts.reserve(wipProducts.size());
	for (const auto &x : wipProducts)
		mainProducts.push_back(x.first);
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
			for (const std::size_t &subset : graph[i].second)
				refs.push_back(refsrefs[subset]);
		}
	}
	for (std::size_t i = 0; i != mainProducts.size(); ++i)
	{
		auto &refs = wipProducts[mainProducts[i]];
		refs.reserve(chosenSubsets[i].size());
		for (const std::size_t &subset : chosenSubsets[i])
		{
			const ref_t ref = refsrefs[possibleSubsets[i][subset]];
			if (&refs != ref)
				refs.push_back(ref);
		}
	}
}

void OptimizedSolution::extractCommonParts(wipProducts_t &wipProducts)
{
	const auto [graph, possibleSubsets] = buildGraph(wipProducts);
	const auto [chosenSubsets, usageCounts] = findBestSubsets(wipProducts, graph, possibleSubsets);
	putChosenSubsetsBackToWips(wipProducts, graph, possibleSubsets, chosenSubsets, usageCounts);
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
