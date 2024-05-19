#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
	[[nodiscard]] static std::string getName();
	
	solutionses_t makeSolutionses() const;
	void findBestNonOptimizedSolutions(const solutionses_t &solutionses);
	void findBestOptimizedSolutions(const solutionses_t &solutionses);
	void findBestSolutions(const solutionses_t &solutionses);
	
public:
	[[nodiscard]] bool loadData(Input &input);
	
	void solve();
	void print(std::ostream &o);
};
