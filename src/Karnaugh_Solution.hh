#pragma once

#include <vector>

#include "Minterms.hh"
#include "PrimeImplicants.hh"


class Karnaugh_Solution
{
	PrimeImplicants primeImplicants;
	Minterms target;
	std::vector<PrimeImplicants> solutions;
	
	Karnaugh_Solution(const PrimeImplicants &primeImplicants, const Minterms &target);
	
	void solve();
	
public:
	static Karnaugh_Solution solve(const PrimeImplicants &primeImplicants, const Minterms &target);
	
	static void prettyPrintSolution(const PrimeImplicants &solution);
	const std::vector<PrimeImplicants>& getSolutions() const { return solutions; }
};
