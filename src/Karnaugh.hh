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
#include "PrimeImplicant.hh"
#include "PrimeImplicants.hh"


class Karnaugh_Solution;


class Karnaugh
{
	using grayCode_t = std::vector<Minterm>;
	
	static std::size_t nameCount;
	
	std::string functionName;
	Minterms target, dontCares, allowed;
	PrimeImplicants allPrimeImplicants;
	
	Karnaugh() : functionName('f' + std::to_string(nameCount++)) {}
	
	static grayCode_t makeGrayCode(const bits_t bits);
	static void printBits(const Minterm minterm, const bits_t bits);
	static void prettyPrintTable(const Minterms &target, const Minterms &allowed = {});
	
	bool loadMinterms(Minterms &minterms, std::string &line) const;
	bool loadData(lines_t &lines);
	
	void findPrimeImplicants();
	void printPrimeImplicant(const PrimeImplicant primeImplicant, const bool parentheses) const;
	void printPrimeImplicants(PrimeImplicants primeImplicants) const;
	
	Karnaugh_Solution solve() const;
	
	friend class Karnaugh_Solution;
	
public:
	Karnaugh(Karnaugh &&) = default;
	Karnaugh& operator=(Karnaugh &&) = default;
	
	static bool processMultiple(lines_t &lines);
};
