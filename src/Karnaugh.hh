#pragma once

#include <cstdint>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "global.hh"


using bits_t = std::uint_fast8_t;
constexpr bits_t maxBits = 16;


class Karnaugh_Solution;


class Karnaugh
{
	using number_t = std::uint_fast16_t;
	using numbers_t = std::set<number_t>;
	using grayCode_t = std::vector<number_t>;
	using minterm_t = std::pair<number_t, number_t>;
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
	
	static constexpr bits_t getOnesCount(const minterm_t minterm) { return __builtin_popcount(minterm.first | minterm.second) - __builtin_popcount(minterm.first & minterm.second); }
	void findMinterms();
	static bool compareMinterms(const minterm_t x, const minterm_t y);
	static splitMinterm_t splitMinterm(const bits_t bits, const minterm_t &minterm);
	static void printMinterm(const bits_t bits, std::ostream &o, const names_t &inputNames, const minterm_t minterm, const bool parentheses);
	void printMinterm(const minterm_t minterm, const bool parentheses) const;
	void printMinterms(minterms_t minterms) const;
	
	Karnaugh_Solution solve() const;
	
	friend class Karnaugh_Solution;
	
public:
	Karnaugh(Karnaugh &&) = default;
	Karnaugh& operator=(Karnaugh &&) = default;
	
	static bool processMultiple(const names_t &inputNames, lines_t &lines);
};
