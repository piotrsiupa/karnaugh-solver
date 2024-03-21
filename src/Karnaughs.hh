#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Solution.hh"
#include "Input.hh"
#include "Names.hh"
#include "Karnaugh.hh"
#include "OptimizedSolutions.hh"
#include "Solutions.hh"


// This could be split into 2 classes (`Karnaughs` and `Solutions`) so `&&` functions make more sense but it is coupled enough for me to not bother.
class Karnaughs
{
	using karnaughs_t = std::vector<Karnaugh>;
	using solutionses_t = std::vector<Solutions>;
	
	karnaughs_t karnaughs;
	Solutions bestSolutions;
	OptimizedSolutions optimizedSolutions;
	
	[[nodiscard]] bool shouldFunctionNamesBeUsed() const;
	Names gatherFunctionNames() const;
	void printGraphInputs() const;
	std::pair<bool, bool> checkForUsedConstants() const;
	void printGraphConstants() const;
	void printGraphRoots() const;
	void printHumanBestSolutions() const;
	void printHumanOptimizedSolution() const;
	void printGraphBestSolutions() const;
	void printGraphOptimizedSolution() const;
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
	void printGraph();
	void printVerilog();
	void printVhdl();
	void printCpp();
	void printMath();
	void printGateCost();
	void print();
};
