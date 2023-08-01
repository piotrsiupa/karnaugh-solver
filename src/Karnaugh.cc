#include "./Karnaugh.hh"

#include <algorithm>
#include <iostream>
#include <map>

#include "./Karnaugh_Solution.hh"
#include "input.hh"


template<bits_t BITS>
std::size_t Karnaugh<BITS>::nameCount = 0;

template<bits_t BITS>
typename Karnaugh<BITS>::grayCode_t Karnaugh<BITS>::makeGrayCode(const bits_t bits)
{
	grayCode_t grayCode;
	grayCode.reserve(1 << bits);
	grayCode.push_back(0);
	grayCode.push_back(1);
	for (bits_t i = 1; i != bits; ++i)
		for (number_t j = 0; j != unsigned(1) << i; ++j)
			grayCode.push_back(grayCode[j ^ ((1 << i) - 1)] | (1 << i));
	return grayCode;
}

template<bits_t BITS>
void Karnaugh<BITS>::printBits(const number_t number, const bits_t bits)
{
	for (bits_t i = bits; i != 0; --i)
		std::cout << ((number & (1 << (i - 1))) != 0 ? '1' : '0');
}

template<bits_t BITS>
void Karnaugh<BITS>::prettyPrintTable(const table_t &target, const table_t &acceptable)
{
	static constexpr bits_t vBits = (BITS + 1) / 2;
	static constexpr bits_t hBits = BITS / 2;
	static const grayCode_t vGrayCode = makeGrayCode(vBits);
	static const grayCode_t hGrayCode = makeGrayCode(hBits);
	for (int i = 0; i != vBits; ++i)
		std::cout << ' ';
	std::cout << ' ';
	for (const number_t y : hGrayCode)
	{
		printBits(y, hBits);
		std::cout << ' ';
	}
	std::cout << '\n';
	for (const number_t x : vGrayCode)
	{
		printBits(x, vBits);
		std::cout << ' ';
		bool first = true;
		for (int i = 0; i != (hBits - 1) / 2; ++i)
			std::cout << ' ';
		for (const number_t y : hGrayCode)
		{
			if (first)
				first = false;
			else
				for (int i = 0; i != hBits; ++i)
					std::cout << ' ';
			const bits_t index = (x << hBits) | y;
			std::cout << (target[index] ? 'T' : (acceptable[index] ? '-' : 'F'));
		}
		std::cout << '\n';
	}
	std::cout << std::endl;
}

template<bits_t BITS>
bool Karnaugh<BITS>::loadTable(table_t &table, std::string &line)
{
	for (const std::string &string : readStrings(line))
	{
		try
		{
			const auto num = std::stoi(string);
			if (num >= (1 << BITS))
			{
				std::cerr << '"' << string << "\" is too big!\n";
				return false;
			}
			table[num] = true;
		}
		catch (std::invalid_argument &e)
		{
			std::cerr << '"' << string << "\" is not a number!\n";
			return false;
		}
	}
	return true;
}

template<bits_t BITS>
bool Karnaugh<BITS>::loadData(lines_t &lines)
{
	if (!lines.empty() && (lines.front().front() < '0' || lines.front().front() >'9'))
	{
		functionName = std::move(lines.front());
		lines.pop_front();
	}
	if (lines.size() < 2)
	{
		std::cerr << "A description of a Karnaugh map has to have 2 lines!\n";
		return false;
	}
	if (!loadTable(target, lines.front()))
		return false;
	lines.pop_front();
	if (!loadTable(acceptable, lines.front()))
		return false;
	lines.pop_front();
	acceptable |= target;
	return true;
}

template<bits_t BITS>
typename Karnaugh<BITS>::minterms_t Karnaugh<BITS>::makeAllPossibleMinterms()
{
	static_assert(sizeof(int) >= 4);
	minterms_t minterms;
	for (std::size_t i = 0; i != (std::size_t(1) << (BITS * 2)); ++i)
	{
		const number_t normal = i & ((1 << BITS) - 1);
		const number_t negated = i >> BITS;
		if ((normal & negated) == 0)
			minterms.emplace_back(normal, negated);
	}
	static const auto hasLessOnes = [](const minterm_t x, const minterm_t y) -> bool { return getOnesCount(x) < getOnesCount(y); };
	std::stable_sort(minterms.begin(), minterms.end(), hasLessOnes);
	minterms.shrink_to_fit();
	return minterms;
}

