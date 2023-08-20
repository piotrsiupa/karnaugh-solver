#include "./Karnaugh.hh"

#include <algorithm>
#include <cctype>
#include <iostream>

#include "input.hh"
#include "QuineMcCluskey.hh"


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

void Karnaugh::prettyPrintTable() const
{
	return prettyPrintTable(targetMinterms, allowedMinterms);
}

void Karnaugh::prettyPrintSolution(const PrimeImplicants &solution)
{
	Minterms minterms;
	for (const auto &minterm : solution)
	{
		const auto x = minterm.findMinterms();
		minterms.insert(x.cbegin(), x.end());
	}
	prettyPrintTable(minterms);
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
		for (const PrimeImplicant &primeImplicant : primeImplicants)
		{
			if (&primeImplicant != &primeImplicants.front())
				std::cout << " || ";
			printPrimeImplicant(primeImplicant, true);
		}
	}
}

void Karnaugh::printSolution(const PrimeImplicants &solution) const
{
	std::cout << "--- " << functionName << " ---\n\n";
	
	std::cout << "goal:\n";
	prettyPrintTable();
	
	std::cout << "best fit:\n";
	prettyPrintSolution(solution);
	
	std::cout << "solution:\n";
	printPrimeImplicants(solution);
	std::cout << std::endl;
}

void Karnaugh::printSolutions(const karnaughs_t &karnaughs, const solutions_t &solutions)
{
	for (std::size_t i = 0; i != karnaughs.size(); ++i)
	{
		if (i != 0)
			std::cout << '\n';
		karnaughs[i].printSolution(solutions[i]);
	}
}

void Karnaugh::printOptimizedSolution(const karnaughs_t &karnaughs, const OptimizedSolution &optimizedSolution)
{
	names_t functionNames;
	functionNames.reserve(karnaughs.size());
	for (const Karnaugh &karnaugh : karnaughs)
		functionNames.push_back(karnaugh.functionName);
	optimizedSolution.print(std::cout, functionNames);
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
	
	if (!loadMinterms(targetMinterms, lines.front()))
		return false;
	lines.pop_front();
	
	Minterms dontCareMinterms;
	if (!loadMinterms(dontCareMinterms, lines.front()))
		return false;
	lines.pop_front();
	
	allowedMinterms.insert(targetMinterms.cbegin(), targetMinterms.cend());
	allowedMinterms.insert(dontCareMinterms.cbegin(), dontCareMinterms.cend());
	
	return true;
}

bool Karnaugh::loadKarnaughs(lines_t &lines, karnaughs_t &karnaughs)
{
	while (!lines.empty())
	{
		karnaughs.push_back({});
		if (!karnaughs.back().loadData(lines))
			return false;
	}
	return true;
}

Karnaugh::solutions_t Karnaugh::solve() const
{
	return QuineMcCluskey().solve(allowedMinterms, targetMinterms);
}

Karnaugh::solutionses_t Karnaugh::makeSolutionses(const karnaughs_t &karnaughs)
{
	solutionses_t solutionses;
	solutionses.reserve(karnaughs.size());
	for (const Karnaugh &karnaugh : karnaughs)
		solutionses.emplace_back(karnaugh.solve());
	return solutionses;
}

void Karnaugh::findBestSolutions(const solutionses_t &solutionses, solutions_t &bestSolutions, OptimizedSolution &bestOptimizedSolution)
{
	if (solutionses.empty())
		return;
	
	bestSolutions.resize(solutionses.size());
	std::size_t bestGateScore = SIZE_MAX;
	for (std::vector<std::size_t> indexes(solutionses.size(), 0);;)
	{
		std::vector<const PrimeImplicants*> solutions;
		solutions.reserve(indexes.size());
		for (std::size_t i = 0; i != indexes.size(); ++i)
			solutions.push_back(&solutionses[i][indexes[i]]);
		OptimizedSolution optimizedSolution = OptimizedSolution::create(solutions);
		
		if (optimizedSolution.getGateScore() < bestGateScore)
		{
			bestGateScore = optimizedSolution.getGateScore();
			for (std::size_t i = 0; i != indexes.size(); ++i)
				bestSolutions[i] = solutionses[i][indexes[i]];
			bestOptimizedSolution = std::move(optimizedSolution);
		}
		
		for (std::size_t i = indexes.size() - 1;; --i)
		{
			if (++indexes[i] != solutionses[i].size())
				break;
			indexes[i] = 0;
			if (i == 0)
				return;
		}
	}
}

bool Karnaugh::solveAll(lines_t &lines)
{
	karnaughs_t karnaughs;
	if (!loadKarnaughs(lines, karnaughs))
		return false;
	
	const std::vector<solutions_t> solutionses = makeSolutionses(karnaughs);
	
	solutions_t solutions;
	OptimizedSolution optimizedSolution;
	findBestSolutions(solutionses, solutions, optimizedSolution);
	
	printSolutions(karnaughs, solutions);
	if (!solutions.empty())
		std::cout << '\n';
	std::cout << "=== optimized solution ===\n\n";
	printOptimizedSolution(karnaughs, optimizedSolution);
	std::cout << std::flush;
	
	return true;
}
