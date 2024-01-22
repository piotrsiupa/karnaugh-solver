#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Implicants.hh"
#include "Input.hh"
#include "Names.hh"
#include "Karnaugh.hh"
#include "OptimizedSolutions.hh"


class Karnaughs
{
	using karnaughs_t = std::vector<Karnaugh>;
	using solutions_t = std::vector<Implicants>;
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
	
	solutionses_t makeSolutionses() &&;  // This function is `&&` as a reminder the it removes some data in the process (to save memory) and because of that it cannot be called twice.
	void findBestNonOptimizedSolutions(const solutionses_t &solutionses);
	void findBestOptimizedSolutions(const solutionses_t &solutionses);
	void findBestSolutions(const solutionses_t &solutionses);
	
public:
	[[nodiscard]] bool loadData(Input &input);
	void solve() &&;  // This function is `&&` as a reminder the it removes some data in the process (to save memory) and because of that it cannot be called twice.
	void printHuman();
	void printVerilog();
	void printVhdl();
	void printCpp();
	void printMath();
	void print();
};
