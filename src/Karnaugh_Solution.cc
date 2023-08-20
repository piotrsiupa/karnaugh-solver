#include "./Karnaugh.hh"
#include "./Karnaugh_Solution.hh"

#include <iostream>

#include "Minterm.hh"
#include "PetricksMethod.hh"
#include "PrimeImplicant.hh"


Karnaugh_Solution::Karnaugh_Solution(const PrimeImplicants &primeImplicants, const Minterms &target) :
	primeImplicants(primeImplicants),
	target(target)
{
}

void Karnaugh_Solution::solve()
{
	solutions = PetricksMethod<Minterm, PrimeImplicant>::solve(std::move(target), std::move(primeImplicants));
}

void Karnaugh_Solution::prettyPrintSolution(const PrimeImplicants &solution)
{
	Minterms minterms;
	for (const auto &minterm : solution)
	{
		const auto x = minterm.findMinterms();
		minterms.insert(x.cbegin(), x.end());
	}
	std::cout << "best fit:\n";
	Karnaugh::prettyPrintTable(minterms);
}

Karnaugh_Solution Karnaugh_Solution::solve(const PrimeImplicants &primeImplicants, const Minterms &target)
{
	Karnaugh_Solution karnaugh_solver(primeImplicants, target);
	karnaugh_solver.solve();
	return karnaugh_solver;
}
