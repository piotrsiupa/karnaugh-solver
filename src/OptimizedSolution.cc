#include "./OptimizedSolution.hh"

#include <algorithm>
#include <iomanip>
#include <iostream>


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
		const auto &product = products[i];
		bool first = product.first == PrimeImplicant::all();
		if (!first || product.second.empty())
			product.first.print(o, false);
		for (const auto &productRef : product.second)
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

void OptimizedSolution::printFinalSums(std::ostream &o, const names_t &functionNames) const
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

void OptimizedSolution::extractCommonParts(wipProducts_t &wipProducts)
{
	for (auto iter = wipProducts.rbegin(); iter != wipProducts.rend(); ++iter)
	{
		//TODO This approach does not strictly guarantee to find the best solution. Petrick's method could be used here.
		PrimeImplicant remainingInputs = iter->first;
		for (auto jiter = std::next(iter); jiter != wipProducts.rend(); ++jiter)
		{
			const PrimeImplicant commonInputs = remainingInputs & jiter->first;
			if (commonInputs.getBitCount() > 1)
			{
				remainingInputs -= commonInputs;
				const auto &commonProduct = wipProducts[commonInputs];
				if (&commonProduct != &iter->second)
					iter->second.push_back(&commonProduct);
			}
		}
	}
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
	{
		products.push_back({wipProduct.first, {}});
		auto &product = products.back();
		for (auto wipProductRefIter = wipProduct.second.crbegin(); wipProductRefIter != wipProduct.second.crend(); ++wipProductRefIter)
			product.second.push_back(findWipProductRefIndex(wipProducts, *wipProductRefIter));
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

void OptimizedSolution::print(std::ostream &o, const names_t &functionNames) const
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
