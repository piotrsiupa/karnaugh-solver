#include "./Karnaughs.hh"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>

#include "options.hh"
#include "Progress.hh"


void Karnaughs::printHumanBestSolutions() const
{
	for (std::size_t i = 0; i != karnaughs.size(); ++i)
	{
		std::cout << "--- " << karnaughs[i].getFunctionName() << " ---\n";
		if (options::outputFormat.getValue() == options::OutputFormat::LONG_HUMAN)
			std::cout << '\n';
		karnaughs[i].printHumanSolution(bestSolutions[i]);
		std::cout << '\n';
	}
}

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

void Karnaughs::printHumanOptimizedSolution() const
{
	const Names functionNames = gatherFunctionNames();
	optimizedSolutions.printHuman(std::cout, functionNames);
}

void Karnaughs::printVerilogBestSolutions(const Names &functionNames) const
{
	if (!karnaughs.empty())
	{
		for (std::size_t i = 0; i != karnaughs.size(); ++i)
		{
			std::cout << "\tassign ";
			functionNames.printVerilogName(std::cout, i);
			std::cout << " = ";
			karnaughs[i].printVerilogSolution(bestSolutions[i]);
			std::cout << ";\n";
		}
		std::cout << "\t\n";
	}
}

void Karnaughs::printVerilogOptimizedSolution(const Names &functionNames) const
{
	optimizedSolutions.printVerilog(std::cout, functionNames);
}

void Karnaughs::printVhdlOptimizedSolution(const Names &functionNames) const
{
	optimizedSolutions.printVhdl(std::cout, functionNames);
}

void Karnaughs::printVhdlBestSolutions(const Names &functionNames) const
{
	std::cout << "begin\n";
	if (!karnaughs.empty())
	{
		std::cout << "\t\n";
		for (std::size_t i = 0; i != karnaughs.size(); ++i)
		{
			std::cout << '\t';
			functionNames.printVhdlName(std::cout, i);
			std::cout << " <= ";
			karnaughs[i].printVhdlSolution(bestSolutions[i]);
			std::cout << ";\n";
		}
		std::cout << "\t\n";
	}
}

void Karnaughs::printName()
{
	if (options::name.getValue())
		std::cout << *options::name.getValue();
	else if (::inputFilePath)
		std::cout << std::filesystem::path(*::inputFilePath).stem().string();
	else
		std::cout << "Karnaugh";
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
		auto substeps = progress.makeCountingSubsteps(static_cast<Progress::completion_t>(solutions.size()));
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

void Karnaughs::printHuman()
{
	const bool bestSolutionsVisible = options::skipOptimization.isRaised() || options::outputFormat.getValue() != options::OutputFormat::SHORT_HUMAN;
	if (bestSolutionsVisible)
		printHumanBestSolutions();
	if (!options::skipOptimization.isRaised())
	{
		if (options::outputFormat.getValue() != options::OutputFormat::SHORT_HUMAN)
			std::cout << "=== optimized solution ===\n\n";
		printHumanOptimizedSolution();
		std::cout << std::flush;
	}
}

void Karnaughs::printVerilog()
{
	std::cout << "module ";
	printName();
	std::cout << " (\n";
	if (!::inputNames.isEmpty())
	{
		std::cout << "\tinput wire";
		::inputNames.printVerilogNames(std::cout);
		std::cout << '\n';
	}
	const Names functionNames = gatherFunctionNames();
	if (!karnaughs.empty())
	{
		std::cout << "\toutput wire";
		functionNames.printVerilogNames(std::cout);
		std::cout << '\n';
	}
	std::cout << ");\n";
	std::cout << "\t\n";
	if (options::skipOptimization.isRaised())
		printVerilogBestSolutions(functionNames);
	else
		printVerilogOptimizedSolution(functionNames);
	std::cout << "endmodule" << std::endl;
}

void Karnaughs::printVhdl()
{
	std::cout << "library IEEE;\n"
			"use IEEE.std_logic_1164.all;\n";
	std::cout << '\n';
	std::cout << "entity ";
	printName();
	std::cout << " is\n";
	const Names functionNames = gatherFunctionNames();
	if (!::inputNames.isEmpty() || !karnaughs.empty())
	{
		std::cout << "\tport(\n";
		if (!::inputNames.isEmpty())
		{
			std::cout << "\t\t";
			::inputNames.printVhdlNames(std::cout);
			std::cout << " : in ";
			::inputNames.printVhdlType(std::cout);
			if (!karnaughs.empty())
				std::cout << ';';
			std::cout << '\n';
		}
		if (!karnaughs.empty())
		{
			std::cout << "\t\t";
			functionNames.printVhdlNames(std::cout);
			std::cout << " : out ";
			functionNames.printVhdlType(std::cout);
			std::cout << '\n';
		}
		std::cout << "\t);\n";
	}
	std::cout << "end ";
	printName();
	std::cout << ";\n";
	std::cout << '\n';
	std::cout << "architecture behavioural of ";
	printName();
	std::cout << " is\n";
	if (options::skipOptimization.isRaised())
		printVhdlBestSolutions(functionNames);
	else
		printVhdlOptimizedSolution(functionNames);
	std::cout << "end behavioural;\n";
}

void Karnaughs::print()
{
	switch (options::outputFormat.getValue())
	{
	case options::OutputFormat::LONG_HUMAN:
	case options::OutputFormat::HUMAN:
	case options::OutputFormat::SHORT_HUMAN:
		printHuman();
		break;
	case options::OutputFormat::VERILOG:
		printVerilog();
		break;
	case options::OutputFormat::VHDL:
		printVhdl();
		break;
	}
}
