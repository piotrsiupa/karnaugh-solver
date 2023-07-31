#include "./Karnaugh.hh"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>

#include "input.hh"
#include "math.hh"


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
	const auto startTime = std::chrono::system_clock::now();
	auto bestTime = startTime;
	std::uintmax_t progressInterval = 65536;
	bool progressAdjusted = false;
	intSignalFlag = false;
	static constexpr char clearProgress[] = "\033[2K\033[A\033[2K\033[A\033[2K\033[A\033[2K\033[A\033[2K\r";
	const double solutionSpaceSize = std::pow(2.0, allMinterms.size());
	for (std::uintmax_t progressCounter = 0;; ++progressCounter)
	{
		if (progressCounter == progressInterval)
		{
			if (progressAdjusted)
				std::clog << clearProgress;
			if (intSignalFlag)
				break;
			const auto currentTime = std::chrono::system_clock::now();
			const std::chrono::duration<double> elapsedTime = currentTime - startTime;
			if (!progressAdjusted)
			{
				static constexpr double targetInterval = 5.0;
				if (elapsedTime.count() < targetInterval)
				{
					std::uintmax_t intervalAdjustment = progressInterval * ((targetInterval - elapsedTime.count()) / elapsedTime.count());
					if (intervalAdjustment != 0)
					{
						progressInterval += intervalAdjustment;
						goto skip_progress;
					}
				}
				progressAdjusted = true;
				static constexpr double magicConstantOfBetterTiming = 1.07;
				progressInterval *= magicConstantOfBetterTiming;
			}
			progressCounter = 0;
			const std::chrono::duration<double> elapsedTimeSinceBest = currentTime - bestTime;
			std::clog << "Oops... This is taking a long time. (currently " << std::fixed << std::setprecision(1) << elapsedTime.count() << " seconds)\n";
			std::clog << "The best solution so far uses " << best.size() << " out of " << allMinterms.size() << " minterms. (found " << elapsedTimeSinceBest.count() << " seconds ago)\n";
			std::clog << "(Currently testing combination:";
			for (const std::size_t x : current)
				std::clog << ' ' << x;
			std::clog << ")\n";
			double remaingSolutionsFactor = 1.0;
			double currentCutoffFactor = 1.0;
			std::size_t expectedValue = 0;
			for (const std::size_t x : current)
			{
				currentCutoffFactor /= 2;
				while (x != expectedValue)
				{
					remaingSolutionsFactor -= currentCutoffFactor;
					currentCutoffFactor /= 2;
					expectedValue += 1;
				}
				expectedValue += 1;
			}
			const double remainingSolutions = calcBinomialCoefficientSum(allMinterms.size(), best.size()) * remaingSolutionsFactor;
			const double progressPercent = remainingSolutions / solutionSpaceSize * 100.0;
			std::clog << "Estimation of remaining solution space: ~" << std::fixed << std::setprecision(3) << progressPercent << "% (~" << std::setprecision(0) << remainingSolutions << " combinations)\n";
			std::clog << "Press Ctrl-C to abort further search and use this solution..." << std::flush;
		}
		skip_progress:
		table_t currentTable;
		for (const std::size_t x : current)
			currentTable |= mintermTables[x];
		if ((currentTable & target) == target && (currentTable & acceptable) == currentTable)
		{
			best = current;
			bestTime = std::chrono::system_clock::now();
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
		{
			if (progressAdjusted)
				std::clog << clearProgress;
			break;
		}
		go_next:
		if (++current.back() == allMinterms.size())
			goto go_back;
	}
	std::clog << std::flush;
	
	table_t bestTable;
	for (const std::size_t x : best)
		bestTable |= mintermTables[x];
	std::cout << "best fit:\n";
	prettyPrintTable(bestTable);
	
	minterms_t bestMinterms;
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
		
		const minterms_t solution = karnaugh.solve();
		
		std::cout << "solution:\n";
		bool first = true;
		for (const minterm_t &minterm : solution)
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
