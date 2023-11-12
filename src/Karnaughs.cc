#include "./Karnaughs.hh"

#include <algorithm>
#include <iostream>
#include <limits>

#include "options.hh"
#include "Progress.hh"


void Karnaughs::printBestSolutions() const
{
	for (std::size_t i = 0; i != karnaughs.size(); ++i)
	{
		if (i != 0)
			std::cout << '\n';
		std::cout << "--- " << karnaughs[i].getFunctionName() << " ---\n\n";
		karnaughs[i].printSolution(bestSolutions[i]);
	}
}

void Karnaughs::printOptimizedSolution() const
{
	std::vector<std::string> functionNames;
	functionNames.reserve(karnaughs.size());
	for (const Karnaugh &karnaugh : karnaughs)
		functionNames.push_back(karnaugh.getFunctionName());
	optimizedSolutions.print(std::cout, functionNames);
}

bool Karnaughs::loadData(Input &input)
{
	while (true)
	{
		if (options::prompt.getValue())
			std::cerr << "Either end the input or enter a name of the next function (optional) or a list of its minterms:\n";
		if (input.hasError())
			return false;
		if (input.isEmpty())
			break;
		karnaughs.push_back({});
		if (!karnaughs.back().loadData(input))
			return false;
	}
	return true;
}

Karnaughs::solutionses_t Karnaughs::makeSolutionses() const
{
	solutionses_t solutionses;
	solutionses.reserve(karnaughs.size());
	for (const Karnaugh &karnaugh : karnaughs)
		solutionses.emplace_back(karnaugh.solve());
	return solutionses;
}

void Karnaughs::findBestNonOptimizedSolutions(const solutionses_t &solutionses)
{
	Progress progress(Progress::Stage::OPTIMIZING, "Electing the best solutions", solutionses.size());
	bestSolutions.reserve(solutionses.size());
	for (const solutions_t &solutions : solutionses)
	{
		progress.step();
		auto substeps = progress.makeCountingSubsteps(solutions.size());
		using score_t = std::size_t;
		const Implicants *bestSolution = nullptr;
		score_t bestScore = std::numeric_limits<score_t>::max();
		for (const Implicants &solution : solutions)
		{
			substeps.substep();
			if (solution.empty())
			{
				bestSolution = &solution;
				break;
			}
			Implicant::mask_t falseBits = 0;
			score_t score = (solution.size() - 1) * 2;
			for (const Implicant &implicant : solution)
			{
				score += (implicant.getBitCount() - 1) * 2;
				falseBits |= implicant.getFalseBits();
			}
			score += std::bitset<32>(falseBits).count();
			if (score < bestScore)
			{
				bestScore = score;
				bestSolution = &solution;
			}
		}
		bestSolutions.push_back(*bestSolution);
	}
}

void Karnaughs::findBestOptimizedSolutions(const solutionses_t &solutionses)
{
	if (solutionses.empty())
		return;
	
	bestSolutions.resize(solutionses.size());
	std::size_t bestGateScore = SIZE_MAX;
	Progress::steps_t steps = 1;
	if (options::status.getValue())
		for (const solutions_t &solutions : solutionses)
			steps *= solutions.size();
	Progress progress(Progress::Stage::OPTIMIZING, "Eliminating common subexpressions", steps);
	for (std::vector<std::size_t> indexes(solutionses.size(), 0);;)
	{
		progress.step();
		std::vector<const Implicants*> solutions;
		solutions.reserve(indexes.size());
		for (std::size_t i = 0; i != indexes.size(); ++i)
			solutions.push_back(&solutionses[i][indexes[i]]);
		OptimizedSolutions currentOSs(solutions, progress);
		
		if (currentOSs.getGateScore() < bestGateScore)
		{
			bestGateScore = currentOSs.getGateScore();
			for (std::size_t i = 0; i != indexes.size(); ++i)
				bestSolutions[i] = solutionses[i][indexes[i]];
			optimizedSolutions = std::move(currentOSs);
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

void Karnaughs::findBestSolutions(const solutionses_t &solutionses)
{
	if (options::skipOptimization.isRaised())
		findBestNonOptimizedSolutions(solutionses);
	else
		findBestOptimizedSolutions(solutionses);
}	

void Karnaughs::solve()
{
	const std::vector<solutions_t> solutionses = makeSolutionses();
	findBestSolutions(solutionses);
}

void Karnaughs::print()
{
	printBestSolutions();
	if (!options::skipOptimization.isRaised())
	{
		if (!bestSolutions.empty())
			std::cout << '\n';
		std::cout << "=== optimized solution ===\n\n";
		printOptimizedSolution();
		std::cout << std::flush;
	}
}
