#include "./Karnaugh_Solution.hh"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>


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
	solution = removeEssentials();
	
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
					result.emplace_back(x);
					x.second = true;
					y.second = true;
				}
				else if (std::includes(y.first.begin(), y.first.end(), x.first.begin(), x.first.end()))
				{
					result.emplace_back(y);
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
	
	if (!magic.empty())
		for (const auto &x : magic.front().front().first)
			solution.push_back(minterms[x]);
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::prettyPrintSolution() const
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
