#include <algorithm>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <regex>
#include <string>
#include <utility>
#include <vector>


using lines_t = std::list<std::string>;
using strings_t = std::vector<std::string>;
using names_t = std::vector<std::string>;
using bits_t = std::uint_fast8_t;

strings_t readStrings(std::string &line)
{
	strings_t strings;
	if (line == "-")
		return strings;
	static const std::regex separator("\\s*,\\s*|\\s+");
	static const std::regex_token_iterator<std::string::const_iterator> rend;
	for (std::sregex_token_iterator iter(line.cbegin(), line.cend(), separator, -1); iter != rend; ++iter)
		if (iter->length() != 0)
			strings.emplace_back(*iter);
	return strings;
}


template<bits_t BITS>
class Karnaugh
{
	using table_t = std::bitset<1 << BITS>;
	using number_t = std::uint_fast16_t;
	using grayCode_t = std::vector<number_t>;
	using minterm_t = std::pair<number_t, number_t>;
	using minterms_t = std::vector<minterm_t>;
	using mintermPart_t = std::pair<bits_t, bool>;
	using splitMinterm_t = std::vector<mintermPart_t>;
	
	static std::size_t nameCount;
	
	const names_t &inputNames;
	std::string functionName;
	table_t target, acceptable;
	minterms_t allMinterms;
	
	Karnaugh(const names_t &inputNames) : inputNames(inputNames), functionName('f' + std::to_string(nameCount++)) {}
	
	static grayCode_t makeGrayCode(const bits_t bits);
	static void printBits(const number_t number, const bits_t bits);
	static void prettyPrintTable(const table_t &target, const table_t &acceptable = {});
	
	static bool loadTable(table_t &table, std::string &line);
	bool loadData(lines_t &lines);
	
	static bits_t getOnesCount(const minterm_t minterm) { return __builtin_popcount((minterm.first << 16) | minterm.second); }
	static minterms_t makeAllPossibleMinterms();
	void findMinterms();
	static splitMinterm_t splitMinterm(const minterm_t &minterm);
	void printMinterm(const minterm_t minterm) const;
	
	static void applyHeuristic(std::list<Karnaugh> &karnaughs);
	
	minterms_t solve() const;
	
public:
	Karnaugh(Karnaugh &&) = default;
	Karnaugh& operator=(Karnaugh &&) = default;
	
	static bool processMultiple(const names_t &inputNames, lines_t &lines);
};

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
	std::cout << functionName << '\n';
	prettyPrintTable(bestTable);
	
	bestMinterms.reserve(best.size());
	for (const std::size_t x : best)
		bestMinterms.emplace_back(allMinterms[x]);
	return bestMinterms;
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
		std::cout << karnaugh.functionName << '\n';
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
	}
	
	applyHeuristic(karnaughs);
	
	for (Karnaugh &karnaugh : karnaughs)
	{
		const minterms_t solution = karnaugh.solve();
		bool first = true;
		for (const minterm_t &minterm : solution)
		{
			if (first)
				first = false;
			else
				std::cout << " + ";
			karnaugh.printMinterm(minterm);
		}
		std::cout << '\n' << std::endl;
	}
	
	return true;
}


void trimString(std::string &string)
{
    string.erase(string.begin(), std::find_if(string.begin(), string.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    string.erase(std::find_if(string.rbegin(), string.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), string.end());
}

bool loadLines(std::istream &in, lines_t &lines)
{
	lines.clear();
	while (true)
	{
		std::string line;
		std::getline(in, line);
		if (in.eof())
			return true;
		if (!in)
		{
			std::cerr << "Cannot read from stdin!\n";
			return false;
		}
		trimString(line);
		if (!line.empty() && line.front() != '#')
			lines.emplace_back(std::move(line));
	}
}

names_t loadNames(std::string &line)
{
	return readStrings(line);
}

int main()
{
	lines_t lines;
	if (!loadLines(std::cin, lines))
		return 1;
	if (lines.empty())
	{
		std::cerr << "Input names are missing!\n";
		return 1;
	}
	const names_t names = loadNames(lines.front());
	lines.pop_front();
	bool result;
	switch (names.size())
	{
	case 2:
		result = Karnaugh<2>::processMultiple(names, lines);
		break;
	case 3:
		result = Karnaugh<3>::processMultiple(names, lines);
		break;
	case 4:
		result = Karnaugh<4>::processMultiple(names, lines);
		break;
	case 5:
		result = Karnaugh<5>::processMultiple(names, lines);
		break;
	case 6:
		result = Karnaugh<6>::processMultiple(names, lines);
		break;
	case 7:
		result = Karnaugh<7>::processMultiple(names, lines);
		break;
	case 8:
		result = Karnaugh<8>::processMultiple(names, lines);
		break;
	case 9:
		result = Karnaugh<9>::processMultiple(names, lines);
		break;
	case 10:
		result = Karnaugh<10>::processMultiple(names, lines);
		break;
	case 11:
		result = Karnaugh<11>::processMultiple(names, lines);
		break;
	case 12:
		result = Karnaugh<12>::processMultiple(names, lines);
		break;
	case 13:
		result = Karnaugh<13>::processMultiple(names, lines);
		break;
	case 14:
		result = Karnaugh<14>::processMultiple(names, lines);
		break;
	case 15:
		result = Karnaugh<15>::processMultiple(names, lines);
		break;
	case 16:
		result = Karnaugh<16>::processMultiple(names, lines);
		break;
	default:
		std::cerr << "Unsupported number of variables!\n";
		return 1;
	}
	return result ? 0 : 1;
}
