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
	
	const names_t &inputNames;
	std::string functionName;
	bits_t bits;
	Minterms target, dontCares, allowed;
	PrimeImplicants allPrimeImplicants;
	
	Karnaugh(const names_t &inputNames, const bits_t bits) : inputNames(inputNames), functionName('f' + std::to_string(nameCount++)), bits(bits) {}
	
	static grayCode_t makeGrayCode(const bits_t bits);
	static void printBits(const Minterm minterm, const bits_t bits);
	static void prettyPrintTable(const bits_t bits, const Minterms &target, const Minterms &allowed = {});
	
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
	
	static bool processMultiple(const names_t &inputNames, lines_t &lines);
};
