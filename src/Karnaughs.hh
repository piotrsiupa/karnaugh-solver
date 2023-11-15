#pragma once

#include <string_view>
#include <vector>

#include "Implicants.hh"
#include "Input.hh"
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
	
	std::vector<std::string_view> gatherFunctionNames() const;
	void printHumanBestSolutions() const;
	void printHumanOptimizedSolution() const;
	void printVerilogBestSolutions() const;
	void printVerilogOptimizedSolution() const;
	
	solutionses_t makeSolutionses() const;
	void findBestNonOptimizedSolutions(const solutionses_t &solutionses);
	void findBestOptimizedSolutions(const solutionses_t &solutionses);
	void findBestSolutions(const solutionses_t &solutionses);
	
public:
	bool loadData(Input &input);
	void solve();
	void printHuman();
	void printVerilog();
	void print();
};
