#include <algorithm>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <limits>
#include <regex>
#include <string>
#include <utility>
#include <vector>


using names_t = std::vector<std::string>;
using bits_t = std::uint_fast8_t;


template<bits_t BITS>
class Karnaugh
{
	using table_t = std::bitset<1 << BITS>;
	using number_t = std::uint_fast16_t;
	using grayCode_t = std::vector<number_t>;
	using minterm_t = std::pair<number_t, number_t>;
	using minterms_t = std::vector<minterm_t>;
	
	const names_t &names;
	table_t target, acceptable;
	minterms_t allMinterms;
	
	Karnaugh(const names_t &names) : names(names) {}
	
	static grayCode_t makeGrayCode(const bits_t bits);
	static void printBits(const number_t number, const bits_t bits);
	static void prettyPrintTable(const table_t &target, const table_t &acceptable = {});
	
	static bool loadTable(table_t &table);
	bool loadData();
	
	static minterms_t makeAllPossibleMinterms();
	void findMinterms();
	void printMinterm(const minterm_t minterm) const;
	
	minterms_t solve() const;
	
public:
	static bool doItAll(const names_t &names);
};

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
	const grayCode_t vGrayCode = makeGrayCode(vBits);
	const grayCode_t hGrayCode = makeGrayCode(hBits);
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
bool Karnaugh<BITS>::loadTable(table_t &table)
{
	std::string line;
	std::getline(std::cin, line);
	if (!std::cin)
	{
		std::cerr << "Cannot read line from stdin!\n";
		return false;
	}
	static std::regex separator("\\s*,\\s*|\\s+");
	static std::regex_token_iterator<std::string::const_iterator> rend;
	for (std::sregex_token_iterator iter(line.cbegin(), line.cend(), separator, -1); iter != rend; ++iter)
	{
		try
		{
			const auto num = std::stoi(*iter);
			if (num >= (1 << BITS))
			{
				std::cerr << '"' << *iter << "\" is too big!\n";
				return false;
			}
			table[num] = true;
		}
		catch (std::invalid_argument &e)
		{
			std::cerr << '"' << *iter << "\" is not a number!\n";
			return false;
		}
	}
	return true;
}

template<bits_t BITS>
bool Karnaugh<BITS>::loadData()
{
	if (!loadTable(target))
		return false;
	if (!loadTable(acceptable))
		return false;
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
	static const auto getOnesCount = [](const minterm_t minterm) -> bits_t { return __builtin_popcount((minterm.first << 16) | minterm.second); };
	static const auto hasLessOnes = [](const minterm_t x, const minterm_t y) -> bool { return getOnesCount(x) < getOnesCount(y); };
	std::stable_sort(minterms.begin(), minterms.end(), hasLessOnes);
	return minterms;
}

