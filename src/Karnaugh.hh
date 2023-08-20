#pragma once

#include <string>
#include <vector>

#include "global.hh"
#include "Minterm.hh"
#include "Minterms.hh"
#include "PrimeImplicant.hh"
#include "PrimeImplicants.hh"


class Karnaugh
{
public:
	using solutions_t = std::vector<PrimeImplicants>;
	
private:
	using grayCode_t = std::vector<Minterm>;
	
	static std::size_t nameCount;
	
	std::string functionName;
	Minterms targetMinterms, allowedMinterms;
	
	static grayCode_t makeGrayCode(const bits_t bits);
	static void printBits(const Minterm minterm, const bits_t bits);
	static void prettyPrintTable(const Minterms &target, const Minterms &allowed = {});
	void prettyPrintTable() const;
	static void prettyPrintSolution(const PrimeImplicants &solution);
	void printPrimeImplicant(const PrimeImplicant primeImplicant, const bool parentheses) const;
	void printPrimeImplicants(PrimeImplicants primeImplicants) const;
	
	bool loadMinterms(Minterms &minterms, std::string &line) const;
	
public:
	Karnaugh() : functionName('f' + std::to_string(nameCount++)) {}
	Karnaugh(Karnaugh &&) = default;
	Karnaugh& operator=(Karnaugh &&) = default;
	
	std::string getFunctionName() const { return functionName; }
	
	bool loadData(lines_t &lines);
	solutions_t solve() const;
	
	void printSolution(const PrimeImplicants &solution) const;
};
