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
			for (Minterm j = 0; j != unsigned(1) << i; ++j)
				grayCode.push_back(grayCode[j ^ ((1 << i) - 1)] | (1 << i));
	}
	return grayCode;
}

void Karnaugh::printBits(const Minterm minterm, const bits_t bits)
{
	for (bits_t i = bits; i != 0; --i)
		std::cout << ((minterm & (1 << (i - 1))) != 0 ? '1' : '0');
}

void Karnaugh::prettyPrintTable(const Minterms &target, const Minterms &allowed)
{
	const bits_t vBits = (::bits + 1) / 2;
	const bits_t hBits = ::bits / 2;
	const grayCode_t vGrayCode = makeGrayCode(vBits);
	const grayCode_t hGrayCode = makeGrayCode(hBits);
	for (int i = 0; i != vBits; ++i)
		std::cout << ' ';
	std::cout << ' ';
	for (const Minterm y : hGrayCode)
	{
		printBits(y, hBits);
		std::cout << ' ';
	}
	std::cout << '\n';
	for (const Minterm x : vGrayCode)
	{
		printBits(x, std::max(bits_t(1), vBits));
		std::cout << ' ';
		bool first = true;
		for (int i = 0; i != (hBits - 1) / 2; ++i)
			std::cout << ' ';
		for (const Minterm y : hGrayCode)
		{
			if (first)
				first = false;
			else
				for (int i = 0; i != hBits; ++i)
					std::cout << ' ';
			const Minterm minterm = (x << hBits) | y;
			std::cout << (target.find(minterm) != target.cend() ? 'T' : (allowed.find(minterm) != allowed.cend() ? '-' : 'F'));
		}
		std::cout << '\n';
	}
	std::cout << std::endl;
}

bool Karnaugh::loadMinterms(Minterms &minterms, std::string &line) const
{
	for (const std::string &string : readStrings(line))
	{
		try
		{
			const auto n = std::stoi(string);
			if (n < 0)
			{
				std::cerr << '"' << string << "\" is negative!\n";
				return false;
			}
			else if (n >= (1 << ::bits))
			{
				std::cerr << '"' << string << "\" is too big!\n";
				return false;
			}
			minterms.insert(n);
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
	if (!loadMinterms(target, lines.front()))
		return false;
	lines.pop_front();
	if (!loadMinterms(dontCares, lines.front()))
		return false;
	lines.pop_front();
	allowed.insert(target.cbegin(), target.cend());
	allowed.insert(dontCares.cbegin(), dontCares.cend());
	return true;
}

void Karnaugh::findPrimeImplicants()
{
	std::vector<std::pair<PrimeImplicant, bool>> oldPrimeImplicants;
	for (const Minterm &minterm : allowed)
		oldPrimeImplicants.emplace_back(PrimeImplicant{minterm}, false);
	std::set<PrimeImplicant> newPrimeImplicants;
	while (!oldPrimeImplicants.empty())
	{
		for (auto iter = oldPrimeImplicants.begin(); iter != oldPrimeImplicants.end(); ++iter)
		{
			for (auto jiter = std::next(iter); jiter != oldPrimeImplicants.end(); ++jiter)
			{
				if (PrimeImplicant::areMergeable(iter->first, jiter->first))
				{
					newPrimeImplicants.insert(PrimeImplicant::merge(iter->first, jiter->first));
					iter->second = true;
					jiter->second = true;
				}
			}
		}
		for (const auto &oldPrimeImplicant : oldPrimeImplicants)
			if (!oldPrimeImplicant.second)
				allPrimeImplicants.push_back(oldPrimeImplicant.first);
		oldPrimeImplicants.clear();
		oldPrimeImplicants.reserve(newPrimeImplicants.size());
		for (const auto &newPrimeImplicant : newPrimeImplicants)
			oldPrimeImplicants.emplace_back(newPrimeImplicant, false);
		newPrimeImplicants.clear();
	}
}

void Karnaugh::printPrimeImplicant(const PrimeImplicant primeImplicant, const bool parentheses) const
{
	return primeImplicant.print(std::cout, parentheses);
}

void Karnaugh::printPrimeImplicants(PrimeImplicants primeImplicants) const
{
	if (primeImplicants.size() == 1)
	{
		printPrimeImplicant(primeImplicants.front(), false);
	}
	else
	{
		std::sort(primeImplicants.begin(), primeImplicants.end());
		bool first = true;
		for (const PrimeImplicant &primeImplicant : primeImplicants)
		{
			if (first)
				first = false;
			else
				std::cout << " || ";
			printPrimeImplicant(primeImplicant, true);
		}
	}
}

Karnaugh_Solution Karnaugh::solve() const
{
	return Karnaugh_Solution::solve(allPrimeImplicants, target);
}

bool Karnaugh::processMultiple(lines_t &lines)
{
	std::vector<Karnaugh> karnaughs;
	while (!lines.empty())
	{
		karnaughs.push_back({});
		
		Karnaugh &karnaugh = karnaughs.back();
		if (!karnaugh.loadData(lines))
			return false;
		
		karnaugh.findPrimeImplicants();
	}
	
	std::vector<Karnaugh_Solution> karnaugh_solutions;
	karnaugh_solutions.reserve(karnaughs.size());
	for (Karnaugh &karnaugh : karnaughs)
		karnaugh_solutions.emplace_back(karnaugh.solve());
	
	std::vector<PrimeImplicants> bestSolutions;
	bestSolutions.resize(karnaughs.size());
	typename Karnaugh_Solution::OptimizedSolution bestOptimizedSolution;
	std::size_t bestGateScore = SIZE_MAX;
	if (!karnaughs.empty())
	{
		for (std::vector<std::size_t> indexes(karnaughs.size(), 0);;)
		{
			std::vector<const PrimeImplicants*> solutions;
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
		const PrimeImplicants &bestSolution = bestSolutions[i];
		
		if (i != 0)
			std::cout << '\n';
		std::cout << "--- " << karnaugh.functionName << " ---\n\n";
		
		std::cout << "goal:\n";
		prettyPrintTable(karnaugh.target, karnaugh.allowed);
		
		Karnaugh_Solution::prettyPrintSolution(bestSolution);
		
		std::cout << "solution:\n";
		karnaugh.printPrimeImplicants(bestSolution);
		std::cout << std::endl;
	}
	
	if (!karnaughs.empty())
		std::cout << '\n';
	std::cout << "=== optimized solution ===\n\n";
	names_t functionNames;
	functionNames.reserve(karnaughs.size());
	for (const Karnaugh &karnaugh : karnaughs)
		functionNames.push_back(karnaugh.functionName);
	bestOptimizedSolution.print(std::cout, functionNames);
	std::cout << std::flush;
	
	return true;
}