template<bits_t BITS>
void Karnaugh<BITS>::findMinterms()
{
	for (const minterm_t &newMinterm : makeAllPossibleMinterms())
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
void Karnaugh<BITS>::printMinterm(const minterm_t minterm) const
{
	for (number_t i = 0; i != BITS; ++i)
	{
		const bool normal = (minterm.first & (1 << (BITS - i - 1))) != 0;
		const bool negated = (minterm.second & (1 << (BITS - i - 1))) != 0;
		if (normal || negated)
		{
			std::cout << names[i];
			if (negated)
				std::cout << '\'';
		}
	}
}

template<bits_t BITS>
typename Karnaugh<BITS>::minterms_t Karnaugh<BITS>::solve() const
{
	if (target.none())
		return {};
	
	std::vector<table_t> mintermTables;
	mintermTables.resize(allMinterms.size());
	for (size_t i = 0; i != allMinterms.size(); ++i)
	{
		const number_t positiveMask = allMinterms[i].first;
		const number_t negativeMask = allMinterms[i].second ^ ((1 << BITS) - 1);
		for (number_t j = 0; j != 1 << BITS; ++j)
			mintermTables[i][(j | positiveMask) & negativeMask] = true;
	}
	
	std::vector<std::size_t> best(allMinterms.size());
	for (std::size_t i = 0; i != allMinterms.size(); ++i)
		best[i] = i;
	std::vector<std::size_t> current;
	current.reserve(allMinterms.size());
	current.emplace_back(0);
	while (true)
	{
		table_t currentTable;
		for (const std::size_t x : current)
			currentTable |= mintermTables[x];
		if ((currentTable & target) == target && (currentTable & acceptable) == currentTable)
		{
			best = current;
			goto go_back;
		}
		if (current.size() + 1 == best.size())
			goto go_next;
		current.emplace_back(current.back() + 1);
		if (current.back() == allMinterms.size())
			goto go_back;
		continue;
		go_back:
		current.pop_back();
		if (current.empty())
			break;
		go_next:
		if (++current.back() == allMinterms.size())
			goto go_back;
	}
	minterms_t bestMinterms;
	
	table_t bestTable;
	for (const std::size_t x : best)
		bestTable |= mintermTables[x];
	prettyPrintTable(bestTable);
	
	bestMinterms.reserve(best.size());
	for (const std::size_t x : best)
		bestMinterms.emplace_back(allMinterms[x]);
	return bestMinterms;
}

template<bits_t BITS>
bool Karnaugh<BITS>::doItAll(const names_t &names)
{
	Karnaugh karnaugh(names);
	if (!karnaugh.loadData())
		return false;
	prettyPrintTable(karnaugh.target, karnaugh.acceptable);
	
	karnaugh.findMinterms();
	bool first = true;
	for (const minterm_t &minterm : karnaugh.allMinterms)
	{
		if (first)
			first = false;
		else
			std::cout << ' ';
		karnaugh.printMinterm(minterm);
	}
	std::cout << '\n' << std::endl;
	
	const minterms_t solution = karnaugh.solve();
	first = true;
	for (const minterm_t &minterm : solution)
	{
		if (first)
			first = false;
		else
			std::cout << " + ";
		karnaugh.printMinterm(minterm);
	}
	std::cout << std::endl;
	
	return true;
}


bool loadNames(names_t &names)
{
	std::string line;
	std::getline(std::cin, line);
	if (!std::cin)
	{
		std::cerr << "Cannot read line from stdin!\n";
		return false;
	}
	static std::regex separator("\\s*,\\s*|\\s+");
	static std::regex_token_iterator<std::string::const_iterator> rend;
	for (std::sregex_token_iterator iter(line.cbegin(), line.cend(), separator, -1); iter != rend; ++iter)
		names.emplace_back(*iter);
	return true;
}

int main()
{
	names_t names;
	if (!loadNames(names))
		return 1;
	bool result;
	switch (names.size())
	{
	case 2:
		result = Karnaugh<2>::doItAll(names);
		break;
	case 3:
		result = Karnaugh<3>::doItAll(names);
		break;
	case 4:
		result = Karnaugh<4>::doItAll(names);
		break;
	case 5:
		result = Karnaugh<5>::doItAll(names);
		break;
	case 6:
		result = Karnaugh<6>::doItAll(names);
		break;
	case 7:
		result = Karnaugh<7>::doItAll(names);
		break;
	case 8:
		result = Karnaugh<8>::doItAll(names);
		break;
	case 9:
		result = Karnaugh<9>::doItAll(names);
		break;
	case 10:
		result = Karnaugh<10>::doItAll(names);
		break;
	case 11:
		result = Karnaugh<11>::doItAll(names);
		break;
	case 12:
		result = Karnaugh<12>::doItAll(names);
		break;
	case 13:
		result = Karnaugh<13>::doItAll(names);
		break;
	case 14:
		result = Karnaugh<14>::doItAll(names);
		break;
	case 15:
		result = Karnaugh<15>::doItAll(names);
		break;
	case 16:
		result = Karnaugh<16>::doItAll(names);
		break;
	default:
		std::cerr << "Unsupported number of variables!\n";
		return 1;
	}
	return result ? 0 : 1;
}
