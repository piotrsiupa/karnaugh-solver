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
	OptimizedSolutions optimizedSolutions;
	
	void printSolutions(const solutions_t &solutions) const;
	void printOptimizedSolution() const;
	
	bool loadData(Input &input);
	solutionses_t makeSolutionses() const;
	void findBestSolutions(const solutionses_t &solutionses, solutions_t &bestSolutions);
	void solve();
	
public:
	static bool solve(Input &input);
};
