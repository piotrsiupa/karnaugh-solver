#pragma once

#include <cstdint>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "global.hh"
#include "Minterm.hh"
#include "PrimeImplicant.hh"


class Karnaugh_Solution;


class Karnaugh
{
	using number_t = Minterm;
	using numbers_t = std::set<number_t>;
	using grayCode_t = std::vector<number_t>;
	using minterm_t = PrimeImplicant;
	using minterms_t = std::vector<minterm_t>;
	using mintermPart_t = std::pair<bits_t, bool>;
	using splitMinterm_t = std::vector<mintermPart_t>;
	
	static std::size_t nameCount;
	
	const names_t &inputNames;
	std::string functionName;
	bits_t bits;
	numbers_t target, dontCares, allowed;
	minterms_t allMinterms;
	
	Karnaugh(const names_t &inputNames, const bits_t bits) : inputNames(inputNames), functionName('f' + std::to_string(nameCount++)), bits(bits) {}
	
	static grayCode_t makeGrayCode(const bits_t bits);
	static void printBits(const number_t number, const bits_t bits);
	static void prettyPrintTable(const bits_t bits, const numbers_t &target, const numbers_t &allowed = {});
	
	bool loadNumbers(numbers_t &numbers, std::string &line) const;
	bool loadData(lines_t &lines);
	
	void findMinterms();
	static splitMinterm_t splitMinterm(const bits_t bits, const minterm_t &minterm);
	void printMinterm(const minterm_t minterm, const bool parentheses) const;
	void printMinterms(minterms_t minterms) const;
	
	Karnaugh_Solution solve() const;
	
	friend class Karnaugh_Solution;
	
public:
	Karnaugh(Karnaugh &&) = default;
	Karnaugh& operator=(Karnaugh &&) = default;
	
	static bool processMultiple(const names_t &inputNames, lines_t &lines);
};
