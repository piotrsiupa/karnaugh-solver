#pragma once

#include <numeric>
#include <string>
#include <string_view>
#include <vector>

#include "GateScore.hh"
#include "Solution.hh"
#include "Input.hh"
#include "Names.hh"
#include "Karnaugh.hh"
#include "OptimizedSolutions.hh"


class Karnaughs : public GateScore
{
	using karnaughs_t = std::vector<Karnaugh>;
	using solutions_t = std::vector<Solution>;
	using solutionses_t = std::vector<solutions_t>;
	
	karnaughs_t karnaughs;
	solutions_t bestSolutions;
	OptimizedSolutions optimizedSolutions;
	
	[[nodiscard]] bool shouldFunctionNamesBeUsed() const;
	Names gatherFunctionNames() const;
	void printHumanBestSolutions() const;
	void printHumanOptimizedSolution() const;
	void printVerilogBestSolutions(const Names &functionNames) const;
	void printVerilogOptimizedSolution(const Names &functionNames) const;
	void printVhdlBestSolutions(const Names &functionNames) const;
	void printVhdlOptimizedSolution(const Names &functionNames) const;
	void printCppBestSolutions(const Names &functionNames) const;
	void printCppOptimizedSolution(const Names &functionNames) const;
	[[nodiscard]] static std::string getName();
	[[nodiscard]] bool areInputsUsed() const;
	
	solutionses_t makeSolutionses() const;
	void findBestNonOptimizedSolutions(const solutionses_t &solutionses);
	void findBestOptimizedSolutions(const solutionses_t &solutionses);
	void findBestSolutions(const solutionses_t &solutionses);
	
public:
	[[nodiscard]] bool loadData(Input &input);
	
	std::size_t getNotCount() const final { return std::accumulate(bestSolutions.cbegin(), bestSolutions.cend(), 0, [](const std::size_t acc, const Solution &solution){ return solution.getNotCount() + acc; }); }
	std::size_t getAndCount() const final { return std::accumulate(bestSolutions.cbegin(), bestSolutions.cend(), 0, [](const std::size_t acc, const Solution &solution){ return solution.getAndCount() + acc; }); }
	std::size_t getOrCount() const final { return std::accumulate(bestSolutions.cbegin(), bestSolutions.cend(), 0, [](const std::size_t acc, const Solution &solution){ return solution.getOrCount() + acc; }); }
	
	void solve();
	void printHuman();
	void printVerilog();
	void printVhdl();
	void printCpp();
	void printMath();
	void print();
};
