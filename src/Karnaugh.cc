#include "./Karnaugh.hh"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>

#include "./Karnaugh_Solution.hh"
#include "input.hh"


std::size_t Karnaugh::nameCount = 0;

Karnaugh::grayCode_t Karnaugh::makeGrayCode(const bits_t bits)
{
	grayCode_t grayCode;
	grayCode.reserve(1u << bits);
	grayCode.push_back(0);
	if (bits != 0)
	{
		grayCode.push_back(1);
		for (bits_t i = 1; i != bits; ++i)
			for (number_t j = 0; j != unsigned(1) << i; ++j)
				grayCode.push_back(grayCode[j ^ ((1 << i) - 1)] | (1 << i));
	}
	return grayCode;
}

void Karnaugh::printBits(const number_t number, const bits_t bits)
{
	for (bits_t i = bits; i != 0; --i)
		std::cout << ((number & (1 << (i - 1))) != 0 ? '1' : '0');
}

void Karnaugh::prettyPrintTable(const bits_t bits, const numbers_t &target, const numbers_t &allowed)
{
	const bits_t vBits = (bits + 1) / 2;
	const bits_t hBits = bits / 2;
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
		printBits(x, std::max(bits_t(1), vBits));
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
			const number_t number = (x << hBits) | y;
			std::cout << (target.find(number) != target.cend() ? 'T' : (allowed.find(number) != allowed.cend() ? '-' : 'F'));
		}
		std::cout << '\n';
	}
	std::cout << std::endl;
}

bool Karnaugh::loadNumbers(numbers_t &numbers, std::string &line) const
{
	for (const std::string &string : readStrings(line))
	{
		try
		{
			const auto num = std::stoi(string);
			if (num < 0)
			{
				std::cerr << '"' << string << "\" is negative!\n";
				return false;
			}
			else if (num >= (1 << bits))
			{
				std::cerr << '"' << string << "\" is too big!\n";
				return false;
			}
			numbers.insert(num);
		}
		catch (std::invalid_argument &e)
		{
			std::cerr << '"' << string << "\" is not a number!\n";
			return false;
		}
	}
	return true;
}

bool Karnaugh::loadData(lines_t &lines)
{
	if (!lines.empty() && lines.front() != "-" && !std::all_of(lines.front().cbegin(), lines.front().cend(), [](const char c){ return std::isdigit(c) || std::iswspace(c) || c == ','; }))
	{
		functionName = std::move(lines.front());
		lines.pop_front();
	}
	if (lines.size() < 2)
	{
		std::cerr << "A description of a Karnaugh map has to have 2 lines!\n";
		return false;
	}
	if (!loadNumbers(target, lines.front()))
		return false;
	lines.pop_front();
	if (!loadNumbers(dontCares, lines.front()))
		return false;
	lines.pop_front();
	allowed.insert(target.cbegin(), target.cend());
	allowed.insert(dontCares.cbegin(), dontCares.cend());
	return true;
}

void Karnaugh::findMinterms()
{
	std::vector<std::pair<minterm_t, bool>> oldMinterms;
	for (const number_t &number : allowed)
		oldMinterms.emplace_back(minterm_t{number, number ^ ((1u << bits) - 1)}, false);
	std::set<minterm_t> newMinterms;
	for (bits_t oldOnesCount = bits; oldOnesCount != 0; --oldOnesCount)
	{
		for (auto iter = oldMinterms.begin(); iter != oldMinterms.end(); ++iter)
		{
			for (auto jiter = std::next(iter); jiter != oldMinterms.end(); ++jiter)
			{
				const minterm_t commonPart = {iter->first.first & jiter->first.first, iter->first.second & jiter->first.second};
				if (getOnesCount(commonPart) == oldOnesCount - 1 && ((~commonPart.first & iter->first.first) | (~commonPart.first & jiter->first.first)) == ((~commonPart.second & iter->first.second) | (~commonPart.second & jiter->first.second)))
				{
					newMinterms.insert(commonPart);
					iter->second = true;
					jiter->second = true;
				}
			}
		}
		for (const auto &oldMinterm : oldMinterms)
			if (!oldMinterm.second)
				allMinterms.push_back(oldMinterm.first);
		oldMinterms.clear();
		oldMinterms.reserve(newMinterms.size());
		for (const auto &newMinterm : newMinterms)
			oldMinterms.emplace_back(newMinterm, false);
		newMinterms.clear();
	}
	if (!oldMinterms.empty())
		allMinterms.push_back(oldMinterms.front().first);
}
	
bool Karnaugh::compareMinterms(const minterm_t x, const minterm_t y)
{
	const bits_t xOnes = getOnesCount(x);
	const bits_t yOnes = getOnesCount(y);
	if (xOnes != yOnes)
		return xOnes < yOnes;
	const number_t xMask = x.first | x.second;
	const number_t yMask = y.first | y.second;
	if (xMask != yMask)
		return xMask > yMask;
	return x.first > y.first;
}

