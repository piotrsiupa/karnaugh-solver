#include "./Karnaughs.hh"

#include <algorithm>
#include <iostream>
#include <limits>

#include "options.hh"
#include "Progress.hh"


bool Karnaughs::shouldFunctionNamesBeUsed() const
{
	for (const Karnaugh &karnaugh : karnaughs)
		if (karnaugh.hasCustomName())
			return true;
	return false;
}

Names Karnaughs::gatherFunctionNames() const
{
	Names::names_t functionNames;
	functionNames.reserve(karnaughs.size());
	for (const Karnaugh &karnaugh : karnaughs)
		functionNames.emplace_back(karnaugh.getFunctionName());
	return {shouldFunctionNamesBeUsed(), std::move(functionNames), "o"};
}

bool Karnaughs::loadData(Input &input)
{
	while (true)
	{
		if (options::prompt)
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
	Progress progress(Progress::Stage::OPTIMIZING, "Electing the best solutions", solutionses.size(), true);
	bestSolutions.reserve(solutionses.size());
	for (const Solutions &solutions : solutionses)
	{
		progress.step();
		auto progressStep = progress.makeCountingStepHelper(static_cast<Progress::completion_t>(solutions.size()));
		using score_t = std::size_t;
		const Solution *bestSolution = nullptr;
		score_t bestScore = std::numeric_limits<score_t>::max();
		for (const Solution &solution : solutions)
		{
			progressStep.substep();
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
			score += std::bitset<::maxBits>(falseBits).count();
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
	if (options::status)
		for (const Solutions &solutions : solutionses)
			steps *= solutions.size();
#ifdef NDEBUG
	Progress progress(Progress::Stage::OPTIMIZING, "Eliminating common subexpressions", steps, true);
#else
	Progress progress(Progress::Stage::OPTIMIZING, "Eliminating common subexpressions", steps * 2, false);
#endif
	for (std::vector<std::size_t> indexes(solutionses.size(), 0);;)
	{
		progress.step();
		std::vector<const Solution*> solutions;
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
	if (options::skipOptimization)
		findBestNonOptimizedSolutions(solutionses);
	else
		findBestOptimizedSolutions(solutionses);
}	

void Karnaughs::solve()
{
	const std::vector<Solutions> solutionses = makeSolutionses();
	findBestSolutions(solutionses);
}

OutputComposer Karnaughs::makeOutputComposer()
{
	return OutputComposer(gatherFunctionNames(), karnaughs, bestSolutions, options::skipOptimization ? nullptr : &optimizedSolutions);
}
