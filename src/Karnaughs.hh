#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Solution.hh"
#include "Input.hh"
#include "Names.hh"
#include "Karnaugh.hh"
#include "OptimizedSolutions.hh"
#include "Solutions.hh"


class Karnaughs
{
	using karnaughs_t = std::vector<Karnaugh>;
	using solutionses_t = std::vector<Solutions>;
	
	karnaughs_t karnaughs;
	Solutions bestSolutions;
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
	
	void solve();
	void printHuman();
	void printVerilog();
	void printVhdl();
	void printCpp();
	void printMath();
	void print();
};
