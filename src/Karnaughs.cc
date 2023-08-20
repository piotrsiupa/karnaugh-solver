#include "./Karnaughs.hh"

#include <iostream>


void Karnaughs::printSolutions(const solutions_t &solutions) const
{
	for (std::size_t i = 0; i != karnaughs.size(); ++i)
	{
		if (i != 0)
			std::cout << '\n';
		std::cout << "--- " << karnaughs[i].getFunctionName() << " ---\n\n";
		karnaughs[i].printSolution(solutions[i]);
	}
}

void Karnaughs::printOptimizedSolution(const OptimizedSolution &optimizedSolution) const
{
	names_t functionNames;
	functionNames.reserve(karnaughs.size());
	for (const Karnaugh &karnaugh : karnaughs)
		functionNames.push_back(karnaugh.getFunctionName());
	optimizedSolution.print(std::cout, functionNames);
}

Karnaughs::solutionses_t Karnaughs::makeSolutionses() const
{
	solutionses_t solutionses;
	solutionses.reserve(karnaughs.size());
	for (const Karnaugh &karnaugh : karnaughs)
		solutionses.emplace_back(karnaugh.solve());
	return solutionses;
}

void Karnaughs::findBestSolutions(const solutionses_t &solutionses, solutions_t &bestSolutions, OptimizedSolution &bestOptimizedSolution)
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

bool Karnaughs::loadData(lines_t &lines)
{
	while (!lines.empty())
	{
		karnaughs.push_back({});
		if (!karnaughs.back().loadData(lines))
			return false;
	}
	return true;
}

void Karnaughs::solve() const
{
	const std::vector<solutions_t> solutionses = makeSolutionses();
	
	solutions_t solutions;
	OptimizedSolution optimizedSolution;
	findBestSolutions(solutionses, solutions, optimizedSolution);
	
	printSolutions(solutions);
	if (!solutions.empty())
		std::cout << '\n';
	std::cout << "=== optimized solution ===\n\n";
	printOptimizedSolution(optimizedSolution);
	std::cout << std::flush;
}
