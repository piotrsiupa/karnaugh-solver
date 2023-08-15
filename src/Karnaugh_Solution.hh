#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
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
	
public:
	struct OptimizedSolution
	{
		number_t negatedInputs = 0;
		std::vector<std::pair<minterm_t, std::vector<std::size_t>>> products;
		std::vector<std::vector<std::size_t>> sums;
		std::vector<std::size_t> finalSums;
		
		void print(std::ostream &o, const names_t &inputNames, const names_t &functionNames) const;
		std::size_t getNotCount() const { return __builtin_popcount(negatedInputs); }
		std::size_t getAndCount() const { std::size_t andCount = 0; for (const auto &product : products) andCount += std::max(std::size_t(1), Karnaugh<BITS>::getOnesCount(product.first) + product.second.size()) - 1; return andCount; }
		std::size_t getOrCount() const { std::size_t orCount = 0; for (const auto &sum : sums) orCount += sum.size() - 1; return orCount; }
		std::size_t getGateScore() const { return getNotCount() + 2 * getAndCount() + 2 * getOrCount(); }
	};
	
private:
	minterms_t minterms;
	table_t target;
	const table_t &dontCares;
	mintermTables_t mintermTables;
	std::vector<minterms_t> solutions;
	
	Karnaugh_Solution(const minterms_t &minterms, const table_t &target, const table_t &dontCares);
	
	static table_t createMintermTable(const minterm_t minterm);
	void createMintermTables();
	minterms_t removeEssentials();
	void solve();
	
public:
	static Karnaugh_Solution solve(const minterms_t &allMinters, const table_t &target, const table_t &dontCares);
	
	static void prettyPrintSolution(const minterms_t &solution);
	const std::vector<minterms_t>& getSolutions() const { return solutions; }
	
	static OptimizedSolution optimizeSolutions(const std::vector<const minterms_t*> &solutions);
};
