#include "./Karnaugh_Solution.hh"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>

#include "PetricksMethod.hh"


void Karnaugh_Solution::OptimizedSolution::print(std::ostream &o, const names_t &functionNames) const
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
	
	o << "\nGate scores: NOTs = " << getNotCount() << ", ANDs = " << getAndCount() << ", ORs = " << getOrCount() << '\n';
}

Karnaugh_Solution::Karnaugh_Solution(const PrimeImplicants &primeImplicants, const Minterms &target) :
	primeImplicants(primeImplicants),
	target(target)
{
}

void Karnaugh_Solution::solve()
{
	solutions = PetricksMethod<Minterm, PrimeImplicant>::solve(std::move(target), std::move(primeImplicants));
}

void Karnaugh_Solution::prettyPrintSolution(const PrimeImplicants &solution)
{
	Minterms minterms;
	for (const auto &minterm : solution)
	{
		const auto x = minterm.findMinterms();
		minterms.insert(x.cbegin(), x.end());
	}
	std::cout << "best fit:\n";
	Karnaugh::prettyPrintTable(minterms);
}

Karnaugh_Solution Karnaugh_Solution::solve(const PrimeImplicants &primeImplicants, const Minterms &target)
{
	Karnaugh_Solution karnaugh_solver(primeImplicants, target);
	karnaugh_solver.solve();
	return karnaugh_solver;
}

typename Karnaugh_Solution::OptimizedSolution Karnaugh_Solution::optimizeSolutions(const std::vector<const PrimeImplicants*> &solutions)
{
	static constexpr auto wipSumsSort = [](const std::set<const void*> &x, const std::set<const void*> &y) { return x.size() != y.size() ? x.size() < y.size() : x < y; };
	std::map<PrimeImplicant, std::vector<const void*>> wipProducts;
	std::map<std::set<const void*>, std::vector<const void*>, decltype(wipSumsSort)> wipSums(wipSumsSort);
	std::vector<const void*> wipFinalSums;
	wipFinalSums.reserve(solutions.size());
	OptimizedSolution optimizedSolution;
	for (const PrimeImplicants *const solution : solutions)
	{
		std::set<const void*> wipSum;
		for (const auto &x : *solution)
		{
			optimizedSolution.negatedInputs |= x.getFalseBits();
			wipSum.insert(&wipProducts[x]);
		}
		wipFinalSums.push_back(&wipSums[std::move(wipSum)]);
	}
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
	for (auto iter = wipSums.rbegin(); iter != wipSums.rend(); ++iter)
	{
		//TODO This approach does not strictly guarantee to find the best solution. Petrick's method could be used here.
		std::set<const void*> remainingProducts = iter->first;
		for (auto jiter = std::next(iter); jiter != wipSums.rend(); ++jiter)
		{
			std::set<const void*> commonProducts;
			std::set_intersection(remainingProducts.cbegin(), remainingProducts.cend(), jiter->first.cbegin(), jiter->first.cend(), std::inserter(commonProducts, commonProducts.begin()));
			if (commonProducts.size() > 1)
			{
				for (const void *const commonProduct : commonProducts)
					remainingProducts.erase(commonProduct);
				const auto &commonSum = wipSums[commonProducts];
				if (&commonSum != &iter->second)
					iter->second.push_back(&commonSum);
			}
		}
	}
	optimizedSolution.products.reserve(wipProducts.size());
	for (const auto &wipProduct : wipProducts)
	{
		optimizedSolution.products.push_back({wipProduct.first, {}});
		for (auto wipProductRefIter = wipProduct.second.crbegin(); wipProductRefIter != wipProduct.second.crend(); ++wipProductRefIter)
		{
			std::size_t i = 0;
			for (const auto &wipProduct2 : wipProducts)
			{
				if (&wipProduct2.second == *wipProductRefIter)
				{
					optimizedSolution.products.back().second.push_back(i);
					break;
				}
				++i;
			}
		}
	}
	for (auto productIter = optimizedSolution.products.rbegin(); productIter != optimizedSolution.products.rend(); ++productIter)
		for (const std::size_t productRef : productIter->second)
			productIter->first -= optimizedSolution.products[productRef].first;
	for (const auto &wipSum : wipSums)
	{
		optimizedSolution.sums.emplace_back();
		optimizedSolution.sums.back().reserve(wipSum.first.size() + wipSum.second.size());
		for (const auto &wipProductRef : wipSum.first)
		{
			std::size_t i = 0;
			for (const auto &wipProduct : wipProducts)
			{
				if (&wipProduct.second == wipProductRef)
				{
					optimizedSolution.sums.back().push_back(i);
					break;
				}
				++i;
			}
		}
		for (auto wipSumRefIter = wipSum.second.crbegin(); wipSumRefIter != wipSum.second.crend(); ++wipSumRefIter)
		{
			std::size_t i = 0;
			for (const auto &wipSum2 : wipSums)
			{
				if (&wipSum2.second == *wipSumRefIter)
				{
					optimizedSolution.sums.back().push_back(i + optimizedSolution.products.size());
					break;
				}
				++i;
			}
		}
	}
	for (auto sumIter = optimizedSolution.sums.rbegin(); sumIter != optimizedSolution.sums.rend(); ++sumIter)
	{
		for (const std::size_t sumRef : *sumIter)
		{
			if (sumRef >= optimizedSolution.products.size())
			{
				const auto &referedSum = optimizedSolution.sums[sumRef - optimizedSolution.products.size()];
				sumIter->erase(std::remove_if(sumIter->begin(), sumIter->end(), [&referedSum](const std::size_t x){ return std::find(referedSum.cbegin(), referedSum.cend(), x) != referedSum.cend(); }), sumIter->end());
			}
		}
		std::sort(sumIter->begin(), sumIter->end());
	}
	optimizedSolution.finalSums.reserve(wipFinalSums.size());
	for (const void *wipFinalSum : wipFinalSums)
	{
		std::size_t i = 0;
		for (const auto &wipSum : wipSums)
		{
			if (&wipSum.second == wipFinalSum)
			{
				optimizedSolution.finalSums.push_back(i + optimizedSolution.products.size());
				break;
			}
			++i;
		}
	}
	return optimizedSolution;
}
