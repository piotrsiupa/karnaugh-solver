#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "./Karnaugh.hh"


template<bits_t BITS>
class Karnaugh_Solution
{
	using minterm_t = typename Karnaugh<BITS>::minterm_t;
	using minterms_t = typename Karnaugh<BITS>::minterms_t;
	using table_t = typename Karnaugh<BITS>::table_t;
	using number_t = typename Karnaugh<BITS>::number_t;
	using mintermTables_t = std::vector<table_t>;
	
	minterms_t minterms;
	table_t target;
	const table_t &dontCares;
	mintermTables_t mintermTables;
	minterms_t solution;
	
	Karnaugh_Solution(const minterms_t &minterms, const table_t &target, const table_t &dontCares);
	
	static table_t createMintermTable(const minterm_t minterm);
	void createMintermTables();
	minterms_t removeEssentials();
	void solve();
	
public:
	static Karnaugh_Solution solve(const minterms_t &allMinters, const table_t &target, const table_t &dontCares);
	
	bool isSolutionValid() const { return solution.size() < 2 || solution[0] != solution[1]; }
	void prettyPrintSolution() const;
	const minterms_t& getSolution() const { return solution; }
};
