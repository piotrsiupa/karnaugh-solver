#pragma once

#include <cstddef>
#include <ostream>
#include <vector>

#include "./Karnaugh.hh"
#include "global.hh"
#include "Minterm.hh"
#include "Minterms.hh"
#include "PrimeImplicant.hh"
#include "PrimeImplicants.hh"


class Karnaugh_Solution
{
public:
	struct OptimizedSolution
	{
		Minterm negatedInputs = 0;
		std::vector<std::pair<PrimeImplicant, std::vector<std::size_t>>> products;
		std::vector<std::vector<std::size_t>> sums;
		std::vector<std::size_t> finalSums;
		
		void print(std::ostream &o, const names_t &functionNames) const;
		std::size_t getNotCount() const { return __builtin_popcount(negatedInputs); }
		std::size_t getAndCount() const { std::size_t andCount = 0; for (const auto &product : products) andCount += std::max(std::size_t(1), product.first.getBitCount() + product.second.size()) - 1; return andCount; }
		std::size_t getOrCount() const { std::size_t orCount = 0; for (const auto &sum : sums) orCount += sum.size() - 1; return orCount; }
		std::size_t getGateScore() const { return getNotCount() + 2 * getAndCount() + 2 * getOrCount(); }
	};
	
private:
	PrimeImplicants primeImplicants;
	Minterms target;
	std::vector<PrimeImplicants> solutions;
	
	Karnaugh_Solution(const PrimeImplicants &primeImplicants, const Minterms &target);
	
	void solve();
	
public:
	static Karnaugh_Solution solve(const PrimeImplicants &primeImplicants, const Minterms &target);
	
	static void prettyPrintSolution(const PrimeImplicants &solution);
	const std::vector<PrimeImplicants>& getSolutions() const { return solutions; }
	
	static OptimizedSolution optimizeSolutions(const std::vector<const PrimeImplicants*> &solutions);
};
