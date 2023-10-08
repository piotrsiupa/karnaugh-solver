#pragma once

#include <vector>

#include "Karnaugh.hh"
#include "OptimizedSolutions.hh"
#include "PrimeImplicants.hh"


class Karnaughs
{
	using karnaughs_t = std::vector<Karnaugh>;
	using solutions_t = std::vector<PrimeImplicants>;
	using solutionses_t = std::vector<solutions_t>;
	
	karnaughs_t karnaughs;
	OptimizedSolutions optimizedSolutions;
	
	void printSolutions(const solutions_t &solutions) const;
	void printOptimizedSolution() const;
	
	bool loadData(lines_t &lines);
	solutionses_t makeSolutionses() const;
	void findBestSolutions(const solutionses_t &solutionses, solutions_t &bestSolutions);
	void solve();
	
public:
	static bool solve(lines_t &lines);
};
