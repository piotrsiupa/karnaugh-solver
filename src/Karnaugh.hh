#pragma once

#include <cstdint>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "global.hh"
#include "Minterm.hh"
#include "Minterms.hh"
#include "OptimizedSolution.hh"
#include "PrimeImplicant.hh"
#include "PrimeImplicants.hh"


class Karnaugh
{
	using grayCode_t = std::vector<Minterm>;
	using karnaughs_t = std::vector<Karnaugh>;
	using solutions_t = std::vector<PrimeImplicants>;
	using solutionses_t = std::vector<solutions_t>;
	
	static std::size_t nameCount;
	
	std::string functionName;
	Minterms targetMinterms, allowedMinterms;
	
	Karnaugh() : functionName('f' + std::to_string(nameCount++)) {}
	
	static grayCode_t makeGrayCode(const bits_t bits);
	static void printBits(const Minterm minterm, const bits_t bits);
	static void prettyPrintTable(const Minterms &target, const Minterms &allowed = {});
	void prettyPrintTable() const;
	static void prettyPrintSolution(const PrimeImplicants &solution);
	void printPrimeImplicant(const PrimeImplicant primeImplicant, const bool parentheses) const;
	void printPrimeImplicants(PrimeImplicants primeImplicants) const;
	void printSolution(const PrimeImplicants &solution) const;
	static void printSolutions(const karnaughs_t &karnaughs, const solutions_t &solutions);
	static void printOptimizedSolution(const karnaughs_t &karnaughs, const OptimizedSolution &optimizedSolution);
	
	bool loadMinterms(Minterms &minterms, std::string &line) const;
	bool loadData(lines_t &lines);
	static bool loadKarnaughs(lines_t &lines, karnaughs_t &karnaughs);
	PrimeImplicants findPrimeImplicants() const;
	solutions_t solve() const;
	static solutionses_t makeSolutionses(const karnaughs_t &karnaughs);
	static void findBestSolutions(const solutionses_t &solutionses, solutions_t &bestSolutions, OptimizedSolution &bestOptimizedSolution);
	
public:
	Karnaugh(Karnaugh &&) = default;
	Karnaugh& operator=(Karnaugh &&) = default;
	
	static bool solveAll(lines_t &lines);
};
