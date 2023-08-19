#include "./Karnaugh_Solution.hh"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>


void Karnaugh_Solution::OptimizedSolution::print(const bits_t bits, std::ostream &o, const names_t &inputNames, const names_t &functionNames) const
{
	o << "Negated inputs:";
	bool first = true;
	for (number_t i = 0; i != bits; ++i)
	{
		if ((negatedInputs & (1 << (bits - i - 1))) != 0)
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
			o << inputNames[i];
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
			product.first.print(o, bits, inputNames, false);
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

Karnaugh_Solution::Karnaugh_Solution(const minterms_t &minterms, const numbers_t &target) :
	minterms(minterms),
	target(target)
{
}

Karnaugh_Solution::minterms_t Karnaugh_Solution::removeEssentials()
{
	minterms_t essentials;
	for (numbers_t::const_iterator iter = target.cbegin(); iter != target.cend();)
	{
		std::uint_fast8_t count = 0;
		std::size_t matchingIndex;
		for (std::size_t j = 0; j != minterms.size(); ++j)
		{
			if (minterms[j].covers(*iter))
			{
				if (++count == 2)
					break;
				matchingIndex = j;
			}
		}
		bool removedIter = false;
		if (count == 1)
		{
			essentials.push_back(minterms[matchingIndex]);
			for (numbers_t::iterator jiter = target.begin(); jiter != target.end();)
			{
				if (minterms[matchingIndex].covers(*jiter))
				{
					const bool removingIter = iter == jiter;
					removedIter |= removingIter;
					jiter = target.erase(jiter);
					if (removingIter)
						iter = jiter;
				}
				else
				{
					++jiter;
				}
			}
			minterms.erase(minterms.begin() + matchingIndex);
		}
		if (!removedIter)
			++iter;
	}
	return essentials;
}

void Karnaugh_Solution::solve()
{
	const minterms_t essentials = removeEssentials();
	
	std::vector<std::vector<std::pair<std::set<std::size_t>, bool>>> magic;
	
	for (const number_t &number : target)
	{
		auto &m = magic.emplace_back();
		for (std::size_t i = 0; i != minterms.size(); ++i)
			if (minterms[i].covers(number))
				m.emplace_back().first.insert(i);
	}
	
	for (auto x = magic.begin(); x != magic.end(); ++x)
	{
		if (!x->empty())
		{
			for (auto y = std::next(x); y != magic.end(); ++y)
			{
				if (!y->empty())
				{
					if (std::includes(x->cbegin(), x->cend(), y->cbegin(), y->cend()))
					{
						x->clear();
						break;
					}
					else if (std::includes(y->cbegin(), y->cend(), x->cbegin(), x->cend()))
					{
						y->clear();
					}
				}
			}
		}
	}
	magic.erase(std::remove_if(magic.begin(), magic.end(), [](const auto &x){ return x.empty(); }), magic.end());
	
	while (magic.size() >= 2)
	{
		auto multiplier0 = std::move(magic.back());
		magic.pop_back();
		auto multiplier1 = std::move(magic.back());
		magic.pop_back();
		auto &result = magic.emplace_back();
		
		for (auto &x : multiplier0)
		{
			for (auto &y : multiplier1)
			{
				if (std::includes(x.first.begin(), x.first.end(), y.first.begin(), y.first.end()))
				{
					result.emplace_back(x.first, false);
					x.second = true;
					y.second = true;
				}
				else if (std::includes(y.first.begin(), y.first.end(), x.first.begin(), x.first.end()))
				{
					result.emplace_back(y.first, false);
					x.second = true;
					y.second = true;
				}
			}
		}
		multiplier0.erase(std::remove_if(multiplier0.begin(), multiplier0.end(), [](auto &x){ return x.second; }), multiplier0.end());
		multiplier1.erase(std::remove_if(multiplier1.begin(), multiplier1.end(), [](auto &x){ return x.second; }), multiplier1.end());
		
		for (const auto &x : multiplier0)
		{
			for (const auto &y : multiplier1)
			{
				result.push_back(x);
				result.back().first.insert(y.first.cbegin(), y.first.cend());
			}
		}
	}
	
	if (magic.empty())
	{
		if (!essentials.empty())
			solutions.emplace_back(essentials);
		else
			solutions.emplace_back().push_back(PrimeImplicant::error());
	}
	else
	{
		for (auto x = magic.front().begin(); x != magic.front().end(); ++x)
		{
			if (!x->second)
			{
				for (auto y = std::next(x); y != magic.front().end(); ++y)
				{
					if (!y->second)
					{
						if (std::includes(x->first.cbegin(), x->first.cend(), y->first.cbegin(), y->first.cend()))
						{
							x->second = true;
							break;
						}
						else if (std::includes(y->first.cbegin(), y->first.cend(), x->first.cbegin(), x->first.cend()))
						{
							y->second = true;
						}
					}
				}
			}
		}
		magic.front().erase(std::remove_if(magic.front().begin(), magic.front().end(), [](auto &x){ return x.second; }), magic.front().end());
		magic.front().shrink_to_fit();
		
		solutions.reserve(magic.front().size());
		for (const auto &x : magic.front())
		{
			solutions.emplace_back();
			solutions.back().reserve(essentials.size() + x.first.size());
			for (const auto &e : essentials)
				solutions.back().push_back(e);
			for (const auto &y : x.first)
				solutions.back().push_back(minterms[y]);
		}
	}
}

void Karnaugh_Solution::prettyPrintSolution(const bits_t bits, const minterms_t &solution)
{
	numbers_t numbers;
	for (const auto &minterm : solution)
	{
		const auto x = minterm.findMinterms(bits);
		numbers.insert(x.cbegin(), x.end());
	}
	std::cout << "best fit:\n";
	Karnaugh::prettyPrintTable(bits, numbers);
}

Karnaugh_Solution Karnaugh_Solution::solve(const minterms_t &allMinters, const numbers_t &target)
{
	Karnaugh_Solution karnaugh_solver(allMinters, target);
	karnaugh_solver.solve();
	return karnaugh_solver;
}

typename Karnaugh_Solution::OptimizedSolution Karnaugh_Solution::optimizeSolutions(const std::vector<const minterms_t*> &solutions)
{
	static constexpr auto wipSumsSort = [](const std::set<const void*> &x, const std::set<const void*> &y) { return x.size() != y.size() ? x.size() < y.size() : x < y; };
	std::map<minterm_t, std::vector<const void*>> wipProducts;
	std::map<std::set<const void*>, std::vector<const void*>, decltype(wipSumsSort)> wipSums(wipSumsSort);
	std::vector<const void*> wipFinalSums;
	wipFinalSums.reserve(solutions.size());
	OptimizedSolution optimizedSolution;
	for (const minterms_t *const solution : solutions)
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
		minterm_t remainingInputs = iter->first;
		for (auto jiter = std::next(iter); jiter != wipProducts.rend(); ++jiter)
		{
			const minterm_t commonInputs = remainingInputs & jiter->first;
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
