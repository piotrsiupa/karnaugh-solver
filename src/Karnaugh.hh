#pragma once

#include <string>
#include <vector>

#include "global.hh"
#include "Implicant.hh"
#include "Implicants.hh"
#include "Input.hh"
#include "Minterm.hh"
#include "Minterms.hh"


class Karnaugh
{
public:
	using solutions_t = std::vector<Implicants>;
	
private:
	using grayCode_t = std::vector<Minterm>;
	
	static std::size_t nameCount;
	
	std::string functionName;
	Minterms targetMinterms, allowedMinterms;
	
	static grayCode_t makeGrayCode(const bits_t bitCount);
	static void printBits(const Minterm minterm, const bits_t bitCount);
	static void prettyPrintTable(const Minterms &target, const Minterms &allowed = {});
	void prettyPrintTable() const;
	static void prettyPrintSolution(const Implicants &solution);
	void printPrimeImplicant(const Implicant implicant, const bool parentheses) const;
	void printPrimeImplicants(Implicants implicants) const;
	
	bool loadMinterms(Minterms &minterms, Input &input) const;
#ifndef NDEBUG
	void validate(const solutions_t &solutions) const;
#endif
	
public:
	Karnaugh() : functionName('f' + std::to_string(nameCount++)) {}
	Karnaugh(Karnaugh &&) = default;
	Karnaugh& operator=(Karnaugh &&) = default;
	
	std::string getFunctionName() const { return functionName; }
	
	bool loadData(Input &input);
	solutions_t solve() const;
	
	void printSolution(const Implicants &solution) const;
};