Karnaugh::splitMinterm_t Karnaugh::splitMinterm(const bits_t bits, const minterm_t &minterm)
{
	splitMinterm_t splitMinterm;
	for (number_t i = 0; i != bits; ++i)
	{
		const bool normal = (minterm.first & (1 << (bits - i - 1))) != 0;
		const bool negated = (minterm.second & (1 << (bits - i - 1))) != 0;
		if (normal || negated)
			splitMinterm.emplace_back(i, negated);
	}
	return splitMinterm;
}

void Karnaugh::printMinterm(const bits_t bits, std::ostream &o, const names_t &inputNames, const minterm_t minterm, const bool parentheses)
{
	const bits_t onesCount = getOnesCount(minterm);
	if (onesCount == 0)
	{
		if (minterm == minterm_t{0, 0})
			o << "<True>";
		else
			o << "<False>";
		return;
	}
	const bool needsParentheses = parentheses && onesCount != 1;
	if (needsParentheses)
		o << '(';
	bool first = true;
	for (const mintermPart_t &mintermPart : splitMinterm(bits, minterm))
	{
		if (first)
			first = false;
		else
			o << " && ";
		if (mintermPart.second)
			o << '!';
		o << inputNames[mintermPart.first];
	}
	if (needsParentheses)
		o << ')';
}

void Karnaugh::printMinterm(const minterm_t minterm, const bool parentheses) const
{
	return printMinterm(bits, std::cout, inputNames, minterm, parentheses);
}

void Karnaugh::printMinterms(minterms_t minterms) const
{
	if (minterms.size() == 1)
	{
		printMinterm(minterms.front(), false);
	}
	else
	{
		std::sort(minterms.begin(), minterms.end(), compareMinterms);
		bool first = true;
		for (const minterm_t &minterm : minterms)
		{
			if (first)
				first = false;
			else
				std::cout << " || ";
			printMinterm(minterm, true);
		}
	}
}

Karnaugh_Solution Karnaugh::solve() const
{
	return Karnaugh_Solution::solve(bits, allMinterms, target);
}

bool Karnaugh::processMultiple(const names_t &inputNames, lines_t &lines)
{
	const bits_t bits = inputNames.size();
	
	std::vector<Karnaugh> karnaughs;
	while (!lines.empty())
	{
		karnaughs.push_back(inputNames);
		
		Karnaugh &karnaugh = karnaughs.back();
		if (!karnaugh.loadData(lines))
			return false;
		
		karnaugh.findMinterms();
	}
	
	std::vector<Karnaugh_Solution> karnaugh_solutions;
	karnaugh_solutions.reserve(karnaughs.size());
	for (Karnaugh &karnaugh : karnaughs)
		karnaugh_solutions.emplace_back(karnaugh.solve());
	
	std::vector<minterms_t> bestSolutions;
	bestSolutions.resize(karnaughs.size());
	typename Karnaugh_Solution::OptimizedSolution bestOptimizedSolution;
	std::size_t bestGateScore = SIZE_MAX;
	if (!karnaughs.empty())
	{
		for (std::vector<std::size_t> indexes(karnaughs.size(), 0);;)
		{
			std::vector<const minterms_t*> solutions;
			solutions.reserve(indexes.size());
			for (std::size_t i = 0; i != indexes.size(); ++i)
				solutions.push_back(&karnaugh_solutions[i].getSolutions()[indexes[i]]);
			typename Karnaugh_Solution::OptimizedSolution optimizedSolution = Karnaugh_Solution::optimizeSolutions(solutions);
			if (optimizedSolution.getGateScore() < bestGateScore)
			{
				bestGateScore = optimizedSolution.getGateScore();
				for (std::size_t i = 0; i != indexes.size(); ++i)
					bestSolutions[i] = karnaugh_solutions[i].getSolutions()[indexes[i]];
				bestOptimizedSolution = std::move(optimizedSolution);
			}
			
			for (std::size_t i = indexes.size() - 1;; --i)
			{
				if (++indexes[i] != karnaugh_solutions[i].getSolutions().size())
					break;
				indexes[i] = 0;
				if (i == 0)
					goto all_solutions_checked;
			}
		}
		all_solutions_checked:;
	}
	
	for (std::size_t i = 0; i != karnaughs.size(); ++i)
	{
		const Karnaugh &karnaugh = karnaughs[i];
		const minterms_t &bestSolution = bestSolutions[i];
		
		if (i != 0)
			std::cout << '\n';
		std::cout << "--- " << karnaugh.functionName << " ---\n\n";
		
		std::cout << "goal:\n";
		prettyPrintTable(bits, karnaugh.target, karnaugh.allowed);
		
		Karnaugh_Solution::prettyPrintSolution(bits, bestSolution);
		
		std::cout << "solution:\n";
		karnaugh.printMinterms(bestSolution);
		std::cout << std::endl;
	}
	
	if (!karnaughs.empty())
		std::cout << '\n';
	std::cout << "=== optimized solution ===\n\n";
	names_t functionNames;
	functionNames.reserve(karnaughs.size());
	for (const Karnaugh &karnaugh : karnaughs)
		functionNames.push_back(karnaugh.functionName);
	bestOptimizedSolution.print(bits, std::cout, inputNames, functionNames);
	std::cout << std::flush;
	
	return true;
}