template<bits_t BITS>
void Karnaugh<BITS>::findMinterms()
{
	static const minterms_t allPossibleMinterms = makeAllPossibleMinterms();
	for (const minterm_t &newMinterm : allPossibleMinterms)
	{
		const number_t positiveMask = newMinterm.first;
		const number_t negativeMask = newMinterm.second ^ ((1 << BITS) - 1);
		for (number_t i = 0; i != 1 << BITS; ++i)
			if (!acceptable[(i | positiveMask) & negativeMask])
				goto next_term;
		for (const minterm_t &oldMinterm : allMinterms)
			if ((oldMinterm.first & newMinterm.first) == oldMinterm.first && (oldMinterm.second & newMinterm.second) == oldMinterm.second)
				goto next_term;
		allMinterms.emplace_back(newMinterm);
		next_term:;
	}
}

template<bits_t BITS>
typename Karnaugh<BITS>::splitMinterm_t Karnaugh<BITS>::splitMinterm(const minterm_t &minterm)
{
	splitMinterm_t splitMinterm;
	for (number_t i = 0; i != BITS; ++i)
	{
		const bool normal = (minterm.first & (1 << (BITS - i - 1))) != 0;
		const bool negated = (minterm.second & (1 << (BITS - i - 1))) != 0;
		if (normal || negated)
			splitMinterm.emplace_back(i, negated);
	}
	return splitMinterm;
}

template<bits_t BITS>
void Karnaugh<BITS>::printMinterm(const minterm_t minterm) const
{
	for (const mintermPart_t &mintermPart : splitMinterm(minterm))
	{
		std::cout << inputNames[mintermPart.first];
		if (mintermPart.second)
			std::cout << '\'';
	}
}

template<bits_t BITS>
void Karnaugh<BITS>::applyHeuristic(std::list<Karnaugh> &karnaughs)
{
	std::map<mintermPart_t, std::size_t> mintermPartCounts;
	for (const Karnaugh &karnaugh : karnaughs)
		for (const minterm_t &minterm : karnaugh.allMinterms)
			for (const mintermPart_t &x : splitMinterm(minterm))
				++mintermPartCounts[x];
	std::map<minterm_t, std::size_t> mintermWeights;
	const auto getMintermWeight = [&mintermPartCounts = std::as_const(mintermPartCounts), &mintermWeights](const minterm_t &minterm) -> std::size_t
		{
			std::size_t &weight = mintermWeights[minterm];
			if (weight == 0)
				for (const mintermPart_t &x : splitMinterm(minterm))
					weight += mintermPartCounts.at(x);
			return weight;
		};
	const auto theHeuristic = [&getMintermWeight](const minterm_t &x, const minterm_t &y) -> bool
		{
			const bits_t xOnes = getOnesCount(x), yOnes = getOnesCount(y);
			return xOnes == yOnes
				? getMintermWeight(x) > getMintermWeight(y)
				: xOnes < yOnes;
		};
	for (Karnaugh &karnaugh : karnaughs)
		std::stable_sort(karnaugh.allMinterms.begin(), karnaugh.allMinterms.end(), theHeuristic);
}

template<bits_t BITS>
Karnaugh_Solution<BITS> Karnaugh<BITS>::solve() const
{
	return Karnaugh_Solution<BITS>::solve(*this);
}

template<bits_t BITS>
bool Karnaugh<BITS>::processMultiple(const names_t &inputNames, lines_t &lines)
{
	std::list<Karnaugh> karnaughs;
	while (!lines.empty())
	{
		karnaughs.push_back(inputNames);
		
		Karnaugh &karnaugh = karnaughs.back();
		if (!karnaugh.loadData(lines))
			return false;
		
		karnaugh.findMinterms();
	}
	
	applyHeuristic(karnaughs);
	
	bool firstKarnaugh = true;
	for (Karnaugh &karnaugh : karnaughs)
	{
		if (firstKarnaugh)
			firstKarnaugh = false;
		else
			std::cout << '\n';
		std::cout << "=== " << karnaugh.functionName << " ===\n\n";
		
		std::cout << "goal:\n";
		prettyPrintTable(karnaugh.target, karnaugh.acceptable);
		
		const Karnaugh_Solution<BITS> solution = karnaugh.solve();
		if (!solution.isBestFitValid())
		{
			std::clog << "No solution found!\n";
			return false;
		}
		solution.prettyPrintBestFit();
		
		std::cout << "solution:\n";
		bool first = true;
		for (const minterm_t &minterm : solution.getBestMinterms())
		{
			if (first)
				first = false;
			else
				std::cout << " + ";
			karnaugh.printMinterm(minterm);
		}
		
		std::cout << std::endl;
	}
	
	return true;
}


template class Karnaugh<2>;
template class Karnaugh<3>;
template class Karnaugh<4>;
template class Karnaugh<5>;
template class Karnaugh<6>;
template class Karnaugh<7>;
template class Karnaugh<8>;
template class Karnaugh<9>;
template class Karnaugh<10>;
template class Karnaugh<11>;
template class Karnaugh<12>;
template class Karnaugh<13>;
template class Karnaugh<14>;
template class Karnaugh<15>;
template class Karnaugh<16>;
