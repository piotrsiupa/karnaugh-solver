#include "./Karnaugh_Solution.hh"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>


template<bits_t BITS>
void Karnaugh_Solution<BITS>::OptimizedSolution::print(std::ostream &o, const names_t &inputNames, const names_t &functionNames) const
{
	o << "Negated inputs:";
	bool first = true;
	for (number_t i = 0; i != BITS; ++i)
	{
		if ((negatedInputs & (1 << (BITS - i - 1))) != 0)
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
	o << '\n';
	
	o << "Products:";
	for (std::size_t i = 0; i != products.size(); ++i)
	{
		if (i == 0)
			o << ' ';
		else
			o << ", ";
		o << '[' << i << "] = ";
		Karnaugh<BITS>::printMinterm(o, inputNames, products[i].first);
		for (const auto &productRef : products[i].second)
			o << '[' << productRef << ']';
	}
	o << '\n';
	
	o << "Sums:";
	for (std::size_t i = 0; i != sums.size(); ++i)
	{
		if (i == 0)
			o << ' ';
		else
			o << ", ";
		o << '[' << i + products.size() << "] = ";
		for (const auto &productRef : sums[i])
			o << '[' << productRef << ']';
	}
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
	o << '\n';
	
	o << "\nGate scores: NOTs = " << getNotCount() << ", ANDs = " << getAndCount() << ", ORs = " << getOrCount() << '\n';
}

template<bits_t BITS>
Karnaugh_Solution<BITS>::Karnaugh_Solution(const minterms_t &minterms, const table_t &target, const table_t &dontCares) :
	minterms(minterms),
	target(target),
	dontCares(dontCares)
{
}

template<bits_t BITS>
typename Karnaugh_Solution<BITS>::table_t Karnaugh_Solution<BITS>::createMintermTable(const minterm_t minterm)
{
	table_t table;
	const number_t positiveMask = minterm.first;
	const number_t negativeMask = minterm.second ^ ((1 << BITS) - 1);
	for (number_t i = 0; i != 1 << BITS; ++i)
		table[(i | positiveMask) & negativeMask] = true;
	return table;
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::createMintermTables()
{
	const table_t cares = ~dontCares;
	mintermTables.resize(minterms.size());
	for (size_t i = 0; i != minterms.size(); ++i)
		mintermTables[i] = createMintermTable(minterms[i]) & cares;
}

template<bits_t BITS>
typename Karnaugh_Solution<BITS>::minterms_t Karnaugh_Solution<BITS>::removeEssentials()
{
	createMintermTables();
	
	table_t all, repeated;
	for (const table_t &x : mintermTables)
	{
		repeated |= all & x;
		all |= x;
	}
	const table_t unique = all & ~repeated;
	
	minterms_t essentials;
	table_t essentialTable;
	for (std::size_t i = 0; i != minterms.size(); ++i)
	{
		if ((mintermTables[i] & unique).any())
		{
			essentials.push_back(minterms[i]);
			essentialTable |= mintermTables[i];
			mintermTables[i].reset();
		}
	}
	
	const table_t nonEssentialsMask = ~essentialTable;
	for (table_t &mintermTable : mintermTables)
		mintermTable &= nonEssentialsMask;
	target &= nonEssentialsMask;
	
	for (std::size_t i = 0; i != minterms.size(); ++i)
		if (mintermTables[i].none())
			minterms[i] = {0, 0};
	
	minterms.erase(std::remove_if(minterms.begin(), minterms.end(), [](const minterm_t x){ return x.first == 0 && x.second == 0; }), minterms.end());
	mintermTables.erase(std::remove_if(mintermTables.begin(), mintermTables.end(), [](const table_t &x){ return x.none(); }), mintermTables.end());
	
	return essentials;
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::solve()
{
	const minterms_t essentials = removeEssentials();
	
	std::vector<std::vector<std::pair<std::set<std::size_t>, bool>>> magic;
	
	for (number_t i = 0; i != 1 << BITS; ++i)
	{
		if (target[i])
		{
			auto &m = magic.emplace_back();
			for (std::size_t j = 0; j != minterms.size(); ++j)
				if (mintermTables[j][i])
					m.emplace_back().first.insert(j);
		}
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
		solutions.emplace_back(essentials);
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

template<bits_t BITS>
void Karnaugh_Solution<BITS>::prettyPrintSolution(const minterms_t &solution)
{
	table_t solutionTable;
	for (const auto &x : solution)
		solutionTable |= createMintermTable(x);
	std::cout << "best fit:\n";
	Karnaugh<BITS>::prettyPrintTable(solutionTable);
}

template<bits_t BITS>
Karnaugh_Solution<BITS> Karnaugh_Solution<BITS>::solve(const minterms_t &allMinters, const table_t &target, const table_t &dontCares)
{
	Karnaugh_Solution karnaugh_solver(allMinters, target, dontCares);
	karnaugh_solver.solve();
	return karnaugh_solver;
}

template<bits_t BITS>
typename Karnaugh_Solution<BITS>::OptimizedSolution Karnaugh_Solution<BITS>::optimizeSolutions(const std::vector<const minterms_t*> &solutions)
{
	static constexpr auto wipProductsSort = [](const minterm_t x, const minterm_t y) { return Karnaugh<BITS>::compareMinterms(x, y); };
	static constexpr auto wipSumsSort = [](const std::set<const void*> &x, const std::set<const void*> &y) { return x.size() != y.size() ? x.size() < y.size() : x < y; };
	std::map<minterm_t, std::vector<const void*>, decltype(wipProductsSort)> wipProducts(wipProductsSort);
	std::map<std::set<const void*>, std::vector<const void*>, decltype(wipSumsSort)> wipSums(wipSumsSort);
	std::vector<const void*> wipFinalSums;
	wipFinalSums.reserve(solutions.size());
	OptimizedSolution optimizedSolution;
	for (const minterms_t *const solution : solutions)
	{
		std::set<const void*> wipSum;
		for (const auto &x : *solution)
		{
			optimizedSolution.negatedInputs |= x.second;
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
			const minterm_t commonInputs = {remainingInputs.first & jiter->first.first, remainingInputs.second & jiter->first.second};
			if (Karnaugh<BITS>::getOnesCount(commonInputs) > 1)
			{
				remainingInputs.first &= ~commonInputs.first;
				remainingInputs.second &= ~commonInputs.second;
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
	{
		for (const std::size_t productRef : productIter->second)
		{
			productIter->first.first &= ~optimizedSolution.products[productRef].first.first;
			productIter->first.second &= ~optimizedSolution.products[productRef].first.second;
		}
	}
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


template class Karnaugh_Solution<2>;
template class Karnaugh_Solution<3>;
template class Karnaugh_Solution<4>;
template class Karnaugh_Solution<5>;
template class Karnaugh_Solution<6>;
template class Karnaugh_Solution<7>;
template class Karnaugh_Solution<8>;
template class Karnaugh_Solution<9>;
template class Karnaugh_Solution<10>;
template class Karnaugh_Solution<11>;
template class Karnaugh_Solution<12>;
template class Karnaugh_Solution<13>;
template class Karnaugh_Solution<14>;
template class Karnaugh_Solution<15>;
template class Karnaugh_Solution<16>;
