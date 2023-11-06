#pragma once

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
	
	void printBestSolutions() const;
	void printOptimizedSolution() const;
	
	solutionses_t makeSolutionses() const;
	void findBestSolutions(const solutionses_t &solutionses);
	
public:
	bool loadData(Input &input);
	void solve();
	void print();
};
