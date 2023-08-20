#pragma once

#include <vector>

#include "Karnaugh.hh"
#include "OptimizedSolution.hh"
#include "PrimeImplicants.hh"


class Karnaughs
{
	using karnaughs_t = std::vector<Karnaugh>;
	using solutions_t = std::vector<PrimeImplicants>;
	using solutionses_t = std::vector<solutions_t>;
	
	karnaughs_t karnaughs;
	
	void printSolutions(const solutions_t &solutions) const;
	void printOptimizedSolution(const OptimizedSolution &optimizedSolution) const;
	
	solutionses_t makeSolutionses() const;
	static void findBestSolutions(const solutionses_t &solutionses, solutions_t &bestSolutions, OptimizedSolution &bestOptimizedSolution);
	
public:
	bool loadData(lines_t &lines);
	void solve() const;
};
