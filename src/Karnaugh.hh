#pragma once

#include <bitset>
#include <cstdint>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "global.hh"


using bits_t = std::uint_fast8_t;


template<bits_t>
class Karnaugh_Solution;


template<bits_t BITS>
class Karnaugh
{
	using table_t = std::bitset<1 << BITS>;
	using number_t = std::uint_fast16_t;
	using grayCode_t = std::vector<number_t>;
	using minterm_t = std::pair<number_t, number_t>;
	using minterms_t = std::vector<minterm_t>;
	using mintermPart_t = std::pair<bits_t, bool>;
	using splitMinterm_t = std::vector<mintermPart_t>;
	
	static std::size_t nameCount;
	
	const names_t &inputNames;
	std::string functionName;
	table_t target, dontCares, acceptable;
	minterms_t allMinterms;
	
	Karnaugh(const names_t &inputNames) : inputNames(inputNames), functionName('f' + std::to_string(nameCount++)) {}
	
	static grayCode_t makeGrayCode(const bits_t bits);
	static void printBits(const number_t number, const bits_t bits);
	static void prettyPrintTable(const table_t &target, const table_t &acceptable = {});
	
	static bool loadTable(table_t &table, std::string &line);
	bool loadData(lines_t &lines);
	
	static constexpr bits_t getOnesCount(const minterm_t minterm) { return __builtin_popcount((minterm.first << 16) | minterm.second); }
	void findMinterms();
	static bool compareMinterms(const minterm_t x, const minterm_t y);
	static splitMinterm_t splitMinterm(const minterm_t &minterm);
	static void printMinterm(std::ostream &o, const names_t &inputNames, const minterm_t minterm);
	void printMinterm(const minterm_t minterm) const;
	void printMinterms(minterms_t minterms) const;
	
	Karnaugh_Solution<BITS> solve() const;
	
	template<bits_t>
	friend class Karnaugh_Solution;
	
public:
	Karnaugh(Karnaugh &&) = default;
	Karnaugh& operator=(Karnaugh &&) = default;
	
	static bool processMultiple(const names_t &inputNames, lines_t &lines);
};
